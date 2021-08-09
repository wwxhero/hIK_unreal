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

HBODY FAnimNode_FKRecordUT::InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
											, const FBoneContainer& RequiredBones
											, const BITree& idx_tree
											, const std::set<FString>& namesOnPair)
{
	std::size_t n_bone = ref.GetRawBoneNum();
	m_channelsFBX.SetNum(namesOnPair.size(), false);

	TQueue<CHANNEL> queBFS;
	const auto pose = ref.GetRawRefBonePose();
	_TRANSFORM tm;
	Convert(pose[0], tm);
	FName bone_name = ref.GetBoneName(0);
	FVector bone_scale;

	std::map<FString, FVector> name2scale;
	c_animInst->CopyScale(FAnimNode_MotionPipe::c_idxFBX, name2scale);

	auto AppScale = [&name2scale](const FName& bone_name, _SCALE& scale)
		{
			auto it_scale = name2scale.find(*bone_name.ToString());
			bool exist_scale = it_scale != name2scale.end();
			if (exist_scale)
			{
				const FVector& scale_d = it_scale->second;
				scale.x *= scale_d.X;
				scale.y *= scale_d.Y;
				scale.z *= scale_d.Z;
			}
		};

	AppScale(bone_name, tm.s);

	CHANNEL ch_node_root =
		{
			FBoneReference(bone_name),
			create_fbx_body_node_w
				(
					  *bone_name.ToString()
					, &tm
				)
		};

	DBG_LogTransform(*bone_name.ToString(), &tm);

	HBODY root_body = ch_node_root.h_body;
	queBFS.Enqueue(ch_node_root);

	std::size_t i_channel = 0;
	CHANNEL ch_node;
	while (queBFS.Dequeue(ch_node))
	{
		ch_node.r_bone.Initialize(RequiredBones);
		bone_name = ref.GetBoneName(ch_node.r_bone.BoneIndex);
		if (namesOnPair.end() != namesOnPair.find(*bone_name.ToString()))
			m_channelsFBX[i_channel ++] = ch_node;

		TLinkedList<int32>* children_i = idx_tree[ch_node.r_bone.BoneIndex];
		if (NULL != children_i)
		{
			auto it_child = begin(*children_i);
			int32 id_child = *it_child;
			bone_name = ref.GetBoneName(id_child);
			Convert(pose[id_child], tm);
			// if (c_animInst->CopyScale(0, *bone_name.ToString(), s_x, s_y, s_z))
			AppScale(bone_name, tm.s);
			CHANNEL ch_node_child =
						{
							FBoneReference(bone_name),
							create_fbx_body_node_w
								(
									  *bone_name.ToString()
									, &tm
								)
						};
			queBFS.Enqueue(ch_node_child);
			cnn_arti_body(ch_node.h_body, ch_node_child.h_body, CNN::FIRSTCHD);
			for (it_child ++
				; it_child
				; it_child ++)
			{
				id_child = *it_child;
				bone_name = ref.GetBoneName(id_child);
				Convert(pose[id_child], tm);
				AppScale(bone_name, tm.s);
				CHANNEL ch_node_child_next =
						{
							FBoneReference(bone_name),
							create_fbx_body_node_w
								(
									  *bone_name.ToString()
									, &tm
								)
						};
				cnn_arti_body(ch_node_child.h_body, ch_node_child_next.h_body, CNN::NEXTSIB);
				ch_node_child = ch_node_child_next;
				queBFS.Enqueue(ch_node_child);
			}
		}
	}

	struct FCompareChannel
	{
		FORCEINLINE bool operator()(const CHANNEL& A, const CHANNEL& B) const
		{
			return A.r_bone.BoneIndex < B.r_bone.BoneIndex;
		}
	};
	m_channelsFBX.Sort(FCompareChannel());
	initialize_kina(root_body);
	update_fk(root_body);
#if defined _DEBUG
	UE_LOG(LogHIK, Display, TEXT("Number of bones: %d"), n_bone);
	DBG_printOutSkeletalHierachy(root_body);
	DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	for (auto channel : m_channelsFBX)
	{
		check(ValidCHANNEL(channel));
	}
#endif
	return root_body;
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

void FAnimNode_FKRecordUT::InitializeEEFs_AnyThread(FAnimInstanceProxy_MotionPipe* proxy, TArray<EndEF_Internal>& eefs)
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

	const FTransform& c2w = proxy->GetSkelMeshCompLocalToWorld();
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

void FAnimNode_FKRecordUT::DBG_VisCHANNELs(FAnimInstanceProxy* animProxy) const
{
	for (auto channel: m_channelsFBX)
	{
		_TRANSFORM l2c_sim;
		get_body_transform_l2w(channel.h_body, &l2c_sim);
		FTransform l2c_sim_2;
		Convert(l2c_sim, l2c_sim_2);
		FTransform l2w_sim = l2c_sim_2 * animProxy->GetSkelMeshCompLocalToWorld();
		DBG_VisTransform(l2w_sim, animProxy);
	}
}




