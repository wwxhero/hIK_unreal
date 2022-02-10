// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ActorIKDrivee.h"
#include "ActorIKDriveeErrVis.generated.h"

UCLASS()
class HIK_API AActorIKDriveeErrVis : public AActorIKDrivee
{
	GENERATED_BODY()
	UFUNCTION(BlueprintCallable, Category = HIKAnim, meta=(PinShownByDefault))
	void VisBoneNext();
	UFUNCTION(BlueprintCallable, Category = HIKAnim, meta=(PinShownByDefault))
	void VisBonePrev();
public:
	AActorIKDriveeErrVis();
	virtual void BeginPlay();
	void UpdateBoneVis(int32 boneID_g);
private:
	UMaterialInterface* m_materialVertexClr;
	int32 m_boneGSel;
};

