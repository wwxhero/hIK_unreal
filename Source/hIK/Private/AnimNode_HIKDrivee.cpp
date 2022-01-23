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
	: c_animInstDrivee(NULL)
{
	c_inCompSpace = false;
}

FAnimNode_HIKDrivee::~FAnimNode_HIKDrivee()
{

}

void FAnimNode_HIKDrivee::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	Super::OnInitializeAnimInstance(InProxy, InAnimInstance);
	c_animInstDrivee = Cast<UAnimInstance_HIKDrivee, UAnimInstance>(InAnimInstance);
	check(nullptr != c_animInstDrivee);
}


HBODY FAnimNode_HIKDrivee::InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
														, const FBoneContainer& RequiredBones
														, const FTransform& skelcomp_l2w
														, const BITree& idx_tree
														, const std::set<FString>& namesOnPair
														, const std::map<FString, FVector>& name2scale)
{
	m_rootTM0_p2l = ref.GetRawRefBonePose()[0].Inverse();
#if defined _DEBUG
	FAnimNode_MotionPipe::DBG_LogTransform(FString("m_rootTM0_l2p"), &ref.GetRawRefBonePose()[0]);
	FAnimNode_MotionPipe::DBG_LogTransform(FString("skelcomp_l2w"), &skelcomp_l2w);
#endif

	FString arm_l, arm_r;
	float scale;
	if (c_animInstDrivee->ScaleArm(&arm_l, &arm_r, &scale))
	{
		std::map<FString, FVector> name2scale_prime(name2scale);
		FVector scale_prime(scale, scale, scale);
		name2scale_prime[arm_l] = scale_prime;
		name2scale_prime[arm_r] = scale_prime;
		return Super::InitializeChannelFBX_AnyThread(ref, RequiredBones, skelcomp_l2w, idx_tree, namesOnPair, name2scale_prime);
	}
	else
		return Super::InitializeChannelFBX_AnyThread(ref, RequiredBones, skelcomp_l2w, idx_tree, namesOnPair, name2scale);

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

		TArray<Target> targets_i;
		bool empty_update = proxy->EmptyTargets();
		// LOGIKVar(LogInfoBool, empty_update);
		if (!empty_update)
		{
			proxy->PullUpdateTargets(targets_i);
			int32 n_targets = targets_i.Num();
			check(n_targets == m_targets.Num());
			for (int32 i_target = 0; i_target < n_targets; i_target ++)
			{
				check(targets_i[i_target].name == m_targets[i_target].name);
				m_targets[i_target].tm_l2w = targets_i[i_target].tm_l2w;
			}

#if 0 //defined _DEBUG
			DBG_VisTargets(proxy);
#endif
			bool exists_a_task = false;
			// const Real epsilon_r = 5 / 180 * PI;		// in radian
			// const Real epsilon_tt = 1;				// in centimeters
			for (auto& target_i : m_targets)
			{
				_TRANSFORM l2c_i;
				Convert(target_i.tm_l2w * m_WtoC0, l2c_i);
				bool updated = ik_task_update(target_i.h_body, &l2c_i);
				exists_a_task = exists_a_task || updated;
			}
			if (exists_a_task)
				ik_update(m_mopipe);

#if defined _DEBUG
			DBG_VisEEFs(proxy);
#endif
		}
		else if(proxy->PullIKReset())	// make sure to push an empty set of targets along with the IKReset requests
		{
			ik_reset(m_mopipe);
			for (auto& target : m_targets)
			{
				_TRANSFORM tm_l2c;
				get_body_transform_LtoC0(target.h_body, &tm_l2c);
				FTransform tm_l2c_2;
				Convert(tm_l2c, tm_l2c_2);
				target.tm_l2w = tm_l2c_2 * m_C0toW;
			}
			TArray<Target> targets;
			for (auto target_i : m_targets)
				targets.Add(target_i);
			proxy->PushUpdateTargets(targets); // this target is not for update, but for initial binding
		}
#if defined _DEBUG
		// if (1 == c_animInstDrivee->DBG_VisBody_i)
		// 	DBG_VisCHANNELs(Output.AnimInstanceProxy);
		// else
		// 	DBG_VisSIM(Output.AnimInstanceProxy);

		// LOGIKVar(LogInfoInt, proxy->GetTargets_i().Num());
#endif
		int n_channels = m_channelsFBX.Num();

		OutBoneTransforms.SetNum(n_channels - 1, false); // update every bone transform on channel but NOT root

		_TRANSFORM l2c_body_root;
		get_body_transform_l2p(m_channelsFBX[0].h_body, &l2c_body_root);
		FTransform l2c0_unr_root;
		Convert(l2c_body_root, l2c0_unr_root);
		FTransform l2w_unr_root = l2c0_unr_root * m_C0toW;
#if 0 // defined _DEBUG
		FTransform tm_entity;
		proxy->PullUpdateEntity(tm_entity);
		check(proxy->GetSkelMeshCompLocalToWorld().Equals(tm_entity, 0.001));
#endif
		// entity_l2w(t) * root0_l2p = root(t)_l2w;
		//		=> entity_l2w(t) = root(t)_l2w * (root0_l2p^-1)
		proxy->PushUpdateEntity(m_rootTM0_p2l * l2w_unr_root);

		// entity_l2w(t) * root0_l2p = root(t)_l2w = c2w0 * root(t)_l2c;
		//		=> entity_l2w(t) = root(t)_l2w * (root0_l2p^-1)
		// proxy->PushUpdateEntity(m_rootTM0_l2p * delta_unr_root * m_rootTM0_p2l);

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

#if defined _DEBUG

void FAnimNode_HIKDrivee::DBG_VisSIM(FAnimInstanceProxy* animProxy) const
{
	HBODY body_sim = m_mopipe->bodies[FAnimNode_MotionPipe::c_idxSim];
	const auto& src2dst_w = m_mopipe->src2dst_w;
	FMatrix sim2anim_w = {
			{(float)src2dst_w[0][0],		(float)src2dst_w[1][0],		(float)src2dst_w[2][0],	0},
			{(float)src2dst_w[0][1],		(float)src2dst_w[1][1],		(float)src2dst_w[2][1],	0},
			{(float)src2dst_w[0][2],		(float)src2dst_w[1][2],		(float)src2dst_w[2][2],	0},
			{0,								0,							0,						1},
	};
	float axis_len = 20;
	float thickness = 2;
	auto lam_onEnter = [this, animProxy, &sim2anim_w, &axis_len, &thickness] (HBODY h_this)
						{
							_TRANSFORM l2c0_body_sim;
							get_body_transform_LtoC0(h_this, &l2c0_body_sim);
							FTransform l2c0_body_sim_u;
							Convert(l2c0_body_sim, l2c0_body_sim_u);
							FTransform l_sim2c0_anim = l2c0_body_sim_u * FTransform(sim2anim_w);
							DBG_VisTransform(l_sim2c0_anim * m_C0toW, animProxy, axis_len, thickness);

						};
	auto lam_onLeave = [] (HBODY h_this)
						{

						};
	lam_onEnter(body_sim);

	axis_len = 10; thickness = 1;
	for (HBODY body_sim_sub = get_first_child_body(body_sim)
		; VALID_HANDLE(body_sim_sub)
		; body_sim_sub = get_next_sibling_body(body_sim_sub))
		TraverseDFS(body_sim_sub, lam_onEnter, lam_onLeave);
		// lam_onEnter(body_sim_sub);
}

#endif