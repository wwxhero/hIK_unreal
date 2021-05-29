// Copyright (c) Mathew Wang 2021

#include "AnimGraphNode_FKRecordUT.h"

FText UAnimGraphNode_FKRecordUT::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString("FK Record UT"));
}

FLinearColor UAnimGraphNode_FKRecordUT::GetNodeTitleColor() const
{
	return FLinearColor(0, 1, 1, 1);
}

FString UAnimGraphNode_FKRecordUT::GetNodeCategory() const
{
	return FString("FK Nodes");
}

FText UAnimGraphNode_FKRecordUT::GetControllerDescription() const
{
	return FText::FromString(FString("Access record for unit test"));
}
