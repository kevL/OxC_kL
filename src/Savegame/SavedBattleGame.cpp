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

#include <vector>
#include <deque>
#include <queue>

#include <assert.h>

#include "BattleItem.h"
#include "Node.h"
#include "SavedBattleGame.h"
#include "SavedGame.h"
#include "SerializationHelper.h"
#include "Tile.h"

#include "../Battlescape/AlienBAIState.h"
#include "../Battlescape/CivilianBAIState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/Position.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes a brand new battlescape saved game.
 */
SavedBattleGame::SavedBattleGame()
	:
		_battleState(0),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_tiles(),
		_selectedUnit(0),
		_lastSelectedUnit(0),
		_nodes(),
		_units(),
		_items(),
		_pathfinding(0),
		_tileEngine(0),
		_missionType(""),
		_globalShade(0),
		_side(FACTION_PLAYER),
		_turn(1),
		_debugMode(false),
		_aborted(false),
		_itemId(0),
		_objectiveDestroyed(false),
		_fallingUnits(),
		_unitsFalling(false),
		_cheating(false),
		_tuReserved(BA_NONE),
		_kneelReserved(false),
		_terrain("") // kL sza_MusicRules
{
	//Log(LOG_INFO) << "\nCreate SavedBattleGame";
	_tileSearch.resize(11 * 11);

	for (int
			i = 0;
			i < 121;
			++i)
	{
		_tileSearch[i].x = (i %11) - 5;
		_tileSearch[i].y = (i / 11) - 5;
	}
}

/**
 * Deletes the game content from memory.
 */
SavedBattleGame::~SavedBattleGame()
{
	//Log(LOG_INFO) << "Delete SavedBattleGame";
	for (int
			i = 0;
			i < _mapsize_z * _mapsize_y * _mapsize_x;
			++i)
	{
		delete _tiles[i];
	}
	delete[] _tiles;

	for (std::vector<Node*>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleUnit*>::iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleItem*>::iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleItem*>::iterator
			i = _deleted.begin();
			i != _deleted.end();
			++i)
	{
		delete *i;
	}

	delete _pathfinding;
	delete _tileEngine;
}

/**
 * Loads the saved battle game from a YAML file.
 * @param node YAML node.
 * @param rule for the saved game.
 * @param savedGame Pointer to saved game.
 */
void SavedBattleGame::load(
		const YAML::Node& node,
		Ruleset* rule,
		SavedGame* savedGame)
{
	//Log(LOG_INFO) << "SavedBattleGame::load()";
	_mapsize_x			= node["width"].as<int>(_mapsize_x);
	_mapsize_y			= node["length"].as<int>(_mapsize_y);
	_mapsize_z			= node["height"].as<int>(_mapsize_z);
	_missionType		= node["missionType"].as<std::string>(_missionType);
	_globalShade		= node["globalshade"].as<int>(_globalShade);
	_turn				= node["turn"].as<int>(_turn);
	_terrain			= node["terrain"].as<std::string>(_terrain); // sza_MusicRules
	int selectedUnit	= node["selectedUnit"].as<int>();

	for (YAML::const_iterator
			i = node["mapdatasets"].begin();
			i != node["mapdatasets"].end();
			++i)
	{
		std::string name = i->as<std::string>();
		MapDataSet* mds = new MapDataSet(name);
		_mapDataSets.push_back(mds);
	}

	initMap(
			_mapsize_x,
			_mapsize_y,
			_mapsize_z);

	if (!node["tileTotalBytesPer"])
	{
		// binary tile data not found, load old-style text tiles :(
		for (YAML::const_iterator
				i = node["tiles"].begin();
				i != node["tiles"].end();
				++i)
		{
			Position pos = (*i)["position"].as<Position>();
			getTile(pos)->load((*i));
		}
	}
	else
	{
		// load key to how the tile data was saved
		Tile::SerializationKey serKey;
		size_t totalTiles = node["totalTiles"].as<size_t>();

        memset(
				&serKey,
				0,
				sizeof(Tile::SerializationKey));

		serKey.index			= node["tileIndexSize"].as<Uint8>(serKey.index);
		serKey.totalBytes		= node["tileTotalBytesPer"].as<Uint32>(serKey.totalBytes);
		serKey._fire			= node["tileFireSize"].as<Uint8>(serKey._fire);
		serKey._smoke			= node["tileSmokeSize"].as<Uint8>(serKey._smoke);
		serKey._animOffset		= node["tileOffsetSize"].as<Uint8>(serKey._animOffset); // kL
		serKey._mapDataID		= node["tileIDSize"].as<Uint8>(serKey._mapDataID);
		serKey._mapDataSetID	= node["tileSetIDSize"].as<Uint8>(serKey._mapDataSetID);
		serKey.boolFields		= node["tileBoolFieldsSize"].as<Uint8>(1); // boolean flags used to be stored in an unmentioned byte (Uint8) :|

		// load binary tile data!
		YAML::Binary binTiles = node["binTiles"].as<YAML::Binary>();

		Uint8* r = (Uint8*)binTiles.data();
		Uint8* dataEnd = r + totalTiles * serKey.totalBytes;

		while (r < dataEnd)
		{
			int index = unserializeInt(&r, serKey.index);
			assert(index >= 0 && index < _mapsize_x * _mapsize_z * _mapsize_y);
			_tiles[index]->loadBinary(r, serKey); // loadBinary's privileges to advance *r have been revoked
			r += serKey.totalBytes-serKey.index; // r is now incremented strictly by totalBytes in case there are obsolete fields present in the data
		}
	}

	if (_missionType == "STR_BASE_DEFENSE")
	{
		if (node["moduleMap"])
			_baseModules = node["moduleMap"].as<std::vector<std::vector<std::pair<int, int> > > >();
		else
			// backwards compatibility: imperfect solution, modules that were completely destroyed
			// prior to saving and updating builds will be counted as indestructible.
			calculateModuleMap();
	}

	for (YAML::const_iterator
			i = node["nodes"].begin();
			i != node["nodes"].end();
			++i)
	{
		Node* n = new Node();
		n->load(*i);
		_nodes.push_back(n);
	}

	for (YAML::const_iterator
			i = node["units"].begin();
			i != node["units"].end();
			++i)
	{
		UnitFaction faction = (UnitFaction)(*i)["faction"].as<int>();
		int id = (*i)["soldierId"].as<int>();

		BattleUnit* unit;
		if (id < BattleUnit::MAX_SOLDIER_ID)	// BattleUnit is linked to a geoscape soldier
			unit = new BattleUnit(				// look up the matching soldier
								savedGame->getSoldier(id),
								faction,
								static_cast<int>(savedGame->getDifficulty())); // kL_add: For VictoryPts value per death.
		else
		{
			std::string type	= (*i)["genUnitType"].as<std::string>();
			std::string armor	= (*i)["genUnitArmor"].as<std::string>();

			unit = new BattleUnit( // create a new Unit, not-soldier but Vehicle, Civie, or aLien.
								rule->getUnit(type),
								faction,
								id,
								rule->getArmor(armor),
								static_cast<int>(savedGame->getDifficulty()));
		}
		//Log(LOG_INFO) << "SavedGame::load(), difficulty = " << savedGame->getDifficulty();

		unit->load(*i);
		_units.push_back(unit);

		if (faction == FACTION_PLAYER)
		{
			if (unit->getId() == selectedUnit
				|| (_selectedUnit == 0
					&& !unit->isOut()))
			{
				_selectedUnit = unit;
			}

			// silly hack to fix mind controlled aliens
			// TODO: save stats instead? maybe some kind of weapon will affect them at some point.
			if (unit->getOriginalFaction() == FACTION_HOSTILE)
				unit->adjustStats(savedGame->getDifficulty());
		}

		if (unit->getStatus() != STATUS_DEAD)
		{
			if (const YAML::Node& ai = (*i)["AI"])
			{
				BattleAIState* aiState;

				if (faction == FACTION_NEUTRAL)
					aiState = new CivilianBAIState(
												this,
												unit,
												0);
				else if (faction == FACTION_HOSTILE)
					aiState = new AlienBAIState(
											this,
											unit,
											0);
				else
					continue;

				aiState->load(ai);
				unit->setAIState(aiState);
			}
		}
	}

	// matches up tiles and units
	resetUnitTiles();

	for (YAML::const_iterator
			i = node["items"].begin();
			i != node["items"].end();
			++i)
	{
		_itemId = (*i)["id"].as<int>(_itemId);

		std::string type = (*i)["type"].as<std::string>();
		if (rule->getItem(type))
		{
			BattleItem* item = new BattleItem(
											rule->getItem(type),
											&_itemId);
			item->load(*i);
			type = (*i)["inventoryslot"].as<std::string>();

			if (type != "NULL")
				item->setSlot(rule->getInventory(type));

			int owner = (*i)["owner"].as<int>();
			int unit = (*i)["unit"].as<int>();

			// match up items and units
			for (std::vector<BattleUnit*>::iterator
					bu = _units.begin();
					bu != _units.end();
					++bu)
			{
				if ((*bu)->getId() == owner)
					item->moveToOwner(*bu);

				if ((*bu)->getId() == unit)
					item->setUnit(*bu);
			}

			// match up items and tiles
			if (item->getSlot()
				&& item->getSlot()->getType() == INV_GROUND)
			{
				Position pos = (*i)["position"].as<Position>();

				if (pos.x != -1)
					getTile(pos)->addItem(item, rule->getInventory("STR_GROUND"));
			}

			_items.push_back(item);
		}
	}

	// tie ammo items to their weapons, running through the items again
	std::vector<BattleItem*>::iterator weaponi = _items.begin();
	for (YAML::const_iterator
			i = node["items"].begin();
			i != node["items"].end();
			++i)
	{
		if (rule->getItem((*i)["type"].as<std::string>()))
		{
			int ammo = (*i)["ammoItem"].as<int>();
			if (ammo != -1)
			{
				for (std::vector<BattleItem*>::iterator
						ammoi = _items.begin();
						ammoi != _items.end();
						++ammoi)
				{
					if ((*ammoi)->getId() == ammo)
					{
						(*weaponi)->setAmmoItem((*ammoi));

						break;
					}
				}
			}

			++weaponi;
		}
	}

	_objectiveDestroyed	= node["objectiveDestroyed"].as<bool>(_objectiveDestroyed);
	_tuReserved			= (BattleActionType)node["tuReserved"].as<int>(_tuReserved);
	_kneelReserved		= node["kneelReserved"].as<bool>(_kneelReserved);
}

/**
 * Loads the resources required by the map in the battle save.
 * @param game, Pointer to the game.
 */
void SavedBattleGame::loadMapResources(Game* game)
{
	ResourcePack* res = game->getResourcePack();

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		(*i)->loadData();

		if (game->getRuleset()->getMCDPatch((*i)->getName()))
			game->getRuleset()->getMCDPatch((*i)->getName())->modifyData(*i);
	}

	int
		mdID,
		mdsID;

	for (int
			i = 0;
			i < _mapsize_z * _mapsize_y * _mapsize_x;
			++i)
	{
		for (int
				part = 0;
				part < 4;
				part++)
		{
			_tiles[i]->getMapData(
								&mdID,
								&mdsID,
								part);

			if (mdID != -1
				&& mdsID != -1)
			{
				_tiles[i]->setMapData(
								_mapDataSets[mdsID]->getObjects()->at(mdID),
								mdID,
								mdsID,
								part);
			}
		}
	}

	initUtilities(res);

	getTileEngine()->calculateSunShading();
	getTileEngine()->calculateTerrainLighting();
	getTileEngine()->calculateUnitLighting();
	getTileEngine()->recalculateFOV();
}

/**
 * Saves the saved battle game to a YAML file.
 * @return YAML node.
 */
YAML::Node SavedBattleGame::save() const
{
	YAML::Node node;

	if (_objectiveDestroyed)
		node["objectiveDestroyed"] = _objectiveDestroyed;

	node["width"]			= _mapsize_x;
	node["length"]			= _mapsize_y;
	node["height"]			= _mapsize_z;
	node["missionType"]		= _missionType;
	node["globalshade"]		= _globalShade;
	node["turn"]			= _turn;
	node["terrain"]			= _terrain; // kL sza_MusicRules
	node["selectedUnit"]	= (_selectedUnit? _selectedUnit->getId(): -1);

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		node["mapdatasets"].push_back((*i)->getName());
	}

#if 0
	for (int i = 0; i < _mapsize_z * _mapsize_y * _mapsize_x; ++i)
	{
		if (!_tiles[i]->isVoid())
		{
			node["tiles"].push_back(_tiles[i]->save());
		}
	}
#else
	// first, write out the field sizes we're going to use to write the tile data
	node["tileIndexSize"]		= Tile::serializationKey.index;
	node["tileTotalBytesPer"]	= Tile::serializationKey.totalBytes;
	node["tileFireSize"]		= Tile::serializationKey._fire;
	node["tileSmokeSize"]		= Tile::serializationKey._smoke;
	node["tileOffsetSize"]		= Tile::serializationKey._animOffset; // kL
	node["tileIDSize"]			= Tile::serializationKey._mapDataID;
	node["tileSetIDSize"]		= Tile::serializationKey._mapDataSetID;
    node["tileBoolFieldsSize"]	= Tile::serializationKey.boolFields;

	size_t tileDataSize = Tile::serializationKey.totalBytes * _mapsize_z * _mapsize_y * _mapsize_x;
	Uint8* tileData = (Uint8*) calloc(tileDataSize, 1);
	Uint8* w = tileData;

	for (int
			i = 0;
			i < _mapsize_z * _mapsize_y * _mapsize_x;
			++i)
	{
		if (!_tiles[i]->isVoid())
		{
			serializeInt(&w, Tile::serializationKey.index, i);
			_tiles[i]->saveBinary(&w);
		}
		else
			tileDataSize -= Tile::serializationKey.totalBytes;
	}

	node["totalTiles"]	= tileDataSize / Tile::serializationKey.totalBytes; // not strictly necessary, just convenient
	node["binTiles"]	= YAML::Binary(tileData, tileDataSize);

	free(tileData);
#endif

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		node["nodes"].push_back((*i)->save());
	}

	if (_missionType == "STR_BASE_DEFENSE")
		node["moduleMap"] = _baseModules;

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		node["units"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		node["items"].push_back((*i)->save());
	}

	node["tuReserved"]		= static_cast<int>(_tuReserved);
    node["kneelReserved"]	= _kneelReserved;

	return node;
}

/**
 * Gets the array of tiles.
 * @return A pointer to the Tile array.
 */
Tile** SavedBattleGame::getTiles() const
{
	return _tiles;
}

/**
 * Initializes the array of tiles and creates a pathfinding object.
 * @param mapsize_x
 * @param mapsize_y
 * @param mapsize_z
 */
void SavedBattleGame::initMap(
		int mapsize_x,
		int mapsize_y,
		int mapsize_z)
{
	if (!_nodes.empty())
	{
		for (int
				i = 0;
				i < _mapsize_z * _mapsize_y * _mapsize_x;
				++i)
		{
			delete _tiles[i];
		}

		delete[] _tiles;

		for (std::vector<Node*>::iterator
				i = _nodes.begin();
				i != _nodes.end();
				++i)
		{
			delete *i;
		}

		_nodes.clear();
		_mapDataSets.clear();
	}

	/* create tile objects */
	_mapsize_x = mapsize_x;
	_mapsize_y = mapsize_y;
	_mapsize_z = mapsize_z;
	_tiles = new Tile*[_mapsize_z * _mapsize_y * _mapsize_x];

	for (int
			i = 0;
			i < _mapsize_z * _mapsize_y * _mapsize_x;
			++i)
	{
		Position pos;
		getTileCoords(
					i,
					&pos.x,
					&pos.y,
					&pos.z);

		_tiles[i] = new Tile(pos);
	}
}

/**
 * Initializes the map utilities.
 * @param res Pointer to resource pack.
 */
void SavedBattleGame::initUtilities(ResourcePack* res)
{
	_pathfinding	= new Pathfinding(this);
	_tileEngine		= new TileEngine(
								this,
								res->getVoxelData());
}

/**
 * Sets the mission type.
 * @param missionType The mission type.
 */
void SavedBattleGame::setMissionType(const std::string& missionType)
{
	_missionType = missionType;
}

/**
 * Gets the mission type.
 * @return The mission type.
 */
std::string SavedBattleGame::getMissionType() const
{
	return _missionType;
}

/**
 * Sets the global shade.
 * @param shade The global shade.
 */
void SavedBattleGame::setGlobalShade(int shade)
{
	_globalShade = shade;
}

/**
 * Gets the global shade.
 * @return The global shade.
 */
int SavedBattleGame::getGlobalShade() const
{
	return _globalShade;
}

/**
 * Gets the map width.
 * @return The map width (Size X) in tiles.
 */
int SavedBattleGame::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map length.
 * @return The map length (Size Y) in tiles.
 */
int SavedBattleGame::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map height.
 * @return The map height (Size Z) in layers.
 */
int SavedBattleGame::getMapSizeZ() const
{
	return _mapsize_z;
}

/**
 * Gets the map size in tiles.
 * @return The map size.
 */
int SavedBattleGame::getMapSizeXYZ() const
{
	return _mapsize_x * _mapsize_y * _mapsize_z;
}

/**
 *
 */
void SavedBattleGame::setTerrain(std::string terrain) // sza_MusicRules
{
	_terrain = terrain;
}

/**
 *
 */
std::string SavedBattleGame::getTerrain() const // sza_MusicRules
{
	return _terrain;
}

/**
 * Converts a tile index to coordinates.
 * @param index The (unique) tileindex.
 * @param x Pointer to the X coordinate.
 * @param y Pointer to the Y coordinate.
 * @param z Pointer to the Z coordinate.
 */
void SavedBattleGame::getTileCoords(
		int index,
		int* x,
		int* y,
		int* z) const
{
	*z = index / (_mapsize_y * _mapsize_x);
	*y = (index %(_mapsize_y * _mapsize_x)) / _mapsize_x;
	*x = (index %(_mapsize_y * _mapsize_x)) %_mapsize_x;
}

/**
 * Gets the currently selected unit
 * @return Pointer to BattleUnit.
 */
BattleUnit* SavedBattleGame::getSelectedUnit() const
{
	return _selectedUnit;
}

/**
 * Sets the currently selected unit.
 * @param unit Pointer to BattleUnit.
 */
void SavedBattleGame::setSelectedUnit(BattleUnit* unit)
{
	_selectedUnit = unit;
}

/**
* Selects the previous player unit.
* @param checkReselect Whether to check if we should reselect a unit.
* @param setDontReselect Don't reselect a unit.
* @param checkInventory Whether to check if the unit has an inventory.
* @return Pointer to new selected BattleUnit, NULL if none can be selected.
* @sa selectFactionUnit
*/
BattleUnit* SavedBattleGame::selectPreviousFactionUnit(
		bool checkReselect,
		bool setDontReselect,
		bool checkInventory)
{
	return selectFactionUnit(
						-1,
						checkReselect,
						setDontReselect,
						checkInventory);
}

/**
 * Selects the next player unit.
 * @param checkReselect Whether to check if we should reselect a unit.
 * @param setDontReselect Don't reselect a unit.
 * @param checkInventory Whether to check if the unit has an inventory.
 * @return Pointer to new selected BattleUnit, NULL if none can be selected.
 * @sa selectFactionUnit
 */
BattleUnit* SavedBattleGame::selectNextFactionUnit(
		bool checkReselect,
		bool setDontReselect,
		bool checkInventory)
{
	return selectFactionUnit(
						+1,
						checkReselect,
						setDontReselect,
						checkInventory);
}

/**
 * Selects the next player unit in a certain direction.
 * @param dir Direction to select, eg. -1 for previous and 1 for next.
 * @param checkReselect Whether to check if we should reselect a unit.
 * @param setDontReselect Don't reselect a unit.
 * @param checkInventory Whether to check if the unit has an inventory.
 * @return, Pointer to new selected BattleUnit, NULL if none can be selected.
 */
BattleUnit* SavedBattleGame::selectFactionUnit(
		int dir,
		bool checkReselect,
		bool setDontReselect,
		bool checkInventory)
{
	//Log(LOG_INFO) << "SavedBattleGame::selectFactionUnit()";
	if (_selectedUnit != 0
		&& setDontReselect)
	{
		//Log(LOG_INFO) << ". dontReselect";
		_selectedUnit->dontReselect();
	}

	if (_units.empty())
	{
		//Log(LOG_INFO) << ". units.Empty, ret 0";
		_selectedUnit = 0;		// kL
		_lastSelectedUnit = 0;	// kL

		return 0;
	}

	std::vector<BattleUnit*>::iterator
		begin,
		end;

	if (dir > 0)
	{
		begin = _units.begin();
		end = _units.end() - 1;
	}
	else //if (dir < 1)
	{
		begin = _units.end() - 1;
		end = _units.begin();
	}


	std::vector<BattleUnit*>::iterator i = std::find(
			_units.begin(),
			_units.end(),
			_selectedUnit);

	do
	{
		//Log(LOG_INFO) << ". do";
		if (i == _units.end()) // no unit selected
		{
			//Log(LOG_INFO) << ". . i = begin, continue";
			i = begin;

			continue;
		}

		if (i != end)
		{
			//Log(LOG_INFO) << ". . i += dir";
			i += dir;
		}
		else // reached the end, wrap-around
		{
			//Log(LOG_INFO) << ". . i = begin";
			i = begin;
		}

		// back to where we started... no more units found
		if (*i == _selectedUnit)
		{
			//Log(LOG_INFO) << ". . *i == _selectedUnit, found one, test";
			if (checkReselect
				&& !_selectedUnit->reselectAllowed())
			{
				//Log(LOG_INFO) << ". . . negative, ret 0";
				_selectedUnit = 0;
			}

			//Log(LOG_INFO) << ". . ret selectedUnit";
			return _selectedUnit;
		}
		else if (_selectedUnit == 0
			&& i == begin)
		{
			//Log(LOG_INFO) << ". . finish do, ret selectedUnit = 0";
			return _selectedUnit;
		}
	}
	while (!(*i)->isSelectable(
							_side,
							checkReselect,
							checkInventory));


	//Log(LOG_INFO) << ". fallthrough, ret selectedUnit = *i";
	_selectedUnit = *i;

	return _selectedUnit;
}

/**
 * Selects the unit at the given position on the map.
 * @param pos Position.
 * @return Pointer to a BattleUnit, or 0 when none is found.
 */
BattleUnit* SavedBattleGame::selectUnit(const Position& pos)
{
	BattleUnit* bu = getTile(pos)->getUnit();

	if (bu
		&& bu->isOut())
	{
		return 0;
	}
	else
		return bu;
}

/**
 * Gets the list of nodes.
 * @return Pointer to the list of nodes.
 */
std::vector<Node*>* SavedBattleGame::getNodes()
{
	return &_nodes;
}

/**
 * Gets the list of units.
 * @return Pointer to the list of units.
 */
std::vector<BattleUnit*>* SavedBattleGame::getUnits()
{
	return &_units;
}

/**
 * Gets the list of items.
 * @return Pointer to the list of items.
 */
std::vector<BattleItem*>* SavedBattleGame::getItems()
{
	return &_items;
}

/**
 * Gets the pathfinding object.
 * @return Pointer to the pathfinding object.
 */
Pathfinding* SavedBattleGame::getPathfinding() const
{
	return _pathfinding;
}

/**
 * Gets the terrain modifier object.
 * @return Pointer to the terrain modifier object.
 */
TileEngine* SavedBattleGame::getTileEngine() const
{
	return _tileEngine;
}

/**
* Gets the array of mapblocks.
* @return Pointer to the array of mapblocks.
*/
std::vector<MapDataSet*>* SavedBattleGame::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Gets the side currently playing.
 * @return The unit faction currently playing.
 */
UnitFaction SavedBattleGame::getSide() const
{
	return _side;
}

/**
 * Gets the current turn number.
 * @return, The current turn.
 */
int SavedBattleGame::getTurn() const
{
	//Log(LOG_INFO) << ". getTurn()"; // kL
	return _turn;
}

/**
 * Ends the current turn and progresses to the next one.
 */
void SavedBattleGame::endTurn()
{
	//Log(LOG_INFO) << "SavedBattleGame::endTurn()";

	if (_side == FACTION_PLAYER) // end of Xcom turn.
	{
		//Log(LOG_INFO) << ". end Faction_Player";

		if (_selectedUnit
			&& _selectedUnit->getOriginalFaction() == FACTION_PLAYER)
		{
			_lastSelectedUnit = _selectedUnit;
		}

		_side = FACTION_HOSTILE;
		_selectedUnit = 0;

		// kL_begin: sbg::endTurn() no Reselect xCom units at endTurn!!!
		for (std::vector<BattleUnit*>::iterator
				i = getUnits()->begin();
				i != getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == FACTION_PLAYER)
				(*i)->dontReselect(); // either zero tu's or set no reselect
		} // kL_end.
	}
	else if (_side == FACTION_HOSTILE) // end of Alien turn.
	{
		//Log(LOG_INFO) << ". end Faction_Hostile";

		_side = FACTION_NEUTRAL;

		// if there is no neutral team, we skip this section
		// and instantly prepare the new turn for the player.
		if (selectNextFactionUnit() == 0) // this will now cycle through NEUTRAL units
			// so shouldn't that really be 'selectNextFactionUnit()'???!
			// see selectFactionUnit() -> isSelectable()
		{
			//Log(LOG_INFO) << ". . nextFactionUnit == 0";

			prepareNewTurn();
			//Log(LOG_INFO) << ". . prepareNewTurn DONE";
			_turn++;

			_side = FACTION_PLAYER;

			if (_lastSelectedUnit
				&& _lastSelectedUnit->isSelectable(FACTION_PLAYER))
			{
				//Log(LOG_INFO) << ". . . lastSelectedUnit is aLive";
				_selectedUnit = _lastSelectedUnit;
			}
			else
				//Log(LOG_INFO) << ". . . select nextFactionUnit";
				selectNextFactionUnit();

			while (_selectedUnit
				&& _selectedUnit->getFaction() != FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". . . finding a Unit to select";
//kL				selectNextFactionUnit();
				selectNextFactionUnit(true); // kL
			}
		}
	}
	else if (_side == FACTION_NEUTRAL) // end of Civilian turn.
	{
		//Log(LOG_INFO) << ". end Faction_Neutral";

		prepareNewTurn();
		_turn++;

		_side = FACTION_PLAYER;

		if (_lastSelectedUnit
			&& _lastSelectedUnit->isSelectable(FACTION_PLAYER))
		{
			//Log(LOG_INFO) << ". . . lastSelectedUnit is aLive";
			_selectedUnit = _lastSelectedUnit;
		}
		else
			//Log(LOG_INFO) << ". . . select nextFactionUnit";
			selectNextFactionUnit();

		while (_selectedUnit
			&& _selectedUnit->getFaction() != FACTION_PLAYER)
		{
			//Log(LOG_INFO) << ". . . finding a Unit to select";
//kL			selectNextFactionUnit();
			selectNextFactionUnit(true); // kL
		}
	}
	//Log(LOG_INFO) << "done Factions";



	// ** _side HAS ADVANCED to next faction after here!!! ** //


	int
		liveAliens,
		liveSoldiers;
	_battleState->getBattleGame()->tallyUnits(
											liveAliens,
											liveSoldiers,
											false);
	//Log(LOG_INFO) << "done tallyUnits";

	// kL_begin: pseudo the Turn20 reveal and the less than 3 aliens left rule.
	if (_side == FACTION_HOSTILE)
	{
		int rand = RNG::generate(0, 5);
		if (_turn > 17 + rand
			|| liveAliens < rand - 1)
		{
			_cheating = true;
		}
		//Log(LOG_INFO) << "done custom cheating";
	}

	for (std::vector<BattleUnit*>::iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getFaction() == _side)	// this causes an Mc'd unit to lose its turn.
			(*i)->prepareNewTurn();			// reverts faction, tu/stun recovery, Fire damage, etc

		if ((*i)->getFaction() == FACTION_PLAYER) // including units Mc'd by xCom
		{
			if ((*i)->isOut(true, true))
				(*i)->setTurnsExposed(255);
			else if (_cheating
				&& _side == FACTION_HOSTILE)
			{
				(*i)->setTurnsExposed(0); // they see you.
			}
			else if ((*i)->getTurnsExposed() < 255
				&& _side == FACTION_PLAYER)
			{
				(*i)->setTurnsExposed((*i)->getTurnsExposed() + 1);
			}
		}
		else
		{
			(*i)->setVisible(false);
//			(*i)->convertToFaction((*i)->getOriginalFaction());

			if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
				(*i)->setTurnsExposed(0);	// aLiens always know where their buddies are,
												// Mc'd or not.
		}

//		if ((*i)->getFaction() == _side)
//			(*i)->prepareNewTurn();
	}
	//Log(LOG_INFO) << "done looping units";

	// redo calculateFOV() *after* aliens & civies have been set notVisible
	_tileEngine->recalculateFOV();
	//Log(LOG_INFO) << "done recalculateFoV";

	if (_side != FACTION_PLAYER)
		selectNextFactionUnit();

	//Log(LOG_INFO) << "SavedBattleGame::endTurn() EXIT";
}

/**
 * Turns on debug mode.
 */
void SavedBattleGame::setDebugMode()
{
	for (int
			i = 0;
			i < _mapsize_z * _mapsize_y * _mapsize_x;
			++i)
	{
		_tiles[i]->setDiscovered(true, 2);
	}

	_debugMode = true;
}

/**
 * Gets the current debug mode.
 * @return Debug mode.
 */
bool SavedBattleGame::getDebugMode() const
{
	return _debugMode;
}

/**
 * Gets the BattlescapeState.
 * @return Pointer to the BattlescapeState.
 */
BattlescapeState* SavedBattleGame::getBattleState()
{
	return _battleState;
}

/**
 * Gets the BattlescapeState.
 * @return Pointer to the BattlescapeState.
 */
BattlescapeGame* SavedBattleGame::getBattleGame()
{
	return _battleState->getBattleGame();
}

/**
 * Sets the BattlescapeState.
 * @param bs A Pointer to a BattlescapeState.
 */
void SavedBattleGame::setBattleState(BattlescapeState* bs)
{
	_battleState = bs;
}

/**
 * Resets all the units to their current standing tile(s).
 */
void SavedBattleGame::resetUnitTiles()
{
	for (std::vector<BattleUnit*>::iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if (!(*i)->isOut())
		{
			int size = (*i)->getArmor()->getSize() - 1;

			if ((*i)->getTile()
				&& (*i)->getTile()->getUnit() == *i)
			{
				for (int
						x = size;
						x >= 0;
						x--)
					for (int
							y = size;
							y >= 0;
							y--)
						getTile((*i)->getTile()->getPosition() + Position(x, y, 0))->setUnit(0);
			}

			for (int
					x = size;
					x >= 0;
					x--)
				for (int
						y = size;
						y >= 0;
						y--)
				{
					Tile* t = getTile((*i)->getPosition() + Position(x, y, 0));
					t->setUnit(
							*i,
							getTile(t->getPosition() + Position(0, 0, -1)));
				}
		}

		if ((*i)->getFaction() == FACTION_PLAYER)
			(*i)->setVisible(true);
	}
}

/**
 * Gives access to the "storage space" vector, for distribution of items in base defense missions.
 */
std::vector<Position>& SavedBattleGame::getStorageSpace()
{
	return _storageSpace;
}

/**
 * Move all the leftover items in base defense missions to random locations in the storage facilities.
 * @param t, The tile where all the goodies are initially stored.
 */
void SavedBattleGame::randomizeItemLocations(Tile* t)
{
	if (!_storageSpace.empty())
	{
		for (std::vector<BattleItem*>::iterator
				it = t->getInventory()->begin();
				it != t->getInventory()->end();
				)
		{
			if ((*it)->getSlot()->getId() == "STR_GROUND")
			{
				getTile(_storageSpace.at(RNG::generate(0, _storageSpace.size() - 1)))
						->addItem(
								*it,
								(*it)->getSlot());

				it = t->getInventory()->erase(it);
			}
			else
				++it;
		}
	}
}

/**
 * Removes an item from the game. Eg. when ammo item is depleted.
 * @param item, The Item to remove.
 */
void SavedBattleGame::removeItem(BattleItem* item)
{
	//Log(LOG_INFO) << "SavedBattleGame::removeItem()";

	// due to strange design, the item has to be removed from the tile it is on too (if it is on a tile)
	BattleUnit* b = item->getOwner();
	Tile* t = item->getTile();
	if (t)
	{
		for (std::vector<BattleItem*>::iterator
				it = t->getInventory()->begin();
				it != t->getInventory()->end();
				++it)
		{
			if (*it == item)
			{
				t->getInventory()->erase(it);

				break;
			}
		}
	}

	if (b)
	{
		for (std::vector<BattleItem*>::iterator
				it = b->getInventory()->begin();
				it != b->getInventory()->end();
				++it)
		{
			if (*it == item)
			{
				b->getInventory()->erase(it);

				break;
			}
		}
	}

	for (std::vector<BattleItem*>::iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if (*i == item)
		{
			_items.erase(i);

			//Log(LOG_INFO) << "SavedBattleGame::removeItem() [1]EXIT";
			return;
		}
	}

	_deleted.push_back(item);

/*	for (int i = 0; i < _mapsize_x * _mapsize_y * _mapsize_z; ++i)
	{
		for (std::vector<BattleItem*>::iterator it = _tiles[i]->getInventory()->begin(); it != _tiles[i]->getInventory()->end(); )
		{
			if ((*it) == item)
			{
				it = _tiles[i]->getInventory()->erase(it);
				return;
			}
			++it;
		}
	} */
	//Log(LOG_INFO) << "SavedBattleGame::removeItem() [2]EXIT";
}

/**
 * Sets whether the mission was aborted or successful.
 * @param flag, True if the mission was aborted, or false, if the mission was successful.
 */
void SavedBattleGame::setAborted(bool flag)
{
	_aborted = flag;
}

/**
 * Returns whether the mission was aborted or successful.
 * @return, True if the mission was aborted, or false, if the mission was successful.
 */
bool SavedBattleGame::isAborted() const
{
	return _aborted;
}

/**
 * Sets whether the objective is destroyed.
 * @param flag, True if the objective is destroyed.
 */
void SavedBattleGame::setObjectiveDestroyed(bool flag)
{
	_objectiveDestroyed = flag;

	if (flag
		&& Options::battleAutoEnd)
	{
		setSelectedUnit(0);
		_battleState->getBattleGame()->cancelCurrentAction(true);

		_battleState->getBattleGame()->requestEndTurn();
	}
}

/**
 * Returns whether the objective is detroyed.
 * @return, True if the objective is destroyed.
 */
bool SavedBattleGame::isObjectiveDestroyed()
{
	return _objectiveDestroyed;
}

/**
 * Gets the current item ID.
 * @return, Current item ID pointer.
 */
int* SavedBattleGame::getCurrentItemId()
{
	return &_itemId;
}

/**
 * Finds a fitting node where a unit can spawn.
 * @param nodeRank, Rank of the node (this is not the rank of the alien!)
 * kL_note: actually, it pretty much is the rank of the aLien.
 * @param unit, Pointer to the unit (to test-set its position)
 * @return, Pointer to the chosen node
 */
Node* SavedBattleGame::getSpawnNode(
		int nodeRank,
		BattleUnit* unit)
{
	std::vector<Node*> legitNodes;
	int priority = -1;

	for (std::vector<Node*>::iterator
			i = getNodes()->begin();
			i != getNodes()->end();
			++i)
	{
		if ((*i)->getRank() == nodeRank								// ranks must match
			&& (!((*i)->getType() & Node::TYPE_SMALL)				// the small unit bit is not set
				|| unit->getArmor()->getSize() == 1)					// or the unit is small
			&& (!((*i)->getType() & Node::TYPE_FLYING)				// the flying unit bit is not set
				|| unit->getArmor()->getMovementType() == MT_FLY)		// or the unit can fly
			&& (*i)->getPriority() > 0								// priority 0 is not spawnplace
			&& setUnitPosition(										// check if unit can be set at this node
							unit,										// ie. it's big enough
							(*i)->getPosition(),						// and there's not already a unit there.
							true))										// runs w/ false on return to bgen::addAlien()
		{
			if ((*i)->getPriority() > priority) // hold it. This does not *weight* the nodes by priority. but so waht
			{
				priority = (*i)->getPriority();
				legitNodes.clear(); // drop the last nodes, as we found a higher priority now
			}

			if ((*i)->getPriority() == priority)
				legitNodes.push_back((*i));
		}
	}

	if (legitNodes.empty())
		return 0;

	int node = RNG::generate(
						0,
						static_cast<int>(legitNodes.size()) - 1);

	return legitNodes[node];
}

/**
 * Finds a fitting node where a unit can patrol to.
 * @param nodeRank Rank of the node (this is not the rank of the alien!).
 * @param unit Pointer to the unit (to get its position).
 * @return Pointer to the choosen node.
 */
Node* SavedBattleGame::getPatrolNode(
		bool scout,
		BattleUnit* unit,
		Node* fromNode)
{
	//Log(LOG_INFO) << "SavedBattleGame::getPatrolNode()";

	if (fromNode == 0)
	{
		if (Options::traceAI) Log(LOG_INFO) << "This alien got lost. :(";

		fromNode = getNodes()->at(RNG::generate(0, static_cast<int>(getNodes()->size()) - 1));
	}


	std::vector<Node*> legitNodes;
	Node* bestNode = 0;
	Node* node = 0;

	// scouts roam all over while all others shuffle around to adjacent nodes at most:
//kL	int const linksEnd = scout? getNodes()->size(): fromNode->getNodeLinks()->size();
	int linksEnd = fromNode->getNodeLinks()->size();
	if (scout)
		linksEnd = getNodes()->size();

	for (int
			i = 0;
			i < linksEnd;
			++i)
	{
		if (!scout
			&& fromNode->getNodeLinks()->at(i) < 1)
		{
			continue; // nothing to patrol to
		}

//kL		node = getNodes()->at(scout? i: fromNode->getNodeLinks()->at(i));
		if (scout)
			node = getNodes()->at(i);
		else
			node = getNodes()->at(fromNode->getNodeLinks()->at(i));

		if ((node->getFlags() > 0
				|| node->getRank() > 0
				|| scout)											// for non-scouts we find a node with a desirability above 0
			&& (!(node->getType() & Node::TYPE_SMALL)				// the small unit bit is not set
				|| unit->getArmor()->getSize() == 1)					// or the unit is small
			&& (!(node->getType() & Node::TYPE_FLYING)				// the flying unit bit is not set
				|| unit->getArmor()->getMovementType() == MT_FLY)		// or the unit can fly
			&& !node->isAllocated()									// check if not allocated
			&& !(node->getType() & Node::TYPE_DANGEROUS)			// don't go there if an alien got shot there; stupid behavior like that
			&& setUnitPosition(										// check if unit can be set at this node
							unit,										// ie. it's big enough
							node->getPosition(),						// and there's not already a unit there.
							true)										// but don't actually set the unit...
			&& getTile(node->getPosition())							// the node is on a tile
			&& !getTile(node->getPosition())->getFire()				// you are not a firefighter; do not patrol into fire
			&& (!getTile(node->getPosition())->getDangerous()		// aliens don't run into a grenade blast
				|| unit->getFaction() != FACTION_HOSTILE)				// but civies do!
			&& (node != fromNode									// scouts push forward
				|| !scout)												// others can mill around.. ie, stand there
			&& node->getPosition().x > 0							// x-pos valid
			&& node->getPosition().y > 0)							// y-pos valid
		{
			if (!bestNode
//kL				|| (bestNode->getRank() == Node::nodeRank[unit->getRankInt()][0]
//kL					&& bestNode->getFlags() < node->getFlags())
//kL				|| bestNode->getFlags() < node->getFlags())
				|| bestNode->getFlags() > node->getFlags()) // <- !!!!! NODES HAVE FLAGS !!!!! ->
			{
				bestNode = node; // for non-scouts
			}

			legitNodes.push_back(node); // for scouts
		}
	}

	if (legitNodes.empty())
	{
		//Log(LOG_INFO) << " . legitNodes is EMPTY.";

		if (Options::traceAI) Log(LOG_INFO) << (scout? "Scout ": "Guard ") << "found no patrol node! XXX XXX XXX";

		if (unit->getArmor()->getSize() > 1
			&& !scout)
		{
//			return Sectopod::CTD();
			return getPatrolNode(
								true,
								unit,
								fromNode); // move damnit
		}
		else
			//Log(LOG_INFO) << " . legitNodes is NOT Empty.";
			//Log(LOG_INFO) << " . return 0";
			return 0;
	}

	if (scout) // picks a random destination
	{
		//Log(LOG_INFO) << " . scout";

//kL		return legitNodes[RNG::generate(0, static_cast<int>(legitNodes.size()) - 1)];
		size_t legit = static_cast<size_t>(RNG::generate(0, static_cast<int>(legitNodes.size()) - 1));

		//Log(LOG_INFO) << " . return legitNodes @ " << legit;
		return legitNodes[legit];
	}
	else
	{
		//Log(LOG_INFO) << " . !scout";
		if (!bestNode)
			//Log(LOG_INFO) << " . no bestNode, return 0";
			return 0;

		// non-scout patrols to highest value unoccupied node that's not fromNode
		if (Options::traceAI) Log(LOG_INFO) << "Choosing node flagged " << bestNode->getFlags();

		//Log(LOG_INFO) << " . return bestNode";
		return bestNode;
	}

	//Log(LOG_INFO) << "SavedBattleGame::getPatrolNode() EXIT";
}

/**
 * Carries out new turn preparations such as fire and smoke spreading.
 */
void SavedBattleGame::prepareNewTurn()
{
	//Log(LOG_INFO) << "SavedBattleGame::prepareNewTurn()";
	std::vector<Tile*> tilesOnFire;
	std::vector<Tile*> tilesOnSmoke;

	for (int
			i = 0;
			i < _mapsize_x * _mapsize_y * _mapsize_z;
			++i) // prepare a list of tiles on fire
	{
		if (getTiles()[i]->getFire() > 0)
			tilesOnFire.push_back(getTiles()[i]);
	}

	for (std::vector<Tile*>::iterator
			i = tilesOnFire.begin();
			i != tilesOnFire.end();
			++i) // first: fires spread
	{
		if ((*i)->getOverlaps() == 0) // if we haven't added fire here this turn
		{
			(*i)->setFire((*i)->getFire() - 1); // reduce the fire timer

			if ((*i)->getFire()) // if we're still burning
			{
				for (int
						dir = 0;
						dir < 7;
						dir += 2) // propagate in four cardinal directions (0, 2, 4, 6)
				{
					Position pos;
					Pathfinding::directionToVector(dir, &pos);
					Tile* tile = getTile((*i)->getPosition() + pos);

					if (tile
						&& getTileEngine()->horizontalBlockage(
															*i,
															tile,
															DT_IN)
														== 0) // if there's no wall blocking the path of the flames...
					{
						tile->ignite((*i)->getSmoke()); // attempt to set this tile on fire
					}
				}
			}
			else // fire has burnt out
			{
				(*i)->setSmoke(0);

				if ((*i)->getMapData(MapData::O_OBJECT)) // burn this tile, and any object in it, if it's not fireproof/indestructible.
				{
					if ((*i)->getMapData(MapData::O_OBJECT)->getFlammable() != 255
						&& (*i)->getMapData(MapData::O_OBJECT)->getArmor() != 255)
					{
						if ((*i)->destroy(MapData::O_OBJECT))
							_objectiveDestroyed = true;

						if ((*i)->destroy(MapData::O_FLOOR))
							_objectiveDestroyed = true;
					}
				}
				else if ((*i)->getMapData(MapData::O_FLOOR))
				{
					if ((*i)->getMapData(MapData::O_FLOOR)->getFlammable() != 255
						&& (*i)->getMapData(MapData::O_FLOOR)->getArmor() != 255)
					{
						if ((*i)->destroy(MapData::O_FLOOR))
							_objectiveDestroyed = true;
					}
				}
			}
		}
	}

	for (int // prepare a list of tiles on fire/with smoke in them (smoke acts as fire intensity)
			i = 0;
			i < _mapsize_x * _mapsize_y * _mapsize_z;
			++i)
	{
		if (getTiles()[i]->getSmoke() > 0)
			tilesOnSmoke.push_back(getTiles()[i]);
	}

	for (std::vector<Tile*>::iterator // now make the smoke spread.
			i = tilesOnSmoke.begin();
			i != tilesOnSmoke.end();
			++i)
	{
		if ((*i)->getFire() == 0) // smoke and fire follow slightly different rules.
		{
			(*i)->setSmoke((*i)->getSmoke() - 1); // reduce the smoke counter

			if ((*i)->getSmoke()) // if we're still smoking
			{
				for (int // spread in four cardinal directions
						dir = 0;
						dir < 7;
						dir += 2)
				{
					Position pos;
					Pathfinding::directionToVector(dir, &pos);
					Tile* tile = getTile((*i)->getPosition() + pos);
					if (tile
						&& getTileEngine()->horizontalBlockage( // as long as there are no blocking walls
															*i,
															tile,
															DT_SMOKE)
														== 0)
					{
						if (tile->getSmoke() == 0				// add smoke only to smokeless tiles,
							|| (tile->getFire() == 0			// or tiles with no fire
								&& tile->getOverlaps() != 0))	// and no smoke that was added this turn
						{
							tile->addSmoke((*i)->getSmoke());
						}
					}
				}
			}
		}
		else
		{
			Position pos = Position(0, 0, 1); // smoke from fire spreads upwards one level if there's no floor blocking it.
			Tile* tile = getTile((*i)->getPosition() + pos);
			if (tile
				&& tile->hasNoFloor(*i))
			{
				tile->addSmoke((*i)->getSmoke() / 2); // only add smoke equal to half the intensity of the fire
			}

			for (int
					dir = 0;
					dir < 7;
					dir += 2) // then it spreads in the four cardinal directions.
			{
				Pathfinding::directionToVector(dir, &pos);
				tile = getTile((*i)->getPosition() + pos);
				if (tile
					&& getTileEngine()->horizontalBlockage(
														*i,
														tile,
														DT_SMOKE)
													== 0)
				{
					tile->addSmoke((*i)->getSmoke() / 2);
				}
			}
		}
	}

	if (!tilesOnFire.empty()
		|| !tilesOnSmoke.empty())
	{
		for (int // do damage to units, average out the smoke, etc.
				i = 0;
				i < _mapsize_x * _mapsize_y * _mapsize_z;
				++i)
		{
			if (getTiles()[i]->getSmoke() != 0)
				getTiles()[i]->prepareNewTurn();
		}

		getTileEngine()->calculateTerrainLighting(); // fires could have been started, stopped or smoke could reveal/conceal units.
	}

	reviveUnconsciousUnits();
}

/**
 * Checks for units that are unconcious and revives them if they shouldn't be.
 * kL, does this still need a check to see if the unit revives
 * *on a floor* (if not, drop him/her down to a floor tile) <- yes, it does. (also raise up onto terrainLevel)
 * Revived units need a tile to stand on. If the unit's current position is occupied, then
 * all directions around the tile are searched for a free tile to place the unit in.
 * If no free tile is found the unit stays unconscious.
 */
void SavedBattleGame::reviveUnconsciousUnits()
{
	for (std::vector<BattleUnit*>::iterator
			i = getUnits()->begin();
			i != getUnits()->end();
			++i)
	{
		if ((*i)->getArmor()->getSize() == 1)
		{
			Position originalPosition = (*i)->getPosition();
			if (originalPosition == Position(-1, -1, -1))
			{
				for (std::vector<BattleItem*>::iterator
						j = _items.begin();
						j != _items.end();
						++j)
				{
					if ((*j)->getUnit()
						&& (*j)->getUnit() == *i
						&& (*j)->getOwner())
					{
						originalPosition = (*j)->getOwner()->getPosition();
					}
				}
			}

			if ((*i)->getStatus() == STATUS_UNCONSCIOUS
				&& (*i)->getStunlevel() < (*i)->getHealth()
				&& (*i)->getHealth() > 0)
			{
				if (placeUnitNearPosition(
										*i,
										originalPosition))
				{
					// recover from unconscious
					(*i)->turn(false); // -> STATUS_STANDING

//kL					(*i)->kneel(false);
					if ((*i)->getOriginalFaction() == FACTION_PLAYER
						&& (*i)->getArmor()->getSize() == 1)		// kL
					{
						(*i)->kneel(true);							// kL
					}

					// Map::cacheUnit(BattleUnit* unit)
//					UnitSprite::setBattleUnit(unit, part);
					// UnitSprite::setBattleUnit(BattleUnit* unit, int part);
					(*i)->setCache(0);

					(*i)->setDirection(RNG::generate(0, 7));		// kL
					(*i)->setTimeUnits(0);							// kL

					getTileEngine()->calculateUnitLighting();
					getTileEngine()->calculateFOV(*i);

					removeUnconsciousBodyItem(*i);

					break;
				}
			}
		}
	}
}

/**
 * Removes the body item (corpse) that corresponds to a unit.
 */
void SavedBattleGame::removeUnconsciousBodyItem(BattleUnit* bu)
{
	for (std::vector<BattleItem*>::iterator
			corpse = getItems()->begin();
			corpse != getItems()->end();
			++corpse)
	{
		if ((*corpse)->getUnit() == bu)
		{
			removeItem(*corpse);

			return;
		}
	}
}

/**
 * Places units on the map. Handles large units that are placed on multiple tiles.
 * @param bu, The unit to be placed.
 * @param pos, The position to place the unit.
 * @param testOnly, If true then just checks if the unit can be placed at the position.
 * @return, True if the unit could be successfully placed.
 */
bool SavedBattleGame::setUnitPosition(
		BattleUnit* bu,
		const Position& pos,
		bool testOnly)
{
	int size = bu->getArmor()->getSize() - 1;

	for (int // first check if the tiles are occupied
			x = size;
			x >= 0;
			x--)
	{
		for (int
				y = size;
				y >= 0;
				y--)
		{
			Tile* t = getTile(pos + Position(x, y, 0));
			Tile* tb = getTile(pos + Position(x, y, -1));
			if (t == 0
				|| (t->getUnit() != 0
					&& t->getUnit() != bu)
				|| t->getTUCost(MapData::O_OBJECT, bu->getArmor()->getMovementType()) == 255
				|| (t->hasNoFloor(tb)
					&& bu->getArmor()->getMovementType() != MT_FLY))
			{
				return false;
			}
		}
	}

	if (size > 0)
	{
		getPathfinding()->setUnit(bu);
		for (int
				dir = 2;
				dir <= 4;
				++dir)
		{
			if (getPathfinding()->isBlocked(getTile(pos), 0, dir, 0))
				return false;
		}
	}

	if (testOnly)
		return true;


	for (int // set the unit in position
			x = size;
			x >= 0;
			x--)
	{
		for (int
				y = size;
				y >= 0;
				y--)
		{
			if (x == 0 && y == 0)
			{
				bu->setPosition(pos);
//				bu->setTile(getTile(pos), getTile(pos - Position(0, 0, 1)));
			}

			getTile(pos + Position(x, y, 0))->setUnit(
													bu,
													getTile(pos + Position(x, y, -1)));
		}
	}

	return true;
}

/**
 * Places a unit on or near a position.
 * @param unit, The unit to place.
 * @param entryPoint, The position around which to attempt to place the unit.
 * @return, True if the unit was successfully placed.
 */
bool SavedBattleGame::placeUnitNearPosition(
		BattleUnit* unit,
		Position entryPoint)
{
	if (setUnitPosition(
					unit,
					entryPoint)) return true;

	for (int
			dir = 0;
			dir <= 7;
			++dir)
	{
		Position offset;
		getPathfinding()->directionToVector(dir, &offset);

		Tile* t = getTile(entryPoint + offset);
		if (t
			&& !getPathfinding()->isBlocked(getTile(entryPoint), t, dir, 0)
			&& setUnitPosition(
							unit,
							entryPoint + offset))
		{
			return true;
		}
	}

	if (unit->getArmor()->getMovementType() == MT_FLY)
	{
		Tile* t = getTile(entryPoint + Position(0, 0, 1));
		if (t
			&& t->hasNoFloor(getTile(entryPoint))
			&& setUnitPosition(
							unit,
							entryPoint + Position(0, 0, 1)))
		{
			return true;
		}
	}

	return false;
}

/**
 * @brief Checks whether anyone on a particular faction is looking at the unit.
 *
 * Similar to getSpottingUnits() but returns a bool and stops searching if one positive hit is found.
 *
 * @param faction Faction to check through.
 * @param unit Whom to spot.
 * @return True when the unit can be seen
 */
/*kL bool SavedBattleGame::eyesOnTarget(UnitFaction faction, BattleUnit* unit)
{
	for (std::vector<BattleUnit*>::iterator i = getUnits()->begin(); i != getUnits()->end(); ++i)
	{
		if ((*i)->getFaction() != faction) continue;

		std::vector<BattleUnit*>* vis = (*i)->getVisibleUnits();

		if (std::find(vis->begin(), vis->end(), unit) != vis->end())
		{
			return true;
			// aliens know the location of all XCom agents sighted by all other
			// aliens due to sharing locations over their space-walkie-talkies
		}
	}

	return false;
} */

/**
 * Adds this unit to the vector of falling units.
 * @param unit The unit.
 */
bool SavedBattleGame::addFallingUnit(BattleUnit* unit)
{
	//Log(LOG_INFO) << "SavedBattleGame::addFallingUnit() ID = " << unit->getId();

	bool add = true;
	for (std::list<BattleUnit*>::iterator
			i = _fallingUnits.begin();
			i != _fallingUnits.end();
			++i)
	{
		if (unit == *i)
			add = false;
	}

	if (add)
	{
		_fallingUnits.push_front(unit);
		_unitsFalling = true;
	}

	return add;
}

/**
 * Gets all units in the battlescape that are falling.
 * @return The falling units in the battlescape.
 */
std::list<BattleUnit*>* SavedBattleGame::getFallingUnits()
{
	return &_fallingUnits;
}

/**
 * Toggles the switch that says "there are units falling, start the fall state".
 * @param fall, True if there are any units falling in the battlescape.
 */
void SavedBattleGame::setUnitsFalling(bool fall)
{
	_unitsFalling = fall;
}

/**
 * Returns whether there are any units falling in the battlescape.
 * @return, True if there are any units falling in the battlescape.
 */
bool SavedBattleGame::getUnitsFalling() const
{
	return _unitsFalling;
}

/**
 * Gets the highest ranked, living, non Mc'd unit of faction.
 * @param bool, xCom=true if faction_Player else false if faction_Hostile.
 * @return BattleUnit*, The highest ranked, living unit of the given faction.
 */
BattleUnit* SavedBattleGame::getHighestRanked(bool xcom)
{
	//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked() xcom = " << xcom;

	BattleUnit* leader = 0;

	for (std::vector<BattleUnit*>::iterator
			bu = _units.begin();
			bu != _units.end();
			++bu)
	{
		if (*bu
			&& !(*bu)->isOut(true, true))
		{
			if (xcom)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is Xcom";
				if ((*bu)->getOriginalFaction() == FACTION_PLAYER
					&& (*bu)->getFaction() == FACTION_PLAYER)
				{
					if (leader == 0
						|| (*bu)->getRankInt() > leader->getRankInt())
					{
						leader = *bu;
					}
				}
			}
			else if ((*bu)->getOriginalFaction() == FACTION_HOSTILE
				&& (*bu)->getFaction() == FACTION_HOSTILE)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is aLien";
				if (leader == 0
					|| (*bu)->getRankInt() < leader->getRankInt())
				{
					leader = *bu;
				}
			}
		}
	}

	//if (leader) Log(LOG_INFO) << ". leaderID = " << leader->getId();
	//else Log(LOG_INFO) << ". leaderID = 0";

	return leader;
}

/**
 * Gets the morale modifier, either
 * - a bonus based on the highest ranked, living unit of the xcom/alien factions
 * - or penalty for a unit (deceased..) passed into this function.
 * @param unit, Unit deceased; higher rank gets higher penalty to faction
 * @param xcom, If no unit is passed in this determines whether bonus applies to xcom/aliens
 * @return, The morale modifier
 */
int SavedBattleGame::getMoraleModifier(
		BattleUnit* unit,
		bool xcom)
{
	//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier()";
	int result = 100;

	if (unit == 0) // leadership Bonus
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), leadership Bonus";
		if (xcom)
		{
			BattleUnit* leader = getHighestRanked();
			if (leader)
			{
				switch (leader->getRankInt())
				{
					case 5:				// commander
						result += 15;	// 135, was 150
					case 4:				// colonel
						result += 5;	// 120, was 125
					case 3:				// captain
						result += 5;	// 115
					case 2:				// sergeant
						result += 10;	// 110
					case 1:				// squaddie
						result += 15;	// 100
					case 0:				// rookies...
						result -= 15;	// 85

					default:
					break;
				}
			}

			//Log(LOG_INFO) << ". . xCom leaderModifi = " << result;
		}
		else // alien
		{
			BattleUnit* leader = getHighestRanked(false);
			if (leader)
			{
				switch (leader->getRankInt())
				{
					case 0:				// commander
						result += 25;	// 150
					case 1:				// leader
						result += 10;	// 125
					case 2:				// engineer
						result += 5;	// 115
					case 3:				// medic
						result += 10;	// 110
					case 4:				// navigator
						result += 10;	// 100
					case 5:				// soldiers...
						result -= 10;	// 90

					// kL_note: terrorists are ranks #6 and #7

					default:
					break;
				}
			}

			//Log(LOG_INFO) << ". . aLien leaderModifi = " << result;
		}
	}
	else // morale Loss when 'unit' slain
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), unit slain Penalty";
		if (unit->getOriginalFaction() == FACTION_PLAYER) // XCOM dies. (mind controlled or not)
		{
			switch (unit->getRankInt())
			{
				case 5:					// commander
					result += 30;		// 200
				case 4:					// colonel
					result += 25;		// 170
				case 3:					// captain
					result += 20;		// 145
				case 2:					// sergeant
					result += 10;		// 125
				case 1:					// squaddie
					result += 15;		// 115

				default:
				break;
			}

			//Log(LOG_INFO) << ". . xCom lossModifi = " << result;
		}
		else if (unit->getFaction() == FACTION_HOSTILE) // aliens or Mind Controlled XCOM dies.
														// note this does funny things. A low-ranked, mind-controlled xCom unit's
														// death will make the aLiens take a hard hit (due to loss of malleable
														// 'fresh meat' for the Hive/Borg collective). But a high-ranking 'cannot
														// teach an old dog new tricks' Mc'd xCom soldier is not as much of a loss
														// to aLiens.....
		{
			switch (unit->getRankInt())
			{
				case 0:					// commander
					result += 30;		// 200
				case 1:					// leader
					result += 25;		// 170
				case 2:					// engineer
					result += 20;		// 145
				case 3:					// medic
					result += 10;		// 125
				case 4:					// navigator
					result += 15;		// 115

				// kL_note: terrorists are ranks #6 and #7 (soldiers are #5)

				default:
				break;
			}
			// else if a mind-controlled alien dies nobody cares.

			//Log(LOG_INFO) << ". . aLien lossModifi = " << result;
		}
	}

	//Log(LOG_INFO) << ". totalModifier = " << result;

	return result;
}

/**
 * Resets the turn counter.
 */
void SavedBattleGame::resetTurnCounter()
{
	_turn = 1;
}

/**
 * Resets visibility of all the tiles on the map.
 */
void SavedBattleGame::resetTiles()
{
	for (int
			i = 0;
			i != getMapSizeXYZ();
			++i)
	{
		_tiles[i]->setDiscovered(false, 0);
		_tiles[i]->setDiscovered(false, 1);
		_tiles[i]->setDiscovered(false, 2);
	}
}

/**
 * @return the tilesearch vector for use in AI functions.
 */
const std::vector<Position> SavedBattleGame::getTileSearch()
{
	return _tileSearch;
}

/**
 * is the AI allowed to cheat?
 * @return true if cheating.
 */
bool SavedBattleGame::isCheating()
{
	return _cheating;
}

/**
 * Gets the TU reserved type.
 * @return A battleactiontype.
 */
BattleActionType SavedBattleGame::getTUReserved() const
{
	return _tuReserved;
}

/**
 * Sets the TU reserved type.
 * @param reserved A battleactiontype.
 */
void SavedBattleGame::setTUReserved(BattleActionType reserved)
{
	_tuReserved = reserved;
}

/**
 * Gets the kneel reservation setting.
 * @return Should we reserve an extra 4 TUs to kneel?
 */
bool SavedBattleGame::getKneelReserved() const
{
	return _kneelReserved;
}

/**
 * Sets the kneel reservation setting.
 * @param reserved Should we reserve an extra 4 TUs to kneel?
 */
void SavedBattleGame::setKneelReserved(bool reserved)
{
	_kneelReserved = reserved;
}

/**
 * Return a reference to the base module destruction map
 * this map contains information on how many destructible base modules
 * remain at any given grid reference in the basescape, using [x][y] format.
 * -1 for "no items" 0 for "destroyed" and any actual number represents how many left.
 * @Return the base module damage map.
 */
std::vector<std::vector<std::pair<int, int> > >& SavedBattleGame::getModuleMap()
{
	return _baseModules;
}

/**
 * Calculate the number of map modules remaining by counting the map objects
 * on the top floor who have the baseModule flag set. We store this data in the grid
 * as outlined in the comments above, in pairs representing intial and current values.
 */
void SavedBattleGame::calculateModuleMap()
{
	_baseModules.resize(
				_mapsize_x / 10,
				std::vector<std::pair<int, int> >(
											_mapsize_y / 10,
											std::make_pair(-1, -1)));

	for (int
			x = 0;
			x != _mapsize_x;
			++x)
	{
		for (int
				y = 0;
				y != _mapsize_y;
				++y)
		{
			Tile* tile = getTile(Position(
										x,
										y,
										_mapsize_z - 1));

			if (tile
				&& tile->getMapData(MapData::O_OBJECT)
				&& tile->getMapData(MapData::O_OBJECT)->isBaseModule())
			{
				_baseModules[x / 10][y / 10].first += _baseModules[x / 10][y / 10].first > 0? 1: 2;
				_baseModules[x / 10][y / 10].second = _baseModules[x / 10][y / 10].first;
			}
		}
	}
}

/**
 * get a pointer to the geoscape save
 * @return a pointer to the geoscape save.
 */
SavedGame* SavedBattleGame::getGeoscapeSave()
{
	return _battleState->getGame()->getSavedGame();
}

}
