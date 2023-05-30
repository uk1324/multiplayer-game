#pragma once

#include <math/triangle.hpp>
#include <utils/span.hpp>

#include <vector>

struct SimplePolygonTriangulator {
	/*
	There can be numerical issues for example when the data is to aligned to a grid (for example vertices from marching squares on a pixel image. This can create cases when the intermediate triangulated polygon's point lies on one of the lines. Example of this can be seen in assets/example1.jpg. The issuse in the image was caused by the function checking if a point is inside a triangle not considering points on the boundary as inside. The issue is that doing the correct point in triangle test can also cause a lot of issues. When running the Bad Apple test ("Mesh simplification" commit) with the test that considers points on the boundary as inside it creates a lot of glitches ("Triangulation glitches demo" commit). This is probably happening because of the cuts to make holes, which create a lot of overlapping points.
	One way to deal with this issue would be to actually perform the intersection test instead of just checking if there are any points inside to find if there are intersections, but there can be numerical issues with this approach as well as described in the line segment vs polygon intersection test in math/polygon.hpp.
	TODO: For improving the quality of the marching squares data could probably offset the vertices maybe based on the triangle they form with the neighbouring vertices or some other heuristic.
	TODO: Mesh simplification algorithms can also create some small self intersections with can completely break an image. This happens on one frame of the bad apple triangulation (frame = i = 408) (This self intersection was also caused because of the hole cutting algorithm so this has to also be considered when finding that algorithm, could try making the hole cuts wider). Find some algorithm to remove the self intersections, which also considers the previous issues like the hole cuts.
	Actually this self intersection was probably caused by the intersectsShape function checking endpoints with epsilons. Disabling it fixes this example ("Epsilon check bugs" commit), but there are probably cases when it would make it impossible to find a hole cut.
	Another numerical issue which happens even with the correct point in triangle test is zero area triangles. One example can be seen in assets/example2.jpg. The diagonal going from the bottom left to the top right actually has a point in the middle so this creates a zero area triangle (bottom left, top right, middle). 
	TODO: To fix this could probably skip triangles that are zero area. This would still create weird triangulations because the midpoint has to be considered as a vertex.

	In the bad apple video ("Mesh simplification" committ) not removing the colinear points in marching squares made the results have more coherency between frames for some reason.
	*/
	// The resulting triangles are clockwise.
	auto operator()(Span<const Vec2> vertices) -> const std::vector<Triangle>&;

private:
	std::vector<Vec2> verts;
	std::vector<Triangle> result;
};