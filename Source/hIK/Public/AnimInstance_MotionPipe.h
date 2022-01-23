#pragma once
#include <map>
#include <set>
#include "Animation/AnimInstance.h"
#include "Misc/ScopeTryLock.h"
#include "articulated_body.h"
#include "transform_helper.h"
#include "AnimInstanceProxy_MotionPipe.h"
#include "AnimInstance_MotionPipe.generated.h"

const float c_epsilon = 1e-5f;

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_MotionPipe : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstance_MotionPipe();
public:
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault))
	FString FileConfName;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	/** Override point for derived classes to create their own proxy objects (allows custom allocation) */
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy();

	FString GetFileConfName() const
	{
		return FileConfName;
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