// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ActorIKDrivee.h"
#include "ActorIKDriver.h"
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
	void Connect(AActor* driver);
	void UpdateBoneVis(int32 boneID_g);
	FSkinWeightVertexBuffer* GetSkinWeightBuffer(const USkinnedMeshComponent* pThis, int32 LODIndex);
private:
	UMaterialInterface* m_materialVertexClr;
	int32 m_boneGSel;

	AActorIKDriver* m_driver;
};

