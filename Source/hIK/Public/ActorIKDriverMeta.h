// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActorIKDriverMeta.generated.h"

UCLASS()
class HIK_API AActorIKDriverMeta : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AActorIKDriverMeta();
	// Called when the game starts or when spawned
	virtual void Connect(const TArray<AActor*> &actor_drivees);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
