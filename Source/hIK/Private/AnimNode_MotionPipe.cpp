// Copyright(c) Mathew Wang 2021
#include "AnimNode_MotionPipe.h"
#include <stack>
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Misc/Paths.h"
#include "fk_joint.h"
#include "motion_pipeline.h"
#include "DrawDebugHelpers.h"
#include "ik_logger_unreal.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "transform_helper.h"

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

const int FAnimNode_MotionPipe::c_idxSim = 0;
const int FAnimNode_MotionPipe::c_idxFBX = 1;

FAnimNode_MotionPipe::FAnimNode_MotionPipe()
	: c_animInst(NULL)
	, m_mopipe(NULL)
{
}

FAnimNode_MotionPipe::~FAnimNode_MotionPipe()
{
	UnCacheBones_AnyThread();
}

void FAnimNode_MotionPipe::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread);
	GetEvaluateGraphExposedInputs().Execute(Context);
	BasePose.Update(Context);
}

HBODY FAnimNode_MotionPipe::InitializeChannelFBX_AnyThread(const FReferenceSkeleton& ref
														, const FBoneContainer& RequiredBones
														, const FTransform& skelcomp_l2w
														, const BITree& idx_tree
														, const std::set<FString>& namesOnPair
														, const std::map<FString, FVector>& name2scale)
{
	m_C0toW = skelcomp_l2w;
	m_WtoC0 = skelcomp_l2w.Inverse();

	std::size_t n_bone = ref.GetRawBoneNum();
	m_channelsFBX.SetNum(namesOnPair.size(), false);

	TQueue<CHANNEL> queBFS;
	const auto pose = ref.GetRawRefBonePose();
	_TRANSFORM tm;
	Convert(pose[0], tm);
	FName bone_name = ref.GetBoneName(0);

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
					  *(bone_name.ToString().ToLower())
					, &tm
				)
		};

#if defined _DEBUG
	DBG_LogTransform(*bone_name.ToString(), &tm);
#endif

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
			AppScale(bone_name, tm.s);
			CHANNEL ch_node_child =
						{
							FBoneReference(bone_name),
							create_fbx_body_node_w
								(
									  *(bone_name.ToString().ToLower())
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
									  *(bone_name.ToString().ToLower())
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
// #if defined _DEBUG
	UE_LOG(LogHIK, Display, TEXT("Number of bones: %d"), n_bone);
	printOutSkeletalHierachy(root_body);
	printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	for (auto channel : m_channelsFBX)
	{
		check(ValidCHANNEL(channel));
	}
// #endif
	return root_body;
}

HBODY FAnimNode_MotionPipe::ProcInitBody_FBX(void* param
											, const wchar_t* filePath
											, const wchar_t* namesOnPair[]
											, int n_pairs
											, const B_Scale scales[]
											, int n_scales
											, const wchar_t* nameTargets[]
											, int n_targets)
{
	ParamFBXCreator* param_ctr = reinterpret_cast<ParamFBXCreator*>(param);
	FAnimNode_MotionPipe* pThis = param_ctr->pThis;
	const FReferenceSkeleton& ref = param_ctr->boneRef;
	const FBoneContainer& RequiredBones = param_ctr->RequiredBones;
	const FTransform& skeleTM_l2w = param_ctr->skelecom_l2w;
	const BITree& idx_tree = param_ctr->idx_tree;

	std::set<FString> namesOnPair_fbx;
	for (int i_pair = 0; i_pair < n_pairs; i_pair ++)
	{
		namesOnPair_fbx.insert(namesOnPair[i_pair]);
	}

	std::map<FString, FVector> name2scale;
	for (int i_scale = 0; i_scale < n_scales; i_scale ++)
	{
		const B_Scale& b_scale_i = scales[i_scale];
		const wchar_t* name_i = b_scale_i.bone_name;
		FVector scale_i(b_scale_i.scaleX, b_scale_i.scaleY, b_scale_i.scaleZ);
		name2scale[FString(name_i)] = scale_i;
	}

	HBODY body_fbx = pThis->InitializeChannelFBX_AnyThread(ref
												, RequiredBones
												, skeleTM_l2w
												, idx_tree
												, namesOnPair_fbx
												, name2scale);
#ifdef _DEBUG
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_MotionPipe::CacheBones_AnyThread"));
	pThis->DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	pThis->DBG_printOutSkeletalHierachy(body_fbx);
#endif
	// end of initialization
	bool fbx_created = VALID_HANDLE(body_fbx);

	if (fbx_created
		&& n_targets > 0)
	{
		std::set<FString> targets_name_fbx;
		for (int i_target = 0; i_target < n_targets; i_target ++)
			targets_name_fbx.insert(nameTargets[i_target]);
		pThis->InitializeTargets_AnyThread(body_fbx, skeleTM_l2w, targets_name_fbx);
	}

	return body_fbx;
}

void FAnimNode_MotionPipe::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);
	FAnimNode_Base::CacheBones_AnyThread(Context);
	// auto proxy = Cast<FAnimInstanceProxy_MotionPipe, FAnimInstanceProxy>(Context.AnimInstanceProxy);
	auto proxy = static_cast<FAnimInstanceProxy_MotionPipe*>(Context.AnimInstanceProxy);
#if defined _DEBUG
	check(proxy->ValidPtr());
#endif
	UnCacheBones_AnyThread();

	auto RequiredBones = proxy->GetRequiredBones();

	// Initialize the native FBX bodies
	const FReferenceSkeleton& ref = RequiredBones.GetReferenceSkeleton();
	BITree idx_tree;
	ConstructBITree(ref, idx_tree);

	FTransform skeleTM_l2w;
	if (c_inCompSpace)
		skeleTM_l2w = FTransform::Identity;
	else
		skeleTM_l2w = proxy->GetSkelMeshCompLocalToWorld();

	ParamFBXCreator paramFBXCreator = {
						  this
						, ref
						, RequiredBones
						, skeleTM_l2w
						, idx_tree
					};
	FuncBodyInit callbacks[] = { NULL, FAnimNode_MotionPipe::ProcInitBody_FBX };
	if (!load_mopipe(&m_mopipe
					, *c_animInst->GetFileConfPath()
					, callbacks
					, &paramFBXCreator))
		UnCacheBones_AnyThread();
	else
	{
		proxy->PushUpdateNFrames(m_mopipe->n_frames);
		TArray<Target> targets;
		for (auto target_i : m_targets)
			targets.Add(target_i);
		proxy->PushUpdateTargets(targets); // this target is not for update, but for initial binding
	}

	ReleaseBITree(idx_tree);
	BasePose.CacheBones(Context);
}

void FAnimNode_MotionPipe::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);

	if (NULL == m_mopipe)
		return;

	BasePose.Evaluate(Output);

	BoneTransforms.Reset(BoneTransforms.Num());
	EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);

	for (auto b_tm : BoneTransforms)
	{
		Output.Pose[b_tm.BoneIndex] = b_tm.Transform;
	}

}



void FAnimNode_MotionPipe::UnCacheBones_AnyThread()
{
	LOGIK("FAnimNode_MotionPipe::UnCacheBones_AnyThread");

	int32 n_bones = m_channelsFBX.Num();
	for (int32 i_bone = 0
		; i_bone < n_bones
		; i_bone++)
	{
		ResetCHANNEL(m_channelsFBX[i_bone]);
	}
	m_channelsFBX.SetNum(0);

	unload_mopipe(m_mopipe);
	m_mopipe = NULL;
}



void FAnimNode_MotionPipe::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	LOGIK("FAnimNode_MotionPipe::OnInitializeAnimInstance");
	Super::OnInitializeAnimInstance(InProxy, InAnimInstance);
	c_animInst = Cast<UAnimInstance_MotionPipe, UAnimInstance>(InAnimInstance);
	auto mesh = c_animInst->GetSkelMeshComponent();
	// prevent anim frame skipping optimization based on visibility etc
	mesh->bEnableUpdateRateOptimizations = false;
	// update animation even when mesh is not visible
	mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void FAnimNode_MotionPipe::InitializeTargets_AnyThread(HBODY h_bodyFbx
												, const FTransform& skelcomp_l2w
												, const std::set<FString> &targets_name)
{
	int32 n_targets = targets_name.size();
	bool exist_target = (0 < n_targets);
	if (!exist_target)
		return;

	TArray<Target_Internal> targets;
	targets.Reset(n_targets);

	const FTransform& c2w = skelcomp_l2w;
	auto onEnterBody = [this, &targets, &targets_name, &c2w] (HBODY h_this)
		{
			FString name_this(body_name_w(h_this));
			bool is_target = (targets_name.end() != targets_name.find(name_this));
			if (is_target)
			{
				_TRANSFORM tm_l2c;
				get_body_transform_LtoC0(h_this, &tm_l2c);
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

	TraverseDFS(h_bodyFbx, onEnterBody, onLeaveBody);

	targets.Sort(FCompareTarget());

	m_targets.SetNum(n_targets);
	for (int i_target = 0; i_target < n_targets; i_target ++)
	{
		m_targets[i_target] = targets[i_target];
	}
}

void FAnimNode_MotionPipe::printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
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
		printOutSkeletalHierachy_recur(ref, idx_tree, *it, identation + 1);
	}
}

void FAnimNode_MotionPipe::printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
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

	printOutSkeletalHierachy_recur(ref, idx_tree, id_node, identation);
}

void FAnimNode_MotionPipe::printOutSkeletalHierachy(HBODY root_body) const
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

#if defined _DEBUG

void FAnimNode_MotionPipe::DBG_LogTransform(const FString& name, const FTransform* tm) const
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

void FAnimNode_MotionPipe::DBG_LogTransform(const FString& name, const _TRANSFORM* tm) const
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

bool FAnimNode_MotionPipe::DBG_EqualTransform(const FTransform& tm_1, const _TRANSFORM& tm_2) const
{
	//FMatrix m1 = tm_1.ToMatrixWithScale();
	FTransform tm_2_prime;
	tm_2_prime.SetLocation(FVector(tm_2.tt.x, tm_2.tt.y, tm_2.tt.z));
	tm_2_prime.SetRotation(FQuat(tm_2.r.x, tm_2.r.y, tm_2.r.z, tm_2.r.w));
	tm_2_prime.SetScale3D(FVector(tm_2.s.x, tm_2.s.y, tm_2.s.z));
	return tm_1.Equals(tm_2_prime, 0.05f);
}

void FAnimNode_MotionPipe::DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
{
	printOutSkeletalHierachy(ref, idx_tree, id_node, identation);
}

void FAnimNode_MotionPipe::DBG_printOutSkeletalHierachy(HBODY root_body) const
{
	printOutSkeletalHierachy(root_body);
}

void FAnimNode_MotionPipe::DBG_VisTransform(const FTransform& tm_l2w, FAnimInstanceProxy* animProxy, float axis_len, float thickness) const
{
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
	FVector4 ori_w = tm_l2w.TransformFVector4(ori);
	const int n_axis = sizeof(axis_ends) / sizeof(FVector4);
	for (int i_end = 0; i_end < n_axis; i_end ++)
	{
		FVector4 end_w = tm_l2w.TransformFVector4(axis_ends[i_end]);
		animProxy->AnimDrawDebugLine(ori_w
									, end_w
									, axis_color[i_end]
									, false // bPersistentLines =
									, -1.f  // LifeTime =
									, thickness);
	}
}



void FAnimNode_MotionPipe::DBG_VisTargets(FAnimInstanceProxy_MotionPipe* animProxy) const
{
	for (auto target : m_targets)
	{
		DBG_VisTransform(target.tm_l2w, animProxy);
	}
}

void FAnimNode_MotionPipe::DBG_VisEEFs(FAnimInstanceProxy_MotionPipe* animProxy) const
{
	for (auto target : m_targets)
	{
		_TRANSFORM l2c0_b;
		get_body_transform_LtoC0(target.h_body, &l2c0_b);
		FTransform l2c0_u;
		Convert(l2c0_b, l2c0_u);
		DBG_VisTransform(l2c0_u * m_C0toW, animProxy);
	}
}

void FAnimNode_MotionPipe::DBG_VisCHANNELs(FAnimInstanceProxy* animProxy) const
{
	auto Draw_Transform = [this, m_C0toW = std::as_const(m_C0toW), animProxy](HBODY hBody, float axis_len, float thickness)
						{
							_TRANSFORM l2c0_sim;
							get_body_transform_LtoC0(hBody, &l2c0_sim);
							FTransform l2c0_sim_2;
							Convert(l2c0_sim, l2c0_sim_2);
							FTransform l2w_sim = l2c0_sim_2 * m_C0toW;
							DBG_VisTransform(l2w_sim, animProxy, axis_len, thickness);
						};

	int32 n_channels = m_channelsFBX.Num();
	Draw_Transform(m_channelsFBX[0].h_body, 20.0f, 2.0f);
	for (int i_channel = 1; i_channel < n_channels; i_channel ++)
		Draw_Transform(m_channelsFBX[i_channel].h_body, 10.0f, 1.0f);


}

#endif