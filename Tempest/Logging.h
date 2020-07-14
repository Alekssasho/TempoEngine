#pragma once
#include <Defines.h>

namespace Tempest
{
enum class LogSeverity
{
	Debug,
	Trace,
	Info,
	Warning,
	Error,
	Fatal
};

class TEMPEST_API Logger
{
public:
	Logger();
	void WriteLog(LogSeverity severity, const char* system, const char* message);

	static Logger* gLogger;

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
};

#define LOG(Severity, System, Message) Tempest::Logger::gLogger->WriteLog(Tempest::LogSeverity::##Severity, #System, Message)
}