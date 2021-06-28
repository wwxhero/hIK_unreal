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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	FString m_rcBVHPath;
private:
	typedef TLinkedList<int32> BIChildren;
	typedef TArray<BIChildren*> BITree;

	void ConstructBITree(const FReferenceSkeleton& ref, BITree& idx_tree) const
	{
		int32 n_bone = ref.GetNum();
		idx_tree.SetNum(n_bone, false);
		for (int32 i_bone = n_bone - 1
			; i_bone > 0
			; i_bone--)
		{
			int32 i_parent = ref.GetParentIndex(i_bone);
			ensure(i_parent > -1);
			auto& i_siblings = idx_tree[i_parent];
			auto i_this = new BIChildren(i_bone);
			i_this->LinkHead(i_siblings);
		}
	}

	void ReleaseBITree(BITree& bi_tree) const
	{
		int32 n_bone = bi_tree.Num();
		for (int32 i_bone = 0
			; i_bone < n_bone
			; i_bone++)
		{
			TLinkedList<int32>* children_i = bi_tree[i_bone];
			TLinkedList<int32>* it_child = NULL;
			for (auto it = children_i
				; NULL != it
				; it = it->Next())
			{
				delete it_child;
				it_child = it;
			}
			delete it_child;
		}
	}

	typedef struct
	{
		FBoneReference r_bone;
		HBODY h_body;
	} CHANNEL;

	bool ValidCHANNEL(const CHANNEL& bone_n)
	{
		return INDEX_NONE != bone_n.r_bone.BoneIndex
			&& H_INVALID != bone_n.h_body;
	}
	bool ConsistentCHANNEL(const CHANNEL& bone_n)
	{
		return (INDEX_NONE == bone_n.r_bone.BoneIndex)
			== (H_INVALID == bone_n.h_body);
	}
	void ResetCHANNEL(CHANNEL& bone_n)
	{
		bone_n.r_bone.BoneIndex = INDEX_NONE;
		destroy_tree_body_node(bone_n.h_body);
		bone_n.h_body = H_INVALID;
	}

	void InitializeChannel_BITree(const FReferenceSkeleton& ref, const FBoneContainer& RequiredBones, const BITree& idx_tree);

public:
	FAnimNode_FKRecordUT()
	{
	}

	virtual ~FAnimNode_FKRecordUT()
	{
		UnInitializeBoneReferences();
	}

protected:
	void GetBoneLocalTranform(const FTransform& tm_s, _TRANSFORM& tm_t) const
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
	void DBG_LogTransform(const FString& name, const FTransform* tm) const;
	void DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	void DBG_printOutSkeletalHierachy(HBODY root_body) const;
	void DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	void DBG_GetComponentSpaceTransform(const CHANNEL& channel, _TRANSFORM& tm, const FReferenceSkeleton& skeleton) const;
	void DBG_GetComponentSpaceTransform2(const CHANNEL& channel, _TRANSFORM& tm, const FReferenceSkeleton& skeleton) const;
	bool DBG_EqualTransform(const FTransform& tm_1, const _TRANSFORM& tm_2) const;
#endif
protected:
	TArray<CHANNEL> m_channels;
};
