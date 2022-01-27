// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriveeMeta.h"

// Sets default values
AActorIKDriveeMeta::AActorIKDriveeMeta()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AActorIKDriveeMeta::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AActorIKDriveeMeta::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AActorIKDriveeMeta::Initialize(USkeletalMeshComponent* body)
{
}

class USkeletalMeshComponent* AActorIKDriveeMeta::GetBodySkeletalMeshComponent() const
{
	TInlineComponentArray<USkeletalMeshComponent*> primComponents(this, true);
	USkeletalMeshComponent* ret = nullptr;
	for (USkeletalMeshComponent* comp_i : primComponents)
	{
		if (TEXT("Body") == comp_i->GetName())
		{
			ret = comp_i;
			break;
		}
	}
	check(nullptr != ret);
	return ret;
}