// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "StudentPerceptor.generated.h"

class ABaseItem;
class UBlackboardComponent;


USTRUCT(BlueprintType)
struct DETOLLENAEREWANNESZOMBIERUNTIME_API FHouseMemory
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* HouseActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bScouted = false;

	UPROPERTY()
	TArray<ABaseItem*> KnownItems;

	bool IsValidHouse() const;
	FVector GetLocation() const;

	void PurgeStaleItems();
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DETOLLENAEREWANNESZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float SafetyDistance = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float WeaponGrabRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float ItemToHouseRadius = 2000.f;

	TArray<FHouseMemory>& GetKnownHouses() { return KnownHouses; }
	const TArray<FHouseMemory>& GetKnownHousesConst() const { return KnownHouses; }
	const TArray<AActor*>& GetKnownItems() const { return KnownItems;  }

	void RemoveKnownItem(AActor* Item) { KnownItems.Remove(Item); }
	void PurgeInvalidItems() { KnownItems.RemoveAll([](AActor* A){ return !IsValid(A); }); }

	void MarkHouseScouted(AActor* HouseActor);

	static float ScoreItem(ABaseItem* Item, APawn* OwnerPawn);

private:
	UPROPERTY()
	TArray<AActor*> KnownItems;

	UPROPERTY()
	TArray<FHouseMemory> KnownHouses;

	UPROPERTY()
	UBlackboardComponent* CachedBlackboard = nullptr;

	UBlackboardComponent* GetBlackboard();

	FHouseMemory* FindNearestHouseMemory(const FVector& WorldPos, float MaxRadius);
};
