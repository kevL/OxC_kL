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
#include "SerializationHelper.h"

#include "../Battlescape/Pathfinding.h"

#include "../Engine/SurfaceSet.h"

#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleArmor.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/// How many bytes various fields use in a serialized tile. See header.
Tile::SerializationKey Tile::serializationKey =
{
	4, // index
	2, // _mapDataSetId, four of these
	2, // _mapDataId, four of these
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
		_pos(pos),
		_smoke(0),
		_fire(0),
		_explosive(0),
		_explosiveType(0),
		_unit(NULL),
		_animOffset(0),
		_markerColor(0),
		_visible(false),
		_preview(-1),
		_tuMarker(-1),
		_danger(false)
{
	for (size_t
			i = 0;
			i != PARTS_TILE;
			++i)
	{
		_objects[i]			=  0;
		_mapDataId[i]		= -1;
		_mapDataSetId[i]	= -1;
		_curFrame[i]		=  0;
	}

	for (size_t
			i = 0;
			i != SECTIONS;
			++i)
	{
		_discovered[i] = false;
	}

	for (size_t
			i = 0;
			i != LIGHTLAYERS;
			++i)
	{
		_light[i] = 0;
	}
}

/**
 * dTor.
 */
Tile::~Tile()
{
	_inventory.clear();
}

/**
 * Load the tile from a YAML node.
 * @param node - reference a YAML node
 */
void Tile::load(const YAML::Node& node)
{
	for (size_t
			i = 0;
			i != PARTS_TILE;
			++i)
	{
		_mapDataId[i]		= node["mapDataID"][i]		.as<int>(_mapDataId[i]);
		_mapDataSetId[i]	= node["mapDataSetID"][i]	.as<int>(_mapDataSetId[i]);
	}

	_fire		= node["fire"]		.as<int>(_fire);
	_smoke		= node["smoke"]		.as<int>(_smoke);
	_animOffset	= node["animOffset"].as<int>(_animOffset);

	if (node["discovered"])
	{
		for (size_t
				i = 0;
				i != SECTIONS;
				++i)
		{
			_discovered[i] = node["discovered"][i].as<bool>();
		}
	}

	if (node["openDoorWest"])
		_curFrame[1] = 7;

	if (node["openDoorNorth"])
		_curFrame[2] = 7;
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
	_mapDataId[0] = unserializeInt(&buffer, serKey._mapDataId);
	_mapDataId[1] = unserializeInt(&buffer, serKey._mapDataId);
	_mapDataId[2] = unserializeInt(&buffer, serKey._mapDataId);
	_mapDataId[3] = unserializeInt(&buffer, serKey._mapDataId);

	_mapDataSetId[0] = unserializeInt(&buffer, serKey._mapDataSetId);
	_mapDataSetId[1] = unserializeInt(&buffer, serKey._mapDataSetId);
	_mapDataSetId[2] = unserializeInt(&buffer, serKey._mapDataSetId);
	_mapDataSetId[3] = unserializeInt(&buffer, serKey._mapDataSetId);

	_smoke		= unserializeInt(&buffer, serKey._smoke);
	_fire		= unserializeInt(&buffer, serKey._fire);
	_animOffset	= unserializeInt(&buffer, serKey._animOffset);

	const int boolFields = unserializeInt(
										&buffer,
										serKey.boolFields);

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
			i != PARTS_TILE;
			++i)
	{
		node["mapDataID"].push_back(_mapDataId[i]);
		node["mapDataSetID"].push_back(_mapDataSetId[i]);
	}

	if (_smoke != 0)		node["smoke"]		= _smoke;
	if (_fire != 0)			node["fire"]		= _fire;
	if (_animOffset != 0)	node["animOffset"]	= _animOffset;

	if (_discovered[0] == true
		|| _discovered[1] == true
		|| _discovered[2] == true)
	{
		for (size_t
				i = 0;
				i != SECTIONS;
				++i)
		{
			node["discovered"].push_back(_discovered[i]);
		}
	}

	if (isUfoDoorOpen(O_WESTWALL) == true)
		node["openDoorWest"] = true;
	if (isUfoDoorOpen(O_NORTHWALL) == true)
		node["openDoorNorth"] = true;

	return node;
}

/**
 * Saves the tile to binary.
 * @param buffer - pointer to pointer to buffer
 */
void Tile::saveBinary(Uint8** buffer) const
{
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[0]);
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[1]);
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[2]);
	serializeInt(buffer, serializationKey._mapDataId, _mapDataId[3]);

	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[0]);
	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[1]);
	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[2]);
	serializeInt(buffer, serializationKey._mapDataSetId, _mapDataSetId[3]);

	serializeInt(buffer, serializationKey._smoke,		_smoke);
	serializeInt(buffer, serializationKey._fire,		_fire);
	serializeInt(buffer, serializationKey._animOffset,	_animOffset);

	int boolFields = (_discovered[0] ? 0x01 : 0) + (_discovered[1] ? 0x02 : 0) + (_discovered[2] ? 0x04 : 0);

	boolFields |= isUfoDoorOpen(O_WESTWALL) ? 0x08 : 0;
	boolFields |= isUfoDoorOpen(O_NORTHWALL) ? 0x10 : 0;

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
		const MapDataType part)
{
	_objects[part] = data;
	_mapDataId[part] = dataID;
	_mapDataSetId[part] = dataSetID;
}

/**
 * Gets the MapData references of parts 0 to 3.
 * @param dataID	- pointer to dataID
 * @param dataSetID	- pointer to dataSetID
 * @param part		- the part number (MapData.h)
 * @return, the object ID
 */
void Tile::getMapData(
		int* dataID,
		int* dataSetID,
		MapDataType part) const
{
	*dataID = _mapDataId[part];
	*dataSetID = _mapDataSetId[part];
}

/**
 * Gets whether this tile has no objects.
 * @note The function does not check for a BattleUnit in this Tile.
 * @param testInventory - true to check for inventory items (default true)
 * @param testVolatiles - true to check for smoke and/or fire (default true)
 * @return, true if there is nothing but air on this tile
 */
bool Tile::isVoid(
		const bool testInventory,
		const bool testVolatiles) const
{
	bool ret = _objects[O_FLOOR] == NULL
			&& _objects[O_WESTWALL] == NULL
			&& _objects[O_NORTHWALL] == NULL
			&& _objects[O_OBJECT] == NULL;

	if (testInventory == true)
		ret = ret
		   && _inventory.empty() == true;

	if (testVolatiles == true)
		ret = ret
		   && _smoke == 0; // -> fireTiles always have smoke.

	return ret;
}

/**
 * Gets the TU cost to move over a certain part of the tile.
 * @param part		- the part type (MapData.h)
 * @param moveType	- the movement type
 * @return, TU cost
 */
int Tile::getTuCostTile(
		MapDataType part,
		MovementType moveType) const
{
	if (_objects[part] != NULL
		&& !(_objects[part]->isUfoDoor() == true
			&& _curFrame[part] > 1)
		&& !(part == O_OBJECT
			&& _objects[part]->getBigWall() > BIGWALL_NWSE)) // ie. side-walls
	{
		return _objects[part]->getTuCostPart(moveType);
	}

	return 0;
}

/**
 * Gets whether this tile has a floor or not.
 * @note If no tile-part defined as floor then it has no floor.
 * @param tileBelow - the tile below this tile (default NULL)
 * @return, true if tile has no floor
 */
bool Tile::hasNoFloor(const Tile* const tileBelow) const
{
	if (tileBelow != NULL
		&& tileBelow->getTerrainLevel() == -24)
	{
		return false;
	}

	if (_objects[O_FLOOR] != NULL)
		return _objects[O_FLOOR]->isNoFloor();

	// NOTE: Technically a bigWallBlock object-part could be a valid floor
	// but it's also bad form to place an object without a floor in the same Tile.

	return true;
}

/**
 * Gets whether this tile has a bigwall.
 * @return, true if the content-object in this tile has a bigwall ( see Big Wall enum )
 */
bool Tile::isBigWall() const
{
	if (_objects[O_OBJECT] != NULL)
		return (_objects[O_OBJECT]->getBigWall() != BIGWALL_NONE);

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

	if (_objects[static_cast<size_t>(O_FLOOR)] != NULL)
		level = _objects[static_cast<size_t>(O_FLOOR)]->getTerrainLevel();

	if (_objects[static_cast<size_t>(O_OBJECT)] != NULL)
		level = std::min(
					level,
					_objects[static_cast<size_t>(O_OBJECT)]->getTerrainLevel());

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

	if (_objects[static_cast<size_t>(O_OBJECT)] != NULL
		&& _objects[static_cast<size_t>(O_OBJECT)]->getBigWall() < BIGWALL_NESW // ie. None or Block
		&& _objects[static_cast<size_t>(O_OBJECT)]->getFootstepSound() > 0) // > -1
	{
		sound = _objects[static_cast<size_t>(O_OBJECT)]->getFootstepSound();
	}
	else if (_objects[static_cast<size_t>(O_FLOOR)] != NULL)
		sound = _objects[static_cast<size_t>(O_FLOOR)]->getFootstepSound();
	else if (_objects[static_cast<size_t>(O_OBJECT)] == NULL
		&& tileBelow != NULL
		&& tileBelow->getTerrainLevel() == -24)
	{
		sound = tileBelow->getMapData(O_OBJECT)->getFootstepSound();
	}

	return sound;
}

/**
 * Open a door on this Tile.
 * @param part		- a tile part type
 * @param unit		- pointer to a BattleUnit (default NULL)
// * @param reserved	- BattleActionType (BattlescapeGame.h) (default BA_NONE)
 * @return, -1 no door opened
 *			 0 normal door
 *			 1 ufo door
 *			 3 ufo door is still opening (animated)
 *			 4 not enough TUs
 */
int Tile::openDoor(
		const MapDataType part,
		const BattleUnit* const unit)
//		const BattleActionType reserved)
{
	if (_objects[part] != NULL)
	{
		if (_objects[part]->isDoor() == true)
		{
			if (_unit != NULL
				&& _unit != unit
				&& _unit->getPosition() != getPosition())
			{
				return DR_NONE;
			}

			if (unit != NULL
				&& unit->getTimeUnits() < _objects[part]->getTuCostPart(unit->getMoveTypeUnit()))
//											+ unit->getActionTu(reserved, unit->getMainHandWeapon(false)))
			{
				return DR_ERR_TU;
			}

			setMapData(
					_objects[part]->getDataset()->getObjects()->at(_objects[part]->getAltMCD()),
					_objects[part]->getAltMCD(),
					_mapDataSetId[part],
					_objects[part]->getDataset()->getObjects()->at(_objects[part]->getAltMCD())->getPartType());

			setMapData(NULL,-1,-1, part);

			return DR_OPEN_WOOD;
		}

		if (_objects[part]->isUfoDoor() == true)
		{
			if (_curFrame[part] == 0) // ufo door part 0 - door is closed
			{
				if (unit != NULL
					&& unit->getTimeUnits() < _objects[part]->getTuCostPart(unit->getMoveTypeUnit()))
//												+ unit->getActionTu(reserved, unit->getMainHandWeapon(false)))
				{
					return DR_ERR_TU;
				}

				_curFrame[part] = 1; // start opening door
				return DR_OPEN_METAL;
			}

			if (_curFrame[part] != 7) // ufo door != part 7 -> door is still opening
				return DR_WAIT_METAL;
		}
	}

	return DR_NONE;
}

/**
 * Closes a ufoDoor on this Tile.
 * @return, 1 if door got closed
 */
int Tile::closeUfoDoor()
{
	int ret = 0;

	for (size_t
			i = 0;
			i != PARTS_TILE;
			++i)
	{
		if (isUfoDoorOpen(static_cast<MapDataType>(i)) == true)
		{
			_curFrame[i] = 0;
			ret = 1;
		}
	}

	return ret;
}

/**
 * Sets this Tile's sections' visible flag.
 * @note Also re-caches the sprites for any unit on this Tile if the visibility
 * of the tile changes.
 * @param visible - true if discovered
 * @param section - 0 westwall
 *					1 northwall
 *					2 object+floor
 */
void Tile::setDiscovered(
		bool visible,
		int section)
{
	const size_t i = static_cast<size_t>(section);

	if (_discovered[i] != visible)
	{
		_discovered[i] = visible;

		if (visible == true && section == 2)
		{
			_discovered[0] = // if object+floor is discovered set west & north walls discovered also.
			_discovered[1] = true;
		}

		if (_unit != NULL)
			_unit->clearCache();
	}
}

/**
 * Get the black fog of war state of this Tile.
 * @param section - 0 westwall
 *					1 northwall
 *					2 object+floor
 * @return, true if discovered
 */
bool Tile::isDiscovered(int section) const
{
	return _discovered[static_cast<size_t>(section)];
}

/**
 * Reset the light amount on the tile.
 * @note This is done before a light level recalculation.
 * @param layer - light is separated in 3 layers: Ambient, Static and Dynamic
 */
void Tile::resetLight(size_t layer)
{
	_light[layer] = 0;
}

/**
 * Add the light amount on the tile.
 * @note Only add light if the current light is lower.
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
 * Gets the tile's shade amount 0-15.
 * @note It returns the brightest of all light layers.
 * @note Shade is the inverse of light level so a maximum amount of light (15)
 * returns shade level 0.
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
 * @note First remove the old object then replace it with the destroyed one
 * because the object type of the old and new are not necessarily the same. If
 * the destroyed part is an explosive set the tile's explosive value which will
 * trigger a chained explosion.
 * @param part - this Tile's part for destruction (MapData.h)
 * @param type - SpecialTileType
 * @return, true if an 'objective' was destroyed
 */
bool Tile::destroyTilePart(
		MapDataType part,
		SpecialTileType type)
{
	bool objective = false;

	if (_objects[part])
	{
		if (_objects[part]->isGravLift() == true
			|| _objects[part]->getArmor() == 255) // <- set to 255 in MCD for Truly Indestructability.
		{
			return false;
		}

		objective = (_objects[part]->getSpecialType() == type);

		const MapData* const partOrg = _objects[part];
		const int origMapDataSetID = _mapDataSetId[part];

		setMapData(NULL,-1,-1, part);

		if (partOrg->getDieMCD() != 0)
		{
			MapData* const dead = partOrg->getDataset()->getObjects()->at(partOrg->getDieMCD());
			setMapData(
					dead,
					partOrg->getDieMCD(),
					origMapDataSetID,
					dead->getPartType());
		}

		if (partOrg->getExplosive() != 0)
			setExplosive(
					partOrg->getExplosive(),
					partOrg->getExplosiveType());
	}

	if (part == O_FLOOR) // check if the floor on the lowest level is gone.
	{
		if (_pos.z == 0 && _objects[O_FLOOR] == NULL)
			setMapData( // replace with scorched earth
					MapDataSet::getScorchedEarthTile(),
					1,0, O_FLOOR);

		if (_objects[O_OBJECT] != NULL // destroy the object if floor is gone.
			&& _objects[O_OBJECT]->getBigWall() == BIGWALL_NONE)
		{
			destroyTilePart(O_OBJECT, type); // stop floating haybales.
		}
	}

	return objective;
}

/**
 * Damages terrain (check against terrain-part armor/hitpoints/constitution).
 * @param part	- part of tile to check (MapData.h)
 * @param power	- power of the damage
 * @param type	- SpecialTileType
 * @return, true if an objective was destroyed
 */
bool Tile::hitTile(
		MapDataType part,
		int power,
		SpecialTileType type)
{
	//Log(LOG_INFO) << "Tile::damage() vs part = " << part << ", hp = " << _objects[part]->getArmor();
	bool objectiveDestroyed = false;

	if (power >= _objects[part]->getArmor())
		objectiveDestroyed = destroyTilePart(part, type);

	return objectiveDestroyed;
}

/**
 * Sets a "virtual" explosive on this tile.
 * @note Mark a tile this way to detonate it later because the same tile can be
 * visited multiple times by "explosion rays". The explosive power that gets set
 * on a tile is that of the most powerful ray that passes through it -- see
 * TileEngine::explode().
 * @param power		- how big the BOOM will be / how much tile-destruction
 * @param explType	- the type of this Tile's explosion (set in MCD)
 * @param force		- forces value even if lower (default false)
 */
void Tile::setExplosive(
		int power,
		int explType,
		bool force)
{
	if (force == true || _explosive < power)
	{
		_explosive = power;
		_explosiveType = explType;
	}
}

/**
 * Gets if & how powerfully this tile will explode.
 * @note Don't confuse this with a tile's inherent explosive power. This value
 * is set by explosions external to the tile itself.
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
 * Flammability of a tile is the flammability of its most flammable tile-part.
 * @return, the lower the value the higher the chance the tile catches fire - BSZAAST!!!
 */
int Tile::getFlammability() const
{
	int burn = 255; // not burnable. <- lower is better :)

	for (size_t
			i = 0;
			i != PARTS_TILE;
			++i)
	{
		if (_objects[i] != NULL
			&& _objects[i]->getFlammable() < burn)
		{
			burn = _objects[i]->getFlammable();
		}
	}

	return convertBurnToPct(burn);
}

/**
 * Gets the flammability of a tile-part.
 * @note I now decree that this returns the inverse of 0..255 as a percentage!
 * @param part - the part to check (MapData.h)
 * @return, the lower the value the higher the chance the tile-part catches fire - BSZAAST!!!
 */
int Tile::getFlammability(MapDataType part) const
{
	return convertBurnToPct(_objects[part]->getFlammable());
}

/**
 * Converts obscure inverse MCD notation to understandable percentages.
 * @note Chance can be increased by the power of the spark.
 * @param burn - flammability from an MCD file (see MapData)
 * @return, basic percent chance that this stuff burns
 */
int Tile::convertBurnToPct(int burn) const // private.
{
	if (burn > 254)
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
 * Gets the fuel of a tile-part.
 * @note Fuel of a tile is the highest fuel of its parts/objects.
 * @note This is NOT the sum of the fuel of the objects!
 * @param part - the part to check or O_NULTYPE to check all parts (default O_NULTYPE)
 * @return, turns to burn
 */
int Tile::getFuel(MapDataType part) const
{
	if (part == O_NULTYPE)
	{
		int fuel = 0;

		for (size_t
				i = 0;
				i != PARTS_TILE;
				++i)
		{
			if (_objects[i] != NULL
				&& _objects[i]->getFuel() > fuel)
			{
				fuel = _objects[i]->getFuel();
			}
		}

		return fuel;
	}

	return _objects[part]->getFuel();
}

/**
 * Tries to start fire on this Tile.
 * @note If true it will add its fuel as turns to burn.
 * @note Called by floor-burning Silacoids and fire spreading @ turnovers and
 * by TileEngine::detonate() after HE explosions.
 * @param power - rough chance to get things going
 * @return, true if tile catches fire
 */
bool Tile::ignite(int power)
{
	if (power != 0 && isSmokable() == true)
	{
		const int fuel = getFuel();
		if (fuel != 0)
		{
			const int burn = getFlammability();
			if (burn != 0)
			{
				power = ((((power + 4) / 5) + ((burn + 7) / 8) + (fuel * 3) + 6) / 7);
				if (RNG::percent(power) == true)
				{
					addSmoke((burn + 15) / 16);

					// TODO: pass in tileBelow and check its terrainLevel for -24; drop fire through to any tileBelow ...
					if (isFirable() == true)
						addFire(fuel + 1);

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
	if (turns != 0 && isFirable() == true)
	{
		if (_smoke == 0 && _fire == 0)
			_animOffset = RNG::seedless(0,3);

		_fire += turns;

		if (_fire > 12) _fire = 12;

		if (_smoke < _fire + 2)
			_smoke = _fire + RNG::seedless(2,3);
	}
}

/**
 * Reduces the number of turns this Tile will burn.
 */
void Tile::decreaseFire()
{
	if (--_fire < 1)
	{
		_fire = 0;
		if (_smoke == 0)
			_animOffset = 0;
	}
}

/**
 * Gets the number of turns left for this tile to be on fire.
 * @return, turns left
 */
int Tile::getFire() const
{
	return _fire;
}

/**
 * Adds smoke to this Tile.
 * @param turns - turns to smoke for
 */
void Tile::addSmoke(int turns)
{
	if (turns != 0 && isSmokable() == true)
	{
		if (_smoke == 0 && _fire == 0)
			_animOffset = RNG::seedless(0,3);

		if ((_smoke += turns) > 17)
			_smoke = 17;
	}
}

/**
 * Reduces the number of turns this Tile will smoke for.
 */
void Tile::decreaseSmoke()
{
	if (_fire != 0) // don't let smoke deplete faster than fire depletes.
		--_smoke;
	else
		_smoke -= (RNG::seedless(1, _smoke) + 2) / 3;

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
 * Checks if this Tile can have smoke.
 * @note Only the object is checked. Diagonal bigWalls never smoke.
 * @return, true if smoke can occupy this Tile
 */
bool Tile::isSmokable() const // private.
{
	return _objects[O_OBJECT] == NULL
		|| (_objects[O_OBJECT]->getBigWall() != BIGWALL_NESW
			&& _objects[O_OBJECT]->getBigWall() != BIGWALL_NWSE
			&& _objects[O_OBJECT]->blockSmoke() == false);
}

/**
 * Checks if this Tile can have fire.
 * @note Only the floor and object are checked. Diagonal bigWalls never fire.
 * Fire needs a floor.
 * @return, true if fire can occupy this Tile
 */
bool Tile::isFirable() const // private.
{
	return (_objects[O_FLOOR] != NULL
		&& _objects[O_FLOOR]->blockFire() == false)
		&& (_objects[O_OBJECT] == NULL
			|| (_objects[O_OBJECT]->getBigWall() != BIGWALL_NESW
				&& _objects[O_OBJECT]->getBigWall() != BIGWALL_NWSE
				&& _objects[O_OBJECT]->blockFire() == false));
}

/**
 * Ends this tile's turn. Units catch on fire.
 * @note Separated from resolveOverlaps() above so that units take damage before
 * smoke/fire spreads to them; this is so that units would have to end their
 * turn on a tile before smoke/fire damages them. That is they get a chance to
 * get off the tile during their turn.
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
	//Log(LOG_INFO) << ". pFire = " << pFire;

	float vulnr;

	if (battleSave == NULL)	// damage standing units at end of faction's turn-phase. Notice this hits only the primary quadrant!
//		&& _unit != NULL)	// safety. call from BattlescapeGame::endTurnPhase() checks only Tiles w/ units, but that could change ....
	{
		//Log(LOG_INFO) << ". . hit Unit";
		if (pSmoke != 0
			&& _unit->isHealable() == true)
		{
			//Log(LOG_INFO) << ". . . healable TRUE";
			vulnr = _unit->getArmor()->getDamageModifier(DT_SMOKE);
			if (vulnr > 0.f) // try to knock _unit out.
			{
				_unit->damage(
							Position(0,0,0),
							static_cast<int>(Round(static_cast<float>(pSmoke) * vulnr)),
							DT_SMOKE, // -> DT_STUN
							true);
				//Log(LOG_INFO) << ". . . . smoke Dam = " << d;
			}
		}

		if (pFire != 0)
		{
			vulnr = _unit->getArmor()->getDamageModifier(DT_IN);
			if (vulnr > 0.f)
			{
				_unit->damage(
							Position(0,0,0),
							static_cast<int>(Round(static_cast<float>(pFire) * vulnr)),
							DT_IN,
							true);

				if (RNG::percent(static_cast<int>(Round(40.f * vulnr))) == true) // try to set _unit on fire. Do damage from fire here, too.
				{
					const int dur = RNG::generate(1,
												static_cast<int>(Round(5.f * vulnr)));
					if (dur > _unit->getFireUnit())
						_unit->setFireUnit(dur);
				}
			}
		}
	}
	else //if (battleSave != NULL) // try to destroy items & kill unconscious units at end of full-turns
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
					&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
					&& unit->getTakenExpl() == false)
				{
					//Log(LOG_INFO) << ". . unConsc unit (smoke) " << unit->getId();
					unit->setTakenExpl();

					vulnr = unit->getArmor()->getDamageModifier(DT_SMOKE);
					if (vulnr > 0.f)
						unit->damage(
								Position(0,0,0),
								static_cast<int>(Round(static_cast<float>(pSmoke) * vulnr)),
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
						&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
						&& unit->getTakenFire() == false)
					{
						//Log(LOG_INFO) << ". . unConsc unit (fire) " << unit->getId();
						unit->setTakenFire();

						vulnr = unit->getArmor()->getDamageModifier(DT_IN);
						if (vulnr > 0.f)
						{
							unit->damage(
									Position(0,0,0),
									static_cast<int>(static_cast<float>(pFire) * vulnr),
									DT_IN,
									true);

							if (unit->getHealth() == 0)
							{
								//Log(LOG_INFO) << ". . . dead";
								unit->instaKill();
								unit->killedBy(unit->getFaction()); // killed by self ....
								//Log(LOG_INFO) << "Tile::hitStuff() " << unit->getId() << " killedBy = " << (int)unit->getFaction();
							}
						}

						++i;
						done = (i == _inventory.end());
					}
					else if (pFire > (*i)->getRules()->getArmor() // no modifier when destroying items, not even corpse in bodyarmor.
						&& (unit == NULL || unit->getUnitStatus() == STATUS_DEAD))
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
 * Animate the tile.
 * @note This means to advance the current frame for every part. Ufo doors are a
 * bit special - they animate only when triggered. When ufo doors are on frame 0
 * (closed) or frame 7 (open) they are not animated further.
 */
void Tile::animateTile()
{
	int nextFrame;

	for (size_t
			i = 0;
			i != PARTS_TILE;
			++i)
	{
		if (_objects[i] != NULL)
		{
			const int isPsycho = _objects[i]->isPsychedelic();
			if (isPsycho == 0)
			{
				if (_objects[i]->isUfoDoor() == false
					|| (_curFrame[i] != 0
						&& _curFrame[i] != 7)) // ufo door is currently static
				{
					nextFrame = _curFrame[i] + 1;

					if (_objects[i]->isUfoDoor() == true // special handling for Avenger & Lightning doors
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
			else if (isPsycho == 1)
			{
				if (RNG::seedless(0,2) != 0)
					_curFrame[i] = RNG::seedless(0,7);
			}
			else if (RNG::seedless(0,2) == 0) // isPsycho== 2
				_curFrame[i] = RNG::seedless(0,7);
		}
	}
}

/**
 * Get the number of frames the fire or smoke animation is off-sync.
 * @note To void fire and smoke animations of different tiles moving nice in
 * sync - that'd look fake.
 * @return, offset
 */
int Tile::getAnimationOffset() const
{
	return _animOffset;
}

/**
 * Get the sprite of a certain part of this Tile.
 * @param part - tile part to get a sprite for
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
 * Sets a unit on this Tile.
 * @param unit		- pointer to a BattleUnit
 * @param tileBelow	- pointer to the Tile below this Tile (default NULL)
 */
void Tile::setUnit(
		BattleUnit* const unit,
		const Tile* const tileBelow)
{
	if (unit != NULL)
		unit->setTile(this, tileBelow);

	_unit = unit;
}

/**
 * Sets a unit transitorily on this Tile.
 * @param unit - pointer to a BattleUnit
 */
void Tile::setTransitUnit(BattleUnit* const unit)
{
	_unit = unit;
}

/**
 * Add an item on this Tile.
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
 * Remove an item from this Tile.
 * @param item - pointer to a BattleItem
 */
void Tile::removeItem(BattleItem* const item)
{
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
 * Gets a corpse-sprite to draw on the battlefield.
 * @param fired - pointer to set fire true
 * @return, sprite ID in floorobs (-1 none)
 */
int Tile::getCorpseSprite(bool* fired) const
{
	int sprite (-1);

	if (_inventory.empty() == false)
	{
		*fired = false;
		int
			weight (-1),
			weightTest;

		for (std::vector<BattleItem*>::const_iterator // 1. soldier body
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getUnit() != NULL
				&& (*i)->getUnit()->getGeoscapeSoldier() != NULL)
			{
				weightTest = (*i)->getRules()->getWeight();
				if (weightTest > weight)
				{
					weight = weightTest;
					sprite = (*i)->getRules()->getFloorSprite();

					if ((*i)->getUnit()->getFireUnit() != 0)
						*fired = true;
				}
			}
		}

		if (sprite == -1)
		{
			weight = -1;
			for (std::vector<BattleItem*>::const_iterator // 2. non-soldier body
					i = _inventory.begin();
					i != _inventory.end();
					++i)
			{
				if ((*i)->getUnit() != NULL)
				{
					weightTest = (*i)->getRules()->getWeight();
					if (weightTest > weight)
					{
						weight = weightTest;
						sprite = (*i)->getRules()->getFloorSprite();

						if ((*i)->getUnit()->getFireUnit() != 0)
							*fired = true;
					}
				}
			}

			if (sprite == -1)
			{
				weight = -1;
				for (std::vector<BattleItem*>::const_iterator // 3. corpse
						i = _inventory.begin();
						i != _inventory.end();
						++i)
				{
					if ((*i)->getRules()->getBattleType() == BT_CORPSE)
					{
						weightTest = (*i)->getRules()->getWeight();
						if (weightTest > weight)
						{
							weight = weightTest;
							sprite = (*i)->getRules()->getFloorSprite();
						}
					}
				}
			}
		}
	}

	return sprite;
}

/**
 * Get the topmost item sprite to draw on the battlefield.
 * @param primed - pointer to set primed true
 * @return, sprite ID in floorobs (-1 none)
 */
int Tile::getTopSprite(bool* primed) const
{
	int sprite (-1);

	if (_inventory.empty() == false)
	{
		const BattleItem* grenade (NULL);
		*primed = false;
		BattleType bType;

		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getFuse() > -1)
			{
				bType = (*i)->getRules()->getBattleType();
				if (bType == BT_PROXYGRENADE)
				{
					*primed = true;
					return (*i)->getRules()->getFloorSprite();
				}
				else if (bType == BT_GRENADE)
				{
					*primed = true;
					grenade = *i;
				}
			}
		}

		if (grenade != NULL)
			return grenade->getRules()->getFloorSprite();

		int
			weight (-1),
			weightTest;

		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			weightTest = (*i)->getRules()->getWeight();
			if (weightTest > weight)
			{
				weight = weightTest;
				sprite = (*i)->getRules()->getFloorSprite();
			}
		}
	}

	return sprite;
}

/**
 * Gets if this Tile has an unconscious unit in its inventory.
 * @param playerOnly - true to check for only xCom units (default true)
 * @return,	0 - no living Soldier
 *			1 - stunned Soldier
 *			2 - stunned and wounded Soldier
 */
int Tile::hasUnconsciousUnit(bool playerOnly) const
{
	int ret (0);

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		const BattleUnit* const bu ((*i)->getUnit());

		if (bu != NULL
			&& bu->getUnitStatus() == STATUS_UNCONSCIOUS
			&& (bu->getOriginalFaction() == FACTION_PLAYER
				|| playerOnly == false))
		{
			if (playerOnly == true && bu->getFatalWounds() == 0)
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
 * Sets the direction of path preview arrows.
 * @param dir - a direction
 */
void Tile::setPreviewDir(int dir)
{
	_preview = dir;
}

/**
 * Gets the direction of path preview arrows.
 * @return, preview direction
 */
int Tile::getPreviewDir() const
{
	return _preview;
}

/**
 * Sets a number to be displayed by pathfinding preview.
 * @param tu - # of TUs left if/when this tile is reached
 */
void Tile::setPreviewTu(int tu)
{
	_tuMarker = tu;
}

/**
 * Gets the number to be displayed for pathfinding preview.
 * @return, # of TUs left if/when this tile is reached
 */
int Tile::getPreviewTu() const
{
	return _tuMarker;
}

/**
 * Sets the path preview marker color on this tile.
 * @param color - color of marker
 */
void Tile::setPreviewColor(Uint8 color)
{
	_markerColor = color;
}

/**
 * Gets the path preview marker color on this tile.
 * @return, color of marker
 */
int Tile::getPreviewColor() const
{
	return static_cast<int>(_markerColor);
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

}
