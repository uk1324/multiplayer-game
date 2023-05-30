#include <math/marchingSquares.hpp>
#include <math/utils.hpp>


auto marchingSquares(const ImageRgba& texture, bool pixelPerfect, bool conntectDiagonals) -> std::vector<std::vector<Vec2>> {
	auto isOutOfRange = [&](i64 x, i64 y) {
		return x < 0 || y < 0 || x >= texture.size().x || y >= texture.size().y;
	};

	auto at = [&](i64 x, i64 y) -> bool {
		// TODO: Could add an option for chosing what to do in the case of out of range.
		if (isOutOfRange(x, y)) {
			return false;
		}
		return (texture.get(Vec2T{ x, texture.size().y - 1 - y }).r != 0);
	};

	auto value = [&](i64 x, i64 y) -> i32 {
		i32 value = at(x, y) << 0;
		value += at(x + 1, y) << 1;
		value += at(x, y + 1) << 2;
		value += at(x + 1, y + 1) << 3;
		return value;
	};

	std::vector<bool> visited;
	visited.resize(texture.size().x * texture.size().y, false);

	auto setVisited = [&](i64 x, i64 y) -> void {
		if (isOutOfRange(x, y)) {
			return;
		}
		visited[y * texture.size().x + x] = true;
	};

	auto getVisited = [&](i64 x, i64 y) -> bool {
		if (isOutOfRange(x, y)) {
			return true;
		}
		return visited[y * texture.size().x + x];
	};

	std::vector<std::vector<Vec2>> polygons;
	Vec2T<i64> startPos{ 0, 0 };

	for (;;) {
		for (i64 y = startPos.y; y < texture.size().y; y++) {
			for (i64 x = startPos.x; x < texture.size().x; x++) {
				if (getVisited(x, y)) {
					continue;
				}

				setVisited(x, y);

				const auto v = value(x, y);
				const auto atLeastOneIsWhiteButNotAll = v > 0 && !(v == (1 | 2 | 4 | 8));
				if (const auto liesOnTheBoundary = atLeastOneIsWhiteButNotAll) {
					startPos = Vec2T{ x, y };
					goto foundStaringPoint;
				}
			}
			if (y == texture.size().y - 1) {
				return polygons;
			}
			startPos.x = 0;
		}

		foundStaringPoint:
		Vec2T<i64> current = startPos;

		Vec2T<i64> previousMove{ 0, 0 };
		Vec2 previousTranslation{ 0.0f };
		std::vector<Vec2> verts;
		for (;;) {
			// Don't know what is a good way to explain why these directions are chosen. Some cases for example the diagonals or 3 true cell ones might be weird so it's best to just drawing them by hand. It is simpler to understand the pixel perfect version.
			Vec2 pos{ current };
			Vec2T<i64> move;
			/*
			4 8
			1 2
			*/
			const auto v = value(current.x, current.y);
			std::optional<Vec2> vert;
			switch (v) {
				/*
				0 0 | 4 0 | 4 8
				1 0 | 1 0 | 1 0
				*/
			case 1:
			case 1 | 4:
			case 1 | 4 | 8:
				move = { 0, -1 };
				break;

				/*
				0 0 | 0 0 | 4 0
				0 2 | 1 2 | 1 2
				*/
			case 2:
			case 1 | 2:
			case 1 | 2 | 4:
				move = { 1, 0 };
				break;

				/*
				4 0 | 4 8 | 4 8
				0 0 | 0 0 | 0 2
				*/
			case 4:
			case 4 | 8:
			case 2 | 4 | 8:
				move = { -1, 0 };
				break;

				/*
				0 8 | 0 8 | 0 8
				0 0 | 0 2 | 1 2
				*/
			case 8:
			case 2 | 8:
			case 1 | 2 | 8:
				move = { 0, 1 };
				break;

				/*
				0 8
				1 0
				*/
			case 1 | 8:
				if (conntectDiagonals) {
					if (previousMove == Vec2T<i64>{ -1, 0 }) {
						move = { 0, -1 };
					} else {
						move = { 0, 1 };
					}
				} else {
					if (previousMove == Vec2T<i64>{ -1, 0 }) {
						move = { 0, 1 };
					} else {
						move = { 0, -1 };
					}
				}
				
				break;

				/*
				4 0
				0 2
				*/
			case 2 | 4:
				if (conntectDiagonals) {
					if (previousMove == Vec2T<i64>{ 0, -1 }) {
						move = { 1, 0 };
					} else {
						move = { -1, 0 };
					}
				} else {
					if (previousMove == Vec2T<i64>{ 0, -1 }) {
						move = { -1, 0 };
					} else {
						move = { 1, 0 };
					}
				}
				break;

			default:
				ASSERT_NOT_REACHED();
				return polygons;
			}

			if (pixelPerfect) {
				verts.push_back(Vec2{ current } + Vec2{ 1.0f });
				if (previousMove == move) {
					verts.pop_back();
				}
			} else {
				auto nextPos = Vec2{ current } + Vec2{ 1.0f } + Vec2{ move } / 2.0f;
				if (verts.size() >= 1) {
					const auto translation = nextPos - verts.back();
					if (translation == previousTranslation) {
						verts.pop_back();
					}
					previousTranslation = translation;
				}
				verts.push_back(nextPos);
			}

			current += move;
			setVisited(current.x, current.y);

			if (current == startPos)
				break;

			previousMove = move;
		}

		// TODO: This logic can definitely be simplified by changing the order in which things are added in the loop. One of these ifs could be removed then. 
		if (verts.size() >= 3) {
			if (const auto isColinear = (verts.back() - verts[verts.size() - 2]).applied(signOrZero) == (verts[0] - verts.back()).applied(signOrZero)) {
				verts.erase(verts.end() - 1);
			}
		}
		if (verts.size() >= 3) {
			if (const auto isColinear = (verts[0] - verts.back()).applied(signOrZero) == (verts[1] - verts[0]).applied(signOrZero)) {
				verts.erase(verts.begin());
			}
			polygons.push_back(std::move(verts));
		}
	}

	return polygons;
}