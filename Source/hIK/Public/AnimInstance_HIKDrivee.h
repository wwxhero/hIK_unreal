#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_MotionPipe.h"
#include "Misc/ScopeTryLock.h"
#include "bvh.h"
#include "conf_mopipe.h"
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

	FORCEINLINE void UpdateEEF(int32 i_eef
							, const FTransform& tm_l2w
#ifdef _DEBUG
							, const FString& name
#endif
							)
	{
		m_eefs[i_eef].tm_l2w = tm_l2w;
#ifdef _DEBUG
		check(m_eefs[i_eef].name == name);
#endif
	}

	FORCEINLINE int32 N_eefs() const
	{
		return m_eefs.Num();
	}


private:
	virtual void OnPreUpdate(TArray<EndEF>& eefs_i) const override;
	virtual void OnPostUpdate(const TArray<EndEF>& eefs_0) override;

private:
	TArray<EndEF> m_eefs;
};
