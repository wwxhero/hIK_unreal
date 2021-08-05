#pragma once
#include <list>
#include "hIK.h"
#include "CoreMinimal.h"
#include "AnimInstance_HIK.h"
#include "Misc/ScopeTryLock.h"
#include "bvh.h"
#include "AnimInstance_HIKDriver.generated.h"

class UAnimInstance_HIKDrivee;

struct Target
{
	FString name_eef_drivee;
	FTransform tm_l2w;
};

struct FCompareTarget
{
	FORCEINLINE bool operator()(const Target& A, const Target& B) const
	{
		FString nameA(A.name_eef_drivee);
		FString nameB(B.name_eef_drivee);
		return nameA < nameB;
	}
};

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDriver : public UAnimInstance_HIK
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
	struct Target_BVH : Target
	{
		HBODY h_body;
	};

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual FString GetFileConfName() const override;

	virtual void OnPostUpdate(const FAnimInstanceProxy_HIK* proxy) override;
public:

	FORCEINLINE HBVH getBVH() const
	{
		return m_hBVH;
	}

private:
	const FString c_BVHFile;
	HBVH m_hBVH;
	TArray<Target_BVH> m_targets;


};
