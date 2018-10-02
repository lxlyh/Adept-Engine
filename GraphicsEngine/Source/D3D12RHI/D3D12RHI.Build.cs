using EngineBuildTool;

class D3D12RHIModule : ModuleDef
{
    public D3D12RHIModule()
    {
        ModuleName = "D3D12RHI";
        SourceFileSearchDir = "D3D12RHI";
        ModuleOutputType = ModuleDef.ModuleType.LIB;
        IncludeDirectories.Add("Source/Core");       
        SolutionFolderPath = "Engine/Modules/RHI";
        ModuleOutputType = ModuleDef.ModuleType.ModuleDLL;
    }
}