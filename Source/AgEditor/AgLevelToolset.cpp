// Copyright Woogle. All Rights Reserved.

#include "AgLevelToolset.h"

#include "Editor.h"
#include "Engine/Brush.h"
#include "Engine/BrushBuilder.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LevelUtils.h"
#include "ScopedTransaction.h"
#include "Settings/LevelEditorMiscSettings.h"
#include "ToolsetRegistry/ToolsetLibrary.h"
#include "UObject/Package.h"

namespace AgLevelToolsetPrivate
{
	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	// 엔진의 SetObjectProperties는 JSON으로 읽히지 않는 문자열이면 오류 없이 false만 돌려줍니다.
	void RaiseSettingsError()
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("브러시 세팅을 넣지 못했습니다. 빌더 속성 이름과 값의 JSON이어야 합니다."));
	}
}

ABrush* UAgLevelToolset::AddBrush(TSubclassOf<UBrushBuilder> BuilderClass, FVector Location, FRotator Rotation, bool bSubtract, const FString& Settings)
{
	UWorld* World = AgLevelToolsetPrivate::GetEditorWorld();
	ABrush* DefaultBrush = World ? World->GetDefaultBrush() : nullptr;
	if (!DefaultBrush || !BuilderClass || BuilderClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("열린 레벨이 없거나 빌더 클래스가 비어 있거나 추상 클래스입니다."));
		return nullptr;
	}

	// 세팅이 잘못되면 레벨에 아무것도 남기지 않도록 임시 패키지에서 먼저 만들어 봅니다(액터 배치 패널도 임시 빌더를 복제해 씁니다).
	UBrushBuilder* Builder = NewObject<UBrushBuilder>(GetTransientPackage(), BuilderClass);
	if (!Settings.IsEmpty() && !UToolsetLibrary::SetObjectProperties(Builder, Settings))
	{
		AgLevelToolsetPrivate::RaiseSettingsError();
		return nullptr;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("AgLevelToolset", "AddBrush", "Add Brush"));
	// 액터 배치 패널과 같은 순서입니다. BRUSH 명령은 빌더 브러시의 모양, 변환, 빌더를 복사해 새 브러시를 만들므로,
	// 빌더 브러시를 이 빌더로 만들고 옮긴 뒤 빌더를 달아 둡니다. 빌더가 없으면 새 브러시에 브러시 세팅이 없습니다.
	if (!Builder->Build(World))
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("빌더가 이 세팅으로 모양을 만들지 못했습니다."));
		return nullptr;
	}
	DefaultBrush->Modify();
	DefaultBrush->SetActorTransform(FTransform(Rotation, Location));
	DefaultBrush->BrushBuilder = DuplicateObject<UBrushBuilder>(Builder, DefaultBrush->GetOuter());
	// BRUSH 명령은 브러시를 만들지 못하면 선택을 그대로 두므로, 전에 선택한 브러시를 돌려주지 않도록 선택을 비웁니다.
	GEditor->SelectNone(false, true);
	GEditor->Exec(World, bSubtract ? TEXT("BRUSH SUBTRACT SELECTNEWBRUSH") : TEXT("BRUSH ADD SELECTNEWBRUSH"));
	DefaultBrush->SetIsTemporarilyHiddenInEditor(true);
	ABrush* NewBrush = GEditor->GetSelectedActors()->GetTop<ABrush>();
	if (!NewBrush)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("에디터가 이 레벨에 브러시를 놓지 못했습니다."));
	}
	return NewBrush;
}

bool UAgLevelToolset::SetBrushSettings(ABrush* Brush, const FString& Settings)
{
	UBrushBuilder* Builder = Brush ? Brush->BrushBuilder.Get() : nullptr;
	UWorld* World = Brush ? Brush->GetWorld() : nullptr;
	if (!Builder || !World)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("브러시 세팅이 없는 브러시입니다."));
		return false;
	}

	// 엔진은 맞는 속성을 넣은 뒤에 틀린 속성을 알리고, 빌더는 모양을 만들 때에야 값을 검사합니다. 그래서 임시 복제본에 넣고
	// 그것으로 모양을 만들어 본 뒤에 브러시의 빌더에 넣습니다. 빌더는 값이 틀리면 브러시를 건드리지 않고 돌아옵니다.
	UBrushBuilder* Probe = DuplicateObject<UBrushBuilder>(Builder, GetTransientPackage());
	if (!UToolsetLibrary::SetObjectProperties(Probe, Settings))
	{
		AgLevelToolsetPrivate::RaiseSettingsError();
		return false;
	}

	FScopedTransaction Transaction(NSLOCTEXT("AgLevelToolset", "SetBrushSettings", "Set Brush Settings"));
	Brush->Modify();
	if (!Probe->Build(World, Brush))
	{
		Transaction.Cancel();
		UKismetSystemLibrary::RaiseScriptError(TEXT("빌더가 이 세팅으로 모양을 만들지 못했습니다."));
		return false;
	}
	Builder->Modify();
	UToolsetLibrary::SetObjectProperties(Builder, Settings);
	RebuildGeometry();
	return true;
}

bool UAgLevelToolset::RebuildGeometry()
{
	UWorld* World = AgLevelToolsetPrivate::GetEditorWorld();
	if (!World)
	{
		return false;
	}

	// 콘솔 MAP REBUILD는 다시 빌드하기 전에 실행 취소 기록을 비워, 저장 전에 지운 액터의 파일이 저장 때 지워지지 않고 남습니다.
	// 그래서 에디터가 브러시를 옮길 때 쓰는 레벨 단위 다시 빌드를 씁니다. 이것은 BSP 자동 업데이트가 꺼져 있으면 아무것도 하지 않습니다.
	if (!GetDefault<ULevelEditorMiscSettings>()->bBSPAutoUpdate)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("에디터 환경설정의 'BSP 자동 업데이트'가 꺼져 있어 지오메트리를 다시 빌드하지 못했습니다."));
		return false;
	}
	for (ULevel* Level : World->GetLevels())
	{
		if (Level && FLevelUtils::IsLevelVisible(Level))
		{
			GEditor->RebuildLevel(*Level);
		}
	}
	return true;
}
