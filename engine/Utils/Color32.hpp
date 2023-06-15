#pragma once 

#include <stdint.h>

class Color32
{
public:
	Color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

	Color32 operator+(Color32 rhs) const;
	Color32 operator*(float rhs) const;

public:
	uint8_t r, g, b, a;
};