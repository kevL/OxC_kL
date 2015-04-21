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

#include "SavedBattleGame.h"

//#include <vector>
//#include <deque>
//#include <queue>
//#include <assert.h>

#include "BattleItem.h"
#include "Node.h"
#include "SavedGame.h"
#include "SerializationHelper.h"
#include "Tile.h"

#include "../Battlescape/AlienBAIState.h"
#include "../Battlescape/CivilianBAIState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/ExplosionBState.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/Position.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/OperationPool.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes a brand new SavedBattleGame.
 * @param titles - pointer to a vector of pointers to OperationPool (default NULL)
 */
SavedBattleGame::SavedBattleGame(const std::vector<OperationPool*>* titles)
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
		_objectivesDestroyed(0),
		_objectivesNeeded(0),
		_unitsFalling(false),
		_cheating(false),
		_batReserved(BA_NONE),
		_depth(0),
		_kneelReserved(false),
		_invBattle(NULL),
		_ambience(-1),
		_groundLevel(-1)
{
	//Log(LOG_INFO) << "\nCreate SavedBattleGame";
	_tileSearch.resize(11 * 11);

	for (size_t
			i = 0;
			i != 121;
			++i)
	{
		_tileSearch[i].x = (static_cast<int>(i) % 11) - 5;
		_tileSearch[i].y = (static_cast<int>(i) / 11) - 5;
	}

	if (titles != NULL)
	{
		const size_t pool = 0; //RNG::generate(0, titles->size()-1 // <- in case I want to expand this for different missionTypes. eg, Cydonia -> "Blow Hard"
		_operationTitle = titles->at(pool)->genOperation();
	}
}

/**
 * Deletes the game content from memory.
 */
SavedBattleGame::~SavedBattleGame()
{
	//Log(LOG_INFO) << "Delete SavedBattleGame";
	for (size_t
			i = 0;
			i != static_cast<size_t>(_mapsize_z * _mapsize_y * _mapsize_x);
			++i)
	{
		delete _tiles[i];
	}
	delete[] _tiles;

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		(*i)->unloadData();
	}

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleItem*>::iterator
			i = _recoverGuaranteed.begin();
			i != _recoverGuaranteed.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleItem*>::iterator
			i = _recoverConditional.begin();
			i != _recoverConditional.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleItem*>::const_iterator
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
 * Loads the SavedBattleGame from a YAML file.
 * @param node		- reference a YAML node
 * @param rule		- pointer to the Ruleset
 * @param savedGame	- pointer to the SavedGame
 */
void SavedBattleGame::load(
		const YAML::Node& node,
		Ruleset* rule,
		SavedGame* savedGame)
{
	Log(LOG_INFO) << "SavedBattleGame::load()";

	_mapsize_x			= node["width"]			.as<int>(_mapsize_x);
	_mapsize_y			= node["length"]		.as<int>(_mapsize_y);
	_mapsize_z			= node["height"]		.as<int>(_mapsize_z);
	_missionType		= node["missionType"]	.as<std::string>(_missionType);
	_globalShade		= node["globalshade"]	.as<int>(_globalShade);
	_turn				= node["turn"]			.as<int>(_turn);
	_depth				= node["depth"]			.as<int>(_depth);
	_terrain			= node["terrain"]		.as<std::string>(_terrain); // sza_MusicRules

	const int selectedUnit = node["selectedUnit"].as<int>();

	Log(LOG_INFO) << ". load mapdatasets";
	for (YAML::const_iterator
			i = node["mapdatasets"].begin();
			i != node["mapdatasets"].end();
			++i)
	{
		const std::string name = i->as<std::string>();
		MapDataSet* const mds = rule->getMapDataSet(name);
		_mapDataSets.push_back(mds);
	}

	Log(LOG_INFO) << ". init map";
	initMap(
		_mapsize_x,
		_mapsize_y,
		_mapsize_z);

	if (!node["tileTotalBytesPer"])
	{
		Log(LOG_INFO) << ". load tiles [1]";
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
		Log(LOG_INFO) << ". load tiles [2]";
		// load key to how the tile data was saved
		Tile::SerializationKey serKey;
		// WARNING: Don't trust extracting integers from YAML as anything other than 'int' ...
		// NOTE: Many load sequences use '.as<size_t>' .....
		const size_t totalTiles = node["totalTiles"].as<size_t>();

		std::memset(
				&serKey,
				0,
				sizeof(Tile::SerializationKey));

		// WARNING: Don't trust extracting integers from YAML as anything other than 'int' ...
		serKey.index			= node["tileIndexSize"]		.as<Uint8>(serKey.index);
		serKey.totalBytes		= node["tileTotalBytesPer"]	.as<Uint32>(serKey.totalBytes);
		serKey._fire			= node["tileFireSize"]		.as<Uint8>(serKey._fire);
		serKey._smoke			= node["tileSmokeSize"]		.as<Uint8>(serKey._smoke);
		serKey._animOffset		= node["tileOffsetSize"]	.as<Uint8>(serKey._animOffset); // kL
		serKey._mapDataID		= node["tileIDSize"]		.as<Uint8>(serKey._mapDataID);
		serKey._mapDataSetID	= node["tileSetIDSize"]		.as<Uint8>(serKey._mapDataSetID);
		serKey.boolFields		= node["tileBoolFieldsSize"].as<Uint8>(1); // boolean flags used to be stored in an unmentioned byte (Uint8) :|

		// load binary tile data!
		const YAML::Binary binTiles = node["binTiles"].as<YAML::Binary>();

		Uint8
			* readBuffer = (Uint8*)binTiles.data(),
			* const dataEnd = readBuffer + totalTiles * serKey.totalBytes;

		while (readBuffer < dataEnd)
		{
			int index = unserializeInt(
									&readBuffer,
									serKey.index);
			assert(
				index >= 0
				&& index < _mapsize_x * _mapsize_z * _mapsize_y);

			_tiles[static_cast<size_t>(index)]->loadBinary( // loadBinary's privileges to advance *readBuffer have been revoked
														readBuffer,
														serKey);
			readBuffer += serKey.totalBytes - serKey.index;	// readBuffer is now incremented strictly by totalBytes in case there are obsolete fields present in the data
		}
	}

	if (_missionType == "STR_BASE_DEFENSE")
	{
		Log(LOG_INFO) << ". load xcom base";
		if (node["moduleMap"])
			_baseModules = node["moduleMap"].as<std::vector<std::vector<std::pair<int, int> > > >();
		else
			// backwards compatibility: imperfect solution, modules that were completely destroyed
			// prior to saving and updating builds will be counted as indestructible.
			calculateModuleMap();
	}

	Log(LOG_INFO) << ". load nodes";
	for (YAML::const_iterator
			i = node["nodes"].begin();
			i != node["nodes"].end();
			++i)
	{
		Node* const n = new Node();
		n->load(*i);
		_nodes.push_back(n);
	}


	int id;

	BattleUnit* unit;
	UnitFaction
		faction,
		originalFaction;

	Log(LOG_INFO) << ". load units";
	for (YAML::const_iterator
			i = node["units"].begin();
			i != node["units"].end();
			++i)
	{
		id				= (*i)["soldierId"]									.as<int>();
		faction			= static_cast<UnitFaction>((*i)["faction"]			.as<int>());
		originalFaction	= static_cast<UnitFaction>((*i)["originalFaction"]	.as<int>(faction));

		if (id < BattleUnit::MAX_SOLDIER_ID)	// BattleUnit is linked to a geoscape soldier
		{
			unit = new BattleUnit(				// look up the matching soldier
								savedGame->getSoldier(id),
								_depth,
								static_cast<int>(savedGame->getDifficulty())); // kL_add: For VictoryPts value per death.
		}
		else
		{
			const std::string
				type	= (*i)["genUnitType"]	.as<std::string>(),
				armor	= (*i)["genUnitArmor"]	.as<std::string>();

			unit = new BattleUnit( // create a new Unit, not-soldier but Vehicle, Civie, or aLien.
								rule->getUnit(type),
								originalFaction,
								id,
								rule->getArmor(armor),
								static_cast<int>(savedGame->getDifficulty()),
								_depth,
								savedGame->getMonthsPassed());
		}

		Log(LOG_INFO) << ". . load unit " << id;
		unit->load(*i);
		_units.push_back(unit);
		Log(LOG_INFO) << ". . . Done";

		if (faction == FACTION_PLAYER)
		{
			if (unit->getId() == selectedUnit
				|| (_selectedUnit == NULL
					&& unit->isOut() == false))
			{
				_selectedUnit = unit;
			}
		}

		if (faction != FACTION_PLAYER
			&& unit->getStatus() != STATUS_DEAD)
		{
			if (const YAML::Node& ai = (*i)["AI"])
			{
				BattleAIState* aiState;

				if (faction == FACTION_HOSTILE)
					aiState = new AlienBAIState(
											this,
											unit,
											NULL);
				else
					aiState = new CivilianBAIState(
												this,
												unit,
												NULL);

				aiState->load(ai);
				unit->setAIState(aiState);
			}
		}
	}

	// matches up tiles and units
	Log(LOG_INFO) << ". reset tiles";
	resetUnitTiles();

	Log(LOG_INFO) << ". load items";
	for (YAML::const_iterator
			i = node["items"].begin();
			i != node["items"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (rule->getItem(type) != NULL)
		{
			id = (*i)["id"].as<int>(-1);
			BattleItem* const item = new BattleItem(
												rule->getItem(type),
												NULL,
												id);
			item->load(*i);
			type = (*i)["inventoryslot"].as<std::string>();

			if (type != "NULL")
				item->setSlot(rule->getInventory(type));

			const int
				owner		= (*i)["owner"]			.as<int>(),
				prevOwner	= (*i)["previousOwner"]	.as<int>(-1),
				unit		= (*i)["unit"]			.as<int>();

			// match up items and units
			for (std::vector<BattleUnit*>::const_iterator
					bu = _units.begin();
					bu != _units.end();
					++bu)
			{
				if ((*bu)->getId() == owner)
					item->moveToOwner(*bu);

				if ((*bu)->getId() == unit)
					item->setUnit(*bu);
			}

			for (std::vector<BattleUnit*>::iterator
					bu = _units.begin();
					bu != _units.end();
					++bu)
			{
				if ((*bu)->getId() == prevOwner)
					item->setPreviousOwner(*bu);
			}

			// match up items and tiles
			if (item->getSlot() != NULL
				&& item->getSlot()->getType() == INV_GROUND)
			{
				const Position pos = (*i)["position"].as<Position>();

				if (pos.x != -1)
					getTile(pos)->addItem(item, rule->getInventory("STR_GROUND"));
			}

			_items.push_back(item);
		}
	}

	Log(LOG_INFO) << ". load weapons w/ ammo";
	// tie ammo items to their weapons, running through the items again
	std::vector<BattleItem*>::const_iterator weapon = _items.begin();
	for (YAML::const_iterator
			i = node["items"].begin();
			i != node["items"].end();
			++i)
	{
		if (rule->getItem((*i)["type"].as<std::string>()) != NULL)
		{
			const int ammo = (*i)["ammoItem"].as<int>();
			if (ammo != -1)
			{
				for (std::vector<BattleItem*>::const_iterator
						j = _items.begin();
						j != _items.end();
						++j)
				{
					if ((*j)->getId() == ammo)
					{
						(*weapon)->setAmmoItem(*j);
						break;
					}
				}
			}

			++weapon;
		}
	}

	Log(LOG_INFO) << ". set some vars";
	_objectivesDestroyed	= node["objectivesDestroyed"]					.as<int>(_objectivesDestroyed);
	_objectivesNeeded		= node["objectivesNeeded"]						.as<int>(_objectivesNeeded);
	_batReserved			= (BattleActionType)node["batReserved"]			.as<int>(_batReserved);
	_kneelReserved			= node["kneelReserved"]							.as<bool>(_kneelReserved);
	_ambience				= node["ambience"]								.as<int>(_ambience);
	_alienRace				= node["alienRace"]								.as<std::string>(_alienRace);
	_operationTitle			= Language::utf8ToWstr(node["operationTitle"]	.as<std::string>());

	Log(LOG_INFO) << ". load conditional recovery";
	for (YAML::const_iterator
			i = node["recoverConditional"].begin();
			i != node["recoverConditional"].end();
			++i)
	{
		const std::string type = (*i)["type"].as<std::string>();
		if (rule->getItem(type) != NULL)
		{
			id = (*i)["id"].as<int>(-1);
			BattleItem* item = new BattleItem(
										rule->getItem(type),
										NULL,
										id);
			item->load(*i);
			_recoverConditional.push_back(item);
		}
	}

	Log(LOG_INFO) << ". load guaranteed recovery";
	for (YAML::const_iterator
			i = node["recoverGuaranteed"].begin();
			i != node["recoverGuaranteed"].end();
			++i)
	{
		const std::string type = (*i)["type"].as<std::string>();
		if (rule->getItem(type) != NULL)
		{
			id = (*i)["id"].as<int>(-1);
			BattleItem* item = new BattleItem(
										rule->getItem(type),
										NULL,
										id);
			item->load(*i);
			_recoverGuaranteed.push_back(item);
		}
	}

	_music = node["music"].as<std::string>(_music);

	Log(LOG_INFO) << ". set item ID";
	setNextItemId(); // kL
	Log(LOG_INFO) << "SavedBattleGame::load() EXIT";

	// TEST, reveal all tiles
//	const size_t totalTiles = static_cast<size_t>(_mapsize_x * _mapsize_z * _mapsize_y);
//	for (size_t
//			i = 0;
//			i != totalTiles;
//			++i)
//	{
//		_tiles[i]->setDiscovered(true, 2);
//	}
}

/**
 * Loads the resources required by the map in the battle save.
 * @param game - pointer to Game
 */
void SavedBattleGame::loadMapResources(Game* game)
{
	ResourcePack* const res = game->getResourcePack();

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		(*i)->loadData();

		if (game->getRuleset()->getMCDPatch((*i)->getName()) != NULL)
			game->getRuleset()->getMCDPatch((*i)->getName())->modifyData(*i);
	}

	int
		mapDataID,
		mapDataSetID;

	for (size_t
			i = 0;
			i != static_cast<size_t>(_mapsize_z * _mapsize_y * _mapsize_x);
			++i)
	{
		for (int
				part = 0;
				part != 4;
				++part)
		{
			_tiles[i]->getMapData(
								&mapDataID,
								&mapDataSetID,
								part);

			if (mapDataID != -1
				&& mapDataSetID != -1)
			{
				_tiles[i]->setMapData(
								_mapDataSets[static_cast<size_t>(mapDataSetID)]->getObjects()->at(static_cast<size_t>(mapDataID)),
								mapDataID,
								mapDataSetID,
								part);
			}
		}
	}

	initUtilities(res);

	_tileEngine->calculateSunShading();
	_tileEngine->calculateTerrainLighting();
	_tileEngine->calculateUnitLighting();

	_tileEngine->recalculateFOV();
}

/**
 * Saves the saved battle game to a YAML file.
 * @return, YAML node
 */
YAML::Node SavedBattleGame::save() const
{
	YAML::Node node;

	if (_objectivesNeeded > 0)
	{
		node["objectivesDestroyed"]	= _objectivesDestroyed;
		node["objectivesNeeded"]	= _objectivesNeeded;
	}

	node["width"]			= _mapsize_x;
	node["length"]			= _mapsize_y;
	node["height"]			= _mapsize_z;
	node["missionType"]		= _missionType;
	node["globalshade"]		= _globalShade;
	node["turn"]			= _turn;
	node["terrain"]			= _terrain; // kL sza_MusicRules
	node["selectedUnit"]	= (_selectedUnit ? _selectedUnit->getId() : -1);

	for (std::vector<MapDataSet*>::const_iterator
			i = _mapDataSets.begin();
			i != _mapDataSets.end();
			++i)
	{
		node["mapdatasets"].push_back((*i)->getName());
	}

#if 0
	for (size_t
			i = 0;
			i != _mapsize_z * _mapsize_y * _mapsize_x;
			++i)
	{
		if (_tiles[i]->isVoid() == false)
			node["tiles"].push_back(_tiles[i]->save());
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

	const size_t totalTiles = _mapsize_z * _mapsize_y * _mapsize_x;
	size_t tileDataSize = static_cast<size_t>(Tile::serializationKey.totalBytes * static_cast<Uint32>(totalTiles));
	Uint8
		* const tileData = (Uint8*)calloc(tileDataSize, 1),
		* writeBuffer = tileData;

	for (size_t
			i = 0;
			i != totalTiles;
			++i)
	{
		serializeInt( // kL <- save ALL Tiles. (Stop void tiles returning undiscovered postReload.)
				&writeBuffer,
				Tile::serializationKey.index,
				static_cast<int>(i));
		_tiles[i]->saveBinary(&writeBuffer);
/*		if (_tiles[i]->isVoid() == false)
		{
			serializeInt(
					&writeBuffer,
					Tile::serializationKey.index,
					static_cast<int>(i));
			_tiles[i]->saveBinary(&writeBuffer);
		}
		else
			tileDataSize -= Tile::serializationKey.totalBytes; */
	}

	node["totalTiles"]	= tileDataSize / static_cast<size_t>(Tile::serializationKey.totalBytes); // not strictly necessary, just convenient
	node["binTiles"]	= YAML::Binary(
									tileData,
									tileDataSize);

	std::free(tileData);
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

	node["batReserved"]		= static_cast<int>(_batReserved);
	node["kneelReserved"]	= _kneelReserved;
	node["depth"]			= _depth;
	node["ambience"]		= _ambience;
	node["alienRace"]		= _alienRace;
	node["operationTitle"]	= Language::wstrToUtf8(_operationTitle);

	for (std::vector<BattleItem*>::const_iterator
			i = _recoverGuaranteed.begin();
			i != _recoverGuaranteed.end();
			++i)
	{
		node["recoverGuaranteed"].push_back((*i)->save());
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _recoverConditional.begin();
			i != _recoverConditional.end();
			++i)
	{
		node["recoverConditional"].push_back((*i)->save());
	}

	node["music"] = _music;

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
 * @param mapsize_x -
 * @param mapsize_y -
 * @param mapsize_z -
 */
void SavedBattleGame::initMap(
		const int mapsize_x,
		const int mapsize_y,
		const int mapsize_z)
{
	size_t totalTiles = _mapsize_z * _mapsize_y * _mapsize_x;

	if (_nodes.empty() == false)
	{
		for (size_t
				i = 0;
				i != totalTiles;
				++i)
		{
			delete _tiles[i];
		}

		delete[] _tiles;

		for (std::vector<Node*>::const_iterator
				i = _nodes.begin();
				i != _nodes.end();
				++i)
		{
			delete *i;
		}

		_nodes.clear();
		_mapDataSets.clear();
	}

	// create tile objects
	_mapsize_x = mapsize_x;
	_mapsize_y = mapsize_y;
	_mapsize_z = mapsize_z;

	totalTiles = _mapsize_z * _mapsize_y * _mapsize_x;
	_tiles = new Tile*[totalTiles];

	for (size_t
			i = 0;
			i != totalTiles;
			++i)
	{
		Position pos;
		getTileCoords(
					static_cast<int>(i),
					&pos.x,
					&pos.y,
					&pos.z);

		_tiles[i] = new Tile(pos);
	}
}

/**
 * Initializes the map utilities.
 * @param res - pointer to ResourcePack
 */
void SavedBattleGame::initUtilities(ResourcePack* res)
{
	delete _pathfinding;
	delete _tileEngine;

	_pathfinding = new Pathfinding(this);
	_tileEngine = new TileEngine(
							this,
							res->getVoxelData());
}

/**
 * Sets the mission type.
 * @param missionType - reference a mission type
 */
void SavedBattleGame::setMissionType(const std::string& missionType)
{
	_missionType = missionType;
}

/**
 * Gets the mission type.
 * @return, the mission type
 */
std::string SavedBattleGame::getMissionType() const
{
	return _missionType;
}

/**
 * Sets the global shade.
 * @param shade - the global shade
 */
void SavedBattleGame::setGlobalShade(int shade)
{
	_globalShade = shade;
}

/**
 * Gets the global shade.
 * @return, the global shade
 */
int SavedBattleGame::getGlobalShade() const
{
	return _globalShade;
}

/**
 * Gets the map width.
 * @return, the map width (Size X) in tiles
 */
int SavedBattleGame::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map length.
 * @return, the map length (Size Y) in tiles
 */
int SavedBattleGame::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map height.
 * @return, the map height (Size Z) in layers
 */
int SavedBattleGame::getMapSizeZ() const
{
	return _mapsize_z;
}

/**
 * Gets the map size in tiles.
 * @return, the map size
 */
int SavedBattleGame::getMapSizeXYZ() const
{
	return _mapsize_x * _mapsize_y * _mapsize_z;
}

/**
 * Sets the terrain-type string.
 * @param terrain - the terrain
 */
void SavedBattleGame::setBattleTerrain(const std::string& terrain)
{
	_terrain = terrain;
}

/**
 * Gets the terrainType string.
 * @return, the terrain
 */
std::string SavedBattleGame::getBattleTerrain() const
{
	return _terrain;
}

/**
 * Converts a tile index to coordinates.
 * @param index	- the unique tileindex
 * @param x		- pointer to the X coordinate
 * @param y		- pointer to the Y coordinate
 * @param z		- pointer to the Z coordinate
 */
void SavedBattleGame::getTileCoords(
		int index,
		int* x,
		int* y,
		int* z) const
{
	*z =  index / (_mapsize_y * _mapsize_x);
	*y = (index % (_mapsize_y * _mapsize_x)) / _mapsize_x;
	*x = (index % (_mapsize_y * _mapsize_x)) % _mapsize_x;
}

/**
 * Gets the currently selected unit.
 * @return, pointer to the BattleUnit
 */
BattleUnit* SavedBattleGame::getSelectedUnit() const
{
	return _selectedUnit;
}

/**
 * Sets the currently selected unit.
 * @param unit - pointer to a BattleUnit
 */
void SavedBattleGame::setSelectedUnit(BattleUnit* unit)
{
	_selectedUnit = unit;
}

/**
* Selects the previous player unit.
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param setDontReselect	- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or NULL if none can be selected
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
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param setDontReselect	- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or NULL if none can be selected
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
 * @param dir				- direction to select (1 for next and -1 for previous)
 * @param checkReselect		- true to check the reselectable flag
 * @param setDontReselect	- true to set the reselectable flag FALSE
 * @param checkInventory	- true to check if the unit has an inventory
 * @return, pointer to newly selected BattleUnit or NULL if none can be selected
 */
BattleUnit* SavedBattleGame::selectFactionUnit(
		int dir,
		bool checkReselect,
		bool setDontReselect,
		bool checkInventory)
{
	if (_units.empty() == true)
	{
		_selectedUnit = NULL;
		_lastSelectedUnit = NULL;

		return NULL;
	}

	if (setDontReselect == true
		&& _selectedUnit != NULL)
	{
		_selectedUnit->dontReselect();
	}


	std::vector<BattleUnit*>::const_iterator
		unitFirst,
		unitLast;

	if (dir > 0)
	{
		unitFirst = _units.begin();
		unitLast = _units.end() - 1;
	}
	else
	{
		unitFirst = _units.end() - 1;
		unitLast = _units.begin();
	}

	std::vector<BattleUnit*>::const_iterator i = std::find(
														_units.begin(),
														_units.end(),
														_selectedUnit);
	do
	{
		if (i == _units.end()) // no unit selected
		{
			i = unitFirst;
			continue;
		}

		if (i != unitLast)
			i += dir;
		else // reached the end, wrap-around
			i = unitFirst;

		if (*i == _selectedUnit) // back to start ... no more units found
		{
			if (checkReselect == true
				&& _selectedUnit->reselectAllowed() == false)
			{
				_selectedUnit = NULL;
			}

			return _selectedUnit;
		}
		else if (_selectedUnit == NULL
			&& i == unitFirst)
		{
			return NULL;
		}
	}
	while ((*i)->isSelectable(
						_side,
						checkReselect,
						checkInventory) == false);


	_selectedUnit = *i;

	return _selectedUnit;
}

/**
 * Gets the unit at Position if it's valid and conscious.
 * @param pos - reference a Position
 * @return, pointer to the BattleUnit or NULL
 */
BattleUnit* SavedBattleGame::selectUnit(const Position& pos)
{
	BattleUnit* const bu = getTile(pos)->getUnit();

	if (bu != NULL
		&& bu->isOut(true, true) == true)
	{
		return NULL;
	}

	return bu;
}

/**
 * Gets the list of nodes.
 * @return, pointer to a vector of pointers to the Nodes
 */
std::vector<Node*>* SavedBattleGame::getNodes()
{
	return &_nodes;
}

/**
 * Gets the list of units.
 * @return, pointer to a vector of pointers to the BattleUnits
 */
std::vector<BattleUnit*>* SavedBattleGame::getUnits()
{
	return &_units;
}

/**
 * Gets the list of items.
 * @return, pointer to a vector of pointers to the BattleItems
 */
std::vector<BattleItem*>* SavedBattleGame::getItems()
{
	return &_items;
}

/**
 * Gets the pathfinding object.
 * @return, pointer to Pathfinding
 */
Pathfinding* SavedBattleGame::getPathfinding() const
{
	return _pathfinding;
}

/**
 * Gets the terrain modifier object.
 * @return, pointer to the TileEngine
 */
TileEngine* SavedBattleGame::getTileEngine() const
{
	return _tileEngine;
}

/**
* Gets the array of mapblocks.
* @return, pointer to a vector of pointers to the MapDataSet
*/
std::vector<MapDataSet*>* SavedBattleGame::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Gets the side currently playing.
 * @return, the unit faction currently playing
 */
UnitFaction SavedBattleGame::getSide() const
{
	return _side;
}

/**
 * Gets the current turn number.
 * @return, the current turn
 */
int SavedBattleGame::getTurn() const
{
	return _turn;
}

/**
 * Ends the current faction-turn and progresses to the next one.
 * @note Called from BattlescapeGame::endTurnPhase()
 * @return, true if the turn rolls-over back to faction Player
 */
bool SavedBattleGame::endBattlePhase()
{
	//Log(LOG_INFO) << "sbg:endBattlePhase()";
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getFaction() == _side)
		{
			(*i)->setRevived(false);

			if (_side == FACTION_PLAYER)
				(*i)->dontReselect();
		}
	}

	bool ret = false;

	if (_side == FACTION_PLAYER) // end of Player turn.
	{
		if (_selectedUnit != NULL
			&& _selectedUnit->getOriginalFaction() == FACTION_PLAYER)
		{
			_lastSelectedUnit = _selectedUnit;
		}

		_side = FACTION_HOSTILE;
		_selectedUnit = NULL;
	}
	else if (_side == FACTION_HOSTILE) // end of Alien turn.
	{
		_side = FACTION_NEUTRAL;

		// if there is no neutral team, skip this section
		// and instantly prepare new turn for the player.
		if (selectNextFactionUnit() == NULL) // else this will cycle through NEUTRAL units
		{
			spreadFireSmoke(); // do Tile stuff
			++_turn;
			ret = true;

			_side = FACTION_PLAYER;

			if (_lastSelectedUnit != NULL
				&& _lastSelectedUnit->isSelectable(FACTION_PLAYER))
			{
				_selectedUnit = _lastSelectedUnit;
			}
			else
				selectNextFactionUnit();

			while (_selectedUnit != NULL
				&& _selectedUnit->getFaction() != FACTION_PLAYER)
			{
				selectNextFactionUnit(true);
			}
		}
	}
	else if (_side == FACTION_NEUTRAL) // end of Civilian turn.
	{
		spreadFireSmoke(); // do Tile stuff
		++_turn;
		ret = true;

		_side = FACTION_PLAYER;

		if (_lastSelectedUnit != NULL
			&& _lastSelectedUnit->isSelectable(FACTION_PLAYER))
		{
			_selectedUnit = _lastSelectedUnit;
		}
		else
			selectNextFactionUnit();

		while (_selectedUnit != NULL
			&& _selectedUnit->getFaction() != FACTION_PLAYER)
		{
			selectNextFactionUnit(true);
		}
	}



	// ** _side HAS ADVANCED to next faction after here!!! ** //


	int
		liveAliens,
		liveSoldiers;
	_battleState->getBattleGame()->tallyUnits(
											liveAliens,
											liveSoldiers);

	// kL_begin: pseudo the Turn20 reveal and the less than 3 aliens left rule.
	if (_side == FACTION_HOSTILE)
	{
		const int delta = RNG::generate(0,5);
		if (_turn > 17 + delta
			|| (_turn > 8
				&& liveAliens < delta - 1))
		{
			_cheating = true;
		}
	}

	//Log(LOG_INFO) << ". side = " << (int)_side;
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		(*i)->setDashing(false); // no longer dashing; dash is effective vs. Reaction Fire only.

		if ((*i)->getOriginalFaction() == _side)
		{
			reviveUnit(*i, true);
			(*i)->takeFire();
		}

		if ((*i)->getFaction() == _side)	// This causes an Mc'd unit to lose its turn.
			(*i)->prepUnit();				// REVERTS FACTION, does tu/stun recovery, Fire damage, etc.

		// if newSide=XCOM, xCom agents DO NOT revert to xCom; MC'd aLiens revert to aLien.
		// if newSide=Alien, xCom agents revert to xCom; MC'd aLiens DO NOT revert to aLien.


		if ((*i)->isOut(true, true) == true)	// safety.
			(*i)->setExposed(-1);				// That got done when unit went down.
		else if ((*i)->getFaction() == FACTION_HOSTILE
			|| (*i)->getOriginalFaction() == FACTION_HOSTILE
			|| _cheating == true) // aLiens know where xCom is when cheating ~turn20
		{
			(*i)->setExposed(); // aLiens always know where their buddies are, Mc'd or not.
		}
		else if ((*i)->getExposed() != -1
			&& _side == FACTION_PLAYER)
		{
			(*i)->setExposed((*i)->getExposed() + 1);
		}

		if ((*i)->getFaction() != FACTION_PLAYER)
			(*i)->setUnitVisible(false);
	}

	_tileEngine->calculateSunShading();
	_tileEngine->calculateTerrainLighting();
	_tileEngine->calculateUnitLighting(); // turn off MCed alien lighting.

	// redo calculateFOV() *after* aliens & civies have been set
	// notVisible -> AND *only* after a calcLighting has been done !
	_tileEngine->recalculateFOV();

	if (_side != FACTION_PLAYER)
		selectNextFactionUnit();

	return ret;
}

/**
 * Turns on debug mode.
 */
void SavedBattleGame::setDebugMode()
{
	_debugMode = true;

	for (size_t // reveal tiles.
			i = 0;
			i != static_cast<size_t>(_mapsize_z * _mapsize_y * _mapsize_x);
			++i)
	{
		_tiles[i]->setDiscovered(true, 2);
	}
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
 * Gets the BattlescapeGame.
 * @return, pointer to the BattlescapeGame
 */
BattlescapeGame* SavedBattleGame::getBattleGame() const
{
	return _battleState->getBattleGame();
}

/**
 * Gets the BattlescapeState.
 * @return, pointer to the BattlescapeState
 */
BattlescapeState* SavedBattleGame::getBattleState() const
{
	return _battleState;
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
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->isOut() == false)
		{
			const int unitSize = (*i)->getArmor()->getSize() - 1;

			if ((*i)->getTile() != NULL // remove unit from its current tile
				&& (*i)->getTile()->getUnit() == *i) // wtf, is this super-safety ......
			{
				for (int
						x = unitSize;
						x > -1;
						--x)
				{
					for (int
							y = unitSize;
							y > -1;
							--y)
					{
						getTile((*i)->getTile()->getPosition() + Position(x, y, 0))->setUnit(NULL);
					}
				}
			}

			for (int // set unit onto its proper tile
					x = unitSize;
					x > -1;
					--x)
			{
				for (int
						y = unitSize;
						y > -1;
						--y)
				{
					Tile* const tile = getTile((*i)->getPosition() + Position(x, y, 0));
					tile->setUnit(
								*i,
								getTile(tile->getPosition() + Position(0, 0,-1)));
				}
			}
		}

		if ((*i)->getFaction() == FACTION_PLAYER)
			(*i)->setUnitVisible();
	}
}

/**
 * Gives access to the storageSpace vector
 * for distribution of items in base defense missions.
 * @return, reference a vector of storage positions
 */
std::vector<Position>& SavedBattleGame::getStorageSpace()
{
	return _storageSpace;
}

/**
 * Move all the leftover items in base defense missions
 * to random locations in the storage facilities.
 * @param tile - pointer to a tile where all the goodies are initially stored
 */
void SavedBattleGame::randomizeItemLocations(Tile* tile)
{
	if (_storageSpace.empty() == false)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				)
		{
			if ((*i)->getSlot()->getId() == "STR_GROUND")
			{
				getTile(_storageSpace.at(RNG::generate(
													0,
													_storageSpace.size() - 1)))->addItem(
																					*i,
																					(*i)->getSlot());

				i = tile->getInventory()->erase(i);
			}
			else
				++i;
		}
	}
}

/**
 * Removes an item from the game - when ammo item is depleted for example.
 * Due to strange design the item has to be removed
 * from the tile it is on too if it is on a tile.
 * @param item - item to remove
 */
void SavedBattleGame::removeItem(BattleItem* const item)
{
	//Log(LOG_INFO) << "SBG::removeItem() id " << item->getId();
	Tile* const tile = item->getTile();
	if (tile != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
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

	BattleUnit* const bu = item->getOwner();
	if (bu != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
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

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if (*i == item)
		{
			_items.erase(i);
			return;
		}
	}

	_deleted.push_back(item);

/*	for (int i = 0; i < _mapsize_x * _mapsize_y * _mapsize_z; ++i)
	{
		for (std::vector<BattleItem*>::const_iterator it = _tiles[i]->getInventory()->begin(); it != _tiles[i]->getInventory()->end(); )
		{
			if ((*it) == item)
			{
				it = _tiles[i]->getInventory()->erase(it);
				return;
			}
			++it;
		}
	} */
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
 * Increments the objectives-needed counter.
 */
void SavedBattleGame::addToObjectiveCount()
{
	++_objectivesNeeded;
}

/**
 * Increments the objectives-destroyed counter.
 */
void SavedBattleGame::addDestroyedObjective()
{
	++_objectivesDestroyed;

	if (Options::battleAutoEnd == true
		&& allObjectivesDestroyed() == true)
	{
		setSelectedUnit(NULL);
		_battleState->getBattleGame()->cancelCurrentAction(true);
		_battleState->getBattleGame()->requestEndTurn();
	}
}

/**
 * Returns whether the objectives are detroyed.
 * @return, true if the objectives are destroyed
 */
bool SavedBattleGame::allObjectivesDestroyed() const
{
	return _objectivesNeeded > 0
		&& _objectivesNeeded == _objectivesDestroyed;
}

/**
 * Sets the next available item ID value.
 * @note Used only at the finish of loading a SavedBattleGame.
 * @note ItemIDs start at 0.
 */
void SavedBattleGame::setNextItemId()
{
	int
		highValue = -1,
		id;

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		id = (*i)->getId();
		if (id > highValue)
			highValue = id;
	}

	_itemId = ++highValue;
}

/**
 * Gets the next available item ID value.
 * @return, pointer to the highest available value
 */
int* SavedBattleGame::getNextItemId()
{
	return &_itemId;
}

/**
 * Finds a fitting node where a unit can spawn.
 * bgen.addAlien() uses a fallback mechanism to test assorted nodeRanks ...
 * @param unitRank	- rank of the unit attempting to spawn
 * @param unit		- pointer to the unit (to test-set its position)
 * @return, pointer to the chosen node
 */
Node* SavedBattleGame::getSpawnNode(
		int unitRank,
		BattleUnit* unit)
{
	std::vector<Node*> legitNodes;

	for (std::vector<Node*>::const_iterator
			i = getNodes()->begin();
			i != getNodes()->end();
			++i)
	{
		if ((*i)->getPriority() > 0						// spawn-priority 0 is not spawnplace
			&& (*i)->getRank() == unitRank				// ranks must match
			&& (!((*i)->getType() & Node::TYPE_SMALL)	// the small unit bit is not set on the node
				|| unit->getArmor()->getSize() == 1)		// or the unit is small
			&& (!((*i)->getType() & Node::TYPE_FLYING)	// the flying unit bit is not set on the node
				|| unit->getMovementType() == MT_FLY)		// or the unit can fly
			&& setUnitPosition(							// check if unit can be set at this node
							unit,							// ie. it's big enough
							(*i)->getPosition(),			// and there's not already a unit there.
							true) == true)				// testOnly, runs again w/ FALSE on return to bgen::addAlien()
		{
			for (int
					j = (*i)->getPriority();
					j != 0;
					--j)
			{
				legitNodes.push_back(*i); // weighted by Priority
			}
		}
	}

	if (legitNodes.empty() == true)
		return NULL;

	const size_t pick = static_cast<size_t>(RNG::generate(
													0,
													static_cast<int>(legitNodes.size()) - 1));

	return legitNodes[pick];
}

/**
 * Finds a fitting node where a given unit can patrol to.
 * @param scout		- true if the unit is scouting
 * @param unit		- pointer to a BattleUnit
 * @param curNode	- pointer to the node that unit is currently at
 * @return, pointer to the destination Node
 */
Node* SavedBattleGame::getPatrolNode(
		bool scout,
		BattleUnit* unit,
		Node* curNode)
{
	//Log(LOG_INFO) << "SavedBattleGame::getPatrolNode()";
	if (curNode == NULL)
		curNode = getNodes()->at(static_cast<size_t>(RNG::generate(
																0,
																static_cast<int>(getNodes()->size()) - 1)));

	std::vector<Node*>
		legitNodes,
		rankedNodes;
	Node* node = NULL;

	size_t eligibleQty;
	if (scout == true)
		eligibleQty = getNodes()->size();
	else
		eligibleQty = curNode->getNodeLinks()->size();

	for (size_t
			i = 0;
			i < eligibleQty;
			++i)
	{
		if (scout == false
			&& curNode->getNodeLinks()->at(i) < 1)
		{
			continue; // non-scouts need Links to travel along.
		}

		if (scout == true)
			node = getNodes()->at(i);
		else
			node = getNodes()->at(curNode->getNodeLinks()->at(i));

		if ((node->getFlags() > 0
				|| node->getRank() > 0
				|| scout == true)										// for non-scouts we find a node with a desirability above 0
			&& (!(node->getType() & Node::TYPE_SMALL)					// the small unit bit is not set
				|| unit->getArmor()->getSize() == 1)						// or the unit is small
			&& (!(node->getType() & Node::TYPE_FLYING)					// the flying unit bit is not set
				|| unit->getMovementType() == MT_FLY)						// or the unit can fly
			&& node->isAllocated() == false								// check if not allocated
			&& !(node->getType() & Node::TYPE_DANGEROUS)				// don't go there if an alien got shot there; stupid behavior like that
			&& setUnitPosition(											// check if unit can be set at this node
							unit,											// ie. it's big enough
							node->getPosition(),							// and there's not already a unit there.
							true) == true									// but don't actually set the unit...
			&& getTile(node->getPosition()) != NULL						// the node is on a valid tile
			&& getTile(node->getPosition())->getFire() == 0				// you are not a firefighter; do not patrol into fire
			&& (getTile(node->getPosition())->getDangerous() == false	// aliens don't run into a grenade blast
				|| unit->getFaction() != FACTION_HOSTILE)					// but civies do!
			&& (node != curNode											// scouts push forward
				|| scout == false)											// others can mill around.. ie, stand there
			&& node->getPosition().x > -1								// x-pos valid
			&& node->getPosition().y > -1)								// y-pos valid
		{
			for (int
					j = node->getFlags();
					j != 0;
					--j)
			{
				legitNodes.push_back(node); // weighted by Flags
			}

			if (scout == false
				&& node->getRank() == Node::nodeRank[static_cast<size_t>(unit->getRankInt())]
													[0])				// high-class node here.
			{
				rankedNodes.push_back(node);
			}
		}
	}

	if (legitNodes.empty() == true)
	{
		//Log(LOG_INFO) << " . legitNodes is EMPTY.";
		if (scout == false
			&& unit->getArmor()->getSize() > 1)
		{
//			return Sectopod::CTD();
			return getPatrolNode(
								true,
								unit,
								curNode);
		}

		//Log(LOG_INFO) << " . return NULL";
		return NULL;
	}
	//Log(LOG_INFO) << " . legitNodes is NOT Empty.";

	size_t pick;

	if (scout == true // picks a random destination
		|| rankedNodes.empty() == true
		|| RNG::percent(21) == true) // officers can go for a stroll ...
	{
		//Log(LOG_INFO) << " . scout";
		pick = static_cast<size_t>(RNG::generate(
											0,
											static_cast<int>(legitNodes.size()) - 1));
		//Log(LOG_INFO) << " . return legitNodes @ " << pick;
		return legitNodes[pick];
	}
	//Log(LOG_INFO) << " . !scout";

	pick = static_cast<size_t>(RNG::generate(
										0,
										static_cast<int>(rankedNodes.size()) - 1));
	//Log(LOG_INFO) << " . return legitNodes @ " << pick;
	return rankedNodes[pick];
}

/**
 * Carries out new turn preparations such as fire and smoke spreading.
 * Also explodes any explosive tiles that get destroyed by fire.
 */
void SavedBattleGame::spreadFireSmoke()
{
	const size_t mapSize = static_cast<size_t>(_mapsize_x * _mapsize_y * _mapsize_z);
	std::vector<Tile*>
		tilesFired,
		tilesSmoked;

	Tile* tile;
	int var;

	for (size_t
			i = 0;
			i != mapSize;
			++i)
	{
		if (getTiles()[i]->getFire() != 0)
			tilesFired.push_back(getTiles()[i]);

		if (getTiles()[i]->getSmoke() != 0)
			tilesSmoked.push_back(getTiles()[i]);

		getTiles()[i]->setDangerous(false);
	}

	for (std::vector<Tile*>::const_iterator
			i = tilesFired.begin();
			i != tilesFired.end();
			++i)
	{
		(*i)->decreaseFire();

		var = (*i)->getFire() / 2;

		if (var != 0)
		{
			for (int
					dir = 0;
					dir < 7;
					dir += 2)
			{
				Position spreadPos;
				Pathfinding::directionToVector(
											dir,
											&spreadPos);
				tile = getTile((*i)->getPosition() + spreadPos);

				if (tile != NULL
					&& _tileEngine->horizontalBlockage(
													*i,
													tile,
													DT_IN) == 0)
				{
					tile->ignite(var);
				}
			}
		}
		else
		{
			if ((*i)->getMapData(MapData::O_OBJECT) != NULL)
			{
				if ((*i)->getMapData(MapData::O_OBJECT)->getFlammable() != 255
					&& (*i)->getMapData(MapData::O_OBJECT)->getArmor() != 255)
				{
					if ((*i)->destroy(MapData::O_OBJECT) == true)
						addDestroyedObjective();

					if ((*i)->destroy(MapData::O_FLOOR) == true)
						addDestroyedObjective();
				}
			}
			else if ((*i)->getMapData(MapData::O_FLOOR) != NULL)
			{
				if ((*i)->getMapData(MapData::O_FLOOR)->getFlammable() != 255
					&& (*i)->getMapData(MapData::O_FLOOR)->getArmor() != 255)
				{
					if ((*i)->destroy(MapData::O_FLOOR) == true)
						addDestroyedObjective();
				}
			}

			_tileEngine->applyGravity(*i);
		}
	}

	for (std::vector<Tile*>::const_iterator
			i = tilesSmoked.begin();
			i != tilesSmoked.end();
			++i)
	{
		(*i)->decreaseSmoke();

		var = (*i)->getSmoke() / 2;

		if (var != 0)
		{
			tile = getTile((*i)->getPosition() + Position(0,0,1));
			if (tile != NULL
				&& tile->hasNoFloor(*i) == true)
			{
				tile->addSmoke(var / 3);
			}

			for (int
					dir = 0;
					dir < 7;
					dir += 2)
			{
				if (RNG::percent(37) == true)
				{
					Position spreadPos;
					Pathfinding::directionToVector(
												dir,
												&spreadPos);
					tile = getTile((*i)->getPosition() + spreadPos);
					if (tile != NULL
						&& _tileEngine->horizontalBlockage(
														*i,
														tile,
														DT_SMOKE) == 0)
					{
						tile->addSmoke(var / 2);
					}
				}
			}
		}
	}
}

/**
 * Checks for and revives unconscious BattleUnits.
 * @param faction - the faction to check
 */
/* void SavedBattleGame::reviveUnits(const UnitFaction faction)
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getOriginalFaction() == faction
			&& (*i)->getStatus() != STATUS_DEAD) // etc. See below_
		{
			reviveUnit(*i, true);
		}
	}
} */

/**
 * Checks for units that are unconscious and revives them if they shouldn't be.
 * kL, does this still need a check to see if the unit revives
 * *on a floor* (if not, drop him/her down to a floor tile) <- yes, it does. (also raise up onto terrainLevel)
 * Revived units need a tile to stand on. If the unit's current position is occupied, then
 * all directions around the tile are searched for a free tile to place the unit in.
 * If no free tile is found the unit stays unconscious.
 * @param atTurnOver - true if called from SavedBattleGame::endBattlePhase (default false)
 */
void SavedBattleGame::reviveUnit(
		BattleUnit* const unit,
		bool atTurnOver)
{
	if (unit->getStatus() == STATUS_UNCONSCIOUS
		&& unit->getStun() < unit->getHealth() + static_cast<int>(atTurnOver) // do health=stun if unit is about to get healed in Prep Turn.
		&& (unit->getGeoscapeSoldier() != NULL
			|| (unit->getUnitRules()->isMechanical() == false
				&& unit->getArmor()->getSize() == 1)))
	{
		if (unit->getFaction() == FACTION_HOSTILE)	// faction will be Original here
			unit->setExposed();						// due to death/stun sequence.
		else
			unit->setExposed(-1);


		Position pos = unit->getPosition();

		if (pos == Position(-1,-1,-1)) // if carried
		{
			for (std::vector<BattleItem*>::const_iterator
					i = _items.begin();
					i != _items.end();
					++i)
			{
				if ((*i)->getUnit() != NULL
					&& (*i)->getUnit() == unit
					&& (*i)->getOwner() != NULL)
				{
					pos = (*i)->getOwner()->getPosition();
					break;
				}
			}
		}

		if (placeUnitNearPosition(
								unit,
								pos) == true)
		{
			unit->setStatus(STATUS_STANDING);

			if (unit->getGeoscapeSoldier() != NULL)
				unit->kneel(true);

			unit->setCache(NULL);

			unit->setDirection(RNG::generate(0,7));
			unit->setTimeUnits(0);
			unit->setEnergy(0);
			unit->setRevived();

			_tileEngine->calculateUnitLighting();
			_tileEngine->calculateFOV(unit->getPosition());

			removeCorpse(unit);
		}
	}
}

/**
 * Removes the body item (corpse) that corresponds to a unit.
 * @param unit - pointer to a BattleUnit
 */
void SavedBattleGame::removeCorpse(const BattleUnit* const unit)
{
	int part = unit->getArmor()->getSize() * unit->getArmor()->getSize();

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if ((*i)->getUnit() == unit)
		{
			removeItem(*i);
			--i;

			--part;
			if (part == 0)
				return;
		}
	}
}

/**
 * Places units on the map. Handles large units that are placed on multiple tiles.
 * @param unit		- pointer to a unit to be placed
 * @param pos		- reference the position to place the unit
 * @param testOnly	- true just checks if unit can be placed at the position (default false)
 * @return, true if unit was placed successfully
 */
bool SavedBattleGame::setUnitPosition(
		BattleUnit* unit,
		const Position& pos,
		bool testOnly)
{
	if (unit == NULL)
		return false;

	const int unitSize = unit->getArmor()->getSize() - 1;
	for (int // first check if the tiles are occupied
			x = unitSize;
			x != -1;
			--x)
	{
		for (int
				y = unitSize;
				y != -1;
				--y)
		{
			const Tile
				* const tile = getTile(pos + Position(x,y,0)),
				* const tileBelow = getTile(pos + Position(x,y,-1)),
				* const tileAbove = getTile(pos + Position(x,y, 1));

			if (tile == NULL
				|| (tile->getUnit() != NULL
					&& tile->getUnit() != unit)
				|| tile->getTUCost(MapData::O_OBJECT, unit->getMovementType()) == 255
				|| (tile->hasNoFloor(tileBelow)
					&& unit->getMovementType() != MT_FLY)
				|| (tile->getMapData(MapData::O_OBJECT)
					&& tile->getMapData(MapData::O_OBJECT)->getBigWall() > 0
					&& tile->getMapData(MapData::O_OBJECT)->getBigWall() < 4)
				|| (tileAbove
					&& tileAbove->getUnit() != NULL
					&& tileAbove->getUnit() != unit
					&& unit->getHeight() - tile->getTerrainLevel() > 26)) // don't stuck yer head up someone's flying arse.
				// note: no check for ceilings yet ....
			{
				return false;
			}
		}
	}

	if (unitSize != 0)
	{
		_pathfinding->setUnit(unit);

		for (int
				dir = 2;
				dir != 5;
				++dir)
		{
			if (_pathfinding->isBlocked(
									getTile(pos),
									NULL,
									dir) == true)
			{
				return false;
			}
		}
	}

	if (testOnly == true)
		return true;


	for (int // set the unit in position
			x = unitSize;
			x != -1;
			--x)
	{
		for (int
				y = unitSize;
				y != -1;
				--y)
		{
			if (x == 0 && y == 0)
			{
				unit->setPosition(pos);
//				unit->setTile(getTile(pos), getTile(pos - Position(0, 0, 1)));
			}

			getTile(pos + Position(x,y,0))->setUnit(
												unit,
												getTile(pos + Position(x,y,-1)));
		}
	}

	return true;
}

/**
 * Places a unit on or near a position.
 * @param unit	- pointer to a unit to place
 * @param pos	- the position around which to attempt to place the unit
 * @return, true if unit was successfully placed
 */
bool SavedBattleGame::placeUnitNearPosition(
		BattleUnit* unit,
		Position pos)
{
	if (unit == NULL)
		return false;

	if (setUnitPosition(
					unit,
					pos) == true)
	{
		return true;
	}


	const Tile* tile;
	const int dirRand = RNG::generate(0, 7);
	for (int
			dir = dirRand;
			dir != dirRand + 8;
			++dir)
	{
		Position posOffset;
		getPathfinding()->directionToVector(
										dir %8,
										&posOffset);

		tile = getTile(pos + posOffset);
		if (tile != NULL
			&& getPathfinding()->isBlocked(
										getTile(pos),
										tile,
										dir) == false
			&& setUnitPosition(
							unit,
							pos + posOffset) == true)
		{
			return true;
		}
	}

/*kL: uhh no.
	if (unit->getMovementType() == MT_FLY)
	{
		Tile* tile = getTile(pos + Position(0, 0, 1));
		if (tile
			&& tile->hasNoFloor(getTile(pos))
			&& setUnitPosition(
							unit,
							pos + Position(0, 0, 1)))
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
	for (std::vector<BattleUnit*>::const_iterator i = getUnits()->begin(); i != getUnits()->end(); ++i)
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
 * Adds this unit to the vector of falling units if it doesn't already exist there.
 * @param unit - the unit to add
 * @return, true if the unit was added
 */
bool SavedBattleGame::addFallingUnit(BattleUnit* const unit)
{
	for (std::list<BattleUnit*>::const_iterator
			i = _fallingUnits.begin();
			i != _fallingUnits.end();
			++i)
	{
		if (unit == *i)
			return false;
	}

	_fallingUnits.push_front(unit);
	_unitsFalling = true;

	return true;
}

/**
 * Gets all units in the battlescape that are falling.
 * @return, pointer to the list of pointers to the falling BattleUnits
 */
std::list<BattleUnit*>* SavedBattleGame::getFallingUnits()
{
	return &_fallingUnits;
}

/**
 * Toggles the switch that says "there are units falling, start the fall state".
 * @param fall, true if there are any units falling in the battlescap
 */
void SavedBattleGame::setUnitsFalling(bool fall)
{
	_unitsFalling = fall;
}

/**
 * Returns whether there are any units falling in the battlescape.
 * @return, true if there are any units falling in the battlescape
 */
bool SavedBattleGame::getUnitsFalling() const
{
	return _unitsFalling;
}

/**
 * Gets the highest ranked, living, non Mc'd unit of faction.
 * @param qtyAllies	- reference the number of allied units that are conscious and not MC'd
 * @param isXCOM	- true if examining Faction_Player, false for Faction_Hostile (default true)
 * @return, pointer to highest ranked BattleUnit of faction
 */
BattleUnit* SavedBattleGame::getHighestRanked(
		int& qtyAllies,
		bool isXCOM)
{
	//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked() xcom = " << xcom;
	BattleUnit* leader = NULL;
	qtyAllies = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if (*i != NULL
			&& (*i)->isOut(true, true) == false)
		{
			if (isXCOM == true)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is Xcom";
				if ((*i)->getOriginalFaction() == FACTION_PLAYER
					&& (*i)->getFaction() == FACTION_PLAYER)
				{
					++qtyAllies;

					if (leader == NULL
						|| (*i)->getRankInt() > leader->getRankInt())
					{
						leader = *i;
					}
				}
			}
			else if ((*i)->getOriginalFaction() == FACTION_HOSTILE
				&& (*i)->getFaction() == FACTION_HOSTILE)
			{
				//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked(), side is aLien";
				++qtyAllies;

				if (leader == NULL
					|| (*i)->getRankInt() < leader->getRankInt())
				{
					leader = *i;
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
 * Either a bonus/penalty for faction based on the highest ranked living unit
 * of the faction or a penalty for a single deceased BattleUnit.
 * @param unit		- pointer to BattleUnit deceased; higher rank is higher penalty (default NULL)
 * @param isXCOM	- if no unit is passed in this determines whether penalty applies to xCom or aLiens (default true)
 * @return, morale modifier
 */
int SavedBattleGame::getMoraleModifier( // note: Add bonus to aLiens for Cydonia & Final Assault.
		BattleUnit* unit,
		bool isXCOM)
{
	//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier()";
	if (unit != NULL
		&& unit->getOriginalFaction() == FACTION_NEUTRAL)
	{
		return 100;
	}

	int ret = 100;

	if (unit != NULL) // morale Loss when 'unit' slain
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), unit slain Penalty";
		if (unit->getOriginalFaction() == FACTION_PLAYER) // xCom dies. MC'd or not
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
			}
			//Log(LOG_INFO) << ". . xCom lossModifi = " << ret;
		}
		else if (unit->getFaction() == FACTION_HOSTILE) // aLien dies. MC'd aliens return 100, or 50 on Mars
		{
			switch (unit->getRankInt()) // soldiers are rank #5, terrorists are ranks #6 and #7
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
			}

			if (_missionType == "STR_MARS_CYDONIA_LANDING"
				|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
			{
				ret /= 2; // less hit for losing a unit on Cydonia.
			}
			//Log(LOG_INFO) << ". . aLien lossModifi = " << ret;
		}
	}
	else // leadership Bonus
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), leadership Bonus";
		BattleUnit* leader = NULL;
		int qtyAllies;

		if (isXCOM == true)
		{
			leader = getHighestRanked(qtyAllies);

			if (leader != NULL)
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
				}
			}
			//Log(LOG_INFO) << ". . xCom leaderModifi = " << ret;
		}
		else // aLien
		{
			leader = getHighestRanked(
									qtyAllies,
									false);
			if (leader != NULL)
			{
				switch (leader->getRankInt()) // terrorists are ranks #6 and #7
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
				}
			}

			if (_missionType == "STR_TERROR_MISSION"
				|| _missionType == "STR_ALIEN_BASE_ASSAULT"
				|| _missionType == "STR_BASE_DEFENSE")
			{
				ret += 50; // higher morale.
			}
			else if (_missionType == "STR_MARS_CYDONIA_LANDING"
						|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
			{
				ret += 100; // higher morale.
			}
			//Log(LOG_INFO) << ". . aLien leaderModifi = " << ret;
		}

		ret += qtyAllies - 9; // use 9 allies as Unity.
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
 *
 * @return, the tilesearch vector for use in AI functions
 */
const std::vector<Position> SavedBattleGame::getTileSearch()
{
	return _tileSearch;
}

/**
 * Gets if the AI has started to cheat.
 * @return, true if AI is cheating
 */
bool SavedBattleGame::isCheating()
{
	return _cheating;
}

/**
 * Gets the TU reserved type.
 * @return, a BattleActionType
 */
BattleActionType SavedBattleGame::getBATReserved() const
{
	return _batReserved;
}

/**
 * Sets the TU reserved type.
 * @param reserved - a BattleActionType
 */
void SavedBattleGame::setBATReserved(BattleActionType reserved)
{
	_batReserved = reserved;
}

/**
 * Gets the kneel reservation setting.
 * @return, true if an extra 4 TUs should be reserved to kneel
 */
bool SavedBattleGame::getKneelReserved() const
{
	return _kneelReserved;
}

/**
 * Sets the kneel reservation setting.
 * @param reserved - true if an extra 4 TUs should be reserved to kneel
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
 * @return, reference to a vector of vectors containing pairs of ints that make up base module damage maps
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
			const Tile* const tile = getTile(Position(
													x,
													y,
													_mapsize_z - 1));

			if (tile != NULL
				&& tile->getMapData(MapData::O_OBJECT) != NULL
				&& tile->getMapData(MapData::O_OBJECT)->isBaseModule() == true)
			{
				_baseModules[x / 10]
							[y / 10].first += _baseModules[x / 10][y / 10].first > 0 ? 1 : 2;
				_baseModules[x / 10]
							[y / 10].second = _baseModules[x / 10][y / 10].first;
			}
		}
	}
}

/**
 * Gets a pointer to the geoscape save.
 * @return, pointer to the geoscape save
 */
SavedGame* SavedBattleGame::getGeoscapeSave() const
{
	return _battleState->getGame()->getSavedGame();
}

/**
 * Gets the depth of the battlescape.
 * @return depth.
 */
int SavedBattleGame::getDepth() const
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
int SavedBattleGame::getAmbientSound() const
{
	return _ambience;
}

/**
 * Gets the list of items that are guaranteed to be recovered (ie: items that were in the skyranger).
 * @return, the list of items guaranteed recovered
 */
std::vector<BattleItem*>* SavedBattleGame::getGuaranteedRecoveredItems()
{
	return &_recoverGuaranteed;
}

/**
 * Gets the list of items that are not guaranteed to be recovered (ie: items that were NOT in the skyranger).
 * @return, the list of items conditionally recovered
 */
std::vector<BattleItem*>* SavedBattleGame::getConditionalRecoveredItems()
{
	return &_recoverConditional;
}

/**
 * Gets the music track for the current battle.
 * @return, the name of the music track
 */
/*std::string& SavedBattleGame::getMusic()
{
	return _music;
} */

/**
 * Sets the music track for this battle.
 * @param track - the track name
 */
void SavedBattleGame::setMusic(std::string track)
{
	_music = track;
}

/**
 * Sets the battlescape inventory tile when BattlescapeGenerator runs.
 * For use in base missions to randomize item locations.
 * @param invBattle - pointer to the tile where battle inventory is created
 */
void SavedBattleGame::setBattleInventory(Tile* invBattle)
{
	_invBattle = invBattle;
}

/**
 * Gets the inventory tile for preBattle InventoryState OK click.
 */
Tile* SavedBattleGame::getBattleInventory() const
{
	return _invBattle;
}

/**
 * Sets the alien race for this battle.
 * Currently used only for Base Defense missions, but should fill for other missions also.
 */
void SavedBattleGame::setAlienRace(const std::string& alienRace)
{
	_alienRace = alienRace;
}

/**
 * Gets the alien race participating in this battle.
 * Currently used only to get the alien race for SoldierDiary statistics
 * after a Base Defense mission.
 */
const std::string& SavedBattleGame::getAlienRace() const
{
	return _alienRace;
}

/**
 * Sets the ground level.
 * @param level - ground level as determined in BattlescapeGenerator.
 */
void SavedBattleGame::setGroundLevel(const int level)
{
	_groundLevel = level;
}

/**
 * Gets the ground level.
 * @return, ground level
 */
int SavedBattleGame::getGroundLevel() const
{
	return _groundLevel;
}

/**
 * Gets the operation title of the mission.
 * return, reference to the title
 */
const std::wstring& SavedBattleGame::getOperation() const
{
	return _operationTitle;
}

/**
 * Sets variables for what music to play in a particular terrain or lack thereof.
 * @param music		- address of the music category to play
 * @param terrain	- address of the terrain to choose music for
 */
void SavedBattleGame::calibrateMusic(
		std::string& music,
		std::string& terrain)
{
/*	if (_save->getMusic() == "")
		_game->getResourcePack()->playMusic("GMTACTIC", true);
	else
		_game->getResourcePack()->playMusic(_save->getMusic()); */

	if (_music.empty() == false)
		music = _music;
	else if (_missionType == "STR_UFO_CRASH_RECOVERY")
	{
		music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOCRASHED;
		terrain = _terrain;
	}
	else if (_missionType == "STR_UFO_GROUND_ASSAULT")
	{
		music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOLANDED;
		terrain = _terrain;
	}
	else if (_missionType == "STR_ALIEN_BASE_ASSAULT")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEASSAULT;
	else if (_missionType == "STR_BASE_DEFENSE")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEDEFENSE;
	else if (_missionType == "STR_TERROR_MISSION")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_TERRORSITE;
	else if (_missionType == "STR_MARS_CYDONIA_LANDING")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS1;
	else if (_missionType == "STR_MARS_THE_FINAL_ASSAULT")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS2;
	else
		music = OpenXcom::res_MUSIC_TAC_BATTLE; // default/ safety.
}

}
