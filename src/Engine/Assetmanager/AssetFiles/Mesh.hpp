#ifndef MESH_HPP
#define MESH_HPP

#include"GpuResource.hpp"
#include <string>

class Mesh : public GpuResource
{

private:
	std::string m_Name;
	unsigned int m_VerticesCount = 0;

public:
	inline Mesh(const std::string& Name, unsigned int verticesCount) noexcept : GpuResource(Mesh_Type), m_Name(Name), m_VerticesCount(verticesCount) {}
	virtual ~Mesh() = default;

	inline const std::string& GetName() noexcept{ return m_Name; }
	inline const unsigned int& GetVertexCount() noexcept{ return m_VerticesCount; }

};

#endif // !MESH_HPP

