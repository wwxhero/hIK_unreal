// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_HIKDrivee.h"
#include "Animation/AnimTrace.h"
#include "motion_pipeline.h"
#include "ik_logger_unreal.h"

DECLARE_CYCLE_STAT(TEXT("HIK UT"), STAT_HIK_UT_Eval, STATGROUP_Anim);

/////////////////////////////////////////////////////
// FAnimNode_HIKDrivee

FAnimNode_HIKDrivee::FAnimNode_HIKDrivee()
{
}

FAnimNode_HIKDrivee::~FAnimNode_HIKDrivee()
{

}

HBODY FAnimNode_HIKDrivee::InitializeBodySim_AnyThread(HBODY body_fbx)
{
	const wchar_t* (*matches)[2] = NULL;
	int n_match = c_animInst->CopyMatches(&matches);
	HBODY body_htr_1 = H_INVALID;
	HBODY body_htr_2 = H_INVALID;
	if (! (clone_body_interests(body_fbx, htr, &body_htr_1, matches, n_match, false) // body_htr_1 is an intermediate body, orient bone with src bone information
		&& clone_body(body_htr_1, htr, &body_htr_2)) )						  // body_htr_2 is the result, orient bone with the interest bone information
		body_htr_2 = H_INVALID;

	if (VALID_HANDLE(body_htr_1))
		destroy_tree_body(body_htr_1);
	return body_htr_2;
}

void FAnimNode_HIKDrivee::EvaluateSkeletalControl_AnyThread(FPoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_HIK_UT_Eval);

	const FBoneContainer& requiredBones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();

	auto driverHTR = m_bodies[0];
	auto moDriverHTR = m_moNodes[0];
	bool exists_a_channel = (m_channelsFBX.Num() > 0);
	bool ok = (VALID_HANDLE(driverHTR)
			&& VALID_HANDLE(moDriverHTR)
			&& exists_a_channel);

	IKAssert(ok);

	if (ok)
	{
		// replace with an HIK computation
		// pose_body(c_animInstDriver->getHTR(), driverHTR, c_animInstDriver->I_Frame_);

		motion_sync(moDriverHTR);

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
		FMatrix htr2unrel_m;
		c_animInst->CopySrc2Dst_w(htr2unrel_m);
		FTransform htr2unrel(htr2unrel_m);
		// DBG_VisTransform(world, HTR2unrel, m_driverHTR, 0);
		FTransform fbx_w;
		DBG_VisTransform(Output.AnimInstanceProxy, fbx_w, driverHTR, 0);
		FVector offset(300, 0, 0);
		FTransform tm_offset(offset);
		// DBG_VisTransform(world, HTR2unrel*tm_offset, driverBVH, 0);
		// DBG_VisTargetTransform(world, targets);
#endif
	}
}
