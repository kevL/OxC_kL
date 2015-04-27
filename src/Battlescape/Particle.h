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

#ifndef OPENXCOM_PARTICLE_H
#define OPENXCOM_PARTICLE_H

//#include <algorithm>
//#include <SDL_types.h>


namespace OpenXcom
{

class Particle
{

private:
	Uint8
		_color,
		_opacity,
		_size;
	float
		_xOffset,
		_yOffset,
		_density;


	public:
		/// Creates a particle.
		Particle(
				float xOffset,
				float yOffset,
				float density,
				Uint8 color,
				Uint8 opacity);
		/// Destroys a particle.
		~Particle();

		/// Animates a particle.
		bool animate();

		/// Gets the size value.
		int getSize()
		{ return _size; }

		/// Gets the color.
		Uint8 getColor()
		{ return _color; }

		/// Gets the opacity.
		Uint8 getOpacity()
		{ return std::min(
					(_opacity + 7) / 10,
					3); }

		/// Gets the horizontal shift.
		float getX()
		{ return _xOffset; }
		/// Gets the vertical shift.
		float getY()
		{ return _yOffset; }
};

}

#endif
