
#include "DataAssets/GSStationDataAsset.h"
#include "Machines/GSUtilityStation.h"
#include "TimerManager.h"

UGSStationDataAsset::UGSStationDataAsset()
{
}

UGSStationModule_Spawner::UGSStationModule_Spawner()
{
}

void UGSStationModule_Spawner::InitializeModule(AGSUtilityStation* Station)
{
	if (!IsValid(Station) || !Station->HasAuthority())
	{
		return;
	}

	UWorld* World = Station->GetWorld();
	if (!World)
	{
		return;
	}

	if (bAutoFillAllStationSockets && AutoFillItemClass)
	{
		if (bAutoFillSpawnImmediatelyOnStart)
		{
			TArray<FName> FreeSockets = Station->GetAllFreeSockets();
			for (const FName& FreeSocket : FreeSockets)
			{
				Station->SpawnItemInSocket(AutoFillItemClass, FreeSocket);
			}
		}

		FTimerDelegate AutoFillDel = FTimerDelegate::CreateUObject(this, &UGSStationModule_Spawner::TryAutoFillSpawn, TWeakObjectPtr<AGSUtilityStation>(Station));
		float FirstDelay = bAutoFillSpawnImmediatelyOnStart ? AutoFillSpawnInterval : FMath::Max(0.01f, AutoFillInitialDelay);
		World->GetTimerManager().SetTimer(AutoFillTimerHandle, AutoFillDel, AutoFillSpawnInterval, true, FirstDelay);
	}

	for (int32 i = 0; i < SpawnerConfigs.Num(); ++i)
	{
		const FGSSocketSpawnerConfig& Config = SpawnerConfigs[i];
		if (!Config.ItemClass)
		{
			continue;
		}

		if (Config.bSpawnImmediatelyOnStart)
		{
			TrySpawnForConfig(Station, i);
		}

		FTimerDelegate TimerDel = FTimerDelegate::CreateUObject(this, &UGSStationModule_Spawner::TrySpawnForConfig, TWeakObjectPtr<AGSUtilityStation>(Station), i);
		FTimerHandle& Handle = ActiveTimerHandles.FindOrAdd(i);

		float FirstDelay = Config.bSpawnImmediatelyOnStart ? Config.SpawnInterval : FMath::Max(0.01f, Config.InitialDelay);
		World->GetTimerManager().SetTimer(Handle, TimerDel, Config.SpawnInterval, true, FirstDelay);
	}
}

void UGSStationModule_Spawner::ShutdownModule(AGSUtilityStation* Station)
{
	if (IsValid(Station))
	{
		if (UWorld* World = Station->GetWorld())
		{
			if (AutoFillTimerHandle.IsValid())
			{
				World->GetTimerManager().ClearTimer(AutoFillTimerHandle);
			}

			for (auto& Pair : ActiveTimerHandles)
			{
				World->GetTimerManager().ClearTimer(Pair.Value);
			}
		}
	}
	ActiveTimerHandles.Empty();
}

void UGSStationModule_Spawner::OnItemRemoved(AGSUtilityStation* Station, AActor* Item)
{
	if (!IsValid(Station) || !Station->HasAuthority())
	{
		return;
	}
}

void UGSStationModule_Spawner::TrySpawnForConfig(TWeakObjectPtr<AGSUtilityStation> WeakStation, int32 ConfigIndex)
{
	AGSUtilityStation* Station = WeakStation.Get();
	if (!IsValid(Station) || !Station->HasAuthority())
	{
		return;
	}

	if (!SpawnerConfigs.IsValidIndex(ConfigIndex))
	{
		return;
	}

	const FGSSocketSpawnerConfig& Config = SpawnerConfigs[ConfigIndex];
	if (!Config.ItemClass)
	{
		return;
	}

	FName TargetSocket = Config.TargetSocket;
	if (TargetSocket.IsNone())
	{
		TargetSocket = Station->GetFreeSocket();
	}

	if (TargetSocket.IsNone() || Station->IsSocketOccupied(TargetSocket))
	{
		return;
	}

	Station->SpawnItemInSocket(Config.ItemClass, TargetSocket);
}

void UGSStationModule_Spawner::TryAutoFillSpawn(TWeakObjectPtr<AGSUtilityStation> WeakStation)
{
	AGSUtilityStation* Station = WeakStation.Get();
	if (!IsValid(Station) || !Station->HasAuthority() || !AutoFillItemClass)
	{
		return;
	}

	FName FreeSocket = bRandomizeSpawnOrder ? Station->GetRandomFreeSocket() : Station->GetFirstFreeSocket();
	if (FreeSocket.IsNone())
	{
		return;
	}

	Station->SpawnItemInSocket(AutoFillItemClass, FreeSocket);
}


