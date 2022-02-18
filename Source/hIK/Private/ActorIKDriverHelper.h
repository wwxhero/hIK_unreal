#pragma once

class ActorIKDriverHelper
{
public:
	template<typename TActor>
	static void Connect(TActor* pThis, const TArray<AActor*> &actor_drivees)
	{
		TArray<UAnimInstance_HIKDrivee*> drivees;

		for (auto drivee : actor_drivees)
		{
			AActorIKDrivee* drivee_sk = Cast<AActorIKDrivee, AActor>(drivee);
			if (nullptr != drivee_sk)
			{
				auto anim_inst_temp = drivee_sk->GetSkeletalMeshComponent()->GetAnimInstance();
				check(NULL != anim_inst_temp);
				auto anim_inst = Cast<UAnimInstance_HIKDrivee, UAnimInstance>(anim_inst_temp);
				check(NULL != anim_inst);
				drivees.Add(anim_inst);
			}

			AActorIKDriveeMeta* drivee_mh = Cast<AActorIKDriveeMeta, AActor>(drivee);
			if (nullptr != drivee_mh)
			{
				auto anim_inst_temp = drivee_mh->GetBodySkeletalMeshComponent()->GetAnimInstance();
				check(NULL != anim_inst_temp);
				auto anim_inst = Cast<UAnimInstance_HIKDrivee, UAnimInstance>(anim_inst_temp);
				check(NULL != anim_inst);
				drivees.Add(anim_inst);
			}
		}


		TInlineComponentArray<USkeletalMeshComponent*> primComponents(pThis, true);
		for (USkeletalMeshComponent* comp_i : primComponents)
		{
			UAnimInstance_HIKDriver* anim_driver = Cast<UAnimInstance_HIKDriver, UAnimInstance>(comp_i->GetAnimInstance());
			if (nullptr != anim_driver)
				anim_driver->InitializeDrivees(drivees);
		}
	}
};