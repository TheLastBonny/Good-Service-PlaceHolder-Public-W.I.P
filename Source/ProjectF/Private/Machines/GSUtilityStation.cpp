#include "Machines/GSUtilityStation.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Core/GSGameplayTags.h"
#include "DataAssets/GSStationDataAsset.h"

AGSUtilityStation::AGSUtilityStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	StationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("StationVolume"));
	RootComponent = StationVolume;
	StationVolume->SetCollisionProfileName(TEXT("Trigger"));
	StationVolume->SetGenerateOverlapEvents(true);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UAbilitySystemComponent* AGSUtilityStation::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSUtilityStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGSUtilityStation, PlacedItems);
}

void AGSUtilityStation::BeginPlay()
{
	Super::BeginPlay();

	// Cargar configuración e hitbox dinámicamente desde el Data Asset
	if (StationData)
	{
		EffectsToApply = StationData->StationDetails.EffectsToApply;
		AttributeSets = StationData->StationDetails.AttributeSets;

		if (StationVolume)
		{
			StationVolume->SetBoxExtent(StationData->StationDetails.HitBoxSize, true);
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Instanciar dinámicamente los Attribute Sets configurados para la estación
		for (const TSubclassOf<UAttributeSet>& SetClass : AttributeSets)
		{
			if (SetClass)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}
		}
	}


	if (StationVolume)
	{
		StationVolume->OnComponentBeginOverlap.AddDynamic(this, &AGSUtilityStation::OnOverlapBegin);
		StationVolume->OnComponentEndOverlap.AddDynamic(this, &AGSUtilityStation::OnOverlapEnd);
	}
}

AActor* AGSUtilityStation::GetLastPlacedItem() const
{
	for (int32 i = PlacedItems.Num() - 1; i >= 0; --i)
	{
		if (IsValid(PlacedItems[i]))
		{
			return PlacedItems[i];
		}
	}
	return nullptr;
}

AActor* AGSUtilityStation::RemoveLastPlacedItem()
{
	if (!HasAuthority())
	{
		return GetLastPlacedItem();
	}

	for (int32 i = PlacedItems.Num() - 1; i >= 0; --i)
	{
		AActor* Item = PlacedItems[i];
		if (IsValid(Item))
		{
			RemovePlacedItem(Item);
			return Item;
		}
		PlacedItems.RemoveAt(i);
	}
	return nullptr;
}

AActor* AGSUtilityStation::RemovePlacedItem(AActor* Item)
{
	if (!IsValid(Item)) { return nullptr; }

	if (HasAuthority())
	{
		// Remover efectos de juego aplicados al ASC del ítem
		if (TArray<FActiveGameplayEffectHandle>* Handles = AppliedEffectsMap.Find(Item))
		{
			if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
			{
				if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
				{
					for (const FActiveGameplayEffectHandle& Handle : *Handles)
					{
						if (Handle.IsValid())
						{
							TargetASC->RemoveActiveGameplayEffect(Handle);
						}
					}
				}
			}
			AppliedEffectsMap.Remove(Item);
		}

		PlacedItems.Remove(Item);
	}

	return Item;
}

const TArray<AActor*>& AGSUtilityStation::GetPlacedItems() const
{
	return PlacedItems;
}

AActor* AGSUtilityStation::GetFirstReadyItem() const
{
	for (AActor* Item : PlacedItems)
	{
		if (IsValid(Item))
		{
			if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Item))
			{
				if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
				{
					// Retorna el primer ítem que esté cocinado, enfriado o lleno
					if (ASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooked) ||
						ASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooled) ||
						ASC->HasMatchingGameplayTag(GSGameplayTags::State_Filled))
					{
						return Item;
					}
				}
			}
		}
	}
	return GetLastPlacedItem();
}

void AGSUtilityStation::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) { return; }

	if (OtherActor && OtherActor != this)
	{
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(OtherActor))
		{
			if (UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent())
			{
				if (!PlacedItems.Contains(OtherActor))
				{
					PlacedItems.Add(OtherActor);
				}

				// Aplicar los efectos de la estación al ítem
				TArray<FActiveGameplayEffectHandle>& Handles = AppliedEffectsMap.FindOrAdd(OtherActor);
				for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
				{
					if (EffectClass)
					{
						FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
						EffectContext.AddInstigator(this, this);

						FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
						if (SpecHandle.IsValid())
						{
							FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
							if (Handle.IsValid())
							{
								Handles.Add(Handle);
							}
						}
					}
				}
			}
		}
	}
}

void AGSUtilityStation::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) { return; }

	if (OtherActor)
	{
		RemovePlacedItem(OtherActor);
	}
}
