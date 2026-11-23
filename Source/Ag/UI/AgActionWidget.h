// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActionWidget.h"
#include "AgActionWidget.generated.h"

/**
 * 입력 액션의 키 아이콘입니다. 아이콘은 CommonInput 컨트롤러 데이터에서 현재 입력 종류와 키로 찾습니다.
 * 엔진은 Enhanced Input 액션의 첫 키 아이콘만 찾으므로, 키가 여럿인 액션(이동의 W·A·S·D)도 컨트롤러 데이터의 키 묶음
 * 아이콘을 찾도록 액션의 키를 모두 넘깁니다. 키 묶음을 여러 키 하나의 아이콘으로 찾는 것은 엔진의 데이터 테이블 액션과 같습니다.
 */
UCLASS()
class UAgActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:
	virtual FSlateBrush GetIcon() const override;
};
