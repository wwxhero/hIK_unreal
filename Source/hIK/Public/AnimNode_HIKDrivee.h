// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_MotionPipe.h"
#include "AnimNode_HIKDrivee.generated.h"

//@TODO: Comment



USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_HIKDrivee : public FAnimNode_MotionPipe
{
	GENERATED_USTRUCT_BODY()

public:
	FAnimNode_HIKDrivee();
	virtual ~FAnimNode_HIKDrivee();

	virtual HBODY InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref, const FBoneContainer& RequiredBones, const FTransform& skelecom_l2w, const BITree& idx_tree, const std::set<FString>& namesOnPair) override;
	virtual HBODY InitializeBodySim_AnyThread(HBODY body_fbx);
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	virtual void InitializeEEFs_AnyThread(FAnimInstanceProxy_MotionPipe* proxy, TArray<EndEF_Internal>& eefs) override;

#if defined _DEBUG
	void DBG_VisSIM(FAnimInstanceProxy* animProxy) const;
#endif

private:
	FTransform m_rootTM0_p2l;

};
