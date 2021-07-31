// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_HIKDrivee.h"


/////////////////////////////////////////////////////
// UAnimGraphNode_HIKDrivee

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_HIKDrivee::UAnimGraphNode_HIKDrivee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_HIKDrivee::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString("HIK"));
}

FLinearColor UAnimGraphNode_HIKDrivee::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

FText UAnimGraphNode_HIKDrivee::GetTooltipText() const
{
	return FText::FromString(FString("A hybrid fullbody IK solution"));
}



FString UAnimGraphNode_HIKDrivee::GetNodeCategory() const
{
	return TEXT("IK Nodes");
}

void UAnimGraphNode_HIKDrivee::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	Super::CustomizePinData(Pin, SourcePropertyName, ArrayIndex);

	// if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_HIKDrivee, Pitch))
	// {
	// 	if (!Pin->bHidden)
	// 	{
	// 		Pin->PinFriendlyName = Node.PitchScaleBiasClamp.GetFriendlyName(Pin->PinFriendlyName);
	// 	}
	// }

	// if (Pin->PinName == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_HIKDrivee, Yaw))
	// {
	// 	if (!Pin->bHidden)
	// 	{
	// 		Pin->PinFriendlyName = Node.YawScaleBiasClamp.GetFriendlyName(Pin->PinFriendlyName);
	// 	}
	// }
}

void UAnimGraphNode_HIKDrivee::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None);

	// Reconstruct node to show updates to PinFriendlyNames.
	if ((PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, bMapRange))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputRange, Min))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputRange, Max))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, Scale))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, Bias))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, bClampResult))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, ClampMin))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, ClampMax))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, bInterpResult))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, InterpSpeedIncreasing))
		|| (PropertyName == GET_MEMBER_NAME_STRING_CHECKED(FInputScaleBiasClamp, InterpSpeedDecreasing)))
	{
		ReconstructNode();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#undef LOCTEXT_NAMESPACE
