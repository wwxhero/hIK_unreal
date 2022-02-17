// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriveeErrVis.h"
#include "ik_logger_unreal.h"
#include "EngineUtils.h"
#include "Rendering/SkeletalMeshRenderData.h"

AActorIKDriveeErrVis::AActorIKDriveeErrVis()
	: m_boneGSel(-1)
	, m_driver(nullptr)
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
	UpdateBoneVis(m_boneGSel);
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

void AActorIKDriveeErrVis::UpdateBoneVis(int32 boneID_k)
{
	USkeletalMeshComponent* meshComp = GetSkeletalMeshComponent();
	int n_materials = meshComp->GetNumMaterials();
	auto RD = meshComp->GetSkeletalMeshRenderData();
	for (int i_material = 0; i_material < n_materials; i_material++)
	{
		meshComp->SetMaterial(i_material, m_materialVertexClr);
	}

	int n_kbones = meshComp->GetNumBones();
	TArray<FLinearColor> k2clr_l;
	TArray<FColor> k2clr;
	k2clr_l.Init(FLinearColor::Black, n_kbones);
	k2clr.Init(FColor::Black, n_kbones);
	if (-1 < boneID_k
	 && boneID_k < n_kbones)
	{
		k2clr_l[boneID_k] = FLinearColor::White;
		k2clr[boneID_k] = FColor::White;
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
					FLinearColor addcolor_i = k2clr_l[id_k]*weight_f;
					clrVert_i = (clrVert_i + addcolor_i.GetClamped()).GetClamped();
				}
				clrVert[i_v] = clrVert_i.GetClamped().ToFColor(false);
			}
		}

		LOGIKVar(LogInfoInt, boneID_k);
		FName name_i = meshComp->GetBoneName(boneID_k);
		LOGIKVar(LogInfoWCharPtr, *name_i.ToString());
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
	m_boneGSel ++;
	UpdateBoneVis(m_boneGSel);
}

void AActorIKDriveeErrVis::VisBonePrev()
{
	m_boneGSel --;
	UpdateBoneVis(m_boneGSel);
}
