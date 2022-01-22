// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/SkeletalMeshActor.h"
#include "ActorIKDrivee.generated.h"

/**
 * 
 */
UCLASS()
class HIK_API AActorIKDrivee : public ASkeletalMeshActor
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = Settings, meta=(PinShownByDefault))
	void SetDBGVisBody_I(int32 i_body);
};
