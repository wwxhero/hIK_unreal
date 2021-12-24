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
	, m_ikConnected(false)
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
	if (m_ikConnected)
		proxy->PushUpdateTargets(m_targets);
}

void UAnimInstance_HIKDrivee::OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy)
{
#if 0
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
 	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
 	LOGIKVar(LogInfoWCharPtr, *ThreadName);
 	LOGIKVar(LogInfoInt, ThreadId);
#endif
	if (!m_ikConnected)
		proxy->PullUpdateTargets(m_targets);
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
	m_ikConnected = true;
}

void UAnimInstance_HIKDrivee::VRIK_UpdateTargets()
{
	if (!m_ikConnected)
		return;
	check(m_targets.Num() == m_bindings.Num());
	for (auto bind_i : m_bindings)
	{
		auto tracker_i = m_trackers[bind_i.trID];
		FTransform tr2w = tracker_i->GetComponentTransform();
		FTransform tar2w = bind_i.tar2tr * tr2w;
		m_targets[bind_i.tarID].tm_l2w = tar2w;
	}
}