#pragma once

#include <DataDefinitions/Level_generated.h>

#include <fstream>
#include <filesystem>

#include <EASTL/unordered_set.h>
#include <EASTL/set.h>

#pragma warning(push)
#pragma warning(disable: 4018 4267 4996)
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#pragma warning(pop)

class Scene
{
public:
	Scene(const std::filesystem::path& sceneFile)
	{
		std::string err, warn;
		tinygltf::TinyGLTF loader;
		loader.LoadASCIIFromFile(&m_Model, &err, &warn, sceneFile.string());

		m_RootNodes = m_Model.scenes[0].nodes;
		ExtractCamera();
		GatherMeshMaterialCarIndices();
	}

private:
	void ExtractCamera()
	{
		for (const auto& rootNode : m_RootNodes)
		{
			using ReturnType = std::pair<std::tuple<float, float, float, float>, glm::mat4>;
			auto value = WalkNodes<ReturnType>(rootNode, glm::identity<glm::mat4>(), [](const tinygltf::Model& model, int index, const glm::mat4& transform) {
				if (model.nodes[index].camera != -1)
				{
					const auto& cameraData = model.cameras[model.nodes[index].camera];
					if (cameraData.type == "perspective")
					{
						ReturnType camera(std::make_pair(std::make_tuple(
							float(cameraData.perspective.yfov),
							float(cameraData.perspective.aspectRatio != 0.0 ? cameraData.perspective.aspectRatio : 16.0f / 9.0f),
							float(cameraData.perspective.znear),
							float(cameraData.perspective.zfar != 0.0 ? cameraData.perspective.zfar : 1000.0f)
						), transform));
						return std::optional(camera);
					}
				}
				return std::optional<ReturnType>();
			});

			if (value.has_value())
			{
				glm::vec3 scale, translation, skew;
				glm::quat rotation;
				glm::vec4 perspective;
				glm::decompose(value->second, scale, rotation, translation, skew, perspective);

				auto rotatedUp = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
				auto rotatedForward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);

				m_Camera = Tempest::Definition::Camera(
					std::get<0>(value->first),
					std::get<2>(value->first),
					std::get<3>(value->first),
					std::get<1>(value->first),
					Common::Tempest::Vec3(translation.x, translation.y, translation.z),
					Common::Tempest::Vec3(rotatedForward.x, rotatedForward.y, rotatedForward.z),
					Common::Tempest::Vec3(rotatedUp.x, rotatedUp.y, rotatedUp.z)
				);
			}
		}
	}

	void GatherMeshMaterialCarIndices()
	{
		auto nodeStack = m_RootNodes;

		eastl::unordered_set<int> meshes, carIndices;
		eastl::set<int> materials; // we want this to be ordered

		while (!nodeStack.empty())
		{
			int firstNode = nodeStack.front();
			tinygltf::Node& node = m_Model.nodes[firstNode];
			for (int child : node.children)
			{
				nodeStack.push_back(child);
			}

			if (node.mesh != -1)
			{
				meshes.insert(node.mesh);
				for (const tinygltf::Primitive& primitive : m_Model.meshes[node.mesh].primitives)
				{
					if (primitive.material != -1)
					{
						materials.insert(primitive.material);
					}
				}
			}
			auto extension = node.extensions.find("TEMPEST_extension");
			if (extension != node.extensions.end())
			{
				assert(extension->second.IsObject());
				if (extension->second.Has("is_car"))
				{
					if (extension->second.Get("is_car").Get<bool>())
					{
						carIndices.insert(firstNode);
					}
				}
			}

			nodeStack.erase(nodeStack.begin());
		}

		m_Meshes.insert(m_Meshes.begin(), meshes.begin(), meshes.end());
		m_Materials.insert(m_Materials.begin(), materials.begin(), materials.end());
		m_CarIndices.insert(m_CarIndices.begin(), carIndices.begin(), carIndices.end());
	}


	glm::mat4 GetNodeTransform(int index)
	{
		glm::mat4 result;
		if (!m_Model.nodes[index].matrix.empty())
		{
			const auto& m = m_Model.nodes[index].matrix;
			result = glm::mat4(
				m[0], m[1], m[2], m[3],
				m[4], m[5], m[6], m[7],
				m[11], m[10], m[9], m[8],
				m[12], m[13], m[14], m[15]);
		}
		else
		{
			glm::mat4 scale = glm::identity<glm::mat4>();
			glm::mat4 translation = glm::identity<glm::mat4>();
			glm::mat4 rotation = glm::identity<glm::mat4>();

			if (!m_Model.nodes[index].translation.empty())
			{
				translation = glm::translate(glm::vec3(m_Model.nodes[index].translation[0], m_Model.nodes[index].translation[1], m_Model.nodes[index].translation[2]));
			}

			if (!m_Model.nodes[index].rotation.empty())
			{
				rotation = glm::mat4_cast(glm::quat(float(m_Model.nodes[index].rotation[0]), float(m_Model.nodes[index].rotation[1]), float(m_Model.nodes[index].rotation[2]), float(m_Model.nodes[index].rotation[3])));
			}

			if (!m_Model.nodes[index].scale.empty())
			{
				scale = glm::scale(glm::vec3(m_Model.nodes[index].scale[0], m_Model.nodes[index].scale[1], m_Model.nodes[index].scale[2]));
			}

			result = translation * rotation * scale;
		}

		return result;
	}

	template<typename T>
	std::optional<T> WalkNodes(int nodeIndex, const glm::mat4& parentTransform, eastl::function<std::optional<T>(const tinygltf::Model& model, int index, const glm::mat4& transform)> walkFunction)
	{
		glm::mat4 worldTransform = parentTransform * GetNodeTransform(nodeIndex);

		std::optional<T> returnValue = walkFunction(m_Model, nodeIndex, worldTransform);
		if (returnValue.has_value())
		{
			return returnValue;
		}

		for (const auto& child : m_Model.nodes[nodeIndex].children)
		{
			std::optional<T> data = WalkNodes(child, worldTransform, walkFunction);
			if (data.has_value())
			{
				return data;
			}
		}

		return std::optional<T>();
	}
public:
	tinygltf::Model m_Model;

	Tempest::Definition::Camera m_Camera;
	std::vector<int> m_RootNodes;
	std::vector<int> m_Meshes;
	std::vector<int> m_Materials;
	std::vector<int> m_CarIndices;
};