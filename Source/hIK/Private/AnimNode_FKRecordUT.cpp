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

HBODY FAnimNode_FKRecordUT::InitializeBodySim_AnyThread(HBODY /*body_fbx*/)
{
	check(nullptr != c_animInstDriver);
	HBODY driverBVH = H_INVALID;
	bool bvh_load = (VALID_HANDLE(c_animInstDriver->getBVH())
				&& VALID_HANDLE(driverBVH = create_tree_body_bvh(c_animInstDriver->getBVH())));
	LOGIKVar(LogInfoBool, bvh_load);
	return driverBVH;
}

void FAnimNode_FKRecordUT::InitializeEEFs_AnyThread(const FTransform& skelecom_l2w, TArray<EndEF_Internal>& eefs)
{
	HBODY h_bodyFBX = m_bodies[FAnimNode_MotionPipe::c_idxFBX];
	std::set<FString> eefs_name;
	int32 n_eefs = c_animInstDriver->CopyEEFs(eefs_name, FAnimNode_MotionPipe::c_idxFBX);
	bool exist_eef = (0 < n_eefs);
	if (!exist_eef)
		return;

	std::map<FString, FString> fbx2sim;
	c_animInstDriver->CopyMatches(fbx2sim, FAnimNode_MotionPipe::c_idxFBX);

	TArray<Target_Internal> targets;
	targets.Reset(n_eefs);

	const FTransform& c2w = skelecom_l2w;
	auto onEnterBody = [this, &targets, &eefs_name, &fbx2sim, &c2w] (HBODY h_this)
		{
			FString name_this(body_name_w(h_this));
			bool is_eef = (eefs_name.end() != eefs_name.find(name_this));
			if (is_eef)
			{
				_TRANSFORM tm_l2c;
				get_body_transform_l2w(h_this, &tm_l2c);
				FTransform tm_l2c_2;
				Convert(tm_l2c, tm_l2c_2);
				auto it_src = fbx2sim.find(name_this);
				check (fbx2sim.end() != it_src);
				Target_Internal target;
				InitializeTarget_Internal(&target, name_this, it_src->second, tm_l2c_2 * c2w, h_this);
				targets.Add(target);
			}

		};

	auto onLeaveBody = [] (HBODY h_this)
		{

		};

	TraverseDFS(h_bodyFBX, onEnterBody, onLeaveBody);

	targets.Sort(FCompareEEF());

	eefs.SetNum(n_eefs);
	for (int i_eef = 0; i_eef < n_eefs; i_eef ++)
	{
		eefs[i_eef] = targets[i_eef];
	}
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

		FAnimInstanceProxy_MotionPipe* proxy = static_cast<FAnimInstanceProxy_MotionPipe*> (Output.AnimInstanceProxy);
#if defined _DEBUG
		check(proxy->ValidPtr() && proxy->EmptyEndEEFs());
#endif
		int32 n_eefs = m_eefs.Num();
		FTransform c2w = proxy->GetSkelMeshCompLocalToWorld();
		for (int i_eef = 0; i_eef < n_eefs; i_eef ++)
		{
			EndEF_Internal& eef_i = m_eefs[i_eef];
			_TRANSFORM l2c;
			get_body_transform_l2w(eef_i.h_body, &l2c);
			FTransform l2c_2;
			Convert(l2c, l2c_2);
			eef_i.tm_l2w = l2c_2 * c2w;
			proxy->PushUpdateEEF(eef_i);
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



#endif


