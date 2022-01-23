#include "AnimInstance_HIKDrivee.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "HAL/ThreadManager.h"

const float UAnimInstance_HIKDrivee::c_maxSigmaDistsqrTr2Tar = 600; // sigma(10*10) = 15*15*6

UAnimInstance_HIKDrivee::UAnimInstance_HIKDrivee()
	: DBG_VisBody_i(0)
	, m_bindings({
				{RH, -1, FTransform::Identity},	// "wrist_R"
				{LH, -1, FTransform::Identity},	// "wrist_L"
				{RF, -1, FTransform::Identity},	// "foot_R"
				{LF, -1, FTransform::Identity},	// "foot_L"
				{PW, -1, FTransform::Identity},	// "root"
				{HMD,-1, FTransform::Identity}	// "head"
			})
	, m_nUpdateTargets(0)
	, m_nIKReset(0)
{
	FileConfName = FString("HIK.xml");
}


void UAnimInstance_HIKDrivee::PreUpdateAnimation(float DeltaSeconds)
{
	Super::PreUpdateAnimation(DeltaSeconds);
}

void UAnimInstance_HIKDrivee::NativeInitializeAnimation()
{
	m_targets.Reset();
	Super::NativeInitializeAnimation();
}

void UAnimInstance_HIKDrivee::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
	m_targets.Reset();
}

void UAnimInstance_HIKDrivee::OnPreUpdate(FAnimInstanceProxy_MotionPipe* proxy) const
{
#if 0
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
 	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
 	LOGIKVar(LogInfoWCharPtr, *ThreadName);
 	LOGIKVar(LogInfoInt, ThreadId);
#endif
	if (m_nIKReset > 0)
	{
		proxy->PushIKReset();
		m_nIKReset --;
	}
	if (m_nUpdateTargets > 0)
	{
		proxy->PushUpdateTargets(m_targets);
		m_nUpdateTargets --;
	}
}

void UAnimInstance_HIKDrivee::OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy)
{
#if 0
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
 	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
 	LOGIKVar(LogInfoWCharPtr, *ThreadName);
 	LOGIKVar(LogInfoInt, ThreadId);
#endif
	if (!(m_nUpdateTargets > 0))
	{
		proxy->PullUpdateTargets(m_targets);
		m_nUpdateTargets ++;
	}
	FTransform tm_entity;
	proxy->PullUpdateEntity(tm_entity);
	AActor* act = GetOwningActor();
	act->SetActorTransform(tm_entity);
}

bool UAnimInstance_HIKDrivee::VRIK_Connect(const TArray<USceneComponent*>& trackers)
{
	m_trackers = trackers;
	check(m_targets.Num() == m_bindings.Num());
	if (m_targets.Num() != m_bindings.Num())
		return false;

	int32 N = m_targets.Num();
	TArray<int32> targetIDs; targetIDs.SetNum(N);
	TArray<bool> binded; binded.SetNum(N);
	for (int32 i_tar = 0; i_tar < N; i_tar++)
	{
		targetIDs[i_tar] = i_tar;
		binded[i_tar] = false;
	}

	auto closestTarget = [&c_binded = std::as_const(binded)
							, &c_targets = std::as_const(m_targets)](const FVector& ttTR) -> int32
		{
			float distsqr_min = FLT_MAX;
			int32 i_target_min = -1;
			for (int32 i_target = 0; i_target < c_targets.Num(); i_target ++)
			{
				if (c_binded[i_target])
					continue;
				auto target_i = c_targets[i_target];
				auto distsqr_i = FVector::DistSquared(target_i.tm_l2w.GetTranslation(), ttTR);
				if (distsqr_i < distsqr_min)
				{
					distsqr_min = distsqr_i;
					i_target_min = i_target;
				}
			}
			check(-1 != i_target_min);
			return i_target_min;
		};

	for (auto& bind_i : m_bindings)
	{
		auto pos_tr_i = trackers[bind_i.trID]->GetComponentLocation();
		int32 i_tar_min = closestTarget(pos_tr_i);
		bind_i.tarID = i_tar_min;
		binded[i_tar_min] = true;
	}


	float sigma_dist_sqr_tr_tar = 0;
	int n_bindings = 0;
	for (auto& bind_i : m_bindings)
	{
		const FTransform& tar2w_i = m_targets[bind_i.tarID].tm_l2w;
		const FTransform& tr2w_i = trackers[bind_i.trID]->GetComponentTransform();
		sigma_dist_sqr_tr_tar += (FVector::DistSquared(tar2w_i.GetTranslation(), tr2w_i.GetTranslation()));
		FTransform w2tr_i = tr2w_i.Inverse();
		bind_i.tar2tr = tar2w_i*w2tr_i;
		n_bindings ++;
	}
	if (sigma_dist_sqr_tr_tar > c_maxSigmaDistsqrTr2Tar
		|| n_bindings < N_SPECTRAKS+1) // 6 tracking locations
	{
		VRIK_Disconnect();
		return false;
	}
	else
		return true;
}

void UAnimInstance_HIKDrivee::VRIK_Disconnect()
{
	m_trackers.Reset();
	m_targets.Reset();
	m_nUpdateTargets = 1; // to push an empty set of targets
	m_nIKReset = 1;
	for (auto& bind_i : m_bindings)
	{
		bind_i.tarID = -1;
	}
}

