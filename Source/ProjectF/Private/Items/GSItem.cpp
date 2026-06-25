#include "Items/GSItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"

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

		// Instanciar dinámicamente los Attribute Sets configurados
		for (const TSubclassOf<UAttributeSet>& SetClass : AttributeSets)
		{
			if (SetClass)
			{
				AbilitySystemComponent->InitStats(SetClass, nullptr);
			}
		}

		// Registrar las tags de identidad con replicación hacia clientes
		if (ItemTags.Num() > 0)
		{
			AbilitySystemComponent->AddLooseGameplayTags(ItemTags, 1, EGameplayTagReplicationState::TagOnly);
		}
	}
}
