// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorIKDriveeErrVis.h"
#include "ik_logger_unreal.h"

AActorIKDriveeErrVis::AActorIKDriveeErrVis()
	: m_boneGSel(-1)
{
	static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/Game/StarterContent/Makehuman_163/Materials/vertex_color.vertex_color'"));
	if (nullptr != Material.Object)
		m_materialVertexClr = (UMaterial*)Material.Object;
	else
		m_materialVertexClr = nullptr;
}

void AActorIKDriveeErrVis::BeginPlay()
{
	Super::BeginPlay();
	UpdateBoneVis(m_boneGSel);
}

void AActorIKDriveeErrVis::UpdateBoneVis(int32 boneID_g)
{
	USkeletalMeshComponent* meshComp = GetSkeletalMeshComponent();
	int n_materials = meshComp->GetNumMaterials();

	for (int i_material = 0; i_material < n_materials; i_material++)
	{
		meshComp->SetMaterial(i_material, m_materialVertexClr);
	}

	// LOGIKVar(LogInfoInt, meshComp->GetNumBones());
	// failed to verify the functions: FSkinWeightVertexBuffer::GetBoneIndex, FSkinWeightVertexBuffer::GetBoneWeight
	FSkinWeightVertexBuffer* buffer = nullptr;
	int32 lod = 0;
	for (
		; nullptr == buffer
		; lod++)
		buffer = meshComp->GetSkinWeightBuffer(lod);

	if (buffer)
	{
		TArray<FColor> clrVert;
		clrVert.Init(FColor::Black, buffer->GetNumVertices());
		for (uint32 i_v = 0; i_v < buffer->GetNumVertices(); i_v++)
		{
			FLinearColor clrVert_i = FLinearColor::Black;
			for (int i_w = 0; i_w < MAX_TOTAL_INFLUENCES; i_w++) //
			{
				int32 i_bone = buffer->GetBoneIndex(i_v, i_w);
				// const int8 threshold = (int8)(0.1f*256.0f);
				if (0 != i_bone)
				{
				// FName name_i = meshComp->GetBoneName(i_bone);
				// LOGIKVar(LogInfoWCharPtr, *name_i.ToString());
				// LOGIKVar(LogInfoInt, buffer->GetBoneIndex(i_v, i_w));
				// LOGIKVar(LogInfoInt, buffer->GetBoneWeight(i_v, i_w));
				}
				if (i_bone == boneID_g)
				{
					int8 weight_i = buffer->GetBoneWeight(i_v, i_w);
					float weight_f = weight_i/256.0f;
					FLinearColor addcolor_i = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, weight_f);
					clrVert_i += addcolor_i.GetClamped();
				}
			}
			clrVert[i_v] = clrVert_i.GetClamped().ToFColor(true);
		}

		LOGIKVar(LogInfoInt, boneID_g);
		meshComp->SetVertexColorOverride(lod-1, clrVert);
	}

	// LOGIKVar(LogInfoPtr, buffer);
	// LOGIKVar(LogInfoInt, lod);
	// LOGIKVar(LogInfoInt, buffer->GetNumVertices());


	// // proved wrong:

	// int n_bones = meshComp->GetNumBones();
	// // TArray<int32> boneIdx; boneIdx.SetNum(n_bones);
	// TArray<int32> vcount;
	// vcount.Init(0, n_bones);

	// for (uint32 i_v = 0; i_v < buffer->GetNumVertices(); i_v++)
	// {
	//  	for (int i_w = 0; i_w < 4; i_w++) //MAX_TOTAL_INFLUENCES
	//  	{
	//  		int i_bone = buffer->GetBoneIndex(i_v, i_w);
	//  		if (0 != i_bone)
	//  		{
	//  			// FName name_i = meshComp->GetBoneName(i_bone);
	//  			// LOGIKVar(LogInfoWCharPtr, *name_i.ToString());
	//  			// LOGIKVar(LogInfoInt, buffer->GetBoneIndex(i_v, i_w));
	//  			// LOGIKVar(LogInfoInt, buffer->GetBoneWeight(i_v, i_w));
	//  		}
	// 		vcount[i_bone] ++;
	//  	}
	// }

	// for (int i_bone = 0; i_bone < n_bones; i_bone++)
	// {
	// 	FName name_i = meshComp->GetBoneName(i_bone);
	// 	LOGIKVar(LogInfoWCharPtr, *name_i.ToString());
	// 	LOGIKVar(LogInfoInt, vcount[i_bone]);
	// }

	// // end of proof






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
