// Fill out your copyright notice in the Description page of Project Settings.


#include "StudentPerceptorJonckheereChloe.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "DrawDebugHelpers.h"
#include "Items/Pistol.h"
#include "Items/Shotgun.h"
#include "Survivor/SurvivorPawn.h"

UStudentPerceptorJonckheereChloe::UStudentPerceptorJonckheereChloe()
{
	PrimaryComponentTick.bCanEverTick = true;
	m_ItemsInInventory.SetNum(5);
}

void UStudentPerceptorJonckheereChloe::BeginPlay()
{
	Super::BeginPlay();

	m_pInventory = GetOwner()->GetComponentByClass<UInventoryComponent>();
	m_pHealth = GetOwner()->GetComponentByClass<UHealthComponent>();
	m_pStamina = GetOwner()->GetComponentByClass<UStaminaComponent>();
	
	m_MaxHealth = m_pHealth->GetMaxHealth();
	m_MaxStamina = m_pStamina->GetMaxStamina();
	
	if (auto PerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptorJonckheereChloe::OnPerceptionUpdated);
	}
	
	if (APawn* pPawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* pAIController = Cast<AAIController>(pPawn->GetController()))
		{
			m_pBlackBoard = pAIController->GetBlackboardComponent();
		}
	}
	
	if (m_pBlackBoard != nullptr)
	{
		m_pBlackBoard->SetValueAsBool("SawSomething", false);
	}
}

void UStudentPerceptorJonckheereChloe::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)

{
	if (m_pBlackBoard == nullptr) return;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	m_pBlackBoard->SetValueAsFloat("Health", m_pHealth->GetHealth()); // Always update health
	m_pBlackBoard->SetValueAsFloat("Stamina", m_pStamina->GetCurrentStamina()); // Always update stamina
	
	UpdateDistanceToVillage(GetOwner()->GetActorLocation());
}

void UStudentPerceptorJonckheereChloe::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (m_pBlackBoard == nullptr) return;
	if (Stimulus.WasSuccessfullySensed())
	{
		// HOUSE
		//*******
		if (AHouse* House = dynamic_cast<AHouse*>(Actor))
		{
			GEngine->AddOnScreenDebugMessage(5, 5.f, FColor::Blue, 
	FString::Printf(TEXT("SAW HOUSE")));
			if (CanVisitHouse(House))
			{
				EnterHouse(House);
			}
		}
		// ITEMS
		//*******
		if (ABaseItem* Item = dynamic_cast<ABaseItem*>(Actor))
		{
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("Item: %s"), *ItemEnumToString(Item->GetItemType())));
			if (Item->GetItemType() == EItemType::Garbage) return; // skip garbage
			
			m_pBlackBoard->SetValueAsObject("Item", Item);
			SpecifySeenItem(Item->GetItemType());
			m_pBlackBoard->SetValueAsBool("SawItem", true);
			m_pBlackBoard->SetValueAsVector("ItemLocation", Item->GetActorLocation());
		}
		
		// ZOMBIES
		//********
		if (ABaseZombie* Zombie = dynamic_cast<ABaseZombie*>(Actor))
		{
			if (m_pBlackBoard->GetValueAsBool("SawZombie") == false) // Prevent overwriting the previous sawn zombie
			{
				m_pBlackBoard->SetValueAsBool("SawZombie", true);
				m_pBlackBoard->SetValueAsObject("Zombie", Zombie);
			}
		}
	}
}

FString UStudentPerceptorJonckheereChloe::ItemEnumToString(const EItemType& ItemType) const
{
	switch (ItemType)
	{
	case EItemType::Garbage:
		return "Garbage";
	case EItemType::Food:
		return "Food";
	case EItemType::Medkit:
		return "Medkit";
	case EItemType::Shotgun:
		return "Shotgun";
	case EItemType::Pistol:
		return "Pistol";
	}
	return "Unknown";
}

int UStudentPerceptorJonckheereChloe::GetFreeSlot() const
{
	for (int index = 0; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr)
		{
			return index;
		}
	}
	return m_ItemsInInventory.Num(); // inv full
}

void UStudentPerceptorJonckheereChloe::GrabItem(ABaseItem* Item)
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	
	int FreeIdx{GetFreeSlot()};
	if (m_pInventory->GrabItem(FreeIdx, Item))
	{
		GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, 
	FString::Printf(TEXT("GRAB %s"), *Item->GetName()));
		
		if (Item->GetItemType() == EItemType::Shotgun || Item->GetItemType() == EItemType::Pistol)
		{
			m_pBlackBoard->SetValueAsBool("HasWeapon", true);
		}
		else if (Item->GetItemType() == EItemType::Medkit)
		{
			m_pBlackBoard->SetValueAsBool("HasMedkit", true);
		}
		else if (Item->GetItemType() == EItemType::Food)
		{
			m_pBlackBoard->SetValueAsBool("HasFood", true);
		}
	}
}

bool UStudentPerceptorJonckheereChloe::HasItem(const EItemType& Item)
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	
	if (Item == EItemType::Food)
	{
		int NrFood{};
		for (const auto& ItemInv : m_ItemsInInventory)
		{
			if (ItemInv == nullptr) continue;
			if (EItemType::Food == ItemInv->GetItemType())
			{
				NrFood++;
			}
		}
		if (NrFood >= 1)
			return true;
	}
	else
	{
		for (const auto& ItemInv : m_ItemsInInventory)
		{
			if (ItemInv == nullptr) continue;
			if (Item == ItemInv->GetItemType())
			{
				return true;
			}
		}
	}
	
	return false;
}

void UStudentPerceptorJonckheereChloe::SaveObject(ABaseItem* Item)
{
	switch (Item->GetItemType())
	{
	case EItemType::Food:
		m_pBlackBoard->SetValueAsObject("Food", Item);	
		break;
	case EItemType::Medkit:
		m_pBlackBoard->SetValueAsObject("Medkit", Item);	
		break;
	case EItemType::Shotgun:
		m_pBlackBoard->SetValueAsObject("Shotgun", Item);	
		break;
	case EItemType::Pistol:
		m_pBlackBoard->SetValueAsObject("Pistol", Item);	
		break;
	}
}

void UStudentPerceptorJonckheereChloe::SpecifySeenItem(const EItemType& ItemType)
{
	switch (ItemType)
	{
	case EItemType::Food:
		m_pBlackBoard->SetValueAsBool("SawFood", true);
		break;
	case EItemType::Medkit:
		m_pBlackBoard->SetValueAsBool("SawMedkit", true);
		break;
	case EItemType::Shotgun:
		m_pBlackBoard->SetValueAsBool("SawShotgun", true);
		break;
	case EItemType::Pistol:
		m_pBlackBoard->SetValueAsBool("SawPistol", true);
		break;
	}
}

void UStudentPerceptorJonckheereChloe::ShootLine()
{
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Red, 
		FString::Printf(TEXT("SHOOT")));
	FVector Start = GetOwner()->GetActorLocation();
	FVector End = Start + GetOwner()->GetActorForwardVector() * 10000.f;
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.5f);
}

void UStudentPerceptorJonckheereChloe::EnterHouse(AHouse* House)
{
	m_pBlackBoard->SetValueAsBool("SawHouse", true);
	m_pBlackBoard->SetValueAsObject("LastVisitedHouse", House);
	m_pBlackBoard->SetValueAsVector("HouseLocation", House->GetBounds().Origin);
	m_HouseLocations.push_back(House->GetBounds().Origin);
}

bool UStudentPerceptorJonckheereChloe::CanVisitHouse(AHouse* House)
{
	auto itr = std::ranges::find(m_VisitedHouses, House);
	if (itr == m_VisitedHouses.end())
	{
		m_VisitedHouses.push_back(House);
		return true;
	}
	GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Blue, 
	FString::Printf(TEXT("cannot enter")));
	return false;
}

void UStudentPerceptorJonckheereChloe::UpdateDistanceToVillage(const FVector& SelfLocation)
{
	float ClosestDistanceToVillage{FLT_MAX};
	for (const auto& HouseLocation : m_HouseLocations)
	{
		float NewDist{(float)FVector::Dist2D(HouseLocation, SelfLocation)};
		if (NewDist < ClosestDistanceToVillage)
		{
			ClosestDistanceToVillage = NewDist;
		}
	}
	m_pBlackBoard->SetValueAsFloat("DistanceToVillage", ClosestDistanceToVillage);
}

void UStudentPerceptorJonckheereChloe::Shoot()
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	// Look for a weapon to use
	EItemType PreferredWeapon{};
	EItemType SecondChoise{};
	
	UObject* Object = m_pBlackBoard->GetValueAsObject("Zombie");
	if (ABaseZombie* Zombie = Cast<ABaseZombie>(Object))
	{
		const float Radius{300.f};
		if (FVector::Dist2D(Zombie->GetActorLocation(), GetOwner()->GetActorLocation()) < Radius)
		{
			PreferredWeapon = EItemType::Shotgun;
			SecondChoise = EItemType::Pistol;
		}
		else
		{
			PreferredWeapon = EItemType::Pistol;
			SecondChoise = EItemType::Shotgun;
		}
	
		for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
		{
			if (m_ItemsInInventory[index] == nullptr) continue;
		
			if (UseItem(PreferredWeapon))
			{
				ShootLine();
			}
			else if (UseItem(SecondChoise))
			{
				ShootLine();
			}
		}
		UpdateHasWeapon();
	}
}

void UStudentPerceptorJonckheereChloe::UpdateHasWeapon()
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	for (const auto& Item : m_ItemsInInventory)
	{
		if (Item == nullptr) continue;
		if (Item->GetItemType() == EItemType::Shotgun || Item->GetItemType() == EItemType::Pistol)
		{
			m_pBlackBoard->SetValueAsBool("HasWeapon", true);
			return;
		}
	}
	m_pBlackBoard->SetValueAsBool("HasWeapon", false);
}

void UStudentPerceptorJonckheereChloe::AttackBehavior(const FVector& TargetLocation, float DeltaT)
{
	UpdateHasWeapon();
	if (not m_pBlackBoard->GetValueAsBool("HasWeapon"))
	{
		return;
	}
	if (Face(TargetLocation, DeltaT)) // Only shoot if facing the zombie
	{
		Shoot();
	}
	const float Radius{300.f};
	// Walk backwards whilst trying to shoot a zombie
	if (FVector::Dist(GetOwner()->GetActorLocation(), TargetLocation) <= Radius)
	{
		SteeringOutput Steering = Flee(TargetLocation);
		Move(Steering.Direction);
	}
}

SteeringOutput UStudentPerceptorJonckheereChloe::Seek(const FVector& TargetLocation)
{
	FVector Dir{(FVector(TargetLocation) - GetOwner()->GetActorLocation()).GetSafeNormal()};
	Dir.Z = 0;
	return Dir;
}

SteeringOutput UStudentPerceptorJonckheereChloe::Flee(const FVector& TargetLocation)
{
	SteeringOutput Steering{Seek(TargetLocation)};
	Steering.Direction *= -1;
	return  Steering;
}

bool UStudentPerceptorJonckheereChloe::Face(const FVector& TargetLocation, float DeltaT)
{
	constexpr float Threshold{0.1f};
	const float RotSpeed{80.f};
	double AngularVelocity{0.f};

	FVector2D AgentForward(GetOwner()->GetActorForwardVector().X, GetOwner()->GetActorForwardVector().Y);

	// calc delta angle
	FVector2D Destination = FVector2D{TargetLocation - GetOwner()->GetActorLocation()};

	double TargetAngle = FMath::Atan2(Destination.Y, Destination.X);
	double ForwardAngle = FMath::Atan2(AgentForward.Y, AgentForward.X);

	double DeltaAngle = TargetAngle - ForwardAngle;
	DeltaAngle = FMath::UnwindRadians(DeltaAngle); // Get smallest angle

	if (abs(DeltaAngle) >= Threshold)
	{
		AngularVelocity =  DeltaAngle * DeltaT * RotSpeed;
	}
	else
	{
		FRotator ExactRotation = (TargetLocation - GetOwner()->GetActorLocation()).ToOrientationRotator();
		ExactRotation.Pitch = 0;
		ExactRotation.Roll = 0;
		GetOwner()->SetActorRotation(ExactRotation);
		return true;
	}
	
	const float MaxAngularSpeed{400.f};
	float const DeltaYaw = FMath::Clamp(AngularVelocity, -1.0f, 1.0f) * MaxAngularSpeed * DeltaT;
				
	FRotator const CurrentRotation{GetOwner()->GetActorForwardVector().ToOrientationRotator()};
	FRotator const DeltaRotation{0, DeltaYaw, 0};
	FRotator const DesiredRotation{CurrentRotation + DeltaRotation};
				
	// We only ever care about yaw
	if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw))
	{
		GetOwner()->SetActorRotation(DesiredRotation);
	}
	return false;
}

void UStudentPerceptorJonckheereChloe::Move(const FVector& Direction)
{
	if (m_pStamina->GetCurrentStamina() > 0) // only move when stamina left
	{
		Cast<APawn>(GetOwner())->AddMovementInput(Direction);
	}
}

SteeringOutput UStudentPerceptorJonckheereChloe::Avoid(const FVector& TargetLocation)
{
	SteeringOutput Steering{};
	constexpr float StepSize{30.f};
	FCollisionQueryParams CollisionParams{};
	CollisionParams.AddIgnoredActor(GetOwner());
	const FVector Start = GetOwner()->GetActorLocation();

	for (float Angle = 0.f; Angle < 360.f; Angle += StepSize)
	{
		// Try find an angle where there is no obstacle in front of you
		FVector FreeDir = TargetLocation.RotateAngleAxis(Angle, FVector::UpVector);
		FreeDir.Z = 0;

		FHitResult HitResult{};
		GetWorld()->LineTraceSingleByChannel(HitResult, Start,
			Start + FreeDir * 170.f, ECC_Pawn, CollisionParams);

		if (!HitResult.bBlockingHit)
		{
			Steering.IsValid = true;
			Steering.Direction = FreeDir.GetSafeNormal();
			return Steering;
		}
	}
	
	Steering.IsValid = false;
	Steering.Direction = TargetLocation;
	return Steering;
}

SteeringOutput UStudentPerceptorJonckheereChloe::Priority()
{
	SteeringOutput Steering = {};

	for (auto Behavior : m_PriorityBehaviors)
	{
		Steering = Behavior();
	
		if (Steering.IsValid)
		{
			break;
		}
	}
	// No valid behavior
	return Steering;
}

void UStudentPerceptorJonckheereChloe::AddPriorityBehavior(std::function<SteeringOutput()> behavior)
{
	m_PriorityBehaviors.push_back(behavior);
}

void UStudentPerceptorJonckheereChloe::ClearPriorityBehaviors()
{
	m_PriorityBehaviors.clear();
}

bool UStudentPerceptorJonckheereChloe::UseItem(const EItemType& ItemType)
{
	m_ItemsInInventory = m_pInventory->GetInventory();
	// Look if we have the correct item we want to use
	for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr) continue;
		if (m_ItemsInInventory[index]->GetItemType() == ItemType)
		{
			m_pInventory->UseItem(index);
			GEngine->AddOnScreenDebugMessage(5, 1.f, FColor::Green, FString::Printf(TEXT("Used item: %s"), *ItemEnumToString(ItemType)));
			if (m_ItemsInInventory[index]->GetValue() == 0) // Drop item if it has no value left
			{
				m_pInventory->RemoveItem(index);
			}
			return true;
		}
	}
	return false;
}

bool UStudentPerceptorJonckheereChloe::IsMoreValuable(ABaseItem* Item)
{
	m_ItemsInInventory = m_pInventory->GetInventory();

	for (int index{0}; index < m_ItemsInInventory.Num(); ++index)
	{
		if (m_ItemsInInventory[index] == nullptr) continue;
		if (m_ItemsInInventory[index]->GetItemType() == Item->GetItemType() && m_ItemsInInventory[index]->GetValue() < Item->GetValue())
		{
			GEngine->AddOnScreenDebugMessage(6, 3.f, FColor::Green, 
	FString::Printf(TEXT("Dropped %s, prev value %i new value %i"), *ItemEnumToString(Item->GetItemType()), m_ItemsInInventory[index]->GetValue(), Item->GetValue()));
			m_pInventory->RemoveItem(index); // Drop so we can pickup the more valuable version
			return true;
		}
	}
	return false;
}

// TASKS
UFleeTask::UFleeTask()
{
	NodeName = "Flee";
	bNotifyTick = true;
}

EBTNodeResult::Type UFleeTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	m_Pawn = Controller->GetPawn();
	m_Blackboard = Controller->GetBlackboardComponent();

	UObject* Object = m_Blackboard->GetValueAsObject("Zombie");
	m_Zombie = Cast<ABaseZombie>(Object);
	if (!m_Zombie) return EBTNodeResult::Failed;
	m_Perceptor = m_Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	m_Perceptor->ClearPriorityBehaviors();
	
	m_Perceptor->AddPriorityBehavior([this]()
	{
		FVector FleeDir = m_Perceptor->Flee(m_Zombie->GetActorLocation()).Direction;
		return m_Perceptor->Avoid(FleeDir);
	});

	m_Perceptor->AddPriorityBehavior([this]()
	{
		return m_Perceptor->Flee(m_Zombie->GetActorLocation());
	});
	
	return EBTNodeResult::InProgress;
}

void UFleeTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	SteeringOutput Output = m_Perceptor->Priority();

	m_Perceptor->Move(Output.Direction);
	m_Perceptor->Face(m_Pawn->GetActorLocation() + Output.Direction * 100.f, DeltaSeconds);
	
	// Distance
	const float SafeDistance{1500.f};
	if (FVector::Dist(m_Pawn->GetActorLocation(), m_Zombie->GetActorLocation()) >= SafeDistance)
	{
		GEngine->AddOnScreenDebugMessage(16, 3.f, FColor::Magenta, 
	FString::Printf(TEXT("SAFE SPOT")));
		m_Blackboard->SetValueAsBool("SawZombie", false);
		Cast<ASurvivorPawn>(m_Pawn)->StopRunning();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UFleeTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Cast<ASurvivorPawn>(m_Pawn)->StopRunning();
	GEngine->AddOnScreenDebugMessage(16, 3.f, FColor::Magenta, 
	FString::Printf(TEXT("STOP RUNNING")));
	return EBTNodeResult::Aborted;
}

EBTNodeResult::Type USprint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	Cast<ASurvivorPawn>(Pawn)->StartRunning();
	return EBTNodeResult::Succeeded;
}

UAttackTask::UAttackTask()
{
	NodeName = "Attack";
	bNotifyTick = true;
}

EBTNodeResult::Type UAttackTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	m_Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	m_Blackboard = Controller->GetBlackboardComponent();

	UObject* Object = m_Blackboard->GetValueAsObject("Zombie");
	m_Zombie = Cast<ABaseZombie>(Object);
	return EBTNodeResult::InProgress;
}

void UAttackTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (m_Zombie)
	{
		m_Perceptor->AttackBehavior(m_Zombie->GetActorLocation(), DeltaSeconds);
	}
	
	if (not IsValid(m_Zombie))
	{
		GEngine->AddOnScreenDebugMessage(16, 3.f, FColor::Magenta, 
	FString::Printf(TEXT("ZOMBIE DIED")));
		m_Blackboard->SetValueAsBool("SawZombie", false);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

UGrab::UGrab()
{
	NodeName = "Grab";
}

EBTNodeResult::Type UGrab::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	UObject* Object = Blackboard->GetValueAsObject("Item");
	ABaseItem* Item = Cast<ABaseItem>(Object);
	if (Item == nullptr) return EBTNodeResult::Failed;
	
	// Grab if not in inv
	if (not Perceptor->HasItem(Item->GetItemType()))
	{
		Perceptor->GrabItem(Item);
	}
	else if (Perceptor->IsMoreValuable(Item))
	{
		Perceptor->GrabItem(Item);
	}
	else
	{
		Perceptor->SaveObject(Item);
	}
	return EBTNodeResult::Succeeded;
}

UFetchWeapon::UFetchWeapon()
{
	NodeName = "Fetch weapon";
}

EBTNodeResult::Type UFetchWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	
	if (UObject* Object = Blackboard->GetValueAsObject("Shotgun"))
	{
		if (AShotgun* Shotgun = Cast<AShotgun>(Object))
		{
			FVector Location{Shotgun->GetActorLocation()};
			SetWeaponLocation(Location, Blackboard);
			return EBTNodeResult::Succeeded;
		}
	}
	else
	{
		if (UObject* Object1 = Blackboard->GetValueAsObject("Pistol"))
		{
			if (APistol* Pistol = Cast<APistol>(Object1))
			{
				FVector Location{Pistol->GetActorLocation()};
				SetWeaponLocation(Location, Blackboard);
				return EBTNodeResult::Succeeded;
			}
		}
	}
	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Orange, FString::Printf(TEXT("No weapons seen")));
	return EBTNodeResult::Failed;
}

void UFetchWeapon::SetWeaponLocation(const FVector& Location, UBlackboardComponent* Blackboard)
{
	Blackboard->SetValueAsVector("WeaponLocation", Location);
	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Green, FString::Printf(TEXT("Fetching pistol")));
}

UFetchFood::UFetchFood()
{
	NodeName = "Fetch food";
}

EBTNodeResult::Type UFetchFood::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	
	UObject* Object = Blackboard->GetValueAsObject("Food");
	if (ABaseItem* Food = Cast<ABaseItem>(Object))
	{
		FVector Location{Food->GetActorLocation()};
		Blackboard->SetValueAsVector("FoodLocation", Location);
		GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Green, FString::Printf(TEXT("Fetching food")));
		return EBTNodeResult::Succeeded;
	}

	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Orange, FString::Printf(TEXT("No food seen")));
	return EBTNodeResult::Failed;
}

UFetchMedkit::UFetchMedkit()
{
	NodeName = "Fetch medkit";
}

EBTNodeResult::Type UFetchMedkit::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	
	UObject* Object = Blackboard->GetValueAsObject("Medkit");
	if (ABaseItem* Food = Cast<ABaseItem>(Object))
	{
		FVector Location{Food->GetActorLocation()};
		Blackboard->SetValueAsVector("MedkitLocation", Location);
		GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Green, FString::Printf(TEXT("Fetching medkit")));
		return EBTNodeResult::Succeeded;
	}

	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Orange, FString::Printf(TEXT("No medkit seen")));
	return EBTNodeResult::Failed;
}

UConsumeMedkit::UConsumeMedkit()
{
	NodeName = "Consume medkit";
}

EBTNodeResult::Type UConsumeMedkit::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	if (Perceptor->UseItem(EItemType::Medkit))
	{
		Controller->GetBlackboardComponent()->SetValueAsBool("HasMedkit", false);
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
}

UConsumeFood::UConsumeFood()
{
	NodeName = "Consume food";
}

EBTNodeResult::Type UConsumeFood::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller->GetPawn();
	UStudentPerceptorJonckheereChloe* Perceptor = Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	if (Perceptor->UseItem(EItemType::Food))
	{
		if (not Perceptor->HasItem(EItemType::Food))
		{
			Controller->GetBlackboardComponent()->SetValueAsBool("HasFood", false);
			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}

UMove::UMove()
{
	NodeName = "Move";
	bNotifyTick = true;
}

EBTNodeResult::Type UMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	m_CurrIdx = 0;
	AAIController* Controller = OwnerComp.GetAIOwner();
	m_Pawn = Controller->GetPawn();
	ASurvivorPawn* Survivor = Cast<ASurvivorPawn>(m_Pawn);
	m_Perceptor = m_Pawn->GetComponentByClass<UStudentPerceptorJonckheereChloe>();
	UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
	m_TargetLocation = Blackboard->GetValueAsVector(BlackboardKey.SelectedKeyName);
	m_Locations = Survivor->CalculatePath(m_TargetLocation);
	
	return EBTNodeResult::InProgress;
}

void UMove::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (m_Locations.IsEmpty())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	GEngine->AddOnScreenDebugMessage(7, 1.f, FColor::Magenta, FString::Printf(TEXT("%f %f"), m_Locations[m_CurrIdx].X, m_Locations[m_CurrIdx].Y));
	m_TargetLocation = m_Locations[m_CurrIdx];
	SteeringOutput Steering{m_Perceptor->Seek(m_TargetLocation)};
	GEngine->AddOnScreenDebugMessage(8, 1.f, FColor::Magenta, FString::Printf(TEXT("%f %f"), Steering.Direction.X, Steering.Direction.Y));
	m_Perceptor->Face(m_TargetLocation, DeltaSeconds);
	m_Perceptor->Move(Steering.Direction);
	
	const float Distance{50.f};
	if (FVector::Dist(m_Pawn->GetActorLocation(), m_Locations[m_CurrIdx]) < Distance)
	{
		++m_CurrIdx;
	}
	
	if (m_CurrIdx > m_Locations.Num() - 1)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UMove::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BlackboardData = GetBlackboardAsset())
	{
		BlackboardKey.ResolveSelectedKey(*BlackboardData);
	}
}



