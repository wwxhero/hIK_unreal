// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_MotionPipe.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimNode_HIKDrivee.generated.h"

//@TODO: Comment



USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_HIKDrivee : public FAnimNode_MotionPipe
{
	GENERATED_USTRUCT_BODY()

public:
	FAnimNode_HIKDrivee();
	virtual ~FAnimNode_HIKDrivee();

	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual HBODY InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
												, const FBoneContainer& RequiredBones
												, const FTransform& skelecom_l2w
												, const BITree& idx_tree
												, const std::set<FString>& namesOnPair
												, const std::map<FString, FVector>& name2scale) override;

	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	virtual void InitializeTargets_AnyThread(HBODY body_fbx, const FTransform& skelecom_l2w, const std::set<FString> &targets_name) override;

// #if defined _DEBUG
	void DBG_VisSIM(FAnimInstanceProxy* animProxy) const;
// #endif

private:
	FTransform m_rootTM0_p2l;
	FTransform m_C0toW;
	FTransform m_WtoC0;
	const UAnimInstance_HIKDrivee* c_animInstDrivee;

};
