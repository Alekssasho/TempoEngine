#include <CommonIncludes.h>

#include <Logging.h>
#include <stdio.h>
#include <Windows.h>

#define MINIMUM_LOG_SEVERITY LogSeverity::Info

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
	if (severity < MINIMUM_LOG_SEVERITY)
	{
		return;
	}
	// TODO: This could be better
	//printf("%s: [%s] %s\n", StringifySeverity(severity), system, message);
    eastl::string buffer;
    buffer.sprintf("%s: [%s] %s\n", StringifySeverity(severity), system, message);
	::OutputDebugString(buffer.c_str());
}
}