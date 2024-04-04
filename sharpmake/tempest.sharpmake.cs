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
            conf.IncludePaths.Add("[project.RootPath]");

            conf.Output = Configuration.OutputType.Lib;
        }
    }
}