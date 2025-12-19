// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SUMOManager.h"
#include "GameFramework/Actor.h"
#include "VehicleDataUpdate.h"
#include "TrafficLightDataUpdate.h"
#include "SUMOTester.generated.h"

UCLASS()
class ASUMOTester : public AActor, public IVehicleDataUpdate, public ITrafficLightDataUpdate
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASUMOTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "GEMSTAR")
	ASUMOManager* sumoManager;

protected:
	virtual void TrafficLightDataUpdate_Implementation(FTrafficLightData& trafficLightData) override;
	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;
};
