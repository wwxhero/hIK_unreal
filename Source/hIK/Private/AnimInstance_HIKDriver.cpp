#include "AnimInstance_HIKDriver.h"

void UAnimInstance_HIKDriver::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeInitializeAnimation"));
	if (VALID_HANDLE(m_hBVH))
		unload_bvh(m_hBVH);
	if (BVHPath_.Len() > 0)
		m_hBVH = load_bvh(*BVHPath_);
	if (VALID_HANDLE(m_hBVH))
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
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeUninitializeAnimation"));
}