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
	virtual bool NeedsOnInitializeAnimInstance() const { return true; }
	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	// End FAnimNode_SkeletalControlBase Interface

	FORCEINLINE UAnimInstance_HIKDriver* LockAnimInst(bool force = false)
	{
		return Cast<UAnimInstance_HIKDriver, UAnimInstance_HIK>(Super::LockAnimInst(false));
	}

	virtual HBODY InitializeBodySim(HBODY body_fbx) override
	{
		auto animInst = LockAnimInst(true);
		HBODY driverBVH = H_INVALID;
		bool bvh_load = (VALID_HANDLE(animInst->getBVH())
					&& VALID_HANDLE(driverBVH = create_tree_body_bvh(animInst->getBVH())));
		UnLockAnimInst(animInst);
		LOGIKVar(LogInfoBool, bvh_load);
		return driverBVH;
	}

};
