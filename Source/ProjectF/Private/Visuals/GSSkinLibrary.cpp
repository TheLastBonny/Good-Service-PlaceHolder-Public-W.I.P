#include "Visuals/GSSkinLibrary.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"

#if WITH_EDITOR || PLATFORM_DESKTOP
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#endif

bool UGSSkinLibrary::OpenSkinFileDialog(FString& OutFilePath)
{
	OutFilePath.Empty();

#if WITH_EDITOR || PLATFORM_DESKTOP
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		TArray<FString> OpenFilenames;
		const FString Title = TEXT("Seleccionar Skin (.png)");
		const FString FileTypes = TEXT("Imágenes (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|Todos los archivos (*.*)|*.*");

		bool bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowHandle,
			Title,
			TEXT(""),
			TEXT(""),
			FileTypes,
			EFileDialogFlags::None,
			OpenFilenames
		);

		if (bOpened && OpenFilenames.Num() > 0)
		{
			OutFilePath = OpenFilenames[0];
			return true;
		}
	}
#endif

	return false;
}

UTexture2D* UGSSkinLibrary::LoadTextureFromFile(const FString& FilePath, TArray<uint8>& OutRawBytes)
{
	OutRawBytes.Empty();

	if (FilePath.IsEmpty())
	{
		return nullptr;
	}

	if (!FFileHelper::LoadFileToArray(OutRawBytes, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GSSkinLibrary] Error al leer el archivo en ruta: %s"), *FilePath);
		return nullptr;
	}

	return LoadTextureFromBuffer(OutRawBytes);
}

UTexture2D* UGSSkinLibrary::LoadTextureFromBuffer(const TArray<uint8>& Buffer)
{
	if (Buffer.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GSSkinLibrary] El buffer de imagen está vacío."));
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	EImageFormat Format = ImageWrapperModule.DetectImageFormat(Buffer.GetData(), Buffer.Num());
	if (Format == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GSSkinLibrary] Formato de imagen no válido."));
		return nullptr;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
	if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(Buffer.GetData(), Buffer.Num()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GSSkinLibrary] Falló al establecer datos comprimidos de imagen."));
		return nullptr;
	}

	TArray<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GSSkinLibrary] Falló al obtener datos RAW de la imagen."));
		return nullptr;
	}

	int32 Width = ImageWrapper->GetWidth();
	int32 Height = ImageWrapper->GetHeight();

	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!NewTexture)
	{
		return nullptr;
	}

#if WITH_EDITORONLY_DATA
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
#endif
	NewTexture->SRGB = true;

	void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

	NewTexture->UpdateResource();
	return NewTexture;
}
