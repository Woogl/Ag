// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AgBlueprintToolset.generated.h"

class UBlueprint;
class UWidgetBlueprint;

/** 고친 에셋은 저장하지 않으므로 에셋 도구로 저장합니다. */
UCLASS(Hidden)
class UAgBlueprintToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 블루프린트에서 더한 컴포넌트를 부모 컴포넌트의 소켓에 붙입니다(디테일 패널의 Parent Socket과 같음).
	 * C++에서 만든 상속 컴포넌트는 에디터처럼 바꿀 수 없습니다. 블루프린트를 컴파일합니다.
	 * @return 블루프린트에 그 이름의 컴포넌트가 없으면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetComponentParentSocket(UBlueprint* Blueprint, FName ComponentName, FName SocketName);

	/**
	 * 위젯 애니메이션 AnimationName을 만들거나 바꿉니다. WidgetName 위젯의 렌더 불투명도가 Duration초 동안 StartOpacity에서
	 * EndOpacity로 선형으로 바뀝니다.
	 * @return 위젯 트리에 그 위젯이 없거나 Duration이 0 이하면 false입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity);

	/** 위젯 애니메이션 AnimationName을 위젯 디자이너와 같은 방식으로 지웁니다. 없었으면 false입니다. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool RemoveWidgetAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName);
};
