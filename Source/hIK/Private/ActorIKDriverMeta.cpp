// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriverMeta.h"
#include "ActorIKDrivee.h"
#include "ActorIKDriveeMeta.h"
#include "EngineUtils.h"
#include "AnimInstance_HIKDrivee.h"
#include "AnimInstance_HIKDriver.h"
#include "ActorIKDriverHelper.h"

// Sets default values
AActorIKDriverMeta::AActorIKDriverMeta()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AActorIKDriverMeta::Connect(const TArray<AActor*> &actor_drivees)
{
	ActorIKDriverHelper::Connect(this, actor_drivees);
}

// Called every frame
void AActorIKDriverMeta::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

