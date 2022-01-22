// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDrivee.h"
#include "AnimInstance_HIKDrivee.h"

void AActorIKDrivee::SetDBGVisBody_I(int32 i_body)
{
	UAnimInstance* anim_inst_temp = GetSkeletalMeshComponent()->GetAnimInstance();
	UAnimInstance_HIKDrivee* anim_inst = Cast<UAnimInstance_HIKDrivee, UAnimInstance>(anim_inst_temp);
	anim_inst->DBG_VisBody_i = i_body;
}