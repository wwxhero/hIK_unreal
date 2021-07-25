// Copyright (c) Mathew Wang 2021

#pragma once
#include <stack>
#include <set>
#include "hIK.h"
#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "articulated_body.h"
#include "AnimInstance_HIKDriver.h"
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

USTRUCT(BlueprintInternalUseOnly)
struct HIK_API FAnimNode_FKRecordUT : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose;
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

	HBODY InitializeChannel(const FReferenceSkeleton& ref, const FBoneContainer& RequiredBones, const BITree& idx_tree, int i_col_match);

	bool LoadConf(HCONF hConf);
	void UnLoadConf();

public:
	FAnimNode_FKRecordUT()
		: m_animInst(NULL)
		, m_driverBVH(H_INVALID)
		, m_driverHTR(H_INVALID)
		, m_driverStub(H_INVALID)
		, m_moDriverBVH(H_INVALID)
		, m_moDriverHTR(H_INVALID)
		, m_moDriverStub(H_INVALID)
		, m_match(NULL)
		, m_nMatches(0)
		, m_bvh2fbxWorld{
			{1, 0, 0},
			{0, 1, 0},
			{0, 0, 1}
		}
		, m_scales(NULL)
		, m_nScales(0)
		, m_targetnames(NULL)
		, m_nTargets(0)
	{
	}

	virtual ~FAnimNode_FKRecordUT()
	{
		UnInitializeBoneReferences();
	}

protected:
	void Convert(const FTransform& tm_s, _TRANSFORM& tm_t) const
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

	void Convert(const _TRANSFORM& tm_s, FTransform& tm_t) const
	{
		const auto& s_s = tm_s.s;
		const auto& r_s = tm_s.r;
		const auto& tt_s = tm_s.tt;

		tm_t.SetLocation(FVector(tt_s.x, tt_s.y, tt_s.z));
		tm_t.SetRotation(FQuat(r_s.x, r_s.y, r_s.z, r_s.w));
		tm_t.SetScale3D(FVector(s_s.x, s_s.y, s_s.z));
	}

protected:
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
protected:
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const { return true; }
	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FPoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones);
	void UnInitializeBoneReferences();
	// End FAnimNode_SkeletalControlBase Interface
#if defined _DEBUG
	void DBG_LogTransform(const FString& name, const FTransform* tm) const;
	void DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	void DBG_printOutSkeletalHierachy(HBODY root_body) const;
	void DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& tree, int32 id_node, int identation) const;
	void DBG_GetComponentSpaceTransform(const CHANNEL& channel, FTransform& tm, const FReferenceSkeleton& skeleton) const;
	void DBG_GetComponentSpaceTransform2(const CHANNEL& channel, _TRANSFORM& tm, const FReferenceSkeleton& skeleton) const;
	bool DBG_EqualTransform(const FTransform& tm_1, const _TRANSFORM& tm_2) const;
	bool DBG_verifyChannel(const FReferenceSkeleton& ref_sk) const;
	void DBG_LogTransform(const FString& name, const _TRANSFORM* tm) const;
	void DBG_VisTransform(const UWorld* world, const FTransform& c2w, HBODY hBody, int i_col_match) const;
	void DBG_VisTransform(const UWorld* world, const FTransform& tm) const;
	void DBG_VisTargetTransform(const UWorld* world, const TArray<UAnimInstance_HIK::Target>* targets) const;
#endif

protected:
	TArray<CHANNEL> m_channels;
	const UAnimInstance_HIKDriver* m_animInst;
	HBODY m_driverBVH;
	HBODY m_driverHTR;
	HBODY m_driverStub;
	HMOTIONNODE m_moDriverBVH;
	HMOTIONNODE m_moDriverHTR;
	HMOTIONNODE m_moDriverStub;

	// conf variables:
	const wchar_t* (*m_match)[2];
	int m_nMatches;
	float m_bvh2fbxWorld[3][3];
	B_Scale *m_scales;
	int m_nScales;
	const wchar_t** m_targetnames;
	int m_nTargets;
	// end of conf variables

	std::set<std::wstring> m_mapmatch[2];

	inline bool getScale(const wchar_t* bone_name, float &s_x, float &s_y, float &s_z) const
	{
		bool b_match = false;

		int i_scale = 0;
		for (
			; i_scale < m_nScales && !b_match
			; i_scale ++)
		{
			b_match = (0 == wcscmp(bone_name, m_scales[i_scale].bone_name));
		}
		if (b_match)
		{
			s_x = m_scales[i_scale-1].scaleX;
			s_y = m_scales[i_scale-1].scaleY;
			s_z = m_scales[i_scale-1].scaleZ;
		}
		return b_match;
	}
private:
	// Resused bone transform array to avoid reallocating in skeletal controls
	TArray<FBoneTransform> BoneTransforms;
};
