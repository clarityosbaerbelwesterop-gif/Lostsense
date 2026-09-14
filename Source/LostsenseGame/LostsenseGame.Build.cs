using UnrealBuildTool;

public class LostsenseGame : ModuleRules
{
    public LostsenseGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "EnhancedInput",
                "InputCore",
                "Lostsense"
            });
    }
}
