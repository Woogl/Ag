# Copyright Woogle. All Rights Reserved.
"""테스트 창(게임 프로세스) 안에서 도는 공통 실행기. run.py가 시나리오 뒤에 붙여 보낸다.

시나리오가 정하는 것: TITLE, AI_ON, STEPS, evaluate(ctx), 선택 WATCH_TAGS. run.py가 정하는 것: OUT(결과 파일 경로).
프레임마다 상태를 기록하고, 단계마다 화면에 안내를 띄우며, 끝나면 판정을 화면과 OUT에 남긴다.
캐릭터는 키로 가리킨다: "p" 플레이어, "b" 보스, "e0"·"e1"… 보스를 뺀 적(준비할 때 플레이어에 가까운 순서).
레벨이 바뀌면(Start, Restart, 타이틀로) 새 레벨을 다시 준비하고, 그동안 시간은 멈춘다.
"""
import unreal, json, time, math, ctypes, os, inspect, traceback
from ctypes import wintypes

# 보낸 글에서 실행기가 시작하는 줄. 그 앞은 시나리오이고, 시나리오가 글의 맨 앞이라 줄 번호가 시나리오 파일과 같다.
_RUNNER_LINE = inspect.currentframe().f_lineno

# PIE 밖에서 액터 속성을 알림과 함께 쓰면 엔진이 컴포넌트를 다시 등록하고, 그때 ASC가 어빌리티를 모두 지운다.
NEVER = unreal.PropertyAccessChangeNotifyMode.NEVER

GRAY, YELLOW, DONE = (0.7, 0.7, 0.7), (1, 0.9, 0.2), (0.6, 0.85, 0.6)
PASS, FAIL = (0.3, 1, 0.4), (1, 0.4, 0.3)


def _tag(name):
    t = unreal.GameplayTag()
    t.import_text('(TagName="%s")' % name)
    return t


# 사람이 누르는 키(IMC_Default). 시나리오는 입력 태그 이름으로 누르고, 키 매핑이 바뀌면 이 표도 바꾼다.
KEYS = {"Input.Attack": "LeftMouseButton", "Input.Guard": "RightMouseButton", "Input.Dodge": "LeftShift",
        "Input.Jump": "SpaceBar", "Input.Skill1": "One", "Input.Skill2": "Two", "Input.Skill3": "Three",
        "Input.Skill4": "Four", "Input.Ultimate": "Q", "Input.Interact": "F", "Input.LockOn": "MiddleMouseButton",
        "Input.Potion": "R"}

# UAgUILayout의 레이어 스택 이름. 앞일수록 위에 그려지고, 사람은 가장 위 레이어의 화면만 조작한다.
UI_LAYERS = ("ModalStack", "MenuStack", "HUDStack")


def _valid(obj):
    try:
        return unreal.SystemLibrary.is_valid(obj)
    except TypeError:
        return False  # 지워진 객체를 엔진 GC가 거두면 그 파이썬 참조는 null이 되어 함수에 넘길 수 없다.


def _ui_chain(widget):
    """위젯에서 위로 올라가며 (위젯, 바로 위 위젯 안에서의 순서). 위젯 블루프린트의 위젯 트리 맨 위에서는 그 트리를 가진
    위젯으로, 목록 항목에서는 그 목록으로 이어진다."""
    while widget:
        parent, index = widget.get_parent(), 0
        if parent:
            index = parent.get_child_index(widget)
        else:
            parent, index = _owning_list(widget)
        if not parent:
            outer = widget.get_outer()
            if outer and not isinstance(outer, unreal.Widget):
                outer = outer.get_outer()  # 위젯 트리의 주인
            parent = outer if isinstance(outer, unreal.Widget) else None
        yield widget, index
        widget = parent


def _owning_list(widget):
    """목록 항목이면 (목록, 항목 순서), 아니면 (None, 0)."""
    try:
        view = unreal.UserListEntryLibrary.get_owning_list_view(widget)
    except TypeError:
        return None, 0  # 목록 항목 인터페이스가 없는 위젯
    return (view, unreal.UserObjectListEntryLibrary.get_list_item_index(widget)) if view else (None, 0)


def _short_class(obj):
    name = obj.get_class().get_name()
    return name[:-2] if name.endswith("_C") else name


class Ctx:
    """시나리오 단계 함수가 받는 도우미. 캐릭터는 키("p", "b", "e0"…)로 가리킨다."""

    def __init__(self):
        self.rows, self.notes, self.fps = [], [], {}
        self.t0 = self.warm = self.spawn_wait_start = None
        self.paused_at = 0.0  # 레벨이 바뀌어 멈춘 시나리오 시각
        self.levels = 0
        self.step_index = 0
        self.window_locked = False
        self.world = self.pc = self.player = self.map = None
        self.chars, self.asc, self.aset, self.stacks = {}, {}, {}, {}
        self.keys = []  # 기록에 나온 캐릭터 키(처음 나온 순서)
        self._reset_inputs()

    def _reset_inputs(self):
        self.down, self.move_keys, self.taps = set(), set(), []
        self.mashing = self.holding = None
        self.mouse = (0.0, 0.0)

    # 화면 안내. 줄마다 key가 있고, 같은 key의 글자는 제자리에서 바뀐다. 줄은 처음 띄운 순서대로 위에서부터 쌓인다.
    def banner(self, text, key="note", color=(1, 1, 1), secs=600.0):
        world = self.world if _valid(self.world) else None
        unreal.SystemLibrary.print_string(world, text, True, False, unreal.LinearColor(color[0], color[1], color[2], 1), secs, key)

    def _step_line(self, i, state):
        prefix, color = {"wait": ("[대기]", GRAY), "run": ("[진행]", YELLOW), "done": ("[완료]", DONE)}[state]
        self.banner("%s %d. %s" % (prefix, i + 1, STEPS[i][1]), "step%d" % i, color)

    # 입력: 사람이 누르는 키와 마우스를 테스트용 콘솔 명령(AgPressKey 등)으로 넣는다. 넣은 입력은 다음 프레임에 처리된다.
    def press(self, name):
        """키를 누르고 다음 프레임에 뗀다."""
        self._key(KEYS[name], True)
        self.taps.append(KEYS[name])

    def mash(self, name=None):
        """프레임마다 누름과 뗌을 번갈아 넣는다. 이름 없이 부르면 멈춘다."""
        if self.mashing in self.down:
            self._key(self.mashing, False)
        self.mashing = KEYS[name] if name else None
        if self.mashing:
            self._key(self.mashing, True)

    def hold(self, name=None):
        """키를 누른 채 둔다. 이름 없이 부르면 뗀다."""
        if self.holding:
            self._key(self.holding, False)
        self.holding = KEYS[name] if name else None
        if self.holding:
            self._key(self.holding, True)

    def move(self, x, y):
        """x 오른쪽, y 앞(WASD라 -1, 0, 1). 회피 방향은 회피 키보다 한 프레임 이상 먼저 넣는다."""
        keys = {k for k, on in (("D", x > 0), ("A", x < 0), ("W", y > 0), ("S", y < 0)) if on}
        for k in self.move_keys - keys:
            self._key(k, False)
        for k in keys - self.move_keys:
            self._key(k, True)
        self.move_keys = keys

    def look(self, x, y=0.0):
        """마우스를 프레임마다 x만큼 오른쪽, y만큼 위로 움직인다. look(0)이면 멈춘다."""
        self.mouse = (x, y)

    def clear_inputs(self):
        self.mash()
        self.hold()
        self.move(0, 0)
        self.look(0)
        for k in list(self.down):
            self._key(k, False)
        self.taps = []

    def _key(self, key, down):
        self.console(("AgPressKey " if down else "AgReleaseKey ") + key)
        (self.down.add if down else self.down.discard)(key)

    def _pump(self):
        """프레임마다 단계보다 먼저: 지난 프레임에 press로 누른 키를 떼고, 연타 키를 번갈아 넣고, 마우스를 움직인다."""
        for k in self.taps:
            self._key(k, False)
        self.taps = []
        if self.mashing:
            self._key(self.mashing, self.mashing not in self.down)
        if self.mouse != (0, 0):
            self.console("AgMoveMouse %g %g" % self.mouse)

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
        """적 AI를 모두 켜거나 끈다. 사람이 쓰는 콘솔 명령과 같은 경로라, 이미 그 상태인 적과 죽은 적은 건드리지 않는다."""
        self.console("AgStartAI" if on else "AgStopAI")

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

    # 화면과 버튼: 사람이 조작하는 화면은 가장 위 레이어의 화면이고, 버튼은 그 화면 안에 보이는 것만 누른다.
    def screen(self):
        """가장 위 레이어의 화면 블루프린트 이름(예: WBP_Title). 화면이 없으면 None."""
        top = self._top_screen()
        return _short_class(top) if top else None

    def buttons(self):
        """가장 위 화면에 보이는 버튼의 이름을 위젯 트리 순서(세로 상자는 위에서 아래, 가로 상자는 왼쪽에서 오른쪽)로."""
        return [self._button_name(b) for b in self._shown_buttons()]

    def click(self, name, index=0):
        """가장 위 화면에 보이는 그 이름의 버튼 중 index번째(buttons 순서, 0부터)를 마우스로 클릭한 것처럼 누른다.
        버튼이 꺼져 있어 클릭이 듣지 않으면 False. 그 버튼이 보이지 않으면 오류로 멈춘다."""
        found = [b for b in self._shown_buttons() if self._button_name(b) == name]
        if index >= len(found):
            raise LookupError("화면 %s에 '%s' 버튼 %d번째가 보이지 않음 (보이는 버튼: %s)"
                              % (self.screen(), name, index, ", ".join(self.buttons()) or "없음"))
        button = found[index]
        if not button.is_interaction_enabled():
            return False
        # 마우스 클릭과 Enter가 거치는 CommonUI 버튼의 클릭 처리. 블루프린트에 노출되지 않아 이름으로 부른다.
        button.call_method("HandleButtonClicked")
        return True

    def focused(self):
        """가장 위 화면에서 포커스를 가진 버튼의 이름. 없으면 None."""
        for b in self._shown_buttons():
            if b.has_any_user_focus() or b.has_focused_descendants():
                return self._button_name(b)
        return None

    def stack(self, layer):
        """그 레이어("MenuStack" 등)의 화면 스택. 일시정지 메뉴처럼 UI 액션(Esc)으로 여는 화면은 키 입력 명령이 닿지 않으므로
        stack("MenuStack").bp_add_widget(화면 클래스)로 띄운다."""
        if not self.stacks:
            # 위젯은 레벨이 아닌 게임 인스턴스에 달려 옛 레벨의 레이아웃이 GC 전까지 남고, 그 주인 컨트롤러는 새 컨트롤러로
            # 읽힌다. 그래서 뷰포트에 붙은 레이아웃의 스택만 고른다.
            for s in unreal.ObjectIterator(unreal.CommonActivatableWidgetStack):
                layout = s.get_outer().get_outer()
                if s.get_name() in UI_LAYERS and s.get_owning_player() == self.pc and layout.is_in_viewport():
                    self.stacks[s.get_name()] = s
        return self.stacks.get(layer)

    def _top_screen(self):
        for layer in UI_LAYERS:
            stack = self.stack(layer)
            top = stack.get_active_widget() if stack else None
            if top and top.is_activated():
                return top
        return None

    def _shown_buttons(self):
        """가장 위 화면 안에서 보이는 버튼. 버튼에서 화면까지 모든 위젯이 보이고, 거치는 화면(탭 등)이 모두 활성이어야 한다."""
        top = self._top_screen()
        if not top:
            return []
        shown = []
        for button in unreal.ObjectIterator(unreal.CommonButtonBase):
            if button.get_owning_player() != self.pc:
                continue
            chain = list(_ui_chain(button))
            widgets = [w for w, _ in chain]
            if top in widgets and all(w.is_rendered() for w in widgets) \
                    and all(w.is_activated() for w in widgets if isinstance(w, unreal.CommonActivatableWidget)):
                shown.append((tuple(i for _, i in reversed(chain)), button))
        # 화면 위치(지오메트리)는 파이썬에서 0으로만 읽혀, 위젯 트리 순서로 정렬한다.
        return [b for _, b in sorted(shown, key=lambda x: x[0])]

    @staticmethod
    def _button_name(button):
        """위젯 블루프린트에서 붙인 이름(NewGameButton 등). 목록 항목처럼 실행 중에 만든 버튼은 클래스 이름이다."""
        name, cls = button.get_name(), button.get_class().get_name()
        return _short_class(button) if name.startswith(cls) else name


ctx = Ctx()


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


def _find_level():
    """지금 레벨의 월드와 플레이어 컨트롤러. 레벨을 읽는 중이면 (None, None)."""
    for world in unreal.ObjectIterator(unreal.World):
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        if _valid(pc):
            return world, pc
    return None, None


def _setup():
    """레벨 하나의 준비를 시작한다. 처음과 레벨이 바뀔 때마다 부르고, 아직 준비할 수 없으면 다음 프레임에 다시 온다."""
    world, pc = _find_level()
    if not pc:
        return
    player = pc.get_controlled_pawn()
    game_mode = unreal.GameplayStatics.get_game_mode(world)
    # 타이틀처럼 게임 모드에 플레이어 캐릭터가 없는 레벨은 캐릭터 없이 진행한다.
    if not player and game_mode and game_mode.get_editor_property("default_pawn_class"):
        return
    # 적 스포너는 에셋을 비동기로 읽은 뒤 적을 생성하므로, 모두 생성할 때까지(늦어도 10초) 기다린다.
    spawners = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/Ag.AgEnemySpawner"))
    ctx.spawn_wait_start = ctx.spawn_wait_start or time.time()
    if time.time() - ctx.spawn_wait_start < 10.0 and any(not s.get_editor_property("Enemy") for s in spawners):
        return
    ctx.spawn_wait_start = None
    ctx.world, ctx.pc, ctx.player, ctx.map = world, pc, player, world.get_name()
    ctx.chars = {"p": player} if player else {}
    if player:
        boss_class = unreal.load_class(None, "/Script/Ag.AgBossCharacter")
        enemies = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/Ag.AgEnemyCharacter")))
        bosses = [e for e in enemies if unreal.MathLibrary.class_is_child_of(e.get_class(), boss_class)]
        others = sorted((e for e in enemies if e not in bosses), key=lambda e: e.get_distance_to(player))
        if bosses:
            ctx.chars["b"] = bosses[0]
        ctx.chars.update(("e%d" % i, e) for i, e in enumerate(others))
    for key, actor in ctx.chars.items():
        ctx.asc[key] = actor.get_component_by_class(unreal.AbilitySystemComponent)
        ctx.aset[key] = unreal.find_object(None, actor.get_path_name() + ".AttributeSet")
    # 패드처럼 창을 거치지 않는 입력도 뷰포트에서 버린다. 키 입력 명령(AgPressKey 등)은 뷰포트를 거치지 않는다.
    ctx.console("AgIgnoreDeviceInput 1")
    # 준비: 적 AI를 멈추고, 맵을 연 직후의 적 공격이 끝나고 프레임이 안정되기를 기다린다.
    ctx.ai(False)
    ctx.levels += 1
    if ctx.levels == 1:
        ctx.banner(TITLE, "title")
        for i in range(len(STEPS)):
            ctx._step_line(i, "wait")
    ctx.banner("[진행] %s 준비: 동작이 끝나고 프레임이 안정되기를 기다림" % ctx.map, "warm", YELLOW)
    ctx.warm = {"start": time.time(), "times": []}


def _level_alive():
    """준비한 레벨이 그대로인지. 레벨이 바뀌면 옛 레벨의 컨트롤러와 캐릭터는 지워진다."""
    return _valid(ctx.pc) and (ctx.player is None or _valid(ctx.player))


def _leave_level():
    """레벨이 바뀌었다. 옛 레벨의 객체를 놓고, 다음 레벨의 준비가 끝날 때까지 시간을 멈춘다."""
    if ctx.t0 is not None and ctx.rows:
        ctx.paused_at = ctx.rows[-1]["t"]
    ctx.t0 = ctx.warm = None
    ctx.world = ctx.pc = ctx.player = None
    ctx.chars, ctx.asc, ctx.aset, ctx.stacks = {}, {}, {}, {}
    ctx._reset_inputs()


def _warm_up():
    """준비 중의 한 프레임. 모든 캐릭터의 몽타주가 끝나고 프레임이 안정되면(늦어도 20초 뒤) 시간을 이어서 센다."""
    now = time.time()
    w = ctx.warm
    w["times"] = [x for x in w["times"] + [now] if now - x <= 2.0]
    fps = sum(1 for x in w["times"] if now - x <= 1.0)
    # 창마다 프레임 수준이 달라 정해진 fps가 아니라, 직전 1초와 비슷해졌는지로 안정을 본다.
    steady = now - w["start"] >= 2.0 and abs(fps - (len(w["times"]) - fps)) <= max(2, fps // 10)
    idle = all(ctx.montage(k) is None for k in ctx.chars)
    if (idle and steady) or now - w["start"] > 20.0:
        ctx.notes.append("%s 준비 %.1f초, 시작 직전 %d fps" % (ctx.map, now - w["start"], fps))
        ctx.banner("[완료] %s 준비 (%.1f초, %d fps)" % (ctx.map, now - w["start"], fps), "warm", DONE)
        if AI_ON:
            ctx.ai(True)
        ctx.t0 = now - ctx.paused_at


def _record(t):
    """한 프레임의 기록. 플레이어는 p로 시작하는 값, 적은 그 키(b, e0…)로 시작하는 값이다. 지워진 적은 남기지 않는다."""
    row = {"t": round(t, 3), "map": ctx.map, "ui": ctx.screen()}
    if ctx.player:
        velocity = ctx.player.get_velocity()
        row.update({"pm": ctx.montage("p"), "pHP": ctx.attr("p", "HP"), "pMP": ctx.attr("p", "MP"), "pUP": ctx.attr("p", "UP"),
                    "guard": ctx.has("p", "State.Guarding"), "pdead": ctx.has("p", "State.Dead"), "pvz": round(velocity.z, 1),
                    "pspeed": round(math.hypot(velocity.x, velocity.y), 1), "fov": round(ctx.fov(), 2)})
    live = [k for k, actor in ctx.chars.items() if _valid(actor)]
    for k in live:
        if k not in ctx.keys:
            ctx.keys.append(k)
        if k == "p":
            continue
        row.update({k + "m": ctx.montage(k), k + "HP": ctx.attr(k, "HP"), k + "PP": ctx.attr(k, "PP"),
                    k + "dead": ctx.has(k, "State.Dead"), k + "focus": ctx.focus(k), k + "known": ctx.known(k),
                    k + "seen": ctx.seen(k), k + "angle": round(ctx.angle(k), 1)})
    watch = globals().get("WATCH_TAGS", ())
    row["tags"] = {k: [tag for tag in watch if ctx.has(k, tag)] for k in live}
    return row


def _summary(e):
    """예외를 "종류: 내용 (시나리오 41줄)"로 줄인다. 시나리오를 거치지 않았으면 예외가 난 실행기 함수 이름을 붙인다."""
    # 보낸 글(시나리오 + 실행기)의 프레임만 본다. 그 안에서 부른 다른 모듈의 줄 번호는 이 글의 줄이 아니다.
    frames = [f for f in traceback.extract_tb(e.__traceback__) if f.filename == _summary.__code__.co_filename]
    lines = [f.lineno for f in frames if f.lineno < _RUNNER_LINE]
    where = "시나리오 %d줄" % lines[-1] if lines else "실행기 %s" % frames[-1].name
    return "%s: %s (%s)" % (type(e).__name__, e, where)


def _finish(error=None):
    unreal.unregister_slate_post_tick_callback(ctx.handle)
    try:
        if _valid(ctx.pc):
            ctx.clear_inputs()
            ctx.ai(False)  # 판정 뒤에 적이 계속 공격하지 않게 한다.
    except Exception as e:
        ctx.notes.append("정리 오류: " + _summary(e))
    ok, items = False, [(False, "오류: " + error)] if error else []
    if not error:
        try:
            ok, lines = evaluate(ctx)
            # 판정 문구는 글자만 주거나, 항목별 판정을 붙여 (통과 여부, 글자)로 준다.
            items = [(None, l) if isinstance(l, str) else (bool(l[0]), l[1]) for l in lines]
        except Exception as e:
            ok = False
            error = "판정 오류: " + _summary(e)
            items = [(False, error)]
    verdict = "실행 실패" if error else "통과" if ok else "테스트 실패"
    lines = ["%s %s" % ("-" if i is None else "[통과]" if i else "[실패]", text) for i, text in items]
    try:
        if not error and 0 < ctx.step_index <= len(STEPS):
            ctx._step_line(ctx.step_index - 1, "done")
        ctx.banner("판정: %s" % verdict, "verdict", PASS if ok else FAIL)
        for n, (i, text) in enumerate(items):
            ctx.banner("  " + lines[n], "verdict%d" % n, PASS if (ok if i is None else i) else FAIL)
    except Exception as e:
        ctx.notes.append("판정 표시 오류: " + _summary(e))
    full = [v for k, v in ctx.fps.items() if k < max(ctx.fps)] if ctx.fps else []
    # run.py는 OUT이 생기면 바로 읽으므로, 다 쓴 뒤에 이름을 바꿔 내놓는다. 쓰다가 실패하면 run.py가 제한 시간까지
    # 기다리므로, 시나리오가 기록에 넣은 엔진 값(Vector 등)은 글자로 남긴다.
    with open(OUT + ".tmp", "w", encoding="utf-8") as f:
        json.dump({"title": TITLE, "verdict": verdict, "lines": lines, "notes": ctx.notes,
                   "montages": {k: ctx.seq(k) for k in ctx.keys},
                   "seconds": ctx.rows[-1]["t"] if ctx.rows else 0, "frames": len(ctx.rows),
                   "fps_min": min(full) if full else 0, "rows": ctx.rows}, f, ensure_ascii=False, default=str)
    os.replace(OUT + ".tmp", OUT)


def _tick(dt):
    try:
        if not ctx.window_locked:
            ctx.window_locked = _lock_window()
        if ctx.warm is not None and not _level_alive():
            _leave_level()
        if ctx.warm is None:
            _setup()
            return
        if ctx.t0 is None:
            _warm_up()
            return
        t = time.time() - ctx.t0
        ctx.fps[int(t)] = ctx.fps.get(int(t), 0) + 1
        ctx._pump()
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
        _finish(_summary(e))


ctx.handle = unreal.register_slate_post_tick_callback(_tick)
print("runner started:", TITLE)
