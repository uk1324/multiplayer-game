#pragma once

#include <utils/imageRgba.hpp>

#include <vector>

// texture - if the pixel is outside it should be r = 0 else it can be anything.
// pixelPerfect makes the boundary aligned to the pixels. When false it creates the marching squares look. Pixel perfect also means that diagonals create overlapping pixels which can also be seen in the demo.
// connectDiagonals resolves the ambigous diagonal cases. It connects the inside parts, which also means that outisde parts are disconnected. This can be seen in the demo.
auto marchingSquares(const ImageRgba& texture, bool pixelPerfect, bool conntectDiagonals) -> std::vector<std::vector<Vec2>>;

