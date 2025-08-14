#pragma once

#include "Resource.h"
#include "Mesh.h"

#include "../GLTFScene.h"

#include <Physics/PhysicsConstants.h>

#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <DataDefinitions/PhysicsDatabase_generated.h>

struct InMemoryStreamOut : public JPH::StreamOut
{
	InMemoryStreamOut(eastl::vector<uint8_t>& outData)
		: Data(outData)
	{
		Data.reserve(10 * 1024);
	}

	virtual void WriteBytes(const void* inData, size_t inNumBytes) override
	{
		Data.reserve(Data.size() + inNumBytes);

		Data.insert(Data.end(), reinterpret_cast<const uint8_t*>(inData), reinterpret_cast<const uint8_t*>(inData) + inNumBytes);
	}

	virtual bool IsFailed() const override
	{
		return false;
	};

	eastl::vector<uint8_t>& Data;
};

enum class PhysicsMeshRequestType
{
    Mesh,
    Sphere,
};

struct PhysicsRequests
{
    flecs::entity UserData;
    uint32_t MeshIndex;
    PhysicsMeshRequestType Type;
	Tempest::Components::Transform Transform;
};

struct PrefabPhysicsRequest
{
	uint32_t MeshIndex;
	PhysicsMeshRequestType Type;

	bool operator==(const PrefabPhysicsRequest& o) const
	{
		return o.MeshIndex == MeshIndex &&
			o.Type == Type;
	}
};

struct Hasher
{
	size_t operator()(const PrefabPhysicsRequest& req) const { return uint64_t(req.MeshIndex) + (uint64_t(req.Type) << 32); }
};

struct PhysicsDatabaseResource : Resource<eastl::vector<uint8_t>>
{
public:
	PhysicsDatabaseResource(
		const eastl::vector<MeshResource>& meshes,
		const eastl::vector<PhysicsRequests>& initialRequests,
		const eastl::vector<PrefabPhysicsRequest>& prefabRequests
	)
		: m_Meshes(meshes)
		, m_InitialRequests(initialRequests)
		, m_PrefabRequests(prefabRequests)
	{}

	JPH::ShapeSettings* CreateShape(const MeshResource& mesh, PhysicsMeshRequestType type)
	{
		const auto& meshCompiledData = mesh.GetCompiledData();
		const uint32_t sizeVertex = mesh.m_Type == Tempest::Definition::MeshType::MeshType_StaticMesh ? sizeof(VertexLayout) : sizeof(SkeletonVertexLayout);
		if (type == PhysicsMeshRequestType::Sphere)
		{
			glm::vec3 minPoint(eastl::numeric_limits<float>::max());
			glm::vec3 maxPoint(eastl::numeric_limits<float>::min());

			for (const auto& primMesh : meshCompiledData)
			{
				for (uint32_t i = 0; i < primMesh.Vertices.size(); i += sizeVertex)
				{
					const VertexLayout* vertex = reinterpret_cast<const VertexLayout*>(primMesh.Vertices.data() + i);
					minPoint = glm::min(minPoint, vertex->Position);
					maxPoint = glm::max(maxPoint, vertex->Position);
				}
			}

			return new JPH::SphereShapeSettings(glm::distance(minPoint, maxPoint) / 2.0f);
		}
		else if (type == PhysicsMeshRequestType::Mesh)
		{
			// Concatenate all prim meshes
			JPH::VertexList wholeVertices;
			JPH::IndexedTriangleList indices;

			for (const auto& primMesh : meshCompiledData)
			{
				uint32_t currentVertexSize = uint32_t(wholeVertices.size());
				for (uint32_t i = 0; i < primMesh.Vertices.size(); i += sizeVertex)
				{
					const VertexLayout* vertex = reinterpret_cast<const VertexLayout*>(primMesh.Vertices.data() + i);
					wholeVertices.emplace_back(vertex->Position.x, vertex->Position.y, vertex->Position.z);
				}

				for (uint32_t i = 0; i < primMesh.WholeMeshIndices.size(); i += 3)
				{
					indices.emplace_back(
						primMesh.WholeMeshIndices[i] + currentVertexSize,
						primMesh.WholeMeshIndices[i + 1] + currentVertexSize,
						primMesh.WholeMeshIndices[i + 2] + currentVertexSize,
						0);
				}
			}

			return new JPH::MeshShapeSettings(wholeVertices, indices);
		}
		assert(false);
		return nullptr;
	}

	void Compile() override
	{
		JPH::RegisterDefaultAllocator();

		JPH::PhysicsScene initialScene;
		JPH::PhysicsScene prefabScene;

		eastl::unordered_map<PrefabPhysicsRequest, JPH::ShapeSettings*, Hasher> cachedShapes;
		auto getOrCreateShape = [&](const PrefabPhysicsRequest& request) {
				auto findItr = cachedShapes.find(request);
				if (findItr != cachedShapes.end())
				{
					return findItr->second;
				}
				auto newShape = CreateShape(m_Meshes[request.MeshIndex], request.Type);
				cachedShapes.insert(eastl::make_pair(request, newShape));
				return newShape;
			};

		for (const auto& initialRequest : m_InitialRequests)
		{
			JPH::BodyCreationSettings settings;
			settings.mUserData = initialRequest.UserData;
			// TODO: Jolt uses Right handed system, but we use Left handed, check what needs to be different to accommodate this
			settings.mPosition.Set(initialRequest.Transform.Position.x, initialRequest.Transform.Position.y, initialRequest.Transform.Position.z);
			settings.mRotation.Set(initialRequest.Transform.Rotation.x, initialRequest.Transform.Rotation.y, initialRequest.Transform.Rotation.z, initialRequest.Transform.Rotation.w);
			settings.mMotionType = JPH::EMotionType::Static;
			settings.mObjectLayer = Tempest::Physics::ObjectLayers::Static;
			settings.SetShapeSettings(getOrCreateShape(PrefabPhysicsRequest{initialRequest.MeshIndex, initialRequest.Type}));

			initialScene.AddBody(settings);
		}

		for (const auto& prefabRequest : m_PrefabRequests)
		{
			JPH::BodyCreationSettings settings;
			settings.mMotionType = JPH::EMotionType::Kinematic;
			settings.mObjectLayer = Tempest::Physics::ObjectLayers::Dynamic;
			settings.SetShapeSettings(getOrCreateShape(prefabRequest));

			prefabScene.AddBody(settings);
		}

		eastl::vector<uint8_t> initialSceneArray;
		eastl::vector<uint8_t> prefabSceneArray;
		InMemoryStreamOut initialStreamOut(initialSceneArray);
		InMemoryStreamOut prefabStreamOut(prefabSceneArray);

		initialScene.SaveBinaryState(initialStreamOut, true, true);
		prefabScene.SaveBinaryState(prefabStreamOut, true, true);

		flatbuffers::FlatBufferBuilder builder(1024 * 1024);
		auto initialSceneData = builder.CreateVector<uint8_t>(initialSceneArray.data(), initialSceneArray.size());
		auto prefabSceneData = builder.CreateVector<uint8_t>(prefabSceneArray.data(), prefabSceneArray.size());

		auto root = Tempest::Definition::CreatePhysicsDatabase(
			builder,
			initialSceneData,
			prefabSceneData
		);

		Tempest::Definition::FinishPhysicsDatabaseBuffer(builder, root);

		m_CompiledData.resize(builder.GetSize());
		memcpy(m_CompiledData.data(), builder.GetBufferPointer(), m_CompiledData.size());
	}

private:
	const eastl::vector<MeshResource>& m_Meshes;
	const eastl::vector<PhysicsRequests>& m_InitialRequests;
	const eastl::vector<PrefabPhysicsRequest>& m_PrefabRequests;
};
