#include "AnimInstance_HIKDriver.h"
#include "Misc/Paths.h"
#include "ik_logger.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
void UAnimInstance_HIKDriver::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeInitializeAnimation"));
	if (VALID_HANDLE(m_hBVH))
		unload_bvh(m_hBVH);
	FString rootDir = FPaths::ProjectDir();

	FString bvhpath_full = rootDir + c_BVHFile;
	m_hBVH = load_bvh_w(*bvhpath_full);
	LOGIK(TCHAR_TO_ANSI(*bvhpath_full));

	bool bvh_loaded = VALID_HANDLE(m_hBVH);
	LOGIKVar(LogInfoBool, bvh_loaded);
	if (bvh_loaded)
	{
		NUM_Frames_ = get_n_frames(m_hBVH);
		I_Frame_ = 0;
	}

	if (VALID_HANDLE(m_hDrvConf))
		unload_conf(m_hDrvConf);

	FString drvpath_full = rootDir + c_DRVConfFile;
	m_hDrvConf = load_conf(*drvpath_full);
	LOGIK(TCHAR_TO_ANSI(*drvpath_full));

	bool conf_loaded = VALID_HANDLE(m_hDrvConf);
	LOGIKVar(LogInfoBool, conf_loaded);

}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	if (VALID_HANDLE(m_hBVH))
	 	unload_bvh(m_hBVH);
	m_hBVH = H_INVALID;
	NUM_Frames_ = 0;
	I_Frame_ = -1;

	if (VALID_HANDLE(m_hDrvConf))
		unload_conf(m_hDrvConf);
	m_hDrvConf = H_INVALID;

	Super::NativeUninitializeAnimation();
	LOGIK("UAnimInstance_HIKDriver::NativeUninitializeAnimation");
}

void UAnimInstance_HIKDriver::UnLockTarget(TArray<Target>* targets)
{
	bool locked = (NULL != targets);
	if (locked)
	{
		for (auto drivee : Drivees_)
		{
			auto* targets_drivee = drivee->LockTarget(false);
			if (targets_drivee)
				*targets_drivee = *targets;
			drivee->UnLockTarget(targets_drivee);
		}
	}
	Super::UnLockTarget(targets);
}