// Copyright Woogle. All Rights Reserved.

#include "Character/AgBossCharacter.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgTargeting.h"
#include "AbilitySystemComponent.h"
#include "AI/AgBossAIController.h"
#include "Character/AgPlayerCharacter.h"
#include "Components/WidgetComponent.h"
#include "Core/AgAssetManager.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Data/AgCharacterData.h"
#include "Data/AgEffectData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AAgBossCharacter::AAgBossCharacter()
{
	CharacterDataClass = UAgBossData::StaticClass();
	AIControllerClass = AAgBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 보스는 걷는 방향이 아니라 AI의 대상을 향해 스스로 돕니다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 화면 공간이라 거리와 관계없이 크기가 같습니다. 데이터의 본에는 BeginPlay에서 붙입니다.
	LockOnMarker = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnMarker"));
	ExecutionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExecutionPrompt"));
	for (UWidgetComponent* WorldWidget : {LockOnMarker.Get(), ExecutionPrompt.Get()})
	{
		WorldWidget->SetupAttachment(GetMesh());
		WorldWidget->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidget->SetDrawAtDesiredSize(true);
		WorldWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldWidget->SetGenerateOverlapEvents(false);
		WorldWidget->SetHiddenInGame(true);
	}

	// 처형 안내는 본 바로 위(보스 머리 위)에 섭니다.
	ExecutionPrompt->SetPivot(FVector2D(0.5f, 1.f));
}

void AAgBossCharacter::EnterPhase2()
{
	Phase = 2;
}

AAgCharacterBase* AAgBossCharacter::GetTarget() const
{
	const AAgBossAIController* AIController = Cast<AAgBossAIController>(GetController());
	return AIController ? AIController->GetTarget() : nullptr;
}

void AAgBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	AttachWorldWidgets();
	GetAbilitySystemComponent()
		->RegisterGameplayTagEvent(AgGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleGroggyChanged);
}

void AAgBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateWorldWidgets();
}

void AAgBossCharacter::HandleGroggyChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0 || IsDead())
	{
		return;
	}
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AgGameplayTags::State_GroggyTime));
	ASC->ApplyGameplayEffectToSelf(UAgAssetManager::Get().GetEffect(EAgEffect::PPRefill), 1.f, ASC->MakeEffectContext());
}

bool AAgBossCharacter::IsExecutionTargetOf(AAgPlayerCharacter& Player) const
{
	const UAgAbilitySystemComponent* ASC = Cast<UAgAbilitySystemComponent>(Player.GetAbilitySystemComponent());
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindSpecByTag(AgGameplayTags::Ability_Action_Execution) : nullptr;
	const FAgAbilityData* Execution = Spec ? Player.FindAbility(Spec->Handle) : nullptr;
	return Execution && Execution->bHasTargeting && AgTargeting::FindTarget(Execution->Targeting, Player) == this;
}

void AAgBossCharacter::AttachWorldWidgets()
{
	const UAgBossData* Data = CastChecked<UAgBossData>(GetCharacterData());
	LockOnMarker->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->LockOnBone);
	ExecutionPrompt->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->ExecutionPromptBone);
}

void AAgBossCharacter::UpdateWorldWidgets()
{
	// 둘 다 플레이어나 보스의 사망 연출이 시작되는 순간부터, 처형 중, 궁극기 컷신(플레이어 컨트롤러의 시네마틱 모드) 동안
	// 숨깁니다. 플레이어 화면에 관한 것이라 AI의 대상이 아니라 로컬 플레이어를 기준으로 합니다.
	AAgPlayerCharacter* Player = Cast<AAgPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const bool bInCombat = Player && !Player->IsDead() && !IsDead() &&
	                       !GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Execution) &&
	                       !(PlayerController && PlayerController->bCinematicMode);

	LockOnMarker->SetHiddenInGame(!(bInCombat && Player->GetLockOnTarget() == this));

	// 처형 발동 조건 1, 2(그로기, 범위 안)를 만족하는 처형 대상이면 표시합니다. 지금 하는 행동 때문에 처형 키를 받지 않는 동안에도
	// 표시합니다. 범위 확인은 대상 찾기 쿼리를 돌리므로 그로기 중에만 합니다.
	const bool bGroggy = GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Groggy);
	ExecutionPrompt->SetHiddenInGame(!(bInCombat && bGroggy && IsExecutionTargetOf(*Player)));
}
