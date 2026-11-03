// Copyright Woogle. All Rights Reserved.

#include "UI/AgCharacterPreview.h"

#include "Character/AgCharacterBase.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

AAgCharacterPreview::AAgCharacterPreview()
{
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	SetRootComponent(Capture);
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// 띄운 캐릭터를 보여 주는 화면은 게임을 멈춰 두므로, 멈춘 동안에도 매 프레임 그립니다.
	Capture->PrimaryComponentTick.bTickEvenWhenPaused = true;

	// 띄운 캐릭터와 이 조명만 라이팅 채널 1에 두어, 레벨의 조명과 서로 비추지 않고 레벨에 그림자도 생기지 않게 합니다.
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Capture);
	Light->LightingChannels.bChannel0 = false;
	Light->LightingChannels.bChannel1 = true;
}

void AAgCharacterPreview::ShowCharacter(TSubclassOf<AAgCharacterBase> CharacterClass)
{
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(RenderTargetSize.X, RenderTargetSize.Y);
	Capture->TextureTarget = RenderTarget;

	const FTransform SpawnTransform(GetActorRotation() + FRotator(0., 180., 0.), GetActorLocation());
	Character = GetWorld()->SpawnActorDeferred<AAgCharacterBase>(CharacterClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	// 적은 스폰하면 AI 컨트롤러가 빙의해 행동을 시작하므로 모습만 보이게 빙의를 끕니다.
	Character->AutoPossessAI = EAutoPossessAI::Disabled;
	Character->FinishSpawning(SpawnTransform);
	// 캐릭터 데이터가 없으면 캐릭터는 스폰 중에 스스로 지워집니다.
	if (!IsValid(Character))
	{
		return;
	}

	// 메시를 키운 캐릭터(보스)는 캡슐보다 커서 메시 범위로 맞춥니다. 범위의 가운데를 캡처 높이에 두고,
	// 여유를 둔 범위 높이가 세로 시야에 꼭 맞는 거리에 세웁니다.
	const FBoxSphereBounds Bounds = Character->GetMesh()->Bounds;
	const double Aspect = static_cast<double>(RenderTargetSize.X) / RenderTargetSize.Y;
	const double Distance = Bounds.BoxExtent.Z * FramingScale * Aspect / FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle * 0.5));
	Character->SetActorLocation(GetActorLocation() + GetActorForwardVector() * Distance + FVector(0., 0., Character->GetActorLocation().Z - Bounds.Origin.Z));

	TInlineComponentArray<UPrimitiveComponent*> Primitives(Character);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		Primitive->SetVisibleInSceneCaptureOnly(true);
		Primitive->SetLightingChannels(false, true, false);
		if (USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Primitive))
		{
			Mesh->SetTickableWhenPaused(true);
		}
	}
	Capture->ShowOnlyActorComponents(Character);
}
