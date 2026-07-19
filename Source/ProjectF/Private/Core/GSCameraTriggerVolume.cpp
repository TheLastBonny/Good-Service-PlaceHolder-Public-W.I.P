#include "Core/GSCameraTriggerVolume.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Characters/GSPlayerController.h"
#include "GameFramework/Pawn.h"

AGSCameraTriggerVolume::AGSCameraTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);

	BlendTime = 0.5f;
	BlendFunction = VTBlend_Cubic;
	BlendExp = 2.0f;
}

void AGSCameraTriggerVolume::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AGSCameraTriggerVolume::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AGSCameraTriggerVolume::OnOverlapEnd);
}

void AGSCameraTriggerVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (Pawn->IsLocallyControlled())
		{
			if (AGSPlayerController* PC = Cast<AGSPlayerController>(Pawn->GetController()))
			{
				AActor* Target = CustomCameraTarget.IsValid() ? CustomCameraTarget.Get() : this;
				PC->PushCameraVolume(this, Target, BlendTime, BlendFunction, BlendExp);
			}
		}
	}
}

void AGSCameraTriggerVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (Pawn->IsLocallyControlled())
		{
			if (AGSPlayerController* PC = Cast<AGSPlayerController>(Pawn->GetController()))
			{
				PC->PopCameraVolume(this, BlendTime, BlendFunction, BlendExp);
			}
		}
	}
}
