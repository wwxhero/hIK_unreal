#include "AnimInstance_HIKDrivee.h"
#include "Misc/Paths.h"
#include "ik_logger_unreal.h"
#include "AnimInstanceProxy_HIK.h"



void UAnimInstance_HIKDrivee::PreUpdateAnimation(float DeltaSeconds)
{
	Super::PreUpdateAnimation(DeltaSeconds);
}

FString UAnimInstance_HIKDrivee::GetFileConfName() const
{
	return FString(L"HIK.xml");
}

void UAnimInstance_HIKDrivee::OnPostUpdate(const FAnimInstanceProxy_HIK* proxy)
{
	const TArray<EndEF>& eefs = proxy->GetEEFs();
	if (eefs.Num() > 0)
	{
		m_eefs = eefs;
		m_eefs.Sort(FCompareEEF());
#ifdef _DEBUG
		for (auto eef : m_eefs)
		{
			LOGIKVar(LogInfoWCharPtr, body_name_w(eef.h_body));
		}
#endif
	}
}

