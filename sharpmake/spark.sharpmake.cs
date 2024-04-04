using Sharpmake;

namespace TempoEngine
{
    [Sharpmake.Generate]
    public class Spark : CommonProject
    {
        public Spark()
        {
            Name = "Spark";
            SourceRootPath = @"[project.SharpmakeCsPath]\..\Spark";
        }

        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);
            conf.Output = Configuration.OutputType.Exe;

            conf.AddPrivateDependency<Tempest>(target);
        }
    }
}