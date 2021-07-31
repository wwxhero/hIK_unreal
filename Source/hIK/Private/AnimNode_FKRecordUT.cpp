// Copyright(c) Mathew Wang 2021

#include "AnimNode_FKRecordUT.h"
#include <map>
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "bvh.h"
#include "fk_joint.h"
#include "motion_pipeline.h"
#include "conf_mopipe.h"
#include "DrawDebugHelpers.h"
#include "ik_logger.h"



bool FAnimNode_FKRecordUT::LoadConf(HCONF hConf)
{
	HCONFMOPIPE hConfFKRC = init_fkrc(hConf);
	bool valid_fkrc = (VALID_HANDLE(hConfFKRC));
	LOGIKVar(LogInfoBool, valid_fkrc);
	bool ok = valid_fkrc;
	if (ok)
	{
		m_nScales = load_dst_scale(hConfFKRC, &m_scales);
		bool mtx_loaded = get_mopipe_mtx(hConfFKRC, m_bvh2fbxWorld);
		m_nMatches = load_mopipe_pairs(hConfFKRC, &m_match);
		m_nTargets = load_dst_endeff_names(hConfFKRC, &m_targetnames);
		bool scale_loaded = (m_nScales > -1);
		bool match_loaded = (m_nMatches > 0);
		bool targets_loaded = (m_nTargets > 0);
		ok = ( scale_loaded && mtx_loaded && match_loaded && targets_loaded);
		LOGIKVar(LogInfoBool, scale_loaded);
		LOGIKVar(LogInfoBool, mtx_loaded);
		LOGIKVar(LogInfoBool, match_loaded);
		LOGIKVar(LogInfoBool, targets_loaded);
	}
	uninit_fkrc(hConfFKRC);
	return ok;
}

void FAnimNode_FKRecordUT::UnLoadConf()
{
	free_dst_scale(m_scales, m_nScales);
	m_scales = NULL; m_nScales = 0;
	free_mopipe_pairs(m_match, m_nMatches);
	m_match = NULL; m_nMatches = 0;
	free_dst_endeff_names(m_targetnames, m_nTargets);
	m_targetnames = NULL; m_nTargets = 0;
}


inline void printArtName(const TCHAR* name, int n_indent)
{
	FString item;
	for (int i_indent = 0
		; i_indent < n_indent
		; i_indent ++)
		item += TEXT("\t");
	item += name;
	UE_LOG(LogHIK, Display, TEXT("%s"), *item);
}


template<typename LAMaccessEnter, typename LAMaccessLeave>
inline void TraverseDFS(HBODY root, LAMaccessEnter OnEnterBody, LAMaccessLeave OnLeaveBody)
{
	check(VALID_HANDLE(root));
	typedef struct _EDGE
	{
		HBODY body_this;
		HBODY body_child;
	} EDGE;
	std::stack<EDGE> stkDFS;
	stkDFS.push({root, get_first_child_body(root)});
	//printArtName(body_name_w(root), 0);
	OnEnterBody(root);
	while (!stkDFS.empty())
	{
		EDGE &edge = stkDFS.top();
		int n_indent = stkDFS.size();
		if (!VALID_HANDLE(edge.body_child))
		{
			stkDFS.pop();
			OnLeaveBody(edge.body_this);
		}
		else
		{
			//printArtName(body_name_w(edge.body_child), n_indent);
			OnEnterBody(edge.body_child);
			HBODY body_grandchild = get_first_child_body(edge.body_child);
			HBODY body_nextchild = get_next_sibling_body(edge.body_child);
			stkDFS.push({edge.body_child, body_grandchild});
			edge.body_child = body_nextchild;
		}
	}
}



DECLARE_CYCLE_STAT(TEXT("FK UT"), STAT_FK_UT_Eval, STATGROUP_Anim);

void FAnimNode_FKRecordUT::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);
	GetEvaluateGraphExposedInputs().Execute(Context);
	BasePose.Update(Context);
}

void FAnimNode_FKRecordUT::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);
	FAnimNode_Base::CacheBones_AnyThread(Context);
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
	BasePose.CacheBones(Context);
}

void FAnimNode_FKRecordUT::Evaluate_AnyThread(FPoseContext& Output)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);

	BasePose.Evaluate(Output);

	BoneTransforms.Reset(BoneTransforms.Num());
	EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);

	for (auto b_tm : BoneTransforms)
	{
		Output.Pose[b_tm.BoneIndex] = b_tm.Transform;
	}
}

void FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread(FPoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);

	bool exists_a_channel = (m_channels.Num() > 0);

	if (!exists_a_channel)
		return;

	const bool rotate_on_entity = false;

	const FBoneContainer& requiredBones = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();
	USkeletalMeshComponent* skeleton = Output.AnimInstanceProxy->GetSkelMeshComponent();
	AActor* owner = skeleton->GetOwner();
	if (owner)
	{
		pose_body(m_animInst->getBVH(), m_driverBVH, m_animInst->I_Frame_);
		motion_sync(m_moDriverBVH);

		int n_channels = m_channels.Num();

		OutBoneTransforms.SetNum(n_channels, false);

		for (int i_channel = 0; i_channel < n_channels; i_channel ++)
		{
			FCompactPoseBoneIndex boneCompactIdx = m_channels[i_channel].r_bone.GetCompactPoseIndex(requiredBones);
			_TRANSFORM l2w_body;
			get_body_transform_l2p(m_channels[i_channel].h_body, &l2w_body);
			FTransform l2w_unr;
			Convert(l2w_body, l2w_unr);
			FBoneTransform tm_bone(boneCompactIdx, l2w_unr);
			OutBoneTransforms[i_channel] = tm_bone;
		}

		auto c2w = owner->GetActorTransform();
		auto c2w_2 = owner->GetTransform();
		check(c2w.Equals(c2w_2));

		TArray<UAnimInstance_HIK::Target>* targets = const_cast<UAnimInstance_HIKDriver*>(m_animInst)->LockTarget();
		if (NULL != targets)
		{
			auto n_targets = targets->Num();
			for (int i_target = 0; i_target < n_targets; i_target ++)
			{
				const HBODY& body_i = (*targets)[i_target].h_body;
				FTransform& trans_i_unrealw = (*targets)[i_target].tm_l2w;
				_TRANSFORM tran_i_bvh_w;
				get_body_transform_l2w((*targets)[i_target].h_body, &tran_i_bvh_w);
				FTransform tran_i_unreal_c;
				Convert(tran_i_bvh_w, tran_i_unreal_c);
				trans_i_unrealw = tran_i_unreal_c * c2w;
			}
		}



#if defined _DEBUG
		auto world = owner->GetWorld();
		FMatrix bvh2unrel_m = {
			{m_bvh2fbxWorld[0][0],		m_bvh2fbxWorld[1][0],		m_bvh2fbxWorld[2][0],	0},
			{m_bvh2fbxWorld[0][1],		m_bvh2fbxWorld[1][1],		m_bvh2fbxWorld[2][1],	0},
			{m_bvh2fbxWorld[0][2],		m_bvh2fbxWorld[1][2],		m_bvh2fbxWorld[2][2],	0},
			{0,							0,							0,						1},
		};
		FTransform bvh2unrel(bvh2unrel_m);
		// DBG_VisTransform(world, bvh2unrel, m_driverHTR, 0);
		// DBG_VisTransform(world, owner->GetTransform(), m_driverStub, 1);
		FVector offset(300, 0, 0);
		FTransform tm_offset(offset);
		// DBG_VisTransform(world, bvh2unrel*tm_offset, m_driverBVH, 0);
		DBG_VisTargetTransform(world, targets);
#endif
		const_cast<UAnimInstance_HIKDriver*>(m_animInst)->UnLockTarget(targets);
		// OutBoneTransforms.SetNum(1, false);
		// const FTransform* l2world = NULL;
		// if (rotate_on_entity)
		// {
		// 	l2world = &owner->GetTransform();
		// 	const FTransform& tm_entity = Output.AnimInstanceProxy->GetSkelMeshCompOwnerTransform();
		// 	check(tm_entity.Equals(*l2world, c_epsilon));
		// }
		// else
		// {
		// 	FCompactPoseBoneIndex boneCompactIdx = m_channels[0].r_bone.GetCompactPoseIndex(requiredBones);
		// 	l2world = &Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
		// }

		// float delta_deg = m_animInst->I_Frame_;
		// const float c_deg2rad = PI / 180;
		// FVector axis(0, 0, 1);
		// float delta_rad = delta_deg * c_deg2rad;
		// FQuat delta_q(axis, delta_rad);
		// FTransform delta_world(delta_q);
		// FTransform l2world_prime = (*l2world) * delta_world;
		// if (rotate_on_entity)
		// 	owner->SetActorTransform(l2world_prime);
		// else
		// {
		// 	FCompactPoseBoneIndex boneCompactIdx = m_channels[0].r_bone.GetCompactPoseIndex(requiredBones);
		// 	FBoneTransform tm_bone(boneCompactIdx, l2world_prime);
		// 	OutBoneTransforms.Add(tm_bone);
		// 	delta_deg = ik_test(delta_deg);
		// }
	}

	// IAnimClassInterface* anim = Output.AnimInstanceProxy->GetAnimClassInterface();
	// int32 n_bones = m_channels.Num();

	// auto name_owner = (NULL == owner) ? "None" : owner->GetName();
	// const FTransform* tm_owner = (NULL == owner) ? NULL : &(owner->GetTransform());
	// DBG_LogTransform(name_owner, tm_owner);
	// auto name = skeleton->GetName();
	// DBG_LogTransform(name, NULL);

	// for (int32 i_bone = 0
	// 	; i_bone < n_bones
	// 	; i_bone ++)
	// {
	// 	const auto & r_bone_i = m_channels[i_bone].r_bone;
	// 	FCompactPoseBoneIndex boneCompactIdx = r_bone_i.GetCompactPoseIndex(requiredBones);
	// 	FTransform l2enti = Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
	// 	DBG_LogTransform(*r_bone_i.BoneName.ToString(), &l2enti);
	// }



	// FString result[] = {"failed", "successful"};
	// for (int32 i_bone = 0
	// 	; i_bone < n_bones
	// 	; i_bone ++)
	// {
	// 	const auto & r_bone_i = m_channels[i_bone].r_bone;
	// 	FCompactPoseBoneIndex boneCompactIdx = r_bone_i.GetCompactPoseIndex(requiredBones);
	// 	FTransform l2enti = Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
	// 	_TRANSFORM tm;
	// 	DBG_GetComponentSpaceTransform2(m_channels[i_bone], tm, refSkele);
	// 	int i_result = ( DBG_EqualTransform(l2enti, tm) ? 1 : 0 );
	// 	UE_LOG(LogHIK, Display, TEXT("confirm %s %s"), *r_bone_i.BoneName.ToString(), *result[i_result]);
	// }
// #endif
}

HBODY FAnimNode_FKRecordUT::InitializeChannel(const FReferenceSkeleton& ref
											, const FBoneContainer& RequiredBones
											, const BITree& idx_tree
											, int i_match_col)
{
	for (int i_match = 0
		; i_match < m_nMatches
		; i_match ++)
	{
		m_mapmatch[0].insert(m_match[i_match][0]);
		m_mapmatch[1].insert(m_match[i_match][1]);
	}

	std::set<std::wstring> set_targets;
	for (int i_target = 0; i_target < m_nTargets; i_target ++)
		set_targets.insert(m_targetnames[i_target]);

	TArray<UAnimInstance_HIK::Target> targets;
	targets.SetNum(m_nTargets);

	std::size_t n_bone = ref.GetRawBoneNum();
	m_channels.SetNum(m_nMatches, false);

	TQueue<CHANNEL> queBFS;
	const auto pose = ref.GetRawRefBonePose();
	_TRANSFORM tm;
	Convert(pose[0], tm);
	FName bone_name = ref.GetBoneName(0);

	float s_x = 1.0f;
	float s_y = 1.0f;
	float s_z = 1.0f;
	if (getScale(*bone_name.ToString(), s_x, s_y, s_z))
	{
		tm.s.x *= s_x;
		tm.s.y *= s_y;
		tm.s.z *= s_z;
		check(m_mapmatch[0].end() != m_mapmatch[0].find(*bone_name.ToString())
			|| m_mapmatch[1].end() != m_mapmatch[1].find(*bone_name.ToString()));
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
	HBODY root_body = ch_node_root.h_body;
	queBFS.Enqueue(ch_node_root);

	int32 i_target = 0;
	std::size_t i_channel = 0;
	CHANNEL ch_node;
	while (queBFS.Dequeue(ch_node))
	{
		ch_node.r_bone.Initialize(RequiredBones);
		bone_name = ref.GetBoneName(ch_node.r_bone.BoneIndex);
		if (m_mapmatch[i_match_col].end() != m_mapmatch[i_match_col].find(*bone_name.ToString()))
			m_channels[i_channel ++] = ch_node;

		bool is_a_target = (set_targets.end() != set_targets.find(body_name_w(ch_node.h_body)));
		if (is_a_target)
		{
			targets[i_target ++].h_body = ch_node.h_body;
		}
		TLinkedList<int32>* children_i = idx_tree[ch_node.r_bone.BoneIndex];
		if (NULL != children_i)
		{
			auto it_child = begin(*children_i);
			int32 id_child = *it_child;
			bone_name = ref.GetBoneName(id_child);
			Convert(pose[id_child], tm);
			if (getScale(*bone_name.ToString(), s_x, s_y, s_z))
			{
				tm.s.x *= s_x;
				tm.s.y *= s_y;
				tm.s.z *= s_z;
				check(m_mapmatch[0].end() != m_mapmatch[0].find(*bone_name.ToString())
					|| m_mapmatch[1].end() != m_mapmatch[1].find(*bone_name.ToString()));
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
				if (getScale(*bone_name.ToString(), s_x, s_y, s_z))
				{
					tm.s.x *= s_x;
					tm.s.y *= s_y;
					tm.s.z *= s_z;
					check(m_mapmatch[0].end() != m_mapmatch[0].find(*bone_name.ToString())
						|| m_mapmatch[1].end() != m_mapmatch[1].find(*bone_name.ToString()));
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

	auto targets_dst = const_cast<UAnimInstance_HIKDriver*>(m_animInst)->LockTarget(true);
	*targets_dst = targets;
	const_cast<UAnimInstance_HIKDriver*>(m_animInst)->UnLockTarget(targets_dst);

	struct FCompareChannel
	{
		FORCEINLINE bool operator()(const CHANNEL& A, const CHANNEL& B) const
		{
			return A.r_bone.BoneIndex < B.r_bone.BoneIndex;
		}
	};
	m_channels.Sort(FCompareChannel());
	initialize_kina(root_body);
	update_fk(root_body);
#if defined _DEBUG
	UE_LOG(LogHIK, Display, TEXT("Number of bones: %d"), n_bone);
	DBG_printOutSkeletalHierachy(root_body);
	DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	for (auto channel : m_channels)
	{
		check(ValidCHANNEL(channel));
	}
#endif
	return root_body;
}

inline void InitName2BodyMap(HBODY root, std::map<FString, HBODY>& name2body)
{
	auto lam_onEnter = [&name2body] (HBODY h_this)
						{
							name2body[FString(body_name_w(h_this))] = h_this;
						};
	auto lam_onLeave = [] (HBODY h_this)
						{
						};
	TraverseDFS(root, lam_onEnter, lam_onLeave);
}

inline bool InitName2BRMap(const FReferenceSkeleton& ref, const FBoneContainer& RequiredBones, std::map<FString, FBoneReference>& name2br)
{
	int32 n_bone = ref.GetNum();
	bool initialized = true;
	for (int32 i_bone = 0
		; i_bone < n_bone && initialized
		; i_bone++)
	{
		FName name_i = ref.GetBoneName(i_bone);
		FBoneReference r_bone(name_i);
		initialized = r_bone.Initialize(RequiredBones);
		name2br[name_i.ToString()] = r_bone;
	}
	return initialized;
}

void FAnimNode_FKRecordUT::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	LOGIK("FAnimNode_FKRecordUT::OnInitializeAnimInstance");
	Super::OnInitializeAnimInstance(InProxy, InAnimInstance);
	m_animInst = Cast<UAnimInstance_HIKDriver, UAnimInstance>(InAnimInstance);
	auto mesh = m_animInst->GetSkelMeshComponent();
	// prevent anim frame skipping optimization based on visibility etc
	mesh->bEnableUpdateRateOptimizations = false;
	// update animation even when mesh is not visible
	mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void FAnimNode_FKRecordUT::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	UnInitializeBoneReferences();
	LOGIK("FAnimNode_FKRecordUT::InitializeBoneReferences");
	bool conf_load = (VALID_HANDLE(m_animInst->getConfDrv())
					&& LoadConf(m_animInst->getConfDrv()));
	bool bvh_load = (conf_load && VALID_HANDLE(m_animInst->getBVH())
			 		&& VALID_HANDLE(m_driverBVH = create_tree_body_bvh(m_animInst->getBVH())));
	bool htr_clone = (bvh_load && clone_body(m_driverBVH, htr, &m_driverHTR));
	LOGIKVar(LogInfoBool, conf_load);
	LOGIKVar(LogInfoBool, bvh_load);
	LOGIKVar(LogInfoBool, htr_clone);

	bool ok = (conf_load
			&& bvh_load
	 		&& htr_clone);

	if (!ok)
		return;

	const FReferenceSkeleton& ref = RequiredBones.GetReferenceSkeleton();

	BITree idx_tree;
	ConstructBITree(ref, idx_tree);
	std::set<FString> channel_names_unrel;
	for (int i_match = 0; i_match < m_nMatches; i_match ++)
	{
		channel_names_unrel.insert(m_match[i_match][1]);
	}


	m_driverStub = InitializeChannel(ref, RequiredBones, idx_tree, 1);
#ifdef _DEBUG
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::InitializeBoneReferences"));
	DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	DBG_printOutSkeletalHierachy(m_driverStub);
	// check(DBG_verifyChannel(ref));
#endif
	ReleaseBITree(idx_tree);
	bool drv_created = VALID_HANDLE(m_driverStub);
	ok = drv_created;
	LOGIKVar(LogInfoBool, drv_created);

	if (ok)
	{
		m_moDriverBVH = create_tree_motion_node(m_driverBVH);
		m_moDriverHTR = create_tree_motion_node(m_driverHTR);
		m_moDriverStub = create_tree_motion_node(m_driverStub);
		bool mo_bvh_created = VALID_HANDLE(m_moDriverBVH);
		bool mo_htr_created = VALID_HANDLE(m_moDriverHTR);
		bool mo_drv_created = VALID_HANDLE(m_moDriverStub);
		bool cnn_bvh2htr = mo_bvh_created && mo_htr_created
						&& motion_sync_cnn_cross_w(m_moDriverBVH, m_moDriverHTR, FIRSTCHD, NULL, 0, NULL);
		bool cnn_htr2drv = mo_bvh_created && mo_htr_created && cnn_bvh2htr
						&& motion_sync_cnn_cross_w(m_moDriverHTR, m_moDriverStub, FIRSTCHD, m_match, m_nMatches, m_bvh2fbxWorld);
		ok =  (mo_bvh_created
			&& mo_htr_created
			&& mo_drv_created
			&& cnn_bvh2htr
			&& cnn_htr2drv);
		LOGIKVar(LogInfoBool, mo_bvh_created);
		LOGIKVar(LogInfoBool, mo_htr_created);
		LOGIKVar(LogInfoBool, mo_drv_created);
		LOGIKVar(LogInfoBool, cnn_bvh2htr);
		LOGIKVar(LogInfoBool, cnn_htr2drv);
	}

	if (!ok)
		UnInitializeBoneReferences();
}

void FAnimNode_FKRecordUT::UnInitializeBoneReferences()
{
	LOGIK("FAnimNode_FKRecordUT::UnInitializeBoneReferences");
	UnLoadConf();

	int32 n_bones = m_channels.Num();
	for (int32 i_bone = 0
		; i_bone < n_bones
		; i_bone++)
	{
		ResetCHANNEL(m_channels[i_bone]);
	}
	m_channels.SetNum(0);

	if (VALID_HANDLE(m_driverBVH))
		destroy_tree_body(m_driverBVH);
	m_driverBVH = H_INVALID;

	if (VALID_HANDLE(m_driverHTR))
		destroy_tree_body(m_driverHTR);
	m_driverHTR = H_INVALID;

	if (VALID_HANDLE(m_driverStub))
		destroy_tree_body(m_driverStub);
	m_driverStub = H_INVALID;

	if (VALID_HANDLE(m_moDriverBVH))
		destroy_tree_motion_node(m_moDriverBVH);
	m_moDriverBVH = H_INVALID;

	if (VALID_HANDLE(m_moDriverStub))
		destroy_tree_motion_node(m_moDriverStub);
	m_moDriverStub = H_INVALID;
}


// #if defined _DEBUG

void FAnimNode_FKRecordUT::DBG_LogTransform(const FString& name, const FTransform* tm) const
{
	UE_LOG(LogHIK, Display, TEXT("Item name: %s"), *name);
	if (tm)
	{
		UE_LOG(LogHIK, Display, TEXT("Transform Value:"));
		auto tt = tm->GetTranslation();
		UE_LOG(LogHIK, Display, TEXT("\t%.4f\t%.4f\t%.4f"), tt.X, tt.Y, tt.Z);
		auto r = tm->GetRotation();
		UE_LOG(LogHIK, Display, TEXT("\t%.4f\t%.4f\t%.4f\t%.4f"),  r.W, r.X, r.Y, r.Z);
		auto s = tm->GetScale3D();
		UE_LOG(LogHIK, Display, TEXT("\t%.4f\t%.4f\t%.4f"), s.X, s.Y, s.Z);

		// UE_LOG(LogHIK, Display, TEXT("Matrix Value:"));
		// FMatrix tm_m_t = tm->ToMatrixWithScale();
		// for (int i_r = 0; i_r < 4; i_r ++)
		// {
		// 	UE_LOG(LogHIK, Display, TEXT("%.4f\t%.4f\t%.4f\t%.4f")
		// 		, tm_m_t.M[0][i_r], tm_m_t.M[1][i_r], tm_m_t.M[2][i_r], tm_m_t.M[3][i_r]);

		// }
	}
}

void FAnimNode_FKRecordUT::DBG_LogTransform(const FString& name, const _TRANSFORM* tm) const
{
	UE_LOG(LogHIK, Display, TEXT("Item name: %s"), *name);
	if (tm)
	{
		UE_LOG(LogHIK, Display, TEXT("Transform Value:"));
		auto tt = tm->tt;
		UE_LOG(LogHIK, Display, TEXT("\t%.4f\t%.4f\t%.4f"), tt.x, tt.y, tt.z);
		auto r = tm->r;
		UE_LOG(LogHIK, Display, TEXT("\t%.4f\t%.4f\t%.4f\t%.4f"), r.w, r.x, r.y, r.z);
		auto s = tm->s;
		UE_LOG(LogHIK, Display, TEXT("\t%.4f\t%.4f\t%.4f"), s.x, s.y, s.z);
	}
}

void FAnimNode_FKRecordUT::DBG_GetComponentSpaceTransform(const FAnimNode_FKRecordUT::CHANNEL& channel, FTransform& tm_l2compo, const FReferenceSkeleton& ref_sk) const
{
	FBoneReference r_bone = channel.r_bone;
	int32 idx_bone = r_bone.BoneIndex;
	auto pose_local = ref_sk.GetRawRefBonePose();
	tm_l2compo = FTransform::Identity;
	do
	{
		FTransform pose_i = pose_local[idx_bone];
		float bs_x = 1.0;
		float bs_y = 1.0;
		float bs_z = 1.0;
		if (getScale(*ref_sk.GetBoneName(idx_bone).ToString(), bs_x, bs_y, bs_z))
		{
			auto bs_0 = pose_i.GetScale3D();
			bs_0.X *= bs_x;
			bs_0.Y *= bs_y;
			bs_0.Z *= bs_z;
			pose_i.SetScale3D(bs_0);
		}
		tm_l2compo = tm_l2compo * pose_i;
	} while ((idx_bone = ref_sk.GetParentIndex(idx_bone)) > -1);

	// DBG_LogTransform(r_bone.BoneName.ToString(), &tm_l2compo);
	// Convert(tm_l2compo, tm);
	// float epsilon = 1e-6f;
	// float e_x = tm.s.x - 1;
	// float e_y = tm.s.y - 1;
	// float e_z = tm.s.z - 1;
	// check( e_x < epsilon && e_x > -epsilon
	// 	&& e_y < epsilon && e_y > -epsilon
	// 	&& e_z < epsilon && e_z > -epsilon);
}

void FAnimNode_FKRecordUT::DBG_GetComponentSpaceTransform2(const FAnimNode_FKRecordUT::CHANNEL& channel, _TRANSFORM& tm, const FReferenceSkeleton& ref_sk) const
{
	HBODY h_body = channel.h_body;
	get_body_transform_l2w(h_body, &tm);
}

bool FAnimNode_FKRecordUT::DBG_EqualTransform(const FTransform& tm_1, const _TRANSFORM& tm_2) const
{
	//FMatrix m1 = tm_1.ToMatrixWithScale();
	FTransform tm_2_prime;
	tm_2_prime.SetLocation(FVector(tm_2.tt.x, tm_2.tt.y, tm_2.tt.z));
	tm_2_prime.SetRotation(FQuat(tm_2.r.x, tm_2.r.y, tm_2.r.z, tm_2.r.w));
	tm_2_prime.SetScale3D(FVector(tm_2.s.x, tm_2.s.y, tm_2.s.z));
	return tm_1.Equals(tm_2_prime, 0.05f);
}

bool FAnimNode_FKRecordUT::DBG_verifyChannel(const FReferenceSkeleton& ref_sk) const
{
	bool verified = true;
	auto n_channels = m_channels.Num();
	const wchar_t* res[2] = {L"NEqual", L"Equal"};
	for (int32 i_channel = 0
		; i_channel < n_channels
		&& verified
		; i_channel++)
	{
		FTransform tm_unrel;
		_TRANSFORM tm_arti;
		DBG_GetComponentSpaceTransform(m_channels[i_channel], tm_unrel, ref_sk);
		DBG_GetComponentSpaceTransform2(m_channels[i_channel], tm_arti, ref_sk);
		verified = DBG_EqualTransform(tm_unrel, tm_arti);
		FName name = ref_sk.GetBoneName(m_channels[i_channel].r_bone.BoneIndex);
		DBG_LogTransform(name.ToString(), &tm_unrel);
		DBG_LogTransform(name.ToString(), &tm_arti);
		int i_res = verified ? 1 : 0;
		UE_LOG(LogHIK, Display, TEXT("%s"), res[i_res]);
	}
	FName name = ref_sk.GetBoneName(0);
	auto pose_local = ref_sk.GetRawRefBonePose();
	FTransform tm_l2compo = pose_local[0];
	DBG_LogTransform(name.ToString(), &tm_l2compo);
	return verified;
}

void FAnimNode_FKRecordUT::DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
{
	auto name = ref.GetBoneName(id_node);
	FString item;
	for (int i_ident = 0; i_ident < identation; i_ident++)
		item += TEXT("\t");
	item += name.ToString();
	UE_LOG(LogHIK, Display, TEXT("%s"), *item);
	const BIChildren* children = idx_tree[id_node];
	for (auto it = begin(*children)
		; it
		; it ++)
	{
		DBG_printOutSkeletalHierachy_recur(ref, idx_tree, *it, identation + 1);
	}
}

void FAnimNode_FKRecordUT::DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
{
	UE_LOG(LogHIK, Display, TEXT("Skeletal structure:"));
	auto & raw_tms = ref.GetRawRefBonePose();
	auto & ref_tms = ref.GetRefBonePose();
	auto n_tms = std::min(raw_tms.Num(), ref_tms.Num());
	bool verified = true;
	for (int32 i_tm = 0; i_tm < n_tms && verified; i_tm++)
	{
		const FTransform& raw_tm = raw_tms[i_tm];
		const FTransform& ref_tm = ref_tms[i_tm];
		verified = raw_tm.Equals(ref_tm, c_epsilon);
	}
	check(verified);

	DBG_printOutSkeletalHierachy_recur(ref, idx_tree, id_node, identation);
}

void FAnimNode_FKRecordUT::DBG_printOutSkeletalHierachy(HBODY root_body) const
{
	int n_indent = 1;
	auto lam_onEnter = [&n_indent] (HBODY h_this)
						{
							printArtName(body_name_w(h_this), n_indent ++);
							log_body_node(h_this);
						};
	auto lam_onLeave = [&n_indent] (HBODY h_this)
						{
							n_indent --;

						};
	UE_LOG(LogHIK, Display, TEXT("Articulated Body structure:"));
	TraverseDFS(root_body, lam_onEnter, lam_onLeave);
}

void FAnimNode_FKRecordUT::DBG_VisTransform(const UWorld* world, const FTransform& tm) const
{
	const float axis_len = 10; // cm

	const FVector4 ori(0, 0, 0, 1);

	const FVector4 axis_ends[] = {
		FVector4(axis_len,	0,	0,	1),
		FVector4(0,	axis_len,	0,	1),
		FVector4(0,	0,	axis_len,	1),
	};
	const FColor axis_color[] = {
		FColor::Red,
		FColor::Green,
		FColor::Blue
	};
	FVector4 ori_w = tm.TransformFVector4(ori);
	const int n_axis = sizeof(axis_ends) / sizeof(FVector4);
	for (int i_end = 0; i_end < n_axis; i_end ++)
	{
		FVector4 end_w = tm.TransformFVector4(axis_ends[i_end]);
		DrawDebugLine(world
					, ori_w
					, end_w
					, axis_color[i_end]
					, false // bPersistentLines =
					, -1.f  // LifeTime =
					, 0		//uint8 DepthPriority =
					, 1);
	}
}

void FAnimNode_FKRecordUT::DBG_VisTransform(const UWorld* world, const FTransform& c2w, HBODY hBody, int i_match_col) const
{

	auto lam_onEnter = [this, c2w, world, i_match_col] (HBODY h_this)
						{
							bool is_a_channel = (m_mapmatch[i_match_col].end() != m_mapmatch[i_match_col].find(body_name_w(h_this)));
							if (is_a_channel)
							{
								_TRANSFORM l2c_body;
								get_body_transform_l2w(h_this, &l2c_body);
								FTransform l2c_unrel;
								Convert(l2c_body, l2c_unrel);
								FTransform l2w = l2c_unrel * c2w;
								DBG_VisTransform(world, l2w);
							}


						};
	auto lam_onLeave = [] (HBODY h_this)
						{

						};
	TraverseDFS(hBody, lam_onEnter, lam_onLeave);
}

void FAnimNode_FKRecordUT::DBG_VisTargetTransform(const UWorld* world, const TArray<UAnimInstance_HIK::Target>* targets) const
{
	for (auto target : (*targets))
	{
		DBG_VisTransform(world, target.tm_l2w);
	}
}


// #endif
