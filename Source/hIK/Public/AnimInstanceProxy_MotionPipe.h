// Copyright (c) Mathew Wang 2021
#pragma once
#include "Animation/AnimInstanceProxy.h"
#include "articulated_body.h"
#include "transform_helper.h"
#include "AnimInstanceProxy_MotionPipe.generated.h"

class UAnimInstance_MotionPipe;

struct EndEF
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

	FORCEINLINE void PushUpdateEEF(const EndEF& eef)
	{
		m_endEEFs.Add(eef);
	}

	FORCEINLINE void PushUpdateEEFs(const TArray<EndEF>& eefs)
	{
		m_endEEFs = eefs;
	}

	FORCEINLINE void PullUpdateEEFs(TArray<EndEF>& eefs) const
	{
		eefs = m_endEEFs;
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

	FORCEINLINE bool EmptyEndEEFs() const
	{
		return 1 > m_endEEFs.Num();
	}
#endif

private:
	UAnimInstance_MotionPipe* m_animInst;

	TArray<EndEF> m_endEEFs;

	FTransform m_tmEntity;

	mutable TArray<int32> m_numFrames;

#ifdef _DEBUG
	int c_validPtr;
#endif

};