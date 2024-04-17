using Sharpmake;

[module: Sharpmake.Include("common.sharpmake.cs")]
[module: Sharpmake.Include("thirdparty.sharpmake.cs")]
[module: Sharpmake.Include("tempest.sharpmake.cs")]
[module: Sharpmake.Include("maelstrom.sharpmake.cs")]
[module: Sharpmake.Include("spark.sharpmake.cs")]

namespace TempoEngine
{
    [Sharpmake.Generate]
    public class TempoEngine : Solution
    {
        public TempoEngine()
        {
            Name = "TempoEngine";
            AddTargets(TempoEngineTargets.Targets);
        }

        [Configure]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "TempoEngine";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\..";

            conf.AddProject<Spark>(target);
            conf.AddProject<Maelstrom>(target);
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