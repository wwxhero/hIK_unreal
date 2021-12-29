#include "AnimInstanceProxy_MotionPipe.h"
#include "AnimInstance_MotionPipe.h"
#include "ik_logger_unreal.h"

FAnimInstanceProxy_MotionPipe::FAnimInstanceProxy_MotionPipe()
	: m_animInst(nullptr)
	, m_resetIK(false)
#ifdef _DEBUG
	, c_validPtr(404)
#endif
{
}

FAnimInstanceProxy_MotionPipe::FAnimInstanceProxy_MotionPipe(UAnimInstance_MotionPipe* Instance)
	: Super(Instance)
	, m_animInst(Instance)
	, m_resetIK(false)
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
	m_targets.Reset();
	m_animInst->OnPreUpdate(this);
	Super::PreUpdate(InAnimInstance, DeltaSeconds);
	m_tmEntity = GetSkelMeshCompLocalToWorld();
	check (m_tmEntity.Equals(GetSkelMeshCompOwnerTransform(), 0.001));
}

/** Called after update so we can copy any data we need */
void FAnimInstanceProxy_MotionPipe::PostUpdate(UAnimInstance* InAnimInstance) const
{
	Super::PostUpdate(InAnimInstance);
	m_animInst->OnPostUpdate(this);
}

