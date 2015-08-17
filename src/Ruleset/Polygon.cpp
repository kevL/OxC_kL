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

//#define _USE_MATH_DEFINES

#include "Polygon.h"

//#include <cmath>


namespace OpenXcom
{

/**
 * Initializes the polygon with arrays to store each point's coordinates.
 * @param points - number of points
 */
Polygon::Polygon(size_t points)
	:
		_points(points),
		_texture(0)
{
	_lat = new double[_points];
	_lon = new double[_points];
	_x = new Sint16[_points];
	_y = new Sint16[_points];

	for (size_t
			i = 0;
			i != _points;
			++i)
	{
		_lat[i] = 0.;
		_lon[i] = 0.;
		_x[i] = 0;
		_y[i] = 0;
	}
}

/**
 * Performs a deep copy of an existing polygon.
 * @param other - reference another Polygon to copy
 */
Polygon::Polygon(const Polygon& other)
{
	_points	= other._points;
	_lat = new double[_points];
	_lon = new double[_points];
	_x = new Sint16[_points];
	_y = new Sint16[_points];

	for (size_t
			i = 0;
			i != _points;
			++i)
	{
		_lat[i] = other._lat[i];
		_lon[i] = other._lon[i];
		_x[i] = other._x[i];
		_y[i] = other._y[i];
	}

	_texture = other._texture;
}

/**
 * Deletes the arrays from memory.
 */
Polygon::~Polygon()
{
	delete[] _lat;
	delete[] _lon;
	delete[] _x;
	delete[] _y;
}

/**
 * Loads the polygon from a YAML file.
 * @param node - reference a YAML node
 */
void Polygon::load(const YAML::Node& node)
{
	delete[] _lat;
	delete[] _lon;
	delete[] _x;
	delete[] _y;

	std::vector<double> coords = node.as<std::vector<double> >();
	_points = (coords.size() - 1) / 2;

	_lat = new double[_points];
	_lon = new double[_points];

	_x = new Sint16[_points];
	_y = new Sint16[_points];

	_texture = static_cast<size_t>(coords[0]);

	for (size_t
			i = 1;
			i < coords.size();
			i += 2)
	{
		size_t j = (i - 1) / 2;

		_lon[j] = coords[i] * M_PI / 180.;
		_lat[j] = coords[i + 1] * M_PI / 180.;

		_x[j] =
		_y[j] = 0;
	}
}

/**
 * Returns the latitude of a given point.
 * @param i - point number (0-max)
 * @return, point's latitude
 */
double Polygon::getLatitude(size_t i) const
{
	return _lat[i];
}

/**
 * Changes the latitude of a given point.
 * @param i		- point number (0-max)
 * @param lat	- point's latitude
 */
void Polygon::setLatitude(
		size_t i,
		double lat)
{
	_lat[i] = lat;
}

/**
 * Returns the longitude of a given point.
 * @param i - point number (0-max)
 * @return, point's longitude
 */
double Polygon::getLongitude(size_t i) const
{
	return _lon[i];
}

/**
 * Changes the latitude of a given point.
 * @param i		- point number (0-max)
 * @param lon	- point's longitude
 */
void Polygon::setLongitude(
		size_t i,
		double lon)
{
	_lon[i] = lon;
}

/**
 * Returns the X coordinate of a given point.
 * @param i - point number (0-max)
 * @return, point's X coordinate
 */
Sint16 Polygon::getX(size_t i) const
{
	return _x[i];
}

/**
 * Changes the X coordinate of a given point.
 * @param i - point number (0-max)
 * @param x - point's X coordinate
 */
void Polygon::setX(
		size_t i,
		Sint16 x)
{
	_x[i] = x;
}

/**
 * Returns the Y coordinate of a given point.
 * @param i - point number (0-max)
 * @return, point's Y coordinate
 */
Sint16 Polygon::getY(size_t i) const
{
	return _y[i];
}

/**
 * Changes the Y coordinate of a given point.
 * @param i - point number (0-max)
 * @param y - point's Y coordinate
 */
void Polygon::setY(
		size_t i,
		Sint16 y)
{
	_y[i] = y;
}

/**
 * Returns the texture used to draw the polygon - textures are stored in a set.
 * @return, texture sprite number
 */
size_t Polygon::getPolyTexture() const
{
	return _texture;
}

/**
 * Changes the texture used to draw the polygon.
 * @param tex - texture sprite number
 */
void Polygon::setPolyTexture(size_t tex)
{
	_texture = tex;
}

/**
 * Returns the number of points (vertexes) that make up the polygon.
 * @return, number of points
 */
size_t Polygon::getPoints() const
{
	return _points;
}

}
