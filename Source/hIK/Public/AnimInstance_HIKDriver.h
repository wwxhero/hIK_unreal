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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	FString BVHPath_;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	int32 NUM_Frames_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	int32 I_Frame_;
public:
	UAnimInstance_HIKDriver()
		: NUM_Frames_(0)
		, I_Frame_(-1)
		, m_hBVH(H_INVALID)
	{}
	~UAnimInstance_HIKDriver() {}
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	HBVH m_hBVH;
};
