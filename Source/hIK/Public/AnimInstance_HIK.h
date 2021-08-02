#pragma once
#include <map>
#include <set>
#include <string>
#include "Animation/AnimInstance.h"
#include "Misc/ScopeTryLock.h"
#include "articulated_body.h"
#include "conf_mopipe.h"
#include "AnimInstance_HIK.generated.h"

UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType, meta = (BlueprintThreadSafe), Within = SkeletalMeshComponent)
class HIK_API UAnimInstance_HIK : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstance_HIK();

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
	typedef struct
	{
		HBODY h_body;
		_TRANSFORM tm_l2w;
	} Target;

	FORCEINLINE bool CopyScale(int idx, const wchar_t* bone_name, float &s_x, float &s_y, float &s_z) const
	{
		bool b_match = false;

		int i_scale = 0;
		for (
			; i_scale < m_nScales[idx] && !b_match
			; i_scale++)
		{
			b_match = (0 == wcscmp(bone_name, m_scales[idx][i_scale].bone_name));
		}
		if (b_match)
		{
			s_x = m_scales[idx][i_scale - 1].scaleX;
			s_y = m_scales[idx][i_scale - 1].scaleY;
			s_z = m_scales[idx][i_scale - 1].scaleZ;
		}
		return b_match;
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

	FORCEINLINE int32 CopyTargets(std::set<std::wstring>& targets, int32 idx) const
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

	virtual void OnPreUpdate() const;
	virtual void OnPostUpdate();

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

protected:
	TArray<Target> m_targets;
};