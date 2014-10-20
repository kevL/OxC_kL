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

#include "Tile.h"

#include <algorithm>

#include "BattleItem.h"
#include "BattleUnit.h"
#include "SerializationHelper.h"

#include "../Battlescape/Particle.h"

#include "../Engine/Exception.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

/// How many bytes various fields use in a serialized tile. See header.
Tile::SerializationKey Tile::serializationKey =
{
	4,	// index
	2,	// _mapDataSetID, four of these
	2,	// _mapDataID, four of these
	1,	// _fire
	1,	// _smoke
	1,	// _animOffset
	1,	// one 8-bit bool field
//kL	4 + (2 * 4) + (2 * 4) + 1 + 1 + 1	// total bytes to save one tile
	4 + (2 * 4) + (2 * 4) + 1 + 1 + 1 + 1	// kL
};


/**
* constructor
* @param pos Position.
*/
Tile::Tile(const Position& pos)
	:
		_smoke(0),
		_fire(0),
		_explosive(0),
		_pos(pos),
		_unit(NULL),
		_animOffset(0),
		_markerColor(0),
		_visible(false),
		_preview(-1),
		_tuMarker(-1),
		_overlaps(0),
		_danger(false),
		_inventory() // kL
{
	for (int
			i = 0;
			i < 4;
			++i)
	{
		_objects[i]			=  0;
		_mapDataID[i]		= -1;
		_mapDataSetID[i]	= -1;
		_curFrame[i]		=  0;
	}

	for (int
			layer = 0;
			layer < LIGHTLAYERS;
			layer++)
	{
		_light[layer]		=  0;
		_lastLight[layer]	= -1;
	}

	for (int
			i = 0;
			i < 3;
			++i)
	{
		_discovered[i] = false;
	}
}

/**
 * dTor.
 */
Tile::~Tile()
{
	_inventory.clear();

	for (std::list<Particle*>::iterator
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
 * @param node YAML node.
 */
void Tile::load(const YAML::Node& node)
{
	//_position = node["position"].as<Position>(_position);
	for (int
			i = 0;
			i < 4;
			i++)
	{
		_mapDataID[i]		= node["mapDataID"][i].as<int>(_mapDataID[i]);
		_mapDataSetID[i]	= node["mapDataSetID"][i].as<int>(_mapDataSetID[i]);
	}

	_fire		= node["fire"].as<int>(_fire);
	_smoke		= node["smoke"].as<int>(_smoke);
	_animOffset	= node["animOffset"].as<int>(_animOffset); // kL

	for (int
			i = 0;
			i < 3;
			i++)
	{
		_discovered[i] = node["discovered"][i].as<bool>();
	}

	if (node["openDoorWest"])
		_curFrame[1] = 7;

	if (node["openDoorNorth"])
		_curFrame[2] = 7;
}

/**
 * Load the tile from binary.
 * @param buffer - pointer to buffer
 * @param serKey - serialization key
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
	_animOffset	= unserializeInt(&buffer, serKey._animOffset); // kL

//kL	Uint8 boolFields = unserializeInt(&buffer, serKey.boolFields);
	Uint8 boolFields = static_cast<Uint8>(unserializeInt(&buffer, serKey.boolFields)); // kL

	_discovered[0] = (boolFields & 1)? true: false;
	_discovered[1] = (boolFields & 2)? true: false;
	_discovered[2] = (boolFields & 4)? true: false;

	_curFrame[1] = (boolFields & 8)? 7: 0;
	_curFrame[2] = (boolFields & 0x10)? 7: 0;
}

/**
 * Saves the tile to a YAML node.
 * @return YAML node.
 */
YAML::Node Tile::save() const
{
	YAML::Node node;

	node["position"] = _pos;

	for (int
			i = 0;
			i < 4;
			i++)
	{
		node["mapDataID"].push_back(_mapDataID[i]);
		node["mapDataSetID"].push_back(_mapDataSetID[i]);
	}

	if (_smoke) node["smoke"]			= _smoke;
	if (_fire) node["fire"]				= _fire;
	if (_animOffset) node["animOffset"]	= _animOffset; // kL

	if (_discovered[0]
		|| _discovered[1]
		|| _discovered[2])
	{
		for (int
				i = 0;
				i < 3;
				i++)
		{
			node["discovered"].push_back(_discovered[i]);
		}
	}

	if (isUfoDoorOpen(1))
		node["openDoorWest"] = true;
	if (isUfoDoorOpen(2))
		node["openDoorNorth"] = true;

	return node;
}

/**
 * Saves the tile to binary.
 * @param buffer pointer to buffer.
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

	serializeInt(buffer, serializationKey._smoke, _smoke);
	serializeInt(buffer, serializationKey._fire, _fire);
	serializeInt(buffer, serializationKey._animOffset, _animOffset); // kL

	Uint8 boolFields = (_discovered[0]? 1: 0) + (_discovered[1]? 2: 0) + (_discovered[2]? 4: 0);

	boolFields |= isUfoDoorOpen(1)? 8: 0; // west
	boolFields |= isUfoDoorOpen(2)? 0x10: 0; // north?

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
		MapData* data,
		int dataID,
		int dataSetID,
		int part)
{
	_objects[part] = data;
	_mapDataID[part] = dataID;
	_mapDataSetID[part] = dataSetID;
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
	*dataID = _mapDataID[part];
	*dataSetID = _mapDataSetID[part];
}

/**
 * Gets whether this tile has no objects.
 * Note that we CAN have a unit (but not smoke) on this tile.
 * @return, true if there is nothing but air on this tile
 */
bool Tile::isVoid() const
{
	return _objects[0] == NULL		// floor
			&& _objects[1] == NULL	// westwall
			&& _objects[2] == NULL	// northwall
			&& _objects[3] == NULL	// content
			&& _smoke == 0
			&& _inventory.empty();
}

/**
 * Gets the TU cost to move over a certain part of the tile.
 * @param part			- the part number
 * @param movementType	- the movement type
 * @return, TU cost
 */
int Tile::getTUCost(
		int part,
		MovementType movementType) const
{
	//Log(LOG_INFO) << "Tile::getTUCost() part = " << part << " MT = " << (int)movementType;
	if (_objects[part])
	{
		if (_objects[part]->isUFODoor()
			&& _curFrame[part] > 1)
		{
			return 0;
		}

		if (part == MapData::O_OBJECT
			&& _objects[part]->getBigWall() > 3)
		{
			return 0;
		}

		//int ret = _objects[part]->getTUCost(movementType);
		//Log(LOG_INFO) << ". ret = " << ret;
		//return ret;
		return _objects[part]->getTUCost(movementType);
	}

	return 0;
}

/**
 * Gets whether this tile has a floor or not. If no object defined as floor, it has no floor.
 * @param tileBelow - the tile below this tile
 * @return, true if tile has no floor
 */
bool Tile::hasNoFloor(Tile* tileBelow) const
{
	if (tileBelow != NULL
		&& tileBelow->getTerrainLevel() == -24)
	{
		return false;
	}

	if (_objects[MapData::O_FLOOR])
		return _objects[MapData::O_FLOOR]->isNoFloor();

	return true;
}

/**
 * Gets whether this tile has a bigwall.
 * @return, true if the content-object in this tile has a bigwall ( see Big Wall enum )
 */
bool Tile::isBigWall() const
{
	if (_objects[MapData::O_OBJECT])
		return (_objects[MapData::O_OBJECT]->getBigWall() != 0);

	return false;
}

/**
 * Gets the terrain level of this tile. For graphical Y offsets, etc.
 * Notice that terrain level starts and 0 and goes upwards to -24.
 * @return, the level in pixels (so negative values are higher)
 */
int Tile::getTerrainLevel() const
{
	int level = 0;

	if (_objects[MapData::O_FLOOR])
		level = _objects[MapData::O_FLOOR]->getTerrainLevel();

	if (_objects[MapData::O_OBJECT]) // kL_note: wait, aren't these negative=high
		level = std::min(
						level,
						_objects[MapData::O_OBJECT]->getTerrainLevel());

	return level;
}

/**
 * Gets a tile's footstep sound.
 * @param tileBelow - pointer to the tile below this tile
 * @return, sound ID
 */
int Tile::getFootstepSound(Tile* tileBelow) const
{
	int sound = 0;

	if (_objects[MapData::O_FLOOR])
		sound = _objects[MapData::O_FLOOR]->getFootstepSound();

	if (_objects[MapData::O_OBJECT]
		&& _objects[MapData::O_OBJECT]->getBigWall() == 0
		&& _objects[MapData::O_OBJECT]->getFootstepSound() > -1)
	{
		sound = _objects[MapData::O_OBJECT]->getFootstepSound();
	}

	if (_objects[MapData::O_FLOOR] == NULL
		&& _objects[MapData::O_OBJECT] == NULL
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
 * @param unit		- pointer to a unit
 * @param reserve	- see BA_* enum for TU reserves
 * @return, Value: -1 no door opened
 *					0 normal door
 *					1 ufo door
 *					3 ufo door is still opening (animated)
 *					4 not enough TUs
 */
int Tile::openDoor(
		int part,
		BattleUnit* unit,
		BattleActionType reserve)
{
	// kL_note: Am parsing this tighter with if/else's
	// May lead to ambiguity, esp. if isDoor() & isUFODoor() are not exclusive;
	// or if setMapData() is doing something ...... -> to UFO doors.

	if (!_objects[part])
		return -1;
	else if (_objects[part]->isDoor())
	{
		if (_unit
			&& _unit != unit
			&& _unit->getPosition() != getPosition())
		{
			return -1;
		}

		if (unit
			&& unit->getTimeUnits() < _objects[part]->getTUCost(unit->getMovementType())
										+ unit->getActionTUs(
														reserve,
														unit->getMainHandWeapon(false)))
		{
			return 4;
		}

		setMapData(
				_objects[part]->getDataset()->getObjects()->at(_objects[part]->getAltMCD()),
				_objects[part]->getAltMCD(),
				_mapDataSetID[part],
				_objects[part]->getDataset()->getObjects()->at(_objects[part]->getAltMCD())->getObjectType());
		setMapData(0,-1,-1, part);

		return 0;
	}
	else if (_objects[part]->isUFODoor())
	{
		if (_curFrame[part] == 0) // ufo door part 0 - door is closed
		{
			if (unit
				&& unit->getTimeUnits() < _objects[part]->getTUCost(unit->getMovementType())
											+ unit->getActionTUs(
															reserve,
															unit->getMainHandWeapon(false)))
			{
				return 4;
			}

			_curFrame[part] = 1; // start opening door

			return 1;
		}
		else if (_curFrame[part] != 7) // ufo door != part 7 -> door is still opening
			return 3;
	}

	return -1;
}

/**
 *
 */
int Tile::closeUfoDoor()
{
	int ret = 0;

	for (int
			part = 0;
			part < 4;
			part++)
	{
		if (isUfoDoorOpen(part))
		{
			_curFrame[part] = 0;
			ret = 1;
		}
	}

	return ret;
}

/**
 * Sets the tile's parts' cache flag.
 * @param flag - true/false
 * @param part - 0 westwall
 *				 1 northwall
 *				 2 content+floor
 */
void Tile::setDiscovered(
		bool flag,
		int part)
{
//	if (this->isVoid()) return; // kL

	if (_discovered[part] != flag)
	{
		_discovered[part] = flag;

		if (part == 2 && flag)
		{
			_discovered[0] = true;
			_discovered[1] = true;
		}

		// if light/visibility on tile changes, units and objects on it change light too
		if (_unit != NULL)
			_unit->setCache(NULL);
	}
}

/**
 * Get the black fog of war state of this tile.
 * @param part - 0 westwall
 *				 1 northwall
 *				 2 content+floor
 * @return, true if tilepart has been discovered
 */
bool Tile::isDiscovered(int part) const
{
	return _discovered[part];
}

/**
 * Reset the light amount on the tile. This is done before a light level recalculation.
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::resetLight(int layer)
{
	_light[layer] = 0;
	_lastLight[layer] = _light[layer];
}

/**
 * Add the light amount on the tile. Only add light if the current light is lower.
 * @param light - amount of light to add
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::addLight(
		int light,
		int layer)
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

	for (int
			layer = 0;
			layer < LIGHTLAYERS;
			layer++)
	{
		if (_light[layer] > light)
			light = _light[layer];
	}

	return std::max(
				0,
				15 - light);
}

/**
 * Destroy a part on this tile. We first remove the old object, then replace it with the destroyed one.
 * This is because the object type of the old and new one are not necessarily the same.
 * If the destroyed part is an explosive, set the tile's explosive value, which will trigger a chained explosion.
 * @param part
 * @return bool, Return true objective was destroyed
 */
bool Tile::destroy(int part)
{
	//Log(LOG_INFO) << "Tile::destroy()";
	bool _objective = false;

	if (_objects[part])
	{
		//Log(LOG_INFO) << ". objects[part] is Valid";
		if (_objects[part]->isGravLift()
			|| _objects[part]->getArmor() == 255) // kL
		{
			return false;
		}

		//Log(LOG_INFO) << ". . . tile part DESTROYED";
		_objective = _objects[part]->getSpecialType() == MUST_DESTROY;
		MapData* originalPart = _objects[part];

		int originalMapDataSetID = _mapDataSetID[part];
		setMapData(
				 0,
				-1,
				-1,
				part);

		//Log(LOG_INFO) << ". preDie";
		if (originalPart->getDieMCD())
		{
			//Log(LOG_INFO) << ". . getDieMCD()";
			MapData* dead = originalPart->getDataset()->getObjects()->at(originalPart->getDieMCD());
			setMapData(
					dead,
					originalPart->getDieMCD(),
					originalMapDataSetID,
					dead->getObjectType());
		}

		//Log(LOG_INFO) << ". preExplode";
		if (originalPart->getExplosive())
		{
			//Log(LOG_INFO) << ". . getExplosive()";
			setExplosive(originalPart->getExplosive());
		}
	}

	//Log(LOG_INFO) << ". preScorchEarth";
	// check if the floor on the lowest level is gone
	if (part == MapData::O_FLOOR
		&& getPosition().z == 0
		&& _objects[MapData::O_FLOOR] == 0)
	{
		//Log(LOG_INFO) << ". . getScorchedEarthTile()";

		// replace with scorched earth
		setMapData(
				MapDataSet::getScorchedEarthTile(),
				1,
				0,
				MapData::O_FLOOR);
	}

	//Log(LOG_INFO) << ". ret = " << _objective;
	return _objective;
}

/**
 * Damages terrain ( check against terrain-part armor/hitpoints/constitution )
 * @param part	- part of tile to check
 * @param power	- power of the damage
 * @return, true if an objective was destroyed
 */
bool Tile::damage(
		int part,
		int power)
{
	//Log(LOG_INFO) << "Tile::destroy()";
	bool objective = false;

	if (power >= _objects[part]->getArmor())
	{
		//Log(LOG_INFO) << ". . destroy(part)";
		objective = destroy(part);
	}

	//Log(LOG_INFO) << ". ret = " << objective;
	return objective;
}

/**
 * Sets a "virtual" explosive on this tile. We mark a tile this way to
 * detonate it later, because the same tile can be visited multiple times
 * by "explosion rays". The explosive power that gets set on a tile is
 * that of the most powerful ray that passes through it -- see TileEngine::explode().
 * @param power
 */
void Tile::setExplosive(
		int power,
		bool force)
{
	if (force
		|| _explosive < power)
	{
		_explosive = power;
	}
}

/**
 * Gets if & how powerfully this tile will explode.
 * Don't confuse this with a tile's inherent explosive power;
 * this value is set by explosions external to the tile itself.
 * @return, how big the BOOM will be / how much tile-destruction there will be
 */
int Tile::getExplosive() const
{
	return _explosive;
}

/**
 * Flammability of a tile is the flammability of its most flammable part.
 * @return, the lower the value, the higher the chance the tile catches fire
 */
int Tile::getFlammability() const
{
	int burn = 255; // not burnable.

	for (int
			i = 0;
			i < 4;
			++i)
	{
		if (_objects[i]
			&& (_objects[i]->getFlammable() < burn))
		{
			burn = _objects[i]->getFlammable();
		}
	}

	return burn;
}

/**
 * Fuel of a tile is the highest fuel of its parts/objects.
 * @return, how many turns burn
 */
int Tile::getFuel() const
{
	int fuel = 0;

	for (int
			i = 0;
			i < 4;
			++i)
	{
		if (_objects[i]
			&& (_objects[i]->getFuel() > fuel))
		{
			fuel = _objects[i]->getFuel();
		}
	}

	return fuel;
}

/**
 * Gets the flammability of a tile-part.
 * @return, the lower the value, the higher the chance the tile-part catches fire
 */
int Tile::getFlammability(int part) const
{
	return _objects[part]->getFlammable();
}

/**
 * Gets the fuel of a tile-part.
 * @return, how many turns to burn
 */
int Tile::getFuel(int part) const
{
	return _objects[part]->getFuel();
}

/**
 * Ignite starts fire on a tile, it will burn <fuel> rounds.
 * Fuel of a tile is the highest fuel of its objects
 * NOT the sum of the fuel of the objects!
 * @param power
 */
void Tile::ignite(int power)
{
	if (_fire == 0)
	{
		int fuel = getFuel();
		if (fuel > 0)
		{
			int burn = getFlammability(); // <- lower is better :)
			if (burn < 255)
			{
				power -= (burn / 10) - 15;
				if (RNG::percent(power))
				{
					_smoke = std::max(
									1,
									std::min(
											15 - (burn / 10),
											12));

					_fire = fuel + 1;
					_overlaps = 1;
					_animOffset = RNG::generate(0, 3);
				}
			}
		}
	}
}

/**
 * Animate the tile. This means to advance the current frame for every object.
 * Ufo doors are a bit special, they animate only when triggered.
 * When ufo doors are on frame 0(closed) or frame 7(open) they are not animated further.
 */
void Tile::animate()
{
	int nextFrame;

	for (int
			i = 0;
			i < 4;
			++i)
	{
		if (_objects[i])
		{
			if (_objects[i]->isUFODoor() // ufo door is static
				&& (_curFrame[i] == 0
					|| _curFrame[i] == 7))
			{
				continue;
			}

			nextFrame = _curFrame[i] + 1;

			if (_objects[i]->isUFODoor()
				&& _objects[i]->getSpecialType() == START_POINT
				&& nextFrame == 3)
			{
				nextFrame = 7;
			}

			if (nextFrame == 8)
				nextFrame = 0;

			_curFrame[i] = nextFrame;
		}
	}

	for (std::list<Particle*>::iterator
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
 * @return int, Offset
 */
int Tile::getAnimationOffset() const
{
	return _animOffset;
}

/**
 * Get the sprite of a certain part of the tile.
 * @param part
 * @return Pointer to the sprite.
 */
Surface* Tile::getSprite(int part) const
{
	if (_objects[part] == NULL)
		return NULL;

	return _objects[part]->getDataset()->getSurfaceset()->getFrame(_objects[part]->getSprite(_curFrame[part]));
}

/**
 * Sets a unit on this tile.
 * @param unit		- pointer to a unit
 * @param tileBelow	- pointer to the tile below this tile
 */
void Tile::setUnit(
		BattleUnit* unit,
		Tile* tileBelow)
{
	if (unit != NULL)
		unit->setTile(
					this,
					tileBelow);

	_unit = unit;
}

/**
 * Sets the number of turns this tile will be on fire.
 * @param fire - number of turns for this tile to burn (0 = no fire)
 */
void Tile::setFire(int fire)
{
	_fire = fire;
	_animOffset = RNG::generate(0, 3);
}

/**
 * Gets the number of turns left for this tile to be on fire.
 * @return, number of turns left for this tile to burn (0 = no fire)
 */
int Tile::getFire() const
{
	return _fire;
}

/**
 * Sets the number of turns this tile will smoke for; adds to any smoke already on a tile.
 * @param smoke - number of turns for this tile to smoke (0 = no smoke)
 */
void Tile::addSmoke(int smoke)
{
	if (_fire == 0)
	{
		if (_overlaps == 0)
			_smoke = std::max(
							1,
							std::min(
									_smoke + smoke,
									17));
		else
			_smoke += smoke;

		_animOffset = RNG::generate(0, 3);

		addOverlap();
	}
}

/**
 * Sets the number of turns this tile will smoke for. (May include fire?)
 * @param smoke - number of turns for this tile to smoke (0 = no smoke)
 */
void Tile::setSmoke(int smoke)
{
	_smoke = smoke;
	_animOffset = RNG::generate(0, 3);
}

/**
 * Gets the number of turns left for this tile to smoke. (May include fire?)
 * @return, number of turns left for this tile to smoke (0 = no smoke)
 */
int Tile::getSmoke() const
{
	return _smoke;
}

/**
 * Add an item on the tile.
 * @param item
 * @param ground
 */
void Tile::addItem(
		BattleItem* item,
		RuleInventory* ground)
{
	//Log(LOG_INFO) << "Tile::addItem()";
	item->setSlot(ground);
	_inventory.push_back(item);

	//Log(LOG_INFO) << ". setTile()";
	item->setTile(this);
	//Log(LOG_INFO) << "Tile::addItem() DONE";
}

/**
 * Remove an item from the tile.
 * @param item
 */
void Tile::removeItem(BattleItem* item)
{
	for (std::vector<BattleItem*>::iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i) == item)
		{
			_inventory.erase(i);
			break;
		}
	}

	item->setTile(NULL);
}

/**
 * Get the topmost item sprite to draw on the battlescape.
 * @return, sprite ID in floorob, or -1 when no item
 */
int Tile::getTopItemSprite()
{
	int weight = -1;
	int sprite = -1;

	for (std::vector<BattleItem*>::iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_CORPSE)	// kL
			return (*i)->getRules()->getFloorSprite();		// kL

		if ((*i)->getRules()->getWeight() > weight)
		{
			weight = (*i)->getRules()->getWeight();
			sprite = (*i)->getRules()->getFloorSprite();
		}
	}

	return sprite;
}

/**
 * kL. Gets if the tile has an unconscious xCom unit in its inventory.
 * @return, true if there's an unconscious soldier on this tile
 */
bool Tile::getHasUnconsciousSoldier() // kL
{
	for (std::vector<BattleItem*>::iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getUnit()
			&& (*i)->getUnit()->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		{
			return true;
		}
	}

	return false;
}

/**
 * New turn preparations.
 * Average out any smoke added by the number of overlaps.
 * Apply fire/smoke damage to units as applicable.
 */
void Tile::prepareTileTurn()
{
	//Log(LOG_INFO) << "Tile::prepareTileTurn()";

	// we've received new smoke in this turn, but we're not on fire, average out the smoke.
	if (_overlaps != 0
		&& _smoke != 0
		&& _fire == 0)
	{
		_smoke = std::max(
						0,
						std::min(
								(_smoke / _overlaps) - 1,
								15));
	}

/*
	if (_smoke > 0
		&& _unit
		&& _unit->isOut(true) == false
		&& (_unit->getType() == "SOLDIER"
			|| (_unit->getUnitRules()
				&& _unit->getUnitRules()->getMechanical() == false)))
//		&& _unit->getArmor()->getSize() == 1)
	{
		if (_fire)
		{
			float vuln = _unit->getArmor()->getDamageModifier(DT_IN);
			int burn = static_cast<int>(Round(40.f * vuln));
			//Log(LOG_INFO) << "Tile::prepareTileTurn(), ID " << _unit->getId() << " burn = " << burn;
			if (RNG::percent(burn)) // try to set the unit on fire.
			{
				int dur = RNG::generate(
										0,
										static_cast<int>(Round(5.f * vuln)));
				if (dur > _unit->getFire())
				{
					//Log(LOG_INFO) << ". dur = " << dur;
					_unit->setFire(dur);
				}
			}
		}
//		else // no fire: must be smoke. kL: Do both
//		{
		if (_unit->getArmor()->getDamageModifier(DT_SMOKE) > 0.f) // try to knock this unit out.
		{
			int power = (_smoke / 4) + 1;
			//Log(LOG_INFO) << ". damage -> ID " << _unit->getId() << " power = " << power;
			_unit->damage(
						Position(0, 0, 0),
						power,
						DT_SMOKE, // -> DT_STUN
						true);
		}
//		}
	} */

	_overlaps = 0;
	_danger = false;
	//Log(LOG_INFO) << "Tile::prepareTileTurn() EXIT";
}

/**
 * kL. Ends this tile's turn. Units catch on fire.
 * Separated from prepareTileTurn() above so that units take
 * damage before smoke/fire spreads to them; this is so that units
 * have to end their turn on a tile for smoke/fire to affect them.
 */
void Tile::endTileTurn()
{
	float armorVulnerability = _unit->getArmor()->getDamageModifier(DT_IN);

	if (_smoke > 0)	// need to check if unit is unconscious (ie. a corpse item on this tile) and if so give unit damage.
//		&& _unit	// this stuff is all done in BattlescapeGame::endGameTurn(), call to here.
//		&& _unit->isOut(true) == false
//		&& (_unit->getType() == "SOLDIER"
//			|| (_unit->getUnitRules()
//				&& _unit->getUnitRules()->getMechanical() == false)))
	{
		if (_fire)
		{
//			int burn = static_cast<int>(Round(40.f * armorVulnerability));
			//Log(LOG_INFO) << "Tile::endTileTurn(), ID " << _unit->getId() << " burn = " << burn;
			if (RNG::percent(static_cast<int>(Round(40.f * armorVulnerability)))) // try to set the unit on fire. Do damage from fire here, too.
			{
				int dur = RNG::generate(
									0,
									static_cast<int>(Round(5.f * armorVulnerability)));
				if (dur > _unit->getFire())
				{
					//Log(LOG_INFO) << ". dur = " << dur;
					_unit->setFire(dur);
				}
			}
		}

		if (_unit->getArmor()->getDamageModifier(DT_SMOKE) > 0.f) // try to knock this unit out.
		{
//			int power = (_smoke / 4) + 1;
			//Log(LOG_INFO) << ". damage -> ID " << _unit->getId() << " power = " << power;
			_unit->damage(
						Position(0, 0, 0),
						_smoke / 4 + 1,
						DT_SMOKE, // -> DT_STUN
						true);
		}
	}

	if (_unit->getFire() > 0 // kL: moved here from BattlescapeGame::endGameTurn()
		&& armorVulnerability > 0.f)
	{
		_unit->damage(
					Position(0, 0, 0),
					RNG::generate(3, 9),
					DT_IN,
					true);
	}
}

/**
 * Gets the inventory on this tile.
 * @return pointer to a vector of battleitems.
 */
std::vector<BattleItem*>* Tile::getInventory()
{
	return &_inventory;
}

/**
 * Sets the marker color on this tile.
 * @param color
 */
void Tile::setMarkerColor(int color)
{
	_markerColor = color;
}

/**
 * Gets the marker color on this tile.
 * @return color
 */
int Tile::getMarkerColor()
{
	return _markerColor;
}

/**
 * Sets the tile visible flag.
 * @param visibility - true if visible
 */
//kL void Tile::setVisible(int visibility)
void Tile::setVisible(bool vis) // kL
{
//kL	_visible += visibility;
	_visible = vis; // kL
}

/**
 * Gets the tile visible flag.
 * @return, true if visible
 */
//kL int Tile::getVisible()
bool Tile::getVisible() // kL
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
 * Gets the overlap value of this tile.
 * @return, overlap
 */
int Tile::getOverlaps() const
{
	return _overlaps;
}

/**
 * Increments the overlap value on this tile.
 */
void Tile::addOverlap()
{
	++_overlaps;
}

/**
 * Sets the danger flag true on this tile.
 * kL_note: Is this removed anywhere .....
 */
void Tile::setDangerous()
{
	_danger = true;
}

/**
 * Gets the danger flag on this tile.
 * @return, true if the tile is considered dangerous to aLiens
 */
bool Tile::getDangerous()
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
