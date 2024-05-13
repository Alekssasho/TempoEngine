using Sharpmake;

namespace TempoEngine
{
    [Sharpmake.Generate]
    public class Maelstrom : CommonProject
    {
        public Maelstrom()
        {
            Name = "Maelstrom";
            SourceRootPath = @"[project.SharpmakeCsPath]\..\Maelstrom";
        }

        public override void ConfigureAll(Project.Configuration conf, Target target)
        {
            base.ConfigureAll(conf, target);

            conf.Output = Configuration.OutputType.Exe;

            conf.AddPrivateDependency<Tempest>(target);

            conf.AddPublicDependency<TinyGLTF>(target);
        }
    }
}