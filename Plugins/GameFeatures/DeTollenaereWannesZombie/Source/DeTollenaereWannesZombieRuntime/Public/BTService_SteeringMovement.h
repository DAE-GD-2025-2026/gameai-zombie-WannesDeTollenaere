#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SteeringMovement.generated.h"

struct FSteeringMovementMemory
{
	float PathRecalculateTimer = 0.f;
	FVector CachedNextWaypoint = FVector::ZeroVector;
};

UCLASS()
class DETOLLENAEREWANNESZOMBIERUNTIME_API UBTService_SteeringMovement : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SteeringMovement();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ThreatKey;

	//Weights
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float PathFollowingWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float ZombieRepulsionWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float SidestepWeight = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Weights")
	float FleeWeight = 0.5f;

	// Parameters 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Parameters")
	float PathRecalculateInterval = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Parameters")
	float WaypointToleranceRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Parameters")
	float AvoidanceRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SidestepBias = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering|Parameters")
	float RotationSpeed = 1800.f;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FSteeringMovementMemory); }
};