// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Items/ItemType.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Zombies/BaseZombie.h"
#include <functional>
#include <vector>
#include "BehaviorTree/BTTaskNode.h"
#include "StudentPerceptorJonckheereChloe.generated.h"

class ABaseItem;
class AHouse;

struct SteeringOutput final
{
	SteeringOutput() = default;
	SteeringOutput(FVector Dir):
		Direction(Dir)
	{
	}
	FVector Direction{};
	bool IsValid{true};
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class JONCKHEERECHLOEZOMBIERUNTIME_API UStudentPerceptorJonckheereChloe : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStudentPerceptorJonckheereChloe();
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	// STEERING
	SteeringOutput Seek(const FVector& TargetLocation);
	SteeringOutput Flee(const FVector& TargetLocation);
	bool Face(const FVector& TargetLocation, float DeltaT);
	void Move(const FVector& Direction);
	SteeringOutput Avoid(const FVector& TargetLocation);
	SteeringOutput Priority();
	void AddPriorityBehavior(std::function<SteeringOutput()> behavior);
	void ClearPriorityBehaviors();
	
	// ZOMBIE
	void AttackBehavior(const FVector& TargetLocation, float DeltaT);
	
	// ITEMS
	bool UseItem(const EItemType& ItemType);
	bool HasItem(const EItemType& Item);
	void GrabItem(ABaseItem* Item);
	void SaveObject(ABaseItem* Item);
	bool IsMoreValuable(ABaseItem* Item);
private:
	UBlackboardComponent* m_pBlackBoard{};
	UInventoryComponent* m_pInventory{};
	UHealthComponent* m_pHealth{};
	UStaminaComponent* m_pStamina{};
	int m_MaxHealth{};
	int m_MaxStamina{};
	// ITEMS
	TArray<ABaseItem*> m_ItemsInInventory{};
	
	FString ItemEnumToString(const EItemType& ItemType) const;
	int GetFreeSlot() const;
	void SpecifySeenItem(const EItemType& ItemType);
	void ShootLine();
	
	// HOUSE
	void EnterHouse(AHouse* House);
	bool CanVisitHouse(AHouse* House);
	void UpdateDistanceToVillage(const FVector& SelfLocation);
	std::vector<AHouse*> m_VisitedHouses{};
	std::vector<FVector> m_HouseLocations{};
	
	// ZOMBIE
	void Shoot();
	void UpdateHasWeapon();
	
	std::vector<std::function<SteeringOutput()>> m_PriorityBehaviors{};
};

// TASKS
UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFleeTask final : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFleeTask();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask ( UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
private:
	UStudentPerceptorJonckheereChloe* m_Perceptor{};
	APawn* m_Pawn{};
	ABaseZombie* m_Zombie{};
	UBlackboardComponent* m_Blackboard{};
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API USprint final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	USprint() = default;
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UAttackTask final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UAttackTask();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
private:
	UStudentPerceptorJonckheereChloe* m_Perceptor{};
	UBlackboardComponent* m_Blackboard{};
	ABaseZombie* m_Zombie{};
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UGrab final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UGrab();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFetchWeapon final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UFetchWeapon();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
private:
	void SetWeaponLocation(const FVector& Location, UBlackboardComponent* Blackboard);
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFetchFood final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UFetchFood();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UFetchMedkit final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UFetchMedkit();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UConsumeMedkit final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UConsumeMedkit();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UConsumeFood final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UConsumeFood();
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class JONCKHEERECHLOEZOMBIERUNTIME_API UMove final : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UMove();
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector BlackboardKey;
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;	
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

private:
	TArray<FVector> m_Locations{};
	FVector m_TargetLocation{};
	UStudentPerceptorJonckheereChloe* m_Perceptor{};
	APawn* m_Pawn{};
	int m_CurrIdx{};
};
