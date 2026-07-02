#include "Items/GSItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "DataAssets/GSItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Components/GSGrabbableComponent.h"

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
	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSItem::BeginPlay: Item %s, Location: %s"), 
		*NetRole, *GetName(), *GetActorLocation().ToString());

	// Prevent the item's root component from inheriting any parent scale on both server and client
	if (USceneComponent* ItemRoot = GetRootComponent())
	{
		ItemRoot->SetAbsolute(false, false, true);
	}

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
	if (!ItemData) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG] AGSItem::OnStateTagChanged: %s has no ItemData configured!"), *GetName());
		return; 
	}

	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	const FGSItemStateDetails* Details = ItemData->ItemStatesMap.Find(CallbackTag);
	if (Details)
	{
		FString StateNameStr = Details->StateName.ToString();
		if (StateNameStr.IsEmpty()) { StateNameStr = CallbackTag.ToString(); }

		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSItem::OnStateTagChanged: %s CallbackTag: %s, NewCount: %d, StateName: %s"),
			*NetRole, *GetName(), *CallbackTag.ToString(), NewCount, *StateNameStr);

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
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSItem::OnStateTagChanged: %s callback for unmapped tag %s (Count: %d)"),
			*NetRole, *GetName(), *CallbackTag.ToString(), NewCount);
	}
}

void AGSItem::OnRep_AttachmentReplication()
{
	Super::OnRep_AttachmentReplication();

	FString NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
	UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSItem::OnRep_AttachmentReplication called for %s"), *NetRole, *GetName());

	if (GetAttachParentActor())
	{
		// Force absolute scale on client upon attachment replication
		if (USceneComponent* ItemRoot = GetRootComponent())
		{
			ItemRoot->SetAbsolute(false, false, true);
		}

		FName SocketName = GetRootComponent() ? GetRootComponent()->GetAttachSocketName() : NAME_None;
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSItem::OnRep_AttachmentReplication: %s is attached to %s on socket %s"), 
			*NetRole, *GetName(), *GetAttachParentActor()->GetName(), *SocketName.ToString());

		if (SocketName.IsNone() || SocketName == TEXT("None"))
		{
			float HeightOffset = 120.0f;
			if (UGSGrabbableComponent* GrabComp = FindComponentByClass<UGSGrabbableComponent>())
			{
				HeightOffset = GrabComp->FallbackAboveHeadHeight;
			}
			FTransform AboveHeadTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, HeightOffset));
			SetActorRelativeTransform(AboveHeadTransform);
			UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG][%s] AGSItem::OnRep_AttachmentReplication: Applied fallback AboveHeadTransform relative offset of %f for %s"), 
				*NetRole, HeightOffset, *GetName());
		}
	}
}

void AGSItem::OnRep_ReplicatedMovement()
{
	// Skip in-flight replicated movement packets if the item has already been attached to a socket
	if (GetAttachParentActor())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANTIGRAVITY_LOG] AGSItem::OnRep_ReplicatedMovement: Ignored in-flight movement replication for %s because it is attached to %s"),
			*GetName(), *GetAttachParentActor()->GetName());
		return;
	}

	Super::OnRep_ReplicatedMovement();
}
