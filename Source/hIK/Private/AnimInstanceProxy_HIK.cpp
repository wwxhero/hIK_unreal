#include "AnimInstanceProxy_HIK.h"
#include "AnimInstance_HIK.h"

FAnimInstanceProxy_HIK::FAnimInstanceProxy_HIK()
	: m_animInst(nullptr)
{
}

FAnimInstanceProxy_HIK::FAnimInstanceProxy_HIK(UAnimInstance_HIK* Instance)
	: Super(Instance)
	, m_animInst(Instance)
{
}

FAnimInstanceProxy_HIK::~FAnimInstanceProxy_HIK()
{

}

/** Called before update so we can copy any data we need */
void FAnimInstanceProxy_HIK::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	m_animInst->OnPreUpdate();
	Super::PreUpdate(InAnimInstance, DeltaSeconds);
}

/** Called after update so we can copy any data we need */
void FAnimInstanceProxy_HIK::PostUpdate(UAnimInstance* InAnimInstance) const
{
	Super::PostUpdate(InAnimInstance);
	m_animInst->OnPostUpdate();
}