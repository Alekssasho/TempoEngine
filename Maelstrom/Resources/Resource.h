#pragma once
#include <EngineCore.h>

struct CompilerOptions
{
	eastl::string InputFolder;
	eastl::string OutputFolder;
};

CompilerOptions* gCompilerOptions = nullptr;

// This is needed to be able to cast to this and call Compile from a Job
struct ResourceBase
{
public:
    virtual void Compile() = 0;
};

// This is to be inherited so that we can have the storage and getter
template<typename T>
struct Resource : ResourceBase
{
public:
	using OutputDataType = T;

	Resource() = default;
	Resource(Resource&&) = default;

	Resource(const Resource&) = delete;
	Resource& operator=(const Resource&) = delete;

	const OutputDataType& GetCompiledData() const { return m_CompiledData; }
protected:
	OutputDataType m_CompiledData;
};

template<typename T>
void CompileResourceArray(eastl::span<T> resources, Tempest::Job::Counter& outCounter)
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

template<typename... Args>
void CompileResources(Tempest::Job::Counter& outCounter, Args&... args)
    requires (std::derived_from<Args, Resource<typename Args::OutputDataType>> && ...)
{
    auto compileFunc = [](uint32_t, void* resourcePtr) {
        ResourceBase* resource = reinterpret_cast<ResourceBase*>(resourcePtr);
        resource->Compile();
    };

	eastl::vector<Tempest::Job::JobDecl> jobs;
	jobs.reserve(sizeof...(Args));
	(jobs.emplace_back(compileFunc,  &args), ...);

    Tempest::gEngineCore->GetJobSystem().RunJobs("Compile Resources", jobs.data(), uint32_t(jobs.size()), &outCounter);
}
