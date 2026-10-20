// Copyright Woogle. All Rights Reserved.

#include "Data/AgControlsGuideData.h"

#include "InputAction.h"
#include "InputMappingContext.h"

TArray<FAgControlsGuideEntry> UAgControlsGuideData::MakeEntries(const UInputMappingContext& Mapping) const
{
	TArray<const UInputAction*> Actions;
	TMap<const UInputAction*, TArray<FText>> KeysByAction;
	for (const FEnhancedActionKeyMapping& KeyMapping : Mapping.GetMappings())
	{
		if (!KeyMapping.Action)
		{
			continue;
		}
		TArray<FText>& Keys = KeysByAction.FindOrAdd(KeyMapping.Action);
		if (Keys.IsEmpty())
		{
			Actions.Add(KeyMapping.Action);
		}
		const FText* DisplayName = KeyDisplayNames.Find(KeyMapping.Key.GetFName());
		Keys.Add(DisplayName ? *DisplayName : KeyMapping.Key.GetDisplayName(false));
	}

	TArray<FAgControlsGuideEntry> Entries;
	for (const UInputAction* Action : Actions)
	{
		FAgControlsGuideEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Keys = FText::Join(FText::FromString(TEXT(", ")), KeysByAction[Action]);
		Entry.Action = Action->ActionDescription;
		if (const FText* Description = ActionDescriptions.Find(Action->GetFName()))
		{
			Entry.Description = *Description;
		}
	}
	return Entries;
}
