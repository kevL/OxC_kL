/*
 * Copyright 2010-2014 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Explosion.h"


namespace OpenXcom
{

const int Explosion::FRAMES_BULLET	= 10;
const int Explosion::FRAMES_EXPLODE	= 8;
const int Explosion::FRAMES_HIT		= 4;


/**
 * Sets up a Explosion sprite with the specified size and position.
 * @param position		- explosion center position in voxel x/y/z
 * @param frameStart	- can be used to offset different explosions at different frames
 * @param frameDelay	- can be used to delay the start of explosion (default 0)
 * @param big			- flag to indicate it is a real explosion (true), or a bullet hit (default false)
 * @param hit			- true for melee and psi attacks (default false)
 */
Explosion::Explosion(
		Position position,
		int frameStart,
		int frameDelay,
		bool big,
		bool hit)
	:
		_position(position),
		_frameStart(frameStart),
		_frameDelay(frameDelay),
		_frameCurrent(frameStart),
		_big(big),
		_hit(hit)
{
}

/**
 * Deletes the Explosion.
 */
Explosion::~Explosion()
{
}

/**
 * Animates the explosion further.
 * @return, true if the animation is queued or playing, false if finished
 */
bool Explosion::animate()
{
	if (_frameDelay > 0)
	{
		_frameDelay--;

		return true;
	}

	_frameCurrent++;

	if ((_hit
			&& _frameCurrent == _frameStart + FRAMES_HIT)		// melee or psiamp
		|| (_big
			&& _frameCurrent == _frameStart + FRAMES_EXPLODE)	// explosion
		|| (_big == false
			&& _hit == false
			&& _frameCurrent == _frameStart + FRAMES_BULLET))	// bullet
	{
		return false;
	}

	return true;
}

/**
 * Gets the current position in voxel space.
 * @return, position in voxel space
 */
Position Explosion::getPosition() const
{
	return _position;
}

/**
 * Gets the current frame in the animation.
 * @return, currently playing frame number of the animation
 */
int Explosion::getCurrentFrame() const
{
	if (_frameDelay > 0)
		return -1;

	return _frameCurrent;
}

/**
 * Returns flag to indicate if it is a bullet hit (false), or a real explosion (true).
 * @return, true if it is a real explosion; false if it is a bullet, psi, or melee hit.
 */
bool Explosion::isBig() const
{
	return _big;
}

/**
 * Returns flag to indicate if it is a melee or psi hit.
 * @return, true if it is a melee hit or psi hit
 */
bool Explosion::isHit() const
{
	return _hit;
}

}
