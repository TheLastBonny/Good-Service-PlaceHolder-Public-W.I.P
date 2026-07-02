#include "Components/GSNPCComponent.h"
#include "Items/GSItem.h"
#include "Items/GSMoneyItem.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Core/GSGameplayTags.h"

UGSNPCComponent::UGSNPCComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentNPCState = ENPCState::None;
	bHasActiveOrder = false;
	AssignedTargetSpot = nullptr;
	MoneyItemClass = nullptr;
	bRequireCookedState = true;
}

void UGSNPCComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGSNPCComponent::SetNPCState(ENPCState NewState)
{
	if (CurrentNPCState != NewState)
	{
		CurrentNPCState = NewState;
		OnNPCStateChanged.Broadcast(CurrentNPCState);
	}
}

void UGSNPCComponent::ChooseRandomOrder()
{
	if (LevelMenu)
	{
		ActiveOrder = LevelMenu->GetRandomRecipe();
		if (ActiveOrder.FoodTag.IsValid())
		{
			bHasActiveOrder = true;
			OnOrderChosen.Broadcast(ActiveOrder);
			SetNPCState(ENPCState::Ordering);
		}
	}
}

bool UGSNPCComponent::CheckIfItemMatchesOrder(AGSItem* Item) const
{
	if (!Item || !bHasActiveOrder)
	{
		return false;
	}

	UAbilitySystemComponent* ItemASC = Item->GetAbilitySystemComponent();
	if (ItemASC)
	{
		bool bMatchesFood = ItemASC->HasMatchingGameplayTag(ActiveOrder.FoodTag);
		
		if (bRequireCookedState)
		{
			// Fetch state tags using native tags namespace
			bool bIsCooked = ItemASC->HasMatchingGameplayTag(GSGameplayTags::State_Cooked);
			bool bIsBurned = ItemASC->HasMatchingGameplayTag(GSGameplayTags::State_Burned);

			return bMatchesFood && bIsCooked && !bIsBurned;
		}

		return bMatchesFood;
	}
	
	// Fallback to ItemTags if AbilitySystemComponent is not available
	return Item->ItemTags.HasTagExact(ActiveOrder.FoodTag);
}


bool UGSNPCComponent::DeliverItem(AGSItem* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		// Delivery checking and spawning must happen on the server (authority)
		return false;
	}

	if (!Item || !bHasActiveOrder)
	{
		return false;
	}

	if (CheckIfItemMatchesOrder(Item))
	{
		// Spawn the physical money drop
		if (MoneyItemClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = Cast<APawn>(GetOwner());
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			FVector SpawnLoc = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f); // Spawn slightly above feet level
			FRotator SpawnRot = GetOwner()->GetActorRotation();

			if (AGSMoneyItem* SpawnedMoney = GetWorld()->SpawnActor<AGSMoneyItem>(MoneyItemClass, SpawnLoc, SpawnRot, SpawnParams))
			{
				SpawnedMoney->MoneyValue = ActiveOrder.BasePrice;
			}
		}

		// Clear order state
		bHasActiveOrder = false;
		ActiveOrder = FGSFoodRecipeDetails();

		// Notify success
		OnOrderFulfilled.Broadcast(true);

		// Transition state to Eating or Leaving (Eating is a good bridge before leaving)
		SetNPCState(ENPCState::Eating);

		// Destroy the delivered food item
		Item->Destroy();
		return true;
	}

	return false;
}
