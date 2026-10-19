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
#include "Components/ActorComponent.h"
#include "GEMSTAR_SUMO_Loader.generated.h"

class ASUMOManager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UE_SUMO_FEATURERUNTIME_API UGEMSTAR_SUMO_Loader : public UActorComponent
{
	GENERATED_BODY()

public:
	UGEMSTAR_SUMO_Loader();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<ASUMOManager> SumoManager;
};
