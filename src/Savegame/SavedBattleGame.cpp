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

#include "SavedBattleGame.h"

#include <vector>
#include <deque>
#include <queue>

#include <assert.h>

#include "BattleItem.h"
#include "Node.h"
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
		_battleState(NULL),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_selectedUnit(NULL),
		_lastSelectedUnit(NULL),
		_pathfinding(NULL),
		_tileEngine(NULL),
		_globalShade(0),
		_side(FACTION_PLAYER),
		_turn(1),
		_debugMode(false),
		_aborted(false),
		_itemId(0),
		_objectiveDestroyed(false),
		_unitsFalling(false),
		_cheating(false),
		_tuReserved(BA_NONE),
		_depth(0),
		_kneelReserved(false),
		_invBattle(NULL),
		_ambience(-1)
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

	for (std::vector<MapDataSet*>::iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		(*i)->unloadData();
	}

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
		MapDataSet* mds = rule->getMapDataSet(name);
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
			std::string type = (*i)["genUnitType"].as<std::string>();
			std::string armor = (*i)["genUnitArmor"].as<std::string>();

			unit = new BattleUnit( // create a new Unit, not-soldier but Vehicle, Civie, or aLien.
								rule->getUnit(type),
								faction,
								id,
								rule->getArmor(armor),
								static_cast<int>(savedGame->getDifficulty()),
								savedGame->getMonthsPassed()); // kL_add.
		}
		//Log(LOG_INFO) << "SavedGame::load(), difficulty = " << savedGame->getDifficulty();

		unit->load(*i);
		_units.push_back(unit);

		if (faction == FACTION_PLAYER)
		{
			if (unit->getId() == selectedUnit
				|| (_selectedUnit == NULL
					&& !unit->isOut()))
			{
				_selectedUnit = unit;
			}

			// silly hack to fix mind controlled aliens
			// TODO: save stats instead? maybe some kind of weapon will affect them at some point.
			if (unit->getOriginalFaction() == FACTION_HOSTILE)
				unit->adjustStats(
								static_cast<int>(savedGame->getDifficulty()),
								savedGame->getMonthsPassed()); // kL_add.
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
												NULL);
				else if (faction == FACTION_HOSTILE)
					aiState = new AlienBAIState(
											this,
											unit,
											NULL);
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

			int
				owner = (*i)["owner"].as<int>(),
				unit = (*i)["unit"].as<int>();

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
	_depth				= node["depth"].as<int>(_depth);
	_alienRace			= node["alienRace"].as<std::string>(_alienRace);
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
				++part)
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
		if (_tiles[i]->isVoid() == false)
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
	node["depth"]			= _depth;
	node["alienRace"]		= _alienRace; // kL

	return node;
}

/**
 * Gets the array of tiles.
 * @return, pointer to a pointer to the Tile array
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
	if (_nodes.empty() == false)
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
 * Sets the terrainType string.
 */
void SavedBattleGame::setTerrain(std::string terrain) // sza_MusicRules
{
	_terrain = terrain;
}

/**
 * Gets the terrainType string.
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
 * Gets the currently selected unit.
 * @return, pointer to BattleUnit
 */
BattleUnit* SavedBattleGame::getSelectedUnit() const
{
	return _selectedUnit;
}

/**
 * Sets the currently selected unit.
 * @param unit - pointer to BattleUnit
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
	if (_selectedUnit != NULL
		&& setDontReselect)
	{
		//Log(LOG_INFO) << ". dontReselect";
		_selectedUnit->dontReselect();
	}

	if (_units.empty())
	{
		//Log(LOG_INFO) << ". units.Empty, ret NULL";
		_selectedUnit = NULL;		// kL
		_lastSelectedUnit = NULL;	// kL

		return NULL;
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
				//Log(LOG_INFO) << ". . . negative, ret NULL";
				_selectedUnit = NULL;
			}

			//Log(LOG_INFO) << ". . ret selectedUnit";
			return _selectedUnit;
		}
		else if (_selectedUnit == NULL
			&& i == begin)
		{
			//Log(LOG_INFO) << ". . finish do, ret selectedUnit = NULL";
//kL			return _selectedUnit;
			return NULL; // kL
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
 * @param pos - reference a Position
 * @return, pointer to the BattleUnit, or NULL if none found
 */
BattleUnit* SavedBattleGame::selectUnit(const Position& pos)
{
	BattleUnit* bu = getTile(pos)->getUnit();

	if (bu
		&& bu->isOut())
	{
		return NULL;
	}

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
	//Log(LOG_INFO) << ". getTurn()";
	return _turn;
}

/**
 * Ends the current turn and progresses to the next one.
 */
void SavedBattleGame::endBattleTurn()
{
	//Log(LOG_INFO) << "SavedBattleGame::endBattleTurn()";
	if (_side == FACTION_PLAYER) // end of Xcom turn.
	{
		//Log(LOG_INFO) << ". end Faction_Player";

		if (_selectedUnit
			&& _selectedUnit->getOriginalFaction() == FACTION_PLAYER)
		{
			_lastSelectedUnit = _selectedUnit;
		}

		_side = FACTION_HOSTILE;
		_selectedUnit = NULL;

		// kL_begin: sbg::endBattleTurn() no Reselect xCom units at endTurn!!!
		for (std::vector<BattleUnit*>::iterator
				i = getUnits()->begin();
				i != getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == FACTION_PLAYER)
			{
				(*i)->dontReselect(); // either zero tu's or set no reselect
				(*i)->setDashing(false); // no longer dashing; dash is effective vs. Reaction Fire only.
			}
		} // kL_end.
	}
	else if (_side == FACTION_HOSTILE) // end of Alien turn.
	{
		//Log(LOG_INFO) << ". end Faction_Hostile";

		_side = FACTION_NEUTRAL;

		// if there is no neutral team, we skip this section
		// and instantly prepare the new turn for the player.
		if (selectNextFactionUnit() == NULL) // this will now cycle through NEUTRAL units
			// so shouldn't that really be 'selectNextFactionUnit()'???!
			// see selectFactionUnit() -> isSelectable()
		{
			//Log(LOG_INFO) << ". . nextFactionUnit == 0";

			prepareBattleTurn();
			//Log(LOG_INFO) << ". . prepareBattleTurn DONE";
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

		prepareBattleTurn();
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
//kL		selectNextFactionUnit();
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
											liveSoldiers);
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
		if ((*i)->getFaction() == _side)	// this causes an Mc'd unit to lose its turn. Because its faction
											// hasn't reverted until *after* bu->prepareUnitTurn() runs. So it
											// actually switches at the _end_ of its original faction's turn ...
			(*i)->prepareUnitTurn();		// Reverts faction, tu/stun recovery, Fire damage, etc.


		if ((*i)->getFaction() == FACTION_PLAYER) // including units Mc'd by xCom
		{
//			(*i)->setDashing(false); // no longer dashing; dash is effective vs. Reaction Fire only. MOVED UP ^

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
//			(*i)->prepareUnitTurn();
	}
	//Log(LOG_INFO) << "done looping units";

	// redo calculateFOV() *after* aliens & civies have been set notVisible
	_tileEngine->recalculateFOV();
	//Log(LOG_INFO) << "done recalculateFoV";

	if (_side != FACTION_PLAYER)
		selectNextFactionUnit();

	//Log(LOG_INFO) << "SavedBattleGame::endBattleTurn() EXIT";
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
 * @return, debug mode
 */
bool SavedBattleGame::getDebugMode() const
{
	return _debugMode;
}

/**
 * Gets the BattlescapeState.
 * @return, pointer to the BattlescapeState
 */
BattlescapeState* SavedBattleGame::getBattleState()
{
	return _battleState;
}

/**
 * Gets the BattlescapeGame.
 * @return, pointer to the BattlescapeGame
 */
BattlescapeGame* SavedBattleGame::getBattleGame()
{
	return _battleState->getBattleGame();
}

/**
 * Sets the BattlescapeState.
 * @param bs - pointer to the BattlescapeState
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
		if ((*i)->isOut() == false)
		{
			int size = (*i)->getArmor()->getSize() - 1;

			if ((*i)->getTile()
				&& (*i)->getTile()->getUnit() == *i)
			{
				for (int // remove Unit from its current tile
						x = size;
						x > -1;
						--x)
				{
					for (int
							y = size;
							y > -1;
							--y)
					{
						getTile((*i)->getTile()->getPosition() + Position(x, y, 0))->setUnit(NULL);
					}
				}
			}

			for (int // set Unit onto its proper tile
					x = size;
					x > -1;
					--x)
			{
				for (int
						y = size;
						y > -1;
						--y)
				{
					Tile* tile = getTile((*i)->getPosition() + Position(x, y, 0));
					tile->setUnit(
							*i,
							getTile(tile->getPosition() + Position(0, 0,-1)));
				}
			}
		}

		if ((*i)->getFaction() == FACTION_PLAYER)
			(*i)->setVisible();
	}
}

/**
 * Gives access to the storageSpace vector, for distribution of items in base defense missions.
 * @return, reference a vector of storage positions
 */
std::vector<Position>& SavedBattleGame::getStorageSpace()
{
	return _storageSpace;
}

/**
 * Move all the leftover items in base defense missions to random locations in the storage facilities.
 * @param tile - pointer to a tile where all the goodies are initially stored
 */
void SavedBattleGame::randomizeItemLocations(Tile* tile)
{
	//Log(LOG_INFO) << "SavedBattleGame::randomizeItemLocations()";
	if (_storageSpace.empty() == false)
	{
		//Log(LOG_INFO) << ". storageSpace NOT empty";
		for (std::vector<BattleItem*>::iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				)
		{
			//Log(LOG_INFO) << ". . iterate battleItems";
			if ((*i)->getSlot()->getId() == "STR_GROUND")
			{
				//Log(LOG_INFO) << ". . . slot = GROUND";
				getTile(_storageSpace.at(RNG::generate(
													0,
													_storageSpace.size() - 1)))
												->addItem(
														*i,
														(*i)->getSlot());

				i = tile->getInventory()->erase(i);
			}
			else
			{
				//Log(LOG_INFO) << ". . . slot Not GROUND";
				++i;
			}
		}
	}
}

/**
 * Removes an item from the game; when ammo item is depleted for example.
 * @param item - an item to remove
 */
void SavedBattleGame::removeItem(BattleItem* item)
{
	//Log(LOG_INFO) << "SavedBattleGame::removeItem()";

	// due to strange design, the item has to be removed from the tile it is on too (if it is on a tile)
	BattleUnit* bu = item->getOwner();
	Tile* tile = item->getTile();
	if (tile)
	{
		for (std::vector<BattleItem*>::iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				++i)
		{
			if (*i == item)
			{
				tile->getInventory()->erase(i);

				break;
			}
		}
	}

	if (bu)
	{
		for (std::vector<BattleItem*>::iterator
				i = bu->getInventory()->begin();
				i != bu->getInventory()->end();
				++i)
		{
			if (*i == item)
			{
				bu->getInventory()->erase(i);

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

			//Log(LOG_INFO) << "SavedBattleGame::removeItem() EXIT[0]";
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
	//Log(LOG_INFO) << "SavedBattleGame::removeItem() EXIT[1]";
}

/**
 * Sets whether the mission was aborted or successful.
 * @param flag - true if the mission was aborted, or false if the mission was successful.
 */
void SavedBattleGame::setAborted(bool flag)
{
	_aborted = flag;
}

/**
 * Returns whether the mission was aborted or successful.
 * @return, true if the mission was aborted, or false if the mission was successful
 */
bool SavedBattleGame::isAborted() const
{
	return _aborted;
}

/**
 * Sets whether the objective is destroyed.
 * @param flag - true if the objective is destroyed
 */
void SavedBattleGame::setObjectiveDestroyed(bool flag)
{
	_objectiveDestroyed = flag;

	if (flag
		&& Options::battleAutoEnd)
	{
		setSelectedUnit(NULL);
		_battleState->getBattleGame()->cancelCurrentAction(true);
		_battleState->getBattleGame()->requestEndTurn();
	}
}

/**
 * Returns whether the objective is destroyed.
 * @return, true if the objective is destroyed
 */
bool SavedBattleGame::isObjectiveDestroyed()
{
	return _objectiveDestroyed;
}

/**
 * Gets the current item ID.
 * @return, pointer to current item ID
 */
int* SavedBattleGame::getCurrentItemId()
{
	return &_itemId;
}

/**
 * Finds a fitting node where a unit can spawn.
 * @param nodeRank	- rank of the node (this is not the rank of the alien!)
 * kL_note: actually, it pretty much is the rank of the aLien.
 * @param unit		- pointer to the unit (to test-set its position)
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
		return NULL;

	size_t legit = static_cast<size_t>(RNG::generate(
												0,
												static_cast<int>(legitNodes.size()) - 1));

	return legitNodes[legit];
}

/**
 * Finds a fitting node where a unit can patrol to.
 * @param scout		- true if the unit is scouting
 * @param unit		- pointer to a unit, to get its position
 * @param fromNode	- pointer to the node the unit is currently at
 * @return, pointer to the chosen node
 */
Node* SavedBattleGame::getPatrolNode(
		bool scout,
		BattleUnit* unit,
		Node* fromNode)
{
	//Log(LOG_INFO) << "SavedBattleGame::getPatrolNode()";
	if (fromNode == NULL)
	{
		if (Options::traceAI) Log(LOG_INFO) << "This alien got lost. :(";

		fromNode = getNodes()->at(static_cast<size_t>(RNG::generate(
																0,
																static_cast<int>(getNodes()->size()) - 1)));
	}


	std::vector<Node*> legitNodes;
	Node* bestNode = NULL;
	Node* node = NULL;

	// scouts roam all over while all others shuffle around to adjacent nodes at most:
//kL	int const linksEnd = scout? getNodes()->size(): fromNode->getNodeLinks()->size();
	size_t linksEnd = 0;
	if (scout)
		linksEnd = getNodes()->size();
	else
		linksEnd = fromNode->getNodeLinks()->size();

	for (size_t
			i = 0;
			i < linksEnd;
			++i)
	{
		if (scout == false
			&& fromNode->getNodeLinks()->at(i) < 1)
		{
			continue; // nothing to patrol to
		}

//kL	node = getNodes()->at(scout? i: fromNode->getNodeLinks()->at(i));
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
			if (bestNode == NULL
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
			&& scout == false)
		{
//			return Sectopod::CTD();
			return getPatrolNode(
								true,
								unit,
								fromNode); // move damnit
		}
		else
			//Log(LOG_INFO) << " . legitNodes is NOT Empty.";
			//Log(LOG_INFO) << " . return NULL";
			return NULL;
	}

	if (scout) // picks a random destination
	{
		//Log(LOG_INFO) << " . scout";

//kL	return legitNodes[RNG::generate(0, static_cast<int>(legitNodes.size()) - 1)];
		size_t legit = static_cast<size_t>(RNG::generate(
													0,
													static_cast<int>(legitNodes.size()) - 1));
		//Log(LOG_INFO) << " . return legitNodes @ " << legit;
		return legitNodes[legit];
	}
	else
	{
		//Log(LOG_INFO) << " . !scout";
		if (bestNode == NULL)
			//Log(LOG_INFO) << " . no bestNode, return NULL";
			return NULL;

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
void SavedBattleGame::prepareBattleTurn()
{
	//Log(LOG_INFO) << "SavedBattleGame::prepareBattleTurn()";
	std::vector<Tile*> tilesOnFire;
	std::vector<Tile*> tilesOnSmoke;

	for (int // prepare a list of tiles on fire
			i = 0;
			i < _mapsize_x * _mapsize_y * _mapsize_z;
			++i)
	{
		if (getTiles()[i]->getFire() > 0)
			tilesOnFire.push_back(getTiles()[i]);
	}

	for (std::vector<Tile*>::iterator // first: fires spread
			i = tilesOnFire.begin();
			i != tilesOnFire.end();
			++i)
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

					if (tile // if there's no wall blocking the path of the flames...
						&& getTileEngine()->horizontalBlockage(
															*i,
															tile,
															DT_IN)
														== 0)
					{
						//Log(LOG_INFO) << ". fire " << (*i)->getPosition() << " to " << tile->getPosition();
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

				getTileEngine()->applyGravity(*i);
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

	if (tilesOnFire.empty() == false
		|| tilesOnSmoke.empty() == false)
	{
		for (int // do damage to units, average out the smoke, etc.
				i = 0;
				i < _mapsize_x * _mapsize_y * _mapsize_z;
				++i)
		{
			if (getTiles()[i]->getSmoke() != 0)
				getTiles()[i]->prepareTileTurn(); // normalizes overlaps
		}
		// fires could have been started, stopped or smoke could reveal/conceal units.
//kL	getTileEngine()->calculateTerrainLighting();
	}

	reviveUnconsciousUnits();

	getTileEngine()->calculateTerrainLighting(); // kL
	getTileEngine()->recalculateFOV(); // kL
}

/**
 * Checks for units that are unconscious and revives them if they shouldn't be.
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
		if ((*i)->getGeoscapeSoldier() != NULL
//			|| ((*i)->getUnitRules() &&
			|| ((*i)->getUnitRules()->getMechanical() == false
				&& (*i)->getArmor()->getSize() == 1))
		{
			Position originalPosition = (*i)->getPosition();
			if (originalPosition == Position(-1,-1,-1))
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
				&& (*i)->getStun() < (*i)->getHealth()
				&& (*i)->getHealth() > 0)
			{
				if (placeUnitNearPosition(
										*i,
										originalPosition))
				{
					// recover from unconscious
//kL				(*i)->turn(); // -> STATUS_STANDING
					(*i)->setStatus(STATUS_STANDING); // kL

//kL				(*i)->kneel(false);
					if ((*i)->getOriginalFaction() == FACTION_PLAYER)
						(*i)->kneel(true); // kL

					// Map::cacheUnit(BattleUnit* unit)
//					UnitSprite::setBattleUnit(unit, part);
					// UnitSprite::setBattleUnit(BattleUnit* unit, int part);
					(*i)->setCache(NULL);

					(*i)->setDirection(RNG::generate(0, 7));	// kL
					(*i)->setTimeUnits(0);						// kL

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
 * @param bu		- pointer to a unit to be placed
 * @param pos		- reference the position to place the unit
 * @param testOnly	- true just checks if unit can be placed at the position
 * @return, true if unit was placed successfully
 */
bool SavedBattleGame::setUnitPosition(
		BattleUnit* bu,
		const Position& pos,
		bool testOnly)
{
	int size = bu->getArmor()->getSize() - 1;

	for (int // first check if the tiles are occupied
			x = size;
			x > -1;
			--x)
	{
		for (int
				y = size;
				y > -1;
				--y)
		{
			Tile* tile = getTile(pos + Position(x, y, 0));
			Tile* tileBelow = getTile(pos + Position(x, y,-1));
			Tile* tileAbove = getTile(pos + Position(x, y, 1)); // kL

			if (tile == NULL
				|| (tile->getUnit() != NULL
					&& tile->getUnit() != bu)
				|| tile->getTUCost(MapData::O_OBJECT, bu->getArmor()->getMovementType()) == 255
				|| (tile->hasNoFloor(tileBelow)
					&& bu->getArmor()->getMovementType() != MT_FLY)
				|| (tileAbove // kL:
					&& tileAbove->getUnit() != NULL
					&& tileAbove->getUnit() != bu
					&& bu->getHeight() - tile->getTerrainLevel() > 26)) // don't stuck yer head up someone's flying arse.
				// kL_note: no check for ceilings yet ....
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
			if (getPathfinding()->isBlocked(
										getTile(pos),
										NULL,
										dir))
			{
				return false;
			}
		}
	}

	if (testOnly)
		return true;


	for (int // set the unit in position
			x = size;
			x > -1;
			--x)
	{
		for (int
				y = size;
				y > -1;
				--y)
		{
			if (x == 0 && y == 0)
			{
				bu->setPosition(pos);
//				bu->setTile(getTile(pos), getTile(pos - Position(0, 0, 1)));
			}

			getTile(pos + Position(x, y, 0))->setUnit(
													bu,
													getTile(pos + Position(x, y,-1)));
		}
	}

	return true;
}

/**
 * Places a unit on or near a position.
 * @param unit			- pointer to a unit to place
 * @param entryPoint	- the position around which to attempt to place the unit
 * @return, true if unit was successfully placed
 */
bool SavedBattleGame::placeUnitNearPosition(
		BattleUnit* unit,
		Position entryPoint)
{
	if (setUnitPosition(
					unit,
					entryPoint))
	{
		return true;
	}

	int dirRand = RNG::generate(0, 7); // kL_begin:
	for (int
			dir = dirRand;
			dir < dirRand + 8;
			++dir) // kL_end.
//	for (int
//			dir = 0;
//			dir < 8;
//			++dir)
	{
		Position offset;
//kL	getPathfinding()->directionToVector(dir, &offset);
		getPathfinding()->directionToVector(dir %8, &offset); // kL

		Tile* tile = getTile(entryPoint + offset);
		if (tile
			&& !getPathfinding()->isBlocked(
										getTile(entryPoint),
										tile,
										dir)
			&& setUnitPosition(
							unit,
							entryPoint + offset))
		{
			return true;
		}
	}

/*kL: uhh no.
	if (unit->getArmor()->getMovementType() == MT_FLY)
	{
		Tile* tile = getTile(entryPoint + Position(0, 0, 1));
		if (tile
			&& tile->hasNoFloor(getTile(entryPoint))
			&& setUnitPosition(
							unit,
							entryPoint + Position(0, 0, 1)))
		{
			return true;
		}
	} */

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
 * Adds this unit to the vector of falling units, if it doesn't already exist there.
 * @param unit - the unit to add
 * @return, true if the unit was added
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

			break;
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
 * @param isXCOM - true if examining Faction_Player, false for Faction_Hostile
 * @return, pointer to highest ranked BattleUnit of faction
 */
BattleUnit* SavedBattleGame::getHighestRanked(bool isXCOM)
{
	//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked() xcom = " << xcom;
	BattleUnit* leader = NULL;

	for (std::vector<BattleUnit*>::iterator
			bu = _units.begin();
			bu != _units.end();
			++bu)
	{
		if (*bu
			&& (*bu)->isOut(true, true) == false)
		{
			if (isXCOM)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is Xcom";
				if ((*bu)->getOriginalFaction() == FACTION_PLAYER
					&& (*bu)->getFaction() == FACTION_PLAYER)
				{
					if (leader == NULL
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
				if (leader == NULL
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
 * Gets a morale modifier.
 * Either a bonus based on the highest ranked living unit of the faction
 * or a penalty for a deceased unit.
 * @param unit		- pointer to unit deceased; higher rank gives higher penalty
 * @param isXCOM	- if no unit is passed in this determines whether bonus applies to xCom or aLiens
 * @return, morale modifier
 */
int SavedBattleGame::getMoraleModifier( // note: Add bonus to aLiens for Cydonia & Final Assault.
		BattleUnit* unit,
		bool isXCOM)
{
	//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier()";
	int ret = 100;

	bool isMars = false;

	int missionFactor = 0;
	if (_missionType == "STR_TERROR_MISSION"
		|| _missionType == "STR_ALIEN_BASE_ASSAULT"
		|| _missionType == "STR_BASE_DEFENSE")
	{
		missionFactor = 1;
	}
	else if (_missionType == "STR_MARS_CYDONIA_LANDING"
		|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{
		missionFactor = 2;
		isMars = true;
	}

	if (unit == NULL) // leadership Bonus
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), leadership Bonus";
		if (isXCOM)
		{
			BattleUnit* leader = getHighestRanked();
			if (leader)
			{
				switch (leader->getRankInt())
				{
					case 5:			// commander
						ret += 15;	// 135, was 150
					case 4:			// colonel
						ret += 5;	// 120, was 125
					case 3:			// captain
						ret += 5;	// 115
					case 2:			// sergeant
						ret += 10;	// 110
					case 1:			// squaddie
						ret += 15;	// 100
					case 0:			// rookies...
						ret -= 15;	// 85

					default:
					break;
				}
			}

			//Log(LOG_INFO) << ". . xCom leaderModifi = " << ret;
		}
		else // aLien
		{
			BattleUnit* leader = getHighestRanked(false);
			if (leader)
			{
				switch (leader->getRankInt())
				{
					case 0:			// commander
						ret += 25;	// 150
					case 1:			// leader
						ret += 10;	// 125
					case 2:			// engineer
						ret += 5;	// 115
					case 3:			// medic
						ret += 10;	// 110
					case 4:			// navigator
						ret += 10;	// 100
					case 5:			// soldiers...
						ret -= 10;	// 90

					// kL_note: terrorists are ranks #6 and #7

					default:
					break;
				}
			}

			ret += missionFactor * 50; // higher morale.

			//Log(LOG_INFO) << ". . aLien leaderModifi = " << ret;
		}
	}
	else // morale Loss when 'unit' slain
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), unit slain Penalty";
		if (unit->getOriginalFaction() == FACTION_PLAYER) // xCom dies. (mind controlled or not)
		{
			switch (unit->getRankInt())
			{
				case 5:			// commander
					ret += 30;	// 200
				case 4:			// colonel
					ret += 25;	// 170
				case 3:			// captain
					ret += 20;	// 145
				case 2:			// sergeant
					ret += 10;	// 125
				case 1:			// squaddie
					ret += 15;	// 115

				default:
				break;
			}

			//Log(LOG_INFO) << ". . xCom lossModifi = " << ret;
		}
		else if (unit->getFaction() == FACTION_HOSTILE) // aLien dies.
		{
			switch (unit->getRankInt())
			{
				case 0:			// commander
					ret += 30;	// 200
				case 1:			// leader
					ret += 25;	// 170
				case 2:			// engineer
					ret += 20;	// 145
				case 3:			// medic
					ret += 10;	// 125
				case 4:			// navigator
					ret += 15;	// 115

				// note: soldiers are rank #5, terrorists are ranks #6 and #7

				default:
				break;
			}
			// else if a mind-controlled alien dies nobody cares.

			if (isMars) // less hit for losing a leader on Cydonia.
				ret /= 2;

			//Log(LOG_INFO) << ". . aLien lossModifi = " << ret;
		}
	}

	//Log(LOG_INFO) << ". totalModifier = " << ret;
	return ret;
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
 * Gets a reference to the base module destruction map.
 * This map contains information on how many destructible base modules
 * remain at any given grid reference in the basescape, using [x][y] format.
 * -1 for "no items" 0 for "destroyed" and any actual number represents how many left.
 * @return, reference to a vector of vectors containing pairs that make up base module damage maps
 */
std::vector<std::vector<std::pair<int, int> > >& SavedBattleGame::getModuleMap()
{
	return _baseModules;
}

/**
 * Calculates the number of map modules remaining by counting the map objects
 * on the top floor who have the baseModule flag set. We store this data in the grid
 * as outlined in the comments above, in pairs representing intial and current values.
 */
void SavedBattleGame::calculateModuleMap()
{
	_baseModules.resize(
					_mapsize_x / 10,
					std::vector<std::pair<int, int> >(
													_mapsize_y / 10,
													std::make_pair(-1,-1)));

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
 * Gets a pointer to the geoscape save.
 * @return, pointer to the geoscape save
 */
SavedGame* SavedBattleGame::getGeoscapeSave()
{
	return _battleState->getGame()->getSavedGame();
}

/**
 * Gets the depth of the battlescape.
 * @return depth.
 */
int const SavedBattleGame::getDepth() const
{
	return _depth;
}

/**
 * Sets the depth of the battlescape game.
 * @param depth the intended depth 0-3.
 */
void SavedBattleGame::setDepth(int depth)
{
	_depth = depth;
}

/**
 * Uses the depth variable to choose a palette.
 * @param state the state to set the palette for.
 */
void SavedBattleGame::setPaletteByDepth(State* state)
{
	if (_depth == 0)
		state->setPalette("PAL_BATTLESCAPE");
	else
	{
		std::stringstream ss;
		ss << "PAL_BATTLESCAPE_" << _depth;
		state->setPalette(ss.str());
	}
}

/**
 * Sets the ambient battlescape sound effect.
 * @param sound the intended sound.
 */
void SavedBattleGame::setAmbientSound(int sound)
{
	_ambience = sound;
}

/**
 * Gets the ambient battlescape sound effect.
 * @return the intended sound.
 */
const int SavedBattleGame::getAmbientSound() const
{
	return _ambience;
}

/**
 * kL. Sets the battlescape inventory tile when BattlescapeGenerator runs.
 * For use in base missions to randomize item locations.
 * @param invBattle - pointer to the tile where battle inventory is created
 */
void SavedBattleGame::setBattleInventory(Tile* invBattle) // kL
{
	_invBattle = invBattle;
}

/**
 * kL. Gets the inventory tile for preBattle InventoryState OK click.
 */
Tile* SavedBattleGame::getBattleInventory() const // kL
{
	return _invBattle;
}

/**
 * kL. Sets the alien race for this battle.
 * Currently used only for Base Defense missions, but should fill for other missions also.
 */
void SavedBattleGame::setAlienRace(const std::string& alienRace) // kL
{
	_alienRace = alienRace;
}

/**
 * kL. Gets the alien race participating in this battle.
 * Currently used only to get the alien race for SoldierDiary statistics
 * after a Base Defense mission.
 */
const std::string& SavedBattleGame::getAlienRace() const // kL
{
	return _alienRace;
}

}
