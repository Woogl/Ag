// Copyright Woogle. All Rights Reserved.

#include "Data/AgCharacterData.h"

#include "AbilitySystem/AgGameplayAbility.h"
#include "Ag.h"
#include "Animation/AnimMontage.h"
#include "Core/AgAssetManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

void UAgCharacterData::ValidateProjectiles(TArray<FString>& Errors) const
{
	for (const TPair<FName, FAgProjectileDefinition>& Pair : Projectiles)
	{
		if (Pair.Value.Params.Speed <= 0.f || Pair.Value.Params.Range <= 0.f)
		{
			Errors.Add(FString::Printf(TEXT("Projectiles.%s: Speed and Range must be positive"), *Pair.Key.ToString()));
		}
	}
}

void UAgCharacterData::PostLoad()
{
	Super::PostLoad();

	// 수치·설정·에셋 참조는 JSON에서 읽고, 에셋에는 저장하지 않습니다.
	UAgAssetManager::Get().LoadJsonValues(*this, JsonFileName);
}

#if WITH_EDITOR
void UAgCharacterData::PostEditUndo()
{
	Super::PostEditUndo();

	// 실행 취소·다시 실행은 Transient 속성(파일에서 읽은 값)까지 그 편집 때의 값으로 되돌립니다. 기준은 파일이므로 다시 읽습니다.
	UAgAssetManager::Get().LoadJsonValues(*this, JsonFileName);
}

#endif

const FAgAbilityData* UAgCharacterData::FindAbility(FName Id) const
{
	return Abilities.FindByPredicate(
	    [Id](const FAgAbilityData& Ability)
	    {
		    return Ability.Id == Id;
	    });
}

UAnimMontage* UAgCharacterData::FindMontage(FName Id) const
{
	// 몽타주 ID는 코드에 적은 이름이나 어빌리티 ID라 컴파일러가 확인하지 못하므로, 없으면 바로 알립니다.
	UAnimMontage* Montage = Montages.FindRef(Id);
	if (!Montage)
	{
		UE_LOG(LogAg, Error, TEXT("%s has no montage %s in Montages."), *GetName(), *Id.ToString());
	}
	return Montage;
}
