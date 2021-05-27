// Copyright (c) Mathew Wang 2021

#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_HIKTest.generated.h"


// Traces towards down to find the location of the floor under the foot and toe.
// The results of this trace are used to determine where the foot should go during IK,
// among other things(e.g., foot rotation).
//
// The trace proceeds in a downward vertical line through the foot / toe. The trace will start
// at the pelvis height or the foot / toe height, whichever is higher, and end at
// the maximum reach of the leg, plus the pelvis adjustment distance.
//
// Tracing is expensive; for many IK setups, this is the most expensive step. Therefore,
// trace data is stored in a wrapper passed in by pointer. During the execution of this node,
// trace data is store in the TraceData input; you can then re-use this wrapper object
// later in your AnimGraph.
USTRUCT()
struct HIK_API FAnimNode_HIKTest : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

public:
	// // The leg to trace from
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bones, meta = (PinShownByDefault))
	// UHumanoidLegChain_Wrapper* Leg;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Bones, meta = (PinShownByDefault))
	// UIKBoneWrapper* PelvisBone;

	// // The trace data object to fill in. Trace data will be set in this node, you may then
	// // use it later in your AnimGraph.
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace, meta = (PinShownByDefault))
	// UHumanoidIKTraceData_Wrapper* TraceData;

	// // Maximum height above the floor to do pelvis adjustment. Will transition back to base pose if the
	// // required hip adjustment is larger than this value. Should probably be something like 1 / 3 character capsule height
 //    // (more if you're brave)
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	// float MaxPelvisAdjustSize;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	// bool bEnableDebugDraw;

public:

	FAnimNode_HIKTest()
		: BoneRef(TEXT("pelvis_L"))
	{ }

	//UPROPERTY(EditAnywhere, Category = "Settings")
   	FBoneReference BoneRef;

protected:

	// FAnimNode_SkeletalControlBase interface
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End FAnimNode_SkeletalControlBase Interface
	void printOutSkeletalHierachy(const FReferenceSkeleton& ref, int identation = 1);
	typedef TLinkedList<int32> Children;
	void printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const TArray<Children*>& node2children, int32 id_node, int identation);
};
