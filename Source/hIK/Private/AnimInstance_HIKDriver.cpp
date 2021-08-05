#include "AnimInstance_HIKDriver.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstanceProxy_HIK.h"
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

void UAnimInstance_HIKDriver::OnPostUpdate(const FAnimInstanceProxy_HIK* proxy)
{
#ifdef _DEBUG
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
	LOGIKVar(LogInfoWCharPtr, *ThreadName);
	LOGIKVar(LogInfoInt, ThreadId);
#endif
	const TArray<EndEF>& targets = proxy->GetEEFs();
	int32 n_targets = targets.Num(); 	// for a driver, eef == target
	if (n_targets > 0)
	{
		m_targets.SetNum(n_targets, false);
		std::map<FString, FString> driver2drivee;
		CopyMatches(driver2drivee, 1);
		for (int i_target = 0; i_target < n_targets; i_target ++)
		{
			const FTransform& tm_i = targets[i_target].tm_l2w;
			HBODY body_i = targets[i_target].h_body;

			const wchar_t* name_target_driver = body_name_w(body_i);
			auto it_name_eef_drivee = driver2drivee.find(name_target_driver);
			check(driver2drivee.end() != it_name_eef_drivee);
			Target_BVH target;
			target.name_eef_drivee = it_name_eef_drivee->second;
			target.tm_l2w = tm_i;
			target.h_body = body_i;
			m_targets[i_target] = target;
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

	n_targets = m_targets.Num();
	for (auto drivee : Drivees_)
	{
		auto eefs = drivee->GetEEFs();
		int32 n_eefs = eefs.Num();
		check (n_eefs == n_targets);
		for (int32 i_target = 0; i_target < n_targets; i_target ++)
		{
			const Target_BVH& target_i = m_targets[i_target];
#ifdef _DEBUG
			const EndEF& eef_i = eefs[i_target];
			// const wchar_t* eef_i_name = body_name_w(eef_i.h_body);
			// check (target_i.name_eef_drivee == FString(eef_i_name));
			check (target_i.name_eef_drivee == eef_i.name);
#endif
			drivee->UpdateEEF(i_target, target_i.tm_l2w);
		}
	}
}