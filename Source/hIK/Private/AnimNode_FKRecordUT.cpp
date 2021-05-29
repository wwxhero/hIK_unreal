// Copyright(c) Mathew Wang 2021

//#include "stdafx.h"
#include "AnimNode_HIKTest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Runtime/AnimationCore/Public/TwoBoneIK.h"
#include "ik.h"

DECLARE_CYCLE_STAT(TEXT("FK UT"), STAT_FK_UT_Eval, STATGROUP_Anim);

void FAnimNode_FKRecordUT::UpdateInternal(const FAnimationUpdateContext & Context)
{
	// Mark trace data as stale
	Super::UpdateInternal(Context);
	//UE_LOG(LogHIK, Warning, TEXT("FAnimNode_FKRecordUT::UpdateInternal"));
}

void FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);
	FCompactPoseBoneIndex boneCompactIdx = BoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
	//UE_LOG(LogHIK, Warning, TEXT("FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread"));
	USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	//USceneComponent* ParentComponent = SkelComp->GetAttachParent();
	FTransform l2enti = Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
	static float delta_deg = 0;
	const float c_deg2rad = PI / 180;
	FVector axis(0, 1, 0);
	float angleRad = delta_deg * c_deg2rad;
	FQuat rot_enti(axis, angleRad);
	FTransform t_enti(rot_enti);
	FTransform l2enti_prime = t_enti * l2enti;
	FBoneTransform bone_tran(boneCompactIdx, l2enti_prime);
	OutBoneTransforms.Push(bone_tran);
	delta_deg = ik_test(delta_deg);
}


bool FAnimNode_FKRecordUT::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer & RequiredBones)
{
	//UE_LOG(LogHIK, Warning, TEXT("FAnimNode_FKRecordUT::IsValidToEvaluate"));
	return true;
}

void FAnimNode_FKRecordUT::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	//UE_LOG(LogHIK, Warning, TEXT("FAnimNode_FKRecordUT::InitializeBoneReferences"));
	printOutSkeletalHierachy(RequiredBones.GetReferenceSkeleton());
	UE_LOG(LogTemp, Warning, TEXT("Before: BoneName = %s, BoneIndex = %d, bUseSkeletonIndex = %d, CompactIndex = %d")
		, *BoneRef.BoneName.ToString()
		, BoneRef.BoneIndex
		, BoneRef.bUseSkeletonIndex
		, BoneRef.CachedCompactPoseIndex.GetInt());
	BoneRef.Initialize(RequiredBones);
	UE_LOG(LogTemp, Warning, TEXT("After: BoneName = %s, BoneIndex = %d, bUseSkeletonIndex = %d, CompactIndex = %d")
		, *BoneRef.BoneName.ToString()
		, BoneRef.BoneIndex
		, BoneRef.bUseSkeletonIndex
		, BoneRef.CachedCompactPoseIndex.GetInt());
}

void FAnimNode_FKRecordUT::printOutSkeletalHierachy(const FReferenceSkeleton& ref, int identation)
{
	//auto bones = skeleton->GetBoneTree();
	//const FReferenceSkeleton& ref = skeleton->GetReferenceSkeleton();
	int n_bone = ref.GetNum();
	UE_LOG(LogTemp, Warning, TEXT("Number of bones: %d"), n_bone);

	TArray<Children*> node2children;
	node2children.SetNum(n_bone, false);
	for (int32 i_bone = n_bone - 1
	 	; i_bone > 0
	 	; i_bone--)
	{
	 	int32 i_parent = ref.GetParentIndex(i_bone);
	 	ensure(i_parent > -1);
		auto new_child = new Children(i_bone);
		new_child->LinkHead(node2children[i_parent]);
	}


	printOutSkeletalHierachy_recur(ref, node2children, 0, identation);

	for (int32 i_bone = 0
		; i_bone < n_bone
		; i_bone ++)
	{
		TLinkedList<int32>* children_i = node2children[i_bone];
		TLinkedList<int32>* it_m = NULL;
		for (auto it = children_i
			; NULL != it
			; it = it->Next())
		{
			delete it_m;
			it_m = it;
			auto name = ref.GetBoneName(*(*it));
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *name.ToString());
		}
		delete it_m;
	}
}

void FAnimNode_FKRecordUT::printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const TArray<Children*>& node2children, int32 id_node, int identation)
{
	auto name = ref.GetBoneName(id_node);
	FString item;
	for (int i_ident = 0; i_ident < identation; i_ident++)
		item += TEXT("\t");
	item += name.ToString();
	UE_LOG(LogTemp, Warning, TEXT("%s"), *item);
	const Children* children = node2children[id_node];
	for (auto it = begin(*children)
		; it
		; it ++)
	{
		printOutSkeletalHierachy_recur(ref, node2children, *it, identation + 1);
	}
}