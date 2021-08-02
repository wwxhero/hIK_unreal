// Copyright (c) Mathew Wang 2021
#pragma once
#include "Animation/AnimInstanceProxy.h"
#include "AnimInstanceProxy_HIK.generated.h"

class UAnimInstance_HIK;

USTRUCT(meta = (DisplayName = "Pass data amoung anim nodes"))
struct HIK_API FAnimInstanceProxy_HIK : public FAnimInstanceProxy
{
	GENERATED_USTRUCT_BODY()
public:
	FAnimInstanceProxy_HIK();
	FAnimInstanceProxy_HIK(UAnimInstance_HIK* Instance);
	virtual ~FAnimInstanceProxy_HIK();

	/** Called before update so we can copy any data we need */
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds);

	/** Called after update so we can copy any data we need */
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const;

private:
	UAnimInstance_HIK* m_animInst;

};