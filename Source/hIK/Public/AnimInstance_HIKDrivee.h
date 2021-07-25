#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_HIK.h"
#include "Misc/ScopeTryLock.h"
#include "bvh.h"
#include "conf_mopipe.h"
#include "AnimInstance_HIKDrivee.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDrivee : public UAnimInstance_HIK
{
	GENERATED_BODY()
public:
	UAnimInstance_HIKDrivee()
	{}
	~UAnimInstance_HIKDrivee() {}

	virtual void PreUpdateAnimation(float DeltaSeconds) override;
	virtual FString GetFileConfName() const override;
private:

};
