// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCueNotify_Looping.h"

#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"

AAgGameplayCueNotify_Looping::AAgGameplayCueNotify_Looping()
{
	RecurringMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RecurringMesh"));
	RecurringMesh->SetupAttachment(RootComponent);
	RecurringMesh->bAutoRegister = false;
	RecurringMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RecurringMesh->SetGenerateOverlapEvents(false);
}

bool AAgGameplayCueNotify_Looping::OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	const bool bResult = Super::OnActive_Implementation(Target, ResolveParameters(Target, Parameters));
	ApplyNiagaraParameters(ApplicationSpawnResults);
	return bResult;
}

bool AAgGameplayCueNotify_Looping::WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	// 엔진 Looping 큐는 숨긴 채 만든 액터를 다시 보이게 하지 않아(Actor 큐의 WhileActive를 부르지 않음), 재활용되지
	// 않은 액터에서는 놓은 메시가 그려지지 않습니다.
	SetActorHiddenInGame(false);
	const bool bResult = Super::WhileActive_Implementation(Target, ResolveParameters(Target, Parameters));
	ApplyNiagaraParameters(LoopingSpawnResults);
	return bResult;
}

bool AAgGameplayCueNotify_Looping::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	const bool bResult = Super::OnExecute_Implementation(Target, ResolveParameters(Target, Parameters));
	ApplyNiagaraParameters(RecurringSpawnResults);
	PlaceRecurringMesh(Target);
	return bResult;
}

bool AAgGameplayCueNotify_Looping::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	if (!bRemovalEffectsAtRecurringMeshes)
	{
		ClearRecurringMeshes();
		const bool bResult = Super::OnRemove_Implementation(Target, ResolveParameters(Target, Parameters));
		ApplyNiagaraParameters(RemovalSpawnResults);
		return bResult;
	}

	// 부모는 꺼질 때 효과를 큐 위치에 재생하므로 부르지 않고, 부모의 순서(반복 효과 끄기, 꺼질 때 효과, 이벤트)를 따릅니다.
	RemoveLoopingEffects();
	PlayRemovalEffectsAtRecurringMeshes(Target, Parameters);
	ClearRecurringMeshes();
	OnRemoval(Target, Parameters, RemovalSpawnResults);
	return false;
}

FGameplayCueParameters AAgGameplayCueNotify_Looping::ResolveParameters(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	FGameplayCueParameters Resolved = Parameters;
	const AAgCharacterBase* Character = Cast<AAgCharacterBase>(Target);
	if (bAttachToWeapon && Character && Character->GetWeaponComponent())
	{
		Resolved.TargetAttachComponent = Character->GetWeaponComponent();
	}
	return Resolved;
}

void AAgGameplayCueNotify_Looping::ApplyNiagaraParameters(const FGameplayCueNotify_SpawnResult& SpawnResult) const
{
	for (UFXSystemComponent* FX : SpawnResult.FxSystemComponents)
	{
		if (UNiagaraComponent* Niagara = Cast<UNiagaraComponent>(FX))
		{
			for (const TPair<FName, FLinearColor>& Parameter : NiagaraColorParameters)
			{
				Niagara->SetVariableLinearColor(Parameter.Key, Parameter.Value);
			}
			for (const TPair<FName, float>& Parameter : NiagaraFloatParameters)
			{
				Niagara->SetVariableFloat(Parameter.Key, Parameter.Value);
			}
		}
	}
}

void AAgGameplayCueNotify_Looping::PlaceRecurringMesh(AActor* Target)
{
	if (!Target || !RecurringMesh->GetStaticMesh())
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(Target);
	const float ViewYaw = Pawn ? Pawn->GetControlRotation().Yaw : Target->GetActorRotation().Yaw;
	const ACharacter* Character = Cast<ACharacter>(Target);
	const FVector Feet = Target->GetActorLocation() - FVector(0.f, 0.f, Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f);
	const float Yaw = ViewYaw + FMath::FRandRange(RecurringMeshYaw.Min, RecurringMeshYaw.Max);
	const float Distance = FMath::FRandRange(RecurringMeshDistance.Min, RecurringMeshDistance.Max);
	const float Height = FMath::FRandRange(RecurringMeshHeight.Min, RecurringMeshHeight.Max);
	const FVector Location = Feet + FRotator(0.f, Yaw, 0.f).Vector() * Distance + FVector(0.f, 0.f, Height);
	const float Roll = FMath::DegreesToRadians(FMath::FRandRange(RecurringMeshRoll.Min, RecurringMeshRoll.Max));
	const FVector Direction = FRotator(0.f, ViewYaw + 90.f, 0.f).Vector() * FMath::Cos(Roll) + FVector::UpVector * FMath::Sin(Roll);

	// 큐 액터를 따라 움직이지 않도록 붙이지 않고 월드에 놓습니다.
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient, RecurringMesh);
	Mesh->CreationMethod = EComponentCreationMethod::Instance;
	Mesh->SetupAttachment(nullptr);
	Mesh->SetWorldTransform(FTransform(FRotationMatrix::MakeFromY(Direction).Rotator(), Location, RecurringMesh->GetRelativeScale3D()));
	Mesh->RegisterComponent();
	PlacedMeshes.Add(Mesh);
}

void AAgGameplayCueNotify_Looping::PlayRemovalEffectsAtRecurringMeshes(AActor* Target, const FGameplayCueParameters& Parameters)
{
	if (!Target)
	{
		return;
	}
	for (const UStaticMeshComponent* Mesh : PlacedMeshes)
	{
		if (!Mesh)
		{
			continue;
		}
		FGameplayCueParameters MeshParameters = ResolveParameters(Target, Parameters);
		MeshParameters.Location = Mesh->GetComponentLocation();
		FGameplayCueNotify_SpawnContext SpawnContext(GetWorld(), Target, MeshParameters);
		SpawnContext.SetDefaultSpawnCondition(&DefaultSpawnCondition);
		SpawnContext.SetDefaultPlacementInfo(&DefaultPlacementInfo);
		if (DefaultSpawnCondition.ShouldSpawn(SpawnContext))
		{
			RemovalEffects.ExecuteEffects(SpawnContext, RemovalSpawnResults);
			ApplyNiagaraParameters(RemovalSpawnResults);
		}
	}
}

void AAgGameplayCueNotify_Looping::ClearRecurringMeshes()
{
	for (UStaticMeshComponent* Mesh : PlacedMeshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	PlacedMeshes.Reset();
}
