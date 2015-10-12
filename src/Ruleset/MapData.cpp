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

#include "MapData.h"

#include "MapDataSet.h"


namespace OpenXcom
{

/**
 * Creates a new Map Data Object.
 * @param dataSet - pointer to the MapDataSet this object belongs to
 */
MapData::MapData(MapDataSet* const dataSet)
	:
		_dataset(dataSet),
		_specialType(TILE),
		_isUfoDoor(false),
		_stopLOS(false),
		_isNoFloor(false),
		_isGravLift(false),
		_isDoor(false),
		_blockFire(false),
		_blockSmoke(false),
		_baseModule(false),
		_yOffset(0),
		_TUWalk(0),
		_TUSlide(0),
		_TUFly(0),
		_terrainLevel(0),
		_footstepSound(0),
		_dieMCD(0),
		_altMCD(0),
		_objectType(O_FLOOR),
		_lightSource(0),
		_armor(0),
		_flammable(0),
		_fuel(0),
		_explosive(0),
		_explosiveType(0),
		_bigWall(0),
		_miniMapIndex(0),
		_isPsychedelic(0)
{
	std::fill_n(_sprite, 8,0);
	std::fill_n(_block, 6,0);
	std::fill_n(_loftId, 12,0);
}

/**
 * Destroys the object.
 */
MapData::~MapData()
{}

/**
 * Gets the dataset this object belongs to.
 * @return, pointer to MapDataSet
 */
MapDataSet* MapData::getDataset() const
{
	return _dataset;
}

/**
 * Gets the sprite index.
 * @param aniFrame - animation frame 0-7
 * @return, the original sprite index
 */
int MapData::getSprite(int aniFrame) const
{
	return _sprite[static_cast<size_t>(aniFrame)];
}

/**
 * Sets the sprite index for a certain frame.
 * @param aniFrame	- animation frame
 * @param id		- the sprite index in the surfaceset of the mapdataset
 */
void MapData::setSprite(
		size_t aniFrame,
		int id)
{
	_sprite[aniFrame] = id;
}

/**
 * Gets whether this is an animated ufo door.
 * @return, true if this is an animated ufo door
 */
bool MapData::isUfoDoor() const
{
	return _isUfoDoor;
}

/**
 * Gets whether this stops LoS.
 * @return, true if this stops LoS
 */
bool MapData::stopLOS() const
{
	return _stopLOS;
}

/**
 * Gets whether this is a floor.
 * @return, true if this is not a floor
 */
bool MapData::isNoFloor() const
{
	return _isNoFloor;
}

/**
 * Gets whether this is a big wall, which blocks all surrounding paths.
 * See Pathfinding.h enum
 *
 * Return value key:
 * 0: not a bigWall
 * 1: regular bigWall
 * 2: allows movement in ne/sw direction
 * 3: allows movement in nw/se direction
 * 4: acts as a west wall
 * 5: acts as a north wall
 * 6: acts as an east wall
 * 7: acts as a south wall
 * 8: acts as a south and east wall
 * 9: acts as a north and west wall
 * @return, an integer representing what kind of bigwall this is
 */
int MapData::getBigWall() const
{
	return _bigWall;
}

/**
 * Gets whether this is a normal door.
 * @return, true if this is a normal door
 */
bool MapData::isDoor() const
{
	return _isDoor;
}

/**
 * Gets whether this is a grav lift.
 * @return, true if this is a grav lift
 */
bool MapData::isGravLift() const
{
	return _isGravLift;
}

/**
 * Gets whether this tile part blocks smoke.
 * @return, true if it blocks smoke
 */
bool MapData::blockSmoke() const
{
	return _blockSmoke;
}

/**
 * Gets whether this tile part blocks fire.
 * @return, true if it blocks fire
 */
bool MapData::blockFire() const
{
	return _blockFire;
}

/**
 * Sets whether this tile part stops LoS.
 * @return, true if stops LoS (default true)
 */
void MapData::setStopLOS(bool stopLOS)
{
	_stopLOS = stopLOS;
	_block[1] = stopLOS ? 100 : 0;
}

/**
 * Sets a bunch of flags.
 * @param isUfoDoor		- true if this is a ufo door
 * @param stopLOS		- true if this stops line of sight
 * @param isNoFloor		- true if this is *not* a floor
 * @param bigWall		- type of a bigWall
 * @param isGravLift	- true if this is a grav lift
 * @param isDoor		- true if this is a normal door
 * @param blockFire		- true if this blocks fire
 * @param blockSmoke	- true if this blocks smoke
 * @param baseModule	- true if this is a base module (objective) item
 */
void MapData::setFlags(
		bool isUfoDoor,
		bool stopLOS,
		bool isNoFloor,
		int  bigWall,
		bool isGravLift,
		bool isDoor,
		bool blockFire,
		bool blockSmoke,
		bool baseModule)
{
	_isUfoDoor = isUfoDoor;
	_stopLOS = stopLOS;
	_isNoFloor = isNoFloor;
	_bigWall = bigWall;
	_isGravLift = isGravLift;
	_isDoor = isDoor;
	_blockFire = blockFire;
	_blockSmoke = blockSmoke;
	_baseModule = baseModule;
}

/**
 * Gets the amount of blockage of a certain type.
 * @param type - DamageType (RuleItem.h)
 * @return, the blockage (0-255)
 */
int MapData::getBlock(DamageType type) const
{
	switch (type)
	{
/*		case DT_NONE:	return _block[1];
		case DT_HE:		return _block[2];
		case DT_SMOKE:	return _block[3];
		case DT_IN:		return _block[4];
		case DT_STUN:	return _block[5]; */
											// see setBlock() below.
		case DT_NONE:	return _block[1];	// stop LoS: [0 or 100], was [0 or 255]
		case DT_HE:
		case DT_IN:
		case DT_STUN:	return _block[2];	// HE block [int]
		case DT_SMOKE:	return _block[3];	// block smoke: try (bool), was [0 or 256]
	}

	return 0;
}

/**
 * Sets the amount of blockage for all types.
 * @param lightBlock	- light blockage			- Light_Block
 * @param visionBlock	- vision blockage			- Stop_LOS
 * @param HEBlock		- high explosive blockage	- HE_Block
 * @param smokeBlock	- smoke blockage			- Block_Smoke
 * @param fireBlock		- fire blockage				- Flammable (lower = more flammable)
 * @param gasBlock		- gas blockage				- HE_Block
 */
void MapData::setBlock(
		int lightBlock,
		int visionBlock,
		int HEBlock,
		int smokeBlock,
		int fireBlock,
		int gasBlock)
{
/*	_block[0] = lightBlock; // not used...
	_block[1] = visionBlock == 1? 255: 0;
	_block[2] = HEBlock;
	_block[3] = smokeBlock == 1? 255: 0;
	_block[4] = fireBlock == 1? 255: 0;
	_block[5] = gasBlock == 1? 255: 0; */

	_block[0] = lightBlock; // not used
//	_block[1] = visionBlock; // kL
//	_block[1] = visionBlock == 1? 255: 0; // <- why? kL_note. haha
	_block[1] = visionBlock == 1 ? 100 : 0; // kL
		// stopLoS==true needs to be a significantly large integer (only about 10+ really)
		// so that if a directionally opposite Field of View check includes a "-1",
		// meaning block by bigWall or other content-object, the result is not reduced
		// to zero (no block at all) when added to regular stopLoS by a standard wall.
		//
		// It would be unnecessary to use that jigger-pokery if TileEngine::
		// horizontalBlockage() & blockage() were coded differently [verticalBlockage()
		// too, perhaps]
	_block[2] = HEBlock;
//	_block[3] = smokeBlock == 1? 256: 0; // <- why? kL_note. I basically use visionBlock for smoke ....
	_block[3] = smokeBlock;
	_block[4] = fireBlock; // this is Flammable, NOT Block_Fire.
	_block[5] = gasBlock;
}

/**
 * Sets the amount of HE blockage.
 * @param HEBlock - the high explosive blockage
 */
void MapData::setHEBlock(int HEBlock)
{
	_block[2] = HEBlock;
}

/**
 * Gets the Y offset for drawing.
 * @return, the height in pixels
 */
int MapData::getYOffset() const
{
	return _yOffset;
}

/**
 * Sets the offset on the Y axis for drawing this object.
 * @param value - the offset
 */
void MapData::setYOffset(int value)
{
	_yOffset = value;
}

/**
 * Gets the type of object.
 * @return, the object type (0-3)
 */
MapDataType MapData::getPartType() const
{
	return _objectType;
}

/**
 * Sets the type of object.
 * @note Sets '_isPsychedelic' also.
 * @param type - the object type (0-3)
 */
void MapData::setObjectType(MapDataType type)
{
	_objectType = type;

	if (_dataset->getName() == "U_PODS") // kL-> should put this in MCDPatch
	{
		if (_sprite[0] == 7			// disco walls, yellow northWall
			|| _sprite[0] == 8		//  "     "       "    westWall
			|| _sprite[0] == 17		// disco walls, blue northWall
			|| _sprite[0] == 18)	//  "     "       "  westWall
//			|| _sprite[0] == 4)		// disco ball
		{
			_isPsychedelic = 1;
		}
		else if (_sprite[0] == 0)	// red round energy supply
//			||   _sprite[0] == 2)	// red oblong  "      "
		{
			_isPsychedelic = 2;
		}
	}
}

/**
 * Gets info about special tile types.
 * @return, the special tile type
 */
SpecialTileType MapData::getSpecialType() const
{
	return _specialType;
}

/**
 * Sets a special tile type and object type.
 * @param value	- special tile type (MapData.h)
 */
void MapData::setSpecialType(SpecialTileType type)
{
	_specialType = type;
}

/**
 * Gets the TU cost to move over the object/wall/floor.
 * @param moveType - the movement type
 * @return, the TU cost
 */
int MapData::getTuCostPart(MovementType moveType) const
{
	switch (moveType)
	{
		case MT_WALK:	return _TUWalk;
		case MT_SLIDE:	return _TUSlide;
		case MT_FLY:	return _TUFly;
	}

	return 0;
}

/**
 * Sets the TU cost to move over the object.
 * @param walk	- the walking TU cost
 * @param fly	- the flying TU cost
 * @param slide	- the sliding TU cost
 */
void MapData::setTUCosts(
		int walk,
		int fly,
		int slide)
{
	_TUWalk = walk;
	_TUFly = fly;
	_TUSlide = slide;
}

/**
 * Adds this to the graphical Y offset of units or objects on this tile.
 * @return, Y offset
 */
int MapData::getTerrainLevel() const
{
	return _terrainLevel;
}

/**
 * Sets the Y offset for units/objects on this tile.
 * @param value - Y offset
 */
void MapData::setTerrainLevel(int value)
{
	_terrainLevel = value;
}

/**
 * Gets the index to the footstep sound.
 * @return, the sound ID
 */
int MapData::getFootstepSound() const
{
	return _footstepSound;
}

/**
 * Sets the index to the footstep sound.
 * @param value - the sound ID
 */
void MapData::setFootstepSound(int value)
{
	_footstepSound = value;
}

/**
 * Gets the alternative object ID.
 * @return, the alternative object ID
 */
int MapData::getAltMCD() const
{
	return _altMCD;
}

/**
 * Sets the alternative object ID.
 * @param value - The alternative object ID
 */
void MapData::setAltMCD(int value)
{
	_altMCD = value;
}

/**
 * Gets the dead object ID.
 * @return, the dead object ID
 */
int MapData::getDieMCD() const
{
	return _dieMCD;
}

/**
 * Sets the dead object ID.
 * @param value - the dead object ID
 */
void MapData::setDieMCD(int value)
{
	_dieMCD = value;
}

/**
 * Gets the amount of light the object is emitting.
 * @return, the amount of light emitted
 */
int MapData::getLightSource() const
{
	if (_lightSource == 1)	// lamp posts have 1
		return 15;			// but they should emit more light

	return _lightSource - 1;
}

/**
 * Sets the amount of light the object is emitting.
 * @param value - the amount of light emitted
 */
void MapData::setLightSource(int value)
{
	_lightSource = value;
}

/**
 * Gets the amount of armor. Total hitpoints of a tile before destroyed.
 * @return, the amount of armor
 */
int MapData::getArmor() const
{
	return _armor;
}

/**
 * Sets the amount of armor. Total hitpoints of a tile before destroyed.
 * @param value - the amount of armor
 */
void MapData::setArmor(int value)
{
	_armor = value;
}

/**
 * Gets the amount of flammable (how flammable this object is).
 * @return, the amount of flammable
 */
int MapData::getFlammable() const
{
	return _flammable;
}

/**
 * Sets the amount of flammable (how flammable this object is).
 * @param value - the amount of flammable
 */
void MapData::setFlammable(int value)
{
	_flammable = value;
}

/**
 * Gets the amount of fuel.
 * @return, the amount of fuel
 */
int MapData::getFuel() const
{
	return _fuel;
}

/**
 * Sets the amount of fuel.
 * @param value - the amount of fuel
 */
void MapData::setFuel(int value)
{
	_fuel = value;
}

/**
 * Gets the loft index for a certain layer.
 * @param layer - the layer
 * @return, the LOFT index
 */
size_t MapData::getLoftId(size_t layer) const
{
	return _loftId[layer];
}

/**
 * Sets the loft index for a certain layer.
 * @param loft	- the LOFT index
 * @param layer	- the layer (0..11)
 */
void MapData::setLoftId(
		size_t loft,
		size_t layer)
{
	_loftId[layer] = loft;
}

/**
 * Gets the amount of explosive.
 * @return, the amount of explosive
 */
int MapData::getExplosive() const
{
	return _explosive;
}

/**
 * Sets the amount of explosive.
 * @param value - the amount of explosive
 */
void MapData::setExplosive(int value)
{
	_explosive = value;
}

/**
 * Gets the type of explosive.
 * @return, the amount of explosive
 */
int MapData::getExplosiveType() const
{
	return _explosiveType;
}

/**
 * Sets the type of explosive.
 * @param value - the type of explosive
 */
void MapData::setExplosiveType(int value)
{
	_explosiveType = value;
}

/**
 * Sets the SCANG.DAT index for minimap.
 * @param i - the minimap index
 */
void MapData::setMiniMapIndex(unsigned short i)
{
	_miniMapIndex = i;
}

/**
 * Gets the SCANG.DAT index for minimap.
 * @return, the minimap index
 */
unsigned short MapData::getMiniMapIndex() const
{
	return _miniMapIndex;
}

/**
 * Sets the bigWall value.
 * @param bigWall - the new bigWall value
 */
void MapData::setBigWall(const int bigWall)
{
	_bigWall = bigWall;
}

/**
 * Sets the TUWalk value.
 * @param TUWalk - the new TUWalk value
 */
void MapData::setTUWalk(const int TUWalk)
{
	_TUWalk = TUWalk;
}

/**
 * Sets the TUFly value.
 * @param TUFly - the new TUFly value
 */
void MapData::setTUFly(const int TUFly)
{
	_TUFly = TUFly;
}

/**
 * Sets the TUSlide value.
 * @param TUSlide - the new TUSlide value
 */
void MapData::setTUSlide(const int TUSlide)
{
	_TUSlide = TUSlide;
}

/**
 * Sets the "no floor" flag.
 * @param isNoFloor - true if the tile has no floor part
 */
void MapData::setNoFloor(bool isNoFloor)
{
	_isNoFloor = isNoFloor;
}

/**
 * Check if this is an xcom base object.
 * @return, true if it is a base object
 */
bool MapData::isBaseModule() const
{
	return _baseModule;
}

/**
 * Gets if this tilepart is psychedelic.
 * @return, true if psycho
 */
int MapData::isPsychedelic() const
{
	return _isPsychedelic;
}

}
