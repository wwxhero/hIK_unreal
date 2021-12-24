#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_MotionPipe.h"
#include "Misc/ScopeTryLock.h"
#include "AnimInstance_HIKDrivee.generated.h"

UENUM()
enum TRACKER_ID
{
	RH = 0	UMETA(DisplayName = "RightHand"),
	LH		UMETA(DisplayName = "LeftHand"),
	RF		UMETA(DisplayName = "RightFoot"),
	LF		UMETA(DisplayName = "LeftFoot"),
	PW		UMETA(DisplayName = "Pelvis"),
	HMD		UMETA(DisplayName = "Head"),
	N_TRACKERS = HMD,
	Unknown
};

struct FVRTrackingBind
{
	TRACKER_ID trID;
	int32	tarID;
	FTransform tar2tr;
};

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDrivee : public UAnimInstance_MotionPipe
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	int32 DBG_VisBody_i;

public:
	UAnimInstance_HIKDrivee();

	~UAnimInstance_HIKDrivee()
	{
	}

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	virtual void PreUpdateAnimation(float DeltaSeconds) override;


	FORCEINLINE void PushUpdateTargets(const TArray<Target>& targets)
	{
		m_ikConnected = true;
		m_targets = targets;
	}

	void VRIK_Connect(const TArray<USceneComponent*>& trackers);
	void VRIK_Update();

private:
	virtual void OnPreUpdate(FAnimInstanceProxy_MotionPipe* proxy) const override;
	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy) override;

private:
	TArray<Target> m_targets;

	TArray<FVRTrackingBind> m_bindings;
	TArray<USceneComponent*> m_trackers;
	bool m_ikConnected;
};
