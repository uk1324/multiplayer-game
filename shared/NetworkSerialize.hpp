#pragma once

#define NETWORK_SERIALIZE_VEC_2(stream, value) \
	serialize_float(stream, value.x); \
	serialize_float(stream, value.y);