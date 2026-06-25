// Copyright (c) 2026 Bonny. All rights reserved.
#include "Items/GSItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "DataAssets/GSItemDataAsset.h"
#include "Components/StaticMeshComponent.h"

AGSItem::AGSItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}

UAbilitySystemComponent* AGSItem::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSItem::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 1. Cargar dinámicamente los Attribute Sets del Data Asset a la lista de sets a instanciar
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

		// 2. Instanciar dinámicamente los Attribute Sets configurados
		for (const TSubclassOf<UAttributeSet>& SetClass : AttributeSets)
		{
			if (SetClass)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}
		}

		// 3. Inicializar los valores límites de atributos y registrar escuchas de tags dinámicamente
		if (ItemData)
		{
			// Añadir las tags iniciales
			if (ItemData->DefaultTags.Num() > 0)
			{
				AbilitySystemComponent->AddLooseGameplayTags(ItemData->DefaultTags, 1, EGameplayTagReplicationState::TagOnly);
			}

			// Inicializar los atributos numéricos configurados en el mapa
			for (const TPair<FGameplayTag, FGSItemStateDetails>& StatePair : ItemData->ItemStatesMap)
			{
				const FGameplayTag& StateTag = StatePair.Key;
				const FGSItemStateDetails& Details = StatePair.Value;

				if (Details.MaxProgressAttribute.IsValid())
				{
					AbilitySystemComponent->SetNumericAttributeBase(Details.MaxProgressAttribute, Details.MaxProgressValue);
				}

				// Suscribirse dinámicamente al cambio de esta tag
				AbilitySystemComponent->RegisterGameplayTagEvent(StateTag, EGameplayTagEventType::NewOrRemoved)
					.AddUObject(this, &AGSItem::OnStateTagChanged);
			}
		}

		// Registrar las tags de identidad de C++ con replicación hacia clientes
		if (ItemTags.Num() > 0)
		{
			AbilitySystemComponent->AddLooseGameplayTags(ItemTags, 1, EGameplayTagReplicationState::TagOnly);
		}
	}
}

void AGSItem::OnStateTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (!ItemData) { return; }

	const FGSItemStateDetails* Details = ItemData->ItemStatesMap.Find(CallbackTag);
	if (Details)
	{
		FString StateNameStr = Details->StateName.ToString();
		if (StateNameStr.IsEmpty()) { StateNameStr = CallbackTag.ToString(); }

		if (NewCount > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GAS Item] %s ha entrado al estado: %s"), *GetName(), *StateNameStr);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("%s: ¡%s!"), *GetName(), *StateNameStr));
			}

			// Si hay una malla de reemplazo configurada, aplicarla automáticamente
			if (Details->MeshOverride)
			{
				if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass())))
				{
					MeshComp->SetStaticMesh(Details->MeshOverride);
					UE_LOG(LogTemp, Log, TEXT("[GAS Item] %s cambió su malla visual a %s"), *GetName(), *Details->MeshOverride->GetName());
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[GAS Item] %s ha salido del estado: %s"), *GetName(), *StateNameStr);
		}
	}
}
