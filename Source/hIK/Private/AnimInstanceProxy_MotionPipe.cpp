#include "AnimInstanceProxy_MotionPipe.h"
#include "AnimInstance_MotionPipe.h"
#include "ik_logger_unreal.h"

FAnimInstanceProxy_MotionPipe::FAnimInstanceProxy_MotionPipe()
	: m_animInst(nullptr)
#ifdef _DEBUG
	, c_validPtr(404)
#endif
{
}

FAnimInstanceProxy_MotionPipe::FAnimInstanceProxy_MotionPipe(UAnimInstance_MotionPipe* Instance)
	: Super(Instance)
	, m_animInst(Instance)
#ifdef _DEBUG
	, c_validPtr(404)
#endif
{
}

FAnimInstanceProxy_MotionPipe::~FAnimInstanceProxy_MotionPipe()
{

}

/** Called before update so we can copy any data we need */
void FAnimInstanceProxy_MotionPipe::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	m_endEEFs.Reset();
	m_animInst->OnPreUpdate();
	Super::PreUpdate(InAnimInstance, DeltaSeconds);
}

/** Called after update so we can copy any data we need */
void FAnimInstanceProxy_MotionPipe::PostUpdate(UAnimInstance* InAnimInstance) const
{
	Super::PostUpdate(InAnimInstance);
	m_animInst->OnPostUpdate(this);
}

void FAnimInstanceProxy_MotionPipe::RegisterEEF(HBODY hBody)
{
	LOGIKVar(LogInfoCharPtr, body_name_c(hBody));
	EndEF endeff;
	InitEndEF(&endeff, hBody);
	m_endEEFs.Add(endeff);
}