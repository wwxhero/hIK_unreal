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
	UAnimInstance_HIKDrivee() {}
	~UAnimInstance_HIKDrivee() {}

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	virtual void PreUpdateAnimation(float DeltaSeconds) override;
	virtual FString GetFileConfName() const override;


	FORCEINLINE void PushUpdateEEFs(const TArray<EndEF>& eefs)
	{
		m_eefs = eefs;
	}

private:
	virtual void OnPreUpdate(FAnimInstanceProxy_MotionPipe* proxy) const override;
	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy) override;

private:
	TArray<EndEF> m_eefs;
};
