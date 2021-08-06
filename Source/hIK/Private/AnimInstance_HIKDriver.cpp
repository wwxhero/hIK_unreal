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

	bool bvh_loaded = VALID_HANDLE(m_hBVH);
	LOGIKVar(LogInfoBool, bvh_loaded);
	if (bvh_loaded)
	{
		NUM_Frames_ = get_n_frames(m_hBVH);
		I_Frame_ = 0;
	}

	m_targets.Reset();
}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	if (VALID_HANDLE(m_hBVH))
		unload_bvh(m_hBVH);
	m_hBVH = H_INVALID;
	NUM_Frames_ = 0;
	I_Frame_ = -1;

	m_targets.Reset();
	Super::NativeUninitializeAnimation();
}

FString UAnimInstance_HIKDriver::GetFileConfName() const
{
	return FString(L"FK.xml");
}

void UAnimInstance_HIKDriver::OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy)
{
#ifdef _DEBUG
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
	LOGIKVar(LogInfoWCharPtr, *ThreadName);
	LOGIKVar(LogInfoInt, ThreadId);
#endif
	const TArray<EndEF>& eefs = proxy->GetEEFs();
	int32 n_eefs = eefs.Num(); 	// for a driver, eef == target
	if (n_eefs > 0)
	{
		std::map<FString, FString> driver2drivee;
		CopyMatches(driver2drivee, 1);
		m_targets.SetNum(n_eefs, false);
		for (int i_target = 0; i_target < n_eefs; i_target ++)
		{
			auto it_name_eef_drivee = driver2drivee.find(eefs[i_target].name);
			check(driver2drivee.end() != it_name_eef_drivee);
			InitTarget(m_targets[i_target], eefs[i_target], it_name_eef_drivee->second);
		}
		m_targets.Sort(FCompareTarget());

#ifdef _DEBUG
		for (auto target : m_targets)
		{
			LOGIKVar(LogInfoWCharPtr, *target.name_eef_drivee);
			LOGIKVar(LogInfoWCharPtr, body_name_w(target.h_body));
		}
#endif
	}

	int32 n_targets = m_targets.Num();
	for (auto drivee : Drivees_)
	{
		check (drivee->N_eefs() == n_targets);
		for (int32 i_target = 0; i_target < n_targets; i_target ++)
		{
			const Target& target_i = m_targets[i_target];
			drivee->UpdateEEF(i_target
							, target_i.tm_l2w
#ifdef _DEBUG
							, target_i.name_eef_drivee
#endif
							);



		}
	}
}