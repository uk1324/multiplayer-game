#pragma once

#include <math/vec2.hpp>
#include <utils/span.hpp>

#include <vector>

auto simplePolygonSignedArea(Span<const Vec2> verts) -> float;
auto simplePolygonArea(Span<const Vec2> verts) -> float;
auto simplePolygonIsClockwise(Span<const Vec2> verts) -> bool;

// It might not make sense to cut holes inside simplex polygons, because if the 2 shapes overlap it might be better to just subtract the holes shape from the outside shape.

// https://cs.stackexchange.com/questions/43758/turning-a-simple-polygon-with-holes-into-exterior-bounded-only
auto lineSegmentIntersectsSimplePolygon(Vec2 start, Vec2 end, Span<const Vec2> verts) -> bool;

// Can return zero area shapes.
// Can create self intersections.
auto polygonDouglassPeckerSimplify(Span<const Vec2> verts, float maxDistanceFromLine) -> std::vector<Vec2>;

// https://en.wikipedia.org/wiki/Linear_equation
// Different representations of lines
// y = mx + n
// m = (y2 - y1) / (x2 - x1)
// y1 = m * x1 + n
// n = y1 - m * x1
// Slope intercept form
// y = m * x + y1 - m * x1
// y - y1 = m * (x - x1)
// y - y1 = (y2 - y1) / (x2 - x1) * (x - x1)
// Even though there is a multiplication by zero it can be verified that it is also gives the correct result for x1 = x2. This is shown below.
// (x2 - x1) * (y - y1) = (y2 - y1) * (x - x1)
// Determinant form. This just means the the line from endpoint 1 to endpoint 2 is colinear with the line from endpoint 1 to the point (x, y).
// (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1) = 0
// y * (x2 - x1) - y1 * (x2 - x1) - (x * (y2 - y1) - x1 * (y2 - y1)) = 0
// y * (x2 - x1) - y1 * (x2 - x1) - x * (y2 - y1) + x1 * (y2 - y1) = 0
// y * (x2 - x1) - y1 * (x2 - x1) - x * (y2 - y1) + x1 * (y2 - y1) = 0
// y * (x2 - x1) - (y1 * x2 - y1 * x1) - x * (y2 - y1) + x1 * y2 - x1 * y1 = 0
// y * (x2 - x1) - y1 * x2 + y1 * x1 - x * (y2 - y1) + x1 * y2 - x1 * y1 = 0
// y * (x2 - x1) - y1 * x2 - x * (y2 - y1) + x1 * y2 = 0
// -x * (y2 - y1) + y * (x2 - x1) + (x1 * y2 - y1 * x2) = 0
// Normal offset form
// x * (y1 - y2) + y * (x2 - x1) + (x1 * y2 - y1 * x2) = 0

// This formula doesn't work when x1 = x2 = h and y1 = y2 = g. There are infinitely many lines going through a single point.
// x * (y1 - y2) + y * (x2 - x1) + (x1 * y2 - y1 * x2) = 0
// x * (g - g) + y * (h - h) + (h * g - g * h) = 0
// 0 = 0

// h = x1 = x2 and y1 != y2
// x * (y1 - y2) + y * (h - h) + (h * y2 - y1 * h) = 0
// x * (y1 - y2) + y * 0 + h * (y2 - y1) = 0
// x * (y1 - y2) - h * (y1 - y2) = 0
// (x - h) * (y1 - y2) = 0
// y1 != y2
// y1 - y2 != 0
// x - h = 0
// x = h

auto isPointInPolygon(Span<const Vec2> verts, Vec2 p) -> bool;
auto simplePolygonContainsSimplePolygon(Span<const Vec2> enclosed, Span<const Vec2> container) -> bool;

// The outside shapes are clockwise and the holes are counterclockwise.
// https://cs.stackexchange.com/questions/43758/turning-a-simple-polygon-with-holes-into-exterior-bounded-only
// An issue that is not handled is the order in which the holes are removed. Consider the case of a square with 5 holes; 4 near the corners and one in the center (something like the side of a die with a 5). If you try to remove the center hole first then you won't be able to because there is no way to access the outside shape without intersecting other holes. First one of the corner holes has to be removed and only then can the center hole be removed.
// It might be better to not remove holes, but just to do a subract operation on the shape, because some simplification algorithms might produce shapes that intersect with holes, which means that the shape is not contained inside any other shape so these shapes are ignored by this algorithm.
// Using the smallest cut possible produces better result and doesn't lead to glitchy cuts as often. So it is better to set useTheFirstCutFound = false, but this does slow down the algorithm quite a bit.
auto removeHoles(std::vector<std::vector<Vec2>>& polygons, bool useTheFirstCutFound = true) -> void;

namespace { static thread_local std::vector<Vec2> complexPolygonOtlineResult; }
auto complexPolygonOutline(Span<const Vec2> vertices, std::vector<Vec2>& result = complexPolygonOtlineResult) -> const std::vector<Vec2>&;