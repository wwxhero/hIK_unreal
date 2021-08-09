// Copyright (c) Mathew Wang 2021

#pragma once
#include <stack>
#include <set>
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimNode_MotionPipe.h"
#include "AnimInstance_HIKDriver.h"
#include "ik_logger_unreal.h"
#include "AnimNode_FKRecordUT.generated.h"



USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_FKRecordUT : public FAnimNode_MotionPipe
{
	GENERATED_USTRUCT_BODY()

private:

	struct Target_Internal : public FAnimNode_MotionPipe::EndEF_Internal
	{
		FString name_fbx;

	};

	void InitializeTarget_Internal(Target_Internal* target, const FString& name_fbx, const FString& name_sim, const FTransform& a_tm_l2w, HBODY a_body)
	{
		InitializeEEF_Internal(target, name_sim, a_tm_l2w, a_body);
		target->name_fbx = name_fbx;
	}

public:
	FAnimNode_FKRecordUT();
	virtual ~FAnimNode_FKRecordUT();

protected:
	// FAnimNode_MotionPipe interface
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual HBODY InitializeBodySim_AnyThread(HBODY body_fbx) override;
	virtual void InitializeEEFs_AnyThread(FAnimInstanceProxy_MotionPipe* proxy, TArray<EndEF_Internal>& eefs) override;
	// End FAnimNode_MotionPipe Interface
#if defined _DEBUG
	void DBG_VisSIM(FAnimInstanceProxy* animProxy) const;
#endif


private:
	const UAnimInstance_HIKDriver* c_animInstDriver;
};
