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

#include "Tile.h"

//#include <algorithm>

#include "BattleItem.h"
#include "BattleUnit.h"
#include "SerializationHelper.h"

#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/Particle.h"

//#include "../Engine/Exception.h"
//#include "../Engine/Logger.h"
//#include "../Engine/RNG.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/// How many bytes various fields use in a serialized tile. See header.
Tile::SerializationKey Tile::serializationKey =
{
	4, // index
	2, // _mapDataSetID, four of these
	2, // _mapDataID, four of these
	1, // _fire
	1, // _smoke
	1, // _animOffset
	1, // one 8-bit bool field
	4 + (2 * 4) + (2 * 4) + 1 + 1 + 1 + 1 // total bytes to save one tile
};


/**
* cTor.
* @param pos - reference Position
*/
Tile::Tile(const Position& pos)
	:
		_smoke(0),
		_fire(0),
		_explosive(0),
		_explosiveType(0),
		_pos(pos),
		_unit(NULL),
		_animOffset(0),
		_markerColor(0),
		_visible(false),
		_preview(-1),
		_tuMarker(-1),
		_overlapsSMK(0),
		_overlapsINC(0),
		_danger(false)
{
	for (size_t
			i = 0;
			i != PARTS;
			++i)
	{
		_objects[i] =  0;
		_mapDataID[i] = -1;
		_mapDataSetID[i] = -1;
		_curFrame[i] =  0;
	}

	for (size_t
			i = 0;
			i != SECTS;
			++i)
	{
		_discovered[i] = false;
	}

	for (size_t
			layer = 0;
			layer != LIGHTLAYERS;
			++layer)
	{
		_light[layer] = 0;
//		_lastLight[layer] = -1;
	}
}

/**
 * dTor.
 */
Tile::~Tile()
{
	_inventory.clear();

	for (std::list<Particle*>::const_iterator
			i = _particles.begin();
			i != _particles.end();
			++i)
	{
		delete *i;
	}
	_particles.clear();
}

/**
 * Load the tile from a YAML node.
 * @param node - reference a YAML node
 */
void Tile::load(const YAML::Node& node)
{
	//_position = node["position"].as<Position>(_position);
	for (size_t
			i = 0;
			i != PARTS;
			++i)
	{
		_mapDataID[i]		= node["mapDataID"][i]		.as<int>(_mapDataID[i]);
		_mapDataSetID[i]	= node["mapDataSetID"][i]	.as<int>(_mapDataSetID[i]);
	}

	_fire		= node["fire"]		.as<int>(_fire);
	_smoke		= node["smoke"]		.as<int>(_smoke);
	_animOffset	= node["animOffset"].as<int>(_animOffset);

	if (node["discovered"])
	{
		for (size_t
				i = 0;
				i != SECTS;
				++i)
		{
			_discovered[i] = node["discovered"][i].as<bool>();
		}
	}

	if (node["openDoorWest"])
		_curFrame[1] = 7;

	if (node["openDoorNorth"])
		_curFrame[2] = 7;

//	if (_fire || _smoke)
//		_animationOffset = std::rand() %4;
}

/**
 * Load the tile from binary.
 * @param buffer - pointer to buffer
 * @param serKey - reference the serialization key
 */
void Tile::loadBinary(
		Uint8* buffer,
		Tile::SerializationKey& serKey)
{
	_mapDataID[0] = unserializeInt(&buffer, serKey._mapDataID);
	_mapDataID[1] = unserializeInt(&buffer, serKey._mapDataID);
	_mapDataID[2] = unserializeInt(&buffer, serKey._mapDataID);
	_mapDataID[3] = unserializeInt(&buffer, serKey._mapDataID);

	_mapDataSetID[0] = unserializeInt(&buffer, serKey._mapDataSetID);
	_mapDataSetID[1] = unserializeInt(&buffer, serKey._mapDataSetID);
	_mapDataSetID[2] = unserializeInt(&buffer, serKey._mapDataSetID);
	_mapDataSetID[3] = unserializeInt(&buffer, serKey._mapDataSetID);

	_smoke		= unserializeInt(&buffer, serKey._smoke);
	_fire		= unserializeInt(&buffer, serKey._fire);
	_animOffset	= unserializeInt(&buffer, serKey._animOffset);

	const int boolFields = unserializeInt(&buffer, serKey.boolFields);

	_discovered[0] = (boolFields & 0x01) ? true : false;
	_discovered[1] = (boolFields & 0x02) ? true : false;
	_discovered[2] = (boolFields & 0x04) ? true : false;

	_curFrame[1] = (boolFields & 0x08) ? 7 : 0;
	_curFrame[2] = (boolFields & 0x10) ? 7 : 0;

//	if (_fire || _smoke)
//		_animationOffset = std::rand() %4;
}

/**
 * Saves the tile to a YAML node.
 * @return, YAML node
 */
YAML::Node Tile::save() const
{
	YAML::Node node;

	node["position"] = _pos;

	for (size_t
			i = 0;
			i != PARTS;
			++i)
	{
		node["mapDataID"].push_back(_mapDataID[i]);
		node["mapDataSetID"].push_back(_mapDataSetID[i]);
	}

	if (_smoke != 0)		node["smoke"]		= _smoke;
	if (_fire != 0)			node["fire"]		= _fire;
	if (_animOffset != 0)	node["animOffset"]	= _animOffset;

	if (   _discovered[0] == true
		|| _discovered[1] == true
		|| _discovered[2] == true)
	{
		for (size_t
				i = 0;
				i != SECTS;
				++i)
		{
			node["discovered"].push_back(_discovered[i]);
		}
	}

	if (isUfoDoorOpen(1) == true)
		node["openDoorWest"] = true;
	if (isUfoDoorOpen(2) == true)
		node["openDoorNorth"] = true;

	return node;
}

/**
 * Saves the tile to binary.
 * @param buffer - pointer to pointer to buffer
 */
void Tile::saveBinary(Uint8** buffer) const
{
	serializeInt(buffer, serializationKey._mapDataID, _mapDataID[0]);
	serializeInt(buffer, serializationKey._mapDataID, _mapDataID[1]);
	serializeInt(buffer, serializationKey._mapDataID, _mapDataID[2]);
	serializeInt(buffer, serializationKey._mapDataID, _mapDataID[3]);

	serializeInt(buffer, serializationKey._mapDataSetID, _mapDataSetID[0]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapDataSetID[1]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapDataSetID[2]);
	serializeInt(buffer, serializationKey._mapDataSetID, _mapDataSetID[3]);

	serializeInt(buffer, serializationKey._smoke,		_smoke);
	serializeInt(buffer, serializationKey._fire,		_fire);
	serializeInt(buffer, serializationKey._animOffset,	_animOffset); // kL

	int boolFields = (_discovered[0] ? 0x01 : 0) + (_discovered[1] ? 0x02 : 0) + (_discovered[2] ? 0x04 : 0);

	boolFields |= isUfoDoorOpen(1) ? 0x08 : 0; // west
	boolFields |= isUfoDoorOpen(2) ? 0x10 : 0; // north

	serializeInt(
				buffer,
				serializationKey.boolFields,
				boolFields);
}

/**
 * Sets the MapData references of parts 0 to 3.
 * @param data		- pointer to MapData
 * @param dataID	- dataID
 * @param dataSetID	- dataSetID
 * @param part		- the part number
 */
void Tile::setMapData(
		MapData* const data,
		const int dataID,
		const int dataSetID,
		const int part)
{
	const size_t i = static_cast<size_t>(part);
	_objects[i] = data;
	_mapDataID[i] = dataID;
	_mapDataSetID[i] = dataSetID;
}

/**
 * Gets the MapData references of parts 0 to 3.
 * @param dataID	- pointer to dataID
 * @param dataSetID	- pointer to dataSetID
 * @param part		- the part number
 * @return, the object ID
 */
void Tile::getMapData(
		int* dataID,
		int* dataSetID,
		int part) const
{
	const size_t i = static_cast<size_t>(part);
	*dataID = _mapDataID[i];
	*dataSetID = _mapDataSetID[i];
}

/**
 * Gets whether this tile has no objects.
 * Note that we CAN have a unit (but no smoke or inventory) on this tile.
 * @param checkInv		- (default true)
 * @param checkSmoke	- (default true)
 * @param partsOnly - check only for parts & item (default false)
 * @return, true if there is nothing but air on this tile + smoke/fire if partsOnly TRUE
 */
bool Tile::isVoid(
		const bool checkInv,
		const bool checkSmoke) const
{
	bool ret = _objects[MapData::O_FLOOR] == NULL
			&& _objects[MapData::O_WESTWALL] == NULL
			&& _objects[MapData::O_NORTHWALL] == NULL
			&& _objects[MapData::O_OBJECT] == NULL;

	if (checkInv == true)
		ret = (ret && _inventory.empty() == true);

	if (checkSmoke == true)
		ret = (ret && _smoke == 0);

	return ret;
}
/*	return _objects[0] == NULL	// floor
		&& _objects[1] == NULL	// westwall
		&& _objects[2] == NULL	// northwall
		&& _objects[3] == NULL	// content
		&& _smoke == 0
		&& _inventory.empty() == true; */

/**
 * Gets the TU cost to move over a certain part of the tile.
 * @param part		- the part number
 * @param moveType	- the movement type
 * @return, TU cost
 */
int Tile::getTUCostTile(
		int part,
		MovementType moveType) const
{
	const size_t i = static_cast<size_t>(part);

	if (_objects[i] != NULL)
	{
		if (_objects[i]->isUFODoor() == true
			&& _curFrame[i] > 1)
		{
			return 0;
		}

		if (part == MapData::O_OBJECT
			&& _objects[i]->getBigWall() > Pathfinding::BIGWALL_NWSE)
		{
			return 0;
		}

		return _objects[i]->getTUCostObject(moveType);
	}

	return 0;
}

/**
 * Gets whether this tile has a floor or not. If no object defined as floor it has no floor.
 * @param tileBelow - the tile below this tile
 * @return, true if tile has no floor
 */
bool Tile::hasNoFloor(const Tile* const tileBelow) const
{
	if (tileBelow != NULL
		&& tileBelow->getTerrainLevel() == -24)
	{
		return false;
	}

	if (_objects[MapData::O_FLOOR] != NULL)
		return _objects[MapData::O_FLOOR]->isNoFloor();

	return true;
}

/**
 * Gets whether this tile has a bigwall.
 * @return, true if the content-object in this tile has a bigwall ( see Big Wall enum )
 */
bool Tile::isBigWall() const
{
	if (_objects[MapData::O_OBJECT] != NULL)
		return (_objects[MapData::O_OBJECT]->getBigWall() != Pathfinding::BIGWALL_NONE);

	return false;
}

/**
 * Gets the terrain level of this tile.
 * @note For graphical Y offsets etc. Terrain level starts and 0 and goes
 * upwards to -24; negative values are higher.
 * @return, the level in pixels
 */
int Tile::getTerrainLevel() const
{
	int level = 0;

	if (_objects[static_cast<size_t>(MapData::O_FLOOR)] != NULL)
		level = _objects[static_cast<size_t>(MapData::O_FLOOR)]->getTerrainLevel();

	if (_objects[static_cast<size_t>(MapData::O_OBJECT)] != NULL)
		level = std::min(
					level,
					_objects[static_cast<size_t>(MapData::O_OBJECT)]->getTerrainLevel());

	return level;
}

/**
 * Gets this tile's footstep sound.
 * @param tileBelow - pointer to the Tile below this tile
 * @return, sound ID
 *			0 - none
 *			1 - metal
 *			2 - wood/stone
 *			3 - dirt
 *			4 - mud
 *			5 - sand
 *			6 - snow (mars)
 */
int Tile::getFootstepSound(const Tile* const tileBelow) const
{
	int sound = -1;

	if (   _objects[static_cast<size_t>(MapData::O_OBJECT)] != NULL
		&& _objects[static_cast<size_t>(MapData::O_OBJECT)]->getBigWall() < Pathfinding::BIGWALL_NESW // ie. None or Block
		&& _objects[static_cast<size_t>(MapData::O_OBJECT)]->getFootstepSound() > 0) // > -1
	{
		sound = _objects[static_cast<size_t>(MapData::O_OBJECT)]->getFootstepSound();
	}
	else if (_objects[static_cast<size_t>(MapData::O_FLOOR)] != NULL)
		sound = _objects[static_cast<size_t>(MapData::O_FLOOR)]->getFootstepSound();
	else if (_objects[static_cast<size_t>(MapData::O_OBJECT)] == NULL
		&& tileBelow != NULL
		&& tileBelow->getTerrainLevel() == -24)
	{
		sound = tileBelow->getMapData(MapData::O_OBJECT)->getFootstepSound();
	}

	return sound;
}

/**
 * Open a door on this tile.
 * @param part		- a tile part
 * @param unit		- pointer to a BattleUnit (default NULL)
 * @param reserved	- see BA_* enum for TU reserves (default BA_NONE)
 * @return, -1 no door opened
 *			 0 normal door
 *			 1 ufo door
 *			 3 ufo door is still opening (animated)
 *			 4 not enough TUs
 */
int Tile::openDoor(
		const int part,
		const BattleUnit* const unit,
		const BattleActionType reserved)
{
	const size_t i = static_cast<size_t>(part);

	if (_objects[i] != NULL)
	{
		if (_objects[i]->isDoor() == true)
		{
			if (_unit != NULL
				&& _unit != unit
				&& _unit->getPosition() != getPosition())
			{
				return -1;
			}

			if (unit != NULL
				&& unit->getTimeUnits() < _objects[i]->getTUCostObject(unit->getMoveTypeUnit())
											+ unit->getActionTUs(
															reserved,
															unit->getMainHandWeapon(false)))
			{
				return 4;
			}

			setMapData(
					_objects[i]->getDataset()->getObjects()->at(_objects[i]->getAltMCD()),
					_objects[i]->getAltMCD(),
					_mapDataSetID[i],
					_objects[i]->getDataset()->getObjects()->at(_objects[i]->getAltMCD())->getObjectType());

			setMapData(
					NULL,
					-1,-1,
					part);

			return 0;
		}
		else if (_objects[i]->isUFODoor() == true)
		{
			if (_curFrame[i] == 0) // ufo door part 0 - door is closed
			{
				if (unit != NULL
					&& unit->getTimeUnits() < _objects[i]->getTUCostObject(unit->getMoveTypeUnit())
												+ unit->getActionTUs(
																reserved,
																unit->getMainHandWeapon(false)))
				{
					return 4;
				}

				_curFrame[i] = 1; // start opening door
				return 1;
			}
			else if (_curFrame[i] != 7) // ufo door != part 7 -> door is still opening
				return 3;
		}
	}

	return -1;
}

/**
 * Closes a ufoDoor on this Tile.
 * @return, 1 if a door got closed
 */
int Tile::closeUfoDoor()
{
	int ret = 0;

	for (size_t
			part = 0;
			part != PARTS;
			++part)
	{
		if (isUfoDoorOpen(static_cast<int>(part)) == true)
		{
			_curFrame[part] = 0;
			ret = 1;
		}
	}

	return ret;
}

/**
 * Sets this Tile's parts' cache flag.
 * @param vis	- true/false
 * @param part	- 0 westwall
 *				  1 northwall
 *				  2 content+floor
 */
void Tile::setDiscovered(
		bool vis,
		int part)
{
	const size_t i = static_cast<size_t>(part);

	if (_discovered[i] != vis)
	{
		_discovered[i] = vis;

		if (vis == true		// if content+floor is discovered
			&& part == 2)	// set west & north walls discovered too.
		{
			_discovered[0] =
			_discovered[1] = true;
		}

		if (_unit != NULL)			// if visibility of tile changes
			_unit->setCache(NULL);	// units on it change too.
	}
}

/**
 * Get the black fog of war state of this Tile.
 * @param part - 0 westwall
 *				 1 northwall
 *				 2 content+floor
 * @return, true if tilepart has been discovered
 */
bool Tile::isDiscovered(int part) const
{
	return _discovered[static_cast<size_t>(part)];
}

/**
 * Reset the light amount on the tile. This is done before a light level recalculation.
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::resetLight(size_t layer)
{
	_light[layer] = 0;
//	_lastLight[layer] = _light[layer];
}

/**
 * Add the light amount on the tile. Only add light if the current light is lower.
 * @param light - amount of light to add
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::addLight(
		int light,
		size_t layer)
{
	if (_light[layer] < light)
		_light[layer] = light;
}

/**
 * Gets the tile's shade amount 0-15. It returns the brightest of all light layers.
 * Shade level is the inverse of light level. So a maximum amount of light (15) returns shade level 0.
 * @return, shade
 */
int Tile::getShade() const
{
	int light = 0;

	for (size_t
			layer = 0;
			layer != LIGHTLAYERS;
			++layer)
	{
		if (_light[layer] > light)
			light = _light[layer];
	}

	return std::max(
				0,
				15 - light);
}

/**
 * Destroy a part on this tile.
 * First remove the old object then replace it with the destroyed one
 * because the object type of the old and new are not necessarily the same.
 * If the destroyed part is an explosive
 * set the tile's explosive value which will trigger a chained explosion.
 * @param part - this Tile's part for destruction
 * @return, true if an 'objective' was destroyed
 */
bool Tile::destroy(int part)
{
	const size_t i = static_cast<size_t>(part);

	bool _objective = false;

	if (_objects[i])
	{
		if (_objects[i]->isGravLift() == true
			|| _objects[i]->getArmor() == 255) // <- set to 255 in MCD for Truly Indestructability.
		{
			return false;
		}

		_objective = (_objects[i]->getSpecialType() == MUST_DESTROY);

		const MapData* const origPart = _objects[i];
		const int origMapDataSetID = _mapDataSetID[i];

		setMapData(
				 NULL,
				-1,-1,
				part);

		if (origPart->getDieMCD() != 0)
		{
			MapData* const dead = origPart->getDataset()->getObjects()->at(origPart->getDieMCD());
			setMapData(
					dead,
					origPart->getDieMCD(),
					origMapDataSetID,
					dead->getObjectType());
		}

		if (origPart->getExplosive() != 0)
			setExplosive(
					origPart->getExplosive(),
					origPart->getExplosiveType());
	}

	if (part == MapData::O_FLOOR // check if the floor on the lowest level is gone
		&& getPosition().z == 0
		&& _objects[MapData::O_FLOOR] == 0)
	{
		setMapData( // replace with scorched earth
				MapDataSet::getScorchedEarthTile(),
				1,0,
				MapData::O_FLOOR);
	}

	return _objective;
}

/**
 * Damages terrain (check against terrain-part armor/hitpoints/constitution)
 * @param part	- part of tile to check
 * @param power	- power of the damage
 * @return, true if an objective was destroyed
 */
bool Tile::damage(
		int part,
		int power)
{
	//Log(LOG_INFO) << "Tile::damage() vs part = " << part << ", hp = " << _objects[part]->getArmor();
	bool objectiveDestroyed = false;

	if (power >= _objects[static_cast<size_t>(part)]->getArmor())
		objectiveDestroyed = destroy(part);

	return objectiveDestroyed;
}

/**
 * Sets a "virtual" explosive on this tile. We mark a tile this way to
 * detonate it later, because the same tile can be visited multiple times
 * by "explosion rays". The explosive power that gets set on a tile is
 * that of the most powerful ray that passes through it -- see TileEngine::explode().
 * @param power		- how big the BOOM will be / how much tile-destruction
 * @param explType	- the type of this Tile's explosion (set in MCD; not the same as item damage types)
 * @param force		- forces value even if lower (default false)
 */
void Tile::setExplosive(
		int power,
		int explType,
		bool force)
{
	if (force == true
		|| _explosive < power)
	{
		_explosive = power;
		_explosiveType = explType;
	}
}

/**
 * Gets if & how powerfully this tile will explode.
 * Don't confuse this with a tile's inherent explosive power;
 * this value is set by explosions external to the tile itself.
 * @return, how big the BOOM will be / how much tile-destruction
 */
int Tile::getExplosive() const
{
	return _explosive;
}

/**
 * Gets explosive type of this tile.
 * @return, explosive type
 */
int Tile::getExplosiveType() const
{
	return _explosiveType;
}

/**
 * Flammability of a tile is the flammability of its most flammable part.
 * @return, the lower the value the higher the chance the tile catches fire - BSZAAST!!!
 */
int Tile::getFlammability() const
{
	int burn = 255; // not burnable. <- lower is better :)

	for (size_t
			i = 0;
			i != PARTS;
			++i)
	{
		if (   _objects[i] != NULL
			&& _objects[i]->getFlammable() < burn)
		{
			burn = _objects[i]->getFlammable();
		}
	}

	return convertBurnToPCT(burn);
}

/**
 * Gets the flammability of a tile-part.
 * @note I now decree that this returns the inverse of 0..255 as a percentage!
 * @return, the lower the value the higher the chance the tile-part catches fire - BSZAAST!!!
 */
int Tile::getFlammability(int part) const
{
	return convertBurnToPCT(_objects[static_cast<size_t>(part)]->getFlammable());
//	return _objects[static_cast<size_t>(part)]->getFlammable();
}

/**
 * Converts obscure inverse MCD notation to understandable percentages.
 * @note Chance can be increased by the power of the spark.
 * @param burn - flammability from an MCD file (see MapData)
 * @return, basic percent chance that this stuff burns
 */
int Tile::convertBurnToPCT(int burn) const // private.
{
	if (burn == 255)
		return 0;

	burn = 255 - burn;
	burn = std::max(
				1,
				std::min(
					100,
					static_cast<int>(std::ceil(
					static_cast<double>(burn) / 255. * 100.))));

	return burn;
}

/**
 * Fuel of a tile is the highest fuel of its parts/objects.
 * @note This is NOT the sum of the fuel of the objects!
 * @return, how many turns to burn
 */
int Tile::getFuel() const
{
	int fuel = 0;

	for (size_t
			i = 0;
			i != PARTS;
			++i)
	{
		if (   _objects[i] != NULL
			&& _objects[i]->getFuel() > fuel)
		{
			fuel = _objects[i]->getFuel();
		}
	}

	return fuel;
}

/**
 * Gets the fuel of a tile-part.
 * @return, how many turns to burn
 */
int Tile::getFuel(int part) const
{
	return _objects[static_cast<size_t>(part)]->getFuel();
}

/**
 * Tries to start fire on this Tile.
 * @note If true it will add its fuel as turns to burn.
 * @note Called only by floor-burning Silacoids and fire spreading @ turnovers.
 * @note Also used by TileEngine::detonate() after HE explosions.
 * @param power - rough chance to get things going
 * @return, true if tile catches fire
 */
bool Tile::ignite(int power)
{
	if (power != 0
		&& canSmoke() == true)
	{
		const int fuel = getFuel();
		if (fuel != 0)
		{
			const int burn = getFlammability();
			if (burn != 0)
			{
				power = (power * 2) + (burn + 1) / 2;
				if (RNG::percent(power) == true)
				{
					addFire(fuel + 1);
					addSmoke((burn + 15) / 16);

					return true;
				}
			}
		}
	}

	return false;
}

/**
 * Adds fire to this Tile.
 * @param turns - turns to burn
 */
void Tile::addFire(int turns)
{
	if (turns != 0
		&& canSmoke() == true)
	{
		if (_smoke == 0
			&& _fire == 0)
		{
			_animOffset = RNG::generate(0,3);
		}

		_fire += turns;

		if (_fire > 12)
			_fire = 12;
	}
}

/**
 * Reduces the number of turns this Tile will burn.
 */
void Tile::decreaseFire()
{
	--_fire;

	if (_fire < 1)
	{
		_fire = 0;

		if (_smoke == 0)
			_animOffset = 0;
	}
}

/**
 * Gets the number of turns left for this tile to be on fire.
 * @return, number of turns left for this tile to burn
 */
int Tile::getFire() const
{
	return _fire;
}

/**
 * Adds smoke to this Tile.
 * @param turns - turns to smoke
 */
void Tile::addSmoke(int turns)
{
	if (turns != 0
		&& canSmoke() == true)
	{
		if (_smoke == 0
			&& _fire == 0)
		{
			_animOffset = RNG::generate(0,3);
		}

		_smoke += turns;

		if (_smoke > 17)
			_smoke = 17;
	}
}

/**
 * Reduces the number of turns this Tile will smoke for.
 */
void Tile::decreaseSmoke()
{
	_smoke -= (RNG::generate(1, _smoke) + 2) / 2;

	if (_smoke < 1)
	{
		_smoke = 0;

		if (_fire == 0)
			_animOffset = 0;
	}
}

/**
 * Gets the number of turns left for this tile to smoke.
 * @return, turns left
 */
int Tile::getSmoke() const
{
	return _smoke;
}

/**
 * Gets the smoke overlap value of this Tile.
 * @return, smoke overlaps
 */
/*int Tile::getOverlapsSK() const
{
	return _overlapsSMK;
} */

/**
 * Gets the fire overlap value of this Tile.
 * @return, fire overlaps
 */
/*int Tile::getOverlapsIN() const
{
	return _overlapsINC;
} */

/**
 * New turn preparations. Average out any smoke added by the number of overlaps.
 */
/* void Tile::resolveOverlaps()
{
	Log(LOG_INFO) << "pos " << _pos << " s = " << _smoke << " | " << _overlapsSMK;
	if (_smoke != 0
		&& _overlapsSMK != 0)
	{
		_smoke = std::max(
						0,
						std::min(
							17,
							(_smoke + _overlapsSMK - 1) / _overlapsSMK));
	}
	Log(LOG_INFO) << ". s = " << _smoke;

	Log(LOG_INFO) << "pos " << _pos << " f = " << _smoke << " | " << _overlapsINC;
	if (_fire != 0
		&& _overlapsINC != 0)
	{
		_fire = std::max(
						0,
						std::min(
							12,
							(_fire + _overlapsINC - 1) / _overlapsINC));
	}
	Log(LOG_INFO) << ". f = " << _fire;

	_overlapsSMK =
	_overlapsINC = 0;
	_danger = false;
} */

/**
 * Gets if this Tile will accept '_smoke' or '_fire' value.
 * @note diag bigWalls take no smoke/fire. 'Cause I don't want it showing on both sides.
 * And I don't want it to creep through diagonal UFO hulls ....
 * @return, true if smoke/fire possible
 */
bool Tile::canSmoke() const // private
{
	return _objects[MapData::O_OBJECT] == NULL
		   || (    _objects[MapData::O_OBJECT]->getBigWall() != Pathfinding::BIGWALL_NESW
				&& _objects[MapData::O_OBJECT]->getBigWall() != Pathfinding::BIGWALL_NWSE
				&& (_objects[MapData::O_OBJECT]->getBigWall() != Pathfinding::BIGWALL_BLOCK
					|| _objects[MapData::O_OBJECT]->blockSmoke() == false)); // <- TODO: remove bias vs. Smoke; ie. include Fire later
}

/**
 * Ends this tile's turn. Units catch on fire.
 * Separated from resolveOverlaps() above so that units take damage before
 * smoke/fire spreads to them; this is so that units would have to end their
 * turn on a tile before smoke/fire damages them. That is they get a chance
 * to get off the tile during their turn.
 * @param battleSave - pointer to the current SavedBattleGame (default NULL Vs. units)
 */
void Tile::hitStuff(SavedBattleGame* const battleSave)
{
	//Log(LOG_INFO) << "Tile::hitStuff() " << _pos;
	//if (_unit) Log(LOG_INFO) << ". unitID " << _unit->getId();
	int
		pSmoke,
		pFire;

	if (_smoke != 0)
		pSmoke = (_smoke + 3) / 4 + 1;
	else
		pSmoke = 0;

	if (_fire != 0)
		pFire = _fire + RNG::generate(3,9);
	else
		pFire = 0;
	//Log(LOG_INFO) << ". pSmoke = " << pSmoke;
	//Log(LOG_INFO) << ". pFire " << pFire;

	float vuln;

	if (battleSave == NULL)	// damage standing units (at end of faction's turn-phases). Notice this hits only the primary quadrant!
//		&& _unit != NULL)	// safety. call from BattlescapeGame::endTurnPhase() checks only Tiles w/ units, but that could change ....
	{
		//Log(LOG_INFO) << ". . hit Unit";
		if (pSmoke != 0
			&& _unit->isFearable() == true)
		{
			vuln = _unit->getArmor()->getDamageModifier(DT_SMOKE);
			if (vuln > 0.f) // try to knock _unit out.
				_unit->damage(
							Position(0,0,0),
							static_cast<int>(Round(static_cast<float>(pSmoke) * vuln)),
							DT_SMOKE, // -> DT_STUN
							true);
		}

		if (pFire != 0)
		{
			vuln = _unit->getArmor()->getDamageModifier(DT_IN);
			if (vuln > 0.f)
			{
				_unit->damage(
							Position(0,0,0),
							static_cast<int>(Round(static_cast<float>(pFire) * vuln)),
							DT_IN,
							true);

				if (RNG::percent(static_cast<int>(Round(40.f * vuln))) == true) // try to set _unit on fire. Do damage from fire here, too.
				{
					const int dur = RNG::generate(
												1,
												static_cast<int>(Round(5.f * vuln)));
					if (dur > _unit->getFireOnUnit())
						_unit->setFireOnUnit(dur);
				}
			}
		}
	}
	else //if (battleSave != NULL) // try to destroy items & kill unconscious units (only for end of full-turns)
	{
		//Log(LOG_INFO) << ". . hit Inventory's ground-items";
		BattleUnit* unit;

		if (pSmoke != 0)
		{
			for (std::vector<BattleItem*>::const_iterator // handle unconscious units on this Tile vs. DT_SMOKE
					i = _inventory.begin();
					i != _inventory.end();
					++i)
			{
				unit = (*i)->getUnit();

				if (unit != NULL
					&& unit->getStatus() == STATUS_UNCONSCIOUS
					&& unit->getTakenExpl() == false)
				{
					//Log(LOG_INFO) << ". . unConsc unit (smoke) " << unit->getId();
					unit->setTakenExpl();

					vuln = unit->getArmor()->getDamageModifier(DT_SMOKE);
					if (vuln > 0.f)
						unit->damage(
								Position(0,0,0),
								static_cast<int>(Round(static_cast<float>(pSmoke) * vuln)),
								DT_SMOKE,
								true);
				}
			}
		}

		if (pFire != 0)
		{
			bool done = false;
			while (done == false) // handle items including unconscious or dead units on this Tile vs. DT_IN
			{
				if (_inventory.empty() == true)
					break;


				for (std::vector<BattleItem*>::const_iterator
						i = _inventory.begin();
						i != _inventory.end();
						)
				{
					unit = (*i)->getUnit();

					if (unit != NULL
						&& unit->getStatus() == STATUS_UNCONSCIOUS
						&& unit->getTakenFire() == false)
					{
						//Log(LOG_INFO) << ". . unConsc unit (fire) " << unit->getId();
						unit->setTakenFire();

						vuln = unit->getArmor()->getDamageModifier(DT_IN);
						if (vuln > 0.f)
						{
							unit->damage(
									Position(0,0,0),
									static_cast<int>(static_cast<float>(pFire) * vuln),
									DT_IN,
									true);

							if (unit->getHealth() == 0)
							{
								//Log(LOG_INFO) << ". . . dead";
								unit->instaKill();
								unit->killedBy(unit->getFaction()); // killed by self ....
								Log(LOG_INFO) << "Tile::hitStuff() " << unit->getId() << " killedBy = " << (int)unit->getFaction();

								// This bit should be gtg on return to BattlescapeGame::endTurnPhase().
/*								if (Options::battleNotifyDeath == true // send Death notice.
									&& unit->getGeoscapeSoldier() != NULL)
								{
									Game* game = battleSave->getBattleState()->getGame();
									game->pushState(new InfoboxOKState(game->getLanguage()->getString( // "has been killed with Fire ..."
																								"STR_HAS_BEEN_KILLED",
																								unit->getGender())
																							.arg(unit->getName(game->getLanguage()))));
								} */
							}
						}

						++i;
						done = (i == _inventory.end());
					}
					else if (pFire > (*i)->getRules()->getArmor() // no modifier when destroying items, not even corpse in bodyarmor.
						&& (unit == NULL
							|| unit->getStatus() == STATUS_DEAD))
					{
						//Log(LOG_INFO) << ". . destroy item";
						battleSave->removeItem(*i);	// This should not kill *and* remove a unit's corpse on the same
						break;						// tilePhase; but who knows, I haven't traced it comprehensively.
					}
					else
					{
						//Log(LOG_INFO) << ". . iterate";
						++i;
						done = (i == _inventory.end());
					}
				}
			}
		}
	}
	//Log(LOG_INFO) << "Tile::hitStuff() EXIT";
}

/**
 * Animate the tile - this means to advance the current frame for every part.
 * Ufo doors are a bit special; they animate only when triggered.
 * When ufo doors are on frame 0(closed) or frame 7(open) they are not animated further.
 */
void Tile::animate()
{
	int nextFrame;

	for (size_t
			i = 0;
			i != PARTS;
			++i)
	{
		if (_objects[i] != NULL)
		{
			const int isPsycho = _objects[i]->isPsychedelic();
			if (isPsycho == 0)
			{
				if (_objects[i]->isUFODoor() == true // ufo door is currently static
					&& (   _curFrame[i] == 0
						|| _curFrame[i] == 7))
				{
					continue;
				}

				nextFrame = _curFrame[i] + 1;

				if (   _objects[i]->isUFODoor() == true // special handling for Avenger & Lightning doors
					&& _objects[i]->getSpecialType() == START_POINT
					&& nextFrame == 3)
				{
					nextFrame = 7;
				}

				if (nextFrame == 8)
					nextFrame = 0;

				_curFrame[i] = nextFrame;
			}
			else
			{
				if (isPsycho == 1)
				{
					if (std::rand() % 3 != 0)
						_curFrame[i] = SDL_GetTicks() % 8;
				}
				else if (SDL_GetTicks() % 3 == 0) // isPsycho==2
					_curFrame[i] = std::rand() % 8;
			}
		}
	}
//	getMapData(MapData::O_WESTWALL)->getDataset()->getName() == "U_PODS"
//	getMapData(MapData::O_WESTWALL)->getSprite(0) == 61


	for (std::list<Particle*>::const_iterator
			i = _particles.begin();
			i != _particles.end();
			)
	{
		if ((*i)->animate() == false)
		{
			delete *i;
			i = _particles.erase(i);
		}
		else
			++i;
	}
}

/**
 * Get the number of frames the fire or smoke animation is off-sync.
 * To void fire and smoke animations of different tiles moving nice in sync - it looks fake.
 * @return, offset
 */
int Tile::getAnimationOffset() const
{
	return _animOffset;
}

/**
 * Get the sprite of a certain part of the tile.
 * @param part - this Tile's part to get a sprite for
 * @return, pointer to the sprite
 */
Surface* Tile::getSprite(int part) const
{
	const size_t i = static_cast<size_t>(part);

	const MapData* const data = _objects[i];
	if (data == NULL)
		return NULL;

	return data->getDataset()->getSurfaceset()->getFrame(data->getSprite(_curFrame[i]));
}

/**
 * Sets a unit on this tile.
 * @param unit		- pointer to a BattleUnit
 * @param tileBelow	- pointer to the Tile below this Tile (default NULL)
 */
void Tile::setUnit(
		BattleUnit* const unit,
		const Tile* const tileBelow)
{
	if (unit != NULL)
		unit->setTile(
					this,
					tileBelow);

	_unit = unit;
}

/**
 * Add an item on the tile.
 * @param item		- pointer to a BattleItem
 * @param ground	- pointer to RuleInventory ground-slot
 */
void Tile::addItem(
		BattleItem* const item,
		RuleInventory* const ground)
{
	item->setSlot(ground);
	_inventory.push_back(item);

	item->setTile(this);
}

/**
 * Remove an item from the tile.
 * @param item - pointer to a BattleItem
 */
void Tile::removeItem(BattleItem* const item)
{
	//Log(LOG_INFO) << "Tile::removeItem() id " << item->getId();
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if (*i == item)
		{
			_inventory.erase(i);
			break;
		}
	}

	item->setTile(NULL);
}

/**
 * Get the topmost item sprite to draw on the battlescape.
 * @param primed - pointer to whether a proxy-grenade is primed or not
 * @return, sprite ID in floorob (-1 none)
 */
int Tile::getTopItemSprite(bool* ptrPrimed) const
{
	if (_inventory.empty() == true)
		return -1;

	BattleItem
		* proxGrenade = NULL,
		* fuseGrenade = NULL,
		* solCorpse = NULL;

	if (ptrPrimed != NULL)
		*ptrPrimed = false;

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_PROXIMITYGRENADE
			&& (*i)->getFuseTimer() > -1)
		{
			proxGrenade = *i;

			if (ptrPrimed != NULL)
				*ptrPrimed = true;
		}

		if ((*i)->getRules()->getBattleType() == BT_GRENADE
			&& (*i)->getFuseTimer() > -1)
		{
			fuseGrenade = *i;
		}

		if ((*i)->getUnit() != NULL
			&& (*i)->getUnit()->getGeoscapeSoldier() != NULL)
		{
			solCorpse = *i;
		}
	}

	if (proxGrenade != NULL)
		return proxGrenade->getRules()->getFloorSprite();

	if (fuseGrenade != NULL)
		return fuseGrenade->getRules()->getFloorSprite();

	if (solCorpse != NULL)
		return solCorpse->getRules()->getFloorSprite();


	int
		weight = -1,
		sprite = -1;

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_CORPSE)
			return (*i)->getRules()->getFloorSprite();

		if ((*i)->getRules()->getWeight() > weight)
		{
			weight = (*i)->getRules()->getWeight();
			sprite = (*i)->getRules()->getFloorSprite();
		}
	}

	return sprite;
}

/**
 * Gets if this Tile has an unconscious xCom unit in its inventory.
 * @return,	0 - no living Soldier
 *			1 - stunned Soldier
 *			2 - stunned and wounded Soldier
 */
int Tile::getHasUnconsciousSoldier() const
{
	int ret = 0;

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		const BattleUnit* const bu = (*i)->getUnit();

		if (   bu != NULL
			&& bu->getOriginalFaction() == FACTION_PLAYER
			&& bu->getStatus() == STATUS_UNCONSCIOUS)
		{
			if (bu->getFatalWounds() == 0)
				ret = 1;
			else
				return 2;
		}
	}

	return ret;
}

/**
 * Gets the inventory on this tile.
 * @return, pointer to a vector of pointers to BattleItems
 */
std::vector<BattleItem*>* Tile::getInventory()
{
	return &_inventory;
}

/**
 * Sets the marker color on this tile.
 * @param color - color of marker
 */
void Tile::setMarkerColor(int color)
{
	_markerColor = color;
}

/**
 * Gets the marker color on this tile.
 * @return, color of marker
 */
int Tile::getMarkerColor() const
{
	return _markerColor;
}

/**
 * Sets the tile visible flag.
 * @param vis - true if visible (default true)
 */
void Tile::setTileVisible(bool vis)
{
	_visible = vis;
}

/**
 * Gets the tile visible flag.
 * @return, true if visible
 */
bool Tile::getTileVisible() const
{
	return _visible;
}

/**
 * Sets a direction used for path previewing.
 * @param dir - a direction
 */
void Tile::setPreview(int dir)
{
	_preview = dir;
}

/**
 * Gets the preview-direction stored by pathfinding.
 * @return, preview direction
 */
int Tile::getPreview() const
{
	return _preview;
}

/**
 * Sets a number to be displayed by pathfinding preview.
 * @param tu - # of TUs left if/when this tile is reached
 */
void Tile::setTUMarker(int tu)
{
	_tuMarker = tu;
}

/**
 * Gets the number to be displayed for pathfinding preview.
 * @return, # of TUs left if/when this tile is reached
 */
int Tile::getTUMarker() const
{
	return _tuMarker;
}

/**
 * Sets the danger flag on this Tile.
 * @param danger - true if the AI regards the tile as dangerous (default true)
 */
void Tile::setDangerous(bool danger)
{
	_danger = danger;
}

/**
 * Gets the danger flag on this tile.
 * @return, true if the tile is considered dangerous to aLiens
 */
bool Tile::getDangerous() const
{
	return _danger;
}

/**
 * Adds a particle to this tile's internal storage buffer.
 * @param particle - pointer to a particle to add
 */
void Tile::addParticle(Particle* particle)
{
	_particles.push_back(particle);
}

/**
 * Gets a pointer to this tile's particle array.
 * @return, pointer to the internal array of particles
 */
std::list<Particle*>* Tile::getParticleCloud()
{
	return &_particles;
}

}
