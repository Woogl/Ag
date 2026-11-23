// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "UObject/SoftObjectPtr.h"
#include "AgLevelToolset.generated.h"

class ABrush;
class UBrushBuilder;
class UMaterialInterface;

/**
 * 에디터에 열린 레벨의 BSP 브러시를 다룹니다. 레벨은 저장하지 않으므로 에셋 도구로 저장합니다.
 * 브러시 세팅은 빌더 속성 이름과 값의 JSON입니다(디테일 패널의 Brush Settings와 같은 이름, 엔진 ObjectTools.set_properties와 같은 형식).
 */
UCLASS(Hidden, Config = Editor, DefaultConfig)
class UAgLevelToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 액터 배치 패널의 지오메트리를 끌어다 놓는 것과 같이 브러시를 배율 (1, 1, 1)로 놓습니다. 지오메트리에는 바로 반영됩니다.
	 * @param BuilderClass 모양. /Script/UnrealEd.CubeBuilder(박스), CylinderBuilder(원기둥), ConeBuilder(원뿔),
	 *        LinearStairBuilder(직선 계단), CurvedStairBuilder(굽은 계단), SpiralStairBuilder(나선 계단), TetrahedronBuilder(구),
	 *        SheetBuilder(판), VolumetricBuilder(볼류메트릭)
	 * @param Location 브러시 액터의 월드 위치(cm). 모양에서 이 점의 자리는 빌더마다 다릅니다.
	 *        박스·원기둥·구·판·볼류메트릭은 가운데, 원뿔은 밑면 가운데, 나선 계단은 축입니다.
	 *        직선 계단은 이 점에서 +X, +Y 쪽으로 뻗습니다. 첫 단의 윗면이 이 점 높이, 마지막 단의 윗면이 (NumSteps - 1) × StepHeight 위,
	 *        밑면이 StepHeight 아래입니다.
	 *        굽은 계단은 이 점이 굽는 축입니다. +X 쪽에서 시작해 +Y 쪽(CounterClockwise면 -Y 쪽)으로 AngleOfCurve만큼 돌며 오르고,
	 *        축에서 InnerRadius ~ InnerRadius + StepWidth를 차지합니다. 첫 단의 윗면이 StepHeight 위, 마지막 단의 윗면이
	 *        NumSteps × StepHeight 위, 밑면이 이 점 높이입니다.
	 * @param bSubtract true면 빼기 브러시(구멍, 홈)입니다.
	 * @param Settings 브러시 세팅. 예: CubeBuilder면 {"X":400,"Y":200,"Z":50}. 비우면 빌더의 기본값입니다.
	 *        원기둥·원뿔의 OuterRadius는 축에서 꼭짓점이 아니라 옆면까지의 거리입니다.
	 * @param Material 모든 면의 재질. 비우면 Config/DefaultEditor.ini의 BrushMaterial입니다.
	 * @return 놓은 브러시. 열린 레벨이 없거나, 빌더 클래스·재질·세팅이 잘못되었거나, 에디터가 브러시를 놓지 못했으면 null이고 오류를 알립니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static ABrush* AddBrush(TSubclassOf<UBrushBuilder> BuilderClass, FVector Location, FRotator Rotation, bool bSubtract, const FString& Settings, UMaterialInterface* Material = nullptr);

	/**
	 * 브러시의 브러시 세팅을 바꿔(크기 등) 모양을 다시 만들고 지오메트리를 다시 빌드합니다. 볼륨도 됩니다.
	 * @param Settings 바꿀 브러시 세팅만. 예: CubeBuilder면 {"X":600}
	 * @return 브러시 세팅이 없는 브러시(배율로 크기를 정한 옛 브러시 등)거나 세팅이 잘못되었으면 false이고 오류를 알립니다.
	 *         세팅이 잘못되었으면 브러시를 바꾸지 않습니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetBrushSettings(ABrush* Brush, const FString& Settings);

	/**
	 * 열린 레벨의 지오메트리를 에디터가 브러시를 옮길 때처럼 다시 빌드하며, 실행 취소 기록은 비우지 않습니다. 브러시를 옮기거나 돌리거나
	 * 더하기·빼기를 바꾼 뒤 부릅니다. 그 전에는 보이는 모양과 충돌이 옛 모양으로 남습니다.
	 * @return 열린 레벨이 없거나 에디터 환경설정의 'BSP 자동 업데이트'가 꺼져 있으면 false이고, 꺼져 있으면 오류를 알립니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool RebuildGeometry();

private:
	UPROPERTY(Config)
	TSoftObjectPtr<UMaterialInterface> BrushMaterial;
};
