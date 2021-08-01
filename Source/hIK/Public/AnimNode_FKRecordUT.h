// Copyright (c) Mathew Wang 2021

#pragma once
#include <stack>
#include <set>
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimNode_MotionPipe.h"
#include "AnimInstance_HIKDriver.h"
#include "ik_logger.h"
#include "AnimNode_FKRecordUT.generated.h"



USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_FKRecordUT : public FAnimNode_MotionPipe
{
	GENERATED_USTRUCT_BODY()

private:

public:
	FAnimNode_FKRecordUT();
	virtual ~FAnimNode_FKRecordUT();

protected:
	// FAnimNode_MotionPipe interface
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual HBODY InitializeBodySim_AnyThread(HBODY body_fbx) override;
	// End FAnimNode_MotionPipe Interface



private:
	const UAnimInstance_HIKDriver* c_animInstDriver;
};
