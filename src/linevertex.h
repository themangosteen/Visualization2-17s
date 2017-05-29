#pragma once

#include <glm/glm.hpp>

//! \brief The LineVertex struct.
//! Each line vertex has 8 floats: 3 pos, 3 direction to next, 2 uv for triangle strip texturing
struct LineVertex
{
	glm::vec3 pos; //!< vertex position
	glm::vec3 directionToNext; //!< direction to next vertex in line
	glm::vec2 uv; //!< vertex uv for drawing line as triangle strips (u along line direction, v perpendicular to direction)
};
