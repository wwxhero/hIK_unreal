// Copyright(c) Mathew Wang 2021

//#include "stdafx.h"
#include "AnimNode_FKRecordUT.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Runtime/AnimationCore/Public/TwoBoneIK.h"

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
	check(H_INVALID != root);
	typedef struct _EDGE
	{
		HBODY body_this;
		HBODY body_child;
	} EDGE;
	std::stack<EDGE> stkDFS;
	stkDFS.push({root, get_first_child(root)});
	//printArtName(body_name_w(root), 0);
	OnEnterBody(root);
	while (!stkDFS.empty())
	{
		EDGE &edge = stkDFS.top();
		int n_indent = stkDFS.size();
		if (H_INVALID == edge.body_child)
		{
			stkDFS.pop();
			OnLeaveBody(edge.body_this);
		}
		else
		{
			//printArtName(body_name_w(edge.body_child), n_indent);
			OnEnterBody(edge.body_child);
			HBODY body_grandchild = get_first_child(edge.body_child);
			HBODY body_nextchild = get_next_sibling(edge.body_child);
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
	//FCompactPoseBoneIndex boneCompactIdx = BoneRef.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
	////UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::EvaluateSkeletalControl_AnyThread"));
	//USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	////USceneComponent* ParentComponent = SkelComp->GetAttachParent();
	//FTransform l2enti = Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
	//static float delta_deg = 0;
	//const float c_deg2rad = PI / 180;
	//FVector axis(0, 1, 0);
	//float angleRad = delta_deg * c_deg2rad;
	//FQuat rot_enti(axis, angleRad);
	//FTransform t_enti(rot_enti);
	//FTransform l2enti_prime = t_enti * l2enti;
	//FBoneTransform bone_tran(boneCompactIdx, l2enti_prime);
	//OutBoneTransforms.Push(bone_tran);
	//delta_deg = ik_test(delta_deg);

	// FString result[] = {"failed", "successful"};
	// const FBoneContainer& requiredBones = Output.Pose.GetPose().GetBoneContainer();
	// for (auto it_bone = begin(m_bones)
	// 	; it_bone
	// 	; it_bone ++)
	// {
	// 	FCompactPoseBoneIndex boneCompactIdx = it_bone->BoneRef.GetCompactPoseIndex(requiredBones);
	// 	FTransform l2enti = Output.Pose.GetComponentSpaceTransform(boneCompactIdx);
	// 	_TRANSFORM tm; // = DBG_GetComponentSpaceTransform2(boneCompactIdx);
	// 	//_TRANSFORM tm;
	// 	//get_joint_transform_l2w(it_bone->ArtiBody, &tm);
	// 	int i_result = ( DBG_EqualTransform(l2enti, tm) ? 1 : 0 );
	// 	UE_LOG(LogHIK, Display, TEXT("confirm %s %s"), it_bone->BoneRef.GetBoneName(), result[i_result]);
	// }
}


bool FAnimNode_FKRecordUT::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer & RequiredBones)
{
	//UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::IsValidToEvaluate"));
	return true;
}

void FAnimNode_FKRecordUT::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	UnInitializeBoneReferences();
	const FReferenceSkeleton& ref = RequiredBones.GetReferenceSkeleton();

	int n_bone = ref.GetNum();
	UE_LOG(LogHIK, Display, TEXT("Number of bones: %d"), n_bone);
	m_aritiBodies.SetNum(n_bone, false);
	TArray<Children*> node2children;
	node2children.SetNum(n_bone, false);
	for (int32 i_bone = n_bone - 1
		; i_bone > 0
		; i_bone--)
	{
		int32 i_parent = ref.GetParentIndex(i_bone);
		ensure(i_parent > -1);
		auto new_child = new Children(i_bone);
		new_child->LinkHead(node2children[i_parent]);
	}

	TQueue<BONE_NODE> queBFS;
	const auto pose = ref.GetRawRefBonePose();
	_TRANSFORM t;
	GetBoneLocalTranform(pose[0], t);

	FName bone_name = ref.GetBoneName(0);
	m_aritiBodies[0] =
		{
			FBoneReference(bone_name),
			create_arti_body_f
				(
					  *bone_name.ToString()
					, &t
				)
		};
	BONE_NODE bone_node = m_aritiBodies[0];
	queBFS.Enqueue(bone_node);

	while (queBFS.Dequeue(bone_node))
	{
		bone_node.bone.Initialize(RequiredBones);
		TLinkedList<int32>* children_i = node2children[bone_node.bone.BoneIndex];
		if (NULL != children_i)
		{
			auto it_child = begin(*children_i);
			int32 id_child = *it_child;
			bone_name = ref.GetBoneName(id_child);
			GetBoneLocalTranform(pose[id_child], t);
			BONE_NODE bone_node_child =
						{
							FBoneReference(bone_name),
							create_arti_body_f
								(
									  *bone_name.ToString()
									, &t
								)
						};
			m_aritiBodies[id_child] = bone_node_child;
			queBFS.Enqueue(bone_node_child);
			cnn_arti_body(bone_node.h_body, bone_node_child.h_body, CNN::FIRSTCHD);
			for (it_child ++
				; it_child
				; it_child ++)
			{
				id_child = *it_child;
				bone_name = ref.GetBoneName(id_child);
				GetBoneLocalTranform(pose[id_child], t);
				BONE_NODE bone_node_next_child =
						{
							FBoneReference(bone_name),
							create_arti_body_f
								(
									  *bone_name.ToString()
									, &t
								)
						};
				m_aritiBodies[id_child] = bone_node_next_child;
				cnn_arti_body(bone_node_child.h_body, bone_node_next_child.h_body, CNN::NEXTSIB);
				bone_node_child = bone_node_next_child;
				queBFS.Enqueue(bone_node_child);
			}
		}
	}

#if defined _DEBUG
	DBG_printOutSkeletalHierachy(m_aritiBodies[0].h_body);
	DBG_printOutSkeletalHierachy_recur(ref, node2children, 0, 0);
	for (int i_bone = 0
		; i_bone < n_bone
		; i_bone ++)
	{
		check(ValidBONE_NODE(m_aritiBodies[i_bone]));
	}
#endif

	for (int32 i_bone = 0
		; i_bone < n_bone
		; i_bone ++)
	{
		TLinkedList<int32>* children_i = node2children[i_bone];
		TLinkedList<int32>* it_m = NULL;
		for (auto it = children_i
			; NULL != it
			; it = it->Next())
		{
			delete it_m;
			it_m = it;
		}
		delete it_m;
	}
}

void FAnimNode_FKRecordUT::UnInitializeBoneReferences()
{
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_FKRecordUT::UnInitializeBoneReferences"));

	int32 n_bones = m_aritiBodies.Num();
	for (int32 i_bone = 0
		; i_bone < n_bones
		; i_bone++)
	{
		ResetBONE_NODE(m_aritiBodies[i_bone]);
	}

}

#if defined _DEBUG

void FAnimNode_FKRecordUT::DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const TArray<Children*>& node2children, int32 id_node, int identation)
{
	auto name = ref.GetBoneName(id_node);
	FString item;
	for (int i_ident = 0; i_ident < identation; i_ident++)
		item += TEXT("\t");
	item += name.ToString();
	UE_LOG(LogTemp, Display, TEXT("%s"), *item);
	const Children* children = node2children[id_node];
	for (auto it = begin(*children)
		; it
		; it ++)
	{
		DBG_printOutSkeletalHierachy_recur(ref, node2children, *it, identation + 1);
	}
}

void FAnimNode_FKRecordUT::DBG_printOutSkeletalHierachy(HBODY root_body)
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
	TraverseDFS(root_body, lam_onEnter, lam_onLeave);
}

#endif
