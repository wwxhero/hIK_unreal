#include "AnimInstance_HIKDriver.h"

void UAnimInstance_HIKDriver::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeInitializeAnimation"));
	if (H_INVALID != m_hBVH)
		unload_bvh(m_hBVH);
	m_hBVH = load_bvh(L"D:\\motionCapturing\\bvh\\cmuconvert01-09\\01\\01_01.bvh");
	if (H_INVALID != m_hBVH)
		NUM_Frames = get_n_frames(m_hBVH);
}

void UAnimInstance_HIKDriver::NativeUninitializeAnimation()
{
	if (H_INVALID != m_hBVH)
	 	unload_bvh(m_hBVH);
	m_hBVH = H_INVALID;
	NUM_Frames = 0;
	Super::NativeUninitializeAnimation();
	UE_LOG(LogHIK, Display, TEXT("UAnimInstance_HIKDriver::NativeUninitializeAnimation"));
}