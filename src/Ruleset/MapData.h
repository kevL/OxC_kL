/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_MAPDATA_H
#define OPENXCOM_MAPDATA_H

#include "RuleItem.h"


namespace OpenXcom
{

class MapDataSet;


enum SpecialTileType
{
	TILE,					// 0
	START_POINT,			// 1
	UFO_POWER_SOURCE,		// 2
	UFO_NAVIGATION,			// 3
	UFO_CONSTRUCTION,		// 4
	ALIEN_FOOD,				// 5
	ALIEN_REPRODUCTION,		// 6
	ALIEN_ENTERTAINMENT,	// 7
	ALIEN_SURGERY,			// 8
	EXAM_ROOM,				// 9
	ALIEN_ALLOYS,			// 10
	ALIEN_HABITAT,			// 11
	DEAD_TILE,				// 12
	END_POINT,				// 13
	MUST_DESTROY			// 14
};


enum MovementType
{
	MT_WALK,	// 0
	MT_FLY,		// 1
	MT_SLIDE	// 2
};


enum VoxelType
{
	VOXEL_EMPTY = -1,	// -1
	VOXEL_FLOOR,		// 0
	VOXEL_WESTWALL,		// 1
	VOXEL_NORTHWALL,	// 2
	VOXEL_OBJECT,		// 3
	VOXEL_UNIT,			// 4
	VOXEL_OUTOFBOUNDS	// 5
};


/**
 * MapData is the smallest piece of a Battlescape terrain,
 * holding info about a certain object, wall, floor, ...
 * @sa MapDataSet.
 */
class MapData
{

private:
	bool
		_baseModule,
		_blockFire,
		_blockSmoke,
		_isDoor,
		_isGravLift,
		_isNoFloor,
		_isUfoDoor,
		_stopLOS;
	int
		_armor,
		_altMCD,
		_bigWall,
		_block[6],
		_dieMCD,
		_explosive,
		_flammable,
		_footstepSound,
		_fuel,
		_lightSource,
		_loftID[12],
		_objectType,
		_sprite[8],
		_terrainLevel,
		_TUWalk,
		_TUFly,
		_TUSlide,
		_yOffset;
	unsigned short _miniMapIndex;

	MapDataSet* _dataset;
	SpecialTileType _specialType;


	public:
		static const int O_FLOOR;
		static const int O_WESTWALL;
		static const int O_NORTHWALL;
		static const int O_OBJECT;

		MapData(MapDataSet* dataset);
		~MapData();


		/// Gets the dataset this object belongs to.
		MapDataSet* getDataset() const;
		/// Gets the sprite index for a certain frame.
		int getSprite(int frameID) const;
		/// Sets the sprite index for a certain frame.
		void setSprite(int frameID, int value);
		/// Gets whether this is an animated ufo door.
		bool isUFODoor() const;
		/// Gets whether this is a floor.
		bool isNoFloor() const;
		/// Gets whether this is a big wall, which blocks all surrounding paths.
		int getBigWall() const;
		/// Gets whether this is a normal door.
		bool isDoor() const;
		/// Gets whether this is a grav lift.
		bool isGravLift() const;
		/// Sets all kinds of flags.
		void setFlags(
				bool isUfoDoor,
				bool stopLOS,
				bool isNoFloor,
				int bigWall,
				bool isGravLift,
				bool isDoor,
				bool blockFire,
				bool blockSmoke,
				bool baseModule);
		/// Gets the amount of blockage of a certain type.
		int getBlock(ItemDamageType type) const;
		/// Sets the amount of blockage for all types.
		void setBlockValue(
				int lightBlock,
				int visionBlock,
				int HEBlock,
				int smokeBlock,
				int fireBlock,
				int gasBlock);
		/// Gets the offset on the Y axis when drawing this object.
		int getYOffset() const;
		/// Sets the offset on the Y axis for drawing this object.
		void setYOffset(int value);
		/// Gets info about special tile types
		SpecialTileType getSpecialType() const;
		/// Get the type of tile.
		int getObjectType() const;
		/// Sets a special tile type and object type.
		void setSpecialType(int value, int otype);
		/// Gets the TU cost to move over the object.
		int getTUCost(MovementType movementType) const;
		/// Sets the TU cost to move over the object.
		void setTUCosts(int walk, int fly, int slide);
		/// Adds this to the graphical Y offset of units or objects on this tile.
		int getTerrainLevel() const;
		/// Sets Y offset for units/objects on this tile.
		void setTerrainLevel(int value);
		/// Gets the index to the footstep sound.
		int getFootstepSound() const;
		/// Sets the index to the footstep sound.
		void setFootstepSound(int value);
		/// Gets the alternative object ID.
		int getAltMCD() const;
		/// Sets the alternative object ID.
		void setAltMCD(int value);
		/// Gets the dead object ID.
		int getDieMCD() const;
		/// Sets the dead object ID.
		void setDieMCD(int value);
		/// Gets the amount of light the object is emitting.
		int getLightSource() const;
		/// Sets the amount of light the object is emitting.
		void setLightSource(int value);
		/// Gets the amount of armor.
		int getArmor() const;
		/// Sets the amount of armor.
		void setArmor(int value);
		/// Gets the amount of flammable.
		int getFlammable() const;
		/// Sets the amount of flammable.
		void setFlammable(int value);
		/// Gets the amount of fuel.
		int getFuel() const;
		/// Sets the amount of fuel.
		void setFuel(int value);
		/// Gets the loft index for a certain layer.
		int getLoftID(int layer) const;
		/// Sets the loft index for a certain layer.
		void setLoftID(int loft, int layer);
		/// Gets the amount of explosive.
		int getExplosive() const;
		/// Sets the amount of explosive.
		void setExplosive(int value);
		/// Sets the MiniMap index
		void setMiniMapIndex(unsigned short i);
		/// Gets the MiniMap index
		unsigned short getMiniMapIndex() const;
		/// Sets the bigwall value.
		void setBigWall(const int bigWall);
		/// Sets the TUWalk value.
		void setTUWalk(const int TUWalk);
		/// Sets the TUFly value.
		void setTUFly(const int TUFly);
		/// Sets the TUSlide value.
		void setTUSlide(const int TUSlide);
		/// Check if this is an xcom base object.
		bool isBaseModule();
};

}

#endif
