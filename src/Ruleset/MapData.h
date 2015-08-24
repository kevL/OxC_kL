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
	MT_FLY,		// 1 note Fly & Slide should be switched here and in ruleset-armors
	MT_SLIDE,	// 2 to conform to their order in MCD files. because.
	MT_FLOAT	// 3
};


enum VoxelType
{
	VOXEL_EMPTY = -1,	// -1
	VOXEL_FLOOR,		//  0
	VOXEL_WESTWALL,		//  1
	VOXEL_NORTHWALL,	//  2
	VOXEL_OBJECT,		//  3
	VOXEL_UNIT,			//  4
	VOXEL_OUTOFBOUNDS	//  5
};


enum MapDataType
{
	O_BIGWALL = -1,	// -1
	O_FLOOR,		//  0
	O_WESTWALL,		//  1
	O_NORTHWALL,	//  2
	O_OBJECT,		//  3
	O_NULTYPE		//  4
};


/**
 * MapData is the smallest piece of a Battlescape terrain, holding info about a
 * certain object, wall, floor, etc.
 * @note A better ident for this class would be 'TileData' or even 'PartData'.
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
		_explosiveType,
		_flammable,
		_footstepSound,
		_fuel,
		_isPsychedelic,
		_lightSource,
		_sprite[8],
		_terrainLevel,
		_TUWalk,
		_TUFly,
		_TUSlide,
		_yOffset;

	size_t _loftId[12];

	unsigned short _miniMapIndex;

	MapDataSet* _dataset;

	MapDataType _objectType;
	SpecialTileType _specialType;


	public:
		/// cTor.
		explicit MapData(MapDataSet* const dataSet);
		/// dTor.
		~MapData();


		/// Gets the dataset this object belongs to.
		MapDataSet* getDataset() const;

		/// Gets the sprite index for a certain frame.
		int getSprite(int aniFrame) const;
		/// Sets the sprite index for a certain frame.
		void setSprite(
				size_t aniFrame,
				int id);

		/// Gets whether this is an animated ufo door.
		bool isUfoDoor() const;
		/// Gets whether this stops LoS.
		bool stopLOS() const;
		/// Gets whether this is a floor.
		bool isNoFloor() const;
		/// Gets whether this is a big wall.
		int getBigWall() const;
		/// Gets whether this is a normal door.
		bool isDoor() const;
		/// Gets whether this is a grav lift.
		bool isGravLift() const;
		/// Gets whether this blocks smoke.
		bool blockSmoke() const;
		/// Gets whether this blocks fire.
		bool blockFire() const;

		/// Sets whether this stops LoS.
		void setStopLOS(bool stopLOS = true);

		/// Sets all kinds of flags.
		void setFlags(
				bool isUfoDoor,
				bool stopLOS,
				bool isNoFloor,
				int  bigWall,
				bool isGravLift,
				bool isDoor,
				bool blockFire,
				bool blockSmoke,
				bool baseModule);

		/// Gets the amount of blockage of a certain type.
		int getBlock(ItemDamageType type) const;
		/// Sets the amount of blockage for all types.
		void setBlock(
				int lightBlock,
				int visionBlock,
				int HEBlock,
				int smokeBlock,
				int fireBlock,
				int gasBlock);
		/// Sets the amount of HE blockage.
		void setHEBlock(int HEBlock);

		/// Gets the offset on the Y axis when drawing this object.
		int getYOffset() const;
		/// Sets the offset on the Y axis for drawing this object.
		void setYOffset(int value);

		/// Gets the type of tile.
		MapDataType getPartType() const;
		/// Sets the type of tile.
		void setObjectType(MapDataType type);

		/// Gets info about special tile types
		SpecialTileType getSpecialType() const;
		/// Sets a special tile type and object type.
		void setSpecialType(SpecialTileType type);

		/// Gets the TU cost to move over the object.
		int getTuCostPart(MovementType moveType) const;
		/// Sets the TU cost to move over the object.
		void setTUCosts(
				int walk,
				int fly,
				int slide);

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
		size_t getLoftId(size_t layer) const;
		/// Sets the loft index for a certain layer.
		void setLoftId(
				size_t loft,
				size_t layer);

		/// Gets the amount of explosive.
		int getExplosive() const;
		/// Sets the amount of explosive.
		void setExplosive(int value);
		/// Gets the type of explosive.
		int getExplosiveType() const;
		/// Sets the type of explosive.
		void setExplosiveType(int value);

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

		/// Sets this tile as not a floor.
		void setNoFloor(bool isNoFloor);

		/// Check if this is an xcom base object.
		bool isBaseModule() const;

		/// Gets if this tilepart is psychedelic.
		int isPsychedelic() const;
};

}

#endif
