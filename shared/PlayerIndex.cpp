#include <shared/PlayerIndex.hpp>

std::ostream& operator<<(std::ostream& os, const PlayerIndex& playerIndex) {
	os << playerIndex.value;
	return os;
}
