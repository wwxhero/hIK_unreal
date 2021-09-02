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

public:
	FAnimNode_FKRecordUT();
	virtual ~FAnimNode_FKRecordUT();

protected:
	// FAnimNode_MotionPipe interface
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual void InitializeEEFs_AnyThread(HBODY body_fbx, const FTransform& skelecom_l2w, const std::set<FString> &eefs_name) override;
	// End FAnimNode_MotionPipe Interface
#if defined _DEBUG
	void DBG_VisSIM(FAnimInstanceProxy* animProxy) const;
#endif


private:
	const UAnimInstance_HIKDriver* c_animInstDriver;
};
