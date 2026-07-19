#include "Items/GSItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "DataAssets/GSItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Components/GSGrabbableComponent.h"
#include "Machines/GSUtilityStation.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"

AGSItem::AGSItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	bShowDebugLogs = false;
}



UAbilitySystemComponent* AGSItem::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSItem::BeginPlay()
{
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	
	if (HasAuthority())
	{
		SetReplicates(true);
		SetReplicateMovement(true);
	}


	if (USceneComponent* ItemRoot = GetRootComponent())
	{
		ItemRoot->SetAbsolute(false, false, true);
	}

	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);


		if (ItemData)
		{
			for (const TSubclassOf<UAttributeSet>& SetClass : ItemData->AttributeSets)
			{
				if (SetClass && !AttributeSets.Contains(SetClass))
				{
					AttributeSets.Add(SetClass);
				}
			}
		}


		for (const TSubclassOf<UAttributeSet>& SetClass : AttributeSets)
		{
			if (SetClass)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}
		}


		if (ItemData)
		{

			if (ItemData->DefaultTags.Num() > 0)
			{
				AbilitySystemComponent->AddLooseGameplayTags(ItemData->DefaultTags, 1, EGameplayTagReplicationState::TagOnly);
			}


			for (const TPair<FGameplayTag, FGSItemStateDetails>& StatePair : ItemData->ItemStatesMap)
			{
				const FGameplayTag& StateTag = StatePair.Key;
				const FGSItemStateDetails& Details = StatePair.Value;

				if (Details.MaxProgressAttribute.IsValid())
				{
					AbilitySystemComponent->SetNumericAttributeBase(Details.MaxProgressAttribute, Details.MaxProgressValue);
				}


				AbilitySystemComponent->RegisterGameplayTagEvent(StateTag, EGameplayTagEventType::NewOrRemoved)
					.AddUObject(this, &AGSItem::OnStateTagChanged);
			}
		}


		if (ItemTags.Num() > 0)
		{
			AbilitySystemComponent->AddLooseGameplayTags(ItemTags, 1, EGameplayTagReplicationState::TagOnly);
		}
	}
}

void AGSItem::OnStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (!ItemData) 
	{
		
		return; 
	}

	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	const FGSItemStateDetails* Details = ItemData->ItemStatesMap.Find(CallbackTag);
	if (Details)
	{
		FString StateNameStr = Details->StateName.ToString();
		if (StateNameStr.IsEmpty()) { StateNameStr = CallbackTag.ToString(); }

		

		if (NewCount > 0)
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GAS Item] %s ha entrado al estado: %s"), *GetName(), *StateNameStr);
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("%s: ¡%s!"), *GetName(), *StateNameStr));
				}
			}


			for (UGSItemStateAction* Action : Details->Actions)
			{
				if (Action)
				{
					Action->Execute(this);
				}
			}
		}
		else
		{
			if (bShowDebugLogs)
			{
				UE_LOG(LogTemp, Log, TEXT("[GAS Item] %s ha salido del estado: %s"), *GetName(), *StateNameStr);
			}
		}
	}
	else
	{
		
	}
}

void AGSItem::OnRep_AttachmentReplication()
{
	Super::OnRep_AttachmentReplication();

	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	

	if (GetAttachParentActor())
	{

		if (USceneComponent* ItemRoot = GetRootComponent())
		{
			ItemRoot->SetAbsolute(false, false, true);
		}

		UGSGrabbableComponent* GrabComp = FindComponentByClass<UGSGrabbableComponent>();
		if (GrabComp)
		{

			GrabComp->OnRep_KinematicFlightParams();
		}

		FName SocketName = GetRootComponent() ? GetRootComponent()->GetAttachSocketName() : NAME_None;
		

		if (SocketName.IsNone() || SocketName == TEXT("None"))
		{
			float HeightOffset = 120.0f;
			if (GrabComp)
			{
				HeightOffset = GrabComp->FallbackAboveHeadHeight;
			}
			FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, HeightOffset));
			SetActorRelativeTransform(AboveHeadTransform);
		}
		else
		{

			FTransform TargetRelativeTransform = FTransform::Identity;
			

			if (!GetAttachParentActor()->IsA(AGSUtilityStation::StaticClass()))
			{
				if (GrabComp)
				{
					TargetRelativeTransform = GrabComp->RelativeTransform;
				}
			}
			
			SetActorRelativeTransform(TargetRelativeTransform);
		}
	}
}

void AGSItem::OnRep_ReplicatedMovement()
{

	if (GetAttachParentActor())
	{
		
		return;
	}

	Super::OnRep_ReplicatedMovement();
}
