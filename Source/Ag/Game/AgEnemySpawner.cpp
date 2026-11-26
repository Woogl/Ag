// Copyright Woogle. All Rights Reserved.

#include "Game/AgEnemySpawner.h"

#include "Ag.h"
#include "AI/AgPatrolRoute.h"
#include "Character/AgEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Core/AgAssetManager.h"
#include "Data/AgAbilityData.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#if WITH_EDITORONLY_DATA
#include "AI/AgPatrolPreviewComponent.h"
#include "Character/AgEnemySightPreviewComponent.h"
#endif

#if WITH_EDITOR
#include "Components/SkeletalMeshComponent.h"
#include "Data/AgCharacterData.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "EngineUtils.h"
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"
#endif

AAgEnemySpawner::AAgEnemySpawner()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	SightPreview = CreateEditorOnlyDefaultSubobject<UAgEnemySightPreviewComponent>(TEXT("SightPreview"));
	PatrolPreview = CreateEditorOnlyDefaultSubobject<UAgPatrolPreviewComponent>(TEXT("PatrolPreview"));
#endif
}

void AAgEnemySpawner::Spawn()
{
	if (Enemy || bSpawnPending || EnemyClass.IsNull())
	{
		return;
	}
	if (LoadedClass)
	{
		SpawnEnemy();
		return;
	}
	bSpawnPending = true;
	LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(EnemyClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &ThisClass::HandleClassLoaded));
	// 엔진은 경로가 잘못된 요청에는 핸들도 완료 알림도 주지 않습니다.
	if (!LoadHandle)
	{
		HandleClassLoaded();
	}
}

void AAgEnemySpawner::DestroyEnemy()
{
	if (Enemy)
	{
		Enemy->Destroy();
		Enemy = nullptr;
	}
}

#if WITH_EDITOR
const AAgEnemyCharacter* AAgEnemySpawner::GetEnemyDefaults() const
{
	const UClass* Class = EnemyClass.LoadSynchronous();
	return Class ? Class->GetDefaultObject<AAgEnemyCharacter>() : nullptr;
}

void AAgEnemySpawner::SetEnemyClass(TSubclassOf<AAgEnemyCharacter> InEnemyClass)
{
	EnemyClass = InEnemyClass;
	UpdatePreview();
}

void AAgEnemySpawner::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	UpdatePreview();
}

void AAgEnemySpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PatrolRoute) || PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, EnemyClass))
	{
		TArray<TSharedRef<FTokenizedMessage>> Messages;
		CollectPatrolProblems(Messages);
		AAgPatrolRoute::ReportEditProblems(Messages);
	}
}

void AAgEnemySpawner::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		TArray<TSharedRef<FTokenizedMessage>> Messages;
		CollectPatrolProblems(Messages);
		AAgPatrolRoute::ReportEditProblems(Messages);
	}
}

void AAgEnemySpawner::CheckForErrors()
{
	Super::CheckForErrors();

	TArray<TSharedRef<FTokenizedMessage>> Messages;
	if (EnemyClass.IsNull())
	{
		Messages.Add(AAgPatrolRoute::MakeWarning(*this, TEXT("EnemyClass is not set, so nothing spawns here.")));
	}
	CollectPatrolProblems(Messages);
	CollectGroupProblems(Messages);
	FMessageLog(TEXT("MapCheck")).AddMessages(Messages);
}
#endif

void AAgEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	Spawn();
}

void AAgEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LoadHandle)
	{
		LoadHandle->CancelHandle();
		LoadHandle.Reset();
	}
	if (bSpawnPending)
	{
		bSpawnPending = false;
		OnSpawnFinished.Broadcast();
	}
	// 스포너만 내려가면(월드 파티션 스트리밍, 삭제) 적이 남으므로 함께 지웁니다. 월드가 끝날 때는 엔진이 모든 액터를 정리합니다.
	if (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		DestroyEnemy();
	}

	Super::EndPlay(EndPlayReason);
}

void AAgEnemySpawner::HandleClassLoaded()
{
	LoadedClass = EnemyClass.Get();
	if (!LoadedClass)
	{
		UE_LOG(LogAg, Error, TEXT("%s: can't load EnemyClass %s."), *GetName(), *EnemyClass.ToString());
		LoadHandle.Reset();
		bSpawnPending = false;
		OnSpawnFinished.Broadcast();
		return;
	}
	const FName CharacterDataId = LoadedClass->GetDefaultObject<AAgEnemyCharacter>()->GetCharacterDataId();
	LoadHandle = UAgAssetManager::Get().PreloadData<UAgAbilitySet>(CharacterDataId, FStreamableDelegate::CreateUObject(this, &ThisClass::SpawnEnemy));
}

void AAgEnemySpawner::SpawnEnemy()
{
	const bool bWaited = bSpawnPending;
	bSpawnPending = false;

	AbilitySet = UAgAssetManager::Get().LoadData<UAgAbilitySet>(LoadedClass->GetDefaultObject<AAgEnemyCharacter>()->GetCharacterDataId());
	// 미리 읽은 에셋을 묶음이 붙잡은 뒤에 핸들을 놓습니다.
	LoadHandle.Reset();

	// 스포너의 위치는 적의 발밑이고, 적 액터의 위치는 캡슐 중심입니다.
	const float HalfHeight = LoadedClass->GetDefaultObject<AAgEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FTransform Transform(GetActorRotation(), GetActorLocation() + GetActorUpVector() * HalfHeight);
	Enemy = GetWorld()->SpawnActorDeferred<AAgEnemyCharacter>(LoadedClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Enemy)
	{
		// 그룹 등록(BeginPlay)과 AI 시작(컴포넌트 초기화)이 생성을 마칠 때 일어나므로 그 전에 줍니다.
		Enemy->SetPlacement(PatrolRoute, GroupId);
		Enemy->FinishSpawning(Transform);
	}
	if (bWaited)
	{
		OnSpawnFinished.Broadcast();
	}
}

#if WITH_EDITOR
void AAgEnemySpawner::UpdatePreview()
{
	// 적 외형은 편집 화면에서만 보면 되므로 저장하지 않는 컴포넌트로 만듭니다. 게임 월드와 쿡에서는 만들지 않아 적 블루프린트를
	// 읽지 않습니다.
	if (IsRunningCommandlet() || !GetWorld() || GetWorld()->WorldType != EWorldType::Editor)
	{
		return;
	}
	for (UMeshComponent* Component : PreviewComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	PreviewComponents.Reset();

	const AAgEnemyCharacter* Defaults = GetEnemyDefaults();
	if (!Defaults)
	{
		return;
	}
	const USkeletalMeshComponent* Source = Defaults->GetMesh();
	USkeletalMeshComponent* Body = NewObject<USkeletalMeshComponent>(this, NAME_None, RF_Transient | RF_TextExportTransient);
	Body->SetSkeletalMeshAsset(Source->GetSkeletalMeshAsset());
	Body->OverrideMaterials = Source->OverrideMaterials;
	Body->SetAnimInstanceClass(Source->AnimClass);
	// 메시의 자리는 캡슐 중심 기준이므로, 발밑인 스포너에서는 캡슐 반높이만큼 올립니다.
	FTransform BodyTransform = Source->GetRelativeTransform();
	BodyTransform.AddToTranslation(FVector(0., 0., Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	Body->SetRelativeTransform(BodyTransform);
	AddPreviewComponent(*Body, *RootComponent, NAME_None);

	// 블루프린트에서 더한 컴포넌트(무기)는 기본 객체에 없으므로 블루프린트의 컴포넌트 템플릿을 본떠 만듭니다. 몸이나 이미 만든
	// 미리 보기에 붙는 메시만 다루고, 부모 블루프린트의 것부터 만들어 자식이 붙을 자리를 찾게 합니다.
	UBlueprintGeneratedClass* EnemyBlueprintClass = Cast<UBlueprintGeneratedClass>(Defaults->GetClass());
	TArray<const UBlueprintGeneratedClass*> BlueprintClasses;
	UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(Defaults->GetClass(), BlueprintClasses);
	TMap<FName, USceneComponent*> PreviewsByName;
	PreviewsByName.Add(Source->GetFName(), Body);
	for (int32 Index = BlueprintClasses.Num() - 1; Index >= 0; --Index)
	{
		const USimpleConstructionScript* ConstructionScript = BlueprintClasses[Index]->SimpleConstructionScript;
		if (!ConstructionScript)
		{
			continue;
		}
		for (USCS_Node* Node : ConstructionScript->GetAllNodes())
		{
			const USCS_Node* ParentNode = ConstructionScript->FindParentNode(Node);
			USceneComponent* Parent = PreviewsByName.FindRef(ParentNode ? ParentNode->GetVariableName() : Node->ParentComponentOrVariableName);
			UMeshComponent* Template = Cast<UMeshComponent>(Node->GetActualComponentTemplate(EnemyBlueprintClass));
			if (!Parent || !Template)
			{
				continue;
			}
			UMeshComponent* Preview = NewObject<UMeshComponent>(this, Template->GetClass(), NAME_None, RF_Transient | RF_TextExportTransient, Template);
			// 템플릿의 생성 방식(블루프린트 구성)을 그대로 두면 엔진이 이 액터를 다시 구성할 때 이 컴포넌트를 지웁니다.
			Preview->CreationMethod = EComponentCreationMethod::Native;
			AddPreviewComponent(*Preview, *Parent, Node->AttachToName);
			PreviewsByName.Add(Node->GetVariableName(), Preview);
		}
	}
}

void AAgEnemySpawner::AddPreviewComponent(UMeshComponent& Component, USceneComponent& Parent, FName SocketName)
{
	Component.SetupAttachment(&Parent, SocketName);
	Component.SetIsVisualizationComponent(true);
	Component.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component.RegisterComponent();
	PreviewComponents.Add(&Component);
}

void AAgEnemySpawner::CollectPatrolProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const
{
	const AAgEnemyCharacter* Defaults = GetEnemyDefaults();
	if (!Defaults || !PatrolRoute || PatrolRoute->GetPointCount() == 0)
	{
		return;
	}
	const UAgCharacterData* Data = UAgCharacterData::Find(Defaults->GetCharacterDataId());
	if (Data && Data->Attributes.MOV <= 0.f)
	{
		OutMessages.Add(AAgPatrolRoute::MakeWarning(*this, TEXT("MOV is 0, so this enemy does not walk its patrol route.")));
		return;
	}
	const int32 NearestIndex = PatrolRoute->FindNearestPoint(GetActorLocation());
	TArray<FVector> Path;
	if (AAgPatrolRoute::FindWalkPath(*GetWorld(), GetActorLocation(), PatrolRoute->GetPointLocation(NearestIndex), Path) == EAgPatrolPathResult::Unreachable)
	{
		OutMessages.Add(AAgPatrolRoute::MakeWarning(
			*this, FString::Printf(TEXT("Cannot walk from the start position to patrol point %d of %s."), NearestIndex + 1, *PatrolRoute->GetActorLabel())));
	}
}

void AAgEnemySpawner::CollectGroupProblems(TArray<TSharedRef<FTokenizedMessage>>& OutMessages) const
{
	if (GroupId.IsNone())
	{
		return;
	}
	for (TActorIterator<AAgEnemySpawner> It(GetWorld()); It; ++It)
	{
		if (*It != this && It->GroupId == GroupId)
		{
			return;
		}
	}
	OutMessages.Add(AAgPatrolRoute::MakeWarning(*this, FString::Printf(TEXT("No other loaded spawner has group '%s'. Check the name."), *GroupId.ToString())));
}
#endif
