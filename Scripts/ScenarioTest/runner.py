# Copyright Woogle. All Rights Reserved.
"""테스트 창(게임 프로세스) 안에서 도는 공통 실행기. run.py가 시나리오 뒤에 붙여 보낸다.

시나리오가 정하는 것: TITLE, AI_ON, STEPS, evaluate(ctx), 선택 WATCH_TAGS. run.py가 정하는 것: OUT(결과 파일 경로).
프레임마다 상태를 기록하고, 단계마다 화면에 안내를 띄우며, 끝나면 판정을 화면과 OUT에 남긴다.
캐릭터는 키로 가리킨다: "p" 플레이어, "b" 보스, "e0"·"e1"… 보스를 뺀 적(준비할 때 플레이어에 가까운 순서).
"""
import unreal, json, time, math, builtins, ctypes, os
from ctypes import wintypes

# PIE 밖에서 액터 속성을 알림과 함께 쓰면 엔진이 컴포넌트를 다시 등록하고, 그때 ASC가 어빌리티를 모두 지운다.
NEVER = unreal.PropertyAccessChangeNotifyMode.NEVER

# 화면 안내 색
GRAY, YELLOW, DONE = (0.7, 0.7, 0.7), (1, 0.9, 0.2), (0.6, 0.85, 0.6)
PASS, FAIL = (0.3, 1, 0.4), (1, 0.4, 0.3)


def _tag(name):
    t = unreal.GameplayTag()
    t.import_text('(TagName="%s")' % name)
    return t


class Ctx:
    """시나리오 단계 함수가 받는 도우미. 캐릭터는 키("p", "b", "e0"…)로 가리킨다."""

    def __init__(self):
        self.rows, self.notes, self.fps = [], [], {}
        self.t0 = self.warm = None
        self.step_index = 0
        self.done = False
        self.window_locked = False
        self.chars, self.asc, self.aset, self.brains = {}, {}, {}, []

    # 화면 안내. 줄마다 key가 있고, 같은 key의 글자는 제자리에서 바뀐다. 줄은 처음 띄운 순서대로 위에서부터 쌓인다.
    def banner(self, text, key="note", color=(1, 1, 1), secs=600.0):
        unreal.SystemLibrary.print_string(self.world, text, True, False, unreal.LinearColor(color[0], color[1], color[2], 1), secs, key)

    def _step_line(self, i, state):
        prefix, color = {"wait": ("[대기]", GRAY), "run": ("[진행]", YELLOW), "done": ("[완료]", DONE)}[state]
        self.banner("%s %d. %s" % (prefix, i + 1, STEPS[i][1]), "step%d" % i, color)

    # 입력: 플레이어의 개발용 입력 속성(DevPressInput 등)에 넣는다.
    def press(self, name):
        self.player.set_editor_property("DevPressInput", _tag(name), NEVER)

    def mash(self, name=None):
        self.player.set_editor_property("DevMashInput", _tag(name) if name else unreal.GameplayTag(), NEVER)

    def hold(self, name=None):
        self.player.set_editor_property("DevHoldInput", _tag(name) if name else unreal.GameplayTag(), NEVER)

    def move(self, x, y):
        """x 오른쪽, y 앞. 회피 방향은 회피 키보다 한 프레임 이상 먼저 넣는다."""
        self.player.set_editor_property("DevMoveInput", unreal.Vector2D(x, y), NEVER)

    def clear_inputs(self):
        self.mash()
        self.hold()
        self.move(0, 0)

    def console(self, command):
        """콘솔 명령을 실행한다(예: 테스트용 콘솔 명령 AgKillBoss)."""
        unreal.SystemLibrary.execute_console_command(self.world, command, self.pc)

    # 자원(어트리뷰트). 이 창의 메모리만 바꾸고 DesignData JSON은 건드리지 않는다.
    # 값만 바꾸고 변경 이벤트는 일으키지 않으므로, HP를 0으로 해도 사망하지 않고 HUD도 바뀌지 않는다.
    def attr(self, who, name):
        return self.aset[who].get_editor_property(name).current_value

    def set_attr(self, who, name, value):
        aset = self.aset[who]
        v = aset.get_editor_property(name)
        v.set_editor_property("base_value", value)
        v.set_editor_property("current_value", value)
        aset.set_editor_property(name, v, NEVER)

    def has(self, who, tag):
        return self.asc[who].has_matching_gameplay_tag(_tag(tag))

    def event(self, who, tag, instigator=None):
        """who에게 게임플레이 이벤트를 보낸다(공격자 없는 피격 반응 등). instigator는 보낸 쪽 캐릭터 키."""
        payload = unreal.GameplayEventData()
        payload.set_editor_property("event_tag", _tag(tag))
        payload.set_editor_property("target", self.chars[who])
        if instigator:
            payload.set_editor_property("instigator", self.chars[instigator])
        unreal.AbilitySystemLibrary.send_gameplay_event_to_actor(self.chars[who], _tag(tag), payload)

    def ai(self, on):
        """적 AI를 모두 켜거나 끈다."""
        for brain in self.brains:
            if on:
                brain.start_logic()
            else:
                brain.stop_logic("qa")

    def face(self, who, dist):
        """who에서 dist cm 떨어진 곳(지금 방향 그대로)에 플레이어를 두고, 플레이어와 카메라가 who를 보게 한다."""
        b, p = self.chars[who].get_actor_location(), self.player.get_actor_location()
        dx, dy = p.x - b.x, p.y - b.y
        n = math.hypot(dx, dy) or 1.0
        loc = unreal.Vector(b.x + dx / n * dist, b.y + dy / n * dist, p.z)
        self.player.set_actor_location(loc, False, True)
        yaw = math.degrees(math.atan2(b.y - loc.y, b.x - loc.x))
        self.player.set_actor_rotation(unreal.Rotator(roll=0, pitch=0, yaw=yaw), True)
        self.pc.set_control_rotation(unreal.Rotator(roll=0, pitch=-12, yaw=yaw))

    def face_boss(self, dist):
        self.face("b", dist)

    def pos(self):
        return self.player.get_actor_location()

    def forward(self):
        return self.player.get_actor_forward_vector()

    def fov(self):
        return self.pc.get_editor_property("player_camera_manager").get_fov_angle()

    # 적의 지각: 적 AI가 플레이어를 바라보는지(포커스), 기억하는지, 지금 보는지, 몸 정면에서 플레이어까지의 각도.
    # 죽은 적은 컨트롤러가 빙의를 풀어 모두 False가 된다.
    def focus(self, who):
        controller = self.chars[who].get_controller()
        return bool(controller) and controller.get_focus_actor() == self.player

    def known(self, who):
        perception = self._perception(who)
        return bool(perception) and self.player in perception.get_known_perceived_actors(None)

    def seen(self, who):
        perception = self._perception(who)
        return bool(perception) and self.player in perception.get_currently_perceived_actors(None)

    def angle(self, who):
        e, p, f = self.chars[who].get_actor_location(), self.player.get_actor_location(), self.chars[who].get_actor_forward_vector()
        dx, dy = p.x - e.x, p.y - e.y
        n = math.hypot(dx, dy) or 1.0
        return math.degrees(math.acos(max(-1.0, min(1.0, (dx * f.x + dy * f.y) / n))))

    def _perception(self, who):
        controller = self.chars[who].get_controller()
        return controller.get_component_by_class(unreal.AIPerceptionComponent) if controller else None

    def montage(self, who):
        """지금 몽타주 이름. 기본 섹션(Default)이 아닌 섹션이면 "AM_Player_BasicAttack:Attack3"처럼 섹션을 붙인다."""
        anim = self.chars[who].get_editor_property("mesh").get_anim_instance()
        m = anim.get_current_active_montage() if anim else None
        if not m:
            return None
        section = str(anim.montage_get_current_section(m))
        return m.get_name() if section in ("Default", "None") else "%s:%s" % (m.get_name(), section)

    def seq(self, who, t_from=0.0, t_to=1e9):
        """t_from~t_to초 사이에 재생된 몽타주 이름(montage와 같은 형식)을 차례로(연속 중복 없이)."""
        return [m for _, m in self.timeline(who, t_from, t_to) if m]

    def timeline(self, who, t_from=0.0, t_to=1e9):
        """t_from~t_to초 사이에 몽타주가 바뀐 시각과 이름. 몽타주가 끝나면 이름이 None인 항목이 들어간다."""
        out = []
        for r in self.rows:
            m = r.get(who + "m")
            if t_from <= r["t"] <= t_to and (not out or out[-1][1] != m):
                out.append((r["t"], m))
        return out

    def spans(self, who, tag):
        """WATCH_TAGS의 태그가 붙어 있던 구간 [(시작, 끝)]. 끝까지 붙어 있으면 끝은 마지막 기록 시각이다."""
        out, start = [], None
        for r in self.rows:
            on = tag in r["tags"].get(who, ())
            if on and start is None:
                start = r["t"]
            elif not on and start is not None:
                out.append((start, r["t"]))
                start = None
        if start is not None:
            out.append((start, self.rows[-1]["t"]))
        return out


ctx = Ctx()
builtins._ag_qa_ctx = ctx


def _lock_window():
    """이 창이 사람의 마우스·키보드 입력을 받지 않게 Windows에서 비활성으로 만든다. 클릭해도 포커스를 가져가거나
    마우스를 잡지 않는다. 창이 아직 없으면 False."""
    user32 = ctypes.windll.user32
    found = []

    @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    def each(hwnd, _):
        pid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value == os.getpid() and user32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True

    user32.EnumWindows(each, 0)
    for hwnd in found:
        user32.EnableWindow(hwnd, False)
    return bool(found)


def _setup():
    for world in unreal.ObjectIterator(unreal.World):
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        if pc and pc.get_controlled_pawn():
            break
    else:
        return False
    ctx.world, ctx.pc = world, pc
    ctx.player = pc.get_controlled_pawn()
    boss_class = unreal.load_class(None, "/Script/Ag.AgBossCharacter")
    enemies = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/Ag.AgEnemyCharacter")))
    bosses = [e for e in enemies if unreal.MathLibrary.class_is_child_of(e.get_class(), boss_class)]
    others = sorted((e for e in enemies if e not in bosses), key=lambda e: e.get_distance_to(ctx.player))
    ctx.boss = bosses[0] if bosses else None
    ctx.chars = {"p": ctx.player}
    if ctx.boss:
        ctx.chars["b"] = ctx.boss
    ctx.chars.update(("e%d" % i, e) for i, e in enumerate(others))
    for key, actor in ctx.chars.items():
        ctx.asc[key] = actor.get_component_by_class(unreal.AbilitySystemComponent)
        ctx.aset[key] = unreal.find_object(None, actor.get_path_name() + ".AttributeSet")
    ctx.brains = [a.get_controller().get_component_by_class(unreal.BrainComponent) for k, a in ctx.chars.items() if k != "p" and a.get_controller()]
    # 패드처럼 창을 거치지 않는 입력도 막는다. 확인용 입력(DevPressInput 등)은 이 입력 경로를 거치지 않는다.
    ctx.player.disable_input(pc)
    # 준비: 적 AI를 멈추고, 맵을 연 직후의 적 공격이 끝나고 프레임이 안정되기를 기다린다.
    ctx.ai(False)
    # 화면에 제목, 준비, 시험 항목 전체를 한 줄씩 띄운다. 판정은 끝나고 그 아래에 붙는다.
    ctx.banner(TITLE, "title")
    ctx.banner("[진행] 준비: 보스 동작이 끝나고 프레임이 안정되기를 기다림", "warm", YELLOW)
    for i in range(len(STEPS)):
        ctx._step_line(i, "wait")
    ctx.warm = {"start": time.time(), "times": []}
    return True


def _warmed_up():
    now = time.time()
    w = ctx.warm
    w["times"] = [x for x in w["times"] + [now] if now - x <= 2.0]
    fps = sum(1 for x in w["times"] if now - x <= 1.0)
    # 창마다 프레임 수준이 달라 정해진 fps가 아니라, 직전 1초와 비슷해졌는지로 안정을 본다.
    steady = now - w["start"] >= 2.0 and abs(fps - (len(w["times"]) - fps)) <= max(2, fps // 10)
    idle = all(ctx.montage(k) is None for k in ctx.chars)
    if (idle and steady) or now - w["start"] > 20.0:
        ctx.notes.append("준비 %.1f초, 시작 직전 %d fps" % (now - w["start"], fps))
        ctx.banner("[완료] 준비 (%.1f초, %d fps)" % (now - w["start"], fps), "warm", DONE)
        if AI_ON:
            ctx.ai(True)
        ctx.t0 = now
        return True
    return False


def _record(t):
    """한 프레임의 기록. 플레이어는 p로 시작하는 값, 적은 그 키(b, e0…)로 시작하는 값이다."""
    velocity = ctx.player.get_velocity()
    row = {"t": round(t, 3), "pm": ctx.montage("p"), "pHP": ctx.attr("p", "HP"), "pSP": ctx.attr("p", "SP"),
           "pMP": ctx.attr("p", "MP"), "pUP": ctx.attr("p", "UP"), "guard": ctx.has("p", "State.Guarding"),
           "pdead": ctx.has("p", "State.Dead"), "pvz": round(velocity.z, 1), "pspeed": round(math.hypot(velocity.x, velocity.y), 1),
           "fov": round(ctx.fov(), 2)}
    for k in ctx.chars:
        if k == "p":
            continue
        row.update({k + "m": ctx.montage(k), k + "HP": ctx.attr(k, "HP"), k + "PP": ctx.attr(k, "PP"),
                    k + "dead": ctx.has(k, "State.Dead"), k + "focus": ctx.focus(k), k + "known": ctx.known(k),
                    k + "seen": ctx.seen(k), k + "angle": round(ctx.angle(k), 1)})
    watch = globals().get("WATCH_TAGS", ())
    row["tags"] = {k: [tag for tag in watch if ctx.has(k, tag)] for k in ctx.chars}
    return row


def _finish(error=None):
    unreal.unregister_slate_post_tick_callback(ctx.handle)
    try:
        ctx.clear_inputs()
        ctx.ai(False)  # 판정 뒤에 적이 계속 공격하지 않게 한다.
    except Exception:
        pass
    ok, items = False, [(False, "오류: " + error)] if error else []
    if not error:
        try:
            ok, lines = evaluate(ctx)
            # 판정 문구는 글자만 주거나, 항목별 판정을 붙여 (통과 여부, 글자)로 준다.
            items = [(None, l) if isinstance(l, str) else (bool(l[0]), l[1]) for l in lines]
        except Exception as e:
            ok = False
            error = "판정 오류: %s: %s" % (type(e).__name__, e)
            items = [(False, error)]
    verdict = "실행 실패" if error else "통과" if ok else "테스트 실패"
    lines = ["%s %s" % ("-" if i is None else "[통과]" if i else "[실패]", text) for i, text in items]
    try:
        if not error and 0 < ctx.step_index <= len(STEPS):
            ctx._step_line(ctx.step_index - 1, "done")
        ctx.banner("판정: %s" % verdict, "verdict", PASS if ok else FAIL)
        for n, (i, text) in enumerate(items):
            ctx.banner("  " + lines[n], "verdict%d" % n, PASS if (ok if i is None else i) else FAIL)
    except Exception:
        pass
    full = [v for k, v in ctx.fps.items() if k < max(ctx.fps)] if ctx.fps else []
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump({"title": TITLE, "verdict": verdict, "lines": lines, "notes": ctx.notes,
                   "montages": {k: ctx.seq(k) for k in ctx.chars},
                   "seconds": ctx.rows[-1]["t"] if ctx.rows else 0, "frames": len(ctx.rows),
                   "fps_min": min(full) if full else 0, "rows": ctx.rows}, f, ensure_ascii=False)
    ctx.done = True


def _tick(dt):
    try:
        if not ctx.window_locked:
            ctx.window_locked = _lock_window()
        if ctx.warm is None:
            _setup()
            return
        if ctx.t0 is None:
            _warmed_up()
            return
        t = time.time() - ctx.t0
        ctx.fps[int(t)] = ctx.fps.get(int(t), 0) + 1
        ctx.rows.append(_record(t))
        while ctx.step_index < len(STEPS) and t >= STEPS[ctx.step_index][0]:
            if ctx.step_index > 0:
                ctx._step_line(ctx.step_index - 1, "done")
            ctx._step_line(ctx.step_index, "run")
            fn = STEPS[ctx.step_index][2]
            ctx.step_index += 1
            if fn:
                fn(ctx)
        if ctx.step_index >= len(STEPS):
            _finish()
    except Exception as e:
        _finish("%s: %s" % (type(e).__name__, e))


ctx.handle = unreal.register_slate_post_tick_callback(_tick)
print("runner started:", TITLE)
