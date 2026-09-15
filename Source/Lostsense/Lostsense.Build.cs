using System.IO;
using UnrealBuildTool;

public class Lostsense : ModuleRules
{
    public Lostsense(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[] { "Core" });

        // The portable runtime intentionally keeps its existing include form:
        // #include "Lostsense/...". Expose the project Source directory so the
        // same headers are consumed by CMake and Unreal without duplication.
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "..")));
    }
}
