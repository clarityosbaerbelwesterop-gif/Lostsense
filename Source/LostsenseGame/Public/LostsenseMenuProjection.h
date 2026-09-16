#pragma once

#include "CoreMinimal.h"

class ULostsenseRuntimeSubsystem;

struct FLostsenseInventoryMenuEntry final {
  FString Label;
  int64 InstanceId = 0;
  int32 ItemId = 0;
  int32 Quantity = 0;
  bool bEquipped = false;
  bool bEquippable = false;
};

struct FLostsenseScarMenuEntry final {
  int32 NodeId = 0;
  FString Name;
  FString Category;
  FString Detail;
  int32 PointCost = 0;
  bool bAllocated = false;
  bool bAvailable = false;
  bool bBlocked = false;
};

class LOSTSENSEGAME_API FLostsenseMenuProjection final {
public:
  [[nodiscard]] static bool
  CaptureInventory(const ULostsenseRuntimeSubsystem &Runtime,
                   TArray<FLostsenseInventoryMenuEntry> &OutEntries);

  [[nodiscard]] static bool
  CaptureScarAtlas(const ULostsenseRuntimeSubsystem &Runtime,
                   TArray<FLostsenseScarMenuEntry> &OutEntries);
};
