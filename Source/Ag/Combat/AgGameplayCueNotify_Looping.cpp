// Copyright Woogle. All Rights Reserved.

#include "Combat/AgGameplayCueNotify_Looping.h"

#include "Ag.h"
#include "Camera/AgPostProcessComponent.h"
#include "Character/AgCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

AAgGameplayCueNotify_Looping::AAgGameplayCueNotify_Looping()
{
	PostProcess = CreateDefaultSubobject<UAgPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->OnFinished.BindUObject(this, &ThisClass::GameplayCueFinishedCallback);

	RecurringMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RecurringMesh"));
	RecurringMesh->SetupAttachment(RootComponent);
	RecurringMesh->bAutoRegister = false;
	RecurringMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RecurringMesh->SetGenerateOverlapEvents(false);
}

bool AAgGameplayCueNotify_Looping::Recycle()
{
	PostProcess->Reset();
	ClearRecurringMeshes();
	bAutoDestroyOnRemove = GetClass()->GetDefaultObject<AAgGameplayCueNotify_Looping>()->bAutoDestroyOnRemove;
	return Super::Recycle();
}

bool AAgGameplayCueNotify_Looping::GameplayCuePendingRemove()
{
	// 페이드 중 다시 적용되면 새 큐가 맡도록, 게임 시간 종료 타이머 없이도 회수 예정임을 알립니다.
	return PostProcess->IsFadingOut() || Super::GameplayCuePendingRemove();
}

void AAgGameplayCueNotify_Looping::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PostProcess->Reset();
	ClearRecurringMeshes();
	Super::EndPlay(EndPlayReason);
}

void AAgGameplayCueNotify_Looping::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyTargetTimeDilation(LoopingSpawnResults);
}

bool AAgGameplayCueNotify_Looping::OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	CueTarget = Target;
	const bool bResult = Super::OnActive_Implementation(Target, ResolveParameters(Target, Parameters));
	ApplyNiagaraParameters(ApplicationSpawnResults);
	return bResult;
}

bool AAgGameplayCueNotify_Looping::WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	// 엔진 Looping 큐는 숨긴 채 만든 액터를 다시 보이게 하지 않아(Actor 큐의 WhileActive를 부르지 않음), 재활용되지
	// 않은 액터에서는 놓은 메시가 그려지지 않습니다.
	SetActorHiddenInGame(false);
	CueTarget = Target;
	const bool bResult = Super::WhileActive_Implementation(Target, ResolveParameters(Target, Parameters));
	ApplyNiagaraParameters(LoopingSpawnResults);
	if (!bLoopingEffectsRemoved && !PostProcess->IsPlaying())
	{
		PostProcess->Start(Target);
	}
	if (!bLoopingEffectsRemoved && bUseTargetTimeDilation)
	{
		SetActorTickEnabled(true);
	}
	return bResult;
}

bool AAgGameplayCueNotify_Looping::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	CueTarget = Target;
	const bool bResult = Super::OnExecute_Implementation(Target, ResolveParameters(Target, Parameters));
	ApplyNiagaraParameters(RecurringSpawnResults);
	PlaceRecurringMesh(Target);
	return bResult;
}

bool AAgGameplayCueNotify_Looping::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	StopTargetTimeDilation();
	if (PostProcess->IsPlaying())
	{
		// 엔진의 즉시 회수는 페이드가 끝난 뒤 컴포넌트의 완료 콜백이 대신합니다.
		bAutoDestroyOnRemove = !PostProcess->Stop();
	}
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

#if WITH_EDITOR
EDataValidationResult AAgGameplayCueNotify_Looping::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	FString Error;
	if (!PostProcess->ValidateSettings(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

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
			if (!PlacementEndSocketName.IsNone())
			{
				PlaceBetweenSockets(*Niagara);
			}
		}
	}
	ApplyTargetTimeDilation(SpawnResult);
}

void AAgGameplayCueNotify_Looping::PlaceBetweenSockets(UNiagaraComponent& Niagara) const
{
	const USceneComponent* Parent = Niagara.GetAttachParent();
	const FName StartSocketName = Niagara.GetAttachSocketName();
	if (!Parent || !Parent->DoesSocketExist(StartSocketName) || !Parent->DoesSocketExist(PlacementEndSocketName))
	{
		UE_LOG(LogAg, Error, TEXT("%s: %s has no socket %s or %s"), *GetName(), *GetNameSafe(Parent), *StartSocketName.ToString(), *PlacementEndSocketName.ToString());
		return;
	}

	const FVector Start = Parent->GetSocketLocation(StartSocketName);
	const FVector End = Parent->GetSocketLocation(PlacementEndSocketName);
	Niagara.SetWorldLocationAndRotation((Start + End) * 0.5, FRotationMatrix::MakeFromX(End - Start).Rotator());
	if (!NiagaraSocketDistanceParameter.IsNone())
	{
		Niagara.SetVariableFloat(NiagaraSocketDistanceParameter, FVector::Distance(Start, End));
	}
}

void AAgGameplayCueNotify_Looping::ApplyTargetTimeDilation(const FGameplayCueNotify_SpawnResult& SpawnResult) const
{
	const AActor* Target = CueTarget.Get();
	if (!bUseTargetTimeDilation || !Target)
	{
		return;
	}
	for (UFXSystemComponent* FX : SpawnResult.FxSystemComponents)
	{
		if (UNiagaraComponent* Niagara = Cast<UNiagaraComponent>(FX))
		{
			Niagara->SetCustomTimeDilation(Target->CustomTimeDilation);
		}
	}
}

void AAgGameplayCueNotify_Looping::StopTargetTimeDilation()
{
	SetActorTickEnabled(false);
	if (bUseTargetTimeDilation)
	{
		// 꺼진 뒤 남은 꼬리는 월드 시간으로 사라지게 합니다. 풀로 돌아간 나이아가라가 멈춘 배율을 가져가지 않게 합니다.
		for (UFXSystemComponent* FX : LoopingSpawnResults.FxSystemComponents)
		{
			if (UNiagaraComponent* Niagara = Cast<UNiagaraComponent>(FX))
			{
				Niagara->SetCustomTimeDilation(1.f);
			}
		}
	}
	CueTarget.Reset();
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
