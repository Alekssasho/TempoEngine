using Sharpmake;

namespace TempoEngine
{
    [Sharpmake.Export]
    public class Glm : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.ExportDefines.Add("GLM_ENABLE_EXPERIMENTAL");
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
    public class Flecs : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.LibraryFiles.Add("flecs");
            conf.TargetCopyFiles.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\bin\flecs.dll");
        }
    }


    [Sharpmake.Export]
    public class GAInput : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            if (target.Optimization == Optimization.Debug)
            {
                conf.LibraryFiles.Add("gainput-d");
                conf.TargetCopyFiles.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\debug\bin\gainput-d.dll");
            }
            else
            {
                conf.LibraryFiles.Add("gainput");
                conf.TargetCopyFiles.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\bin\gainput.dll");
            }
        }
    }


    [Sharpmake.Export]
    public class Flatbuffers : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);
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

            if(target.Optimization == Optimization.Debug)
            {
                conf.LibraryFiles.Add("imguid");
            }
            else
            {
                conf.LibraryFiles.Add("imgui");
            }
        }
    }

    [Sharpmake.Export]
    public class Stb : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);
        }
    }

    [Sharpmake.Export]
    public class CGLTF : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);
        }
    }

    [Sharpmake.Export]
    public class MeshOptimizer : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.LibraryFiles.Add("meshoptimizer");
            conf.TargetCopyFiles.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\bin\meshoptimizer.dll");
        }
    }

    [Sharpmake.Export]
    public class Nvtt : ThirdPartyVcpkgProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            if(target.Optimization == Optimization.Debug)
            {
                conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\debug\lib\static");
                
            }
            else
            {
                conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\lib\static");
            }
            var suffix = (target.Optimization == Optimization.Debug) ? "_d" : "";
            conf.LibraryFiles.Add("nvtt" + suffix);
            conf.LibraryFiles.Add("nvthread" + suffix);
            conf.LibraryFiles.Add("nvsquish" + suffix);
            conf.LibraryFiles.Add("nvmath" + suffix);
            conf.LibraryFiles.Add("nvimage" + suffix);
            conf.LibraryFiles.Add("nvcore" + suffix);
            conf.LibraryFiles.Add("bc6h" + suffix);
            conf.LibraryFiles.Add("bc7" + suffix);
        }
    }
}