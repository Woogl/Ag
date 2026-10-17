// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AgAnimNotify_SpawnProjectile.generated.h"

UENUM()
enum class EAgProjectileOrigin : uint8
{
	CharacterFeetForward,
	MeshSocket
};

UCLASS(meta = (DisplayName = "Ag Spawn Projectile"))
class UAgAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName ParamsId;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	EAgProjectileOrigin Origin = EAgProjectileOrigin::CharacterFeetForward;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName SocketName;
	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Reference) override;
	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("SpawnProjectile/") + ParamsId.ToString();
	}
};
