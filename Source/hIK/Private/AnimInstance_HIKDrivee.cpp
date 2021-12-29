#include "AnimInstance_HIKDrivee.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "HAL/ThreadManager.h"

UAnimInstance_HIKDrivee::UAnimInstance_HIKDrivee()
	: DBG_VisBody_i(0)
	, m_bindings({
				{RH, 5, FTransform::Identity},	// "wrist_R"
				{LH, 4, FTransform::Identity},	// "wrist_L"
				{RF, 1, FTransform::Identity},	// "foot_R"
				{LF, 0, FTransform::Identity},	// "foot_L"
				{PW, 3, FTransform::Identity},	// "root"
				{HMD, 2, FTransform::Identity}	// "head"
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

void UAnimInstance_HIKDrivee::VRIK_Connect(const TArray<USceneComponent*>& trackers)
{
	m_trackers = trackers;
	check(m_targets.Num() == m_bindings.Num());	
	for (auto& bind_i : m_bindings)
	{
		const FTransform& tar2w_i = m_targets[bind_i.tarID].tm_l2w;
		const FTransform& tr2w_i = trackers[bind_i.trID]->GetComponentTransform();
		FTransform w2tr_i = tr2w_i.Inverse();
		bind_i.tar2tr = tar2w_i*w2tr_i;
	}
}

void UAnimInstance_HIKDrivee::VRIK_Disconnect()
{
	m_trackers.Reset();
	m_targets.Reset();
	m_bindings.Reset();
	m_nUpdateTargets = 1; // to push an empty set targets
	m_nIKReset = 1;
}

