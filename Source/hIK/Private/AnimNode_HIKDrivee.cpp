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
}

FAnimNode_HIKDrivee::~FAnimNode_HIKDrivee()
{

}

HBODY FAnimNode_HIKDrivee::InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
														, const FBoneContainer& RequiredBones
														, const FTransform& skelcomp_l2w
														, const BITree& idx_tree
														, const std::set<FString>& namesOnPair)
{
	std::size_t n_bone = ref.GetRawBoneNum();
	m_channelsFBX.SetNum(namesOnPair.size(), false);

	TQueue<CHANNEL> queBFS;
	const auto pose = ref.GetRawRefBonePose();
	m_rootTM0_p2l = pose[0].Inverse();
	_TRANSFORM tm;
	Convert(pose[0]*skelcomp_l2w, tm);
	FName bone_name = ref.GetBoneName(0);

	float s_x = 1.0f;
	float s_y = 1.0f;
	float s_z = 1.0f;
	if (c_animInst->CopyScale(0, *bone_name.ToString(), s_x, s_y, s_z))
	{
		tm.s.x *= s_x;
		tm.s.y *= s_y;
		tm.s.z *= s_z;
		check(namesOnPair.end() != namesOnPair.find(*bone_name.ToString()));
	}

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
			if (c_animInst->CopyScale(0, *bone_name.ToString(), s_x, s_y, s_z))
			{
				tm.s.x *= s_x;
				tm.s.y *= s_y;
				tm.s.z *= s_z;
				check(namesOnPair.end() != namesOnPair.find(*bone_name.ToString()));
			}
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
				if (c_animInst->CopyScale(0, *bone_name.ToString(), s_x, s_y, s_z))
				{
					tm.s.x *= s_x;
					tm.s.y *= s_y;
					tm.s.z *= s_z;
					check(namesOnPair.end() != namesOnPair.find(*bone_name.ToString()));
				}
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

HBODY FAnimNode_HIKDrivee::InitializeBodySim_AnyThread(HBODY body_fbx)
{
	const wchar_t* (*matches)[2] = NULL;
	int n_match = c_animInst->CopyMatches(&matches);
	HBODY body_htr_1 = H_INVALID;
	HBODY body_htr_2 = H_INVALID;
	if (!(clone_body_interests(body_fbx, htr, &body_htr_1, matches, n_match, false)  // body_htr_1 is an intermediate body, orient bone with src bone information
			&& clone_body(body_htr_1, htr, &body_htr_2))) 						    // body_htr_2 is the result, orient bone with the interest bone information
		body_htr_2 = H_INVALID;
#if 0 // defined _DEBUG
	UE_LOG(LogHIK, Display, TEXT("ArtiBody_SIM"));
	DBG_printOutSkeletalHierachy(body_htr_1);
	UE_LOG(LogHIK, Display, TEXT("ArtiBody_SIM2"));
	DBG_printOutSkeletalHierachy(body_htr_2);
#endif

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

		OutBoneTransforms.SetNum(n_channels - 1, false); // update every bone transform on channel but NOT root

		FAnimInstanceProxy_MotionPipe* proxy = static_cast<FAnimInstanceProxy_MotionPipe*>(Output.AnimInstanceProxy);

		_TRANSFORM l2w_body_root;
		get_body_transform_l2p(m_channelsFBX[0].h_body, &l2w_body_root);
		FTransform l2w_unr_root;
		Convert(l2w_body_root, l2w_unr_root);
#if defined _DEBUG
		FTransform tm_entity;
		proxy->PullUpdateEntity(tm_entity);
		check(proxy->GetSkelMeshCompLocalToWorld().Equals(tm_entity, 0.01));
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
		//	DBG_VisTargets(proxy);
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
	HBODY body_sim = m_bodies[FAnimNode_MotionPipe::c_idxSim];
	FMatrix bvh2unrel_m;
	c_animInst->CopySrc2Dst_w(bvh2unrel_m);
	FTransform bvh2unrel(bvh2unrel_m);
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

void FAnimNode_HIKDrivee::DBG_VisCHANNELs(FAnimInstanceProxy* animProxy) const
{
	for (auto channel: m_channelsFBX)
	{
		_TRANSFORM l2c_sim;
		get_body_transform_l2w(channel.h_body, &l2c_sim);
		FTransform l2c_sim_2;
		Convert(l2c_sim, l2c_sim_2);
		// FTransform l2w_sim = l2c_sim_2 * animProxy->GetSkelMeshCompLocalToWorld();
		FTransform l2w_sim = l2c_sim_2;
		DBG_VisTransform(l2w_sim, animProxy);
	}
}

#endif