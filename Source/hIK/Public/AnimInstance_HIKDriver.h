#pragma once
#include <list>
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_MotionPipe.h"
#include "Misc/ScopeTryLock.h"
#include "bvh.h"
#include "AnimInstance_HIKDriver.generated.h"

class UAnimInstance_HIKDrivee;

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDriver : public UAnimInstance_MotionPipe
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	int32 NUM_Frames_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	int32 I_Frame_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	TArray<UAnimInstance_HIKDrivee*> Drivees_;
public:
	UAnimInstance_HIKDriver()
		: NUM_Frames_(0)
		, I_Frame_(-1)
		, m_hBVH(H_INVALID)
	{}
	~UAnimInstance_HIKDriver() {}
private:

	struct Target : EndEF
	{
		FString name_eef_drivee;
	};

	void InitTarget(Target& tar, const EndEF& eef, const FString& name_eef_drivee)
	{
		tar.h_body = eef.h_body;
		tar.name = eef.name;
		tar.tm_l2w = eef.tm_l2w;
		tar.name_eef_drivee = name_eef_drivee;
	}

	struct FCompareTarget
	{
		FORCEINLINE bool operator()(const Target& A, const Target& B) const
		{
			FString nameA(A.name_eef_drivee);
			FString nameB(B.name_eef_drivee);
			return nameA < nameB;
		}
	};

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual FString GetFileConfName() const override;

	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy) override;
public:

	FORCEINLINE HBVH getBVH() const
	{
		return m_hBVH;
	}

private:
	const FString c_BVHFile;
	HBVH m_hBVH;
	TArray<Target> m_targets;


};
