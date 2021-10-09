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
	c_inCompSpace = true;
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

void FAnimNode_FKRecordUT::InitializeTargets_AnyThread(HBODY h_bodyFBX
												, const FTransform& skelecom_l2w
												, const std::set<FString> &targets_name)
{
	int32 n_targets = targets_name.size();
	bool exist_target = (0 < n_targets);
	if (!exist_target)
		return;

	TArray<Target_Internal> targets;
	targets.Reset(n_targets);

	const FTransform& c2w = skelecom_l2w;
	auto onEnterBody = [this, &targets, &targets_name, &c2w] (HBODY h_this)
		{
			FString name_this(body_name_w(h_this));
			bool is_target = (targets_name.end() != targets_name.find(name_this));
			if (is_target)
			{
				_TRANSFORM tm_l2c;
				get_body_transform_l2w(h_this, &tm_l2c);
				FTransform tm_l2c_2;
				Convert(tm_l2c, tm_l2c_2);
				Target_Internal target;
				InitializeTarget_Internal(&target, name_this, tm_l2c_2 * c2w, h_this);
				targets.Add(target);
			}

		};

	auto onLeaveBody = [] (HBODY h_this)
		{

		};

	TraverseDFS(h_bodyFBX, onEnterBody, onLeaveBody);

	targets.Sort(FCompareTarget());

	m_targets.SetNum(n_targets);
	for (int i_target = 0; i_target < n_targets; i_target ++)
	{
		m_targets[i_target] = targets[i_target];
	}
}

void FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread(FPoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);

	const FBoneContainer& requiredBones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();

	auto driverBVH = m_mopipe->bodies[FAnimNode_MotionPipe::c_idxSim];
	auto moDriverBVH = m_mopipe->mo_nodes[FAnimNode_MotionPipe::c_idxSim];
	bool exists_a_channel = (m_channelsFBX.Num() > 0);
	bool ok = (VALID_HANDLE(driverBVH)
			&& VALID_HANDLE(moDriverBVH)
			&& exists_a_channel);

	IKAssert(ok);

	if (ok)
	{
		fk_update(m_mopipe, c_animInstDriver->I_Frame_);

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

		FAnimInstanceProxy_MotionPipe* proxy = static_cast<FAnimInstanceProxy_MotionPipe*> (Output.AnimInstanceProxy);
#if defined _DEBUG
		check(proxy->ValidPtr() && proxy->EmptyTargets());
#endif
		int32 n_targets = m_targets.Num();
		FTransform c2w = proxy->GetSkelMeshCompLocalToWorld();
		for (int i_target = 0; i_target < n_targets; i_target ++)
		{
			Target_Internal& target_i = m_targets[i_target];
			_TRANSFORM l2c;
			get_body_transform_l2w(target_i.h_body, &l2c);
			FTransform l2c_2;
			Convert(l2c, l2c_2);
			target_i.tm_l2w = l2c_2 * c2w;
			proxy->PushUpdateTarget(target_i);
		}
#if defined _DEBUG
		// DBG_VisCHANNELs(Output.AnimInstanceProxy);
		// DBG_VisSIM(Output.AnimInstanceProxy);
		// DBG_VisTargets(proxy);
#endif

	}
}


#if defined _DEBUG
void FAnimNode_FKRecordUT::DBG_VisSIM(FAnimInstanceProxy* animProxy) const
{
	HBODY body_sim = m_mopipe->bodies[FAnimNode_MotionPipe::c_idxSim];
	const auto& src2dst_w = m_mopipe->src2dst_w;
	FMatrix bvh2unrel_w = {
			{(float)src2dst_w[0][0],		(float)src2dst_w[1][0],		(float)src2dst_w[2][0],	0},
			{(float)src2dst_w[0][1],		(float)src2dst_w[1][1],		(float)src2dst_w[2][1],	0},
			{(float)src2dst_w[0][2],		(float)src2dst_w[1][2],		(float)src2dst_w[2][2],	0},
			{0,								0,							0,						1},
	};
	FTransform bvh2unrel(bvh2unrel_w);
	auto lam_onEnter = [this, animProxy, &bvh2unrel] (HBODY h_this)
						{
							_TRANSFORM l2c_body;
							get_body_transform_l2w(h_this, &l2c_body);
							FTransform l2c_bvh;
							Convert(l2c_body, l2c_bvh);
							FTransform l2w = l2c_bvh * bvh2unrel * animProxy->GetSkelMeshCompLocalToWorld();
							DBG_VisTransform(l2w, animProxy);
						};
	auto lam_onLeave = [] (HBODY h_this)
						{

						};
	TraverseDFS(body_sim, lam_onEnter, lam_onLeave);
}



#endif


