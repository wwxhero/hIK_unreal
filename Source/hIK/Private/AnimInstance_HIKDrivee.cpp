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
	m_eefs.Reset();
	Super::NativeInitializeAnimation();
}

void UAnimInstance_HIKDrivee::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
	m_eefs.Reset();
}

void UAnimInstance_HIKDrivee::OnPreUpdate(TArray<EndEF>& eefs_i) const
{
	eefs_i = m_eefs;
}

void UAnimInstance_HIKDrivee::OnPostUpdate(const TArray<EndEF>& eefs_0)
{
#ifdef _DEBUG
	uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();
 	FString ThreadName = FThreadManager::Get().GetThreadName(ThreadId);
 	LOGIKVar(LogInfoWCharPtr, *ThreadName);
 	LOGIKVar(LogInfoInt, ThreadId);
#endif

	if (eefs_0.Num() > 0)
	{
		m_eefs = eefs_0;
		m_eefs.Sort(FCompareEEF());
#ifdef _DEBUG
		for (int i_eef = 0; i_eef < m_eefs.Num(); i_eef ++)
		{
			LOGIKVar(LogInfoWCharPtr, body_name_w(m_eefs[i_eef].h_body));
		}
#endif
	}
}

