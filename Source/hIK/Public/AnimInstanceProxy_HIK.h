// Copyright (c) Mathew Wang 2021
#pragma once
#include "Animation/AnimInstanceProxy.h"
#include "articulated_body.h"
#include "transform_helper.h"
#include "AnimInstanceProxy_HIK.generated.h"

class UAnimInstance_MotionPipe;

struct EndEF
{
	FString name;
	FTransform tm_l2w;
	HBODY h_body;
};

FORCEINLINE void InitEndEF(EndEF* eef, HBODY h_body)
{
	eef->h_body = h_body;
	eef->name = body_name_w(h_body);
	FTransform& l2w_u = eef->tm_l2w;
	_TRANSFORM l2w_b;
	get_body_transform_l2w(h_body, &l2w_b);
	Convert(l2w_b, l2w_u);
}

struct FCompareEEF
{
	FORCEINLINE bool operator()(const EndEF& A, const EndEF& B) const
	{
		return A.name < B.name;
	}
};

USTRUCT(meta = (DisplayName = "Pass data amoung anim nodes"))
struct HIK_API FAnimInstanceProxy_HIK : public FAnimInstanceProxy
{
	GENERATED_USTRUCT_BODY()
public:
	FAnimInstanceProxy_HIK();
	FAnimInstanceProxy_HIK(UAnimInstance_MotionPipe* Instance);
	virtual ~FAnimInstanceProxy_HIK();

	/** Called before update so we can copy any data we need */
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);

	/** Called after update so we can copy any data we need */
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const;

	void RegisterEEF(HBODY hBody);

	const TArray<EndEF>& GetEEFs() const
	{
		return m_endEEFs;
	}

#ifdef _DEBUG
	inline bool ValidPtr()
	{
		return 404 == c_validPtr;
	}
#endif
private:
	UAnimInstance_MotionPipe* m_animInst;

	TArray<EndEF> m_endEEFs;
#ifdef _DEBUG
	int c_validPtr;
#endif

};