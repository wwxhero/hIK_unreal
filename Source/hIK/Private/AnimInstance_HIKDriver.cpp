#include "AnimInstance_HIKDriver.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "transform_helper.h"
#include "HAL/ThreadManager.h"

void UAnimInstance_HIKDriver::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeInitializeAnimation"));

}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	NUM_Frames_ = 0;
	I_Frame_ = -1;

	Super::NativeUninitializeAnimation();
}

FString UAnimInstance_HIKDriver::GetFileConfName() const
{
	return FString(L"FK.xml");
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
	for (auto drivee : Drivees_)
	{
		drivee->PushUpdateTargets(targets);
	}
}