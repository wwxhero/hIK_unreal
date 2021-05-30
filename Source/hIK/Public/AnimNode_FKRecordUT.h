// Copyright (c) Mathew Wang 2021

#pragma once
#include <stack>
#include "hIK.h"
#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "articulated_body.h"
#include "AnimNode_FKRecordUT.generated.h"



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
struct HIK_API FAnimNode_FKRecordUT : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

private:
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
	typedef struct
	{
		int32 bone_id;
		HBODY h_body;
	} BONE_NODE;
	typedef TLinkedList<int32> Children;

	inline bool ValidBONE_NODE(const BONE_NODE& bone_n)
	{
		return INDEX_NONE != bone_n.bone_id
			&& H_INVALID != bone_n.h_body;
	}
	inline bool ConsistentBONE_NODE(const BONE_NODE& bone_n)
	{
		return (INDEX_NONE == bone_n.bone_id)
			== (H_INVALID == bone_n.h_body);
	}
	inline void ResetBONE_NODE(BONE_NODE& bone_n)
	{
		bone_n.bone_id = INDEX_NONE;
		bone_n.h_body = H_INVALID;
	}



public:

	FAnimNode_FKRecordUT()
		: BoneRef(TEXT("pelvis_L"))
	{
		ResetBONE_NODE(m_boneRoot);
	}

	virtual ~FAnimNode_FKRecordUT()
	{
		UnInitializeBoneReferences();
	}

	//UPROPERTY(EditAnywhere, Category = "Settings")
   	FBoneReference BoneRef;
protected:
	inline void GetBoneLocalTranform(const FTransform& tm_s, _TRANSFORM& tm_t)
	{
		const auto& s_s = tm_s.GetScale3D();
		const auto& r_s = tm_s.GetRotation();
		const auto& tt_s = tm_s.GetTranslation();

		tm_t.s.x = s_s.X;
		tm_t.s.y = s_s.Y;
		tm_t.s.z = s_s.Z;

		tm_t.r.w = r_s.W;
		tm_t.r.x = r_s.X;
		tm_t.r.y = r_s.Y;
		tm_t.r.z = r_s.Z;

		tm_t.tt.x = tt_s.X;
		tm_t.tt.y = tt_s.Y;
		tm_t.tt.z = tt_s.Z;
	}

protected:

	// FAnimNode_SkeletalControlBase interface
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	void UnInitializeBoneReferences();
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End FAnimNode_SkeletalControlBase Interface
#if defined _DEBUG
	void DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const TArray<Children*>& node2children, int32 id_node, int identation);
	void DBG_printOutSkeletalHierachy(HBODY root_body);
#endif
protected:
	BONE_NODE m_boneRoot;
};
