#include <client/Rendering/LineTriangulate.hpp>
#include <imgui/imgui.h>
#include <engine/Math/LineSegment.hpp>
#include <engine/Math/Utils.hpp>
#include <engine/Math/Transform.hpp>

float triangulateLine(std::span<const Vec2> line, float width, std::vector<PtVertex>& output) {
	output.clear();

	if (line.size() < 2) {
		return 0.0f;
	}

	auto addQuad = [&](const PtVertex& up0, const PtVertex& up1, const PtVertex& down0, const PtVertex& down1) {
		output.push_back(up0);
		output.push_back(up1);
		output.push_back(down0);
		output.push_back(up1);
		output.push_back(down0);
		output.push_back(down1);
	};

	auto createArc = [&](Vec2 start, Vec2 end, Vec2 around, Vec2 joinedWith, int count, float arcTextureY, float aroundTextureY, float textureX) {
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

		Vec2 previous = start;
		Vec2 r = start - around;
		const auto length = (around - start).length();
		for (int i = 0; i < count; i++) {
			const auto v0 = around + Vec2::oriented(startAngle + i * angleStep) * length;
			const auto v1 = around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length;
			output.push_back({ v0, Vec2(textureX, arcTextureY) });
			output.push_back({ joinedWith , Vec2(textureX, aroundTextureY) });
			output.push_back({ v1, Vec2(textureX, arcTextureY) });
		}

	};

	auto roundJoin = [&](
		Vec2 start, 
		Vec2 end, 
		Vec2 around, 
		Vec2 joinedWith, 
		int count, 
		bool aroundIsDown, 
		float textureX, 
		const PtVertex&up,
		const PtVertex& down,
		const PtVertex& nextUp,
		const PtVertex& nextDown) {

		float aroundTextureY;
		float arcTextureY;
		if (aroundIsDown) {
			aroundTextureY = 0.0f;
			arcTextureY = 1.0f;
		} else {
			aroundTextureY = 1.0f;
			arcTextureY = 0.0f;
		}
		output.push_back(up);
		output.push_back(down);
		output.push_back({ start, Vec2(textureX, arcTextureY) });
		createArc(start, end, around, joinedWith, count, arcTextureY, aroundTextureY, textureX);
		output.push_back(nextUp);
		output.push_back(nextDown);
		output.push_back({ end, Vec2(textureX, arcTextureY) });
	};

	PtVertex previousUp, previousDown;
	{
		const auto first = line[0];
		const auto second = line[1];
		const auto normal = (second - first).normalized().rotBy90deg();
		previousDown = { first - normal * width, Vec2(0.0f, 0.0f) };
		previousUp = { first + normal * width, Vec2(0.0f, 1.0f) };
	}
	float distanceAlongLine = 0.0f;
	for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
		const auto current = line[i];
		const auto next = line[i + 1];
		const auto currentToNext = (next - current);
		const auto currentToNextLength = currentToNext.length();
		const auto normalToCurrentLine = currentToNext.rotBy90deg().normalized();

		const auto up0 = current + normalToCurrentLine * width;
		auto up1 = up0 + currentToNext;
		const auto down0 = current - normalToCurrentLine * width;
		auto down1 = down0 + currentToNext;

		if (i == line.size() - 2) {
			// Could be down instead of up instead.
			distanceAlongLine += distance(previousUp.pos, up1);
			addQuad(previousUp, { up1, Vec2(distanceAlongLine, 1.0f) }, previousDown, { down1, Vec2(distanceAlongLine, 0.0f) });
			break;
		}

		const auto nextNext = line[i + 2];
		const auto nextToNextNext = (nextNext - next);
		const auto normalToNextLine = nextToNextNext.rotBy90deg().normalized();

		const auto nextUp0 = next + normalToNextLine * width;
		const auto nextUp1 = nextUp0 + nextToNextNext;
		const auto nextDown0 = next - normalToNextLine * width;
		const auto nextDown1 = nextDown0 + nextToNextNext;

		const auto intersectionUp = intersectLineSegments(up0, up1, nextUp0, nextUp1);
		const auto intersectionDown = intersectLineSegments(down0, down1, nextDown0, nextDown1);

		if (intersectionUp.has_value()) {
			const auto cutOutUpPart = up1 - *intersectionUp;
			distanceAlongLine += (*intersectionUp - previousUp.pos).length();
			const PtVertex upVertex{ *intersectionUp, Vec2(distanceAlongLine, 1.0) };
			const PtVertex downVertex{ down1 - cutOutUpPart, Vec2(distanceAlongLine, 0.0f) };
			addQuad(previousUp, upVertex, previousDown, downVertex);
			previousUp = upVertex;
			const auto cutOutPartNext = nextUp0 - *intersectionUp;
			previousDown = { nextDown0 - cutOutPartNext, Vec2(distanceAlongLine, 0.0f) };
			roundJoin(down1, nextDown0, next, *intersectionUp, 5, false, distanceAlongLine, upVertex, downVertex, previousUp, previousDown);
		} else if (intersectionDown.has_value()) {
			const auto cutOutDownPart = down1 - *intersectionDown;
			distanceAlongLine += (*intersectionDown - previousDown.pos).length();
			const PtVertex downVertex{ *intersectionDown, Vec2(distanceAlongLine, 0.0f) };
			const PtVertex upVertex{ up1 - cutOutDownPart, Vec2(distanceAlongLine, 1.0f) };
			addQuad(previousUp, upVertex, previousDown, downVertex);
			previousDown = downVertex;
			const auto cutOutPartNext = nextDown0 - *intersectionDown;
			previousUp = { nextUp0 - cutOutPartNext, Vec2(distanceAlongLine, 1.0f) };
			roundJoin(up1, nextUp0, next, *intersectionDown, 5, true, distanceAlongLine, upVertex, downVertex, previousUp, previousDown);
		} else {
			// Find extended intersections
			const auto downIntersection = Line(down0, down1).intersection(Line(nextDown0, nextDown1));
			const auto upIntersection = Line(up0, up1).intersection(Line(nextUp0, nextUp1));
			distanceAlongLine += currentToNextLength;
			if (const auto parallel = !downIntersection.has_value() || !upIntersection.has_value()) {
				continue;
			}
			
			const PtVertex upVertex{ up1, Vec2(distanceAlongLine, 1.0f) };
			const PtVertex downVertex{ down1, Vec2(distanceAlongLine, 0.0f) };
			addQuad(previousUp, upVertex, previousDown, downVertex);
			previousUp = { nextUp0, Vec2(distanceAlongLine, 1.0f) };
			previousDown = { nextDown0, Vec2(distanceAlongLine, 0.0f) };

			// Check if the intersection is in front or behind.
			if (signedDistance(Line(down1, down1 + normalToCurrentLine), *downIntersection) < 0.0f) {
				createArc(upVertex.pos, nextUp0, next, next, 5, 0.0f, 0.5f, distanceAlongLine);
			} else {
				createArc(downVertex.pos, nextDown0, next, next, 5, 1.0f, 0.5f, distanceAlongLine);
			}

		}
	}

	return distanceAlongLine;
}
