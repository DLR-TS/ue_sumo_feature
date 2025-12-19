// Fill out your copyright notice in the Description page of Project Settings.


#include "SUMOTester.h"

// Sets default values
ASUMOTester::ASUMOTester()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASUMOTester::BeginPlay()
{
	Super::BeginPlay();
	if (sumoManager == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("No SUMO Actor defined. Test failed."));
	}
	else {
		IVehicleDataDispatcher::Execute_RegisterVehicleDataReceiver(sumoManager, this);
		ITrafficLightDataDispatcher::Execute_RegisterTrafficLight(sumoManager, this);
	}
}

// Called every frame
void ASUMOTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (sumoManager == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("No SUMO Actor defined. Test failed."));
	}
}

void ASUMOTester::TrafficLightDataUpdate_Implementation(FTrafficLightData& trafficLightData) {
	UE_LOG(LogTemp, Log, TEXT("TrafficLightData { id: %s, lightState: %s }"),
		*trafficLightData.id.ToString(),
		*trafficLightData.lightState);
}

void ASUMOTester::VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) {
	UE_LOG(LogTemp, Log,
		TEXT("VehicleDataUpdateData { Provider: %s, Name: %s, Position: %s, Speed: %.2f, Accel: %.2f, Signals: %d, VehicleClass: %s, BoundingBox: %s }"),
		*data.Provider.ToString(),
		*data.Name.ToString(),
		*data.Position.ToString(),
		data.speed,
		data.accel,
		data.signals,
		*data.VehicleClass.ToString(),
		*data.BoundingBox.ToString()
	);
}
