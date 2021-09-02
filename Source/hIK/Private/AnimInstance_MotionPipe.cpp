#include "AnimInstance_MotionPipe.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "ik_logger_unreal.h"

UAnimInstance_MotionPipe::UAnimInstance_MotionPipe()
{}

void UAnimInstance_MotionPipe::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UAnimInstance_MotionPipe::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}


FAnimInstanceProxy* UAnimInstance_MotionPipe::CreateAnimInstanceProxy()
{
	FAnimInstanceProxy* ret = new FAnimInstanceProxy_MotionPipe(this);
//	LOGIKVar(LogInfoPtr, ret);
	return ret;
}



