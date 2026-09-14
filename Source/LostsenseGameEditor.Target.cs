using UnrealBuildTool;
using System.Collections.Generic;

public class LostsenseGameEditorTarget : TargetRules
{
    public LostsenseGameEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange(new[] { "LostsenseGame" });
    }
}
