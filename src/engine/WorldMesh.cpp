#include "WorldMesh.h"
#include <zenload/zTypes.h>
#include "World.h"

using namespace World;



WorldMesh::WorldMesh(World::WorldInstance& parentWorld)
	: m_ParentWorld(parentWorld)
{

}

void WorldMesh::load(const ZenLoad::PackedMesh& in)
{
	m_WorldMeshData = in;
}

float WorldMesh::interpolateTriangleShadowValue(size_t triangleIdx, const Math::float3 &worldPosition)
{
	assert(triangleIdx < m_WorldMeshData.triangles.size());

	ZenLoad::WorldTriangle& tri = m_WorldMeshData.triangles[triangleIdx];

	float u,v,w;
	Math::barycentric(worldPosition, tri.vertices[0].Position.v, tri.vertices[1].Position.v, tri.vertices[2].Position.v, u, v, w);

	Math::float4 c[3];
	c[0].fromABGR8(tri.vertices[0].Color);
	c[1].fromABGR8(tri.vertices[1].Color);
	c[2].fromABGR8(tri.vertices[2].Color);

	return (u * c[0] + v * c[1] + w * c[2]).x; // Lighting is greyscale only
}

void WorldMesh::getTriangle(size_t triangleIdx, Math::float3 *v3)
{
	assert(triangleIdx < m_WorldMeshData.triangles.size());

	ZenLoad::WorldTriangle& tri = m_WorldMeshData.triangles[triangleIdx];

	for(int i=0;i<3;i++)
		v3[i] = Math::float3(tri.vertices[i].Position.v);
}
