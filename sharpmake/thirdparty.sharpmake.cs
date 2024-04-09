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
        }
    }
}