#pragma once

#include "Model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <memory>

struct TransformComponent
{
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
	glm::vec3 rotation{};

	glm::mat4 getTransformationMatrix();
	glm::mat3 getNormalMatrix();
};

class GameObject
{
public:
	using id_t = unsigned int;

	std::shared_ptr<Model> model;
	glm::vec3 color{};

	TransformComponent transform{};

private:
	id_t m_id;

public:
	static GameObject CreateGameObject()
	{
		static id_t currentId = 0;
		return GameObject(currentId++);
	}

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	id_t getId() const { return m_id; }

private:
	GameObject(id_t objId): m_id(objId) {}
};