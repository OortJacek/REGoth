#pragma once
#include <engine/WorldTypes.h>
#include <content/StaticLevelMesh.h>

namespace ZenLoad
{
	struct PackedMesh;
}

namespace World
{
	class WorldInstance;
	typedef LevelMesh::StaticLevelMesh<WorldMeshVertex, WorldMeshIndex> WorldMeshData;
	
	class WorldMesh
	{
	public:
		WorldMesh(World::WorldInstance& parentWorld);

		/**
		 * @brief Loads the worldmesh from the given Packed-Mesh
		 */
		void load(const ZenLoad::PackedMesh& in);

		/**
		 * Interpolates the color on of the vertices on the triangle with the given index
		 * @param triangleIdx Triangle to use for the calculation
		 * @param worldPosition Position to interpolate to (in world-coords)
		 * @return Interpolated color-value on the given triangle
		 */
		float interpolateTriangleShadowValue(size_t triangleIdx, const Math::float3& worldPosition);

		/**
		 * Returns the vertices of the given triangle
		 * @param triangleIdx Triangle to get the vertices from
		 * @param v3 pointer to array fo 3 Math::float3s
		 */
		void getTriangle(size_t triangleIdx, Math::float3* v3);

	protected:

		/**
		 * Data of the worldmesh
		 */
		ZenLoad::PackedMesh m_WorldMeshData;

		/**
		 * Reference to the parentworld
		 */
		World::WorldInstance& m_ParentWorld;
	};
}