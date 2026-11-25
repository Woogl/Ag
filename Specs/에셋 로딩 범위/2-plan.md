# 에셋 로딩 범위 기술 설계 (Plan)

## 1. 영향 범위 및 아키텍처

### 1.1 원칙

- 에셋을 쓰는 객체가 그 에셋을 하드 참조로 들고, 그 객체가 사라지면 엔진 가비지 컬렉션이 에셋을 내립니다. 엔진이 블루프린트의 에셋 참조에 하는 일과 같습니다. 핸들, 참조 수, 미리 읽기 목록을 두지 않고, 쓰는 코드(Task, 어빌리티)는 지금처럼 하드 포인터를 그대로 씁니다.
- 에셋 선택을 포함한 데이터는 지금처럼 JSON에 둡니다.
- 에셋 매니저는 게임 데이터의 종류를 알지 못합니다. JSON으로 채우는 데이터 객체의 공통 부모(`UAgJsonData`)만 알고, 파일마다의 내용·읽는 방법·조회는 그 데이터 클래스가 맡습니다.

### 1.2 구조

| 객체 | 맡는 일 |
| - | - |
| `UAgAssetManager` | `DesignData`의 모든 JSON을 파싱해 표로 보관합니다(글자라 에셋을 읽지 않음). `UAgJsonData`의 하위 클래스를 찾아 읽고, 다시 읽고, 쿡에 전달합니다. 늘 들고 있는 데이터는 `GetData<T>()`로, 쓰는 쪽이 붙잡는 데이터는 `LoadData<T>(Key)`로 줍니다. |
| `UAgJsonData` | 데이터 클래스의 공통 부모. 값을 받는 파일 이름(`GetFileName`), 쓰는 쪽이 붙잡는 데이터인지(`IsLoadedOnDemand`), 읽는 방법(`ReadJson`), 다시 읽은 값을 받는 방법(`CopyJson`)을 하위 클래스가 정합니다. |
| 늘 들고 있는 데이터(`UAgEffectSet`, `UAgCombatRules`, `UAgTargetingPresetSet`, `UAgItemSet`, `UAgCharacterSet`) | 에셋을 하드 참조로 가리키지 않는 파일. 게임을 켤 때 읽어 에셋 매니저가 듭니다. 조회 함수는 각 클래스의 정적 함수입니다(`UAgCombatRules::Get`, `UAgEffectSet::FindEffect`·`GetEffect`, `UAgTargetingPresetSet::FindPreset`, `UAgItemSet::FindItem`·`GetItems`, `UAgCharacterData::Find`·`GetIds`). |
| 쓰는 쪽이 붙잡는 데이터(`UAgAbilitySet`, `UAgCameraData`) | 에셋을 하드 참조로 가리키는 데이터. 에셋 매니저는 약한 참조로만 기억하고, 쓰는 캐릭터가 UPROPERTY로 붙잡습니다. |

- `UAgAbilitySet`은 캐릭터 종류 하나의 어빌리티 정의(`FAgAbilityData`, 하드 참조 그대로)입니다. `LoadData<UAgAbilitySet>(캐릭터 ID)`가 그 캐릭터의 `Abilities` 목록에 있는 항목만 읽습니다. 캐릭터는 `PreInitializeComponents`에서 캐릭터 데이터를 찾은 직후 받아 듭니다(적 AI 컨트롤러의 빙의와 어빌리티 부여보다 앞).
- `UAgCameraData`는 플레이어 캐릭터가 듭니다. 카메라 매니저와 처형 카메라 모디파이어는 조종하는 플레이어 캐릭터에게서 받아 씁니다. 다섯 곳 모두 플레이어 캐릭터가 있을 때만 도는 코드입니다. 타이틀 화면에도 같은 카메라 매니저가 있으므로, 주인을 카메라 매니저가 아니라 플레이어 캐릭터로 둡니다.
- 같은 Key로 읽은 객체가 살아 있으면 `LoadData`가 그 객체를 돌려주어, 같은 종류의 캐릭터가 함께 씁니다. 붙잡는 쪽이 모두 사라지면 가비지 컬렉션이 객체와 에셋을 함께 내립니다. 놓는 코드는 없습니다.
- 캐릭터 말고도 붙잡을 수 있습니다. 플레이 중에 캐릭터를 생성하는 장치가 생기면, 그 장치가 생성할 종류의 묶음을 미리 받아 들어 생성하는 프레임의 읽기를 없앱니다.
- 데이터 클래스는 따로 등록하지 않습니다. 에셋 매니저가 `UAgJsonData`의 하위 클래스를 모두 찾습니다.
- 코드 전용 효과(`NativeEffects`)는 에셋 매니저에서 `UAgEffectSet`으로 옮깁니다.

동기 읽기를 고른 까닭: 행동은 발동하는 순간 몽타주가 있어야 합니다. 비동기로 읽으면 다 읽기 전의 발동을 미루거나 막는 규칙이 새로 필요합니다.

### 1.3 생성 액터 정의

- SpawnActor Task의 Params 필드 `Actor`의 타입을 ID(`FName`)에서 생성 액터 정의(`FAgSpawnActorDefinition`)로 바꿉니다. JSON에는 지금처럼 ID 문자열을 적습니다.
- 리더는 이 타입의 필드에서 문자열을 만나면 파싱해 둔 `SpawnActors.json`에서 그 ID의 항목을 읽어 필드에 채웁니다. 태그·어트리뷰트처럼 타입으로 정해지는 읽기라, Task마다 따로 알려 줄 것이 없고 빠뜨릴 수 없습니다.
- 정의가 어빌리티 정의 안에 들어가므로 생성 액터 클래스도 그 캐릭터의 묶음과 함께 붙잡히고 함께 내려갑니다.
- 전역 생성 액터 맵(`UAgSpawnActorSet`)과 `FindSpawnActor`를 없앱니다. SpawnActor Task는 자기 Params의 정의를 씁니다.

### 1.4 검사

- 에셋 매니저는 켤 때와 다시 읽을 때 모든 데이터 클래스를 새 객체에 읽습니다(`ReadJson`에 Key 없이). 쓰는 쪽이 붙잡는 데이터도 이때 한 번 모두 읽어 검사하고 버립니다. 검사로 읽힌 에셋은 붙잡는 것이 없어 다음 가비지 컬렉션 때 내려갑니다. `UAgAbilitySet`은 이때 어느 어빌리티도 가리키지 않는 생성 액터 정의도 읽어 봅니다.
- 검사는 모든 빌드에서 합니다. 에디터와 패키지 게임이 같은 길을 타게 하려는 것입니다. 패키지 게임을 켜는 시간은 지금과 같습니다(지금도 켤 때 모두 읽습니다).
- 캐릭터의 묶음을 읽을 때도 같은 리더가 같은 검사를 하므로, 읽지 못한 항목을 비우고 빼는 규칙이 두 길에서 같습니다.
- `ModifyCook`은 늘 들고 있는 데이터와, 쓰는 쪽이 붙잡는 데이터를 모두 읽은 객체를 지금의 참조 순회에 넣어 쿡 목록을 만듭니다.

### 1.5 다시 읽기 (에디터)

- 묶음 읽기가 성공하면 파싱한 표를 바꾸고, 늘 들고 있는 데이터에 값을 복사한 뒤(`CopyJson`), 살아 있는 붙잡힌 데이터마다 같은 Key로 새로 읽어 기존 객체에 복사합니다. 캐릭터는 같은 객체에서 새 값을 보므로 '재로딩 반영 시점'의 어빌리티 행이 그대로입니다.
- 파생 객체를 만드는 일은 `CopyJson`의 재정의가 맡습니다(효과 세트의 GE, 대상 프리셋 세트의 프리셋, 캐릭터 세트의 기존 객체 유지).
- 생성 액터 정의는 어빌리티 정의의 일부가 되어, 반영 시점이 "생성할 때 조회"에서 "다음 발동 때 정의를 복사"로 바뀝니다.
- 오류가 있으면 지금처럼 이전 데이터를 그대로 둡니다.

### 1.6 리더

- 파일 파싱(`ParseFile`)과 파싱해 둔 파일에서 읽기(`ReadFile`, `ReadEntries`, `ReadSpawnActor`)를 나눕니다.
- 리더가 읽는 동안만 들던 파일별 최상위 키 표를 파싱한 파일의 표(`FAgJsonDocuments`)로 바꿔 에셋 매니저가 들고 리더에 넘깁니다. 정의 ID 검사와 생성 액터 정의 읽기가 이 표를 쓰고, 읽는 순서와 관계없이 검사합니다.

### 1.7 치르는 값

- 처음 등장하는 종류의 캐릭터는 그 캐릭터의 JSON 항목을 리플렉션으로 읽고 에셋을 동기 로딩합니다. 걸리는 시간은 `4-test.md`에 잽니다.
- 레벨을 바꾸면 플레이어의 묶음도 내려갔다가 다시 읽힙니다.
- 데이터 오류가 있으면 켤 때와 그 캐릭터의 묶음을 읽을 때마다 같은 오류가 다시 나옵니다.

### 1.8 공통 규칙 준수

- 엔진 기능(하드 참조, 약한 참조, 가비지 컬렉션)으로 하고 수명 관리 코드를 따로 두지 않습니다.
- JSON 형식, 기획서, 기획 데이터는 바뀌지 않습니다.
- 클래스 관계가 바뀌므로 4단계에서 [데이터 로딩과 스키마](<../../Docs/Architecture/데이터 로딩과 스키마.md>)의 '읽는 방법', '로더의 객체 이름', '오류 검사', '다시 읽기', '패키징', '데이터를 찾는 방법', '어빌리티의 데이터', '파일 구성', '재로딩 반영 시점', '생성 액터 정의', 클래스 구조와 [캐릭터 기반 구조와 GAS](<../../Docs/Architecture/캐릭터 기반 구조와 GAS.md>)의 캐릭터 행, 에디터 작업 스킬의 '에디터 파이썬 이름'을 고칩니다.

## 2. GAS 및 데이터 설계

- 태그, 어빌리티, 효과, JSON 스키마는 바뀌지 않습니다.
- `UAgAbilitySet`은 "`Abilities.json` 전체"에서 "캐릭터 종류 하나의 어빌리티 정의"로 뜻이 바뀝니다. 필드는 그대로입니다.
- `FAgSpawnActorParams::Actor`의 타입이 `FName`에서 `FAgSpawnActorDefinition`으로 바뀝니다. 정의는 오류에 적을 자기 ID를 함께 가집니다.

## 3. 인터페이스 & 함수 시그니처

```cpp
// Core/AgJsonData.h
UCLASS(Abstract)
class UAgJsonData : public UObject
{
	virtual const TCHAR* GetFileName() const;
	virtual bool IsLoadedOnDemand() const;
	/** Key는 LoadData에 준 값입니다. 검사와 쿡은 None으로 전체를 읽습니다. */
	virtual bool ReadJson(FAgJsonReader& Reader, FName Key);
	virtual void CopyJson(const UAgJsonData& Loaded);
};

// Core/AgAssetManager.h
template <typename T>
const T& GetData() const;

template <typename T>
T* LoadData(FName Key = NAME_None);

void ReadObjectFields(const FString& Path, const FJsonObject& Json, UObject& Object);

// Core/AgJsonReader.h
struct FAgJsonDocument { TSharedPtr<FJsonObject> Root; TMap<FName, TSharedPtr<FJsonValue>> Entries; };
using FAgJsonDocuments = TMap<FName, FAgJsonDocument>;

FAgJsonReader(UObject& InOuter, FAgJsonDocuments& InDocuments);
bool ParseFile(const FString& Path);
bool ReadFile(const FString& Path, const FProperty& Property, void* Out);
bool ReadEntries(const FString& Path, TConstArrayView<FName> Ids, const FProperty& Property, void* Out);
bool ReadSpawnActor(const FString& Path, FName Id, FAgSpawnActorDefinition& Out);

// Character/AgCharacterBase.h
const FAgAbilityData* FindAbility(FName Id) const;   // UAgCharacterData::FindAbility를 대신합니다.
UPROPERTY(Transient)
TObjectPtr<UAgAbilitySet> AbilitySet;

// Character/AgPlayerCharacter.h
const UAgCameraData& GetCameraData() const;
UPROPERTY(Transient)
TObjectPtr<UAgCameraData> CameraData;
```

없애는 것: `UAgAssetManager`의 데이터 멤버와 조회 함수 전부(`GetCombatRules`, `GetCameraData`, `FindTargetingPreset`, `FindAbility`, `FindSpawnActor`, `FindEffect`, `GetEffect`, `FindCharacterData`, `GetCharacterDataIds`, `FindItem`, `GetItems`), 파일 목록 표(`GetJsonFiles`), `UAgSpawnActorSet`, `UAgCharacterData::FindAbility`.
