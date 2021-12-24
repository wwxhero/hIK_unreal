// Copyright (c) Mathew Wang 2021
#pragma once
#include "Animation/AnimInstanceProxy.h"
#include "articulated_body.h"
#include "transform_helper.h"
#include "AnimInstanceProxy_MotionPipe.generated.h"

class UAnimInstance_MotionPipe;

struct Target
{
	FString name;
	FTransform tm_l2w;
};

USTRUCT(meta = (DisplayName = "Pass data amoung anim nodes"))
struct HIK_API FAnimInstanceProxy_MotionPipe : public FAnimInstanceProxy
{
	GENERATED_USTRUCT_BODY()
public:
	FAnimInstanceProxy_MotionPipe();
	FAnimInstanceProxy_MotionPipe(UAnimInstance_MotionPipe* Instance);
	virtual ~FAnimInstanceProxy_MotionPipe();

	/** Called before update so we can copy any data we need */
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);

	/** Called after update so we can copy any data we need */
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const;

	FORCEINLINE void PushUpdateTargets(const TArray<Target>& targets)
	{
		m_targets = targets;
	}

	FORCEINLINE void PullUpdateTargets(TArray<Target>& targets) const
	{
		targets = m_targets;
	}

	FORCEINLINE void PushUpdateEntity(const FTransform& tm)
	{
		m_tmEntity = tm;
	}

	FORCEINLINE void PullUpdateEntity(FTransform& tm) const
	{
		tm = m_tmEntity;
	}

	FORCEINLINE void PushUpdateNFrames(int32 nFrames)
	{
		m_numFrames.Add(nFrames);
	}

	FORCEINLINE bool PullUpdateNFrames(int32& nFrames) const
	{
		int32 n_numframes = m_numFrames.Num();
		bool exists_n = (n_numframes > 0);
		if (exists_n)
		{
			check (n_numframes == 1);
			nFrames = m_numFrames[0];
			m_numFrames.Empty();
		}
		return exists_n;
	}

#ifdef _DEBUG
	FORCEINLINE bool ValidPtr() const
	{
		return 404 == c_validPtr;
	}
#endif

	FORCEINLINE bool EmptyTargets() const
	{
		return 1 > m_targets.Num();
	}

private:
	UAnimInstance_MotionPipe* m_animInst;

	TArray<Target> m_targets;

	FTransform m_tmEntity;

	mutable TArray<int32> m_numFrames;

#ifdef _DEBUG
	int c_validPtr;
#endif

};