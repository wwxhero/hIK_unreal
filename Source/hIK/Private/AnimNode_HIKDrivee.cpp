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

void FAnimNode_HIKDrivee::InitializeEEFs_AnyThread(HBODY h_bodyFbx
												, const FTransform& skelcomp_l2w
												, const std::set<FString> &eefs_name)
{
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

	TraverseDFS(h_bodyFbx, onEnterBody, onLeaveBody);

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

	auto driverHTR = m_mopipe->bodies[0];
	auto moDriverHTR = m_mopipe->mo_nodes[0];
	bool exists_a_channel = (m_channelsFBX.Num() > 0);
	bool ok = (VALID_HANDLE(driverHTR)
			&& VALID_HANDLE(moDriverHTR)
			&& exists_a_channel);

	IKAssert(ok);

	if (ok)
	{
		FAnimInstanceProxy_MotionPipe* proxy = static_cast<FAnimInstanceProxy_MotionPipe*>(Output.AnimInstanceProxy);

		TArray<EndEF> eefs_i;
		bool empty_update = proxy->EmptyEndEEFs();
		// LOGIKVar(LogInfoBool, empty_update);
		if (!empty_update)
		{
			proxy->PullUpdateEEFs(eefs_i);
			int32 n_eefs = eefs_i.Num();
			check(n_eefs == m_eefs.Num());
			for (int32 i_eef = 0; i_eef < n_eefs; i_eef ++)
			{
				check(eefs_i[i_eef].name == m_eefs[i_eef].name);
				m_eefs[i_eef].tm_l2w = eefs_i[i_eef].tm_l2w;
			}

#if defined _DEBUG
			DBG_VisTargets(proxy);
#endif

			for (auto& eef_i : m_eefs)
			{
				_TRANSFORM l2w_i;
				Convert(eef_i.tm_l2w, l2w_i);
				ik_task(eef_i.h_body, &l2w_i);
			}
			ik_update(m_mopipe);
		}
#if defined _DEBUG
		// DBG_VisCHANNELs(Output.AnimInstanceProxy);
		DBG_VisSIM(Output.AnimInstanceProxy);

		// LOGIKVar(LogInfoInt, proxy->GetEEFs_i().Num());
#endif
		int n_channels = m_channelsFBX.Num();

		OutBoneTransforms.SetNum(n_channels - 1, false); // update every bone transform on channel but NOT root



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



	}
}


// #if defined _DEBUG

void FAnimNode_HIKDrivee::DBG_VisSIM(FAnimInstanceProxy* animProxy) const
{
	HBODY body_sim = m_mopipe->bodies[FAnimNode_MotionPipe::c_idxSim];
	const auto& src2dst_w = m_mopipe->src2dst_w;
	FMatrix sim2anim_w = {
			{src2dst_w[0][0],		src2dst_w[1][0],		src2dst_w[2][0],	0},
			{src2dst_w[0][1],		src2dst_w[1][1],		src2dst_w[2][1],	0},
			{src2dst_w[0][2],		src2dst_w[1][2],		src2dst_w[2][2],	0},
			{0,						0,						0,					1},
	};
	FMatrix anim2sim_w = sim2anim_w.Inverse();
	FTransform sim2anim_w_tm(sim2anim_w);
	FQuat sim2anim_w_q(sim2anim_w_tm.ToMatrixNoScale());
	FQuat anim2sim_w_q = sim2anim_w_q.Inverse();
	auto lam_onEnter = [this, animProxy, &sim2anim_w_q, &anim2sim_w_q] (HBODY h_this)
						{
							_TRANSFORM l2w_body_sim;
							get_body_transform_l2w(h_this, &l2w_body_sim);
							FTransform l2w_body_sim_u;
							Convert(l2w_body_sim, l2w_body_sim_u);

							FQuat l2w_sim_q = l2w_body_sim_u.GetRotation();
							FQuat l2w_anim_q = sim2anim_w_q * l2w_sim_q * anim2sim_w_q;

							FVector l2w_sim_tt = l2w_body_sim_u.GetTranslation();
							FVector l2w_anim_tt = sim2anim_w_q * l2w_sim_tt;

							FTransform l2w_body_anim(l2w_anim_q, l2w_anim_tt);

							DBG_VisTransform(l2w_body_anim, animProxy);
						};
	auto lam_onLeave = [] (HBODY h_this)
						{

						};
	TraverseDFS(body_sim, lam_onEnter, lam_onLeave);
}



// #endif