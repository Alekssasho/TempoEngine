#pragma once
#include <EngineCore.h>

struct CompilerOptions
{
	eastl::string InputFolder;
	eastl::string OutputFolder;
};

CompilerOptions* gCompilerOptions = nullptr;

template<typename T>
struct Resource
{
public:
	using OutputDataType = T;

	virtual void Compile() = 0;
	const OutputDataType& GetCompiledData() { return m_CompiledData; }
protected:
	OutputDataType m_CompiledData;
};

template<typename T>
void CompileResourceArray(eastl::vector<T>& resources, Tempest::Job::Counter& outCounter)
	requires std::derived_from<T, Resource<typename T::OutputDataType>>
{
	auto compileFunc = [](uint32_t, void* resourcePtr) {
		T* resource = reinterpret_cast<T*>(resourcePtr);
		resource->Compile();
	};

	eastl::vector<Tempest::Job::JobDecl> jobs;
	jobs.resize(resources.size());
	for (uint32_t i = 0; i < resources.size(); ++i)
	{
		jobs[i].Data = &resources[i];
		jobs[i].EntryPoint = compileFunc;
	}

	Tempest::gEngineCore->GetJobSystem().RunJobs("Compile Resources", jobs.data(), uint32_t(jobs.size()), &outCounter);
}
