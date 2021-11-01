#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_MotionPipe.h"
#include "Misc/ScopeTryLock.h"
#include "AnimInstance_HIKDrivee.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDrivee : public UAnimInstance_MotionPipe
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	int32 DBG_VisBody_i;
public:
	UAnimInstance_HIKDrivee()
		: DBG_VisBody_i(0)
	{
		FileConfName = FString("HIK.xml");
	}

	~UAnimInstance_HIKDrivee()
	{
	}

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	virtual void PreUpdateAnimation(float DeltaSeconds) override;


	FORCEINLINE void PushUpdateTargets(const TArray<Target>& targets)
	{
		m_targets = targets;
	}

private:
	virtual void OnPreUpdate(FAnimInstanceProxy_MotionPipe* proxy) const override;
	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy) override;

private:
	TArray<Target> m_targets;
};
