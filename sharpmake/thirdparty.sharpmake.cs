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
    public class EASTL : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\EASTL\include");

            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\EASTL\lib\[target.Name]");
            conf.LibraryFiles.Add("EASTL");
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
    public class ImGUI : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\imgui\include");
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