#include <client/GameClientAdapter.hpp>

yojimbo::MessageFactory* GameClientAdapter::CreateMessageFactory(yojimbo::Allocator& allocator) {
    return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
}
