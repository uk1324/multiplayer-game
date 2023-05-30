#include <math/utils.hpp>

auto lerpPosAndZoom(const PosAndZoom& current, const PosAndZoom& target, float t) -> PosAndZoom {
	const auto zoomRatio = current.zoom / target.zoom;

	PosAndZoom result;
	if (zoomRatio == 1.0f) {
		result.zoom = target.zoom;
		result.pos = lerp(current.pos, target.pos, t);
	} else {
		// https://gamedev.stackexchange.com/questions/188841/how-to-smoothly-interpolate-2d-camera-with-pan-and-zoom/188859#188859
		// Interpolate zoom using logarithms because for example if zooms were powers of 2 then to linearly interpolate between 0.5 and 2 it should reach 1.0 at t = 0.5. The interpolated value should be how many times should the start value be multipled by 2 which is what a logarithm is. To make this work for non integer arguments the exponential function is used because it's rate of changes is equal to itself for all arguments.
		result.zoom = exp(lerp(log(current.zoom), log(target.zoom), t));
		// To make it look like the position is moving with the same speed as the zooming at each point in time the rate of change of position should be proportional to the camera zoom. This proportion can be calculated by integrating the zoom speed which is zoomRatio^x. This integral = (zoomRatio^x - 1) / ln(zoomRatio). The constant ln(zoomRatio) can be ignored because only the proportions are needed for normalizing the values. To normalize the values to the range <0, 1> the value needs to be divided. For 0 the integral is equal to 0 and for 1 it is equals (zoomRatio - 1).
		const auto posInterpolationT = (pow(zoomRatio, t) - 1) / (zoomRatio - 1);
		result.pos = lerp(current.pos, target.pos, posInterpolationT);
	}
	return result;
}
