#include "AnimInstance_HIKDriver.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "transform_helper.h"
#include "HAL/ThreadManager.h"
#include "ActorIKDrivee.h"
#include "ActorIKDriveeMeta.h"
#include "Kismet/GameplayStatics.h"

void UAnimInstance_HIKDriver::InitializeDrivees(const TArray<UAnimInstance_HIKDrivee*>& drivees)
{
	m_drivees = drivees;

}

void UAnimInstance_HIKDriver::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeInitializeAnimation"));
	m_drivees.Reset();
}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	NUM_Frames_ = 0;
	I_Frame_ = -1;

	Super::NativeUninitializeAnimation();
}

void UAnimInstance_HIKDriver::OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy)
{
#if 0
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
	LOGIKVar(LogInfoWCharPtr, *ThreadName);
	LOGIKVar(LogInfoInt, ThreadId);
#endif

	if (proxy->PullUpdateNFrames(NUM_Frames_))
	{
		I_Frame_ = 0;
	}

	TArray<Target> targets;
	proxy->PullUpdateTargets(targets);

	int32 n_targets = targets.Num();
	for (auto drivee : m_drivees)
	{
		drivee->PushUpdateTargets(targets);
	}
}
