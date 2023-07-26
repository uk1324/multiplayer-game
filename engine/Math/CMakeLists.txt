add_library(
	math
	"Aabb.cpp" "Line.cpp" "LineSegment.cpp"  "Polygon.cpp" "Transform.cpp" "Triangulate.cpp" "Triangle.cpp" "Utils.cpp"
 "Color.cpp" "Random.cpp")

target_include_directories(math PUBLIC "../Utils")