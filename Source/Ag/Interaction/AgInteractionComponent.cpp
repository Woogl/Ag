// Copyright Woogle. All Rights Reserved.

#include "Interaction/AgInteractionComponent.h"

#include "AbilitySystem/AgAbilitySystemComponent.h"
#include "AbilitySystem/AgTargeting.h"
#include "Character/AgPlayerCharacter.h"
#include "Core/AgCollision.h"
#include "Core/AgGameplayTags.h"
#include "Data/AgAbilityData.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Game/AgGameState.h"
#include "GameplayAbilitySpecHandle.h"
#include "Interaction/AgInteractable.h"
#include "UI/AgInteractionPrompt.h"

namespace AgInteractionComponentPrivate
{
	/** 엔진에 점 겹침 검사가 없어, 플레이어 캡슐 중심이 상호작용 범위 안에 있는지는 이 반경의 구로 봅니다. */
	constexpr float CenterRadius = 1.f;

	/** 지금 상호작용 키를 누르면 일어날 일입니다. Ability가 유효하면 그 어빌리티를 발동하고, 아니면 Device에 상호작용합니다. */
	struct FAgInteraction
	{
		FGameplayAbilitySpecHandle Ability;
		IAgInteractable* Device = nullptr;
		FVector PromptLocation = FVector::ZeroVector;
		FText Text;

		bool IsValid() const
		{
			return Ability.IsValid() || Device;
		}
	};

	/** 안내와 입력이 같은 대상을 보도록 둘 다 이 함수로 고릅니다. 어빌리티의 대상이 장치보다 우선합니다. */
	FAgInteraction FindInteraction(AAgPlayerCharacter& Player)
	{
		FAgInteraction Interaction;
		TArray<FGameplayAbilitySpecHandle> Handles;
		Player.FindInputAbilities(AgGameplayTags::Input_Interact, Handles);
		for (const FGameplayAbilitySpecHandle Handle : Handles)
		{
			const FAgAbilityData* Ability = Player.FindAbility(Handle);
			const AActor* Target = Ability && Ability->Targeting ? AgTargeting::FindTarget(*Ability->Targeting, Player) : nullptr;
			if (Target)
			{
				Interaction.Ability = Handle;
				Interaction.PromptLocation = Target->GetTargetLocation(&Player);
				Interaction.Text = Ability->DisplayName;
				return Interaction;
			}
		}

		const FVector Center = Player.GetActorLocation();
		TArray<FOverlapResult> Overlaps;
		Player.GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_AgInteraction, FCollisionShape::MakeSphere(CenterRadius));
		double NearestDistance = TNumericLimits<double>::Max();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			IAgInteractable* Device = Cast<IAgInteractable>(Overlap.GetActor());
			const FVector PromptLocation = Device ? Device->GetInteractionLocation() : FVector::ZeroVector;
			const double Distance = FVector::DistSquared(Center, PromptLocation);
			if (Device && Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Interaction.Device = Device;
				Interaction.PromptLocation = PromptLocation;
				Interaction.Text = Device->GetInteractionText();
			}
		}
		return Interaction;
	}
} // namespace AgInteractionComponentPrivate

UAgInteractionComponent::UAgInteractionComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetPivot(FVector2D(0.f, 0.5f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
	// 안내는 주인(플레이어)이 아니라 고른 대상의 안내 지점에 놓입니다.
	SetUsingAbsoluteLocation(true);
}

void UAgInteractionComponent::Interact()
{
	AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetOwner());
	const AgInteractionComponentPrivate::FAgInteraction Interaction = AgInteractionComponentPrivate::FindInteraction(*Player);
	if (Interaction.Ability.IsValid())
	{
		Player->GetAbilitySystemComponent()->TryActivateAbility(Interaction.Ability);
	}
	else if (Interaction.Device)
	{
		Interaction.Device->Interact(*Player);
	}
}

void UAgInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	AAgPlayerCharacter* Player = CastChecked<AAgPlayerCharacter>(GetOwner());
	const AAgGameState* StageState = GetWorld()->GetGameState<AAgGameState>();
	const bool bStageEnded = StageState && StageState->GetStageResult() != EAgStageResult::None;
	const bool bCutscene = Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(AgGameplayTags::State_Cutscene);

	// 지금 하는 행동 때문에 상호작용 키를 받지 않는 동안에도 표시하므로 입력 제한은 보지 않습니다.
	AgInteractionComponentPrivate::FAgInteraction Interaction;
	if (!Player->IsDead() && !bCutscene && !bStageEnded)
	{
		Interaction = AgInteractionComponentPrivate::FindInteraction(*Player);
	}
	UAgInteractionPrompt* Prompt = Cast<UAgInteractionPrompt>(GetUserWidgetObject());
	if (Interaction.IsValid() && Prompt)
	{
		SetWorldLocation(Interaction.PromptLocation);
		Prompt->SetAction(Interaction.Text);
	}
	SetHiddenInGame(!Interaction.IsValid());

	// 화면에 올리고 내리는 일은 부모가 하므로, 이번 프레임의 표시 여부를 정한 뒤에 부릅니다.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
