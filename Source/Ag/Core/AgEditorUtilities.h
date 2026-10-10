// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AgEditorUtilities.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class USkeleton;
class UWidgetBlueprint;

/** One piece of a montage: a time range of an animation. */
USTRUCT(BlueprintType)
struct FAgMontageSegment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float StartTime = 0.f;

	/** 0 plays to the end of the animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float EndTime = 0.f;
};

/** One montage section: where it starts and which section plays after it. */
USTRUCT(BlueprintType)
struct FAgMontageSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (Units = "s"))
	float StartTime = 0.f;

	/** The section that plays next: none ends the montage, the section's own name repeats it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	FName NextSection;
};

/**
 * Editor setup helpers for asset scripts (Python): engine settings the editor exposes only through its UI.
 * Not part of the game; the functions exist in editor builds only.
 */
UCLASS()
class UAgEditorUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** Puts a montage slot in its own slot group on the skeleton, so montages in different groups don't stop each other. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor")
	static bool SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName);

	/** Replaces the montage's track with the segments played back to back, in the given slot. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor")
	static bool SetMontageSegments(UAnimMontage* Montage, FName SlotName, const TArray<FAgMontageSegment>& Segments);

	/** Replaces the montage's sections and their order (for example a looping middle section). */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor")
	static bool SetMontageSections(UAnimMontage* Montage, const TArray<FAgMontageSection>& Sections);

	/**
	 * Creates or replaces the widget animation AnimationName: WidgetName's render opacity goes from StartOpacity to
	 * EndOpacity over Duration seconds in a straight line. The editor tools can't make widget animations.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor")
	static bool SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity);
#endif
};
