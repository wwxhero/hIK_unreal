#pragma once
#include <map>
#include <set>
#include <string>
#include "Animation/AnimInstance.h"
#include "Misc/ScopeTryLock.h"
#include "articulated_body.h"
#include "transform_helper.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "AnimInstance_MotionPipe.generated.h"


UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_MotionPipe : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstance_MotionPipe();

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	/** Override point for derived classes to create their own proxy objects (allows custom allocation) */
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy();

	virtual FString GetFileConfName() const
	{
		check(0); // this function need be implemented by the derived classes
		return FString(L"");
	}
public:
	FORCEINLINE FString GetFileConfPath() const
	{
		FString rootDir = FPaths::ProjectDir();
		FString filePathFull = rootDir + GetFileConfName();
		return filePathFull;
	}
	virtual void OnPreUpdate(FAnimInstanceProxy_MotionPipe* proxy) const { };
	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy) { };

};