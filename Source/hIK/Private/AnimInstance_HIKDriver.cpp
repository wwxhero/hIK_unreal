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
	if (VALID_HANDLE(m_hBVH))
		unload_bvh(m_hBVH);
	FString rootDir = FPaths::ProjectDir();

	FString bvhpath_full = rootDir + m_filenames[0];
	m_hBVH = load_bvh_w(*bvhpath_full);
	LOGIK(TCHAR_TO_ANSI(*bvhpath_full));

//	bool bvh_loaded = VALID_HANDLE(m_hBVH);
//	LOGIKVar(LogInfoBool, bvh_loaded);
//	if (bvh_loaded)
//	{
//		NUM_Frames_ = get_n_frames(m_hBVH);
//		I_Frame_ = 0;
//	}

}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	if (VALID_HANDLE(m_hBVH))
		unload_bvh(m_hBVH);
	m_hBVH = H_INVALID;
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

	TArray<EndEF> targets;
	proxy->PullUpdateEEFs(targets);

	int32 n_targets = targets.Num();
	for (auto drivee : Drivees_)
	{
		drivee->PushUpdateEEFs(targets);
	}
}