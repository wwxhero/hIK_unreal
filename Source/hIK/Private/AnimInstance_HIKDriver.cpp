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

bool UAnimInstance_HIKDriver::OnPostUpdate(const FAnimInstanceProxy_HIK* proxy)
{
	LOGIK("UAnimInstance_HIKDriver::OnPostUpdate()");

	bool initialize_self_targets = Super::OnPostUpdate(proxy);

	bool initialize_other_endeffs = false;
	for (auto drivee : Drivees_)
	{
		TArray<EndEEF>& eefs_i = drivee->GetEEFs();
		initialize_other_endeffs = (eefs_i.Num() > 0);
		if (initialize_other_endeffs)
		{
			int32 n_eefs = eefs_i.Num();
			check(n_eefs == m_targets.Num());
			m_eefsPipe.SetNum(n_eefs, false);
			for (int i_eef = 0; i_eef < n_eefs; i_eef++)
			{
				m_eefsPipe[i_eef].Add(&eefs_i[i_eef]);
				check(FString(body_name_w(m_targets[i_eef].h_body)) == FString(body_name_w(eefs_i[i_eef].h_body)));
			}
		}
	}

	bool update_eefs_pipe = (!initialize_other_endeffs && !initialize_self_targets);
	if (update_eefs_pipe)
	{
		auto c2w_u = proxy->GetSkelMeshCompLocalToWorld(); // get compoenent to world transformation
		check(m_targets.Num() == m_eefsPipe.Num());
		int32 n_targets = m_targets.Num();
		for (int32 i_target = 0; i_target < n_targets; i_target ++)
		{
			auto& t_i = m_targets[i_target];
			FTransform l2c_u;
			_TRANSFORM l2c_b;
			get_body_transform_l2w(t_i.h_body, &l2c_b);
			Convert(l2c_b, l2c_u);
			t_i.tm_l2w = l2c_u * c2w_u;

			for (auto effs_i_j : m_eefsPipe[i_target])
			{
				effs_i_j->tm_l2w = t_i.tm_l2w;
				
			}
		}
	}

	return true;
}