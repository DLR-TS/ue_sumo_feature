/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "SUMOTester.h"

#include "EngineUtils.h"

ASUMOTester::ASUMOTester()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASUMOTester::BeginPlay()
{
	Super::BeginPlay();
}

void ASUMOTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bRegistered)
	{
		return;
	}

	if (!IsValid(sumoManager))
	{

		for (TActorIterator<ASUMOManager> It(GetWorld()); It; ++It)
		{
			sumoManager = *It;
			break;
		}
	}

	if (!IsValid(sumoManager))
	{
		return;
	}
	IVehicleDataDispatcher::Execute_RegisterVehicleDataReceiver(sumoManager, this);
	ITrafficLightDataDispatcher::Execute_RegisterTrafficLight(sumoManager, this);
	bRegistered = true;

	UE_LOG(Sumo, Log, TEXT("SUMOTester: observing %s"), *sumoManager->GetName());
}

void ASUMOTester::TrafficLightDataUpdate_Implementation(FTrafficLightData &trafficLightData)
{
	UE_LOG(Sumo, Log, TEXT("TrafficLightData { id: %s, lightState: %s }"),
		   *trafficLightData.id.ToString(),
		   *trafficLightData.lightState);
}

void ASUMOTester::VehicleDataUpdate_Implementation(FVehicleDataUpdateData &data)
{
	UE_LOG(Sumo, Display,
		   TEXT("VehicleDataUpdateData { Provider: %s, Name: %s, Position: %s, Speed: %.2f, Accel: %.2f, Signals: %d, VehicleClass: %s, BoundingBox: %s }"),
		   *data.Provider.ToString(),
		   *data.Name.ToString(),
		   *data.Position.ToString(),
		   data.speed,
		   data.accel,
		   data.signals,
		   *data.VehicleClass.ToString(),
		   *data.BoundingBox.ToString());
}
