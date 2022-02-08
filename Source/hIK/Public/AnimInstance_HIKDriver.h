#pragma once
#include <list>
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_MotionPipe.h"
#include "Misc/ScopeTryLock.h"
#include "AnimInstance_HIKDriver.generated.h"

class UAnimInstance_HIKDrivee;

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDriver : public UAnimInstance_MotionPipe
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HIKAnim, meta = (PinShownByDefault))
	int32 NUM_Frames_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = HIKAnim, meta = (PinShownByDefault))
	int32 I_Frame_;
	UFUNCTION(BlueprintCallable, Category = HIKAnim, meta = (PinShownByDefault))
	void InitializeDrivees(const TArray<UAnimInstance_HIKDrivee*>& drivees);
public:
	UAnimInstance_HIKDriver()
		: NUM_Frames_(0)
		, I_Frame_(-1)
	{
		FileConfName = FString("FK.xml");
	}
	~UAnimInstance_HIKDriver() {}

private:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy);

private:
	TArray<UAnimInstance_HIKDrivee*> m_drivees;
};
