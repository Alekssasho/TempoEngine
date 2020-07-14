#include <Logging.h>
#include <stdio.h>

namespace Tempest
{
Logger* Logger::gLogger = nullptr;

Logger::Logger()
{
	gLogger = this;
}

static const char* StringifySeverity(LogSeverity severity)
{
	switch (severity)
	{
	case LogSeverity::Debug: return "Debug";
	case LogSeverity::Trace: return "Trace";
	case LogSeverity::Info: return "Info";
	case LogSeverity::Warning: return "Warning";
	case LogSeverity::Error: return "Error";
	case LogSeverity::Fatal: return "Fatal";
	default:
		return "Unknown severity";
	}
}

void Logger::WriteLog(LogSeverity severity, const char* system, const char* message)
{
	// TODO: This could be better
	printf("%s: [%s] %s\n", StringifySeverity(severity), system, message);
}
}