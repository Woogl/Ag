// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgCharacterPreview.generated.h"

class AAgCharacterBase;
class UPointLightComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

/** 캐릭터 하나를 AI 없이 띄워 정면에서 렌더 타깃에 그립니다. 띄운 캐릭터는 씬 캡처에만 보입니다. */
UCLASS(Abstract)
class AAgCharacterPreview : public AActor
{
	GENERATED_BODY()

public:
	AAgCharacterPreview();

	/** 캐릭터를 캡처 앞에 세우고 렌더 타깃을 만듭니다. 한 번만 부릅니다. */
	void ShowCharacter(TSubclassOf<AAgCharacterBase> CharacterClass);

	UTextureRenderTarget2D* GetRenderTarget() const
	{
		return RenderTarget;
	}

protected:
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(EditDefaultsOnly, Category = "Preview")
	FIntPoint RenderTargetSize = FIntPoint(600, 780);

	/** 메시 범위의 높이에 곱해 캡처의 세로 시야에 꼭 맞출 높이입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Preview", meta = (ClampMin = 1))
	float FramingScale = 1.1f;

private:
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY()
	TObjectPtr<AAgCharacterBase> Character;
};
