#pragma once
#include <list>
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_MotionPipe.h"
#include "Misc/ScopeTryLock.h"
#include "bvh.h"
#include "AnimInstance_HIKDriver.generated.h"

class UAnimInstance_HIKDrivee;

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDriver : public UAnimInstance_MotionPipe
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	int32 NUM_Frames_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	int32 I_Frame_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	TArray<UAnimInstance_HIKDrivee*> Drivees_;
public:
	UAnimInstance_HIKDriver()
		: NUM_Frames_(0)
		, I_Frame_(-1)
	{}
	~UAnimInstance_HIKDriver() {}
private:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual FString GetFileConfName() const override;

	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy);
};
