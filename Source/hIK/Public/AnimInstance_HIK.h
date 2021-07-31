#pragma once

#include "Animation/AnimInstance.h"
#include "Misc/ScopeTryLock.h"
#include "articulated_body.h"

#include "AnimInstance_HIK.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIK : public UAnimInstance
{
	GENERATED_BODY()
public:
	typedef struct
	{
		HBODY h_body;
		FTransform tm_l2w;
	} Target;

	FORCEINLINE TArray<Target>* LockTarget(bool force = false)
	{
		bool locked = false;
		if (force)
		{
			m_lockerTargets.Lock();
			locked = true;
		}
		else
			locked = m_lockerTargets.TryLock();


		if (locked)
			return &m_targets;
		else
			return NULL;
	}

	virtual void UnLockTarget(TArray<Target>* targets)
	{
		if (NULL != targets)
			m_lockerTargets.Unlock();
	}
protected:
	TArray<Target> m_targets;
	FCriticalSection m_lockerTargets;
};