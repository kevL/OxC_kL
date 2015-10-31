/*
 * Copyright 2010-2015 OpenXcom Developers.
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

const int
	Explosion::FRAMES_BULLET	= 10,
	Explosion::FRAMES_EXPLODE	= 8,
	Explosion::FRAMES_HIT		= 4,
	Explosion::FRAMES_TORCH		= 6;


/**
 * Sets up a Explosion sprite with the specified size and position.
 * @param pos			- explosion center position in voxel x/y/z
 * @param frameStart	- used to offset different explosions to different frames on the spritesheet
 * @param startDelay	- used to delay the start of explosion (default 0)
 * @param big			- flag to indicate it is a real explosion (true) or a bullet hit (default false)
 * @param hit			- used for melee and psi attacks (default 0)
 *						 1 - is a melee attack that SUCCEEDED or Psi-attack
 *						 0 - is not a hit-type attack
 *						-1 - is a melee attack that MISSED
 */
Explosion::Explosion(
		const Position pos,
		int frameStart,
		int startDelay,
		bool big,
		int hit)
	:
		_pos(pos),
		_frameStart(frameStart),
		_frameCurrent(frameStart),
		_startDelay(startDelay),
		_big(big),
		_hit(hit)
{}

/**
 * Deletes the Explosion.
 */
Explosion::~Explosion()
{}

/**
 * Animates the explosion further.
 * @return, true if the animation is queued or playing
 */
bool Explosion::animate()
{
	if (_startDelay != 0)
	{
		--_startDelay;
		return true;
	}

	++_frameCurrent;

	if (_frameStart == 88) // special handling for Fusion Torch - it has 6 frames that cycle 6 times.
	{
		static int torchCycle;

		if (torchCycle == 7)
		{
			torchCycle = 0;
			return false;
		}
		else
		{
			if (_frameCurrent == _frameStart + FRAMES_TORCH - 1)
			{
				_frameCurrent = 88;
				++torchCycle;
			}

			return true;
		}
	}

	if ((_hit != 0
			&& _frameCurrent == _frameStart + FRAMES_HIT)		// melee or psiamp
		|| (_big == true
			&& _frameCurrent == _frameStart + FRAMES_EXPLODE)	// explosion
		|| (_big == false
			&& _hit == 0
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
const Position Explosion::getPosition() const
{
	return _pos;
}

/**
 * Gets the current frame in the animation.
 * @return, currently playing frame number of the animation
 */
int Explosion::getCurrentFrame() const
{
	if (_startDelay != 0)
		return -1;

	return _frameCurrent;
}

/**
 * Returns flag to indicate if it is a bullet hit (false) or a real explosion (true).
 * @return, true if this is a real explosion; false if a bullet, psi, or melee hit
 */
bool Explosion::isBig() const
{
	return _big;
}

/**
 * Returns flag to indicate if it is a melee or psi attack.
 * @return,	 1 - psi attack or successful melee attack
 *			 0 - not a melee or psi attack
 *			-1 - unsuccessful melee attack
 */
int Explosion::isHit() const
{
	return _hit;
}

}
