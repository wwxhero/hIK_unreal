// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriver.h"
#include "ActorIKDrivee.h"
#include "ActorIKDriveeMeta.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstance_HIKDriver.h"

// Sets default values
AActorIKDriver::AActorIKDriver()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AActorIKDriver::BeginPlay()
{
	Super::BeginPlay();
	TArray<UAnimInstance_HIKDrivee*> drivees;
	for (TActorIterator<AActor> actorItr = TActorIterator<AActor>(GetWorld())
		; actorItr
		; ++ actorItr )
	{
		AActorIKDrivee* drivee_sk = Cast<AActorIKDrivee, AActor>(*actorItr);
		if (nullptr != drivee_sk)
		{
			auto anim_inst_temp = drivee_sk->GetSkeletalMeshComponent()->GetAnimInstance();
			check(NULL != anim_inst_temp);
			auto anim_inst = Cast<UAnimInstance_HIKDrivee, UAnimInstance>(anim_inst_temp);
			check(NULL != anim_inst);
			drivees.Add(anim_inst);
		}

		AActorIKDriveeMeta* drivee_mh = Cast<AActorIKDriveeMeta, AActor>(*actorItr);
		if (nullptr != drivee_mh)
		{
			auto anim_inst_temp = drivee_mh->GetBodySkeletalMeshComponent()->GetAnimInstance();
			check(NULL != anim_inst_temp);
			auto anim_inst = Cast<UAnimInstance_HIKDrivee, UAnimInstance>(anim_inst_temp);
			check(NULL != anim_inst);
			drivees.Add(anim_inst);
		}
	}

	TInlineComponentArray<USkeletalMeshComponent*> primComponents(this, true);
	USkeletalMeshComponent* ret = nullptr;
	for (USkeletalMeshComponent* comp_i : primComponents)
	{
		UAnimInstance_HIKDriver* anim_driver = Cast<UAnimInstance_HIKDriver, UAnimInstance>(comp_i->GetAnimInstance());
		if (nullptr != anim_driver)
			anim_driver->InitializeDrivees(drivees);
	}
}

// Called every frame
void AActorIKDriver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AActorIKDriver::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

