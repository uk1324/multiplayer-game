#include <math/triangulate.hpp>
#include <math/polygon.hpp>

// https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
auto SimplePolygonTriangulator::operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>& {
	if (vertices.size() < 3) {
		ASSERT_NOT_REACHED();
		result.clear();
		return result;
	}

	verts.clear();
	for (auto& vert : vertices)
		verts.push_back(vert);

	if (!simplePolygonIsClockwise(vertices))
		std::reverse(verts.begin(), verts.end());

	result.clear();
	while (verts.size() > 3) {
		auto sizeAtStartOfLoop = verts.size();
		// Could try to improve the triangulations (for example avoid cases where many triangles share a single vertex, which creates a lot of small area triangles) by trying to find the biggest area ear triangle (which may later cause problems, because the later triangles might have small area) or maybe try randomizing which vertex to first check for being an ear.
		for (usize i = 0; i < verts.size(); i++) {
			const auto leftIndex = i == 0 ? verts.size() - 1 : i - 1;
			const auto rightIndex = i == verts.size() - 1 ? 0 : i + 1;
			auto& left = verts[leftIndex];
			auto& right = verts[rightIndex];
			// The algorithm makes sure that the vertices are in clockwise order. When they are the then if you take some point and make a line connecting it's neighbours if the lie lies inside the shape then the determinant of the lines is positive and nagative is it lies outside the shape. For example look at an L shape. Connecting the 2 endpoints creates creates a line that lies outside the shape. Because the top part is the first vertex and the bottom part is the second this just forms a flipped pair of basis, which means that the determinant has to be nagative.
			const auto lineLiesOutsideOfTheShape = det(left - verts[i], right - verts[i]) < 0.0f;
			if (lineLiesOutsideOfTheShape)
				continue;

			Triangle triangle{ right, left, verts[i] };

			// Check if the triangle doesn't intersect any other lines of the shape, which means that if it also lies inside the shape it is an ear.
			// The exact definition of ear https://en.wikipedia.org/wiki/Vertex_(geometry)#Principal_vertex.
			// Basically for the triangle to intersect with other line segments of the shape there would need to be some vertex of that shape inside the triangle.
			for (usize j = 0; j < verts.size(); j++) {
				if (j == i || j == leftIndex || j == rightIndex)
					continue;

				if (triangle.contains(verts[j]))
					goto isNotAnEar;
			}

			// @Performance: Could use a linked list or iterate backwards to improve the removal times. Iterating backwards might acutally make it slower in some cases probably.
			verts.erase(verts.begin() + i);
			result.push_back(triangle);
			// This break is there to prevent iterator invalidation when a vertex is removed. Tried iterating backwards, but this doesn't seem to make the triangulations any nicer.
			break;

		isNotAnEar:
			continue;
		}

		if (verts.size() == sizeAtStartOfLoop) {
			// Happens when given invalid data or maybe when some intermendiate step produces invalid data.
			return result;
		}
	}

	result.push_back(Triangle{ verts[0], verts[1], verts[2] });

	return result;
}