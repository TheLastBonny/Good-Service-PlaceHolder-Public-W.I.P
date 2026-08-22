#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GSAIController.generated.h"

UCLASS()
class PROJECTF_API AGSAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGSAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual const FNavAgentProperties& GetNavAgentPropertiesRef() const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	mutable FNavAgentProperties CachedNavAgentProps;
};
