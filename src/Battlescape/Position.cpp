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

#include "Position.h"


namespace OpenXcom
{

/**
 * Converts voxel-space to tile-space.
 * @param pos - reference a position
 */
Position Position::toTileSpace(const Position& pos)
{
	return Position(
				pos.x >> 4,
				pos.y >> 4,
				pos.z / 24);
}

/**
 * Converts tile-space to voxel-space.
 * @param pos - reference a position
 */
Position Position::toVoxelSpace(const Position& pos)
{
	return Position(
				pos.x << 4,
				pos.y << 4,
				pos.z * 24);
}

/**
 * Converts tile-space to voxel-space and centers the voxel in its Tile.
 * @param pos		- reference a position
 * @param lift		- how many voxels to elevate along the z-axis (default 0)
 * @param armorSize	- tilesize of unit armor if applicable (default 1)
 */
Position Position::toVoxelSpaceCentered(
		const Position& pos,
		int lift,
		int armorSize)
{
	const int voxelOffset = armorSize * 8;
	return Position(
				(pos.x << 4) + voxelOffset,
				(pos.y << 4) + voxelOffset,
				(pos.z * 24) + lift);
}

}
