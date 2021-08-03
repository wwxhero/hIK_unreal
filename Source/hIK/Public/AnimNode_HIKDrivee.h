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

	virtual HBODY InitializeBodySim_AnyThread(HBODY body_fbx);
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
};
