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

#ifndef OPENXCOM_POLYGON_H
#define OPENXCOM_POLYGON_H

//#include <SDL.h>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a polygon in the world map.
 * Polygons constitute the textured land portions
 * of the X-Com globe and typically have 3-4 points.
 */
class Polygon
{

private:
	size_t
		_points,
		_texture;
	Sint16
		* _x,
		* _y;
	double
		* _lat,
		* _lon;


	public:
		/// Creates a polygon with a number of points.
		explicit Polygon(size_t points);
		/// Creates a new polygon from an existing one.
		Polygon(const Polygon& other);
		/// Cleans up the polygon.
		~Polygon();

		/// Loads the polygon from YAML.
		void load(const YAML::Node& node);

		/// Gets the latitude of a point.
		double getLatitude(size_t i) const;
		/// Sets the latitude of a point.
		void setLatitude(
				size_t i,
				double lat);
		/// Gets the longitude of a point.
		double getLongitude(size_t i) const;
		/// Sets the longitude of a point.
		void setLongitude(
				size_t i,
				double lon);

		/// Gets the X coordinate of a point.
		Sint16 getX(size_t i) const;
		/// Sets the X coordinate of a point.
		void setX(
				size_t i,
				Sint16 x);
		/// Gets the Y coordinate of a point.
		Sint16 getY(size_t i) const;
		/// Sets the Y coordinate of a point.
		void setY(
				size_t i,
				Sint16 y);

		/// Gets the texture of the polygon.
		size_t getPolyTexture() const;
		/// Sets the texture of the polygon.
		void setPolyTexture(size_t tex);

		/// Gets the number of points of the polygon.
		size_t getPoints() const;
};

}

#endif
