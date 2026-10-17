# AGENTS.md

## 프로젝트

이 프로젝트는 Unreal Engine 5 기반 액션 RPG다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

1. C++ 타입 이름은 Unreal Engine 5의 기본 Prefix에 `Ag`를 추가한다. (예시: `AAgCharacterBase`)

2. 모든 C++ 소스·헤더 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.

3. 게임플레이 태그는 `AgGameplayTags.h`·`.cpp`에 네이티브로만 선언한다. 태그 ini는 쓰지 않는다.

4. 모든 게임 로직은 C++로 구현한다. BP 이벤트 그래프와 Construction Script에 로직을 작성하지 않는다.

5. 기획 수치와 설정값은 JSON을 단일 원본으로 관리한다. C++ 상수, BP 기본값, Data Asset에 같은 기획 데이터를 중복 정의하지 않는다.

6. 모션·메시·머티리얼·VFX 등 리소스 자체와 제작 정보는 UE 에셋으로 관리한다. JSON에는 필요한 식별자와 참조를 기록한다.

7. `auto` 키워드를 사용하지 않는다. 지역 변수, 범위 기반 반복문, 람다 매개변수에도 명시적인 타입을 쓴다.

8. C++ 줄바꿈과 들여쓰기는 Unreal Engine 스타일을 따른다. 중괄호는 새 줄에 놓고, 한 줄에 여러 문장을 쓰지 않는다. 포맷 설정은 저장소 루트의 `.clang-format`을 따른다.

---

## 작업 방식

작업 방식은 [AI 워크플로우](<Docs/AI 워크플로우.md>)를 따른다.
