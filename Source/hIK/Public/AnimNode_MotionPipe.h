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
USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_MotionPipe : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
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

	struct EndEF_Internal : public EndEF
	{
		HBODY h_body;
	};

	void InitializeEEF_Internal(EndEF_Internal* eef, const FString& a_name, const FTransform& a_tm_l2w, HBODY a_body)
	{
		eef->name = a_name;
		eef->tm_l2w = a_tm_l2w;
		eef->h_body = a_body;
	}

	struct FCompareEEF
	{
		FORCEINLINE bool operator()(const EndEF_Internal& A, const EndEF_Internal& B) const
		{
			return A.name < B.name;
		}
	};

protected:
	virtual HBODY InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref, const FBoneContainer& RequiredBones, const FTransform& skelecom_l2w, const BITree& idx_tree, const std::set<FString>& namesOnPair);
	virtual HBODY InitializeBodySim_AnyThread(HBODY body_fbx) { return H_INVALID; }
	virtual void InitializeEEFs_AnyThread(const FTransform& skelecom_l2w, const std::set<FString> &eefs_name, TArray<EndEF_Internal>& eefs) { }

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

#if defined _DEBUG
	void DBG_LogTransform(const FString& name, const FTransform* tm) const;
	void DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	void DBG_printOutSkeletalHierachy(HBODY root_body) const;
	void DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	bool DBG_EqualTransform(const FTransform& tm_1, const _TRANSFORM& tm_2) const;
	void DBG_LogTransform(const FString& name, const _TRANSFORM* tm) const;
	void DBG_VisTransform(const FTransform& tm_l2w, FAnimInstanceProxy* animProxy) const;
	void DBG_VisTargets(FAnimInstanceProxy_MotionPipe* animProxy) const;
	void DBG_VisCHANNELs(FAnimInstanceProxy* animProxy) const;
#endif

private:
	// Resused bone transform array to avoid reallocating in skeletal controls
	TArray<FBoneTransform> BoneTransforms;

protected:
	const UAnimInstance_MotionPipe* c_animInst;

	TArray<CHANNEL> m_channelsFBX;

	TArray<EndEF_Internal> m_eefs;

	MotionPipe m_mopipe;

	bool c_inCompSpace;
public:
	static const int c_idxSim;
	static const int c_idxFBX;
};