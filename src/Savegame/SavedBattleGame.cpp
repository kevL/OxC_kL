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
SavedBattleGame::SavedBattleGame(const std::vector<OperationPool*>* const titles)
	:
		_battleState(NULL),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_mapSize(0),
		_selectedUnit(NULL),
		_lastSelectedUnit(NULL),
		_pf(NULL),
		_te(NULL),
		_tacticalShade(0),
		_side(FACTION_PLAYER),
		_turn(1),
		_debugMode(false),
		_aborted(false),
		_itemId(0),
		_objectiveType(-1),
		_objectivesDestroyed(0),
		_objectivesNeeded(0),
		_unitsFalling(false),
		_cheatAI(false),
//		_batReserved(BA_NONE),
//		_kneelReserved(false),
		_invBattle(NULL),
		_groundLevel(-1),
		_tacType(TCT_DEFAULT),
		_controlDestroyed(false),
		_tiles(NULL),
		_pacified(false),
		_rfTriggerPosition(0,0,-1)
//		_dragInvert(false),
//		_dragTimeTolerance(0),
//		_dragPixelTolerance(0)
{
	//Log(LOG_INFO) << "\nCreate SavedBattleGame";
//	static const size_t&
//		SEARCH_DIST = 11,
//		SEARCH_SIZE = SEARCH_DIST * SEARCH_DIST; // wtfn. It's c++!
	_tileSearch.resize(SEARCH_SIZE);
	for (size_t
			i = 0;
			i != SEARCH_SIZE;
			++i)
	{
		_tileSearch[i].x = (static_cast<int>(i % SEARCH_DIST)) - 5; // ie. -5 to +5
		_tileSearch[i].y = (static_cast<int>(i / SEARCH_DIST)) - 5; // ie. -5 to +5
	}
	/*	The '_tileSearch' array (Position):
		0		1		2		3		4		5		6		7		8		9		10
	[	-5,-5	-4,-5	-3,-5	-2,-5	-1,-5	0,-5	1,-5	2,-5	3,-5	4,-5	5,-5	//  0
		-5,-4	-4,-4	-3,-4	-2,-4	-1,-4	0,-4	1,-4	2,-4	3,-4	4,-4	5,-4	//  1
		-5,-3	-4,-3	-3,-3	-2,-3	-1,-3	0,-3	1,-3	2,-3	3,-3	4,-3	5,-3	//  2
		-5,-2	-4,-2	-3,-2	-2,-2	-1,-2	0,-2	1,-2	2,-2	3,-2	4,-2	5,-2	//  3
		-5,-1	-4,-1	-3,-1	-2,-1	-1,-1	0,-1	1,-1	2,-1	3,-1	4,-1	5,-1	//  4
		-5, 0	-4, 0	-3, 0	-2, 0	-1, 0	0, 0	1, 0	2, 0	3, 0	4, 0	5, 0	//  5
		-5, 1	-4, 1	-3, 1	-2, 1	-1, 1	0, 1	1, 1	2, 1	3, 1	4, 1	5, 1	//  6
		-5, 2	-4, 2	-3, 2	-2, 2	-1, 2	0, 2	1, 2	2, 2	3, 2	4, 2	5, 2	//  7
		-5, 3	-4, 3	-3, 3	-2, 3	-1, 3	0, 3	1, 3	2, 3	3, 3	4, 3	5, 3	//  8
		-5, 4	-4, 4	-3, 4	-2, 4	-1, 4	0, 4	1, 4	2, 4	3, 4	4, 4	5, 4	//  9
		-5, 5	-4, 5	-3, 5	-2, 5	-1, 5	0, 5	1, 5	2, 5	3, 5	4, 5	5, 5 ]	// 10
	*/

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
			i != _mapSize;
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

	delete _pf;
	delete _te;
}

/**
 * Loads the SavedBattleGame from a YAML file.
 * @param node		- reference a YAML node
 * @param rules		- pointer to the Ruleset
 * @param savedGame	- pointer to the SavedGame
 */
void SavedBattleGame::load(
		const YAML::Node& node,
		Ruleset* const rules,
		const SavedGame* const savedGame)
{
	//Log(LOG_INFO) << "SavedBattleGame::load()";
	_mapsize_x		= node["width"]		.as<int>(_mapsize_x);
	_mapsize_y		= node["length"]	.as<int>(_mapsize_y);
	_mapsize_z		= node["height"]	.as<int>(_mapsize_z);
	_tacticalType	= node["type"]		.as<std::string>(_tacticalType);
	_tacticalShade	= node["shade"]		.as<int>(_tacticalShade);
	_turn			= node["turn"]		.as<int>(_turn);
	_terrain		= node["terrain"]	.as<std::string>(_terrain);

	setTacType(_tacticalType);

	const int selectedUnit (node["selectedUnit"].as<int>());

	Log(LOG_INFO) << ". load mapdatasets";
	for (YAML::const_iterator
			i = node["mapdatasets"].begin();
			i != node["mapdatasets"].end();
			++i)
	{
		_mapDataSets.push_back(rules->getMapDataSet(i->as<std::string>()));
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
			const Position pos ((*i)["position"].as<Position>());
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
		const size_t totalTiles (node["totalTiles"].as<size_t>());

		std::memset(
				&serKey,
				0,
				sizeof(Tile::SerializationKey));

		// WARNING: Don't trust extracting integers from YAML as anything other than 'int' ...
		serKey.index			= node["tileIndexSize"]		.as<Uint8>(serKey.index);
		serKey.totalBytes		= node["tileTotalBytesPer"]	.as<Uint32>(serKey.totalBytes);
		serKey._fire			= node["tileFireSize"]		.as<Uint8>(serKey._fire);
		serKey._smoke			= node["tileSmokeSize"]		.as<Uint8>(serKey._smoke);
		serKey._animOffset		= node["tileOffsetSize"]	.as<Uint8>(serKey._animOffset);
		serKey._mapDataId		= node["tileIDSize"]		.as<Uint8>(serKey._mapDataId);
		serKey._mapDataSetId	= node["tileSetIDSize"]		.as<Uint8>(serKey._mapDataSetId);
		serKey.boolFields		= node["tileBoolFieldsSize"].as<Uint8>(1); // boolean flags used to be stored in an unmentioned byte (Uint8) :|

		// load binary tile data!
		const YAML::Binary binTiles (node["binTiles"].as<YAML::Binary>());

		Uint8
			* readBuffer ((Uint8*)binTiles.data()), // <- static_cast prob. here
			* const dataEnd (readBuffer + totalTiles * serKey.totalBytes);

		while (readBuffer < dataEnd)
		{
			int index (unserializeInt(&readBuffer, serKey.index));
			assert(
				index > -1
				&& index < static_cast<int>(_mapSize));

			_tiles[static_cast<size_t>(index)]->loadBinary(readBuffer, serKey); // loadBinary's privileges to advance *readBuffer have been revoked
			readBuffer += serKey.totalBytes - serKey.index;	// readBuffer is now incremented strictly by totalBytes in case there are obsolete fields present in the data
		}
	}

	if (_tacticalType == "STR_BASE_DEFENSE")
	{
		Log(LOG_INFO) << ". load xcom base";
		if (node["moduleMap"])
			_baseModules = node["moduleMap"].as<std::vector<std::vector<std::pair<int, int> > > >();
//		else
			// backwards compatibility: imperfect solution, modules that were completely destroyed
			// prior to saving and updating builds will be counted as indestructible.
//			calculateModuleMap();
	}

	Log(LOG_INFO) << ". load nodes";
	for (YAML::const_iterator
			i = node["nodes"].begin();
			i != node["nodes"].end();
			++i)
	{
		Node* const nod (new Node());
		nod->load(*i);
		_nodes.push_back(nod);
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
		id				= (*i)["id"]										.as<int>();
		faction			= static_cast<UnitFaction>((*i)["faction"]			.as<int>());
		originalFaction	= static_cast<UnitFaction>((*i)["originalFaction"]	.as<int>(faction));

		const GameDifficulty diff (savedGame->getDifficulty());
		if (id < BattleUnit::MAX_SOLDIER_ID)	// BattleUnit is linked to a geoscape soldier
		{
			unit = new BattleUnit(				// look up the matching soldier
							savedGame->getSoldier(id),
							diff);				// kL_add: For VictoryPts value per death.
		}
		else									// create a new Unit, not-soldier but Vehicle, Civie, or aLien.
		{
			const std::string
				type	((*i)["genUnitType"]	.as<std::string>()),
				armor	((*i)["genUnitArmor"]	.as<std::string>());

			if (rules->getUnit(type) != NULL && rules->getArmor(armor) != NULL) // safeties.
				unit = new BattleUnit(
									rules->getUnit(type),
									originalFaction,
									id,
									rules->getArmor(armor),
									diff,
									savedGame->getMonthsPassed());
			else unit = NULL;
		}

		if (unit != NULL)
		{
			Log(LOG_INFO) << ". . load unit " << id;
			unit->load(*i);
			_units.push_back(unit);

			if (faction == FACTION_PLAYER)
			{
				if (unit->getId() == selectedUnit
					|| (_selectedUnit == NULL && unit->isOut_t(OUT_STAT) == false))
				{
					_selectedUnit = unit;
				}
			}

			if (faction != FACTION_PLAYER
				&& unit->getUnitStatus() != STATUS_DEAD)
			{
				if (const YAML::Node& ai = (*i)["AI"])
				{
					BattleAIState* aiState;

					if (faction == FACTION_HOSTILE)
						aiState = new AlienBAIState(this, unit, NULL);
					else
						aiState = new CivilianBAIState(this, unit, NULL);

					aiState->load(ai);
					unit->setAIState(aiState);
				}
			}
		}
	}

	// load _hostileUnitsThisTurn here:
	// convert unitID's into pointers to BattleUnit
	for (size_t
			i = 0;
			i != _units.size();
			++i)
	{
		_units.at(i)->loadSpotted(this);
	}


	// matches up tiles and units
	Log(LOG_INFO) << ". reset tiles";
	resetUnitsOnTiles();

	Log(LOG_INFO) << ". load items";
	const size_t CONTAINERS = 3;
	std::string fromContainer[CONTAINERS] =
	{
		"items",
		"recoverConditional",
		"recoverGuaranteed"
	};
	std::vector<BattleItem*>* toContainer[CONTAINERS] =
	{
		&_items,
		&_recoverConditional,
		&_recoverGuaranteed
	};

	for (size_t
			pass = 0;
			pass != CONTAINERS;
			++pass)
	{
		for (YAML::const_iterator
				i = node[fromContainer[pass]].begin();
				i != node[fromContainer[pass]].end();
				++i)
		{
			std::string type = (*i)["type"].as<std::string>();
			if (rules->getItem(type) != NULL)
			{
				id = (*i)["id"].as<int>(-1);
				BattleItem* const item (new BattleItem(
													rules->getItem(type),
													NULL,
													id));

				item->load(*i);
				type = (*i)["inventoryslot"].as<std::string>();

				if (type != "NULL")
					item->setSlot(rules->getInventory(type));

				const int
					owner		((*i)["owner"]			.as<int>()),
					prevOwner	((*i)["previousOwner"]	.as<int>(-1)),
					unit		((*i)["unit"]			.as<int>());

				for (std::vector<BattleUnit*>::const_iterator // match up items and units
						bu = _units.begin();
						bu != _units.end();
						++bu)
				{
					if ((*bu)->getId() == owner)
						item->moveToOwner(*bu);

					if ((*bu)->getId() == unit)
						item->setUnit(*bu);
				}

				for (std::vector<BattleUnit*>::const_iterator
						bu = _units.begin();
						bu != _units.end();
						++bu)
				{
					if ((*bu)->getId() == prevOwner)
						item->setPreviousOwner(*bu);
				}


				if (item->getSlot() != NULL // match up items and tiles
					&& item->getSlot()->getType() == INV_GROUND)
				{
					const Position pos ((*i)["position"].as<Position>());

					if (pos.x != -1)
						getTile(pos)->addItem(
											item,
											rules->getInventory("STR_GROUND"));
				}

				toContainer[pass]->push_back(item);
			}
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
		if (rules->getItem((*i)["type"].as<std::string>()) != NULL)
		{
			const int ammo ((*i)["ammoItem"].as<int>());
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
	_objectiveType			= node["objectiveType"]			.as<int>(_objectiveType);
	_objectivesDestroyed	= node["objectivesDestroyed"]	.as<int>(_objectivesDestroyed);
	_objectivesNeeded		= node["objectivesNeeded"]		.as<int>(_objectivesNeeded);
	_alienRace				= node["alienRace"]				.as<std::string>(_alienRace);
//	_kneelReserved			= node["kneelReserved"]			.as<bool>(_kneelReserved);

//	_batReserved = static_cast<BattleActionType>(node["batReserved"].as<int>(_batReserved));
	_operationTitle = Language::utf8ToWstr(node["operationTitle"].as<std::string>());


	if (node["controlDestroyed"])
		_controlDestroyed = node["controlDestroyed"].as<bool>();


	_music = node["music"].as<std::string>(_music);

	Log(LOG_INFO) << ". set item ID";
	setNextItemId();
	//Log(LOG_INFO) << "SavedBattleGame::load() EXIT";

	// TEST, reveal all tiles
//	for (size_t i = 0; i != _mapSize; ++i)
//		_tiles[i]->setDiscovered(true, 2);
}

/**
 * Loads the resources required by the map in the battle save.
 * @param game - pointer to Game
 */
void SavedBattleGame::loadMapResources(const Game* const game)
{
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
		mapDataId,
		mapDataSetId,
		parts = static_cast<int>(Tile::PARTS_TILE);
	MapDataType partType;

	for (size_t
			i = 0;
			i != _mapSize;
			++i)
	{
		for (int
				part = 0;
				part != parts;
				++part)
		{
			partType = static_cast<MapDataType>(part);
			_tiles[i]->getMapData(
								&mapDataId,
								&mapDataSetId,
								partType);

			if (mapDataId != -1
				&& mapDataSetId != -1)
			{
				_tiles[i]->setMapData(
								_mapDataSets[static_cast<size_t>(mapDataSetId)]->getObjects()->at(static_cast<size_t>(mapDataId)),
								mapDataId,
								mapDataSetId,
								partType);
			}
		}
	}

	initUtilities(game->getResourcePack());

	_te->calculateSunShading();
	_te->calculateTerrainLighting();
	_te->calculateUnitLighting();
//	_te->recalculateFOV(); // -> moved to BattlescapeGame::init()
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
		node["objectiveType"]		= _objectiveType;
		node["objectivesDestroyed"]	= _objectivesDestroyed;
		node["objectivesNeeded"]	= _objectivesNeeded;
	}

	node["width"]			= _mapsize_x;
	node["length"]			= _mapsize_y;
	node["height"]			= _mapsize_z;
	node["type"]			= _tacticalType;
	node["shade"]			= _tacticalShade;
	node["turn"]			= _turn;
	node["terrain"]			= _terrain;
	node["selectedUnit"]	= (_selectedUnit != NULL) ? _selectedUnit->getId() : -1;

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
			i != _mapSize;
			++i)
	{
		if (_tiles[i]->isVoid() == false)
			node["tiles"].push_back(_tiles[i]->save());
	}
#else
	// write out the field sizes used to write the tile data
	node["tileIndexSize"]		= Tile::serializationKey.index;
	node["tileTotalBytesPer"]	= Tile::serializationKey.totalBytes;
	node["tileFireSize"]		= Tile::serializationKey._fire;
	node["tileSmokeSize"]		= Tile::serializationKey._smoke;
	node["tileOffsetSize"]		= Tile::serializationKey._animOffset;
	node["tileIDSize"]			= Tile::serializationKey._mapDataId;
	node["tileSetIDSize"]		= Tile::serializationKey._mapDataSetId;
	node["tileBoolFieldsSize"]	= Tile::serializationKey.boolFields;

	size_t tilesDataSize = static_cast<size_t>(Tile::serializationKey.totalBytes) * _mapSize;
	Uint8
		* const tilesData = static_cast<Uint8*>(calloc(tilesDataSize, 1)),
		* writeBuffer = tilesData;

	for (size_t
			i = 0;
			i != _mapSize;
			++i)
	{
		serializeInt( // <- save ALL Tiles. (Stop void tiles returning undiscovered postReload.)
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
			tilesDataSize -= Tile::serializationKey.totalBytes; */
	}

	node["totalTiles"]	= tilesDataSize / static_cast<size_t>(Tile::serializationKey.totalBytes); // not strictly necessary, just convenient
	node["binTiles"]	= YAML::Binary(tilesData, tilesDataSize);

	std::free(tilesData);
#endif

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		node["nodes"].push_back((*i)->save());
	}

	if (_tacticalType == "STR_BASE_DEFENSE")
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

//	node["batReserved"]		= static_cast<int>(_batReserved);
//	node["kneelReserved"]	= _kneelReserved;
	node["alienRace"]		= _alienRace;
	node["operationTitle"]	= Language::wstrToUtf8(_operationTitle);

	if (_controlDestroyed == true)
		node["controlDestroyed"] = _controlDestroyed;

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
 * Deletes the old and initializes a new array of tiles.
 * @param mapsize_x -
 * @param mapsize_y -
 * @param mapsize_z -
 */
void SavedBattleGame::initMap(
		const int mapsize_x,
		const int mapsize_y,
		const int mapsize_z)
{
	if (_nodes.empty() == false) // Delete old stuff
	{
		_mapSize = static_cast<size_t>(_mapsize_x * _mapsize_y * _mapsize_z);
		for (size_t
				i = 0;
				i != _mapSize;
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

	_mapsize_x = mapsize_x; // Create tile objects
	_mapsize_y = mapsize_y;
	_mapsize_z = mapsize_z;
	_mapSize = static_cast<size_t>(mapsize_z * mapsize_y * mapsize_x);

	_tiles = new Tile*[_mapSize];

	for (size_t
			i = 0;
			i != _mapSize;
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
 * @param res - pointer to ResourcePack
 */
void SavedBattleGame::initUtilities(const ResourcePack* const res)
{
	delete _pf;
	delete _te;

	_pf = new Pathfinding(this);
	_te = new TileEngine(
					this,
					res->getVoxelData());
}

/**
 * Sets the TacticalType based on the battle Type.
 * @param type - reference the battle type
 */
void SavedBattleGame::setTacType(const std::string& type) // private.
{
	if (type.compare("STR_UFO_CRASH_RECOVERY") == 0)
		_tacType = TCT_UFOCRASHED;
	else if (type.compare("STR_UFO_GROUND_ASSAULT") == 0)
		_tacType = TCT_UFOLANDED;
	else if (type.compare("STR_BASE_DEFENSE") == 0)
		_tacType = TCT_BASEDEFENSE;
	else if (type.compare("STR_ALIEN_BASE_ASSAULT") == 0)
		_tacType = TCT_BASEASSAULT;
	else if (type.compare("STR_TERROR_MISSION") == 0
		|| type.compare("STR_PORT_ATTACK") == 0)
	{
		_tacType = TCT_MISSIONSITE;
	}
	else if (type.compare("STR_MARS_CYDONIA_LANDING") == 0)
		_tacType = TCT_MARS1;
	else if (type.compare("STR_MARS_THE_FINAL_ASSAULT") == 0)
		_tacType = TCT_MARS2;
	else
		_tacType = TCT_DEFAULT; // <- the default should probly be TCT_UFOCRASHED.
}

/**
 * Gets the TacticalType of this battle.
 * @return, the TacticalType (SavedBattleGame.h)
 */
TacticalType SavedBattleGame::getTacType() const
{
	return _tacType;
}

/**
 * Sets the mission type.
 * @param type - reference a mission type
 */
void SavedBattleGame::setTacticalType(const std::string& type)
{
	_tacticalType = type;
	setTacType(_tacticalType);
}

/**
 * Gets the mission type.
 * @note This should return a const ref
 * except perhaps when there's a nextStage that deletes this SavedBattleGame ...
 * and creates a new one wherein the ref is no longer valid.
 * @return, the mission type
 */
std::string SavedBattleGame::getTacticalType() const
{
	return _tacticalType;
}

/**
 * Sets the tactical shade.
 * @param shade - the tactical shade
 */
void SavedBattleGame::setTacticalShade(int shade)
{
	_tacticalShade = shade;
}

/**
 * Gets the tactical shade.
 * @return, the tactical shade
 */
int SavedBattleGame::getTacticalShade() const
{
	return _tacticalShade;
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
 * Gets the qty of tiles on the battlefield.
 * @return, the total map-size in tiles
 */
size_t SavedBattleGame::getMapSizeXYZ() const
{
	return _mapSize;
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
		size_t index,
		int* x,
		int* y,
		int* z) const
{
	const int idx = static_cast<int>(index);

	*z =  idx / (_mapsize_y * _mapsize_x);
	*y = (idx % (_mapsize_y * _mapsize_x)) / _mapsize_x;
	*x = (idx % (_mapsize_y * _mapsize_x)) % _mapsize_x;
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
void SavedBattleGame::setSelectedUnit(BattleUnit* const unit)
{
	_selectedUnit = unit;
}

/**
* Selects the previous player unit.
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param dontReselect		- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or NULL if none can be selected
* @sa selectFactionUnit
*/
BattleUnit* SavedBattleGame::selectPreviousFactionUnit(
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
	return selectFactionUnit(
						-1,
						checkReselect,
						dontReselect,
						checkInventory);
}

/**
 * Selects the next player unit.
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param dontReselect		- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or NULL if none can be selected
 * @sa selectFactionUnit
 */
BattleUnit* SavedBattleGame::selectNextFactionUnit(
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
	return selectFactionUnit(
						+1,
						checkReselect,
						dontReselect,
						checkInventory);
}

/**
 * Selects the next player unit in a certain direction.
 * @param dir				- direction to select (1 for next and -1 for previous)
 * @param checkReselect		- true to check the reselectable flag (default false)
 * @param dontReselect		- true to set the reselectable flag FALSE (default false)
 * @param checkInventory	- true to check if the unit has an inventory (default false)
 * @return, pointer to newly selected BattleUnit or NULL if none can be selected
 */
BattleUnit* SavedBattleGame::selectFactionUnit( // private.
		int dir,
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
	if (_units.empty() == true)
	{
		_selectedUnit = NULL;
		_lastSelectedUnit = NULL;

		return NULL;
	}

	if (dontReselect == true
		&& _selectedUnit != NULL)
	{
		_selectedUnit->dontReselect();
	}


	std::vector<BattleUnit*>::const_iterator
		firstUnit,
		lastUnit;

	if (dir > 0)
	{
		firstUnit = _units.begin();
		lastUnit = _units.end() - 1;
	}
	else
	{
		firstUnit = _units.end() - 1;
		lastUnit = _units.begin();
	}

	std::vector<BattleUnit*>::const_iterator i = std::find(
														_units.begin(),
														_units.end(),
														_selectedUnit);
	do
	{
		if (i == _units.end()) // no unit selected
		{
			i = firstUnit;
			continue;
		}

		if (i != lastUnit)
			i += dir;
		else // reached the end, wrap-around
			i = firstUnit;

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
			&& i == firstUnit)
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

	if (bu == NULL
		|| bu->isOut_t(OUT_STAT) == true)
//		|| bu->isOut(true, true) == true)
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
	return _pf;
}

/**
 * Gets the terrain modifier object.
 * @return, pointer to the TileEngine
 */
TileEngine* SavedBattleGame::getTileEngine() const
{
	return _te;
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
	for (std::vector<BattleUnit*>::const_iterator // -> would it be safe to exclude Dead & Unconscious units
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
		_scanDots.clear();

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
		//Log(LOG_INFO) << ". end Hostile phase -> NEUTRAL";
		_side = FACTION_NEUTRAL;

		// if there is no neutral team, skip this section
		// and instantly prepare new turn for the player.
		if (selectNextFactionUnit() == NULL) // else this will cycle through NEUTRAL units
		{
			//Log(LOG_INFO) << ". no neutral units to select ... -> PLAYER";
			tileVolatiles(); // do Tile stuff
			++_turn;
			ret = true;

			_side = FACTION_PLAYER;

			if (_lastSelectedUnit != NULL
				&& _lastSelectedUnit->isSelectable(FACTION_PLAYER) == true)
			{
				//Log(LOG_INFO) << ". . last Selected = " << _lastSelectedUnit->getId();
				_selectedUnit = _lastSelectedUnit;
			}
			else
			{
				//Log(LOG_INFO) << ". . select next";
				selectNextFactionUnit();
			}

			while (_selectedUnit != NULL
				&& _selectedUnit->getFaction() != FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". . select next loop";
				selectNextFactionUnit(true);
			}

			//if (_selectedUnit != NULL) Log(LOG_INFO) << ". -> selected Unit = " << _selectedUnit->getId();
			//else Log(LOG_INFO) << ". -> NO UNIT TO SELECT FOUND";
		}
	}
	else if (_side == FACTION_NEUTRAL) // end of Civilian turn.
	{
		//Log(LOG_INFO) << ". end Neutral phase -> PLAYER";
		tileVolatiles(); // do Tile stuff
		++_turn;
		ret = true;

		_side = FACTION_PLAYER;

		if (_lastSelectedUnit != NULL
			&& _lastSelectedUnit->isSelectable(FACTION_PLAYER) == true)
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

	// pseudo the Turn-20 / less-than-3-aliens-left Reveal rule.
	if (_cheatAI == false
		&& _side == FACTION_HOSTILE
		&& _turn > 5)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = _units.begin();
				i != _units.end();
				++i)
		{
			if ((*i)->isOut_t(OUT_STAT) == false // a conscious non-MC'd aLien ...
				&& (*i)->getOriginalFaction() == FACTION_HOSTILE
				&& (*i)->getFaction() == FACTION_HOSTILE)
			{
				const int delta = RNG::generate(0,5);
				if (_turn > 17 + delta
					|| (_turn > 5
						&& liveAliens < delta - 1))
				{
					_cheatAI = true;
				}

				break;
			}
		}
	}

	//Log(LOG_INFO) << ". side = " << (int)_side;
	for (std::vector<BattleUnit*>::const_iterator // -> would it be safe to exclude Dead & Unconscious units
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->getUnitStatus() != STATUS_LIMBO)
		{
			if ((*i)->isOut_t(OUT_HLTH) == false)
			{
				(*i)->setDashing(false);	// Safety. no longer dashing; dash is effective
											// vs. Reaction Fire only and is/ought be
											// reset/removed every time BattlescapeGame::primaryAction()
											// uses the Pathfinding object. Other, more ideal
											// places for this safety are UnitWalkBState dTor
											// and/or BattlescapeGame::popState().
				if ((*i)->getOriginalFaction() == _side)
				{
					reviveUnit(*i, true);
					(*i)->takeFire();
				}

				if ((*i)->getFaction() == _side)	// This causes an Mc'd unit to lose its turn.
					(*i)->prepUnit();				// REVERTS FACTION, does tu/stun recovery, Fire damage, etc.
				// if newSide=XCOM, xCom agents DO NOT revert to xCom; MC'd aLiens revert to aLien.
				// if newSide=Alien, xCom agents revert to xCom; MC'd aLiens DO NOT revert to aLien.

				if ((*i)->getFaction() == FACTION_HOSTILE
					|| (*i)->getOriginalFaction() == FACTION_HOSTILE
					|| _cheatAI == true) // aLiens know where xCom is when cheating ~turn20
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
			else if ((*i)->getFaction() == _side
				&& (*i)->getFireUnit() != 0)
			{
				(*i)->setFireUnit((*i)->getFireUnit() - 1);
			}
		}
	}

	_te->calculateSunShading();
	_te->calculateTerrainLighting();
	_te->calculateUnitLighting(); // turn off MCed alien lighting.

	// redo calculateFOV() *after* aliens & civies have been set
	// notVisible -> AND *only* after a calcLighting has been done !
	_te->recalculateFOV();

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
			i != _mapSize;
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
 * @note There can be cases when BattlescapeGame is valid but BattlescapeState
 * is not; during battlescape generation for example -> CTD. So fix it ....
 * @return, pointer to the BattlescapeGame
 */
BattlescapeGame* SavedBattleGame::getBattleGame() const
{
	if (_battleState == NULL)
		return NULL;

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
void SavedBattleGame::resetUnitsOnTiles()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false)
		{
			const int armorSize = (*i)->getArmor()->getSize() - 1;

			if ((*i)->getTile() != NULL // remove unit from its current tile
				&& (*i)->getTile()->getUnit() == *i) // wtf, is this super-safety ......
			{
				for (int
						x = armorSize;
						x != -1;
						--x)
				{
					for (int
							y = armorSize;
							y != -1;
							--y)
					{
						getTile((*i)->getTile()->getPosition() + Position(x,y,0))->setUnit(NULL);
					}
				}
			}

			for (int // set unit onto its proper tile
					x = armorSize;
					x != -1;
					--x)
			{
				for (int
						y = armorSize;
						y != -1;
						--y)
				{
					Tile* const tile = getTile((*i)->getPosition() + Position(x,y,0));
					tile->setUnit(
								*i,
								getTile(tile->getPosition() + Position(0,0,-1)));
				}
			}
		}

		if ((*i)->getFaction() == FACTION_PLAYER)
			(*i)->setUnitVisible();
	}
}

/**
 * Gives access to the storageSpace vector for distribution of items in base
 * defense missions.
 * @return, reference a vector of storage positions
 */
std::vector<Position>& SavedBattleGame::getStorageSpace()
{
	return _storageSpace;
}

/**
 * Move all the leftover items in base defense missions to random locations in
 * the storage facilities.
 * @param tile - pointer to a tile where all the goodies are initially stored
 */
void SavedBattleGame::randomizeItemLocations(Tile* const tile)
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
				getTile(_storageSpace.at(RNG::pick(_storageSpace.size())))->addItem(*i, (*i)->getSlot());
				i = tile->getInventory()->erase(i);
			}
			else ++i;
		}
	}
}

/**
 * Removes an item from the game - when ammo item is depleted for example.
 * @note Items need to be checked for removal from three vectors:
 *		- tile inventory
 *		- battleunit inventory
 *		- battlegame-items container
 * Upon removal the pointer to the item is kept in the '_deleted' vector that is
 * flushed and destroyed in the SavedBattleGame dTor.
 * @param item - pointer to an item to remove
 */
void SavedBattleGame::removeItem(BattleItem* const item)
{
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

	BattleUnit* const unit = item->getOwner();
	if (unit != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			if (*i == item)
			{
				unit->getInventory()->erase(i);
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
			break;
		}
	}

	_deleted.push_back(item);
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
 * Changes the objectives-needed count.
 * @note Used only to initialize the objective counter; cf addDestroyedObjective() below.
 * @note Objectives were tile-parts marked w/ MUST_DESTROY in their MCD but now
 * can be any specially marked tile. See elsewhere.
 */
void SavedBattleGame::setObjectiveCount(int qty)
{
	_objectivesNeeded = qty;
	_objectivesDestroyed = 0;
}

/**
 * Increments the objectives-destroyed count.
 */
void SavedBattleGame::addDestroyedObjective()
{
	if (allObjectivesDestroyed() == false)
	{
		++_objectivesDestroyed;

		if (allObjectivesDestroyed() == true)
		{
			_controlDestroyed = true;
			_battleState->getBattleGame()->objectiveDone();
		}
	}
}

/**
 * Returns whether the objectives are destroyed.
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
		id = -1,
		idTest;

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		idTest = (*i)->getId();
		if (idTest > id)
			id = idTest;
	}

	_itemId = ++id;
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
 * @note bgen.addAlien() uses a fallback mechanism to test assorted nodeRanks.
 * @param unitRank	- rank of the unit attempting to spawn
 * @param unit		- pointer to the unit (to test-set its position)
 * @return, pointer to the chosen node
 */
Node* SavedBattleGame::getSpawnNode(
		int unitRank,
		BattleUnit* const unit)
{
	std::vector<Node*> spawnNodes;

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		if ((*i)->getPriority() != 0						// spawn-priority 0 is not spawnplace
			&& (*i)->getNodeRank() == unitRank				// ranks must match
			&& isNodeType(*i, unit)
//			&& (!((*i)->getNodeType() & Node::TYPE_SMALL)	// the small unit bit is not set on the node
//				|| unit->getArmor()->getSize() == 1)			// or the unit is small
//			&& (!((*i)->getNodeType() & Node::TYPE_FLYING)	// the flying unit bit is not set on the node
//				|| unit->getMovementType() == MT_FLY)			// or the unit can fly
			&& setUnitPosition(								// check if unit can be set at this node
							unit,								// ie. it's big enough
							(*i)->getPosition(),				// and there's not already a unit there.
							true) == true)					// testOnly, runs again w/ FALSE on return to bgen::addAlien()
		{
			for (int										// weight each eligible node by its Priority.
					j = (*i)->getPriority();
					j != 0;
					--j)
			{
				spawnNodes.push_back(*i);
			}
		}
	}

	if (spawnNodes.empty() == false)
		return spawnNodes[RNG::pick(spawnNodes.size())];

	return NULL;
}

/**
 * Finds a fitting node where a given unit can patrol to.
 * @param scout		- true if the unit is scouting
 * @param unit		- pointer to a BattleUnit
 * @param fromNode	- pointer to the node that unit is currently at
 * @return, pointer to the destination Node
 */
Node* SavedBattleGame::getPatrolNode(
		bool scout,
		BattleUnit* const unit,
		Node* fromNode)
{
	//Log(LOG_INFO) << "SavedBattleGame::getPatrolNode()";
	if (fromNode == NULL)
		fromNode = getNearestNode(unit);

	std::vector<Node*>
		scoutNodes,
		rankedNodes;
	Node* node;

	size_t nodeQty;
	if (scout == true)
		nodeQty = getNodes()->size();
	else
		nodeQty = fromNode->getNodeLinks()->size();

	for (size_t
			i = 0;
			i != nodeQty;
			++i)
	{
		if (scout == true
			|| fromNode->getNodeLinks()->at(i) != 0)							// non-scouts need Links to travel along.
		{
			if (scout == true)
				node = getNodes()->at(i);
			else
				node = getNodes()->at(static_cast<size_t>(fromNode->getNodeLinks()->at(i)));

			if ((node->getPatrol() != 0
					|| node->getNodeRank() > NR_SCOUT
					|| scout == true)										// for non-scouts find a node with a desirability above 0
				&& node->isAllocated() == false								// check if not allocated
				&& isNodeType(node, unit)
//				&& (!(node->getNodeType() & Node::TYPE_SMALL)				// the small unit bit is not set
//					|| unit->getArmor()->getSize() == 1)						// or the unit is small
//				&& (!(node->getNodeType() & Node::TYPE_FLYING)				// the flying unit bit is not set
//					|| unit->getMovementType() == MT_FLY)						// or the unit can fly
//				&& !(node->getNodeType() & Node::TYPE_DANGEROUS)			// don't go there if an alien got shot there; stupid behavior like that
				&& setUnitPosition(											// check if unit can be set at this node
								unit,											// ie. it's big enough
								node->getPosition(),							// and there's not already a unit there.
								true) == true									// but don't actually set the unit...
				&& getTile(node->getPosition()) != NULL						// the node is on a valid tile
				&& getTile(node->getPosition())->getFire() == 0				// you are not a firefighter; do not patrol into fire
				&& (getTile(node->getPosition())->getDangerous() == false	// aliens don't run into a grenade blast
					|| unit->getFaction() != FACTION_HOSTILE)					// but civies do!
				&& (node != fromNode										// scouts push forward
					|| scout == false)											// others can mill around.. ie, stand there
				&& node->getPosition().x > -1								// x-pos valid
				&& node->getPosition().y > -1)								// y-pos valid
			{
				for (int													// weight each eligible node by its Flags.
						j = node->getPatrol();
						j != -1;
						--j)
				{
					scoutNodes.push_back(node);

					if (scout == false
						&& node->getNodeRank() == Node::nodeRank[static_cast<size_t>(unit->getRankInt())]
																[0])			// high-class node here.
					{
						rankedNodes.push_back(node);
					}
				}
			}
		}
	}

	if (scoutNodes.empty() == true)
	{
		//Log(LOG_INFO) << " . scoutNodes is EMPTY.";
		if (scout == false && unit->getArmor()->getSize() > 1)
		{
//			return Sectopod::CTD();
			return getPatrolNode(true, unit, fromNode);
		}

		//Log(LOG_INFO) << " . return NULL";
		return NULL;
	}
	//Log(LOG_INFO) << " . scoutNodes is NOT Empty.";

	if (scout == true // picks a random destination
		|| rankedNodes.empty() == true
		|| RNG::percent(19) == true) // officers can go for a stroll ...
	{
		//Log(LOG_INFO) << " . scout";
		//Log(LOG_INFO) << " . return scoutNodes @ " << pick;
		return scoutNodes[RNG::pick(scoutNodes.size())];
	}
	//Log(LOG_INFO) << " . !scout";

	//Log(LOG_INFO) << " . return scoutNodes @ " << pick;
	return rankedNodes[RNG::pick(rankedNodes.size())];
}

/**
 * Gets the node considered nearest to a BattleUnit.
 * @note Assume closest node is on same level to avoid strange things.
 * @note The node has to match unit size or the AI will freeze.
 * @param unit - pointer to a BattleUnit
 * @return, the nearest node
 */
Node* SavedBattleGame::getNearestNode(const BattleUnit* const unit) const
{
	Node
		* node = NULL,
		* nodeTest;
	int
		distSqr = 100000,
		distTest;

	for (std::vector<Node*>::const_iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		nodeTest = *i;
		distTest = TileEngine::distanceSqr(
										unit->getPosition(),
										nodeTest->getPosition());

		if (unit->getPosition().z == nodeTest->getPosition().z
			&& distTest < distSqr
			&& (unit->getArmor()->getSize() == 1
				|| !(nodeTest->getNodeType() & Node::TYPE_SMALL)))
		{
			distSqr = distTest;
			node = nodeTest;
		}
	}

	return node;
}

/**
 * Gets if a BattleUnit can use a particular Node.
 * @note Small units are allowed to use Large nodes and flying units are
 * allowed to use nonFlying nodes.
 * @param node - pointer to a node
 * @param unit - pointer to a unit trying to use the node
 * @return, true if unit can use node
 */
bool SavedBattleGame::isNodeType(
		const Node* const node,
		const BattleUnit* const unit) const
{
	const int type = node->getNodeType();

	if (type & Node::TYPE_DANGEROUS)
		return false;

	if (type == 0)
		return true;


	if (type & Node::TYPE_FLYING)
		return unit->getMoveTypeUnit() == MT_FLY
			&& unit->getArmor()->getSize() == 1;

	if (type & Node::TYPE_SMALL)
		return unit->getArmor()->getSize() == 1;

	if (type & Node::TYPE_LARGEFLYING)
		return unit->getMoveTypeUnit() == MT_FLY;

	return true;
}

/**
 * Carries out new turn preparations such as fire and smoke spreading.
 * @note Also explodes any explosive tiles that get destroyed by fire.
 */
void SavedBattleGame::tileVolatiles()
{
	std::vector<Tile*>
		tilesFired,
		tilesSmoked;

	for (size_t
			i = 0;
			i != _mapSize;
			++i)
	{
		if (getTiles()[i]->getFire() != 0)
			tilesFired.push_back(getTiles()[i]);

		if (getTiles()[i]->getSmoke() != 0)
			tilesSmoked.push_back(getTiles()[i]);

		getTiles()[i]->setDangerous(false); // reset.
	}

	//Log(LOG_INFO) << "tilesFired.size = " << tilesFired.size();
	//Log(LOG_INFO) << "tilesSmoked.size = " << tilesSmoked.size();

	Tile* tile;
	int var;

	for (std::vector<Tile*>::const_iterator
			i = tilesFired.begin();
			i != tilesFired.end();
			++i)
	{
		(*i)->decreaseFire();

		var = (*i)->getFire() << 4;

		if (var != 0)
		{
			for (int
					dir = 0;
					dir != 8;
					dir += 2)
			{
				Position spreadPos;
				Pathfinding::directionToVector(dir, &spreadPos);
				tile = getTile((*i)->getPosition() + spreadPos);

				if (tile != NULL && _te->horizontalBlockage(*i, tile, DT_IN) == 0)
					tile->ignite(var);
			}
		}
		else
		{
			if ((*i)->getMapData(O_OBJECT) != NULL)
			{
				if ((*i)->getMapData(O_OBJECT)->getFlammable() != 255
					&& (*i)->getMapData(O_OBJECT)->getArmor() != 255)
				{
					if ((*i)->destroyTile(O_OBJECT, getObjectiveType()) == true)
						addDestroyedObjective();

					if ((*i)->destroyTile(O_FLOOR, getObjectiveType()) == true)
						addDestroyedObjective();
				}
			}
			else if ((*i)->getMapData(O_FLOOR) != NULL)
			{
				if ((*i)->getMapData(O_FLOOR)->getFlammable() != 255
					&& (*i)->getMapData(O_FLOOR)->getArmor() != 255)
				{
					if ((*i)->destroyTile(O_FLOOR, getObjectiveType()) == true)
						addDestroyedObjective();
				}
			}

			_te->applyGravity(*i);
		}
	}

	for (std::vector<Tile*>::const_iterator
			i = tilesSmoked.begin();
			i != tilesSmoked.end();
			++i)
	{
		(*i)->decreaseSmoke();

		var = (*i)->getSmoke() >> 1;

		if (var > 1)
		{
			tile = getTile((*i)->getPosition() + Position(0,0,1));
			if (tile != NULL && tile->hasNoFloor(*i) == true) // TODO: use verticalBlockage() instead
				tile->addSmoke(var / 3);

			for (int
					dir = 0;
					dir != 8;
					dir += 2)
			{
				if (RNG::percent(var * 8) == true)
				{
					Position posSpread;
					Pathfinding::directionToVector(dir, &posSpread);
					tile = getTile((*i)->getPosition() + posSpread);
					if (tile != NULL && _te->horizontalBlockage(*i, tile, DT_SMOKE) == 0)
						tile->addSmoke(var >> 1);
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
			&& (*i)->getUnitStatus() != STATUS_DEAD) // etc. See below_
		{
			reviveUnit(*i, true);
		}
	}
} */

/**
 * Checks if unit is unconscious and revives it if it shouldn't be.
 * @note Revived units need a tile to stand on. If the unit's current position
 * is occupied then all directions around the tile are searched for a free tile
 * to place the unit on. If no free tile is found the unit stays unconscious.
 * @param atTurnOver - true if called from SavedBattleGame::endBattlePhase (default false)
 */
void SavedBattleGame::reviveUnit(
		BattleUnit* const unit,
		bool atTurnOver)
{
	if (unit->getUnitStatus() == STATUS_UNCONSCIOUS
		&& unit->getStun() < unit->getHealth() + static_cast<int>(atTurnOver) // do health=stun if unit is about to get healed in Prep Turn.
		&& (unit->getGeoscapeSoldier() != NULL
			|| (unit->getUnitRules()->isMechanical() == false
				&& unit->getArmor()->getSize() == 1)))
	{
		if (unit->getFaction() == FACTION_HOSTILE)	// faction will be Original here
			unit->setExposed();						// due to death/stun sequence.
		else
			unit->setExposed(-1);


		Position posCorpse = unit->getPosition();

		if (posCorpse == Position(-1,-1,-1)) // if carried
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
					posCorpse = (*i)->getOwner()->getPosition();
					break;
				}
			}
		}

		const Tile* const tileCorpse = getTile(posCorpse);
		bool largeUnit = tileCorpse != NULL
					  && tileCorpse->getUnit() != NULL
					  && tileCorpse->getUnit() != unit
					  && tileCorpse->getUnit()->getArmor()->getSize() == 2;

		if (placeUnitNearPosition(unit, posCorpse, largeUnit) == true)
		{
			unit->setUnitStatus(STATUS_STANDING);

			if (unit->getGeoscapeSoldier() != NULL)
				unit->kneel(true);

			unit->clearCache();

			unit->setDirection(RNG::generate(0,7));
			unit->setTimeUnits(0);
			unit->setEnergy(0);
			unit->setRevived();

			_te->calculateUnitLighting();
			_te->calculateFOV(unit->getPosition(), true);
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
	int quad = unit->getArmor()->getSize() * unit->getArmor()->getSize();

	for (std::vector<BattleItem*>::const_iterator
			i = _items.begin();
			i != _items.end();
			++i)
	{
		if ((*i)->getUnit() == unit)
		{
			removeItem(*i);
			--i;

			if (--quad == 0)
				return;
		}
	}
}

/**
 * Places units on the map.
 * @note Also handles large units that are placed on multiple tiles.
 * @param unit	- pointer to a unit to be placed
 * @param pos	- reference the position to place the unit
 * @param test	- true only checks if unit can be placed at the position (default false)
 * @return, true if unit was placed successfully
 */
bool SavedBattleGame::setUnitPosition(
		BattleUnit* const unit,
		const Position& pos,
		bool test) const
{
	if (unit != NULL)
	{
//		_pf->setPathingUnit(unit); // <- this is not valid when doing base equip.
		Position posTest = pos; // strip const.

		const int armorSize = unit->getArmor()->getSize() - 1;
		for (int
				x = armorSize;
				x != -1;
				--x)
		{
			for (int
					y = armorSize;
					y != -1;
					--y)
			{
				const Tile* const tile = getTile(posTest + Position(x,y,0));
				if (tile != NULL)
				{
					if (tile->getTerrainLevel() == -24)
					{
						posTest += Position(0,0,1);
						x =
						y = armorSize + 1; // start over.

						break;
					}

					if ((tile->getUnit() != NULL
							&& tile->getUnit() != unit)
						|| tile->getTuCostTile(
											O_OBJECT,
											unit->getMoveTypeUnit()) == 255
						|| (unit->getMoveTypeUnit() != MT_FLY
							&& tile->hasNoFloor(getTile(posTest + Position(x,y,-1))) == true) // <- so just use the unit's moveType.
						|| (tile->getMapData(O_OBJECT) != NULL
							&& tile->getMapData(O_OBJECT)->getBigWall() > BIGWALL_NONE
							&& tile->getMapData(O_OBJECT)->getBigWall() < BIGWALL_WEST))
					{
						return false;
					}

					// TODO: check for ceiling also.
					const Tile* const tileAbove = getTile(posTest + Position(x,y,1));
					if (tileAbove != NULL
						&& tileAbove->getUnit() != NULL
						&& tileAbove->getUnit() != unit
						&& unit->getHeight(true) - tile->getTerrainLevel() > 26) // don't stuck yer head up someone's flying arse.
					{
						return false;
					}
				}
				else
					return false;
			}
		}

		if (armorSize != 0) // -> however, large units never use base equip, so _pf is valid here.
		{
			_pf->setPathingUnit(unit);
			for (int
					dir = 2;
					dir != 5;
					++dir)
			{
				if (_pf->isBlockedPath(
									getTile(posTest),
									dir) == true)
				{
					return false;
				}
			}
		}

		if (test == false)
		{
			unit->setPosition(posTest);

			for (int
					x = armorSize;
					x != -1;
					--x)
			{
				for (int
						y = armorSize;
						y != -1;
						--y)
				{
					getTile(posTest + Position(x,y,0))->setUnit(
															unit,
															getTile(posTest + Position(x,y,-1)));
				}
			}
		}

		return true;
	}

	return false;
}

/**
 * Places a unit on or near a position.
 * @param unit		- pointer to a BattleUnit to place
 * @param pos		- reference the position around which to attempt to place @a unit
 * @param isLarge	- true if @a unit is large
 * @return, true if unit is placed
 */
bool SavedBattleGame::placeUnitNearPosition(
		BattleUnit* const unit,
		const Position& pos,
		bool isLarge) const
{
	if (unit == NULL)
		return false;

	if (setUnitPosition(unit, pos) == true)
		return true;


	int
		size1 = 0 - unit->getArmor()->getSize(),
		size2 = isLarge ? 2 : 1,
		xArray[8] = {    0, size2, size2, size2,     0, size1, size1, size1},
		yArray[8] = {size1, size1,     0, size2, size2, size2,     0, size1};

	const Tile* tile;
	const int dir = RNG::seedless(0,7);
	for (int
			i = dir;
			i != dir + 8;
			++i)
	{
		Position posOffset = Position(
									xArray[i % 8],
									yArray[i % 8],
									0);
//		getPathfinding()->directionToVector(
//										i % 8,
//										&posOffset);

		tile = getTile(pos + (posOffset / 2));
//		tile = getTile(pos + posOffset);
		if (tile != NULL
			&& getPathfinding()->isBlockedPath(tile, dir, NULL) == false
//			&& getPathfinding()->isBlockedPath(getTile(pos), i) == false
			&& setUnitPosition(unit, pos + posOffset) == true)
		{
			return true;
		}
	}

/*	if (unit->getMovementType() == MT_FLY) // uhh no.
	{
		Tile* tile = getTile(pos + Position(0,0,1));
		if (tile
			&& tile->hasNoFloor(getTile(pos))
			&& setUnitPosition(unit, pos + Position(0,0,1)))
		{
			return true;
		}
	} */

	return false;
}

/**
 * @brief Checks whether anyone on a particular faction is looking at the unit.
 * Similar to getSpottingUnits() but returns a bool and stops searching if one positive hit is found.
 * @param faction Faction to check through.
 * @param unit Whom to spot.
 * @return True when the unit can be seen
 */
/*kL bool SavedBattleGame::eyesOnTarget(UnitFaction faction, BattleUnit* unit)
{
	for (std::vector<BattleUnit*>::const_iterator i = getUnits()->begin(); i != getUnits()->end(); ++i)
	{
		if ((*i)->getFaction() != faction) continue;
		std::vector<BattleUnit*>* vis = (*i)->getHostileUnits();
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
 * @param fall - true if there are any units falling in the battlescap
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
		bool isXCOM) const
{
	//Log(LOG_INFO) << "SavedBattleGame::getHighestRanked() xcom = " << xcom;
	BattleUnit* leader = NULL;
	qtyAllies = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _units.begin();
			i != _units.end();
			++i)
	{
		if ((*i)->isOut(true, true) == false)
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
		const BattleUnit* const unit,
		bool isXCOM) const
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

			if (_tacticalType == "STR_MARS_CYDONIA_LANDING"
				|| _tacticalType == "STR_MARS_THE_FINAL_ASSAULT")
			{
				ret /= 2; // less hit for losing a unit on Cydonia.
			}
			//Log(LOG_INFO) << ". . aLien lossModifi = " << ret;
		}
	}
	else // leadership Bonus
	{
		//Log(LOG_INFO) << "SavedBattleGame::getMoraleModifier(), leadership Bonus";
		const BattleUnit* leader;
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

			if (_tacticalType == "STR_TERROR_MISSION"
				|| _tacticalType == "STR_ALIEN_BASE_ASSAULT"
				|| _tacticalType == "STR_BASE_DEFENSE")
			{
				ret += 50; // higher morale.
			}
			else if (_tacticalType == "STR_MARS_CYDONIA_LANDING"
						|| _tacticalType == "STR_MARS_THE_FINAL_ASSAULT")
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
	_cheatAI = false;
	_side = FACTION_PLAYER;
}

/**
 * Resets visibility of all the tiles on the map.
 */
void SavedBattleGame::resetTiles()
{
	for (size_t
			i = 0;
			i != _mapSize;
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
	return _cheatAI;
}

/*
 * Gets the TU reserved type.
 * @return, a BattleActionType
 *
BattleActionType SavedBattleGame::getBatReserved() const
{
	return _batReserved;
} */

/*
 * Sets the TU reserved type.
 * @param reserved - a BattleActionType
 *
void SavedBattleGame::setBatReserved(BattleActionType reserved)
{
	_batReserved = reserved;
} */

/*
 * Gets the kneel reservation setting.
 * @return, true if an extra 4 TUs should be reserved to kneel
 *
bool SavedBattleGame::getKneelReserved() const
{
	return _kneelReserved;
} */

/*
 * Sets the kneel reservation setting.
 * @param reserved - true if an extra 4 TUs should be reserved to kneel
 *
void SavedBattleGame::setKneelReserved(bool reserved)
{
	_kneelReserved = reserved;
} */

/**
 * Gets a reference to the base module destruction map.
 * @note This map contains information on how many destructible base modules
 * remain at any given grid reference in the basescape, using [x][y] format.
 * -1 for "no items" 0 for "destroyed" and any actual number represents how many
 * left.
 * @return, reference to a vector of vectors containing pairs of ints that make up base module damage maps
 */
std::vector<std::vector<std::pair<int, int> > >& SavedBattleGame::getModuleMap()
{
	return _baseModules;
}

/**
 * Calculates the number of map modules remaining by counting the map objects on
 * the top floor who have the baseModule flag set. We store this data in the
 * grid as outlined in the comments above, in pairs representing initial and
 * current values.
 */
void SavedBattleGame::calculateModuleMap()
{
	_baseModules.resize(
					_mapsize_x / 10,
					std::vector<std::pair<int, int> >(
												_mapsize_y / 10,
												std::make_pair(-1,-1)));

	for (int // need a bunch of size_t ->
			x = 0;
			x != _mapsize_x;
			++x)
	{
		for (int
				y = 0;
				y != _mapsize_y;
				++y)
		{
			for (int
					z = 0;
					z != _mapsize_z;
					++z)
			{
				const Tile* const tile = getTile(Position(x,y,z));

				if (tile != NULL
					&& tile->getMapData(O_OBJECT) != NULL
					&& tile->getMapData(O_OBJECT)->isBaseModule() == true)
				{
					_baseModules[x / 10]
								[y / 10].first += _baseModules[x / 10][y / 10].first > 0 ? 1 : 2;
					_baseModules[x / 10]
								[y / 10].second = _baseModules[x / 10][y / 10].first;
				}
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
 * @return, reference to the title
 */
const std::wstring& SavedBattleGame::getOperation() const
{
	return _operationTitle;
}

/**
 * Tells player that an aLienBase control has been destroyed.
 */
/* void SavedBattleGame::setControlDestroyed()
{
	_controlDestroyed = true;
} */

/**
 * Gets if an aLienBase control has been destroyed.
 * @return, true if destroyed
 */
bool SavedBattleGame::getControlDestroyed() const
{
	return _controlDestroyed;
}

/**
 * Gets the music track for the current battle.
 * @return, address of the title of the music track
 */
/*std::string& SavedBattleGame::getMusic()
{
	return _music;
} */

/**
 * Sets the music track for this battle.
 * @note The track-string is const but I don't want to deal with it.
 * @param track - reference the track's name
 */
void SavedBattleGame::setMusic(std::string& track)
{
	_music = track;
}

/**
 * Sets variables for what music to play in a particular terrain or lack thereof.
 * @note The music-string and terrain-string are both const but I don't want to
 * deal with it.
 * @param music		- reference the music category to play
 * @param terrain	- reference the terrain to choose music for
 */
void SavedBattleGame::calibrateMusic(
		std::string& music,
		std::string& terrain) const
{
	if (_music.empty() == false)
		music = _music;
	else
	{
		switch (_tacType)
		{
			case TCT_UFOCRASHED:	// 0 - STR_UFO_CRASH_RECOVERY
				music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOCRASHED;
				terrain = _terrain;
			break;

			case TCT_UFOLANDED:		// 1 - STR_UFO_GROUND_ASSAULT
				music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOLANDED;
				terrain = _terrain;
			break;

			case TCT_BASEASSAULT:	// 2 - STR_ALIEN_BASE_ASSAULT
				music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEASSAULT;
			break;

			case TCT_BASEDEFENSE:	// 3 - STR_BASE_DEFENSE
				music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEDEFENSE;
			break;

			case TCT_MISSIONSITE:	// 4 - STR_TERROR_MISSION and STR_PORT_ATTACK, see setTacType()
				music = OpenXcom::res_MUSIC_TAC_BATTLE_TERRORSITE;
			break;

			case TCT_MARS1:			// 5 - STR_MARS_CYDONIA_LANDING
				music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS1;
			break;

			case TCT_MARS2:			// 6 - STR_MARS_THE_FINAL_ASSAULT
				music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS2;
			break;

			default:
				music = OpenXcom::res_MUSIC_TAC_BATTLE; // safety.
//				terrain = "CULTA"; // remarked in Music.rul
		}
	}
	//Log(LOG_INFO) << "SBG:calibrateMusic music= " << music << " terrain= " << terrain;
}

/**
 * Sets the objective type for the current battle.
 * @param type - the objective type
 */
void SavedBattleGame::setObjectiveType(int type)
{
	_objectiveType = type;
}

/**
 * Gets the objective type for the current battle.
 * @return, the objective type
 */
SpecialTileType SavedBattleGame::getObjectiveType() const
{
	return static_cast<SpecialTileType>(_objectiveType);
}

/**
 * Sets the aLiens as having been pacified.
 * @note Experience gains are no longer allowed once this is set.
 */
void SavedBattleGame::setPacified()
{
	_pacified = true;
}

/**
 * Gets whether the aLiens have been pacified yet.
 * @note Experience gains are no longer allowed if this is set.
 * @return, true if pacified
 */
bool SavedBattleGame::getPacified() const
{
	return _pacified;
}

/**
 * Stores the camera-position where the last RF-trigger happened.
 * @param pos - position
 */
void SavedBattleGame::storeRfTriggerPosition(const Position& pos)
{
	_rfTriggerPosition = pos;
}

/**
 * Gets the camera-position where the last RF-trigger happened.
 * @return, position
 */
const Position& SavedBattleGame::getRfTriggerPosition() const
{
	return _rfTriggerPosition;
}

/**
 * Gets a ref to the scanner dots vector.
 * @return, reference to a vector of pairs of ints which are positions of current Turn's scanner dots.
 */
std::vector<std::pair<int, int> >& SavedBattleGame::getScannerDots()
{
	return _scanDots;
}

}
