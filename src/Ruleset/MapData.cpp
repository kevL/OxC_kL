/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#include "../Engine/Logger.h"


namespace OpenXcom
{

/**
 * Creates a new Map Data Object.
 * @param dataset The dataset this object belongs to.
 */
MapData::MapData(MapDataSet* dataset)
	:
		_dataset(dataset),
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
		_TUFly(0),
		_TUSlide(0),
		_terrainLevel(0),
		_footstepSound(0),
		_dieMCD(0),
		_altMCD(0),
		_objectType(0),
		_lightSource(0),
		_armor(0),
		_flammable(0),
		_fuel(0),
		_explosive(0),
		_bigWall(0),
		_miniMapIndex(0)
{
	std::fill_n(_sprite, 8, 0);
	std::fill_n(_block, 6, 0);
	std::fill_n(_loftID, 12, 0);
}

/**
 * Destroys the object.
 */
MapData::~MapData()
{
}

// Explicit storage for MapData constants.
// Pathfinding::O_BIGWALL		= -1
const int MapData::O_FLOOR		= 0;
const int MapData::O_WESTWALL	= 1;
const int MapData::O_NORTHWALL	= 2;
const int MapData::O_OBJECT		= 3;


/**
 * Gets the dataset this object belongs to.
 * @return, Pointer to MapDataSet.
 */
MapDataSet* MapData::getDataset() const
{
	return _dataset;
}

/**
 * Gets the sprite index.
 * @param frameID Animation frame 0-7.
 * @return, The original sprite index.
 */
int MapData::getSprite(int frameID) const
{
	return _sprite[frameID];
}

/**
 * Sets the sprite index for a certain frame.
 * @param frameID Animation frame
 * @param value The sprite index in the surfaceset of the mapdataset.
 */
void MapData::setSprite(
		int frameID,
		int value)
{
	_sprite[frameID] = value;
}

/**
 * Gets whether this is an animated ufo door.
 * @return, True if this is an animated ufo door.
 */
bool MapData::isUFODoor() const
{
	return _isUfoDoor;
}

/**
 * kL. Gets whether this stops LoS.
 * @return, True if this stops LoS.
 */
bool MapData::stopLOS() const // kL
{
	return _stopLOS;
}

/**
 * Gets whether this is a floor.
 * @return, True if this is a floor.
 */
bool MapData::isNoFloor() const
{
	return _isNoFloor;
}

/**
 * Gets whether this is a big wall, which blocks all surrounding paths.
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
 * 8: acts as a south and east wall.
 * @return, An integer representing what kind of bigwall this is.
 */
int MapData::getBigWall() const
{
//kL	if (_terrainLevel < 0) // this is a hack for eg. Skyranger Ramps
//kL		return 0;

	return _bigWall;
}

/**
 * Gets whether this is a normal door.
 * @return, True if this is a normal door.
 */
bool MapData::isDoor() const
{
	return _isDoor;
}

/**
 * Gets whether this is a grav lift.
 * @return, True if this is a grav lift.
 */
bool MapData::isGravLift() const
{
	return _isGravLift;
}

/**
 * kL. Gets whether this blocks fire.
 * @return, true if this blocks fire
 */
bool MapData::blockFire() const // kL
{
	return _blockFire;
}

/**
 * kL. Sets whether this stops LoS.
 * @return, true if this stops LoS
 */
void MapData::setStopLOS(bool stopLOS) // kL
{
	_stopLOS = stopLOS;
}

/**
 * Sets all kinds of flags.
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
	_isUfoDoor	= isUfoDoor;
	_stopLOS	= stopLOS;
	_isNoFloor	= isNoFloor;
	_bigWall	= bigWall;
	_isGravLift	= isGravLift;
	_isDoor		= isDoor;
	_blockFire	= blockFire;
	_blockSmoke	= blockSmoke;
	_baseModule = baseModule;
}

/**
 * Gets the amount of blockage of a certain type.
 * @param type Type.
 * @return, The blockage (0-255).
 */
int MapData::getBlock(ItemDamageType type) const
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

		default:
		break;
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
//kL	_block[1] = visionBlock == 1? 255: 0; // <- why? kL_note. haha
	_block[1] = visionBlock == 1? 100: 0; // kL
		// stopLoS==true needs to be a significantly large integer (only about 10+ really)
		// so that if a directionally opposite Field of View check includes a "-1",
		// meaning block by bigWall or other content-object, the result is not reduced
		// to zero (no block at all) when added to regular stopLoS by a standard wall.
		//
		// It would be unnecessary to use that jigger-pokery if TileEngine::
		// horizontalBlockage() & blockage() were coded differently [verticalBlockage()
		// too, perhaps]
	_block[2] = HEBlock;
//kL	_block[3] = smokeBlock == 1? 256: 0; // <- why? kL_note. I basically use visionBlock for smoke ....
	_block[3] = smokeBlock;
	_block[4] = fireBlock; // this is Flammable, NOT Block_Fire.
	_block[5] = gasBlock;
}

/**
 * Sets the amount of HE blockage.
 * @param HEBlock The high explosive blockage.
 */
void MapData::setHEBlock(int HEBlock)
{
	_block[2] = HEBlock;
}

/**
 * Gets the Y offset for drawing.
 * @return, The height in pixels.
 */
int MapData::getYOffset() const
{
	return _yOffset;
}

/**
 * Sets the offset on the Y axis for drawing this object.
 * @param value The offset.
 */
void MapData::setYOffset(int value)
{
	_yOffset = value;
}

/**
 * Gets the type of object.
 * @return, The object type (0-3).
 */
int MapData::getObjectType() const
{
	return _objectType;
}

/**
 * Gets info about special tile types.
 * @return, The special tile type.
 */
SpecialTileType MapData::getSpecialType() const
{
	return _specialType;
}

/**
 * Sets a special tile type and object type.
 * @param value Special tile type.
 * @param otype Object type.
 */
void MapData::setSpecialType(
		int value,
		int otype)
{
	_specialType = static_cast<SpecialTileType>(value);
	_objectType = otype;
}

/**
 * Gets the TU cost to move over the object.
 * @param movementType - the movement type
 * @return, the TU cost
 */
int MapData::getTUCost(MovementType movementType) const
{
	//Log(LOG_INFO) << "MapData::getTUCost()";
	switch (movementType)
	{
		case MT_WALK:
			//Log(LOG_INFO) << ". MT_WALK ret = " << _TUWalk;
		return _TUWalk;

		case MT_FLY:
			//Log(LOG_INFO) << ". MT_FLY ret = " << _TUFly;
		return _TUFly;

		case MT_SLIDE:
			//Log(LOG_INFO) << ". MT_SLIDE ret = " << _TUSlide;
		return _TUSlide;

		default:
		break;
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
	_TUWalk		= walk;
	_TUFly		= fly;
	_TUSlide	= slide;
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
 * @return, The sound ID.
 */
int MapData::getFootstepSound() const
{
	return _footstepSound;
}

/**
 * Sets the index to the footstep sound.
 * @param value The sound ID.
 */
void MapData::setFootstepSound(int value)
{
	_footstepSound = value;
}

/**
 * Gets the alternative object ID.
 * @return, The alternative object ID.
 */
int MapData::getAltMCD() const
{
	return _altMCD;
}

/**
 * Sets the alternative object ID.
 * @param value The alternative object ID.
 */
void MapData::setAltMCD(int value)
{
	_altMCD = value;
}

/**
 * Gets the dead object ID.
 * @return, The dead object ID.
 */
int MapData::getDieMCD() const
{
	return _dieMCD;
}

/**
 * Sets the dead object ID.
 * @param value The dead object ID.
 */
void MapData::setDieMCD(int value)
{
	_dieMCD = value;
}

/**
 * Gets the amount of light the object is emitting.
 * @return, The amount of light emitted.
 */
int MapData::getLightSource() const
{
	if (_lightSource == 1)	// lamp posts have 1,
		return 15;			// but they should emit more light
	else
		return _lightSource - 1;
}

/**
 * Sets the amount of light the object is emitting.
 * @param value The amount of light emitted.
 */
void MapData::setLightSource(int value)
{
	_lightSource = value;
}

/**
 * Gets the amount of armor.
 * Total hitpoints of a tile before destroyed.
 * @return, The amount of armor.
 */
int MapData::getArmor() const
{
	return _armor;
}

/**
 * Sets the amount of armor.
 * Total hitpoints of a tile before destroyed.
 * @param value The amount of armor.
 */
void MapData::setArmor(int value)
{
	_armor = value;
}

/**
 * Gets the amount of flammable (how flammable this object is).
 * @return, The amount of flammable.
 */
int MapData::getFlammable() const
{
	return _flammable;
}

/**
 * Sets the amount of flammable (how flammable this object is).
 * @param value The amount of flammable.
 */
void MapData::setFlammable(int value)
{
	_flammable = value;
}

/**
 * Gets the amount of fuel.
 * @return, The amount of fuel.
 */
int MapData::getFuel() const
{
	return _fuel;
}

/**
 * Sets the amount of fuel.
 * @param value The amount of fuel.
 */
void MapData::setFuel(int value)
{
	_fuel = value;
}

/**
 * Gets the loft index for a certain layer.
 * @param layer The layer.
 * @return, The loft index.
 */
int MapData::getLoftID(int layer) const
{
	return _loftID[layer];
}

/**
 * Sets the loft index for a certain layer.
 * @param loft The loft index.
 * @param layer The layer.
 */
void MapData::setLoftID(
		int loft,
		int layer)
{
	_loftID[layer] = loft;
}

/**
 * Gets the amount of explosive.
 * @return, The amount of explosive.
 */
int MapData::getExplosive() const
{
	return _explosive;
}

/**
 * Sets the amount of explosive.
 * @param value The amount of explosive.
 */
void MapData::setExplosive(int value)
{
	_explosive = value;
}

/**
 * Sets the SCANG.DAT index for minimap.
 * @param i The minimap index.
 */
void MapData::setMiniMapIndex(unsigned short i)
{
	_miniMapIndex = i;
}

/**
 * Gets the SCANG.DAT index for minimap.
 * @return, The minimap index.
 */
unsigned short MapData::getMiniMapIndex() const
{
	return _miniMapIndex;
}

/**
 * Sets the bigWall value.
 * @param bigWall The new bigWall value.
 */
void MapData::setBigWall(const int bigWall)
{
	_bigWall = bigWall;
}

/**
 * Sets the TUWalk value.
 * @param TUWalk The new TUWalk value.
 */
void MapData::setTUWalk(const int TUWalk)
{
	_TUWalk = TUWalk;
}

/**
 * Sets the TUFly value.
 * @param TUFly The new TUFly value.
 */
void MapData::setTUFly(const int TUFly)
{
	_TUFly = TUFly;
}

/**
 * Sets the TUSlide value.
 * @param TUSlide The new TUSlide value.
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
bool MapData::isBaseModule()
{
	return _baseModule;
}

}
