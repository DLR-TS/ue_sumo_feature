/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#include "GEMSTAR_SUMO_Loader.h"
#include "SUMOLog.h"
#include "Subsystems/GemstarSimulationSubsystem.h"
#include "SUMOManager.h"

// Sets default values for this component's properties
UGEMSTAR_SUMO_Loader::UGEMSTAR_SUMO_Loader()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGEMSTAR_SUMO_Loader::BeginPlay()
{
	Super::BeginPlay();

	UWorld *World = GetWorld();

	if (World == nullptr)
	{
		return;
	}

	UGemstarSimulationSubsystem* GemstarSimulationSubsystem = UGemstarSimulationSubsystem::Get(GetWorld());
	
	if (GemstarSimulationSubsystem == nullptr) {
		UE_LOG(Sumo, Error, TEXT("ue_sumo_feature: simulation subsystem unavailable"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();

	SumoManager = World->SpawnActor<ASUMOManager>(
		ASUMOManager::StaticClass(), FTransform::Identity, SpawnParams);

	if (!IsValid(SumoManager))
	{
		UE_LOG(Sumo, Error, TEXT("ue_sumo_feature: could not spawn ASUMOManager"));
		return;
	}

	const TMap<FString, FInnerConfig> config = GemstarSimulationSubsystem->RegisterManagerAndGetConfiguration(SumoManager, "ue_sumo_feature");

	SumoManager->Configure(config, GemstarSimulationSubsystem);

	UE_LOG(Sumo, Log, TEXT("Registered and configured SUMO"));
}

void UGEMSTAR_SUMO_Loader::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	if (IsValid(SumoManager))
	{
		SumoManager->Destroy();
		SumoManager = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
