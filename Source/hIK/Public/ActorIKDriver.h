// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ActorIKDriver.generated.h"

UCLASS()
class HIK_API AActorIKDriver : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AActorIKDriver();
	// Called when the game starts or when spawned
	virtual void Connect(const TArray<AActor*> &actor_drivees);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
