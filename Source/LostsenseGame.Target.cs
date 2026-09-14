using UnrealBuildTool;
using System.Collections.Generic;

public class LostsenseGameTarget : TargetRules
{
    public LostsenseGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange(new[] { "LostsenseGame" });
    }
}
