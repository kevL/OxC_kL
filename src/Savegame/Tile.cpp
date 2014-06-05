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

#include "BattleItem.h"
#include "BattleUnit.h"
#include "SerializationHelper.h"

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
		_unit(0),
		_animOffset(0),
		_markerColor(0),
		_visible(false),
		_preview(-1),
		_tuMarker(0),
		_overlaps(0),
		_danger(false)
{
	for (int
			i = 0;
			i < 4;
			++i)
	{
		_objects[i]			=  0;
		_mapDataID[i]		= -1;
		_mapDataSetID[i]	= -1;
		_currFrame[i]		=  0;
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
 * destructor
 */
Tile::~Tile()
{
	_inventory.clear();
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
		_currFrame[1] = 7;

	if (node["openDoorNorth"])
		_currFrame[2] = 7;
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

//kL    Uint8 boolFields = unserializeInt(&buffer, serKey.boolFields);
    Uint8 boolFields = static_cast<Uint8>(unserializeInt(&buffer, serKey.boolFields)); // kL

	_discovered[0] = (boolFields & 1)? true: false;
	_discovered[1] = (boolFields & 2)? true: false;
	_discovered[2] = (boolFields & 4)? true: false;

	_currFrame[1] = (boolFields & 8)? 7: 0;
	_currFrame[2] = (boolFields & 0x10)? 7: 0;
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
 * @param dat			- pointer to the data object
 * @param mapDataID		- mapDataID
 * @param mapDataSetID	- mapDataSetID
 * @param part			- the part number
 */
void Tile::setMapData(
		MapData* dat,
		int mapDataID,
		int mapDataSetID,
		int part)
{
	_objects[part]		= dat;
	_mapDataID[part]	= mapDataID;
	_mapDataSetID[part]	= mapDataSetID;
}

/**
 * Gets the MapData references of parts 0 to 3.
 * @param mapDataID		- pointer to mapDataID
 * @param mapDataSetID	- pointer to mapDataSetID
 * @param part			- the part number
 * @return, the object ID
 */
void Tile::getMapData(
		int* mapDataID,
		int* mapDataSetID,
		int part) const
{
	*mapDataID		= _mapDataID[part];
	*mapDataSetID	= _mapDataSetID[part];
}

/**
 * Gets whether this tile has no objects. Note that we can have a unit or smoke on this tile.
 * @return, bool True if there is nothing but air on this tile.
 */
bool Tile::isVoid() const
{
	return _objects[0] == 0
			&& _objects[1] == 0
			&& _objects[2] == 0
			&& _objects[3] == 0
			&& _smoke == 0
			&& _inventory.empty();
}

/**
 * Get the TU cost to walk over a certain part of the tile.
 * @param part
 * @param movementType
 * @return TU cost
 */
int Tile::getTUCost(
		int part,
		MovementType movementType) const
{
	if (_objects[part])
	{
		if (_objects[part]->isUFODoor()
//kL			&& _currFrame[part] == 7)
			&& _currFrame[part] > 1) // cfailde:doorcost
		{
			return 0;
		}

		if (_objects[part]->getBigWall() >= 4)
			return 0;

		return _objects[part]->getTUCost(movementType);
	}
	else
		return 0;
}

/**
 * Gets whether this tile has a floor or not. If no object defined as floor, it has no floor.
 * @param tileBelow - the tile below this tile
 * @return, true if tile has no floor
 */
bool Tile::hasNoFloor(Tile* tileBelow) const
{
	if (tileBelow != 0
		&& tileBelow->getTerrainLevel() == -24)
	{
		return false;
	}

	if (_objects[MapData::O_FLOOR])
		return _objects[MapData::O_FLOOR]->isNoFloor();
	else
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
	else
		return false;
}

/**
 * If an object stands on this tile, this returns how high the unit is standing.
 * @return the level in pixels
 */
int Tile::getTerrainLevel() const
{
	int level = 0;

	if (_objects[MapData::O_FLOOR])
		level = _objects[MapData::O_FLOOR]->getTerrainLevel();

	if (_objects[MapData::O_OBJECT])
		level += _objects[MapData::O_OBJECT]->getTerrainLevel();

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
		&& _objects[MapData::O_OBJECT]->getBigWall() == 0)
	{
		sound = _objects[MapData::O_OBJECT]->getFootstepSound();
	}

	if (!_objects[MapData::O_FLOOR]
		&& !_objects[MapData::O_OBJECT]
		&& tileBelow != 0
		&& tileBelow->getTerrainLevel() == -24)
	{
		sound = tileBelow->getMapData(MapData::O_OBJECT)->getFootstepSound();
	}

	return sound;
}

/**
 * Open a door on this tile.
 * @param wall		- a tile part
 * @param unit		- pointer to a unit
 * @param reserve	- see BA_* enum for TU reserves
 * @return, Value: -1 no door opened
 *					0 normal door
 *					1 ufo door
 *					3 ufo door is still opening (animated)
 *					4 not enough TUs
 */
int Tile::openDoor(
		int wall,
		BattleUnit* unit,
		BattleActionType reserve)
{
	// kL_note: Am parsing this tighter with if/else's
	// May lead to ambiguity, esp. if isDoor() & isUFODoor() are not exclusive;
	// or if setMapData() is doing something ...... -> to UFO doors.

	if (!_objects[wall])
		return -1;
	else if (_objects[wall]->isDoor())
	{
		if (_unit
			&& _unit != unit
			&& _unit->getPosition() != getPosition())
		{
			return -1;
		}

		if (unit
			&& unit->getTimeUnits() < _objects[wall]->getTUCost(unit->getArmor()->getMovementType())
									+ unit->getActionTUs(
														reserve,
														unit->getMainHandWeapon(false)))
		{
			return 4;
		}

		setMapData(
				_objects[wall]->getDataset()->getObjects()->at(_objects[wall]->getAltMCD()),
				_objects[wall]->getAltMCD(),
				_mapDataSetID[wall],
				_objects[wall]->getDataset()->getObjects()->at(_objects[wall]->getAltMCD())->getObjectType());
		setMapData(0,-1,-1, wall);

		return 0;
	}
	else if (_objects[wall]->isUFODoor())
	{
		if (_currFrame[wall] == 0) // ufo door wall 0 - door is closed
		{
			if (unit
				&& unit->getTimeUnits() < _objects[wall]->getTUCost(unit->getArmor()->getMovementType())
										+ unit->getActionTUs(
															reserve,
															unit->getMainHandWeapon(false)))
			{
				return 4;
			}

			_currFrame[wall] = 1; // start opening door

			return 1;
		}
		else if (_currFrame[wall] != 7) // ufo door != wall 7 -> door is still opening
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
			_currFrame[part] = 0;
			ret = 1;
		}
	}

	return ret;
}

/**
 * Sets the tile's cache flag. - TODO: set this for each object separately?
 * @param flag, true/false
 * @param part, 0-2 westwall/northwall/content+floor
 */
void Tile::setDiscovered(
		bool flag,
		int part)
{
	if (_discovered[part] != flag)
	{
		_discovered[part] = flag;

		if (part == 2
			&& flag == true)
		{
			_discovered[0] = true;
			_discovered[1] = true;
		}

		// if light/visibility on tile changes, units and objects on it change light too
		if (_unit != 0)
			_unit->setCache(0);
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
 * @param layer Light is separated in 3 layers: Ambient, Static and Dynamic.
 */
void Tile::resetLight(int layer)
{
	_light[layer] = 0;
	_lastLight[layer] = _light[layer];
}

/**
 * Add the light amount on the tile. Only add light if the current light is lower.
 * @param light Amount of light to add.
 * @param layer Light is separated in 3 layers: Ambient, Static and Dynamic.
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
 * @return shade
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

	return 15 - light;
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
 * Flammability of a tile is the flammability of its content or, if no content, its floor.
 * @return, The lower the value, the higher the chance the tile/object catches fire.
 */
int Tile::getFlammability() const
{
	int burn = 255; // not burnable.

	if (_objects[3]) // content-part
		burn = _objects[3]->getFlammable();
	else if (_objects[0]) // floor
		burn = _objects[0]->getFlammable();

	return burn;
}

/**
 * Fuel of a tile is the lowest flammability of its parts/objects.
 * @return how long to burn.
 */
int Tile::getFuel() const
{
	int fuel = 0;

	if (_objects[3])
		fuel = _objects[3]->getFuel();
	else if (_objects[0])
		fuel = _objects[0]->getFuel();

	return fuel;
}

/**
 * Ignite starts fire on a tile, it will burn <fuel> rounds.
 * Fuel of a tile is the highest fuel of its objects,
 * NOT the sum of the fuel of the objects!
 * @param power - i think this is <fuel> rounds ...
 */
void Tile::ignite(int power)
{
	if (_fire == 0)
	{
		int burn = getFlammability(); // <- lower is better :)
		if (burn != 255)
		{
			power = power - (burn / 10) + 15;
			if (RNG::percent(power))
			{
//kL				_smoke = 15 - std::max(
//kL									1,
//kL									std::min(
//kL											burn / 10,
//kL											12));
				// kL. from TileEngine::explode()
				_smoke = std::max(
								1,
								std::min(
										15 - (burn / 10),
										12));
				_fire = getFuel() + 1;
				_overlaps = 1;
				_animOffset = RNG::generate(0, 3);
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
	int newframe;

	for (int
			i = 0;
			i < 4;
			++i)
	{
		if (_objects[i])
		{
			if (_objects[i]->isUFODoor() // ufo door is static
				&& (_currFrame[i] == 0
					|| _currFrame[i] == 7))
			{
				continue;
			}

			newframe = _currFrame[i] + 1;

			if (_objects[i]->isUFODoor()
				&& _objects[i]->getSpecialType() == START_POINT
				&& newframe == 3)
			{
				newframe = 7;
			}

			if (newframe == 8)
				newframe = 0;

			_currFrame[i] = newframe;
		}
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
	if (_objects[part] == 0)
		return 0;

	return _objects[part]->getDataset()->getSurfaceset()->getFrame(_objects[part]->getSprite(_currFrame[part]));
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
	if (unit != 0)
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
									15));
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

	item->setTile(0);
}

/**
 * Get the topmost item sprite to draw on the battlescape.
 * @return item sprite ID in floorob, or -1 when no item
 */
int Tile::getTopItemSprite()
{
	int biggestWeight = -1;
	int biggestItem = -1;

	for (std::vector<BattleItem*>::iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->getWeight() > biggestWeight)
		{
			biggestWeight = (*i)->getRules()->getWeight();
			biggestItem = (*i)->getRules()->getFloorSprite();
		}
	}

	return biggestItem;
}

/**
 * New turn preparations.
 * average out any smoke added by the number of overlaps.
 * apply fire/smoke damage to units as applicable. kL_note: This should happen only at end Turn.
 */
void Tile::prepareNewTurn()
{
	//Log(LOG_INFO) << "Tile::prepareNewTurn()";

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


	if (_unit
		&& !_unit->isOut(true)
		&& _unit->getArmor()->getSize() == 1) // large units don't catch fire, i Guess - nor do Smoke
	{
		if (_fire)
		{
//kL			if (_unit->getArmor()->getSize() == 1
//kL				|| !_unit->getTookFire()) // this is how to avoid hitting the same unit multiple times.
					// kL_note: Set the guy on fire here, set the toggle so he doesn't take
					// damage in BattleUnit::prepareNewTurn() but don't deliver any damage here.
//kL			{
//kL				_unit->toggleFireDamage();

				// battleUnit is standing on fire tile about to start burning.
				//Log(LOG_INFO) << ". ID " << _unit->getId() << " fireDamage = " << _smoke;
//kL				_unit->damage(Position(0, 0, 0), _smoke, DT_IN, true); // _smoke becomes our damage value

			float modifier = _unit->getArmor()->getDamageModifier(DT_IN);
			int burnChance = static_cast<int>(40.f * modifier);
			//Log(LOG_INFO) << "Tile::prepareNewTurn(), ID " << _unit->getId() << " burnChance = " << burnChance;
			if (RNG::percent(burnChance)) // try to set the unit on fire.
			{
				int burnTime = RNG::generate(0, static_cast<int>(5.f * modifier));
				if (burnTime > _unit->getFire())
				{
					//Log(LOG_INFO) << ". burnTime = " << burnTime;
					_unit->setFire(burnTime);
				}
			}
//kL			}
		}
//		else	// no fire: must be smoke -> keep this in, because I don't want something quirky happening
				// like a unit catching fire and going unconscious, although I doubt it matters!! So,
				// leave it out 'cause I'm sure there are other ways to pass out while on fire.
		// kL_note: Take smoke also.
		// kL_note: I could make unconscious units suffer more smoke inhalation... not yet.

		if (_smoke) // if we still have smoke/fire
		{
			if (_unit->getOriginalFaction() != FACTION_HOSTILE // aliens don't breathe
				&& _unit->getArmor()->getSize() == 1 // does not affect tanks. (could use turretType)
				&& _unit->getArmor()->getDamageModifier(DT_SMOKE) > 0.f) // try to knock this soldier out.
			{
				int smokePower = (_smoke / 4) + 1;
				//Log(LOG_INFO) << ". damage -> ID " << _unit->getId() << " smokePower = " << smokePower;
				_unit->damage(
							Position(0, 0, 0),
							smokePower,
							DT_SMOKE, // -> DT_STUN
							true);
			}
		}
	}

	_overlaps = 0;
	_danger = false;
	//Log(LOG_INFO) << "Tile::prepareNewTurn() EXIT";
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
void Tile::setVisible(bool isVis) // kL
{
//kL	_visible += visibility;
	_visible = isVis; // kL
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

}
