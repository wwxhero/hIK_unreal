#pragma once
#include <map>
#include <set>
#include <string>
#include "Animation/AnimInstance.h"
#include "Misc/ScopeTryLock.h"
#include "articulated_body.h"
#include "conf_mopipe.h"
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
	virtual void OnPreUpdate(FAnimInstanceProxy_MotionPipe* proxy) const { };
	virtual void OnPostUpdate(const FAnimInstanceProxy_MotionPipe* proxy) { };

public:
	FORCEINLINE void CopyScale(int idx, std::map<FString, FVector>& name2scale) const
	{
		int n_scales_i = m_nScales[idx];
		B_Scale* scales_i = m_scales[idx];
		for (int j_scale = 0; j_scale < n_scales_i; j_scale ++)
		{
			const B_Scale& scale_ij = scales_i[j_scale];
			name2scale[scale_ij.bone_name] = FVector(scale_ij.scaleX, scale_ij.scaleY, scale_ij.scaleZ);
		}
	}

	FORCEINLINE void CopySrc2Dst_w(float m[3][3]) const
	{
		m[0][0] = m_bvh2fbxWorld[0][0]; m[0][1] = m_bvh2fbxWorld[0][1]; m[0][2] = m_bvh2fbxWorld[0][2];
		m[1][0] = m_bvh2fbxWorld[1][0]; m[1][1] = m_bvh2fbxWorld[1][1]; m[1][2] = m_bvh2fbxWorld[1][2];
		m[2][0] = m_bvh2fbxWorld[2][0]; m[2][1] = m_bvh2fbxWorld[2][1]; m[2][2] = m_bvh2fbxWorld[2][2];
	}

	FORCEINLINE void CopySrc2Dst_w(FMatrix& m) const
	{
		FMatrix bvh2unrel_m = {
		 	{m_bvh2fbxWorld[0][0],		m_bvh2fbxWorld[1][0],		m_bvh2fbxWorld[2][0],	0},
		 	{m_bvh2fbxWorld[0][1],		m_bvh2fbxWorld[1][1],		m_bvh2fbxWorld[2][1],	0},
		 	{m_bvh2fbxWorld[0][2],		m_bvh2fbxWorld[1][2],		m_bvh2fbxWorld[2][2],	0},
		 	{0,							0,							0,						1},
		};
		m = bvh2unrel_m;
	}

	FORCEINLINE int CopyMatches(const wchar_t* (**match)[2]) const
	{
		*match = m_match;
		return m_nMatches;
	}

	FORCEINLINE int32 CopyMatches(std::map<std::wstring, std::wstring> matches[2]) const
	{
		for (int i_match = 0
			; i_match < m_nMatches
			; i_match++)
		{
			auto& name_src_i = m_match[i_match][0];
			auto& name_dst_i = m_match[i_match][1];
			matches[0][name_src_i] = name_dst_i;
			matches[1][name_dst_i] = name_src_i;
		}
		return (int32)m_nMatches;
	}

	FORCEINLINE int32 CopyMatches(std::map<FString, FString>& matches, int32 src_i) const
	{
		int idx_src = src_i;
		int idx_dst = !src_i;

		for (int i_match = 0
			; i_match < m_nMatches
			; i_match++)
		{
			auto& name_src_i = m_match[i_match][idx_src];
			auto& name_dst_i = m_match[i_match][idx_dst];
			matches[name_src_i] = name_dst_i;
		}
		return (int32)m_nMatches;
	}

	FORCEINLINE int32 CopyMatches(std::set<FString> &matches_i, int32 idx) const
	{
		for (int i_match = 0
			; i_match < m_nMatches
			; i_match++)
		{
			matches_i.insert(m_match[i_match][idx]);
		}
		return (int32)m_nMatches;
	}

	FORCEINLINE int32 CopyEEFs(std::set<FString>& targets, int32 idx) const
	{
		auto n_targets_i = m_nTargets[idx];
		auto targetnames_i = m_targetnames[idx];
		for (int i_target = 0; i_target < n_targets_i; i_target++)
			targets.insert(targetnames_i[i_target]);
		return (int32)n_targets_i;
	}

	FORCEINLINE void CopyFileNames(const wchar_t* filenames[2]) const
	{
		filenames[0] = m_filenames[0];
		filenames[1] = m_filenames[1];
	}

protected:
	HCONF m_hConf;

	// conf variables:
	const wchar_t* (*m_match)[2];
	int m_nMatches;
	float m_bvh2fbxWorld[3][3];
	B_Scale *m_scales[2];
	int m_nScales[2];
	const wchar_t** m_targetnames[2];
	int m_nTargets[2];
	const wchar_t* m_filenames[2];
	// end of conf variables


};