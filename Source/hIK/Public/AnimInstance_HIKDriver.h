#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "bvh.h"
#include "AnimInstance_HIKDriver.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDriver : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	int32 NUM_Frames;
public:
	UAnimInstance_HIKDriver()
		: NUM_Frames(0)
		, m_hBVH(H_INVALID)
	{}
	~UAnimInstance_HIKDriver() {}
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	HBVH m_hBVH;
};
