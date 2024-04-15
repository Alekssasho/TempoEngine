using Sharpmake;

namespace TempoEngine
{
    [Sharpmake.Export]
    public class Glm : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\glm\include");
        }
    }

    [Sharpmake.Export]
    public class EASTL : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.LibraryFiles.Add("EASTL");

            // We don't use EaStdC so we need to disable this and use our version
            // of vsnprintf
            conf.ExportDefines.Add("EASTL_EASTDC_VSNPRINTF=0");
        }
    }

    [Sharpmake.Export]
    public class Optick : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\optick\include");

            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\Optick\lib\[target.Name]");
            conf.LibraryFiles.Add("OptickCore");
        }
    }


    [Sharpmake.Export]
    public class Flecs : ThirdPartyProject
    {
        public Flecs()
        {
            SourceRootPath = @"[project.SharpmakeCsPath]\..\ThirdParty\flecs";
        }

        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\flecs");

        }
    }


    [Sharpmake.Export]
    public class GAInput : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\gainput\include");

            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\gainput\lib\[target.Name]");
            conf.LibraryFiles.Add("gainputstatic");
        }
    }


    [Sharpmake.Export]
    public class Flatbuffers : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\flatbuffers\include");
        }
    }

    [Sharpmake.Export]
    public class PhysX : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\PhysX\include");

            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\PhysX\lib\[target.Name]");
            conf.LibraryFiles.Add("PhysXFoundation_64");
            conf.LibraryFiles.Add("PhysXCommon_64");
            conf.LibraryFiles.Add("PhysX_64");
            conf.LibraryFiles.Add("PhysXExtensions_static_64");
            conf.LibraryFiles.Add("PhysXPvdSDK_static_64");
            conf.LibraryFiles.Add("PhysXVehicle_static_64");
        }
    }

    [Sharpmake.Export]
    public class ImGUI : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.LibraryFiles.Add("imgui");
        }
    }

    [Sharpmake.Export]
    public class Stb : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\stb");
        }
    }
}