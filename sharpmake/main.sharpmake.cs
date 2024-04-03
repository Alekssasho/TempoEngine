using System.IO;
using Sharpmake;

namespace Tempo
{
[Sharpmake.Generate]
public class Spark : Project
{
    public Spark()
    {
        Name = "Spark";
        SourceRootPath = @"[project.SharpmakeCsPath]\..\Spark";

        AddTargets(new Target(
            Platform.win64,
            DevEnv.vs2022,
            Optimization.Debug | Optimization.Release
        ));
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.ProjectPath = @"[project.SharpmakeCsPath]\..\projects";
    }
}

[Sharpmake.Generate]
public class TempoEngine : Solution
{
    public TempoEngine()
    {
        Name = "TempoEngine";
        AddTargets(new Target(
            Platform.win64,
            DevEnv.vs2022,
            Optimization.Debug | Optimization.Release
        ));
    }

    [Configure]
    public void ConfigureAll(Configuration conf, Target target)
    {
        conf.SolutionPath = @"[solution.SharpmakeCsPath]\..";

        conf.AddProject<Spark>(target);
    }
}

public static class Main
{
    [Sharpmake.Main]
    public static void SharpmakeMain(Sharpmake.Arguments arguments)
    {
        arguments.Generate<TempoEngine>();
    }
}
}