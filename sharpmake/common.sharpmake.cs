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
        }

        [Configure]
        public virtual void ConfigureAll(Project.Configuration conf, Target target)
        {
            conf.ProjectPath = @"[project.SharpmakeCsPath]\..\projects";

            conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP20);
            conf.Options.Add(Options.Vc.Compiler.Exceptions.Disable);
            conf.Options.Add(Options.Vc.Compiler.RTTI.Disable);

            conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
        }
    }
}