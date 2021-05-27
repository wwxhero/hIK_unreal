// Copyright (c) Mathew Wang 2021

#include "AnimGraphNode_HIKTest.h"

FText UAnimGraphNode_HIKTest::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString("Humanoid HIK Test"));
}

FLinearColor UAnimGraphNode_HIKTest::GetNodeTitleColor() const
{
	return FLinearColor(0, 1, 1, 1);
}

FString UAnimGraphNode_HIKTest::GetNodeCategory() const
{
	return FString("IK Nodes");
}

FText UAnimGraphNode_HIKTest::GetControllerDescription() const
{
	return FText::FromString(FString("Traces from the leg to the floor, providing trace data used later in IK"));
}
