// Copyright (c) 2026 Bonny. All rights reserved.
#include "DataAssets/GSItemDataAsset.h"

UGSItemDataAsset::UGSItemDataAsset()
{
}

FGSItemStateDetails UGSItemDataAsset::GetStateDetails(FGameplayTag StateTag) const
{
	const FGSItemStateDetails* Details = ItemStatesMap.Find(StateTag);
	if (Details)
	{
		return *Details;
	}
	return FGSItemStateDetails();
}
