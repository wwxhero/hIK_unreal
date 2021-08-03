// Copyright(c) Mathew Wang 2021
#include "AnimNode_FKRecordUT.h"
#include <map>
#include "Components/SkeletalMeshComponent.h"
#include "AnimInstanceProxy_HIK.h"
#include "bvh.h"
#include "fk_joint.h"
#include "motion_pipeline.h"
#include "ik_logger.h"
#include "transform_helper.h"


DECLARE_CYCLE_STAT(TEXT("FK UT"), STAT_FK_UT_Eval, STATGROUP_Anim);

FAnimNode_FKRecordUT::FAnimNode_FKRecordUT()
	: c_animInstDriver(NULL)
{
}

FAnimNode_FKRecordUT::~FAnimNode_FKRecordUT()
{
}

void FAnimNode_FKRecordUT::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	Super::OnInitializeAnimInstance(InProxy, InAnimInstance);
	c_animInstDriver = Cast<UAnimInstance_HIKDriver, UAnimInstance>(InAnimInstance);
	check(nullptr != c_animInstDriver);
}

HBODY FAnimNode_FKRecordUT::InitializeBodySim_AnyThread(HBODY /*body_fbx*/, const std::set<FString>& /*namesOnPair*/)
{
	check(nullptr != c_animInstDriver);
	HBODY driverBVH = H_INVALID;
	bool bvh_load = (VALID_HANDLE(c_animInstDriver->getBVH())
				&& VALID_HANDLE(driverBVH = create_tree_body_bvh(c_animInstDriver->getBVH())));
	LOGIKVar(LogInfoBool, bvh_load);
	return driverBVH;
}

void FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread(FPoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);

	bool exists_a_channel = (m_channelsFBX.Num() > 0);

	if (!exists_a_channel)
		return;

	const FBoneContainer& requiredBones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();

	auto driverBVH = m_bodies[0];
	auto moDriverBVH = m_moNodes[0];
	IKAssert(VALID_HANDLE(driverBVH) && VALID_HANDLE(moDriverBVH));
	bool ok = (VALID_HANDLE(driverBVH)
			&& VALID_HANDLE(moDriverBVH));

	if (ok)
	{
		pose_body(c_animInstDriver->getBVH(), driverBVH, c_animInstDriver->I_Frame_);
		motion_sync(moDriverBVH);

		int n_channels = m_channelsFBX.Num();

		OutBoneTransforms.SetNum(n_channels, false);

		for (int i_channel = 0; i_channel < n_channels; i_channel ++)
		{
			FCompactPoseBoneIndex boneCompactIdx = m_channelsFBX[i_channel].r_bone.GetCompactPoseIndex(requiredBones);
			_TRANSFORM l2w_body;
			get_body_transform_l2p(m_channelsFBX[i_channel].h_body, &l2w_body);
			FTransform l2w_unr;
			Convert(l2w_body, l2w_unr);
			FBoneTransform tm_bone(boneCompactIdx, l2w_unr);
			OutBoneTransforms[i_channel] = tm_bone;
		}

		// TArray<UAnimInstance_HIK::Target>* targets = animInst->LockTarget();
		// if (NULL != targets)
		// {
		// 	auto n_targets = targets->Num();
		// 	for (int i_target = 0; i_target < n_targets; i_target ++)
		// 	{
		// 		const HBODY& body_i = (*targets)[i_target].h_body;
		// 		FTransform& trans_i_unrealw = (*targets)[i_target].tm_l2w;
		// 		_TRANSFORM tran_i_bvh_w;
		// 		get_body_transform_l2w((*targets)[i_target].h_body, &tran_i_bvh_w);
		// 		FTransform tran_i_unreal_c;
		// 		Convert(tran_i_bvh_w, tran_i_unreal_c);
		// 		trans_i_unrealw = tran_i_unreal_c * c2w;
		// 	}
		// }



#if defined _DEBUG
		auto owner = Output.AnimInstanceProxy->GetSkelMeshComponent()->GetOwner();
		auto world = owner->GetWorld();
		FMatrix bvh2unrel_m;
		c_animInst->CopySrc2Dst_w(bvh2unrel_m);
		FTransform bvh2unrel(bvh2unrel_m);
		// DBG_VisTransform(world, bvh2unrel, m_driverHTR, 0);
		DBG_VisTransform(Output.AnimInstanceProxy, m_bodies[1], 1);
		FVector offset(300, 0, 0);
		FTransform tm_offset(offset);
		// DBG_VisTransform(world, bvh2unrel*tm_offset, driverBVH, 0);
		// DBG_VisTargetTransform(world, targets);
#endif

		// const bool rotate_on_entity = false;
		// const FTransform* l2world = NULL;
		// if (rotate_on_entity)
		// {
		// 	l2world = &owner->GetTransform();
		// 	const FTransform& tm_entity = Output.AnimInstanceProxy->GetSkelMeshCompOwnerTransform();
		// 	check(tm_entity.Equals(*l2world, c_epsilon));
		// }
		// else
		// {
		// 	FCompactPoseBoneIndex boneCompactIdx = m_channelsFBX[0].r_bone.GetCompactPoseIndex(requiredBones);
		// 	l2world = &Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
		// }

		// float delta_deg = c_animInst->I_Frame_;
		// const float c_deg2rad = PI / 180;
		// FVector axis(0, 0, 1);
		// float delta_rad = delta_deg * c_deg2rad;
		// FQuat delta_q(axis, delta_rad);
		// FTransform delta_world(delta_q);
		// FTransform l2world_prime = (*l2world) * delta_world;
		// if (rotate_on_entity)
		// 	owner->SetActorTransform(l2world_prime);
		// else
		// {
		// 	FCompactPoseBoneIndex boneCompactIdx = m_channelsFBX[0].r_bone.GetCompactPoseIndex(requiredBones);
		// 	FBoneTransform tm_bone(boneCompactIdx, l2world_prime);
		// 	OutBoneTransforms.Add(tm_bone);
		// 	delta_deg = ik_test(delta_deg);
		// }
	}
}







