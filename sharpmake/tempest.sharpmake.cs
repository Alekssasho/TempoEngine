using Sharpmake;

namespace TempoEngine
{
    [Sharpmake.Generate]
    public class Tempest : CommonProject
    {
        public Tempest()
        {
            Name = "Tempest";
            SourceRootPath = @"[project.SharpmakeCsPath]\..\Tempest";
        }

        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.AddPublicDependency<Glm>(target);
            conf.AddPublicDependency<EASTL>(target);
            conf.AddPublicDependency<Optick>(target);

            conf.AddPublicDependency<PhysX>(target);
            conf.AddPublicDependency<Flecs>(target);
            conf.AddPublicDependency<Flatbuffers>(target);
            conf.AddPublicDependency<GAInput>(target);
            conf.AddPublicDependency<ImGUI>(target);
            conf.AddPublicDependency<Stb>(target);

            conf.IncludePaths.Add("[project.RootPath]");

            conf.ReferencesByNuGetPackage.Add("Microsoft.Direct3D.D3D12", "1.4.9");

            conf.LibraryFiles.Add("d3d12");
            conf.LibraryFiles.Add("dxgi");
            conf.LibraryFiles.Add("xinput");

            conf.PrecompHeader = "CommonIncludes.h";
            conf.PrecompSource = "CommonIncludesPch.cpp";

            conf.Output = Configuration.OutputType.Lib;
        }
    }
}