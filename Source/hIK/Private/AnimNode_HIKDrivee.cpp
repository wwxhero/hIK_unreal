// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNode_HIKDrivee.h"
#include "Animation/AnimTrace.h"
#include "fk_joint.h"
#include "motion_pipeline.h"
#include "ik_logger_unreal.h"


DECLARE_CYCLE_STAT(TEXT("HIK UT"), STAT_HIK_UT_Eval, STATGROUP_Anim);

/////////////////////////////////////////////////////
// FAnimNode_HIKDrivee

FAnimNode_HIKDrivee::FAnimNode_HIKDrivee()
{
	c_inCompSpace = false;
}

FAnimNode_HIKDrivee::~FAnimNode_HIKDrivee()
{

}

HBODY FAnimNode_HIKDrivee::InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
														, const FBoneContainer& RequiredBones
														, const FTransform& skelcomp_l2w
														, const BITree& idx_tree
														, const std::set<FString>& namesOnPair
														, const std::map<FString, FVector>& name2scale)
{
	m_rootTM0_p2l = ref.GetRawRefBonePose()[0].Inverse();
	return Super::InitializeChannelFBX_AnyThread(ref, RequiredBones, skelcomp_l2w, idx_tree, namesOnPair, name2scale);
}

// HBODY FAnimNode_HIKDrivee::InitializeBodySim_AnyThread(HBODY body_fbx)
// {
// 	const wchar_t* (*matches)[2] = NULL;
// 	int n_match = c_animInst->CopyMatches(&matches);
// 	HBODY body_htr_1 = H_INVALID;
// 	HBODY body_htr_2 = H_INVALID;
// 	if (!(clone_body_interests(body_fbx, &body_htr_1, matches, n_match, false)  	// body_htr_1 is an intermediate body, orient bone with src bone information
// 			&& clone_body(body_htr_1, htr, &body_htr_2))) 						    // body_htr_2 is the result, orient bone with the interest bone information
// 		body_htr_2 = H_INVALID;
// #if 0 // defined _DEBUG
// 	UE_LOG(LogHIK, Display, TEXT("ArtiBody_SIM"));
// 	DBG_printOutSkeletalHierachy(body_htr_1);
// 	UE_LOG(LogHIK, Display, TEXT("ArtiBody_SIM2"));
// 	DBG_printOutSkeletalHierachy(body_htr_2);
// #endif

// 	if (VALID_HANDLE(body_htr_1))
// 		destroy_tree_body(body_htr_1);
// 	return body_htr_2;
// }

void FAnimNode_HIKDrivee::InitializeEEFs_AnyThread(const FTransform& skelcomp_l2w
												, const std::set<FString> &eefs_name)
{
	HBODY h_bodyAnim = m_mopipe.bodies[FAnimNode_MotionPipe::c_idxFBX];

	int32 n_eefs = eefs_name.size();
	bool exist_eef = (0 < n_eefs);
	if (!exist_eef)
		return;

	TArray<EndEF_Internal> eefs;
	eefs.Reset(n_eefs);

	const FTransform& c2w = skelcomp_l2w;
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

	TraverseDFS(h_bodyAnim, onEnterBody, onLeaveBody);
	check(m_eefs.Num() == eefs.Num());

	eefs.Sort(FCompareEEF());

	m_eefs.SetNum(n_eefs);
	for (int i_eef = 0; i_eef < n_eefs; i_eef ++)
	{
		m_eefs[i_eef] = eefs[i_eef];
	}
}

void FAnimNode_HIKDrivee::EvaluateSkeletalControl_AnyThread(FPoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_HIK_UT_Eval);

	const FBoneContainer& requiredBones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();

	auto driverHTR = m_mopipe.bodies[0];
	auto moDriverHTR = m_mopipe.mo_nodes[0];
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

		OutBoneTransforms.SetNum(n_channels - 1, false); // update every bone transform on channel but NOT root

		FAnimInstanceProxy_MotionPipe* proxy = static_cast<FAnimInstanceProxy_MotionPipe*>(Output.AnimInstanceProxy);

		_TRANSFORM l2w_body_root;
		get_body_transform_l2p(m_channelsFBX[0].h_body, &l2w_body_root);
		FTransform l2w_unr_root;
		Convert(l2w_body_root, l2w_unr_root);
#if defined _DEBUG
		FTransform tm_entity;
		proxy->PullUpdateEntity(tm_entity);
		check(proxy->GetSkelMeshCompLocalToWorld().Equals(tm_entity, 0.001));
#endif
		// entity_l2p * root0_l2p = root(t)_l2w;
		//		=> entity_l2p = root(t)_l2w * (root0_l2p^-1)
		proxy->PushUpdateEntity(m_rootTM0_p2l * l2w_unr_root);

		for (int i_channel = 1; i_channel < n_channels; i_channel ++)
		{
			FCompactPoseBoneIndex boneCompactIdx = m_channelsFBX[i_channel].r_bone.GetCompactPoseIndex(requiredBones);
			_TRANSFORM l2w_body;
			get_body_transform_l2p(m_channelsFBX[i_channel].h_body, &l2w_body);
			FTransform l2w_unr;
			Convert(l2w_body, l2w_unr);
			FBoneTransform tm_bone(boneCompactIdx, l2w_unr);
			OutBoneTransforms[i_channel - 1] = tm_bone;
		}



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


#if defined _DEBUG

void FAnimNode_HIKDrivee::DBG_VisSIM(FAnimInstanceProxy* animProxy) const
{
	HBODY body_sim = m_mopipe.bodies[FAnimNode_MotionPipe::c_idxSim];
	const auto& src2dst_w = m_mopipe.src2dst_w;
	FMatrix bvh2unrel_w = {
			{src2dst_w[0][0],		src2dst_w[1][0],		src2dst_w[2][0],	0},
			{src2dst_w[0][1],		src2dst_w[1][1],		src2dst_w[2][1],	0},
			{src2dst_w[0][2],		src2dst_w[1][2],		src2dst_w[2][2],	0},
			{0,						0,						0,					1},
	};
	FTransform bvh2unrel(bvh2unrel_w);
	auto lam_onEnter = [this, animProxy, &bvh2unrel] (HBODY h_this)
						{
							_TRANSFORM l2w_body;
							get_body_transform_l2w(h_this, &l2w_body);
							FTransform l2w_unrel;
							Convert(l2w_body, l2w_unrel);
							DBG_VisTransform(l2w_unrel, animProxy);
						};
	auto lam_onLeave = [] (HBODY h_this)
						{

						};
	TraverseDFS(body_sim, lam_onEnter, lam_onLeave);
}



#endif