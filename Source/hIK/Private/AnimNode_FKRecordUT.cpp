// Copyright(c) Mathew Wang 2021

#include "AnimNode_FKRecordUT.h"
#include <map>
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Runtime/AnimationCore/Public/TwoBoneIK.h"
#include "bvh.h"
#include "fk_joint.h"

static const wchar_t* s_match[][2] = {
	// makehuman,		aritbody_bvh
	{L"root", 			L"Hips"},
	{L"pelvis_L", 		L"LHipJoint"},
	{L"pelvis_R", 		L"RHipJoint"},
	{L"spine05", 		L"LowerBack"},
	{L"upperleg02_L", 	L"LeftUpLeg"},
	{L"upperleg02_R", 	L"RightUpLeg"},
	{L"spine01", 		L"Spine"},
	{L"lowerleg01_L",	L"LeftLeg"},
	{L"lowerleg01_R",	L"RightLeg"},
	{L"neck01", 		L"Neck"},
	{L"clavicle_L",		L"LeftShoulder"},
	{L"clavicle_R", 	L"RightShoulder"},
	{L"foot_L", 		L"LeftFoot"},
	{L"foot_R", 		L"RightFoot"},
	{L"neck02", 		L"Neck1"},
	{L"upperarm02_L", 	L"LeftArm"},
	{L"upperarm02_R", 	L"RightArm"},
	{L"toe1-1_L",		L"LeftToeBase"},
	{L"toe1-1_R",		L"RightToeBase"},
	{L"head",			L"Head"},
	{L"lowerarm01_L", 	L"LeftForeArm"},
	{L"lowerarm01_R", 	L"RightForeArm"},
	{L"wrist_L",		L"LeftHand"},
	{L"wrist_R",		L"RightHand"},
};

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

void FAnimNode_FKRecordUT::UpdateInternal(const FAnimationUpdateContext & Context)
{
	// Mark trace data as stale
	Super::UpdateInternal(Context);
	//UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::UpdateInternal"));
}

void FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_FK_UT_Eval);
	// GetEvaluateGraphExposedInputs().Execute(Context);
// #if defined _DEBUG
	check(OutBoneTransforms.Num() == 0);
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread: %d %d"), m_animInst, m_animInst->I_Frame_);
	if (!(m_animInst->I_Frame_ > 0))
		return;
	const bool rotate_on_entity = false;

	const FBoneContainer& requiredBones = Output.Pose.GetPose().GetBoneContainer();
	const FReferenceSkeleton& refSkele = requiredBones.GetReferenceSkeleton();
	USkeletalMeshComponent* skeleton = Output.AnimInstanceProxy->GetSkelMeshComponent();
	AActor* owner = skeleton->GetOwner();
	if (owner)
	{
		int n_channels = m_channels.Num();

		OutBoneTransforms.SetNum(n_channels, false);
		pose_body_atom(m_animInst->m_hBVH, m_artiRoot, m_animInst->I_Frame_);
		for (int i_channel = 0; i_channel < n_channels; i_channel ++)
		{
			FCompactPoseBoneIndex boneCompactIdx = m_channels[i_channel].r_bone.GetCompactPoseIndex(requiredBones);
			const FTransform& l2w = Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
			_TRANSFORM delta_arti;
			get_joint_transform(m_channels[i_channel].h_body, &delta_arti);
			FTransform delta_bone;
			Convert(delta_arti, delta_bone);
			FTransform l2w_prime = (delta_bone * l2w);
			FBoneTransform tm_bone(boneCompactIdx, l2w_prime);
			OutBoneTransforms[i_channel] = tm_bone;
		}
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

		// static float delta_deg = 1;
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


bool FAnimNode_FKRecordUT::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer & RequiredBones)
{
	//UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::IsValidToEvaluate"));
	return true;
}


void FAnimNode_FKRecordUT::InitializeChannel_BITree(const FReferenceSkeleton& ref, const FBoneContainer& RequiredBones, const BITree& idx_tree)
{
	int n_bone = ref.GetNum();
	m_channels.SetNum(n_bone, false);

	TQueue<CHANNEL*> queBFS;
	const auto pose = ref.GetRawRefBonePose();
	_TRANSFORM t;
	Convert(pose[0], t);

	FName bone_name = ref.GetBoneName(0);
	m_channels[0] =
		{
			FBoneReference(bone_name),
			create_tree_body_node_w
				(
					  *bone_name.ToString()
					, &t
				)
		};
	CHANNEL* channel_node = &m_channels[0];
	queBFS.Enqueue(channel_node);

	while (queBFS.Dequeue(channel_node))
	{
		channel_node->r_bone.Initialize(RequiredBones);
		TLinkedList<int32>* children_i = idx_tree[channel_node->r_bone.BoneIndex];
		if (NULL != children_i)
		{
			auto it_child = begin(*children_i);
			int32 id_child = *it_child;
			bone_name = ref.GetBoneName(id_child);
			Convert(pose[id_child], t);
			m_channels[id_child] =
						{
							FBoneReference(bone_name),
							create_tree_body_node_w
								(
									  *bone_name.ToString()
									, &t
								)
						};
			CHANNEL* channel_node_child = &m_channels[id_child];
			queBFS.Enqueue(channel_node_child);
			cnn_arti_body(channel_node->h_body, channel_node_child->h_body, CNN::FIRSTCHD);
			for (it_child ++
				; it_child
				; it_child ++)
			{
				id_child = *it_child;
				bone_name = ref.GetBoneName(id_child);
				Convert(pose[id_child], t);
				m_channels[id_child] =
						{
							FBoneReference(bone_name),
							create_tree_body_node_w
								(
									  *bone_name.ToString()
									, &t
								)
						};
				CHANNEL* channel_node_next_child = &m_channels[id_child];
				cnn_arti_body(channel_node_child->h_body, channel_node_next_child->h_body, CNN::NEXTSIB);
				channel_node_child = channel_node_next_child;
				queBFS.Enqueue(channel_node_child);
			}
		}
	}
#if defined _DEBUG

	UE_LOG(LogHIK, Display, TEXT("Number of bones: %d"), n_bone);
	DBG_printOutSkeletalHierachy(m_channels[0].h_body);
	DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	for (int i_bone = 0
		; i_bone < n_bone
		; i_bone++)
	{
		check(ValidCHANNEL(m_channels[i_bone]));
	}
#endif
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
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::OnInitializeAnimInstance"));
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

	if (!VALID_HANDLE(m_animInst->m_hBVH))
		return;

	const FReferenceSkeleton& ref = RequiredBones.GetReferenceSkeleton();

	std::map<FString, HBODY> name2body;
	std::map<FString, FBoneReference> name2br;

	HBODY root = create_tree_body_bvh(m_animInst->m_hBVH);
#if defined _DEBUG
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::InitializeBoneReferences"));
	DBG_printOutSkeletalHierachy(root);
	BITree idx_tree;
	ConstructBITree(ref, idx_tree);
	InitializeChannel_BITree(ref, RequiredBones, idx_tree);
	DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	ReleaseBITree(idx_tree);
#endif
	InitName2BodyMap(root, name2body);
	bool channel_connected = InitName2BRMap(ref, RequiredBones, name2br);

	const int n_map = (sizeof(s_match) / (2 * sizeof(const wchar_t*)));
	m_channels.SetNum(n_map);


	for (int i_map = 0
		; i_map < n_map && channel_connected
		; i_map ++)
	{
		auto it_br_i = name2br.find(FString(s_match[i_map][0]));
		auto it_body_i = name2body.find(FString(s_match[i_map][1]));
		channel_connected = (name2br.end() != it_br_i
							&& name2body.end() != it_body_i);
		if (channel_connected)
		{
			m_channels[i_map].h_body = it_body_i->second;
			m_channels[i_map].r_bone = it_br_i->second;
		}
#if defined _DEBUG
		else
		{
			UE_LOG(LogHIK, Error, TEXT("DISCONNECTED ON: %s, %s"), s_match[i_map][0], s_match[i_map][1]);
		}
#endif
	}

	check(channel_connected);

	if (!channel_connected)
	{
		m_channels.SetNum(0);
		destroy_tree_body(root);
	}
	else
	{
		struct FCompareChannel
		{
			FORCEINLINE bool operator()(const CHANNEL& A, const CHANNEL& B) const
			{
				return A.r_bone.BoneIndex < B.r_bone.BoneIndex;
			}
		};
		m_channels.Sort(FCompareChannel());
		m_artiRoot = root;
	}
}

void FAnimNode_FKRecordUT::UnInitializeBoneReferences()
{
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::UnInitializeBoneReferences"));

	int32 n_bones = m_channels.Num();
	for (int32 i_bone = 0
		; i_bone < n_bones
		; i_bone++)
	{
		ResetCHANNEL(m_channels[i_bone]);
	}
	m_channels.SetNum(0);
	if (VALID_HANDLE(m_artiRoot))
		destroy_tree_body(m_artiRoot);
	m_artiRoot = H_INVALID;
}


#if defined _DEBUG

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

		UE_LOG(LogHIK, Display, TEXT("Matrix Value:"));
		FMatrix tm_m_t = tm->ToMatrixWithScale();
		for (int i_r = 0; i_r < 4; i_r ++)
		{
			UE_LOG(LogHIK, Display, TEXT("%.4f\t%.4f\t%.4f\t%.4f")
				, tm_m_t.M[0][i_r], tm_m_t.M[1][i_r], tm_m_t.M[2][i_r], tm_m_t.M[3][i_r]);

		}
	}
}

void FAnimNode_FKRecordUT::DBG_GetComponentSpaceTransform(const FAnimNode_FKRecordUT::CHANNEL& channel, _TRANSFORM& tm, const FReferenceSkeleton& ref_sk) const
{
	FBoneReference r_bone = channel.r_bone;
	int32 idx_bone = r_bone.BoneIndex;
	auto pose_local = ref_sk.GetRawRefBonePose();
	FTransform tm_l2compo = pose_local[idx_bone];
	while ((idx_bone = ref_sk.GetParentIndex(idx_bone)) > -1)
	{
		tm_l2compo = tm_l2compo * pose_local[idx_bone];
	}
	Convert(tm_l2compo, tm);
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
	return tm_1.Equals(tm_2_prime, 1e-4f);
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
	DBG_printOutSkeletalHierachy_recur(ref, idx_tree, id_node, identation);
}

void FAnimNode_FKRecordUT::DBG_printOutSkeletalHierachy(HBODY root_body) const
{
	int n_indent = 1;
	auto lam_onEnter = [&n_indent] (HBODY h_this)
						{
							printArtName(body_name_w(h_this), n_indent ++);
						};
	auto lam_onLeave = [&n_indent] (HBODY h_this)
						{
							n_indent --;

						};
	UE_LOG(LogHIK, Display, TEXT("Articulated Body structure:"));
	TraverseDFS(root_body, lam_onEnter, lam_onLeave);
}

#endif
