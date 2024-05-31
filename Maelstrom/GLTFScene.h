#pragma once

#include <DataDefinitions/Level_generated.h>

#include <fstream>
#include <filesystem>

#include <EASTL/unordered_set.h>
#include <EASTL/set.h>
#include <EASTL/optional.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

struct TRS
{
	glm::quat Rotation;
	glm::vec3 Translation;
	glm::vec3 Scale;

	TRS(const glm::mat4& mat)
	{
        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::decompose(mat, scale, rotation, translation, skew, perspective);

		Rotation = glm::quat::wxyz(rotation.w, rotation.x, -rotation.y, -rotation.z);
		Translation = glm::vec3(-translation.x, translation.y, translation.z);
		Scale = scale;
	}
};

class Scene
{
public:
	Scene(const std::filesystem::path& sceneFile)
	{
		cgltf_options options = { 0 };
		cgltf_result result = cgltf_parse_file(&options, sceneFile.string().c_str(), &m_Data);
		assert(result == cgltf_result_success);
		result = cgltf_load_buffers(&options, m_Data, sceneFile.string().c_str());
		assert(result == cgltf_result_success);

		ExtractCamera();
		GatherMeshMaterialCarIndices();
	}

	~Scene()
	{
		cgltf_free(m_Data);
	}

	uint32_t MeshPrimitiveCount(int meshIndex) const
	{
		return uint32_t(m_Meshes[meshIndex]->primitives_count);
	}

	eastl::vector<uint32_t> MeshPositionCountPerPrimitive(int meshIndex) const
	{
		eastl::vector<uint32_t> results;
		auto numPrimitives = m_Meshes[meshIndex]->primitives_count;
		results.resize(numPrimitives);

		for (auto i = 0; i < numPrimitives; ++i)
		{
			cgltf_primitive* primitive = m_Meshes[meshIndex]->primitives + i;
			for (int x = 0; x < primitive->attributes_count; ++x)
			{
				if (primitive->attributes[x].type == cgltf_attribute_type_position)
				{
					results[i] = uint32_t(primitive->attributes[x].data->count);
				}
			}
		}

		return results;
	}

	eastl::optional<eastl::vector<uint32_t>> MeshIndices(int meshIndex, int primIndex) const
	{
		cgltf_primitive* primitive = m_Meshes[meshIndex]->primitives + primIndex;
		if (primitive->indices)
		{
			eastl::vector<uint32_t> outData(primitive->indices->count);

			for (int i = 0; i < outData.size(); ++i)
			{
				outData[i] = uint32_t(cgltf_accessor_read_index(primitive->indices, i));
			}

			return eastl::optional(outData);
		}
		return eastl::nullopt;
	}

	eastl::vector<glm::vec3> MeshPositions(int meshIndex, int primIndex) const
	{
		eastl::vector<glm::vec3> outData;
		cgltf_primitive* primitive = m_Meshes[meshIndex]->primitives + primIndex;
		for (int attribute = 0; attribute < primitive->attributes_count; ++attribute)
		{
			if (primitive->attributes[attribute].type == cgltf_attribute_type_position)
			{
				cgltf_accessor* acc = primitive->attributes[attribute].data;
				outData.resize(acc->count);
				for (int i = 0; i < acc->count; ++i)
				{
					float vertex[3];
					cgltf_accessor_read_float(acc, i, vertex, 3);
					outData[i] = glm::vec3(vertex[0], vertex[1], vertex[2]);
				}
				break;
			}
		}

		return outData;
	}

    eastl::vector<glm::vec3> MeshNormals(int meshIndex, int primIndex) const
    {
        eastl::vector<glm::vec3> outData;
        cgltf_primitive* primitive = m_Meshes[meshIndex]->primitives + primIndex;
        for (int attribute = 0; attribute < primitive->attributes_count; ++attribute)
        {
            if (primitive->attributes[attribute].type == cgltf_attribute_type_normal)
            {
                cgltf_accessor* acc = primitive->attributes[attribute].data;
                outData.resize(acc->count);
                for (int i = 0; i < acc->count; ++i)
                {
                    float normal[3];
                    cgltf_accessor_read_float(acc, i, normal, 3);
                    outData[i] = glm::vec3(normal[0], normal[1], normal[2]);
                }
                break;
            }
        }

        return outData;
    }

    eastl::vector<glm::vec2> MeshUVs(int meshIndex, int primIndex) const
    {
        eastl::vector<glm::vec2> outData;
        cgltf_primitive* primitive = m_Meshes[meshIndex]->primitives + primIndex;
        for (int attribute = 0; attribute < primitive->attributes_count; ++attribute)
        {
            if (primitive->attributes[attribute].type == cgltf_attribute_type_texcoord)
            {
                cgltf_accessor* acc = primitive->attributes[attribute].data;
                outData.resize(acc->count);
                for (int i = 0; i < acc->count; ++i)
                {
                    float uv[2];
                    cgltf_accessor_read_float(acc, i, uv, 2);
                    outData[i] = glm::vec2(uv[0], uv[1]);
                }
                break;
            }
        }

        return outData;
    }

	uint32_t MeshMaterialIndex(int meshIndex, int primIndex) const
	{
		cgltf_primitive* primitive = m_Meshes[meshIndex]->primitives + primIndex;
		return uint32_t(eastl::distance(m_Materials.begin(), eastl::find(m_Materials.begin(), m_Materials.end(), primitive->material)));
	}

private:
	void ExtractCamera()
	{
		for (int i = 0; i < m_Data->scene->nodes_count; ++i)
		{
			using ReturnType = std::pair<std::tuple<float, float, float, float>, glm::mat4>;
			auto value = WalkNodes<ReturnType>(m_Data->scene->nodes[i], glm::identity<glm::mat4>(), [](const cgltf_data* data, cgltf_node* node, const glm::mat4& transform) {
				if (node->camera && node->camera->type == cgltf_camera_type_perspective)
				{
					ReturnType camera(std::make_pair(std::make_tuple(
						float(node->camera->data.perspective.yfov),
						float(node->camera->data.perspective.has_aspect_ratio ? node->camera->data.perspective.aspect_ratio : 16.0f / 9.0f),
						float(node->camera->data.perspective.znear),
						float(node->camera->data.perspective.has_zfar ? node->camera->data.perspective.zfar : 1000.0f)
					), transform));
					return eastl::optional(camera);
				}
				return eastl::optional<ReturnType>();
			});

			if (value.has_value())
			{
				TRS trs(value->second);

				auto rotatedUp = trs.Rotation * glm::vec3(0.0f, 1.0f, 0.0f);
				auto rotatedForward = trs.Rotation * glm::vec3(0.0f, 0.0f, -1.0f);

				m_Camera = Tempest::Definition::Camera(
					std::get<0>(value->first),
					std::get<2>(value->first),
					std::get<3>(value->first),
					std::get<1>(value->first),
					Common::Tempest::Vec3(trs.Translation.x, trs.Translation.y, trs.Translation.z),
					Common::Tempest::Vec3(rotatedForward.x, rotatedForward.y, rotatedForward.z),
					Common::Tempest::Vec3(rotatedUp.x, rotatedUp.y, rotatedUp.z)
				);
			}
		}
	}

	void GatherMeshMaterialCarIndices()
	{
		eastl::vector<cgltf_node*> nodeStack(m_Data->scene->nodes_count);
		for (int i = 0; i < nodeStack.size(); ++i)
		{
			nodeStack[i] = m_Data->scene->nodes[i];
		}

		eastl::unordered_set<cgltf_mesh*> meshes;
		eastl::unordered_set<cgltf_node*> carIndices;
		eastl::set<cgltf_material*> materials; // we want this to be ordered

		while (!nodeStack.empty())
		{
			cgltf_node* firstNode = nodeStack.front();
			for (int i = 0; i < firstNode->children_count; ++i)
			{
				nodeStack.push_back(firstNode->children[i]);
			}

			if (firstNode->mesh)
			{
				meshes.insert(firstNode->mesh);
				for (int i = 0; i < firstNode->mesh->primitives_count; ++i)
				{
					if (firstNode->mesh->primitives[i].material)
					{
						materials.insert(firstNode->mesh->primitives[i].material);
					}
				}
			}
			// TODO: we need JSON parsing here
			//auto extension = node.extensions.find("TEMPEST_extension");
			//if (extension != node.extensions.end())
			//{
			//	assert(extension->second.IsObject());
			//	if (extension->second.Has("is_car"))
			//	{
			//		if (extension->second.Get("is_car").Get<bool>())
			//		{
			//			carIndices.insert(firstNode);
			//		}
			//	}
			//}

			nodeStack.erase(nodeStack.begin());
		}

		m_Meshes.insert(m_Meshes.begin(), meshes.begin(), meshes.end());
		m_Materials.insert(m_Materials.begin(), materials.begin(), materials.end());
		m_CarIndices.insert(m_CarIndices.begin(), carIndices.begin(), carIndices.end());
	}

	glm::mat4 GetNodeTransform(cgltf_node* node) const
	{
		float m[16];
		cgltf_node_transform_local(node, m);

        return glm::mat4(
            m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7],
            m[8], m[9], m[10], m[11],
            m[12], m[13], m[14], m[15]);
    }

	template<typename T>
	eastl::optional<T> WalkNodes(cgltf_node* node, const glm::mat4& parentTransform, eastl::function<eastl::optional<T>(const cgltf_data* data, cgltf_node* node, const glm::mat4& transform)> walkFunction) const
	{
		glm::mat4 worldTransform = parentTransform * GetNodeTransform(node);

		eastl::optional<T> returnValue = walkFunction(m_Data, node, worldTransform);
		if (returnValue.has_value())
		{
			return returnValue;
		}

		for (uint32_t i = 0; i < node->children_count; ++i)
		{
			eastl::optional<T> data = WalkNodes(node->children[i], worldTransform, walkFunction);
			if (data.has_value())
			{
				return data;
			}
		}

		return eastl::nullopt;
	}

public:
	template<typename T>
	eastl::optional<T> WalkRootNodes(eastl::function<eastl::optional<T>(const cgltf_data* data, cgltf_node* node, const glm::mat4& transform)> walkFunction) const
	{
		for (int i = 0; i < m_Data->scene->nodes_count; ++i)
		{
			auto returnValue = WalkNodes(
				m_Data->scene->nodes[i],
				glm::identity<glm::mat4>(),
				walkFunction);
			if (returnValue.has_value())
			{
				return returnValue;
			}
		}

		return eastl::nullopt;
	}
public:
	cgltf_data* m_Data;

	Tempest::Definition::Camera m_Camera;
	eastl::vector<cgltf_mesh*> m_Meshes;
	eastl::vector<cgltf_material*> m_Materials;
	eastl::vector<cgltf_node*> m_CarIndices;
};