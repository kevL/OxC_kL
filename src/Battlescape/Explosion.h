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

#ifndef OPENXCOM_EXPLOSION_H
#define OPENXCOM_EXPLOSION_H

#include "Position.h"


namespace OpenXcom
{

/**
 * This class represents an explode animation.
 * @note Map is the owner of an instance of this class during its short life.
 * It animates a bullet or psi hit or a big explosion animation.
 */
class Explosion
{

private:
	bool _big;
	int
		_frameCurrent,
		_frameDelay,
		_frameStart,
		_hit;

	Position _pos;


	public:
		static const int
			FRAMES_BULLET,
			FRAMES_EXPLODE,
			FRAMES_HIT,
			FRAMES_TORCH;

		/// Creates a new Explosion.
		Explosion(
				Position pos,
				int frameStart,
				int frameDelay = 0,
				bool big = false,
				int hit = 0);
		/// Cleans up the Explosion.
		~Explosion();

		/// Moves the Explosion on one frame.
		bool animate();

		/// Gets the current position in voxel space.
		Position getPosition() const;

		/// Gets the current frame.
		int getCurrentFrame() const;

		/// Checks if this is a real explosion.
		bool isBig() const;
		/// Checks if this is a melee or psi hit.
		int isHit() const;
};

}

#endif
