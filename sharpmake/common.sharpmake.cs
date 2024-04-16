using Sharpmake;

namespace TempoEngine
{
    public class TempoEngineTargets
    {
        public static Target[] Targets
        {
            get
            {
                return new Target[]{ new Target(
                    Platform.win64,
                    DevEnv.vs2022,
                    Optimization.Debug | Optimization.Release
                )};
            }
        }
     }

    [Sharpmake.Generate]
    public class CommonProject : Project
    {
        public CommonProject()
        {
            AddTargets(TempoEngineTargets.Targets);
            IsFileNameToLower = false;
            IsTargetFileNameToLower = false;
        }

        [Configure]
        public virtual void ConfigureAll(Project.Configuration conf, Target target)
        {
            conf.ProjectPath = @"[project.SharpmakeCsPath]\..\projects";
            conf.IntermediatePath = @"[project.SharpmakeCsPath]\..\Intermediate\[target.Name]\[project.Name]";
            conf.TargetPath = @"[project.SharpmakeCsPath]\..\Binaries\[target.Name]";

            conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP20);
            conf.Options.Add(Options.Vc.Compiler.Exceptions.Disable);
            conf.Options.Add(Options.Vc.Compiler.RTTI.Disable);
            conf.Options.Add(Options.Vc.Compiler.FiberSafe.Enable);
            if (target.Optimization == Optimization.Debug)
            {
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
            }
            else
            {
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
            }

            conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
            conf.Options.Add(Options.Vc.General.WarningLevel.Level3);

            conf.Defines.Add("_SILENCE_CXX20_CISO646_REMOVED_WARNING");
            conf.Defines.Add("_HAS_EXCEPTIONS=0");

            conf.Defines.Add("NOMINMAX");
            conf.Defines.Add("WIN32_LEAN_AND_MEAN");
        }
    }

    public class ThirdPartyProject : Project
    {
        public ThirdPartyProject()
        {
            AddTargets(TempoEngineTargets.Targets);
        }

        [Configure]
        public virtual void ConfigureAll(Project.Configuration conf, Target target)
        {
        }
    }

    public class ThirdPartyVcpkgProject : ThirdPartyProject
    {
        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\include");
            if(target.Optimization == Optimization.Debug)
            {
                conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\debug\lib");
            }
            else
            {
                conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\vcpkg_installed\x64-windows\lib");
            }
        }
    }
}