#include <client/LineTriangulator.hpp>
#include <imgui/imgui.h>
#include <engine/Math/LineSegment.hpp>
#include <engine/Math/Utils.hpp>
#include <engine/Math/Transform.hpp>
#include <client/Debug.hpp>

LineTriangulator::Result LineTriangulator::operator()(std::span<const Vec2> line) {
	std::vector<Vert> vertices;
	std::vector<u32> indices;
	if (line.size() < 2) {
		return { vertices, indices };
	}

	auto addQuad = [&vertices](Vec2 v0, Vec2 v1, Vec2 v2, Vec2 v3, float z) {
		/*const int v[] = { 0, 1, 2, 2, 1, 3 };
		for (const auto a : v) {

		}*/
		vertices.push_back(Vert{ Vec3(v0.x, v0.y, vertices.size()), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vert{ Vec3(v1.x, v1.y, vertices.size()), Vec2(0.0f, 1.0f) });
		vertices.push_back(Vert{ Vec3(v2.x, v2.y, vertices.size()), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vert{ Vec3(v1.x, v1.y, vertices.size()), Vec2(0.0f, 1.0f) });
		vertices.push_back(Vert{ Vec3(v2.x, v2.y, vertices.size()), Vec2(0.0f, 0.0f) });
		vertices.push_back(Vert{ Vec3(v3.x, v3.y, vertices.size()), Vec2(0.0f, 1.0f) });

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

	//static bool a = true;
	//ImGui::Checkbox("a", &a);


	//auto addQuad = [&](const PtVertex& up0, const PtVertex& up1, const PtVertex& down0, const PtVertex& down1) {
	//	0, 1, 2, 2, 1, 3;
	//	if (!a && up0.pos != up1.pos) {
	//		return;
	//	}

	//	vertices.push_back(up0);
	//	vertices.push_back(up1);
	//	vertices.push_back(down0);
	//	vertices.push_back(up1);
	//	vertices.push_back(down0);
	//	vertices.push_back(down1);
	//	//vertices.push_back(up0);
	//	/*indices.push_back(vertices.size());
	//	vertices.push_back({ v0, Vec2(0.0f, 0.0f) });
	//	indices.push_back(vertices.size());
	//	vertices.push_back({ v1, Vec2(0.0f, 1.0f) });
	//	indices.push_back(vertices.size());
	//	vertices.push_back({ v2, Vec2(0.0f, 0.0f) });
	//	indices.push_back(vertices.size());
	//	vertices.push_back({ v1, Vec2(0.0f, 1.0f) });
	//	indices.push_back(vertices.size());
	//	vertices.push_back({ v2, Vec2(0.0f, 0.0f) });
	//	indices.push_back(vertices.size());
	//	vertices.push_back({ v3, Vec2(0.0f, 1.0f) });*/
	//};

	//static float width = 0.1f;
	//ImGui::SliderFloat("width", &width, 0.0f, 1.0f);

	//static bool b = true;
	//ImGui::Checkbox("b", &b);

	//auto roundJoin = [&](Vec2 start, Vec2 end, Vec2 around, Vec2 belowAround, int count, bool aroundIsDown, float xPos) {
	//	if (!a) return;
	//	float aroundYPos;
	//	float arcYPos;
	//	if (aroundIsDown) {
	//		aroundYPos = 0.0f;
	//		arcYPos = 1.0f;
	//	} else {
	//		aroundYPos = 1.0f;
	//		arcYPos = 0.0f;
	//	}
	//	auto startAngle = (start - around).angle();
	//	auto endAngle = (end - around).angle();
	//	auto angleRange = std::abs(endAngle - startAngle);
	//	auto angleStep = angleRange / count;
	//	if (angleRange > PI<float>) {
	//		angleRange = TAU<float> -angleRange;
	//		angleStep = -(angleRange / count);
	//	}
	//	if (startAngle > endAngle) {
	//		angleStep = -angleStep;
	//	}

	//	if (abs(endAngle - (startAngle + angleStep * count)) > 0.01f) {
	//		int x = 5;
	//	}

	//	Vec2 previous = start;
	//	Vec2 r = start - around;
	//	const auto length = (around - start).length();
	//	for (int i = 0; i < count; i++) {
	//		vertices.push_back({ around + Vec2::oriented(startAngle + i * angleStep) * length, Vec2(xPos, arcYPos) });
	//		vertices.push_back({ belowAround, Vec2(xPos, aroundYPos) });
	//		vertices.push_back({ around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length, Vec2(xPos, arcYPos) });
	//	}
	//};

	//PtVertex previousUp, previousDown;
	//{
	//	const auto first = line[0];
	//	const auto second = line[1];
	//	const auto normal = (second - first).normalized().rotBy90deg();
	//	//indices.push_back(vertices.size());
	//	previousDown = { first - normal * width, Vec2(0.0f, 0.0f) };
	//	//vertices.push_back(previousDown);
	//	//indices.push_back(vertices.size());
	//	previousUp = { first + normal * width, Vec2(0.0f, 1.0f) };
	//	//vertices.push_back(previousUp);
	//}
	//float distanceAlongLine = 0.0f;
	//for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
	//	const auto current = line[i];
	//	const auto next = line[i + 1];
	//	const auto currentToNext = (next - current);
	//	const auto normalToCurrentLine = currentToNext.rotBy90deg().normalized();

	//	const auto up0 = current + normalToCurrentLine * width;
	//	auto up1 = up0 + currentToNext;
	//	const auto down0 = current - normalToCurrentLine * width;
	//	auto down1 = down0 + currentToNext;

	//	if (i == line.size() - 2) {
	//		if (a) {
	//			distanceAlongLine += (next - current).length();
	//			addQuad(previousUp, { up1, Vec2(distanceAlongLine, 1.0f) }, previousDown, { down1, Vec2(distanceAlongLine, 0.0f) });
	//		} else {
	//			addQuad({ up0 }, { up0 }, { up1 }, { up1 });
	//			addQuad({ down0 }, { down0 }, { down1 }, { down1 });
	//		}
	//		break;
	//	}

	//	const auto nextNext = line[i + 2];
	//	const auto nextToNextNext = (nextNext - next);
	//	const auto normalToNextLine = nextToNextNext.rotBy90deg().normalized();

	//	const auto nextUp0 = next + normalToNextLine * width;
	//	const auto nextUp1 = nextUp0 + nextToNextNext;
	//	const auto nextDown0 = next - normalToNextLine * width;
	//	const auto nextDown1 = nextDown0 + nextToNextNext;

	//	const auto intersectionUp = intersectLineSegments(up0, up1, nextUp0, nextUp1);
	//	const auto intersectionDown = intersectLineSegments(down0, down1, nextDown0, nextDown1);

	//	/*Vec2 newV1;
	//	Vec2 newV3;*/

	//	/*Vec2 up;
	//	Vec2 down;*/
	//	//addQuad({ up0 }, { up1 }, { down0 }, { down1 });

	//	if (intersectionUp.has_value()) {
	//		const auto cutOutUpPart = up1 - *intersectionUp;
	//		const auto textureX = distanceAlongLine + (next - current).length() - cutOutUpPart.length();
	//		const PtVertex upVertex{ *intersectionUp, Vec2(textureX, 1.0) };
	//		const PtVertex downVertex{ down1 - cutOutUpPart, Vec2(textureX, 0.0f) };
	//		addQuad(previousUp, upVertex, previousDown, downVertex);
	//		previousUp = upVertex;
	//		const auto cutOutPartNext = nextUp0 - *intersectionUp;
	//		previousDown = { nextDown0 - cutOutPartNext, Vec2(textureX, 0.0f) };
	//		roundJoin(downVertex.pos, previousDown.pos, next, *intersectionUp, 5, false, textureX);
	//		distanceAlongLine = textureX;
	//	} else if (intersectionDown.has_value()) {
	//		const auto cutOutDownPart = down1 - *intersectionDown;
	//		const auto textureX = distanceAlongLine + (next - current).length() - cutOutDownPart.length();
	//		const PtVertex downVertex{ *intersectionDown, Vec2(textureX, 0.0f) };
	//		const PtVertex upVertex{ up1 - cutOutDownPart, Vec2(textureX, 1.0f) };
	//		addQuad(previousUp, upVertex, previousDown, downVertex);
	//		previousDown = downVertex;
	//		const auto cutOutPartNext = nextDown0 - *intersectionDown;
	//		previousUp = { nextUp0 - cutOutPartNext, Vec2(textureX, 1.0f) };
	//		roundJoin(upVertex.pos, previousUp.pos, next, *intersectionDown, 5, true, textureX);
	//		distanceAlongLine = textureX;
	//	} else {
	//		continue;
	//		const auto downIntersection = Line(down0, down1).intersection(Line(nextDown0, nextDown1));
	//		const auto upIntersection = Line(up0, up1).intersection(Line(nextUp0, nextUp1));
	//		distanceAlongLine += (next - current).length();
	//		if (!downIntersection.has_value() || !upIntersection.has_value()) {
	//			continue;
	//		}
	//		/*Debug::drawCircle(*downIntersection, 0.02f);
	//		Debug::drawCircle(*upIntersection, 0.02f);*/
	//		//continue;

	//		//addQuad(PtVertex{ *downIntersection  }, PtVertex{*downIntersection}, PtVertex{Vec2(0.0f)}, PtVertex{Vec2(0.0f)});
	//		
	//		/*addQuad(previousUp, PtVertex{ up1, Vec2(1.0f, distanceAlongLine) }, previousDown, PtVertex{ down1, Vec2(0.0f, distanceAlongLine) });*/
	//		/*previousUp = PtVertex{ nextUp0, Vec2(1.0f, distanceAlongLine) };
	//		previousDown = PtVertex{ nextDown0, Vec2(0.0f, distanceAlongLine) };*/
	//		/*previousUp = PtVertex{ up1 - normalToCurrentLine * width, Vec2(1.0f, distanceAlongLine)};
	//		previousDown = PtVertex{ nextDown0 - normalToCurrentLine * width, Vec2(0.0f, distanceAlongLine) };*/
	//		const PtVertex upVertex{ up1, Vec2(distanceAlongLine, 0.0f) };
	//		const PtVertex downVertex{ down1, Vec2(distanceAlongLine, 1.0f) };
	//		Debug::drawCircle(up1, 0.02f);
	//		Debug::drawCircle(down1, 0.02f);
	//		Debug::drawCircle(previousDown.pos, 0.02f);
	//		Debug::drawCircle(previousUp.pos, 0.02f);
	//		addQuad(previousUp, upVertex, previousDown, downVertex);
	//		
	//		if (signedDistance(Line(down1, down1 + normalToCurrentLine), *downIntersection) < 0.0f) {
	//			/*previousUp = { downVertex.pos - (normalToCurrentLine * width * 2.0f) * Rotation { -0.01f }, Vec2(1.0f, distanceAlongLine) };
	//			previousDown = downVertex;
	//			roundJoin(upVertex.pos, previousUp.pos, downVertex.pos, 5, true, distanceAlongLine);*/
	//			/*const auto cutOutDownPart = *downIntersection - down1;
	//			addQuad(
	//				previousUp, 
	//				{ up1 - cutOutDownPart, Vec2(1.0f, distanceAlongLine) }, 
	//				previousDown, 
	//				{ *downIntersection, Vec2(1.0f, distanceAlongLine) });*/
	//			//roundJoin(up1, nextUp0, next, 5, true, distanceAlongLine);
	//			Debug::drawCircle(*downIntersection, 0.02f);
	//			/*roundJoin(up1, nextUp0, next, 5, true, distanceAlongLine);*/
	//			/*roundJoin(up1, nextUp0, next, 5, false, distanceAlongLine);*/
	//			/*roundJoin(up1, nextUp0, down1, 5, false, distanceAlongLine);
	//			previousUp = { nextUp0, Vec2(1.0f, distanceAlongLine) };
	//			previousDown = downVertex;*/
	//		} else {
	//			Debug::drawCircle(*upIntersection, 0.02f);
	//			/*roundJoin(down1, nextDown0, next, 5, true, distanceAlongLine);*/
	//			/*roundJoin(down1, nextDown0, up1, 5, true, distanceAlongLine);
	//			previousDown = { nextDown0, Vec2(0.0f, distanceAlongLine) };
	//			previousUp = upVertex;*/
	//		}

	//	}
	//	if (!a) {
	//		addQuad({ up0 }, { up0 }, { up1 }, { up1 });
	//		addQuad({ down0 }, { down0 }, { down1 }, { down1 });
	//	}
	//	
	//	//if (i == 0) {
	//	//	//addQuad(up0, down0, newV1, newV3);
	//	//	addQuad(previousUp, PtVertex{ up1, Vec2()})
	//	//}
	//	/*if (a) {
	//		if (i == 0) {
	//			addQuad(up0, down0, newV1, newV3);
	//		} else {
	//			addQuad(previousV1, previousV3, newV1, newV3);
	//		}
	//	}*/
	//	/*previousV1 = down;
	//	previousV3 = up;*/
	//}
	///*for (auto& vertex : vertices) {
	//	vertex.texturePos.x /= distanceAlongLine;
	//}*/
	//for (auto& vertex : vertices) {
	//	vertex.texturePos.x /= width * 2.0;
	//}
	//return Result{ vertices, indices };

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	//	if (line.size() < 2) {
//		return Result{};
//	}
//	std::vector<i32> indices;
//	std::vector<PtVertex> vertices;
//
//	static bool a = true;
//	ImGui::Checkbox("a", &a);
//
//	static bool b = true;
//	ImGui::Checkbox("b", &b);
//
//	static bool c = true;
//	ImGui::Checkbox("c", &c);
//	auto halfWidthAt = [](float t) {
//		t -= 0.5f;
//		return (t * t) / 2.0f;
//	};
//
//	float lineLength = 0.0f;
//	for (int i = 0; i < line.size() - 1; i++) {
//		lineLength += distance(line[i], line[i + 1]);
//	}
//
//	// https://github.com/godotengine/godot/issues/35878
//	// This kind of problem has been around for years... one way to never get overlap is to pre-draw the geometry opaque in a viewport and draw the result with transparency, but in Godot 3.2 it's quite overkill.
//
//	float halfWidth = 0.1;
//	/*PtVertex nextSegmentUp0;
//	PtVertex nextSegmentDown0;*/
//	PtVertex previousUp;
//	PtVertex previousDown;
//	{
//		const auto first = line[0];
//		const auto second = line[1];
//		const auto halfWidth = halfWidthAt(0.0f);
//		const auto normal = (second - first).normalized().rotBy90deg();
//		indices.push_back(vertices.size());
//		previousDown = { first - normal * halfWidth, Vec2(0.0f, 1.0f) };
//		vertices.push_back(previousDown);
//		indices.push_back(vertices.size());
//		previousUp = { first + normal * halfWidth, Vec2(0.0f, 1.0f) };
//		vertices.push_back(previousUp);
//	}
//	float distanceAlongLine = 0.0f;
//	for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
//		const auto current = line[i];
//		const auto next = line[i + 1];
//		const auto currentToNext = (next - current);
//		const auto normalToCurrentLine = currentToNext.rotBy90deg().normalized();
//		const PtVertex up{  };
//
//
//		if (i == line.size() - 2) {
//			//distanceAlong += currentLength;
//			//addQuad(nextSegmentUp0, PtVertex{ up1, Vec2{ distanceAlong, 1.0f } }, nextSegmentDown0, PtVertex{ down1, Vec2{ distanceAlong, 0.0f } });
//			///*addQuad(previousUp1, previousDown1, up1, down1, currentLength + distanceAlong, currentLength + distanceAlong);*/
//			break;
//		}
//	
//		const auto nextNext = line[i + 2];
//		const auto nextToNextNext = (nextNext - next);
//		const auto normalToNextLine = nextToNextNext.rotBy90deg().normalized();
//	
//		const auto nextUp0 = next + normalToNextLine * halfWidth;
//		const auto nextUp1 = nextUp0 + nextToNextNext;
//		const auto nextDown0 = next - normalToNextLine * width;
//		const auto nextDown1 = nextDown0 + nextToNextNext;
//
//		//currentLength = currentToNext.length();
//		//const auto up0 = current + normalToCurrentLine * width;
//		//auto up1 = up0 + currentToNext;
//		//const auto down0 = current - normalToCurrentLine * width;
//		//auto down1 = down0 + currentToNext;
//	}
////	for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
////		
////		/*
////		 u0--u2-nextU0-nextU1
////		 |    |      |      |
////		 d0--d1-nextD0-nextD1
////		*/
////		const auto current = line[i];
////		const auto next = line[i + 1];
////		const auto currentToNext = (next - current);
////		const auto normalToCurrentLine = currentToNext.rotBy90deg().normalized();
////		//currentLength = currentToNext.length();
////		//const auto up0 = current + normalToCurrentLine * width;
////		//auto up1 = up0 + currentToNext;
////		//const auto down0 = current - normalToCurrentLine * width;
////		//auto down1 = down0 + currentToNext;
////		
////
////		if (i == line.size() - 2) {
////			//distanceAlong += currentLength;
////			//addQuad(nextSegmentUp0, PtVertex{ up1, Vec2{ distanceAlong, 1.0f } }, nextSegmentDown0, PtVertex{ down1, Vec2{ distanceAlong, 0.0f } });
////			///*addQuad(previousUp1, previousDown1, up1, down1, currentLength + distanceAlong, currentLength + distanceAlong);*/
////			//break;
////		}
////
////		const auto nextNext = line[i + 2];
////		const auto nextToNextNext = (nextNext - next);
////		const auto normalToNextLine = nextToNextNext.rotBy90deg().normalized();
////
////		const auto nextUp0 = next + normalToNextLine * width;
////		const auto nextUp1 = nextUp0 + nextToNextNext;
////		const auto nextDown0 = next - normalToNextLine * width;
////		const auto nextDown1 = nextDown0 + nextToNextNext;
////
////		auto roundJoin = [&vertices](PtVertex start, Vec2 end, Vec2 roundingCenter, PtVertex segmentsIntersection, int count) -> float {
////			//float sub = 0.5;
////			/*if (aroundTextureY == 1.0f) {
////				sub = -sub;
////			}*/
////			/*vertices.push_back({ start, Vec2(distanceAlong, startTextureY) });
////			vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
////			vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });
////			vertices.push_back({ end, Vec2(0.0f, startTextureY) });
////			vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
////			vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });*/
////			/*vertices.push_back({ end, Vec2(0.0f, startTextureY) });
////			vertices.push_back({ around, Vec2(0.0f, aroundTextureY) });
////			vertices.push_back({ p, Vec2(0.0f, startTextureY) });*/
////			const auto roundingCircleRadiusVector = start.pos - roundingCenter;
////			auto startAngle = (roundingCircleRadiusVector).angle();
////			auto endAngle = (end - roundingCenter).angle();
////			auto angleRange = std::abs(endAngle - startAngle);
////			float angleStep;
////			if (angleRange > PI<float>) {
////				angleRange = TAU<float> -angleRange;
////				angleStep = -(angleRange / count);
////			} else {
////				angleStep = (angleRange / count);
////			}
////
////			if (startAngle > endAngle) {
////				angleStep = -angleStep;
////			}
////
////			Rotation rotationStep(angleRange / count);
////			//auto along = distanceAlong + currentLength;
////			const auto roundingRadius = roundingCircleRadiusVector.length();
////			const auto step = std::abs(angleStep) * roundingRadius;
////			for (int i = 0; i < count; i++) {
////				/*vertices.push_back({ around + Vec2::oriented(startAngle + i * angleStep) * length, Vec2(along, startTextureY) });
////				vertices.push_back({ around, Vec2(along, aroundTextureY + sub) });
////				vertices.push_back({ around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length, Vec2(along + step , startTextureY) });
////				along += step;*/
////			}
////			return step * count;
////		};
////
////		//auto roundJoin = [&](Vec2 start, Vec2 end, Vec2 around, int count, Vec2 p, float startTextureY, float aroundTextureY) -> float {\
////		//	float sub = 0.5;
////		//	if (aroundTextureY == 1.0f) {
////		//		sub = -sub;
////		//	}
////		//	vertices.push_back({ start, Vec2(distanceAlong, startTextureY) });
////		//	vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
////		//	vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });
////		//	vertices.push_back({ end, Vec2(0.0f, startTextureY) });
////		//	vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
////		//	vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });
////		//	/*vertices.push_back({ end, Vec2(0.0f, startTextureY) });
////		//	vertices.push_back({ around, Vec2(0.0f, aroundTextureY) });
////		//	vertices.push_back({ p, Vec2(0.0f, startTextureY) });*/
////		//	auto startAngle = (start - around).angle();
////		//	auto endAngle = (end - around).angle();
////		//	auto angleRange = std::abs(endAngle - startAngle);
////		//	auto angleStep = angleRange / count;
////		//	if (angleRange > PI<float>) {
////		//		//Debug::drawCircle(around, 0.02f);
////		//		angleRange = TAU<float> - angleRange;
////		//		//std::swap(startAngle, endAngle);
////		//		angleStep = -(angleRange / count);
////		//	}
////		//	if (startAngle > endAngle) {
////		//		angleStep = -angleStep;
////		//	}
////
////		//	if (abs(endAngle - (startAngle + angleStep * count)) > 0.01f) {
////	//			int x = 5;
////		//	}
////
////		//	Rotation rotationStep(angleRange / count);
////		//	Vec2 previous = start;
////		//	Vec2 r = start - around;
////		//	auto along = distanceAlong + currentLength;
////		//	const auto length = (around - start).length();
////		//	const auto step = std::abs(angleStep) * length;
////		//	for (int i = 0; i < count; i++) {
////		//		vertices.push_back({ around + Vec2::oriented(startAngle + i * angleStep) * length, Vec2(along, startTextureY) });
////		//		vertices.push_back({ around, Vec2(along, aroundTextureY + sub) });
////		//		vertices.push_back({ around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length, Vec2(along + step , startTextureY) });
////		//		along += step;
////		//	}
////		//	jointLength = step * count;
////		//};
////
////		/*Vec2 newUp1;
////		Vec2 newDown1;
////		Vec2 afterDown0;
////		Vec2 afterUp0;
////		float upTexturePosX;
////		float downTexturePosX;
////		jointLength = 0.0f;
////		upTexturePosX = currentLength + distanceAlong;
////		downTexturePosX = currentLength + distanceAlong;*/
////
////		distanceAlong += currentLength;
////		const auto intersectionUp = intersectLineSegments(up0, up1, nextUp0, nextUp1);
////		const auto intersectionDown = intersectLineSegments(down0, down1, nextDown0, nextDown1);
////		if (intersectionUp.has_value()) {
////			// Because rounding is used to join 2 lines there is no way to assign texture coordinates to points on the line without streching them. The rounding arc is longer than the bottom part and adds to the x coordinate which means the to part travels more than the bottom part so to maintain lengths and not create discontinouties the texture cooridnates at the point opposite to the rounding would need to be constant and the points on the arc would need to add to x distance. This would make it so the distances on one side of the line would be bigger than on the bottom part, which would not be good for texturing (the top part would be skewed relative to the bottom). Unity handles this by making the x texture coordinate constant along the whole arc. This causes articafts, because the whole arc uses the same texture for example. I think the impossiblity of creating a coordinate system on a line without stretching might be a consequence of Theorema Egregium. Straws need to be extendible to be able to bend nicely.
////
////			// To calculate a width at a point you need the position along the line, this could be calculated for example, by adding up all the lengths of segments and dividing the current position by this length to get a value from 0 to 1. The issue is that to keep the textures the length acutally added won't be the actual length from the current point to the next so to keep the lenghts consistent a smaller value would need to be added to the x coordinate, but if a smaller value is added then the total length, changes, which means the the calculated position along the curve from 0 to 1 won't be correct. Another option would be to add the whole length even if it isn't the correct value the issue is that then the value of the width won't be the value it should be for the value of x. The issue is what should the value be at the start of the next line after the arc. Normally it should be the same. Can't just make it wider, because it would change the intersection point and also the correct width. Could interpolate the width along the arc. Not sure how other implementations handle this. In unity it doesn't seem that the length is interpolated along the arc. The only actual solution would probably be to intersect 2 curves instead of lines at these points so that the width changes correctly. It seems that the simplest way to handle this would be to change the width in the fragment shader. A fragment shader would probably need to be used anyway to do anti aliasing. And with differently changing values in the y direction the anti aliasing width would vary along the curve, which might not look correct. The only option to handle this with actually changing triangle width would probably require adding extra quads on both sides like here .
////			const auto shorten = (up0 - *intersectionUp) - (up0 - up1);
////			const auto upVertex = PtVertex{ *intersectionUp, Vec2(distanceAlong - shorten.length(), 1.0f) };
////			const auto downVertex = PtVertex{ down1, Vec2(distanceAlong, 0.0f) };
////			addQuad(nextSegmentUp0, upVertex, nextSegmentDown0, downVertex);
////			const auto jointLength = roundJoin(downVertex, nextDown0, next, upVertex, 5);
////			distanceAlong += jointLength;
////
////			/*nextSegmentUp0 = upVertex;
////			nextSegmentDown0 = PtVertex{ nextDown0, Vec2(distanceAlong, 0.0f) };*/
////			//roundJoin(down1, nextDown0, next, 5, *intersectionUp, 1.0f, 0.0f);
////// 
////			//nextSegmentUp0
////			/*newUp1 = *intersectionUp;
////
////			afterUp0 = newUp1;
////			afterDown0 = nextDown0;
////			newDown1 = down1;*/
////
////			/*if (c) {
////				const auto shorten = std::abs((up0 - newUp1).length() - (up0 - up1).length());
////				upTexturePosX = currentLength + distanceAlong - shorten;
////				downTexturePosX = currentLength + distanceAlong;
////				previousUpTexturePosX = upTexturePosX;
////				previousDownTexturePosY = currentLength + distanceAlong + jointLength;
////			}*/
////
////		} /*else if (intersectionDown.has_value()) {
////			newDown1 = *intersectionDown;
////			newUp1 = up1;
////			roundJoin(up1, nextUp0, next, 5, newDown1, 0.0f, 1.0f);
////			afterDown0 = newDown1;
////			afterUp0 = nextUp0;
////
////			if (c) {
////				const auto shorten = std::abs((down0 - newDown1).length() - (down0 - down1).length());
////				upTexturePosX = currentLength + distanceAlong;
////				downTexturePosX = currentLength + distanceAlong - shorten;
////				previousDownTexturePosY = downTexturePosX;
////				previousUpTexturePosX = currentLength + distanceAlong + jointLength;
////			}
////
////		} else {
////			upTexturePosX = currentLength + distanceAlong;
////			downTexturePosX = currentLength + distanceAlong;
////			newUp1 = up1;
////			newDown1 = down1;
////			afterUp0 = nextUp0;
////			afterDown0 = nextDown0;
////			const auto v3io = Line(down0, down1).intersection(Line(nextDown0, nextDown1));
////			const auto v1io = Line(up0, up1).intersection(Line(nextUp0, nextUp1));
////			if (!v3io.has_value() || !v1io.has_value()) {
////				ImGui::Text("error");
////				continue;
////			}
////
////			if (signedDistance(Line(down1, down1 + normalToCurrentLine), *v3io) < 0.0f) {
////				roundJoin(up1, nextUp0, next, 5, next, 0.0f, 0.0f);
////
////			} else {
////				roundJoin(down1, nextDown0, next, 5, next, 0.0f, 0.0f);
////			}
////
////		}*/
////
////
////		/*if (a) {
////			if (i == 0) {
////				addQuad(up0, down0, newUp1, newDown1, upTexturePosX, downTexturePosX);
////			} else {
////				addQuad(previousUp1, previousDown1, newUp1, newDown1, upTexturePosX, downTexturePosX);
////			}
////		}
////		distanceAlong += currentLength;
////		distanceAlong += jointLength;
////
////		previousUp1 = afterUp0;
////		previousDown1 = afterDown0;*/
////
////	}
////
////	return vertices;
}