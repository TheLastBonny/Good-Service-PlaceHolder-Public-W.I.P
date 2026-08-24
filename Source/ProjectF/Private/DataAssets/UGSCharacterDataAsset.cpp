#include "DataAssets/UGSCharacterDataAsset.h"

UGSCharacterDataAsset::UGSCharacterDataAsset()
{
	MaterialSkinParameterName = FName("SkinTexture");
	MaterialIndex = 0;
}

UTexture2D* UGSCharacterDataAsset::GetRandomOrDefaultSkinTexture() const
{
	TArray<TSoftObjectPtr<UTexture2D>> ValidSkins;
	for (const TSoftObjectPtr<UTexture2D>& SoftTexture : SkinTexturePool)
	{
		if (!SoftTexture.IsNull())
		{
			ValidSkins.Add(SoftTexture);
		}
	}

	if (ValidSkins.Num() > 0)
	{
		int32 RandomIdx = FMath::RandRange(0, ValidSkins.Num() - 1);
		return ValidSkins[RandomIdx].LoadSynchronous();
	}

	if (!DefaultSkinTexture.IsNull())
	{
		return DefaultSkinTexture.LoadSynchronous();
	}

	return nullptr;
}
