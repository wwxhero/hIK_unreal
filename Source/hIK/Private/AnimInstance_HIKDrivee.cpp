#include "AnimInstance_HIKDrivee.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "HAL/ThreadManager.h"


void UAnimInstance_HIKDrivee::PreUpdateAnimation(float DeltaSeconds)
{
	Super::PreUpdateAnimation(DeltaSeconds);
}

FString UAnimInstance_HIKDrivee::GetFileConfName() const
{
	return FString(L"HIK.xml");
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
	// proxy->PullUpdateTargets(m_targets); // I don't know what this is for, but at least it is not harmful
	FTransform tm_entity;
	proxy->PullUpdateEntity(tm_entity);
	AActor* act = GetOwningActor();
	act->SetActorTransform(tm_entity);
}

