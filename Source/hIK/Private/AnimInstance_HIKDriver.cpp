#include "AnimInstance_HIKDriver.h"
#include "Misc/Paths.h"
#include "ik_logger.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstanceProxy_HIK.h"
#include "transform_helper.h"

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

}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	if (VALID_HANDLE(m_hBVH))
	 	unload_bvh(m_hBVH);
	m_hBVH = H_INVALID;
	NUM_Frames_ = 0;
	I_Frame_ = -1;

	Super::NativeUninitializeAnimation();
	LOGIK("UAnimInstance_HIKDriver::NativeUninitializeAnimation");
}

FString UAnimInstance_HIKDriver::GetFileConfName() const
{
	return FString(L"FK.xml");
}

void UAnimInstance_HIKDriver::OnPostUpdate(const FAnimInstanceProxy_HIK* proxy)
{
	// LOGIK("UAnimInstance_HIKDriver::OnPostUpdate()");
	auto c2w_u = proxy->GetSkelMeshCompLocalToWorld(); // get compoenent to world transformation
	for (Target t_i : m_targets)
	{
		FTransform l2c_u;
		_TRANSFORM l2c_b;
		get_body_transform_l2w(t_i.h_body, &l2c_b);
		Convert(l2c_b, l2c_u);
		t_i.tm_l2w = l2c_u * c2w_u;
	}
}