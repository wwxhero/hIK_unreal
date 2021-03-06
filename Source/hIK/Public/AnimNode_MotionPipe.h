// Copyright (c) Mathew Wang 2021
#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "AnimInstance_MotionPipe.h"
#include "articulated_body.h"
#include "motion_pipeline.h"
#include "transform_helper.h"
#include "AnimNode_MotionPipe.generated.h"

#define get_body_transform_LtoC0 get_body_transform_l2w
#define get_body_transform_C0toL get_body_transform_w2l

USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_MotionPipe : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HIKAnim)
	FPoseLink BasePose;
public:
	FAnimNode_MotionPipe();
	virtual ~FAnimNode_MotionPipe();

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
			&& VALID_HANDLE(bone_n.h_body);
	}
	bool ConsistentCHANNEL(const CHANNEL& bone_n)
	{
		return (INDEX_NONE == bone_n.r_bone.BoneIndex)
			== (!VALID_HANDLE(bone_n.h_body));
	}
	void ResetCHANNEL(CHANNEL& bone_n)
	{
		bone_n.r_bone.BoneIndex = INDEX_NONE;
		bone_n.h_body = H_INVALID;
	}

	struct Target_Internal : public Target
	{
		HBODY h_body;
	};

	void InitializeTarget_Internal(Target_Internal* target, const FString& a_name, const FTransform& a_tm_l2w, HBODY a_body)
	{
		target->name = a_name;
		target->tm_l2w = a_tm_l2w;
		target->h_body = a_body;
	}

	struct FCompareTarget
	{
		FORCEINLINE bool operator()(const Target_Internal& A, const Target_Internal& B) const
		{
			return A.name < B.name;
		}
	};

protected:
	virtual HBODY InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
												, const FBoneContainer& RequiredBones
												, const FTransform& skelecom_l2w
												, const BITree& idx_tree
												, const std::set<FString>& namesOnPair
												, const std::map<FString, FVector>& name2scale);

	virtual void InitializeTargets_AnyThread(HBODY body_fbx, const FTransform& skelecom_l2w, const std::set<FString> &targets_name);

	struct ParamFBXCreator {
		FAnimNode_MotionPipe* pThis;
		const FReferenceSkeleton& boneRef;
		const FBoneContainer& RequiredBones;
		const FTransform& skelecom_l2w;
		const BITree& idx_tree;
	};
	static HIKLIB_CB(HBODY, ProcInitBody_FBX)(void* param
											, const wchar_t* filePath
											, const wchar_t* namesOnPair[]
											, int n_pairs
											, const B_Scale scales[]
											, int n_scales
											, const wchar_t* nameTargets[]
											, int n_Targets);

protected:
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override final;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override final;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override final;
	void UnCacheBones_AnyThread();

private:
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {};


protected:
	virtual bool NeedsOnInitializeAnimInstance() const { return true; }
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;

	void printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	void printOutSkeletalHierachy(HBODY root_body) const;
	void printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;

#if defined _DEBUG
	void DBG_LogTransform(const FString& name, const FTransform* tm) const;
	void DBG_printOutSkeletalHierachy(HBODY root_body) const;
	void DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	bool DBG_EqualTransform(const FTransform& tm_1, const _TRANSFORM& tm_2) const;
	void DBG_LogTransform(const FString& name, const _TRANSFORM* tm) const;
	void DBG_VisTransform(const FTransform& tm_l2w, FAnimInstanceProxy* animProxy, float axis_len = 10, float thickness = 1) const;
	void DBG_VisTargets(FAnimInstanceProxy_MotionPipe* animProxy) const;
	void DBG_VisEEFs(FAnimInstanceProxy_MotionPipe* animProxy) const;
	void DBG_VisCHANNELs(FAnimInstanceProxy* animProxy) const;
#endif

private:
	// Resused bone transform array to avoid reallocating in skeletal controls
	TArray<FBoneTransform> BoneTransforms;

protected:
	const UAnimInstance_MotionPipe* c_animInst;

	TArray<CHANNEL> m_channelsFBX;

	TArray<Target_Internal> m_targets;

	MotionPipe* m_mopipe;

	bool c_inCompSpace;

	FTransform m_C0toW;
	FTransform m_WtoC0;

public:
	static const int c_idxSim;
	static const int c_idxFBX;
};