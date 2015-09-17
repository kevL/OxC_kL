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

#ifndef OPENXCOM_POSITION_H
#define OPENXCOM_POSITION_H

//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Easy handling of X-Y-Z coordinates.
 */
class Position
{

public:
	int
		x,y,z;

	/// Null position constructor.
	Position()
		:
			x(0),
			y(0),
			z(0)
	{};
	/// 3d position constructor.
	Position(
			int x_,
			int y_,
			int z_)
		:
			x(x_),
			y(y_),
			z(z_)
	{};
	/// Copy constructor.
	Position(const Position& pos)
		:
			x(pos.x),
			y(pos.y),
			z(pos.z)
	{};

	Position& operator= (const Position& pos)
	{
		if (this != &pos) // kL: cf. RuleUnit.h
		{
			x = pos.x;
			y = pos.y;
			z = pos.z;
		}
		return *this;
	}

	Position operator+ (const Position& pos) const
	{
		return Position(
						x + pos.x,
						y + pos.y,
						z + pos.z);
	}
	Position& operator+= (const Position& pos)
	{
		x += pos.x;
		y += pos.y;
		z += pos.z;
		return *this;
	}

	Position operator- (const Position& pos) const
	{
		return Position(
						x - pos.x,
						y - pos.y,
						z - pos.z);
	}
	Position& operator-= (const Position& pos)
	{
		x -= pos.x;
		y -= pos.y;
		z -= pos.z;
		return *this;
	}

	Position operator* (const Position& pos) const
	{
		return Position(
						x * pos.x,
						y * pos.y,
						z * pos.z);
	}
	Position& operator*= (const Position& pos)
	{
		x *= pos.x;
		y *= pos.y;
		z *= pos.z;
		return *this;
	}
	Position operator* (const int vect) const
	{
		return Position(
						x * vect,
						y * vect,
						z * vect);
	}
	Position& operator*= (const int vect)
	{
		x *= vect;
		y *= vect;
		z *= vect;
		return *this;
	}

	Position operator/ (const Position& pos) const
	{
		return Position(
						x / pos.x,
						y / pos.y,
						z / pos.z);
	}
	Position& operator/= (const Position& pos)
	{
		x /= pos.x;
		y /= pos.y;
		z /= pos.z;
		return *this;
	}
	Position operator/ (const int vect) const
	{
		return Position(
						x / vect,
						y / vect,
						z / vect);
	}
	Position& operator/= (const int vect)	// kL_begin:
	{
		x /= vect;
		y /= vect;
		z /= vect;
		return *this;
	}										// kL_end.

	/// == operator
	bool operator== (const Position& pos) const
	{
		return
			   x == pos.x
			&& y == pos.y
			&& z == pos.z;
	}

	/// != operator
	bool operator!= (const Position& pos) const
	{
		return
			   x != pos.x
			|| y != pos.y
			|| z != pos.z;
	}

	/// Converts voxel-space to tile-space.
	static Position toTileSpace(const Position& pos);
	/// Converts tile-space to voxel-space.
	static Position toVoxelSpace(const Position& pos);
	/// Converts tile-space to voxel-space and centers the voxel in its Tile.
	static Position toVoxelSpaceCentered(
			const Position& pos,
			int lift = 0,
			int armorSize = 1);
};

///
inline std::ostream& operator<< (std::ostream& ostr, const Position& pos)
{
	ostr << "(" << pos.x << "," << pos.y << "," << pos.z << ")";
	return ostr;
}
///
inline std::wostream& operator<< (std::wostream& wostr, const Position& pos)
{
	wostr << "(" << pos.x << "," << pos.y << "," << pos.z << ")";
	return wostr;
}

}


namespace YAML
{

template<>
struct convert<OpenXcom::Position>
{
	///
	static Node encode(const OpenXcom::Position& rhs)
	{
		Node node;

		node.push_back(rhs.x);
		node.push_back(rhs.y);
		node.push_back(rhs.z);

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::Position& rhs)
	{
		if (node.IsSequence() == false
			|| node.size() != 3)
		{
			return false;
		}

		rhs.x = node[0].as<int>();
		rhs.y = node[1].as<int>();
		rhs.z = node[2].as<int>();

		return true;
	}
};

}

#endif
