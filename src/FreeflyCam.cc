#include "FreeflyCam.hh"

#include <typed-geometry/tg.hh>



void gamedev::FreeflyCamState::moveRelative(tg::vec3 distance, bool freeFlyEnabled)
{
    auto angle = tg::angle_between(forward, tg::vec3(0, -1, 0));
    auto dist = tan(angle) * position.y;

	// Rotation center of the camera
    auto focus = position + normalize(forward) * hypot(position.y, dist);

    auto const rotation = forwardToRotation(tg::vec3(forward.x, (freeFlyEnabled ? forward.y : 0.f), forward.z));
	auto const delta = tg::transpose(tg::mat3(rotation)) * distance;

	auto newFocus = focus + delta;

	int mapBorder = 80; // Distance from point 0 in x and z direction

	if (freeFlyEnabled || (newFocus.x <= mapBorder && newFocus.x >= -mapBorder && newFocus.z <= mapBorder && newFocus.z >= -mapBorder))
	{
		position += delta;
    }
}

void gamedev::FreeflyCamState::mouselook(float dx, float dy)
{
	auto altitude = tg::atan2(forward.y, length(tg::vec2(forward.x, forward.z)));
	auto azimuth = tg::atan2(forward.z, forward.x);

	azimuth += 1_rad * dx;
	altitude = tg::clamp(altitude - 1_rad * dy, -89_deg, 89_deg);

	auto caz = tg::cos(azimuth);
	auto saz = tg::sin(azimuth);
	auto cal = tg::cos(altitude);
	auto sal = tg::sin(altitude);

	forward = tg::vec3(cal * caz, sal, cal * saz);
}

void gamedev::FreeflyCamState::setFocus(tg::pos3 focus, tg::vec3 global_offset)
{
	position = focus + global_offset;
	forward = tg::normalize(focus - position);
}

tg::quat gamedev::forwardToRotation(tg::vec3 forward, tg::vec3 up)
{
	auto const fwd = -tg::normalize(forward);
	tg::vec3 rightVector = tg::normalize(tg::cross(fwd, up));
	tg::vec3 upVector = tg::cross(rightVector, fwd);

	tg::mat3 rotMatrix;
	rotMatrix[0][0] = rightVector.x;
	rotMatrix[0][1] = upVector.x;
	rotMatrix[0][2] = -fwd.x;
	rotMatrix[1][0] = rightVector.y;
	rotMatrix[1][1] = upVector.y;
	rotMatrix[1][2] = -fwd.y;
	rotMatrix[2][0] = rightVector.z;
	rotMatrix[2][1] = upVector.z;
	rotMatrix[2][2] = -fwd.z;
	return tg::quat::from_rotation_matrix(rotMatrix);
}

bool gamedev::FreeflyCam::interpolateToTarget(float dt)
{
	auto const alpha_rotation = tg::min(1.f, exponentialDecayAlpha(sensitivityRotation, dt));
	auto const alpha_position = tg::min(1.f, exponentialDecayAlpha(sensitivityPosition, dt));

	auto const forward_diff = target.forward - physical.forward;
	auto const pos_diff = target.position - physical.position;

	if (tg::length_sqr(forward_diff) + tg::length_sqr(pos_diff) < tg::epsilon<float>)
	{
		// no changes below threshold
		return false;
	}
	else
	{
		physical.forward = tg::normalize(physical.forward + alpha_rotation * forward_diff);
		physical.position = physical.position + alpha_position * pos_diff;
		return true;
	}
}
