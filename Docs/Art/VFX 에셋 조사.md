# VFX 에셋 조사

> 조사일 2026-10-09 · 출처: 언리얼 에디터 에셋 레지스트리 (Unreal MCP) · 범위: `/Game` · 총 3개

`사용처`는 이 에셋을 참조하는 에셋입니다. 레벨에 배치된 액터가 참조하면 `레벨명 (액터 N)`으로 묶어 표시합니다 (N = 참조하는 액터 수).

## 나이아가라 시스템

| 이름 | 경로 | 이미터 | 렌더러 (스프라이트/메시/리본) | GPU 시뮬레이션 | 참조 에셋 | 사용처 |
|---|---|---|---|---|---|---|
| `NS_JumpPad` | `/LevelPrototyping/Interactable/JumpPad/Assets` | 3 | 3 / 2 / 0 | 예 | `SM_CircularBand`, `SM_CircularGlow` | BP_JumpPad |
| `NS_Damage` | `/Variant_Combat/VFX` | 1 | 1 / 0 / 0 | 아니오 | - | BP_CombatCharacter, BP_CombatDamageableBox, BP_CombatDummy, BP_CombatEnemy |
| `NS_Jump_Trail` | `/Variant_Platforming/VFX` | 1 | 0 / 0 / 1 | 아니오 | - | BP_PlatformingCharacter |
