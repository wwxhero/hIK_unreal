// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActorIKDriveeMeta.generated.h"

UCLASS()
class HIK_API AActorIKDriveeMeta : public AActor
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = Settings, meta=(PinShownByDefault))
	void Initialize(USkeletalMeshComponent* body);
public:	
	// Sets default values for this actor's properties
	AActorIKDriveeMeta();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	class USkeletalMeshComponent* GetBodySkeletalMeshComponent() const;	
};
