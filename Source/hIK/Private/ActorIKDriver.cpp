// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriver.h"
#include "ActorIKDrivee.h"
#include "ActorIKDriveeMeta.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstance_HIKDriver.h"
#include "ActorIKDriverHelper.h"

// Sets default values
AActorIKDriver::AActorIKDriver()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AActorIKDriver::Connect(const TArray<AActor*> &actor_drivees)
{
	ActorIKDriverHelper::Connect(this, actor_drivees);
}

// Called every frame
void AActorIKDriver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

