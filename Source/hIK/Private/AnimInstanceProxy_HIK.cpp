#include "AnimInstanceProxy_HIK.h"
#include "AnimInstance_HIK.h"
#include "ik_logger.h"

FAnimInstanceProxy_HIK::FAnimInstanceProxy_HIK()
	: m_animInst(nullptr)
#ifdef _DEBUG
	, c_validPtr(404)
#endif
{
}

FAnimInstanceProxy_HIK::FAnimInstanceProxy_HIK(UAnimInstance_HIK* Instance)
	: Super(Instance)
	, m_animInst(Instance)
#ifdef _DEBUG
	, c_validPtr(404)
#endif
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
	m_animInst->OnPostUpdate(this);
}

void FAnimInstanceProxy_HIK::AddEEF(HBODY hBody)
{
	LOGIKVar(LogInfoCharPtr, body_name_c(hBody));
}