// Copyright Woogle. All Rights Reserved.

#include "Interaction/AgGate.h"

#include "Components/BoxComponent.h"
#include "Components/InterpToMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AgCollision.h"
#include "Engine/CollisionProfile.h"

AAgGate::AAgGate()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(RootComponent);
	Door->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// 닫힌 성문 너머로 길을 찾을 일은 없고, 내비메시를 끊어 두면 열린 뒤에도 통로로 길을 찾지 못합니다.
	Door->SetCanEverAffectNavigation(false);

	InteractionRange = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(RootComponent);
	InteractionRange->SetCollisionProfileName(AgCollision::InteractionRangeProfile);

	PromptPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PromptPoint"));
	PromptPoint->SetupAttachment(RootComponent);

	OpenMovement = CreateDefaultSubobject<UInterpToMovementComponent>(TEXT("OpenMovement"));
	// 엔진 기본은 시작하자마자 움직이므로, 상호작용할 때 켭니다.
	OpenMovement->bAutoActivate = false;
	// 문에 충돌이 있어, 쓸면서 움직이면 문 위의 벽에 닿는 순간 멈춥니다.
	OpenMovement->bSweep = false;
}

FText AAgGate::GetInteractionText() const
{
	return InteractionText;
}

FVector AAgGate::GetInteractionLocation() const
{
	return PromptPoint->GetComponentLocation();
}

void AAgGate::Interact(AAgPlayerCharacter& Player)
{
	// 열리기 시작한 성문은 상호작용 후보에서 빠집니다.
	InteractionRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OpenMovement->Activate();
}

void AAgGate::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// 이동 컴포넌트는 따로 정하지 않으면 루트를 움직이고, BeginPlay에서 움직일 컴포넌트의 자리를 출발점으로 기록합니다.
	OpenMovement->SetUpdatedComponent(Door);
}
