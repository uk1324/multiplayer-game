#include <math/polygon.hpp>
#include <math/lineSegment.hpp>

template<typename T>
static auto getWithWrapAround(const T& array, i64 index) -> auto {
	// TODO: Wrap around points like -2 or size + 2 correctly.
	if (index < 0) {
		index = array.size() - 1;
	} else if (index >= array.size()) {
		index = 0;
	}
	return array[index];
}

auto simplePolygonSignedArea(Span<const Vec2> verts) -> float {
	// Formula for signed area explained here https://gamemath.com/book/geomprims.html - 9.6.2 Area of a Triangle
	// This is called the shoelace formula. This formula and other formulas for calculating the area of a simple polygon are described here. 
	// https://en.wikipedia.org/wiki/Shoelace_formula
	// Determinant formula
	// https://mathworld.wolfram.com/PolygonArea.html
	if (verts.size() < 3) {
		return 0.0f;
	}
	auto twiceTheSignedArea = 0.0f;
	auto previous = verts.size() - 1 ;
	for (usize i = 1; i < verts.size(); i++) {
		twiceTheSignedArea += (verts[i].x - verts[previous].x) * (verts[i].y + verts[previous].y);
		previous = i;
	}

	return twiceTheSignedArea / 2.0f;
}

auto simplePolygonArea(Span<const Vec2> verts) -> float {
	return abs(simplePolygonSignedArea(verts));
}

auto simplePolygonIsClockwise(Span<const Vec2> verts) -> bool {
	// If the signed area is negative then the vertices are counter-clockwise.
	return simplePolygonSignedArea(verts) > 0.0f;
}

//auto lineSegmentIntersectsSimplePolygon(Vec2 start, Vec2 end, Span<const Vec2> verts) -> bool {
//	return false;
//}

// https://dyn4j.org/2021/06/2021-06-10-simple-polygon-simplification/
// This algorithm recursively takes the endpoints of the polygon and then finds the point most distant from the line (v[first], v[last]). Then if all the points are less than maxDistanceFromLine from the line then it removes them, because they don't add much to the shape. If there is at least one point that is further than maxDistanceFromLine from the line then it splits the line at that point and runs the algorithm on the 2 split parts.
auto polygonDouglassPeckerSimplify(Span<const Vec2> verts, float maxDistanceFromLine) -> std::vector<Vec2> {
	if (verts.size() <= 2) {
		return std::vector<Vec2>{ verts.begin(), verts.end() };
	}

	const auto& first = verts[0];
	const auto& last = verts[verts.size() - 1];

	auto maxDistance = -std::numeric_limits<float>::infinity();
	i64 maxVertex = 1;
	for (i64 i = 1; i < static_cast<i64>(verts.size()) - 1; i++) {
		const auto d = distance(Line{ first, last }, verts[i]);
		if (d > maxDistance) {
			maxDistance = d;
			maxVertex = i;
		}
	}

	if (maxDistance >= maxDistanceFromLine) {
		// If there is a point on the line (first, last) that is important (is more than maxDistanceFromLine distance from line then split at that point and apply the algorithm on the split parts.
		auto simplified = polygonDouglassPeckerSimplify(std::vector<Vec2>(verts.begin(), verts.begin() + maxVertex + 1), maxDistanceFromLine);
		const auto otherPart = polygonDouglassPeckerSimplify(std::vector<Vec2>(verts.begin() + maxVertex, verts.begin() + verts.size()), maxDistanceFromLine);
		simplified.insert(simplified.end(), otherPart.begin(), otherPart.end());
		return simplified;
	} else {
		// If all the points between first and last are really close to the line (first, last) then remove all of them.
		return std::vector<Vec2>{ first, last };
	}
}

// https://observablehq.com/@tmcw/understanding-point-in-polygon
// https://wrfranklin.org/Research/Short_Notes/pnpoly.html
// IMPLEMENTING SIMULATION OF SIMPLICITY FOR DEGENERACIES https://wrfranklin.org/p/234-implementing-sos-2022.pdf. Has some citations about triangulation.
auto isPointInPolygon(Span<const Vec2> verts, Vec2 p) -> bool {
	// Jordan curve theorem.
	bool crossedOddTimes = false;
	usize previous = verts.size() - 1;
	for (usize i = 0; i < verts.size(); i++) {
		const auto& a = verts[previous];
		const auto& b = verts[i];

		// Because the algorithms sends a horizontal ray the edges which it doesn't cross can be ignored. They actually have to be ignored, because the algorithm does ray vs line intersection and not ray vs line segment intersection.
		const auto pointYIsBetweenEdgeEndpointsY = (b.y > p.y) != (a.y > p.y); // The y's aren't both above or both below. One is above and one is below.

		// Intersection of the line going through a and b with the line y = p.y.
		// Determinant representation
		// (x2 - x1) * (y - y1) = (y2 - y1) * (x - x1)
		// (x2 - x1) * (p.y - y1) = x * (y2 - y1) - x1 * (y2 - y1)
		// x * (y2 - y1) = (x2 - x1) * (p.y - y1) + x1 * (y2 - y1)
		// x * (y2 - y1) = (x2 - x1) * (p.y - y1) + x1 * (y2 - y1)
		// x = (x2 - x1) * (p.y - y1) / (y2 - y1) + x1
		// x = -(x1 - x2) * (p.y - y1) / -(y1 - y2) + x1
		// This version is apparently better, because it handles cases like a line going through a vertex correctly.
		// x = (p.y * (x1 - x2) - y1 * (x1 - x2)) / (y1 - y2) + x1

		// (x2 - x1) * (y - y1) = (y2 - y1) * (x - x1)
		// Swapping x1 and x2 and y1 and y2 apparently makes it so the cases like the ray passing through an edge is handled correctly.
		// The solution without the swap is x = (x1 - x2) * (y - y1) / (y1 - y2) + x1. From what I checked this does change the result slightly.
		// (x1 - x2) * (y - y2) = (y1 - y2) * (x - x2)
		// (x1 - x2) * (y - y2) = x * (y1 - y2) - x2 * (y1 - y2)
		// x * (y1 - y2) = (x1 - x2) * (y - y2) + x2 * (y1 - y2)
		// pointYIsBetweenEdgeEndpointsY ensures that y2 != y1 => y2 - y1 != 0, because it can't be between if the ys are the same.
		// x = (x1 - x2) * (y - y2) / (y1 - y2) + x2
		// y = p.y
		// x = (x1 - x2) * (p.y - y2) / (y1 - y2) + x2

		// TODO: This can divide by zero, but when it does the result isn't used.
		const auto lineIntersectionX = (a.x - b.x) * (p.y - b.y) / (a.y - b.y) + b.x;
		
		// The ray goes from the right side to the left side so any intersections to the left of the point are not counted.
		const auto doesRayIntersect = p.x < lineIntersectionX;
		if (pointYIsBetweenEdgeEndpointsY && doesRayIntersect)
			crossedOddTimes = !crossedOddTimes;

		previous = i;
	}
	return crossedOddTimes;
}

auto simplePolygonContainsSimplePolygon(Span<const Vec2> enclosed, Span<const Vec2> container) -> bool {
	for (const auto& p : enclosed) {
		if (!isPointInPolygon(container, p)) {
			return false;
		}
	}
	return true;
}

auto removeHoles(std::vector<std::vector<Vec2>>& polygons, bool useTheFirstCutFound) -> void {
	for (int p = static_cast<int>(polygons.size()) - 1; p >= 0; p--) {
		if (simplePolygonIsClockwise(polygons[p]))
			continue;

		const auto holePolygonIndex = p;
		auto& hole = polygons[holePolygonIndex];

		int polygonContainingHoleIndex;
		for (int j = 0; j < polygons.size(); j++) {
			if (holePolygonIndex == j)
				continue;

			if (!simplePolygonIsClockwise(polygons[j]))
				continue;

			if (!simplePolygonContainsSimplePolygon(hole, polygons[j]))
				continue;
			
			polygonContainingHoleIndex = j;
			goto found;
		}
		return;
		found:

		auto& outside = polygons[polygonContainingHoleIndex];
		auto intersectsShape = [](const std::vector<Vec2>& shape, const LineSegment& line) -> bool {
			int previous = static_cast<int>(shape.size()) - 1;
			for (int i = 0; i < shape.size(); previous = i, i++) {
				LineSegment l{ shape[previous], shape[i] };
				auto endpoints = line.getCorners();
				auto lendpoints = l.getCorners();
				if (auto p = line.intersection(l); p.has_value()) {
					//return true;
					if (distance(*p, endpoints[0]) > 0.15f && distance(*p, endpoints[1]) > 0.15f
						&& distance(*p, lendpoints[0]) > 0.15f && distance(*p, lendpoints[1]) > 0.15f) {
						return true;
					}

					//if (distance(*p, endpoints[0]) > 0.15f && distance(*p, endpoints[1]) > 0.15f) {
					///*	Debug::drawPoint(*p);
					//	Debug::drawLine(endpoints[0], endpoints[1]);
					//	Debug::drawLine(lendpoints[0], lendpoints[1]);*/
					//	return true;
					//}
				}
			}
			return false;
		};

		// 2 closest points on both shapes so that the line from one to the other don't intersect either of the shapes.
		// The closest points have more coherence between frames.
		int foundInside = 0, foundOutside = 0;
		bool foundLine = false;
		float minDistance = std::numeric_limits<float>::infinity();
		for (int i = 0; i < hole.size(); i++) {
			for (int j = 0; j < outside.size(); j++) {
				auto dir = (outside[j] - hole[i]).normalized();
				LineSegment segment{ hole[i], outside[j] };
				const auto dist = distance(hole[i], outside[j]);
				if (dist > minDistance) {
					continue;
				}
				if (!intersectsShape(hole, segment) && !intersectsShape(outside, segment)) {
					auto intersectsSomeOtherShape = false;
					for (int k = 0; k < polygons.size(); k++) {
						if (k == holePolygonIndex || k == polygonContainingHoleIndex) {
							continue;
						}

						if (intersectsShape(polygons[k], segment)) {
							intersectsSomeOtherShape = true;
							break;
						}
					}
					if (intersectsSomeOtherShape) {
						continue;
					}
					minDistance = dist;
					foundInside = i;
					foundOutside = j;
					foundLine = true;
					if (useTheFirstCutFound) {
						goto lineFound;
					}
				}
			}
		}
		lineFound:
		if (foundLine) {
			const auto cutPoint = hole[foundInside];
			const auto cutPointOutside = outside[foundOutside];
			std::rotate(hole.begin(), hole.begin() + foundInside, hole.end());
			auto k = foundOutside + 1;
			if (k >= outside.size()) {
				k = 0;
			}
			outside.insert(outside.begin() + k, hole.begin(), hole.end());
			auto h = foundOutside + hole.size();
			if (h >= outside.size()) {
				h = 0;
			}
			outside.insert(outside.begin() + h + 1, cutPoint);
			outside.insert(outside.begin() + h + 2, cutPointOutside);
			polygons.erase(polygons.begin() + p);
		}
	}
}

auto complexPolygonOutline(Span<const Vec2> vertices, std::vector<Vec2>& result) -> const std::vector<Vec2>& {
	result.clear();
	if (vertices.size() <= 3) {
		result.insert(result.begin(), vertices.begin(), vertices.end());
		return result;
	}

	const auto leftmostVertex = std::min_element(vertices.begin(), vertices.end(), [](Vec2 a, Vec2 b) { return a.x < b.x; }) - vertices.begin();
	auto getIndex = [&](int i) -> int {
		if (i < 0) {
			i = vertices.size() - 1;
		}
		if (i >= vertices.size()) {
			i = 0;
		}
		return i;
	};

	auto get = [&](int i) -> Vec2 {
		return vertices[getIndex(i)];
	};

	// The leftmost index always lies on the shape's boundary. Any <direction>most point would do.
	const int startIndex = leftmostVertex;
	const auto a = get(startIndex - 1);
	const auto b = get(startIndex);
	const auto c = get(startIndex + 1);
	const auto mid = (a + c) / 2.0f;

	int direction;
	if (signedDistance(Line{ b, mid }, a) < 0.0f) {
		direction = 1;
	} else {
		direction = -1;
	}

	auto nextIndex = getIndex(startIndex + direction);
	Vec2 current = vertices[startIndex];
	std::optional<int> ignored0 = startIndex;
	int ignored01 = nextIndex;
	std::optional<int> ignored1;
	int ignored11 = 0;
	result.push_back(current);
	while (true) {
		const auto line = LineSegment{ current, vertices[nextIndex] };
		const auto sizeBefore = result.size();

		std::optional<Vec2> closestIntersection;
		float closestIntersectionDistance = std::numeric_limits<float>::infinity();
		int closestIntersectionLineStartIndex = 0;
		int closestIntersectionLineEndIndex = 0;

		int previousI = vertices.size() - 1;
		for (int i = 0; i < vertices.size(); previousI = i, i++) {
			// Skip lines that share an endpoint with the ray.
			if (ignored0.has_value()
				&& (i == ignored0 || previousI == ignored01 || i == ignored01 || previousI == ignored0)) {
				continue;
			}

			if (ignored1.has_value()
				&& ((i == ignored1 && previousI == ignored11) || (i == ignored11 && previousI == ignored1))) {
				continue;
			}
			const auto intersection = line.intersection(LineSegment{ vertices[i], vertices[previousI] });
			if (!intersection.has_value()) {
				continue;
			}

			const auto d = distance(*intersection, current);
			if (d < closestIntersectionDistance) {
				closestIntersectionDistance = d;
				closestIntersection = *intersection;
				closestIntersectionLineStartIndex = i;
				closestIntersectionLineEndIndex = previousI;
			}
		}

		if (closestIntersection.has_value()) {
			result.push_back(*closestIntersection);

			current = *closestIntersection;
			ignored1 = ignored0;
			ignored11 = ignored01;

			if (signedDistance(line.line, vertices[closestIntersectionLineStartIndex]) > 0.0f) {
				direction = 1;
				nextIndex = getIndex(closestIntersectionLineStartIndex);
			} else {
				direction = -1;
				nextIndex = getIndex(closestIntersectionLineEndIndex);
			}
			ignored0 = nextIndex;
			ignored01 = getIndex(nextIndex - direction);
		} else {
			result.push_back(vertices[nextIndex]);
			ignored0 = nextIndex;

			if (nextIndex == startIndex) {
				break;
			}
			current = vertices[nextIndex];
			nextIndex = getIndex(nextIndex + direction);

			ignored01 = nextIndex;
			ignored1 = std::nullopt;
		}


		if (result.size() == sizeBefore) {
			break;
		}
	}
	return result;
}
