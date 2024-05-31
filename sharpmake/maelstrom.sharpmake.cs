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

            conf.Defines.Add("_CRT_SECURE_NO_WARNINGS");

            conf.AddPrivateDependency<Tempest>(target);

            conf.AddPublicDependency<CGLTF>(target);
            conf.AddPublicDependency<MeshOptimizer>(target);
            conf.AddPublicDependency<Compressonator>(target);
        }
    }
}