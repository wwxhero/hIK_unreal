// Copyright(c) Mathew Wang 2021
#include "AnimNode_FKRecordUT.h"
#include <map>
#include "Components/SkeletalMeshComponent.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "bvh.h"
#include "fk_joint.h"
#include "motion_pipeline.h"
#include "ik_logger_unreal.h"
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

HBODY FAnimNode_FKRecordUT::InitializeBodySim_AnyThread(HBODY /*body_fbx*/)
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

	const FBoneContainer& requiredBones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();

	auto driverBVH = m_bodies[FAnimNode_MotionPipe::c_idxSim];
	auto moDriverBVH = m_moNodes[FAnimNode_MotionPipe::c_idxSim];
	bool exists_a_channel = (m_channelsFBX.Num() > 0);
	bool ok = (VALID_HANDLE(driverBVH)
			&& VALID_HANDLE(moDriverBVH)
			&& exists_a_channel);

	IKAssert(ok);

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

#if defined _DEBUG
		// DBG_VisCHANNELs(Output.AnimInstanceProxy);
		// DBG_VisSIM(Output.AnimInstanceProxy);
		// DBG_VisEndEFs(Output.AnimInstanceProxy);
#endif
	}
}







