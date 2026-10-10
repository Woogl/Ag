// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AgAssetToolset.generated.h"

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

	/** How fast this piece plays; below 1 it lasts longer in the montage (A1's slowed 예비 동작). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (ClampMin = 0.01))
	float PlayRate = 1.f;
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
 * Asset edits that the engine's MCP tools and editor Python can't make: montage sections, montage segments together
 * with the montage length, skeleton slot groups, widget animations. Registered as an MCP toolset by the AgEditor module.
 * Changed assets are left unsaved; save them with the asset tools.
 */
UCLASS(Hidden)
class UAgAssetToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Puts a montage slot in its own slot group on the skeleton, so montages in different groups don't stop each other.
	 * @return False if an argument is missing or the slot didn't end up in the group.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetSlotGroup(USkeleton* Skeleton, FName SlotName, FName GroupName);

	/**
	 * Replaces the montage's track with the segments played back to back, in the given slot.
	 * Sections that would start past the new end move to the start.
	 * @param Segments StartTime and EndTime are times in the segment's animation.
	 * @return False if the montage has no slot track, Segments is empty or a segment has no animation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetMontageSegments(UAnimMontage* Montage, FName SlotName, const TArray<FAgMontageSegment>& Segments);

	/**
	 * Replaces the montage's sections and their order (for example a looping middle section).
	 * @param Sections StartTime is a time in the montage.
	 * @return False if Sections is empty or a section name repeats.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetMontageSections(UAnimMontage* Montage, const TArray<FAgMontageSection>& Sections);

	/**
	 * Creates or replaces the widget animation AnimationName: WidgetName's render opacity goes from StartOpacity to
	 * EndOpacity over Duration seconds in a straight line.
	 * @return False if the widget isn't in the widget tree or Duration isn't positive.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool SetFadeAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName, FName WidgetName, float Duration, float StartOpacity, float EndOpacity);

	/** Deletes the widget animation AnimationName the way the widget designer does. False if there was none. */
	UFUNCTION(BlueprintCallable, Category = "Ag|Editor", meta = (AICallable))
	static bool RemoveWidgetAnimation(UWidgetBlueprint* WidgetBlueprint, FName AnimationName);
};
