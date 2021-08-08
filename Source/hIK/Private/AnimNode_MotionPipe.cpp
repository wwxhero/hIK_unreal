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
	, m_bodies {H_INVALID, H_INVALID}
	, m_moNodes {H_INVALID, H_INVALID}
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

void FAnimNode_MotionPipe::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);
	FAnimNode_Base::CacheBones_AnyThread(Context);
	// auto proxy = Cast<FAnimInstanceProxy_MotionPipe, FAnimInstanceProxy>(Context.AnimInstanceProxy);
	auto proxy = static_cast<FAnimInstanceProxy_MotionPipe*>(Context.AnimInstanceProxy);
	check(proxy->ValidPtr());
	UnCacheBones_AnyThread();

	auto RequiredBones = proxy->GetRequiredBones();

	// Initialize the native FBX bodies
	const FReferenceSkeleton& ref = RequiredBones.GetReferenceSkeleton();
	BITree idx_tree;
	ConstructBITree(ref, idx_tree);
	std::set<FString> namesOnPair_fbx;
	c_animInst->CopyMatches(namesOnPair_fbx, c_idxFBX);
	HBODY body_fbx_comp = InitializeChannelFBX_AnyThread(ref, RequiredBones, idx_tree, namesOnPair_fbx);
	HBODY body_fbx_world = InitializeChannelFBX_AnyThread(ref
												, RequiredBones
												, proxy->GetSkelMeshCompLocalToWorld()
												, idx_tree
												, namesOnPair_fbx);
	HBODY body_fbx = VALID_HANDLE(body_fbx_comp) ? body_fbx_comp : body_fbx_world;
#ifdef _DEBUG
	UE_LOG(LogHIK, Display, TEXT("FAnimNode_MotionPipe::CacheBones_AnyThread"));
	DBG_printOutSkeletalHierachy(ref, idx_tree, 0, 0);
	DBG_printOutSkeletalHierachy(body_fbx);
	// check(DBG_verifyChannel(ref));
#endif
	ReleaseBITree(idx_tree);
	// end of initialization

	// Initialize the SIM (BVH or HTR) bodies
	HBODY body_sim = InitializeBodySim_AnyThread(body_fbx);
	// end of initialization

	bool fbx_created = VALID_HANDLE(body_fbx);
	LOGIKVar(LogInfoBool, fbx_created);
	bool sim_created = VALID_HANDLE(body_sim);
	LOGIKVar(LogInfoBool, fbx_created);

	bool ok = fbx_created && sim_created;

	if (ok)
	{
		m_bodies[c_idxFBX] = body_fbx;
		m_bodies[c_idxSim] = body_sim;

		const wchar_t* (*matches)[2] = NULL;
		int n_match = c_animInst->CopyMatches(&matches);
		float src2dst_w[3][3] = { 0 };
		c_animInst->CopySrc2Dst_w(src2dst_w);
		auto moDriver = create_tree_motion_node(m_bodies[0]);
		auto moDrivee = create_tree_motion_node(m_bodies[1]);
		bool mo_bvh_created = VALID_HANDLE(moDriver);
		bool mo_drv_created = VALID_HANDLE(moDrivee);
		bool cnn_bvh2htr = mo_bvh_created && mo_drv_created
						&& motion_sync_cnn_cross_w(moDriver, moDrivee, FIRSTCHD, matches, n_match, src2dst_w);
		ok =  (mo_bvh_created
			&& mo_drv_created
			&& cnn_bvh2htr);
		LOGIKVar(LogInfoBool, mo_bvh_created);
		LOGIKVar(LogInfoBool, mo_drv_created);
		LOGIKVar(LogInfoBool, cnn_bvh2htr);

		m_moNodes[0] = moDriver;
		m_moNodes[1] = moDrivee;
	}



	if (!ok)
		UnCacheBones_AnyThread();
	else
		InitializeEEFs_AnyThread(proxy, m_eefs);

	BasePose.CacheBones(Context);
}

void FAnimNode_MotionPipe::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);

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

	for (int i_retar = 0; i_retar < 2; i_retar ++)
	{
		auto& moNode_i = m_moNodes[i_retar];
		if (VALID_HANDLE(moNode_i))
			destroy_tree_motion_node(moNode_i);
		moNode_i = H_INVALID;

		auto & body_i = m_bodies[i_retar];

		if (VALID_HANDLE(body_i))
			destroy_tree_body(body_i);
		body_i = H_INVALID;
	}

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

void FAnimNode_MotionPipe::DBG_GetComponentSpaceTransform(const FAnimNode_MotionPipe::CHANNEL& channel, FTransform& tm_l2compo, const FReferenceSkeleton& ref_sk) const
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
		if (c_animInst->CopyScale(0, *ref_sk.GetBoneName(idx_bone).ToString(), bs_x, bs_y, bs_z))
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

void FAnimNode_MotionPipe::DBG_GetComponentSpaceTransform2(const FAnimNode_MotionPipe::CHANNEL& channel, _TRANSFORM& tm, const FReferenceSkeleton& ref_sk) const
{
	HBODY h_body = channel.h_body;
	get_body_transform_l2w(h_body, &tm);
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

bool FAnimNode_MotionPipe::DBG_verifyChannel(const FReferenceSkeleton& ref_sk) const
{
	bool verified = true;
	auto n_channels = m_channelsFBX.Num();
	const wchar_t* res[2] = {L"NEqual", L"Equal"};
	for (int32 i_channel = 0
		; i_channel < n_channels
		&& verified
		; i_channel++)
	{
		FTransform tm_unrel;
		_TRANSFORM tm_arti;
		DBG_GetComponentSpaceTransform(m_channelsFBX[i_channel], tm_unrel, ref_sk);
		DBG_GetComponentSpaceTransform2(m_channelsFBX[i_channel], tm_arti, ref_sk);
		verified = DBG_EqualTransform(tm_unrel, tm_arti);
		FName name = ref_sk.GetBoneName(m_channelsFBX[i_channel].r_bone.BoneIndex);
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

void FAnimNode_MotionPipe::DBG_printOutSkeletalHierachy_recur(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
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

void FAnimNode_MotionPipe::DBG_printOutSkeletalHierachy(const FReferenceSkeleton& ref, const BITree& idx_tree, int32 id_node, int identation) const
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

void FAnimNode_MotionPipe::DBG_printOutSkeletalHierachy(HBODY root_body) const
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

void FAnimNode_MotionPipe::DBG_VisTransform(const FTransform& tm_l2w, FAnimInstanceProxy* animProxy) const
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
									, 1);
	}
}



void FAnimNode_MotionPipe::DBG_VisTargets(FAnimInstanceProxy_MotionPipe* animProxy) const
{
	for (auto eef : m_eefs)
	{
		DBG_VisTransform(eef.tm_l2w, animProxy);
	}
}

#endif