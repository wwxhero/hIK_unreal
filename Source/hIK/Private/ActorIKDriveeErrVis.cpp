// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriveeErrVis.h"
#include "ik_logger_unreal.h"
#include "EngineUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"

AActorIKDriveeErrVis::AActorIKDriveeErrVis()
	: m_driver(nullptr)
	, m_errS(1.0)
{
	static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/StarterContent/Makehuman_163/Materials/vertex_color.vertex_color'"));
	if (nullptr != Material.Object)
		m_materialVertexClr = (UMaterial*)Material.Object;
	else
		m_materialVertexClr = nullptr;
}

void AActorIKDriveeErrVis::Connect(AActor* driver)
{
	m_driver = Cast<AActorIKDriver, AActor>(driver);
	check(nullptr != m_driver);
	UpdateBoneVis();
}

void AActorIKDriveeErrVis::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds < 0.017)
		UpdateBoneVis();
}

FSkinWeightVertexBuffer* AActorIKDriveeErrVis::GetSkinWeightBuffer(const USkinnedMeshComponent* pThis, int32 LODIndex)
{
	FSkinWeightVertexBuffer* WeightBuffer = nullptr;
	USkeletalMesh* SkeletalMesh = pThis->SkeletalMesh;
	if (SkeletalMesh &&
		SkeletalMesh->GetResourceForRendering() &&
		SkeletalMesh->GetResourceForRendering()->LODRenderData.IsValidIndex(LODIndex) )
	{
		FSkeletalMeshLODRenderData& LODData = SkeletalMesh->GetResourceForRendering()->LODRenderData[LODIndex];
		auto LODInfo = pThis->LODInfo;
		// Grab weight buffer (check for override)
		if (LODInfo.IsValidIndex(LODIndex) &&
			LODInfo[LODIndex].OverrideSkinWeights &&
			LODInfo[LODIndex].OverrideSkinWeights->GetNumVertices() == LODData.GetNumVertices())
		{
			WeightBuffer = LODInfo[LODIndex].OverrideSkinWeights;
		}
		else if (LODInfo.IsValidIndex(LODIndex) &&
			LODInfo[LODIndex].OverrideProfileSkinWeights &&
			LODInfo[LODIndex].OverrideProfileSkinWeights->GetNumVertices() == LODData.GetNumVertices())
		{
			WeightBuffer = LODInfo[LODIndex].OverrideProfileSkinWeights;
		}
		else
		{
			WeightBuffer = LODData.GetSkinWeightVertexBuffer();
		}
	}

	return WeightBuffer;
}

//E: Q x Q -> [0, 1]
float AActorIKDriveeErrVis::Err_q(const FQuat& q_0, const FQuat& q_1) const
{
	float cos_half_alpha = FMath::Abs( q_0.X * q_1.X
								+ q_0.Y * q_1.Y
								+ q_0.Z * q_1.Z
								+ q_0.W * q_1.W );
	// [0, pi/2] -> [1, 0]
	return 1 - cos_half_alpha;
}

void AActorIKDriveeErrVis::UpdateBoneVis()
{
	// int32 boneID_k = 10;
	USkeletalMeshComponent* meshComp = GetSkeletalMeshComponent();
	int n_materials = meshComp->GetNumMaterials();
	auto RD = meshComp->GetSkeletalMeshRenderData();
	for (int i_material = 0; i_material < n_materials; i_material++)
	{
		meshComp->SetMaterial(i_material, m_materialVertexClr);
	}

	int32 n_kbones = meshComp->GetNumBones();
	TArray<float> k2err;
	k2err.Init(m_errS, n_kbones);
	USkeletalMeshComponent* meshSK0 = m_driver->GetMesh();
	TArray<FTransform> tm0 = meshSK0->GetBoneSpaceTransforms();
	USkeletalMeshComponent* meshSK = GetSkeletalMeshComponent();
	TArray<FTransform> tm = meshSK->GetBoneSpaceTransforms();
	check(tm0.Num() == n_kbones
		&& tm.Num() == n_kbones);
	for (int32 i_kbone = 0; i_kbone < n_kbones; i_kbone++)
	{
		k2err[i_kbone] = Err_q(tm0[i_kbone].GetRotation(), tm[i_kbone].GetRotation()) * m_errS;
		// k2err[i_kbone] = 0.0f;
	}


	FSkinWeightVertexBuffer* buffer = nullptr;
	const FSkeletalMeshLODRenderData* renderData = nullptr;
	int32 lod = 0;
	for (
		; nullptr == buffer
		; lod++)
	{
		buffer = meshComp->GetSkinWeightBuffer(lod);
		renderData = &(RD->LODRenderData[lod]);
	}

	if (buffer
		&& renderData)
	{
		LOGIKVar(LogInfoInt, buffer->GetNumVertices());
		TArray<FColor> clrVert;
		clrVert.Init(FColor::Black, buffer->GetNumVertices());

		for (const auto& sec_i : renderData->RenderSections)
		{
			const auto& map_g2k = sec_i.BoneMap;
			uint32 i_v_start = sec_i.BaseVertexIndex;
			uint32 i_v_end = i_v_start + sec_i.NumVertices;
			for (uint32 i_v = i_v_start; i_v < i_v_end; i_v++)
			{
				FLinearColor clrVert_i = FLinearColor::Black;
				for (uint32 i_influence = 0; i_influence < buffer->GetMaxBoneInfluences(); i_influence++)
				{
					int32 id_g = buffer->GetBoneIndex(i_v, i_influence);
					auto id_k = map_g2k[id_g];
					uint8 weight_i = buffer->GetBoneWeight(i_v, i_influence);
					float weight_f = ((float)weight_i)/255.0f;
					// FLinearColor addcolor_i = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, weight_f);
					FLinearColor addcolor_i = FMath::Lerp(FLinearColor::Blue, FLinearColor::Red, k2err[id_k])*weight_f;
					clrVert_i = (clrVert_i + addcolor_i.GetClamped()).GetClamped();
				}
				clrVert[i_v] = clrVert_i.GetClamped().ToFColor(false);
			}
		}

		meshComp->SetVertexColorOverride(lod-1, clrVert);
	}
	// const FSkinWeightDataVertexBuffer* bufferData = buffer->GetDataVertexBuffer();
	// LOGIKVar(LogInfoInt, bufferData->GetNumVertices());
	// LOGIKVar(LogInfoInt, bufferData->GetNumBones());
	// // LOGIKVar(LogInfoBool, bufferData->IsWeightDataValid());
	// // LOGIKVar(LogInfoBool, bufferData->IsWeightDataValid());
	// FSkinWeightInfo* winfo = bufferData->GetWeightData();
	// for (uint32 i_v = 0; i_v < bufferData->GetNumVertices(); i_v++)
	// {
	// 	LOGIKVar(LogInfoInt, i_v);
	// 	const FSkinWeightInfo& winfo_i = winfo[i_v];
	// 	for (int i_w = 0; i_w < MAX_TOTAL_INFLUENCES; i_w++)
	// 	{
	// 		LOGIKVar(LogInfoInt, winfo_i.InfluenceBones[i_w]);
	// 		LOGIKVar(LogInfoInt, winfo_i.InfluenceWeights[i_w]);
	// 	}
	// }
}


void AActorIKDriveeErrVis::VisBoneNext()
{
	m_errS += 0.1f;
	LOGIKVar(LogInfoFloat, m_errS);
	UpdateBoneVis();
}

void AActorIKDriveeErrVis::VisBonePrev()
{
	m_errS -= 0.1f;
	LOGIKVar(LogInfoFloat, m_errS);
	UpdateBoneVis();
}
