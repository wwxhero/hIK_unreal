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
protected:
	virtual void Tick(float DeltaSeconds);
	void UpdateBoneVis();
	float Err_q(const FQuat& q_0, const FQuat& q_1) const;
	FSkinWeightVertexBuffer* GetSkinWeightBuffer(const USkinnedMeshComponent* pThis, int32 LODIndex);
private:
	UMaterialInterface* m_materialVertexClr;
	AActorIKDriver* m_driver;
	float m_errS;
};

