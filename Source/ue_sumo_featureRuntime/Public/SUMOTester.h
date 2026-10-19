/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#pragma once

#include "CoreMinimal.h"
#include "SUMOManager.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Movement/VehicleDataUpdate.h"
#include "Interfaces/TrafficLight/TrafficLightDataUpdate.h"
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
	TObjectPtr<ASUMOManager> sumoManager;

protected:
	virtual void TrafficLightDataUpdate_Implementation(FTrafficLightData& trafficLightData) override;
	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;

private:
	bool bRegistered = false;
};
