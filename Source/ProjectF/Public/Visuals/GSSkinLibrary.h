#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "GSSkinLibrary.generated.h"

/**
 * Utility library for opening skin files in real-time, importing textures,
 * and handling raw image byte buffers for multiplayer skin synchronization.
 */
UCLASS()
class PROJECTF_API UGSSkinLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Opens a native Windows file selection dialog to choose a Minecraft skin image file (.png, .jpg).
	 * @param OutFilePath The absolute file path selected by the user.
	 * @return True if a valid file path was selected.
	 */
	UFUNCTION(BlueprintCallable, Category = "Visuals|Skin", meta = (Keywords = "file dialog open skin texture minecraft"))
	static bool OpenSkinFileDialog(FString& OutFilePath);

	/**
	 * Reads an image file from disk and imports it as a UTexture2D, also outputting the raw file bytes for networking.
	 * @param FilePath Absolute path to the skin image file.
	 * @param OutRawBytes Raw binary content of the file (for sending via RPC).
	 * @return Imported UTexture2D pointer or nullptr if failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Visuals|Skin", meta = (Keywords = "load import texture skin file"))
	static UTexture2D* LoadTextureFromFile(const FString& FilePath, TArray<uint8>& OutRawBytes);

	/**
	 * Converts a raw image byte buffer (e.g. PNG bytes received over network) into a UTexture2D.
	 * @param Buffer Compressed image file bytes.
	 * @return Generated UTexture2D pointer or nullptr if failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Visuals|Skin", meta = (Keywords = "load texture buffer bytes skin"))
	static UTexture2D* LoadTextureFromBuffer(const TArray<uint8>& Buffer);
};
