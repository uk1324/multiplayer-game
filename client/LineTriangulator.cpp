#include <client/LineTriangulator.hpp>
#include <imgui/imgui.h>
#include <engine/Math/LineSegment.hpp>
#include <engine/Math/Utils.hpp>
#include <engine/Math/Transform.hpp>
#include <client/Debug.hpp>

LineTriangulator::Result LineTriangulator::operator()(std::span<const Vec2> line) {
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
	if (line.size() < 2) {
		return { vertices, indices };
	}

	auto addQuad = [&vertices](Vec2 v0, Vec2 v1, Vec2 v2, Vec2 v3, float z) {
		/*const int v[] = { 0, 1, 2, 2, 1, 3 };
		for (const auto a : v) {

		}*/
		vertices.push_back(Vertex{ Vec3(v0.x, v0.y, vertices.size()), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vertex{ Vec3(v1.x, v1.y, vertices.size()), Vec2(0.0f, 1.0f) });
		vertices.push_back(Vertex{ Vec3(v2.x, v2.y, vertices.size()), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vertex{ Vec3(v1.x, v1.y, vertices.size()), Vec2(0.0f, 1.0f) });
		vertices.push_back(Vertex{ Vec3(v2.x, v2.y, vertices.size()), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vertex{ Vec3(v3.x, v3.y, vertices.size()), Vec2(0.0f, 1.0f) });

		//vertices.push_back(Vertex{ Vec3(v0.x, v0.y, z), Vec2(0.0f, 0.0f) });
		//vertices.push_back(Vertex{ Vec3(v1.x, v1.y, z), Vec2(0.0f, 1.0f) });
		//vertices.push_back(Vertex{ Vec3(v2.x, v2.y, z), Vec2(0.0f, 0.0f) });
		//vertices.push_back(Vertex{ Vec3(v1.x, v1.y, z), Vec2(0.0f, 1.0f) });
		//vertices.push_back(Vertex{ Vec3(v2.x, v2.y, z), Vec2(0.0f, 0.0f) });
		//vertices.push_back(Vertex{ Vec3(v3.x, v3.y, z), Vec2(0.0f, 1.0f) });

		/*vertices.push_back(Vertex{ Vec3(v0.x, v0.y, 0.0f), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vertex{ Vec3(v1.x, v1.y, 0.0f), Vec2(0.0f, 1.0f) });
		vertices.push_back(Vertex{ Vec3(v2.x, v2.y, 0.0f), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vertex{ Vec3(v1.x, v1.y, 0.0f), Vec2(0.0f, 1.0f) });
		vertices.push_back(Vertex{ Vec3(v2.x, v2.y, 0.0f), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vertex{ Vec3(v3.x, v3.y, 0.0f), Vec2(0.0f, 1.0f) });*/
	};

	/*auto addLine = [&](Vec2 s, Vec2 e) {
		addQuad(s, e, s, e);
	};*/

	static float width = 0.1f;
	ImGui::SliderFloat("width", &width, 0.0f, 1.0f);
	Vec2 previousV1(0.0f);
	Vec2 previousV3(0.0f);
	static bool a = true;
	ImGui::Checkbox("a", &a);

	static bool b = true;
	ImGui::Checkbox("b", &b);

	static bool color = true;
	ImGui::Checkbox("color", &color);
	for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
		const auto current = line[i];
		const auto next = line[i + 1];
		const auto currentToNext = (next - current);
		const auto normalToLine0 = currentToNext.rotBy90deg().normalized();

		const auto v0 = current + normalToLine0 * width;
		auto v1 = v0 + currentToNext;
		const auto v2 = current - normalToLine0 * width;
		auto v3 = v2 + currentToNext;

		if (i == line.size() - 2) {
			addQuad(previousV1, previousV3, v1, v3, 0.0f);
			break;
		}

		const auto nextNext = line[i + 2];
		const auto nextToNextNext = (nextNext - next);
		const auto normalToLine1 = nextToNextNext.rotBy90deg().normalized();

		const auto nextV0 = next + normalToLine1 * width;
		const auto nextV1 = nextV0 + nextToNextNext;
		const auto nextV2 = next - normalToLine1 * width;
		const auto nextV3 = nextV2 + nextToNextNext;

		float z = cross(currentToNext, nextToNextNext) < 0.0f ? 1.0 : -1.0f;

		const auto intersection0 = intersectLineSegments(v0, v1, nextV0, nextV1);
		const auto intersection1 = intersectLineSegments(v2, v3, nextV2, nextV3);

		Vec2 newV1;
		Vec2 newV3;
		auto roundJoin = [&](Vec2 start, Vec2 end, Vec2 around, int count, Vec2 p, float startTextureY, float aroundTextureY) {
			auto startAngle = (start - around).angle();
			auto endAngle = (end - around).angle();
			auto angleRange = std::abs(endAngle - startAngle);
			auto angleStep = angleRange / count;
			if (angleRange > PI<float>) {
				angleRange = TAU<float> -angleRange;
				angleStep = -(angleRange / count);
			}
			if (startAngle > endAngle) {
				angleStep = -angleStep;
			}

			if (abs(endAngle - (startAngle + angleStep * count)) > 0.01f) {
				int x = 5;
			}

			Rotation rotationStep(angleRange / count);
			Vec2 previous = start;
			Vec2 r = start - around;
			//auto angleStep = angleRange / count;
			const auto length = (around - start).length();
			for (int i = 0; i < count; i++) {
				//vertices.push_back({ around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length });*/
				const auto v0 = around + Vec2::oriented(startAngle + i * angleStep) * length;
				const auto v1 = around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length;
				vertices.push_back({ Vec3(v0.x, v0.y, vertices.size()), Vec2(0.0f, startTextureY)});
				vertices.push_back({ Vec3(p.x, p.y, vertices.size()), Vec2(0.0f, aroundTextureY)});
				vertices.push_back({ Vec3(v1.x, v1.y, vertices.size()), Vec2(0.0f, startTextureY)});

				//if (aroundTextureY == 1.0f) {
				//	vertices.push_back({ Vec3(v0.x, v0.y, 1.0f), Vec2(0.0f, startTextureY) });
				//	vertices.push_back({ Vec3(p.x, p.y, 1.0f), Vec2(0.0f, aroundTextureY) });
				//	vertices.push_back({ Vec3(v1.x, v1.y, 1.0f), Vec2(0.0f, startTextureY) });
				//} else {
				//	vertices.push_back({ Vec3(v0.x, v0.y, -1.0f), Vec2(0.0f, startTextureY) });
				//	vertices.push_back({ Vec3(p.x, p.y, -1.0f), Vec2(0.0f, aroundTextureY) });
				//	vertices.push_back({ Vec3(v1.x, v1.y, -1.0f), Vec2(0.0f, startTextureY) });
				//}
				
			}
		};

		Vec2 up;
		Vec2 down;
		if (intersection0.has_value()) {
			newV1 = *intersection0;
			roundJoin(v3, nextV2, next, 5, newV1, 1.0f, 0.0f);
			down = newV1;
			up = nextV2;
			/*newV3 = nextV2;*/
			newV3 = v3;
		} else if (intersection1.has_value()) {
			newV3 = *intersection1;
			newV1 = v1;
			roundJoin(v1, nextV0, next, 5, newV3, 0.0f, 1.0f);
			up = newV3;
			down = nextV0;
		} else {
			newV1 = v1;
			newV3 = v3;
			down = nextV0;
			up = nextV2;
			const auto v3io = Line(v2, v3).intersection(Line(nextV2, nextV3));
			const auto v1io = Line(v0, v1).intersection(Line(nextV0, nextV1));
			if (!v3io.has_value() || !v1io.has_value()) {
				ImGui::Text("error");
				continue;
			}

			if (signedDistance(Line(v3, v3 + normalToLine0), *v3io) < 0.0f) {
				roundJoin(v1, nextV0, next, 5, next, 0.0f, 0.5f);
			} else {
				roundJoin(v3, nextV2, next, 5, next, 1.0f, 0.5f);
			}

		}


		if (a) {
			if (i == 0) {
				addQuad(v0, v2, newV1, newV3, z);
			} else {
				addQuad(previousV1, previousV3, newV1, newV3, z);
			}
		}
		previousV1 = down;
		previousV3 = up;
	}
	return Result{ vertices, indices };
}