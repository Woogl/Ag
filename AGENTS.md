# AGENTS.md

## 프로젝트

이 프로젝트는 Unreal Engine 5 기반 액션 RPG다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

1. C++ 타입 이름은 Unreal Engine 5의 기본 Prefix에 `Ag`를 추가한다. (예시: `AAgCharacterBase`)

2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.

3. 게임플레이 태그는 `AgGameplayTags.h`·`.cpp`에 네이티브로만 선언한다. 태그 ini는 쓰지 않는다.

4. 모든 게임 로직은 C++로 구현하며, BP 이벤트 그래프는 사용하지 않는다.

5. 코드 주석은 한글로 간결하게 작성한다. 기존 영어 주석은 그 코드를 수정할 때 함께 한글로 바꾼다.

---

## 작업 방식

작업 방식은 [AI 워크플로우](<Docs/AI 워크플로우.md>)를 따른다.
