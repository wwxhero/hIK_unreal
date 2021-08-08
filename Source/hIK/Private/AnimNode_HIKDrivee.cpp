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

void FAnimNode_HIKDrivee::InitializeEEFs_AnyThread(FAnimInstanceProxy_MotionPipe* proxy, TArray<EndEF_Internal>& a_eefs)
{
	HBODY h_bodySIM = m_bodies[FAnimNode_MotionPipe::c_idxSim];
	std::set<FString> eefs_name;
	int32 n_eefs = c_animInst->CopyEEFs(eefs_name, FAnimNode_MotionPipe::c_idxSim);
	bool exist_eef = (0 < n_eefs);
	if (!exist_eef)
		return;

	TArray<EndEF_Internal> eefs;
	eefs.Reset(n_eefs);

	const FTransform& c2w = proxy->GetSkelMeshCompLocalToWorld();
	auto onEnterBody = [this, &eefs, &eefs_name, &c2w] (HBODY h_this)
		{
			FString name_this(body_name_w(h_this));
			bool is_eef = (eefs_name.end() != eefs_name.find(name_this));
			if (is_eef)
			{
				_TRANSFORM tm_l2c;
				get_body_transform_l2w(h_this, &tm_l2c);
				FTransform tm_l2c_2;
				Convert(tm_l2c, tm_l2c_2);
				EndEF_Internal eef;
				InitializeEEF_Internal(&eef, name_this, tm_l2c_2 * c2w, h_this);
				eefs.Add(eef);
			}

		};

	auto onLeaveBody = [] (HBODY h_this)
		{
		};

	TraverseDFS(h_bodySIM, onEnterBody, onLeaveBody);

	eefs.Sort(FCompareEEF());

	a_eefs.SetNum(n_eefs);
	for (int i_eef = 0; i_eef < n_eefs; i_eef ++)
	{
		a_eefs[i_eef] = eefs[i_eef];
	}
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

		FAnimInstanceProxy_MotionPipe* proxy = static_cast<FAnimInstanceProxy_MotionPipe*>(Output.AnimInstanceProxy);

#if defined _DEBUG
		TArray<EndEF> eefs_i;
		bool empty_update = proxy->EmptyEndEEFs();
		// LOGIKVar(LogInfoBool, empty_update);
		if (!empty_update)
		{
			proxy->PullUpdateEEFs(eefs_i);
			int32 n_eefs = eefs_i.Num();
			bool match = (n_eefs == m_eefs.Num());
			if (match)
			{
				for (int32 i_eef = 0; i_eef < n_eefs && match; i_eef ++)
				{
					match = (eefs_i[i_eef].name == m_eefs[i_eef].name);
					m_eefs[i_eef].tm_l2w = eefs_i[i_eef].tm_l2w;
				}
			}
		//	LOGIKVar(LogInfoBool, match);
			DBG_VisTargets(proxy);
		}


		// DBG_VisCHANNELs(Output.AnimInstanceProxy);
		DBG_VisSIM(Output.AnimInstanceProxy);

		// LOGIKVar(LogInfoInt, proxy->GetEEFs_i().Num());


#endif
	}
}

void FAnimNode_HIKDrivee::DBG_VisSIM(FAnimInstanceProxy* animProxy) const
{
	HBODY body_sim = m_bodies[FAnimNode_MotionPipe::c_idxSim];
	FMatrix bvh2unrel_m;
	c_animInst->CopySrc2Dst_w(bvh2unrel_m);
	FTransform bvh2unrel(bvh2unrel_m);
	auto lam_onEnter = [this, animProxy, &bvh2unrel] (HBODY h_this)
						{
							_TRANSFORM l2c_body;
							get_body_transform_l2w(h_this, &l2c_body);
							FTransform l2c_unrel;
							Convert(l2c_body, l2c_unrel);
							FTransform l2w = l2c_unrel * bvh2unrel * animProxy->GetSkelMeshCompLocalToWorld();
							DBG_VisTransform(l2w, animProxy);
						};
	auto lam_onLeave = [] (HBODY h_this)
						{

						};
	TraverseDFS(body_sim, lam_onEnter, lam_onLeave);
}