#include "Components/GSMoneyValueComponent.h"

UGSMoneyValueComponent::UGSMoneyValueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MoneyValue = 10.0f;
	MoneyEffectClass = nullptr;
}
