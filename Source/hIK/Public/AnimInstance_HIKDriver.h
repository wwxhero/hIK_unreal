#pragma once
#include "hIK.h"
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "bvh.h"
#include "fk_drv_conf.h"
#include "AnimInstance_HIKDriver.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIKDriver : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Settings, meta = (PinShownByDefault))
	int32 NUM_Frames_;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
	int32 I_Frame_;
public:
	UAnimInstance_HIKDriver()
		: NUM_Frames_(0)
		, I_Frame_(-1)
		, c_BVHFile(L"01_01_1_s6.bvh")
		, m_hBVH(H_INVALID)
		, c_DRVConfFile(L"FKRecordUT.xml")
		, m_hDrvConf(H_INVALID)
	{}
	~UAnimInstance_HIKDriver() {}
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;

	const FString c_BVHFile;
	HBVH m_hBVH;
	const FString c_DRVConfFile;
	HCONF m_hDrvConf;
};
