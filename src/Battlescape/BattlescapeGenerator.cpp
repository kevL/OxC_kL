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

#include "BattlescapeGenerator.h"

//#include <fstream>
//#include <sstream>
//#include <assert.h>

#include "AlienBAIState.h"
#include "CivilianBAIState.h"
#include "Inventory.h"
#include "Pathfinding.h"
#include "TileEngine.h"

//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

//kL #include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/AlienRace.h"
#include "../Ruleset/MapBlock.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
//#include "../Ruleset/RuleTexture.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/RuleUnit.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Sets up a BattlescapeGenerator.
 * @param game - pointer to Game
 */
BattlescapeGenerator::BattlescapeGenerator(Game* game)
	:
		_game(game),
		_gameSave(game->getSavedGame()),
		_battleSave(game->getSavedGame()->getSavedBattle()),
		_rules(game->getRuleset()),
		_res(game->getResourcePack()),
		_craft(NULL),
		_ufo(NULL),
		_base(NULL),
		_mission(NULL),
		_alienBase(NULL),
//		_isCity(false),
		_terrain(NULL),
//		_texture(NULL),
		_shade(0),
		_alienItemLevel(0),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_unitSequence(0),
		_tileEquipt(NULL),
		_generateFuel(true),
		_craftDeployed(false),
		_craftZ(0),
		_baseEquipScreen(false),
		_battleOrder(0),
		_blocksToDo(0),
		_testBlock(NULL)
//		_tacType(TCT_DEFAULT)
{
//	_allowAutoLoadout = !Options::disableAutoEquip;
}

/**
 * Deletes the BattlescapeGenerator.
 */
BattlescapeGenerator::~BattlescapeGenerator()
{}

/**
 * Sets up the various arrays and whatnot according to the size of the map.
 */
void BattlescapeGenerator::init() // private.
{
	_blocks.clear();
	_landingzone.clear();
	_segments.clear();
	_drillMap.clear();

	_blocks.resize(
				_mapsize_x / 10,
				std::vector<MapBlock*>(_mapsize_y / 10));
	_landingzone.resize(
					_mapsize_x / 10,
					std::vector<bool>(
								_mapsize_y / 10,
								false));
	_segments.resize(
				_mapsize_x / 10,
				std::vector<int>(
							_mapsize_y / 10,
							0));
	_drillMap.resize(
				_mapsize_x / 10,
				std::vector<int>(
							_mapsize_y / 10,
							MD_NONE));

	_blocksToDo = (_mapsize_x / 10) * (_mapsize_y / 10);

	_battleSave->initMap( // creates the tile objects
					_mapsize_x,
					_mapsize_y,
					_mapsize_z);
	_battleSave->initUtilities(_res);
}

/**
 * Sets the XCom craft involved in the battle.
 * @param craft - pointer to Craft
 */
void BattlescapeGenerator::setCraft(Craft* craft)
{
	_craft = craft;
	_craft->setInBattlescape(true);
}

/**
 * Sets the ufo involved in the battle.
 * @param ufo - pointer to Ufo
 */
void BattlescapeGenerator::setUfo(Ufo* ufo)
{
	_ufo = ufo;
	_ufo->setInBattlescape(true);
}

/**
 * Sets the XCom base involved in the battle.
 * @param base - pointer to Base
 */
void BattlescapeGenerator::setBase(Base* base)
{
	_base = base;
	_base->setInBattlescape(true);
}

/**
 * Sets the mission site involved in the battle.
 * @param mission - pointer to MissionSite
 */
void BattlescapeGenerator::setMissionSite(MissionSite* mission)
{
	_mission = mission;
	_mission->setInBattlescape(true);
}

/**
 * Sets the alien base involved in the battle.
 * @param base - pointer to AlienBase
 */
void BattlescapeGenerator::setAlienBase(AlienBase* base)
{
	_alienBase = base;
	_alienBase->setInBattlescape(true);
}

/**
 * Sets if Ufo has landed/crashed at a city as per ConfirmLandingState.
 * This is not for terrorsites.
 * @param isCity - true if UFO crashed or landed at a city (default true)
 */
/*void BattlescapeGenerator::setIsCity(const bool isCity)
{
	_isCity = isCity;
} */

/**
 * Sets the terrain of where a ufo crashed/landed as per ConfirmLandingState.
 * @param texture - pointer to RuleTerrain
 */
void BattlescapeGenerator::setTacTerrain(RuleTerrain* terrain)
{
	_terrain = terrain;
}

/**
 * Sets the world texture where a ufo crashed or landed.
 * This is used to determine the terrain if siteTerrain is ""/ NULL.
 * @param texture - texture id of the polygon on the globe
 */
/*void BattlescapeGenerator::setTacTexture(RuleTexture* texture)
{
	_texture = texture;
} */

/**
 * Sets the world shade where a ufo crashed or landed.
 * This is used to determine the battlescape light level.
 * @param shade - shade of the polygon on the globe
 */
void BattlescapeGenerator::setTacShade(int shade)
{
	if (shade > 15) shade = 15;
	if (shade < 0) shade = 0;

	_shade = shade;
}

/**
 * Sets the alien race on the mission. This is used to determine the various alien types to spawn.
 * @param alienRace - reference the (main) alien race
 */
void BattlescapeGenerator::setAlienRace(const std::string& alienRace)
{
	_alienRace = alienRace;
	_battleSave->setAlienRace(alienRace);
}

/**
 * Sets the alien item level. This is used to determine how advanced the equipment of the aliens will be.
 * note: this only applies to "New Battle" type games. we intentionally don't alter the month for those,
 * because we're using monthsPassed -1 for new battle in other sections of code.
 * - this value should be from 0 to the size of the itemLevel array in the ruleset (default 9).
 * - at a certain number of months higher item levels appear more and more and lower ones will gradually disappear
 * @param alienItemLevel - alienItemLevel
 */
void BattlescapeGenerator::setAlienItemlevel(int alienItemLevel)
{
	_alienItemLevel = alienItemLevel;
}

/**
 * Switches an existing SavedBattleGame to a new stage.
 */
void BattlescapeGenerator::nextStage()
{
	// kill all enemy units, or those not in endpoint area (if aborted)
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getStatus() != STATUS_DEAD
			&& ((*i)->getOriginalFaction() != FACTION_PLAYER)
				|| (_battleSave->isAborted() == true
					&& (*i)->getOriginalFaction() == FACTION_PLAYER
					&& (*i)->isInExitArea(END_POINT) == false))
		{
			(*i)->setStatus(STATUS_TIME_OUT);
		}

		if ((*i)->getTile() != NULL)
			(*i)->getTile()->setUnit(NULL);

		(*i)->setTile(NULL);
		(*i)->setPosition(Position(-1,-1,-1), false);
	}

//	while (_battleSave->getSide() != FACTION_PLAYER)
//		_battleSave->endBattlePhase();

	_battleSave->resetTurnCounter();

	// remove all items not belonging to soldiers from the map.
	// sort items into two categories:
	// - the ones that are guaranteed to be able to take home, barring complete failure (ie: stuff on the ship)
	// - and the ones that are scattered about on the ground, that will be recovered ONLY on success.
	// this does not include items in soldiers' hands.
	std::vector<BattleItem*>
		* takeHomeGuaranteed = _battleSave->getGuaranteedRecoveredItems(),
		* takeHomeConditional = _battleSave->getConditionalRecoveredItems(),
		* toContainer,
		takeToNextStage;
//	std::map<RuleItem*, int>
//		guaranteedRounds,
//		conditionalRounds;

	Tile* tile;
	for (std::vector<BattleItem*>::iterator
			i = _battleSave->getItems()->begin();
			i != _battleSave->getItems()->end();
			)
	{
		if ((*i)->getOwner() == NULL
			|| (*i)->getOwner()->getOriginalFaction() != FACTION_PLAYER)
		{
			tile = (*i)->getTile();
			toContainer = takeHomeConditional;

			if (tile != NULL)
			{
				tile->removeItem(*i);

				if (tile->getMapData(O_FLOOR) != NULL)
				{
					if (tile->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
						toContainer = takeHomeGuaranteed;
					else if (tile->getMapData(O_FLOOR)->getSpecialType() == END_POINT
						&& (*i)->getRules()->isRecoverable() == true
						&& (*i)->getUnit() == NULL)
					{
						takeToNextStage.push_back(*i);

						++i;
						continue;
					}
				}
			}

			if ((*i)->getRules()->isRecoverable() == true
				&& (*i)->getXCOMProperty() == false) // note that <-
			{
				toContainer->push_back(*i);
				i = _battleSave->getItems()->erase(i);

				continue;
			}
		}

		++i;
	}

//	_missionType = _battleSave->getMissionType();
//	AlienDeployment* const ruleDeploy = _rules->getDeployment(_missionType);
	AlienDeployment* const ruleDeploy = _rules->getDeployment(_battleSave->getMissionType());
	ruleDeploy->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);
	const size_t pick = RNG::generate(
								0,
								ruleDeploy->getDeployTerrains().size() - 1);
	_terrain = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(pick));

	_shade = ruleDeploy->getShade();


	const std::vector<MapScript*>* script = _rules->getMapScript(_terrain->getScript());

	if (_rules->getMapScript(ruleDeploy->getScript()) != NULL)
		script = _rules->getMapScript(ruleDeploy->getScript());
	else if (ruleDeploy->getScript().empty() == false)
	{
		throw Exception("Map generator encountered an error: " + ruleDeploy->getScript() + " script not found.");
	}

	if (script == NULL)
	{
		throw Exception("Map generator encountered an error: " + _terrain->getScript() + " script not found.");
	}

	generateMap(script); // <-- BATTLE MAP GENERATION.


	bool selectedFirstSoldier = false;
	int highestSoldierID = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER)
		{
			if ((*i)->isOut() == false)
			{
				(*i)->convertToFaction(FACTION_PLAYER);
				(*i)->setExposed(-1);
//				(*i)->getVisibleTiles()->clear();

				if (selectedFirstSoldier == false
					&& (*i)->getGeoscapeSoldier())
				{
					_battleSave->setSelectedUnit(*i);
					selectedFirstSoldier = true;
				}

				const Node* const node = _battleSave->getSpawnNode(
																NR_XCOM,
																*i);
				if (node != NULL
					|| placeUnitNearFriend(*i) == true)
				{
					if (node != NULL)
						_battleSave->setUnitPosition(
													*i,
													node->getPosition());

					if (_tileEquipt == NULL)
					{
						_tileEquipt = (*i)->getTile();
						_battleSave->setBattleInventory(_tileEquipt);
					}

					_tileEquipt->setUnit(*i);
					(*i)->setUnitVisible(false);

					if ((*i)->getId() > highestSoldierID)
						highestSoldierID = (*i)->getId();

					(*i)->prepUnit(false);
				}
			}
		}
	}

	if (_battleSave->getSelectedUnit() == NULL
		|| _battleSave->getSelectedUnit()->getFaction() != FACTION_PLAYER)
	{
		_battleSave->selectNextFactionUnit();
	}

/*	// remove all items not belonging to the soldiers from the map
	for (std::vector<BattleItem*>::const_iterator
			j = _battleSave->getItems()->begin();
			j != _battleSave->getItems()->end();
			++j)
	{
		if ((*j)->getOwner() == NULL
			|| (*j)->getOwner()->getId() > highestSoldierID)
		{
			(*j)->setTile(NULL);
		}
	} */

	RuleInventory* const ground = _game->getRuleset()->getInventory("STR_GROUND");
	for (std::vector<BattleItem*>::iterator
		i = takeToNextStage.begin();
		i != takeToNextStage.end();
		++i)
	{
		_tileEquipt->addItem(
						*i,
						ground);
	}


	_unitSequence = _battleSave->getUnits()->back()->getId() + 1;
	const size_t unitCount = _battleSave->getUnits()->size();

	for (std::vector<MissionSite*>::const_iterator
			i = _gameSave->getMissionSites()->begin();
			i != _gameSave->getMissionSites()->end()
				&& _alienRace.empty() == true;
			++i)
	{
		if ((*i)->isInBattlescape() == true)
			_alienRace = (*i)->getAlienRace();
	}

	for (std::vector<AlienBase*>::const_iterator
			i = _gameSave->getAlienBases()->begin();
			i != _gameSave->getAlienBases()->end()
				&& _alienRace.empty() == true;
			++i)
	{
		if ((*i)->isInBattlescape() == true)
			_alienRace = (*i)->getAlienRace();
	}

	deployAliens(ruleDeploy);

	if (unitCount == _battleSave->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	deployCivilians(ruleDeploy->getCivilians());

/*kL: Prob. don't need this anymore; it's done via "revealedFloors" in MapScripting ... but not quite.
	for (int
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(O_FLOOR) != NULL
			&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
//				|| (_battleSave->getTiles()[i]->getPosition().z == 1
//					&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
//					&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))
		{
			_battleSave->getTiles()[i]->setDiscovered(true, 2);
		}
	} */

	_battleSave->setAborted(false);

	// set shade (alien bases are a little darker, sites depend on worldshade)
	_battleSave->setGlobalShade(_shade);

	_battleSave->getTileEngine()->calculateSunShading();
	_battleSave->getTileEngine()->calculateTerrainLighting();
	_battleSave->getTileEngine()->calculateUnitLighting();

	_battleSave->getTileEngine()->recalculateFOV();
}

/**
 * Starts the generator; this fills up the SavedBattleGame with data.
 */
void BattlescapeGenerator::run()
{
	_unitSequence = BattleUnit::MAX_SOLDIER_ID; // geoscape soldier IDs should stay below this number

//	_missionType = _battleSave->getMissionType();

	AlienDeployment* ruleDeploy;
	if (_ufo != NULL)
		ruleDeploy = _rules->getDeployment(_ufo->getRules()->getType());
	else
		ruleDeploy = _rules->getDeployment(_battleSave->getMissionType());
//		ruleDeploy = _rules->getDeployment(_missionType);

	ruleDeploy->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);

	if (_terrain == NULL)	// ie. if not NewBattleState ...... & not UFO, & not missionSite -> is Set for those.
	{						// '_terrain' NOT set for Cydonia, Base assault/defense.
		const size_t pick = RNG::generate(
									0,
									ruleDeploy->getDeployTerrains().size() - 1);
		_terrain = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
	}
/* Theirs:
	if (_terrain == NULL)
	{
		if (_texture == NULL
			|| _texture->getTerrainCriteria()->empty() == true
			|| ruleDeploy->getDeployTerrains().empty() == false)
		{
			size_t pick = RNG::generate(
									0,
									ruleDeploy->getDeployTerrains().size() - 1);
			_terrain = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
		}
		else // UFO crashed/landed or MissionSite
		{
			const Target* target;
			if (_mission != NULL)
				target = _mission;
			else
				target = _ufo;

			_terrain = _rules->getTerrain(_texture->getRandomTerrain(target));
		}
	} */
/*	if (ruleDeploy->getDeployTerrains().empty() == true) // UFO crashed/landed
	{
		Log(LOG_INFO) << "bGen::run() deployment-terrains NOT valid";
		if (_siteTerrain == NULL) // kL
		{
			Log(LOG_INFO) << ". siteTexture = " << _texture;
			double lat;
			if (_ufo != NULL)
				lat = _ufo->getLatitude();
			else
				lat = 0.;

			_terrain = getTerrain(
								_texture,
								lat);
		}
		else
		{
			Log(LOG_INFO) << ". ufo mission siteTerrain = " << _siteTerrain->getName();
			_terrain = _siteTerrain; // kL
		}
	}
	else if (_siteTerrain != NULL // kL ->
		&& _missionType == "STR_TERROR_MISSION")
	{
		Log(LOG_INFO) << ". terror mission siteTerrain = " << _siteTerrain->getName();
		_terrain = _siteTerrain; // kL_end.
	}
	else // set-piece battle like Cydonia or Terror site or Base assault/defense
	{
		Log(LOG_INFO) << "bGen::run() Choose terrain from deployment, qty = " << ruleDeploy->getDeployTerrains().size();
		const size_t pick = RNG::generate(
										0,
										ruleDeploy->getDeployTerrains().size() - 1);
		_terrain = _rules->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
	} */


	// new battle menu will have set the depth already
/*	if (_battleSave->getDepth() == 0)
	{
		if (ruleDeploy->getMaxDepth() > 0)
			_battleSave->setDepth(RNG::generate(
											ruleDeploy->getMinDepth(),
											ruleDeploy->getMaxDepth()));
		else if (_terrain->getMaxDepth() > 0)
			_battleSave->setDepth(RNG::generate(
											_terrain->getMinDepth(),
											_terrain->getMaxDepth()));
	} */

	if (ruleDeploy->getShade() != -1)
		_shade = ruleDeploy->getShade();


	const std::vector<MapScript*>* script = _rules->getMapScript(_terrain->getScript());
	Log(LOG_INFO) << "bGen::run() script = " << _terrain->getScript();

	if (_rules->getMapScript(ruleDeploy->getScript()) != NULL) // alienDeployment script overrides terrain script <-
	{
		script = _rules->getMapScript(ruleDeploy->getScript());
		Log(LOG_INFO) << "bGen::run() script = " << ruleDeploy->getScript();
	}
	else if (ruleDeploy->getScript().empty() == false)
	{
		throw Exception("Map generator encountered an error: " + ruleDeploy->getScript() + " script not found.");
	}

	if (script == NULL)
	{
		throw Exception("Map generator encountered an error: " + _terrain->getScript() + " script not found.");
	}

	generateMap(script); // <-- BATTLE MAP GENERATION.


	_battleSave->setBattleTerrain(_terrain->getType());
	setTacticalSprites();

	deployXCOM(); // <-- XCOM DEPLOYMENT.

	const size_t unitCount = _battleSave->getUnits()->size();

	deployAliens(ruleDeploy); // <-- ALIEN DEPLOYMENT.

	if (unitCount == _battleSave->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	deployCivilians(ruleDeploy->getCivilians());

	if (_generateFuel == true)
		fuelPowerSources();

//	if (_missionType == "STR_UFO_CRASH_RECOVERY")
	if (_ufo != NULL
		&& _ufo->getStatus() == Ufo::CRASHED)
	{
		explodePowerSources();
	}

/*	if (_missionType == "STR_BASE_DEFENSE")
	{
		for (int
				i = 0;
				i < _battleSave->getMapSizeXYZ();
				++i)
		{
			_battleSave->getTiles()[i]->setDiscovered(true, 2);
		}

		_battleSave->calculateModuleMap();
	}

	if (_missionType == "STR_ALIEN_BASE_ASSAULT"
		|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{
		for (int
				i = 0;
				i < _battleSave->getMapSizeXYZ();
				++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(O_FLOOR)
				&& (_battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT
					|| (_battleSave->getTiles()[i]->getPosition().z == 1
						&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
						&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	} */

/*
	if (_craftDeployed == false)
	{
		for (int
				i = 0;
				i < _battleSave->getMapSizeXYZ();
				++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(O_FLOOR) != NULL
				&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getSpecialType() == START_POINT)
//					|| (_battleSave->getTiles()[i]->getPosition().z == _mapsize_z - 1
//						&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->isGravLift()
//						&& _battleSave->getTiles()[i]->getMapData(O_OBJECT))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	} */

	std::vector<std::string> deployMusic = ruleDeploy->getMusic(); // this is const but I don't want to deal with it.
	if (deployMusic.empty() == false)
		_battleSave->setMusic(deployMusic.at(static_cast<size_t>(RNG::generate(
																			0,
																			deployMusic.size() - 1))));
	else
	{
		std::vector<std::string> terrainMusic = _terrain->getMusic(); // this is const but I don't want to deal with it.
		if (terrainMusic.empty() == false)
			_battleSave->setMusic(terrainMusic.at(static_cast<size_t>(RNG::generate(
																				0,
																				terrainMusic.size() - 1))));
	}

	// set shade (alien bases are a little darker, sites depend on worldshade)
	_battleSave->setGlobalShade(_shade);

	_battleSave->getTileEngine()->calculateSunShading();
	_battleSave->getTileEngine()->calculateTerrainLighting();
	_battleSave->getTileEngine()->calculateUnitLighting();

	_battleSave->getTileEngine()->recalculateFOV();
}

/**
* Deploys all the X-COM units and equipment based on the Geoscape base / craft.
*/
void BattlescapeGenerator::deployXCOM() // private.
{
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM()";
	if (_craft != NULL)
	{
		_base = _craft->getBase();

		if (_baseEquipScreen == false)
		{
			//Log(LOG_INFO) << ". craft VALID";

			// add all vehicles that are in the craft - a vehicle is actually an item,
			// which you will never see as it is converted to a unit;
			// however the item itself becomes the weapon it equips.
			for (std::vector<Vehicle*>::const_iterator
					i = _craft->getVehicles()->begin();
					i != _craft->getVehicles()->end();
					++i)
			{
				//Log(LOG_INFO) << ". . isCraft: addXCOMVehicle " << (int)*i;
				BattleUnit* const unit = addXCOMVehicle(*i);
				if (unit != NULL
					&& _battleSave->getSelectedUnit() == NULL)
				{
					_battleSave->setSelectedUnit(unit);
				}
			}
		}
	}
	else if (_base != NULL
		&& _baseEquipScreen == false)
	{
		// add vehicles that are in Base inventory.
		for (std::vector<Vehicle*>::const_iterator
				i = _base->getVehicles()->begin();
				i != _base->getVehicles()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . isBase: addXCOMVehicle " << (int)*i;
			BattleUnit* const unit = addXCOMVehicle(*i);
			if (unit != NULL
				&& _battleSave->getSelectedUnit() == NULL)
			{
				_battleSave->setSelectedUnit(unit);
			}
		}

		// Only add vehicles from the craft in new battle mode
		// otherwise the base's vehicle vector will already contain these
		// due to the geoscape calling base->setupDefenses().
		if (_gameSave->getMonthsPassed() == -1)
		{
			// add vehicles from Crafts at Base.
			for (std::vector<Craft*>::const_iterator
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
			{
				for (std::vector<Vehicle*>::const_iterator
					j = (*i)->getVehicles()->begin();
					j != (*i)->getVehicles()->end();
					++j)
				{
					BattleUnit* const unit = addXCOMVehicle(*j);
					if (unit != NULL
						&& _battleSave->getSelectedUnit() == NULL)
					{
						_battleSave->setSelectedUnit(unit);
					}
				}
			}
		}
	}

	// add soldiers that are in the Craft or Base.
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((_craft != NULL
				&& (*i)->getCraft() == _craft)
			|| (_craft == NULL
				&& (*i)->getRecovery() == 0
				&& ((*i)->getCraft() == NULL
					|| (*i)->getCraft()->getStatus() != "STR_OUT")))
		{
			//Log(LOG_INFO) << ". . addXCOMUnit " << (*i)->getId();
			BattleUnit* const unit = addXCOMUnit(new BattleUnit(
															*i,
//															_battleSave->getDepth(),
															static_cast<int>(_gameSave->getDifficulty()))); // kL_add: For VictoryPts value per death.

			if (unit != NULL)
			{
				//Log(LOG_INFO) << "bGen::deployXCOM() ID " << unit->getId() << " battleOrder = " << _battleOrder + 1;
				unit->setBattleOrder(++_battleOrder);

				if (_battleSave->getSelectedUnit() == NULL)
					_battleSave->setSelectedUnit(unit);
			}
		}
	}

	if (_battleSave->getUnits()->empty() == true)
	{
		throw Exception("Map generator encountered an error: no xcom units could be placed on the map.");
	}

	//Log(LOG_INFO) << ". addXCOMUnit(s) DONE";

	for (std::vector<BattleUnit*>::const_iterator // pre-battle equip; give all xCom Soldiers access to the inventory tile.
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER) // kL_note: not really necessary because only xCom is on the field atm. Could exclude tanks ....
		{
			_tileEquipt->setUnit(*i);
			(*i)->setUnitVisible(false);
		}
	}
	//Log(LOG_INFO) << ". setUnit(s) DONE";

	RuleInventory* const ground = _rules->getInventory("STR_GROUND");

	if (_craft != NULL) // add items that are in the Craft.
	{
		//Log(LOG_INFO) << ". . addCraftItems";
		for (std::map<std::string, int>::const_iterator
				i = _craft->getItems()->getContents()->begin();
				i != _craft->getItems()->getContents()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . . *i = _craft->getItems()->getContents()";
			for (int
					j = 0;
					j != i->second;
					++j)
			{
				//Log(LOG_INFO) << ". . . j+";
				_tileEquipt->addItem(
								new BattleItem(
											_rules->getItem(i->first),
											_battleSave->getNextItemId()),
								ground);
				//Log(LOG_INFO) << ". . . j cycle";
			}
		}
		//Log(LOG_INFO) << ". . addCraftItems DONE";
	}
	else // add items that are in the Base.
	{
		// add only items in Craft for skirmish mode;
		// ie. Do NOT add items from Base in skirmish mode:
		if (_gameSave->getMonthsPassed() != -1)
		{
			//Log(LOG_INFO) << ". . addBaseItems";
			for (std::map<std::string, int>::const_iterator // add items from stores in base
					i = _base->getItems()->getContents()->begin();
					i != _base->getItems()->getContents()->end();
					)
			{
				// only add items in the battlescape that make sense
				// (if the item has a sprite, it's probably ok)
				const RuleItem* const rule = _rules->getItem(i->first);
				if (   rule->getBigSprite() > -1
					&& rule->getBattleType() != BT_NONE
					&& rule->getBattleType() != BT_CORPSE
					&& rule->isFixed() == false
					&& _gameSave->isResearched(rule->getRequirements()) == true)
				{
					for (int
							j = 0;
							j != i->second;
							++j)
					{
						_tileEquipt->addItem(
										new BattleItem(
													_rules->getItem(i->first),
													_battleSave->getNextItemId()),
										ground);
					}

					const std::map<std::string, int>::const_iterator mark = i;
					++i;

					if (_baseEquipScreen == false)
						_base->getItems()->removeItem(
													mark->first,
													mark->second);
				}
				else
					++i;
			}
		}
		//Log(LOG_INFO) << ". . addBaseBaseItems DONE, add BaseCraftItems";

		for (std::vector<Craft*>::const_iterator // add items from Crafts at Base.
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
		{
			if ((*i)->getStatus() == "STR_OUT")
				continue;

			for (std::map<std::string, int>::const_iterator
					j = (*i)->getItems()->getContents()->begin();
					j != (*i)->getItems()->getContents()->end();
					++j)
			{
				for (int
						k = 0;
						k != j->second;
						++k)
				{
					_tileEquipt->addItem(
									new BattleItem(
												_rules->getItem(j->first),
												_battleSave->getNextItemId()),
									ground);
				}
			}
		}
		//Log(LOG_INFO) << ". . addBaseCraftItems DONE";
	}
	//Log(LOG_INFO) << ". addItem(s) DONE";


	// kL_note: ALL ITEMS SEEM TO STAY ON THE GROUNDTILE, _tileEquipt,
	// IN THAT INVENTORY(vector) UNTIL EVERYTHING IS EQUIPPED & LOADED. Then
	// the inventory-tile is cleaned up at the end of this function....
	//
	// equip soldiers based on equipment-layout
	//Log(LOG_INFO) << ". placeItemByLayout Start";
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		// don't let the soldiers take extra ammo yet
		if ((*i)->getRules()->getBattleType() != BT_AMMO)
			placeItemByLayout(*i);
	}

	// load weapons before loadouts take extra clips.
//kL	loadWeapons();

	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". placeItemByLayout(*item)";
		// only need to distribute extra ammo at this point.
		if ((*i)->getRules()->getBattleType() == BT_AMMO)
			placeItemByLayout(*i);
	}
	//Log(LOG_INFO) << ". placeItemByLayout all DONE";

	// auto-equip soldiers (only soldiers *without* layout)
//	if (!Options::getBool("disableAutoEquip"))
//	if (_allowAutoLoadout) // kL
/*kL	{
		for (int
				pass = 0;
				pass != 4;
				++pass)
		{
			for (std::vector<BattleItem*>::iterator
					j = _tileEquipt->getInventory()->begin();
					j != _tileEquipt->getInventory()->end();
					)
			{
				if ((*j)->getSlot() == ground)
				{
					bool add = false;

					switch (pass)
					{
						case 0: // priority 1: rifles.
							add = (*j)->getRules()->isRifle();
						break;
						case 1: // priority 2: pistols (assuming no rifles were found).
							add = (*j)->getRules()->isPistol();
						break;
						case 2: // priority 3: ammunition.
							add = (*j)->getRules()->getBattleType() == BT_AMMO;
						break;
						case 3: // priority 4: leftovers.
							add = !(*j)->getRules()->isPistol()
									&& !(*j)->getRules()->isRifle()
									&& ((*j)->getRules()->getBattleType() != BT_FLARE
										|| _shade >= 9);
						break;

						default:
						break;
					}

					if (add)
					{
						for (std::vector<BattleUnit*>::iterator
								i = _battleSave->getUnits()->begin();
								i != _battleSave->getUnits()->end();
								++i)
						{
							if (!(*i)->hasInventory()
								|| !(*i)->getGeoscapeSoldier()
								|| !(*i)->getGeoscapeSoldier()->getEquipmentLayout()->empty())
							{
								continue;
							}

							// let's not be greedy, we'll only take a second extra clip
							// if everyone else has had a chance to take a first.
							bool allowSecondClip = (pass == 3);
							if (addItem(*j, *i, allowSecondClip))
							{
								j = _tileEquipt->getInventory()->erase(j);
								add = false;

								break;
							}
						}

						if (!add)
						{
							continue;
						}
					}
				}

				++j;
			}
		}
	} */
	// kL_note: no more auto-equip, Lolz.

	//Log(LOG_INFO) << ". Load Weapons..."; -> Cf. loadWeapons()
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". loading."; // from loadWeapons() ->
/*		if ((*i)->getRules()->isFixed() == false
			&& (*i)->getRules()->getCompatibleAmmo()->empty() == false
			&& (*i)->getAmmoItem() == NULL
			&& ((*i)->getRules()->getBattleType() == BT_FIREARM
				|| (*i)->getRules()->getBattleType() == BT_MELEE)) */
		if ((*i)->usesAmmo() == true) // or do it My way.
			loadGroundWeapon(*i);
	}
	//Log(LOG_INFO) << ". loading DONE";

	// clean up moved items
	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			)
	{
		if ((*i)->getSlot() == ground)
		{
			(*i)->setXCOMProperty();
			_battleSave->getItems()->push_back(*i);

			++i;
		}
		else
			i = _tileEquipt->getInventory()->erase(i);
	}
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM() EXIT";
}

/**
 * Adds an XCom vehicle to the game.
 * Sets the correct turret depending on the ammo type and adds auxilliary weapons if any.
 * @param tank - pointer to Vehicle
 * @return, pointer to the spawned unit; NULL if unable to create and equip
 */
BattleUnit* BattlescapeGenerator::addXCOMVehicle(Vehicle* const tank) // private.
{
	const std::string vehicle = tank->getRules()->getType();
	RuleUnit* const unitRule = _rules->getUnit(vehicle);

	BattleUnit* const tankUnit = addXCOMUnit(new BattleUnit( // add Vehicle as a unit.
														unitRule,
														FACTION_PLAYER,
														_unitSequence++,
														_rules->getArmor(unitRule->getArmor()),
														0));
//														_battleSave->getDepth()));
	if (tankUnit != NULL)
	{
		tankUnit->setTurretType(tank->getRules()->getTurretType());

		BattleItem* item = new BattleItem( // add Vehicle as an item and assign the unit as its owner.
									_rules->getItem(vehicle),
									_battleSave->getNextItemId());
		if (addItem(
				item,
				tankUnit) == false)
		{
			delete item;
			delete tankUnit;

			return NULL;
		}

		if (tank->getRules()->getCompatibleAmmo()->empty() == false)
		{
			const std::string ammo = tank->getRules()->getCompatibleAmmo()->front();
			BattleItem* const ammoItem = new BattleItem( // add item(ammo) and assign the Vehicle-ITEM as its owner.
													_rules->getItem(ammo),
													_battleSave->getNextItemId());
			if (addItem(
					ammoItem,
					tankUnit) == false)
			{
				delete ammoItem;
				delete item;
				delete tankUnit;

				return NULL;
			}

			ammoItem->setAmmoQuantity(tank->getAmmo());
		}


		if (unitRule->getBuiltInWeapons().empty() == false) // add item(builtInWeapon) -- what about ammo
		{
			for (std::vector<std::string>::const_iterator
					i = unitRule->getBuiltInWeapons().begin();
					i != unitRule->getBuiltInWeapons().end();
					++i)
			{
				RuleItem* const itRule = _rules->getItem(*i);
				if (itRule != NULL)
				{
					item = new BattleItem(
										itRule,
										_battleSave->getNextItemId());

					if (addItem(
							item,
							tankUnit) == false)
					{
						delete item;
					}
				}
			}
		}
	}

	return tankUnit;
}

/**
 * Adds a soldier to the game and places him on a free spawnpoint.
 * Spawnpoints are either tiles in case of an XCom craft that landed
 * or they are mapnodes in case there's no craft.
 * @param unit - pointer to an xCom BattleUnit
 * @return, pointer to the spawned unit if successful else NULL
 */
BattleUnit* BattlescapeGenerator::addXCOMUnit(BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bsg:addXCOMUnit()";
	if ((_craft == NULL
			|| _craftDeployed == false) // (_missionType == "STR_ALIEN_BASE_ASSAULT" || _missionType == "STR_MARS_THE_FINAL_ASSAULT") <- taken care of in MapScripting.
		&& _baseEquipScreen == false)
	{
		//Log(LOG_INFO) << ". no Craft";
		const Node* const node = _battleSave->getSpawnNode(
														NR_XCOM,
														unit);
		if (node != NULL)
		{
			//Log(LOG_INFO) << ". . spawnNode valid";
			_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

			_battleSave->setUnitPosition(
										unit,
										node->getPosition());
			unit->setDirection(RNG::generate(0,7));
//			_battleSave->getTileEngine()->calculateFOV(unit);

			_tileEquipt = _battleSave->getTile(node->getPosition());
			_battleSave->setBattleInventory(_tileEquipt);

			return unit;
		}
		else if (_battleSave->getTacticalType() != TCT_BASEDEFENSE)
		{
			//Log(LOG_INFO) << ". . spawnNode NOT valid - not baseDefense";
			if (placeUnitNearFriend(unit) == true)
			{
				//Log(LOG_INFO) << ". . . placeUnitNearFriend() TRUE";
				_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

				unit->setDirection(RNG::generate(0,7));
//				_battleSave->getTileEngine()->calculateFOV(unit);

				_tileEquipt = _battleSave->getTile(unit->getPosition());
				_battleSave->setBattleInventory(_tileEquipt);

				return unit;
			}
		}
	}
	else if (_craft != NULL // Transport craft deployments (Lightning & Avenger)
		&& _craft->getRules()->getDeployment().empty() == false
		&& _baseEquipScreen == false)
	{
		//Log(LOG_INFO) << ". Craft valid - use Deployment rule";
		for (std::vector<std::vector<int> >::const_iterator
				i = _craft->getRules()->getDeployment().begin();
				i != _craft->getRules()->getDeployment().end();
				++i)
		{
			//Log(LOG_INFO) << ". getDeployment()+";
			const Position pos = Position(
										(*i)[0] + (_craftPos.x * 10),
										(*i)[1] + (_craftPos.y * 10),
										(*i)[2] + _craftZ);
			bool canPlace = true;
			for (int
					x = 0;
					x != unit->getArmor()->getSize()
						&& canPlace == true;
					++x)
			{
				for (int
						y = 0;
						y != unit->getArmor()->getSize()
							&& canPlace == true;
						++y)
				{
					canPlace = canPlaceXCOMUnit(_battleSave->getTile(pos + Position(x,y,0)));
				}
			}

			if (canPlace == true)
			{
				//Log(LOG_INFO) << ". canPlaceXCOMUnit()";
				if (_battleSave->setUnitPosition(
												unit,
												pos) == true)
				{
					//Log(LOG_INFO) << ". setUnitPosition()";
					_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

					unit->setDirection((*i)[3]);

					return unit;
				}
			}
		}
	}
	else // mission w/ transport craft that does not have ruleset Deployments. Or craft/base Equip.
	{
		//Log(LOG_INFO) << ". baseEquip OR Craft w/out Deployment rule";
		int tankPos = 0;

		const size_t mapSize = static_cast<size_t>(_mapsize_x * _mapsize_y * _mapsize_z);
		for (size_t
				i = 0;
				i != mapSize;
				++i)
		{
			if (canPlaceXCOMUnit(_battleSave->getTiles()[i]) == true)
			{
				if (unit->getGeoscapeSoldier() == NULL)
				{
					if ((_battleSave->getTiles()[i]->getPosition().x == _tileEquipt->getPosition().x
							|| unit->getArmor()->getSize() == 1)
						&& ++tankPos == 3
						&& _battleSave->setUnitPosition(
													unit,
													_battleSave->getTiles()[i]->getPosition()) == true)
					{
						_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

						return unit;
					}
				}
				else if (_battleSave->setUnitPosition(
													unit,
													_battleSave->getTiles()[i]->getPosition()) == true)
				{
					_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

					return unit;
				}
			}
		}
	}

	delete unit; // fallthrough, if above fails.
	return NULL;
}

/**
 * Checks if a soldier/tank can be placed on a given tile.
 * @param tile - the given tile
 * @return, true if unit can be placed on Tile
 */
bool BattlescapeGenerator::canPlaceXCOMUnit(Tile* const tile) // private.
{
	// to spawn an xcom soldier, there has to be a tile, with a floor,
	// with the starting point attribute and no objects in the way
	if (tile != NULL																// is a tile
		&& tile->getMapData(O_FLOOR) != NULL								// has a floor
		&& tile->getMapData(O_FLOOR)->getSpecialType() == START_POINT		// is a 'start point', ie. cargo tile
		&& tile->getMapData(O_OBJECT) == NULL								// no object content
		&& tile->getMapData(O_FLOOR)->getTUCostObject(MT_WALK) < 255		// is walkable.
		&& tile->getUnit() == NULL)													// kL, and no unit on Tile.
	{
		// kL_note: ground inventory goes where the first xCom unit spawns
		if (_tileEquipt == NULL)
		{
			_tileEquipt = tile;
			_battleSave->setBattleInventory(_tileEquipt);
		}

		return true;
	}

	return false;
}

/**
 * Loads a weapon on the inventoryTile.
 * @param item - pointer to a BattleItem
 */
void BattlescapeGenerator::loadGroundWeapon(BattleItem* const item) // private.
{
	//Log(LOG_INFO) << "BattlescapeGenerator::loadGroundWeapon()";
	const RuleInventory* const ground = _rules->getInventory("STR_GROUND");
	RuleInventory* const righthand = _rules->getInventory("STR_RIGHT_HAND");
	//Log(LOG_INFO) << ". ground & righthand RuleInventory are set";

	for (std::vector<BattleItem*>::const_iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". . iterate items for Ammo";
		if ((*i)->getSlot() == ground
			&& item->setAmmoItem(*i) == 0) // success
		{
			//Log(LOG_INFO) << ". . . attempt to set righthand as Inv.";
			(*i)->setXCOMProperty();
			(*i)->setSlot(righthand);	// I don't think this is actually setting the ammo
										// into anyone's right hand; I think it's just here
										// to change the inventory-slot away from 'ground'.
			_battleSave->getItems()->push_back(*i);
		}
	}
	//Log(LOG_INFO) << "BattlescapeGenerator::loadGroundWeapon() EXIT false";
}

/**
 * Loads all XCom weaponry before anything else is distributed.
 */
/* void BattlescapeGenerator::loadWeapons() // private.
{
	// let's try to load this weapon, whether we equip it or not.
	for (std::vector<BattleItem*>::iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			++i)
	{
		if ((*i)->getRules()->isFixed() == false
			&& (*i)->getRules()->getCompatibleAmmo()->empty() == false
			&& (*i)->getAmmoItem() == NULL
			&& ((*i)->getRules()->getBattleType() == BT_FIREARM
				|| (*i)->getRules()->getBattleType() == BT_MELEE))
		{
			bool loaded = false;

			for (std::vector<BattleItem*>::iterator
					j = _tileEquipt->getInventory()->begin();
					j != _tileEquipt->getInventory()->end()
						&& loaded == false;
					++j)
			{
				if ((*j)->getSlot() == _rules->getInventory("STR_GROUND")
					&& (*i)->setAmmoItem((*j)) == 0)
				{
					_battleSave->getItems()->push_back(*j);
					(*j)->setXCOMProperty();
					(*j)->setSlot(_rules->getInventory("STR_RIGHT_HAND"));

					loaded = true;
				}
			}
		}
	}

	for (std::vector<BattleItem*>::iterator
			i = _tileEquipt->getInventory()->begin();
			i != _tileEquipt->getInventory()->end();
			)
	{
		if ((*i)->getSlot() != _rules->getInventory("STR_GROUND"))
		{
			i = _tileEquipt->getInventory()->erase(i);
			continue;
		}

		++i;
	}
} */

/**
 * Places an item on an XCom soldier based on equipment layout.
 * @param item - pointer to a BattleItem
 * @return, true if item is placed successfully
 */
bool BattlescapeGenerator::placeItemByLayout(BattleItem* item) // private.
{
	const RuleInventory* const ground = _rules->getInventory("STR_GROUND");
	if (item->getSlot() == ground)
	{
		RuleInventory* const righthand = _rules->getInventory("STR_RIGHT_HAND");
		bool loaded = false;

		// find the first soldier with a matching layout-slot
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			// skip the vehicles, we need only X-Com soldiers WITH equipment-layout
			if ((*i)->getGeoscapeSoldier() == NULL
//kL			|| (*i)->getArmor()->getSize() > 1
				|| (*i)->getGeoscapeSoldier()->getEquipmentLayout()->empty() == true)
			{
				continue;
			}


			// find the first matching layout-slot which is not already occupied
			const std::vector<EquipmentLayoutItem*>* const layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();
			for (std::vector<EquipmentLayoutItem*>::const_iterator
					j = layoutItems->begin();
					j != layoutItems->end();
					++j)
			{
				if (item->getRules()->getType() != (*j)->getItemType()
					|| (*i)->getItem(
									(*j)->getSlot(),
									(*j)->getSlotX(),
									(*j)->getSlotY()) != NULL)
				{
					continue;
				}

				if ((*j)->getAmmoItem() == "NONE")
					loaded = true;
				else
				{
					loaded = false;

					// maybe we find the layout-ammo on the ground to load it with -
					// yeh, maybe: as in, maybe this works maybe it doesn't
					for (std::vector<BattleItem*>::const_iterator
							k = _tileEquipt->getInventory()->begin();
							k != _tileEquipt->getInventory()->end()
								&& loaded == false;
							++k)
					{
						if ((*k)->getRules()->getType() == (*j)->getAmmoItem()
							&& (*k)->getSlot() == ground		// why the redundancy?
																// WHAT OTHER _tileEquipt IS THERE BUT THE GROUND TILE!!??!!!1
							&& item->setAmmoItem(*k) == 0)		// okay, so load the damn item.
						{
							(*k)->setXCOMProperty();
							(*k)->setSlot(righthand);			// why are you putting ammo in his right hand.....
																// maybe just to get it off the ground so it doesn't get loaded into another weapon later.
							_battleSave->getItems()->push_back(*k);

							loaded = true;
							// note: soldier is not owner of the ammo, we are using this fact when saving equipments
						}
					}
				}

				// only place the weapon (or any other item..) onto the soldier when it's loaded with its layout-ammo (if any)
				if (loaded == true)
				{
					item->setXCOMProperty();

					item->moveToOwner(*i);

					item->setSlot(_rules->getInventory((*j)->getSlot()));
					item->setSlotX((*j)->getSlotX());
					item->setSlotY((*j)->getSlotY());

					if (Options::includePrimeStateInSavedLayout == true
						&& (item->getRules()->getBattleType() == BT_GRENADE
							|| item->getRules()->getBattleType() == BT_PROXYGRENADE))
					{
						item->setFuseTimer((*j)->getFuseTimer());
					}

					_battleSave->getItems()->push_back(item);

					return true;
				}
			}
		}
	}

	return false;
}

/**
 * Sets xCom soldiers' combat clothing style - spritesheets & paperdolls.
 * Uses EqualTerms v1 graphics to replace stock resources. Affects soldiers
 * wearing pyjamas (STR_ARMOR_NONE_UC) only.
 * This is done by switching in/out equivalent Armors.
 */
void BattlescapeGenerator::setTacticalSprites() // private.
{
	RuleArmor* const armorRule = _rules->getArmor(_terrain->getPyjamaType());

	Base* base;
	if (_craft != NULL)
		base = _craft->getBase();
	else
		base = _base;

	for (std::vector<Soldier*>::const_iterator
			i = base->getSoldiers()->begin();
			i != base->getSoldiers()->end();
			++i)
	{
		if ((_craft == NULL
				|| (*i)->getCraft() == _craft)
			&& (*i)->getArmor()->isBasic() == true)
		{
			(*i)->setArmor(armorRule);
		}
	}
}
/*
// base defense, craft NULL "STR_BASE_DEFENSE"
// ufo, base NULL "STR_UFO_CRASH_RECOVERY" "STR_UFO_GROUND_ASSAULT" "STR_TERROR_MISSION" "STR_ALIEN_BASE_ASSAULT"
// cydonia "STR_MARS_CYDONIA_LANDING" "STR_MARS_THE_FINAL_ASSAULT"

	if ((_craft == NULL // both Craft & Base are NULL for the 2nd of a 2-part mission.
			&& _base == NULL)
		|| _missionType == "STR_BASE_DEFENSE")
	{
		return;
	}

	std::string stArmor = "STR_ARMOR_NONE_UC";

	if (_isCity == true)
		stArmor = "STR_STREET_URBAN_UC";
	else if (_missionType == "STR_MARS_CYDONIA_LANDING"
		|| _missionType == "STR_MARS_THE_FINAL_ASSAULT")
	{
		stArmor = "STR_STREET_ARCTIC_UC";
	}
	else if (_missionType == "STR_TERROR_MISSION"
		|| _missionType == "STR_ALIEN_BASE_ASSAULT")
	{
		stArmor = "STR_STREET_URBAN_UC";
	}
	else
	{
		if ((_texture > -1 && _texture < 7)
			|| (_texture > 9 && _texture < 12))
		{
			stArmor = "STR_STREET_JUNGLE_UC";
		}
		else if (_texture > 6 && _texture < 10
			|| _texture == 12)
		{
			stArmor = "STR_STREET_ARCTIC_UC";
		}
	} */

/**
 * Adds an item to an XCom soldier (auto-equip ONLY). kL_note: I don't use that part.
 * Or to an XCom tank, also adds items & terrorWeapons to aLiens, deployAliens()!
 * @param item				- pointer to the BattleItem
 * @param unit				- pointer to the BattleUnit
// * @param allowSecondClip	- true to allow the unit to take a second clip
 *							(only applies to xcom soldiers, aliens are allowed regardless)
 * @return, true if item was placed
 */
bool BattlescapeGenerator::addItem( // private.
		BattleItem* item,
		BattleUnit* unit)
//		bool allowSecondClip
{
	// kL_note: Old code that does what I want:
	RuleInventory
		* const rightHand = _rules->getInventory("STR_RIGHT_HAND"),
		* const leftHand = _rules->getInventory("STR_LEFT_HAND");
	BattleItem
		* const rhWeapon = unit->getItem("STR_RIGHT_HAND"),
		* const lhWeapon = unit->getItem("STR_LEFT_HAND");

	bool placed = false;

	switch (item->getRules()->getBattleType())
	{
		case BT_FIREARM:	// kL_note: These are also terrorist weapons:
		case BT_MELEE:		// chryssalids, cyberdiscs, zombies, sectopods, reapers, celatids, silacoids
			if (rhWeapon == NULL)
			{
				item->moveToOwner(unit);
				item->setSlot(rightHand);

				placed = true;
			}

			// kL_note: only for plasma pistol + Blaster (see itemSets in Ruleset)
			// Also now for advanced fixed/innate weapon rules.
			if (placed == false
				&& lhWeapon == NULL)
//				&& (item->getRules()->isFixed()
//					|| unit->getFaction() != FACTION_PLAYER))
			{
				item->moveToOwner(unit);
				item->setSlot(leftHand);

				placed = true;
			}
		break;

		case BT_AMMO:
		{
			// find handheld weapons that can be loaded with this ammo
			if (rhWeapon != NULL
				&& (rhWeapon->getRules()->isFixed() == true
					|| unit->getFaction() != FACTION_PLAYER)
				&& rhWeapon->getAmmoItem() == NULL
				&& rhWeapon->setAmmoItem(item) == 0)
			{
				item->setSlot(rightHand);

				placed = true;
				break;
			}

			// kL_note: only for plasma pistol + Blaster (see itemSets in Ruleset)
			// Also now for advanced fixed/innate weapon rules.
			if (lhWeapon != NULL
				&& (lhWeapon->getRules()->isFixed() == true
					|| unit->getFaction() != FACTION_PLAYER)
				&& lhWeapon->getAmmoItem() == NULL
				&& lhWeapon->setAmmoItem(item) == 0)
			{
				item->setSlot(leftHand);

				placed = true;
				break;
			}

			// else put the clip in Belt or Backpack
			RuleItem* const itemRule = item->getRules();

			for (int
					i = 0;
					i != 4;
					++i)
			{
				if (unit->getItem("STR_BELT", i) == false
					&& _rules->getInventory("STR_BELT")->fitItemInSlot(itemRule, i, 0))
				{
					item->moveToOwner(unit);
					item->setSlot(_rules->getInventory("STR_BELT"));
					item->setSlotX(i);

					placed = true;
					break;
				}
			}

			if (placed == false)
			{
				for (int
						i = 0;
						i != 3;
						++i)
				{
					if (unit->getItem("STR_BACK_PACK", i) == false
						&& _rules->getInventory("STR_BACK_PACK")->fitItemInSlot(itemRule, i, 0))
					{
						item->moveToOwner(unit);
						item->setSlot(_rules->getInventory("STR_BACK_PACK"));
						item->setSlotX(i);

						placed = true;
						break;
					}
				}
			}
		}
		break;

		case BT_GRENADE: // includes AlienGrenades & SmokeGrenades & HE-Packs.
		case BT_PROXYGRENADE:
			for (int
					i = 0;
					i != 4;
					++i)
			{
				if (unit->getItem("STR_BELT", i) == false)
				{
					item->moveToOwner(unit);
					item->setSlot(_rules->getInventory("STR_BELT"));
					item->setSlotX(i);

					placed = true;
					break;
				}
			}
		break;

		case BT_MINDPROBE:
		case BT_MEDIKIT:
		case BT_SCANNER:
			if (unit->getItem("STR_BACK_PACK") == false)
			{
				item->moveToOwner(unit);
				item->setSlot(_rules->getInventory("STR_BACK_PACK"));

				placed = true;
			}
		break;

		default:
		break;
	}

	if (placed == true)
	{
		item->setXCOMProperty(unit->getFaction() == FACTION_PLAYER);
		_battleSave->getItems()->push_back(item);
	}

	// If not placed, the item is deleted from wherever this function was called.
	return placed;
}
// New code that sucks:
//kL	int weight = 0;
	// tanks and aliens don't care about weight or multiple items; their
	// loadouts are defined in the rulesets and more or less set in stone.
/*kL
	if (unit->getFaction() == FACTION_PLAYER // XCOM Soldiers!!! auto-equip
		&& unit->hasInventory())
	{
		weight = unit->getCarriedWeight() + item->getRules()->getWeight();

		if (item->getAmmoItem()
			&& item->getAmmoItem() != item)
		{
			weight += item->getAmmoItem()->getRules()->getWeight();
		}

		// allow all weapons to be loaded by avoiding this check; they'll
		// return false later anyway if the unit has something in its hand.
		if (item->getRules()->getCompatibleAmmo()->empty())
		{
			int tally = 0;

			for (std::vector<BattleItem*>::iterator
					i = unit->getInventory()->begin();
					i != unit->getInventory()->end();
					++i)
			{
				if (item->getRules()->getType() == (*i)->getRules()->getType())
				{
					if (allowSecondClip
						&& item->getRules()->getBattleType() == BT_AMMO)
					{
						tally++;
						if (tally == 2)
						{
							return false;
						}
					}
					else // we already have one, thanks.
					{
						return false;
					}
				}
			}
		}
	} */

//	RuleInventory* ground = _rules->getInventory("STR_GROUND"); // removed.
/*	RuleInventory* rightHand = _rules->getInventory("STR_RIGHT_HAND");
	RuleInventory* leftHand = _rules->getInventory("STR_LEFT_HAND");
	BattleItem* rhWeapon = unit->getItem("STR_RIGHT_HAND");
	BattleItem* lhWeapon = unit->getItem("STR_LEFT_HAND");

	bool placed = false; */
//	bool loaded = false;

/*kL
	bool keep = true;
	switch (item->getRules()->getBattleType())
	{
		case BT_FIREARM:
		case BT_MELEE:
			if (item->getAmmoItem()							// doesn't need ammo
				|| unit->getFaction() != FACTION_PLAYER		// is for an aLien (or civie)
				|| !unit->hasInventory())					// is a large unit
			{
				loaded = true;
			}

			if (loaded
				&& (unit->getGeoscapeSoldier() == 0
					|| _allowAutoLoadout))
			{
				if (!rhWeapon
					&& unit->getBaseStats()->strength * 0.66 >= weight)
				{
					item->moveToOwner(unit);
					item->setSlot(rightHand);

					placed = true;
				}
				if (!placed
					&& !lhWeapon
					&& (unit->getFaction() != FACTION_PLAYER
						|| item->getRules()->isFixed()))
				{
					item->moveToOwner(unit);
					item->setSlot(leftHand);

					placed = true;
				}
			}
		break;
		case BT_AMMO:
		// xcom weapons will already be loaded, aliens and tanks, however, get their ammo added afterwards.
		// so let's try to load them here.
		if (rightWeapon && (rightWeapon->getRules()->isFixed() || unit->getFaction() != FACTION_PLAYER) &&
			!rightWeapon->getRules()->getCompatibleAmmo()->empty() &&
			!rightWeapon->getAmmoItem() &&
			rightWeapon->setAmmoItem(item) == 0)
		{
			item->setSlot(rightHand);
			placed = true;
			break;
		}
		if (leftWeapon && (leftWeapon->getRules()->isFixed() || unit->getFaction() != FACTION_PLAYER) &&
			!leftWeapon->getRules()->getCompatibleAmmo()->empty() &&
			!leftWeapon->getAmmoItem() &&
			leftWeapon->setAmmoItem(item) == 0)
		{
			item->setSlot(leftHand);
			placed = true;
			break;
		}
		// don't take ammo for weapons we don't have.
		keep = (unit->getFaction() != FACTION_PLAYER);
		if (rightWeapon)
		{
			for (std::vector<std::string>::iterator i = rightWeapon->getRules()->getCompatibleAmmo()->begin(); i != rightWeapon->getRules()->getCompatibleAmmo()->end(); ++i)
			{
				if (*i == item->getRules()->getType())
				{
					keep = true;
					break;
				}
			}
		}
		if (leftWeapon)
		{
			for (std::vector<std::string>::iterator i = leftWeapon->getRules()->getCompatibleAmmo()->begin(); i != leftWeapon->getRules()->getCompatibleAmmo()->end(); ++i)
			{
				if (*i == item->getRules()->getType())
				{
					keep = true;
					break;
				}
			}
		}
		if (!keep)
		{
			break;
		}
		default:
		if ((unit->getGeoscapeSoldier() == 0 || _allowAutoLoadout))
		{
			if (unit->getBaseStats()->strength >= weight) // weight is always considered 0 for aliens
			{
				for (std::vector<std::string>::const_iterator
						i = _rules->getInvsList().begin();
						i != _rules->getInvsList().end()
							&& !placed;
						++i)
				{
					RuleInventory* slot = _rules->getInventory(*i);
					if (slot->getType() == INV_SLOT)
					{
						for (std::vector<RuleSlot>::iterator
								j = slot->getSlots()->begin();
								j != slot->getSlots()->end()
									&& !placed;
								++j)
						{
							if (!Inventory::overlapItems(
														unit,
														item,
														slot,
														j->x,
														j->y)
								&& slot->fitItemInSlot(
													item->getRules(),
													j->x,
													j->y))
							{
								item->moveToOwner(unit);

								item->setSlot(slot);
								item->setSlotX(j->x);
								item->setSlotY(j->y);

								placed = true;

								break;
							}
						}
					}
				}
			}
		}
		break;
	} */

/**
 * Deploys the aLiens according to the AlienDeployment rules.
 * @param depRule - pointer to the AlienDeployment rule
 */
void BattlescapeGenerator::deployAliens(AlienDeployment* const depRule) // private.
{
	if (depRule->getRace().empty() == false
		&& _alienRace.empty() == true) //&& month != -1
	{
		_alienRace = depRule->getRace();
	}

/*	if (_battleSave->getDepth() > 0
		&& _alienRace.find("_UNDERWATER") == std::string::npos)
	{
		_alienRace += "_UNDERWATER";
	} */

	const AlienRace* const race = _game->getRuleset()->getAlienRace(_alienRace);

	if (race == NULL)
	{
		throw Exception("Map generator encountered an error: Unknown race: "
			  + _alienRace + " defined in depRule: " + depRule->getType());
	}

	int month = _gameSave->getMonthsPassed();
	if (month != -1)
	{
		const int itemLevel_top = static_cast<int>(_rules->getAlienItemLevels().size()) - 1;
		if (month > itemLevel_top)
			month = itemLevel_top;
	}
	else
		month = _alienItemLevel;


	std::string aLien;
	bool outside;
	int qty;
	size_t itemLevel;

	RuleItem* itRule;
	BattleItem* item;
	RuleUnit* unitRule;
	BattleUnit* unit;

	for (std::vector<DeploymentData>::const_iterator
			data = depRule->getDeploymentData()->begin();
			data != depRule->getDeploymentData()->end();
			++data)
	{
		aLien = race->getMember((*data).alienRank);

		if (_gameSave->getDifficulty() < DIFF_VETERAN)
			qty = (*data).lowQty
				+ RNG::generate( // beginner/experienced
							0,
							(*data).dQty);
		else if (_gameSave->getDifficulty() < DIFF_SUPERHUMAN)
			qty = (*data).lowQty
				+ (((*data).highQty - (*data).lowQty) / 2)
				+ RNG::generate( // veteran/genius
							0,
							(*data).dQty);
		else
			qty = (*data).highQty
				+ RNG::generate( // super (and beyond)
							0,
							(*data).dQty);

		qty += RNG::generate(
						0,
						(*data).extraQty);

		if (_base != NULL
			&& _base->getDefenseResult() != 0)
		{
			qty = std::max(
						qty / 2,
						qty - (qty * _base->getDefenseResult() / 100));
		}

		for (int
				i = 0;
				i != qty;
				++i)
		{
			if (_ufo != NULL)
				outside = RNG::percent((*data).pctOutsideUfo);
			else
				outside = false;

			unitRule = _rules->getUnit(aLien);
			unit = addAlien(
						unitRule,
						(*data).alienRank,
						outside);

			if (unit != NULL)
			{
				// Built in weapons: the unit has this weapon regardless of loadout or what have you.
				if (unitRule->getBuiltInWeapons().empty() == false)
				{
					for (std::vector<std::string>::const_iterator
							j = unitRule->getBuiltInWeapons().begin();
							j != unitRule->getBuiltInWeapons().end();
							++j)
					{
						itRule = _rules->getItem(*j);
						if (itRule != NULL)
						{
							item = new BattleItem(
												itRule,
												_battleSave->getNextItemId());
							if (addItem(
									item,
									unit) == false)
							{
								delete item;
							}
						}
					}
				}

				// terrorist aliens' equipment is a special case - they are fitted
				// with a weapon which is the alien's name with suffix _WEAPON
				if (unitRule->isLivingWeapon() == true)
				{
					std::string terrorWeapon = unitRule->getRace().substr(4);
					terrorWeapon += "_WEAPON";

					itRule = _rules->getItem(terrorWeapon);
					if (itRule != NULL)
					{
						item = new BattleItem( // terror aLiens add their weapons
											itRule,
											_battleSave->getNextItemId());
						if (addItem(
								item,
								unit) == false)
						{
							delete item;
						}
						else
							unit->setTurretType(item->getRules()->getTurretType());
					}
				}
				else
				{
					if ((*data).itemSets.size() == 0)
					{
						throw Exception("Unit generator encountered an error: item set not defined");
					}

					itemLevel = static_cast<size_t>(_rules->getAlienItemLevels().at(static_cast<size_t>(month)).at(RNG::generate(0,9)));
					if (itemLevel > (*data).itemSets.size() - 1)
						itemLevel = (*data).itemSets.size() - 1;
					// Relax item level requirements
					// <- Yankes; https://github.com/Yankes/OpenXcom/commit/4c252470aa2e261b0f449a56aaea5d5b0cb2229c
/*					if (itemLevel > (*data).itemSets.size() - 1)
					{
						std::stringstream ststr;
						ststr	<< "Unit generator encountered an error: not enough item sets defined, expected: "
								<< (itemLevel+1) << " found: " << (*data).itemSets.size();
						throw Exception(ststr.str());
					} */

					for (std::vector<std::string>::const_iterator
							setItem = (*data).itemSets.at(itemLevel).items.begin();
							setItem != (*data).itemSets.at(itemLevel).items.end();
							++setItem)
					{
						itRule = _rules->getItem(*setItem);
						if (itRule != NULL)
						{
							item = new BattleItem( // aLiens add items
												itRule,
												_battleSave->getNextItemId());
							if (addItem(
									item,
									unit) == false)
							{
								delete item;
							}
						}
					}
				}
			}
		}
	}

	if (_base != NULL)
		_base->setDefenseResult(0);
}

/**
 * Adds an alien to the game and places him on a free spawnpoint.
 * @param unitRule	- pointer to the Unit rule which holds info about aLiens
 * @param alienRank	- rank of the alien, used for spawn point search
 * @param outside	- true if the alien should spawn outside the UFO
 * @return, pointer to the created BattleUnit
 */
BattleUnit* BattlescapeGenerator::addAlien( // private.
		RuleUnit* const unitRule,
		int alienRank,
		bool outside)
{
	const int
		diff = static_cast<int>(_gameSave->getDifficulty()),
		month = _gameSave->getMonthsPassed();

	BattleUnit* const unit = new BattleUnit(
										unitRule,
										FACTION_HOSTILE,
										_unitSequence++,
										_rules->getArmor(unitRule->getArmor()),
										diff,
//										_battleSave->getDepth(),
										month);

	// following data is the order in which certain alien ranks spawn on certain node ranks;
	// note that they all can fall back to rank 0 nodes - which is scout (outside ufo)
	Node* node;
	if (outside == true)
		node = _battleSave->getSpawnNode( // Civ-Scout spawnpoints <- 'outside'
									NR_SCOUT,
									unit);
	else
		node = NULL;

	const size_t ranks_2 = (sizeof(Node::nodeRank) / sizeof(Node::nodeRank[0][0]))
						 / (sizeof(Node::nodeRank) / sizeof(Node::nodeRank[0]));
	if (node == NULL) // ie. if not spawning on a Civ-Scout node
	{
		for (size_t
				i = 0;
				i != ranks_2 // =8, 2nd dimension of nodeRank[][], ref Node.cpp
					&& node == NULL;
				++i)
		{
			node = _battleSave->getSpawnNode(
										Node::nodeRank[alienRank][i],
										unit);
		}
	}

	if ((node != NULL
			&& _battleSave->setUnitPosition(
										unit,
										node->getPosition()) == true)
		|| placeUnitNearFriend(unit) == true)
	{
		unit->setAIState(new AlienBAIState(
										_battleSave,
										unit,
										node));
		unit->setRankInt(alienRank);

		Position posCraft = _battleSave->getUnits()->at(0)->getPosition(); // aLiens face Craft
		int dir;
		if (RNG::percent(diff * 20) == true
			&& _battleSave->getTileEngine()->distance(
													node->getPosition(),
													posCraft) < 25)
		{
			dir = unit->directionTo(posCraft);
		}
		else
			dir = _battleSave->getTileEngine()->faceWindow(node->getPosition());

		if (dir == -1)
			dir = RNG::generate(0,7);
		unit->setDirection(dir);

		int tu = unit->getTimeUnits();
		tu = static_cast<int>(ceil(
			 static_cast<double>(tu) * RNG::generate(0.,1.)));
		unit->setTimeUnits(tu);

		_battleSave->getUnits()->push_back(unit);
	}
	else
	{
		delete unit;
		return NULL;
	}

	return unit;
}

/**
 * Places a unit near a friendly unit.
 * @param unit - pointer to the BattleUnit in question
 * @return, true if the unit was successfully placed
 */
bool BattlescapeGenerator::placeUnitNearFriend(BattleUnit* const unit) // private.
{
	if (unit != NULL
		&& _battleSave->getUnits()->empty() == false)
	{
		const BattleUnit* bu;
		Position posEntry;
		int tries;
		bool largeUnit;

		for (int
				i = 0;
				i != 10;
				++i)
		{
			posEntry = Position(-1,-1,-1);
			tries = 100;
			largeUnit = false;

			while (posEntry == Position(-1,-1,-1)
				&& tries > 0)
			{
				bu = _battleSave->getUnits()->at(RNG::generate(
															0,
															static_cast<int>(_battleSave->getUnits()->size()) - 1));
				if (bu->getFaction() == unit->getFaction()
					&& bu->getPosition() != Position(-1,-1,-1)
					&& bu->getArmor()->getSize() >= unit->getArmor()->getSize())
				{
					posEntry = bu->getPosition();
					largeUnit = (bu->getArmor()->getSize() == 2);
				}

				--tries;
			}

			if (tries > 0
				&& _battleSave->placeUnitNearPosition(
													unit,
													posEntry,
													largeUnit) == true)
			{
				return true;
			}
		}
	}

	return false;
}

/**
 * Spawns civilians on a terror mission.
 * @param civilians - maximum number of civilians to spawn
 */
void BattlescapeGenerator::deployCivilians(int civilians) // private.
{
	if (civilians > 0)
	{
		const int qty = RNG::generate(
									civilians / 2,
									civilians);
		if (qty > 0)
		{
			size_t pick;
			for (int
					i = 0;
					i != qty;
					++i)
			{
				pick = RNG::generate(
								0,
								_terrain->getCivilianTypes().size() - 1);
				addCivilian(_rules->getUnit(_terrain->getCivilianTypes().at(pick)));
			}
		}
	}
}

/**
 * Adds a civilian to the game and places him on a free spawnpoint.
 * @param rules - pointer to the Unit rule which holds info about civilians
 * @return, pointer to the created BattleUnit
 */
BattleUnit* BattlescapeGenerator::addCivilian(RuleUnit* rules) // private.
{
	BattleUnit* const unit = new BattleUnit(
										rules,
										FACTION_NEUTRAL,
										_unitSequence++,
										_rules->getArmor(rules->getArmor()),
										0);
//										_battleSave->getDepth());

	Node* const node = _battleSave->getSpawnNode(
													NR_SCOUT,
													unit);
	if ((node != NULL
			&& _battleSave->setUnitPosition(
										unit,
										node->getPosition()) == true)
		|| placeUnitNearFriend(unit) == true)
	{
		unit->setAIState(new CivilianBAIState(
										_battleSave,
										unit,
										node));

		unit->setDirection(RNG::generate(0,7));

		_battleSave->getUnits()->push_back(unit);
	}
	else
	{
		delete unit;
		return NULL;
	}

	return unit;
}

/**
 * Loads a MAP file into the tiles of the BattleGame.
 * @param mapblock		- pointer to MapBlock
 * @param offset_x		- Mapblock offset in X direction
 * @param offset_y		- Mapblock offset in Y direction
 * @param terrainRule	- pointer to RuleTerrain
 * @param dataSetOffset	-
 * @param discovered	- true if this MapBlock is discovered (eg. landingsite of the Skyranger)
 * @param craft			- true if xCom Craft has landed on the MAP
 * @return, height of the loaded mapblock (needed for spawnpoint calculation)
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 * @note Y-axis is in reverse order.
 */
int BattlescapeGenerator::loadMAP( // private.
		MapBlock* const mapblock,
		int offset_x,
		int offset_y,
		RuleTerrain* terrainRule,
		int dataSetOffset,
		bool discovered,
		bool craft)
{
	std::ostringstream file;
	file << "MAPS/" << mapblock->getType() << ".MAP";
	std::ifstream mapFile ( // init. Load file
						CrossPlatform::getDataFile(file.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (mapFile.fail() == true)
	{
		throw Exception(file.str() + " not found");
	}

	char array_Map[3];
	mapFile.read(
			(char*)&array_Map,
			sizeof(array_Map));
	const int
		size_x = static_cast<int>(array_Map[1]), // note X-Y switch!
		size_y = static_cast<int>(array_Map[0]), // note X-Y switch!
		size_z = static_cast<int>(array_Map[2]);

	mapblock->setSizeZ(size_z);

	std::stringstream ststr;
	if (size_z > _battleSave->getMapSizeZ())
	{
		ststr << "Height of map " + file.str() + " too big for this mission, block is " << size_z << ", expected: " << _battleSave->getMapSizeZ();
		throw Exception(ststr.str());
	}


	if (   size_x != mapblock->getSizeX()
		|| size_y != mapblock->getSizeY())
	{
		ststr << "Map block is not of the size specified " + file.str() + " is " << size_x << "x" << size_y << " , expected: " << mapblock->getSizeX() << "x" << mapblock->getSizeY();
		throw Exception(ststr.str());
	}

	int
		x = offset_x,
		y = offset_y,
		z = size_z - 1;

	for (int
			i = _mapsize_z - 1;
			i != 0;
			--i)
	{
		// check if there is already a layer - if so, move Z up
		const MapData* const floorData = _battleSave->getTile(Position(x,y,i))->getMapData(O_FLOOR);
		if (floorData != NULL)
		{
			z += i;

			if (craft == true)
			{
				_craftZ = i;
				_battleSave->setGroundLevel(i);
			}

			break;
		}
	}

	if (z > _battleSave->getMapSizeZ() - 1)
	{
		throw Exception("Something is wrong in the map definitions, craft/ufo map is too tall.");
	}


	unsigned int partId;
	bool revealDone;

	unsigned char array_Parts[4];

	while (mapFile.read(
					(char*)&array_Parts,
					sizeof(array_Parts)) != NULL)
	{
		revealDone = false;

		for (int
				part = 0;
				part != 4;
				++part)
		{
			partId = static_cast<unsigned int>(array_Parts[static_cast<size_t>(part)]);

			// kL_begin: Remove natural terrain that is inside Craft or Ufo. This mimics UFO:orig behavior.
			if (part != 0			// not if it's a floor since Craft/Ufo part will overwrite it anyway
				&& partId == 0	// and only if no Craft/Ufo part would overwrite the part
				&& array_Parts[0] != 0)		// but only if there *is* a floor-part to the Craft/Ufo, so it would (have) be(en) inside the Craft/Ufo
			{
				_battleSave->getTile(Position(x,y,z))->setMapData(NULL, -1,-1, part);
			} // kL_end.

			// Then overwrite previous terrain with Craft or Ufo terrain.
			// nb. See sequence of map-loading in generateMap() (1st terrain, 2nd Ufo, 3rd Craft) <- preMapScripting.
			if (partId > 0)
			{
				unsigned int dataId = partId;
				int dataSetId = dataSetOffset;

				MapData* const data = terrainRule->getMapData(
														&dataId,
														&dataSetId);
//				if (dataSetOffset > 0) // ie: ufo or craft.
//					_battleSave->getTile(Position(x,y,z))->setMapData(NULL,-1,-1,3); // erase content-object

				_battleSave->getTile(Position(x,y,z))->setMapData(
																data,
																static_cast<int>(dataId),
																dataSetId,
																part);
			}

/*			// If the part is not a floor and is empty, remove it; this prevents growing grass in UFOs.
			// note: And outside UFOs. so remark it
			if (part == 3
				&& partId == 0)
			{
				_battleSave->getTile(Position(x,y,z))->setMapData(NULL,-1,-1, part);
			} */

			// kL_begin: Reveal only tiles inside the Craft.
			if (craft == true
				&& partId != 0
				&& z != _craftZ)
			{
				revealDone = true;
				_battleSave->getTile(Position(x,y,z))->setDiscovered(true, 2);
			} // kL_end.
		}

/*		if (craft
			&& _craftZ == z)
		{
			for (int
					z2 = _battleSave->getMapSizeZ() - 1;
					z2 >= _craftZ;
					--z2)
			{
				_battleSave->getTile(Position(x,y,z2))->setDiscovered(true, 2);
			}
		} */

		if (revealDone == false)
			_battleSave->getTile(Position(x,y,z))->setDiscovered(
															discovered == true
																|| mapblock->isFloorRevealed(z) == true,
															2);
		++x;

		if (x == size_x + offset_x)
		{
			x = offset_x;
			++y;
		}

		if (y == size_y + offset_y)
		{
			y = offset_y;
			--z;
		}
	}

	if (mapFile.eof() == false)
	{
		throw Exception("Invalid MAP file: " + file.str());
	}

	mapFile.close();

	if (_generateFuel == true) // if one of the mapBlocks has an items array defined, don't deploy fuel algorithmically
		_generateFuel = (mapblock->getItems()->empty() == true);

	for (std::map<std::string, std::vector<Position> >::const_iterator
			i = mapblock->getItems()->begin();
			i != mapblock->getItems()->end();
			++i)
	{
		RuleItem* const itRule = _rules->getItem((*i).first);
		for (std::vector<Position>::const_iterator
				j = (*i).second.begin();
				j != (*i).second.end();
				++j)
		{
			BattleItem* const item = new BattleItem(
												itRule,
												_battleSave->getNextItemId());
			_battleSave->getItems()->push_back(item);
			_battleSave->getTile((*j) + Position(
												offset_x,
												offset_y,
												0))
											->addItem(
													item,
													_rules->getInventory("STR_GROUND"));
		}
	}

	return size_z;
}

/**
 * Loads an XCom format RMP file as battlefield Nodes.
 * @param mapblock	- pointer to MapBlock
 * @param xoff		- Mapblock offset in X direction
 * @param yoff		- Mapblock offset in Y direction
 * @param segment	- Mapblock segment
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
void BattlescapeGenerator::loadRMP( // private.
		MapBlock* mapblock,
		int xoff,
		int yoff,
		int segment)
{
	char array_RMP[24];

	std::ostringstream file;
	file << "ROUTES/" << mapblock->getType() << ".RMP";

	std::ifstream mapFile ( // init. Load file
					CrossPlatform::getDataFile(file.str()).c_str(),
					std::ios::in | std::ios::binary);
	if (mapFile.fail() == true)
	{
		throw Exception(file.str() + " not found");
	}

	int
		pos_x,
		pos_y,
		pos_z,

		unitType,
		nodeRank,
		ptrlImport,
		disBase,
		spPriority,

		linkId;

	const int nodeOffset = static_cast<int>(_battleSave->getNodes()->size());
	Node* node;
	Position pos;

	while (mapFile.read(
					(char*)&array_RMP,
					sizeof(array_RMP)) != NULL)
	{
		pos_x = static_cast<int>(array_RMP[1]); // note: Here is where x-y values get reversed
		pos_y = static_cast<int>(array_RMP[0]); // vis-a-vis values in .RMP files vs. loaded values.
		pos_z = static_cast<int>(array_RMP[2]);

		if (   pos_x < mapblock->getSizeX()
			&& pos_y < mapblock->getSizeY()
			&& pos_z < _mapsize_z)
		{
			pos = Position(
						xoff + pos_x,
						yoff + pos_y,
						mapblock->getSizeZ() - pos_z - 1);

			unitType	= static_cast<int>(array_RMP[19]); // -> Any=0; Flying=1; Small=2; FlyingLarge=3; Large=4
			nodeRank	= static_cast<int>(array_RMP[20]);
			ptrlImport	= static_cast<int>(array_RMP[21]);
			disBase		= static_cast<int>(array_RMP[22]);
			spPriority	= static_cast<int>(array_RMP[23]);

			// TYPE_FLYING		= 0x01 -> ref Savegame/Node.h
			// TYPE_SMALL		= 0x02
			// TYPE_LARGEFLYING	= 0x04
			// TYPE_LARGE		= 0x08
			// TYPE_DANGEROUS	= 0x10 <- not in RMP file.
			if		(unitType == 3) unitType = 4; // kL
			else if (unitType == 4) unitType = 8; // kL

//			if (_missionType != "STR_BASE_DEFENSE")
			if (_battleSave->getTacticalType() != TCT_BASEDEFENSE)
				disBase = 0; // kL, ensure these get zero'd for nonBaseDefense battles; cf. Node::isTarget()

			node = new Node(
						_battleSave->getNodes()->size(),
						pos,
						segment,
						unitType,
						nodeRank,
						ptrlImport,
						disBase,
						spPriority);

			for (size_t
					j = 0;
					j != 5;
					++j)
			{
				linkId = static_cast<int>(array_RMP[(j * 3) + 4]); // <- 4[5,6],7[8,9],10[11,12],13[14,15],16[17,18] -> [unitType & distance of linked nodes are not used]

				if (linkId < 251) // do not offset special values; ie. links to N,S,E,West, or none.
					linkId += nodeOffset;
				else
					linkId -= 256;
					// 255 -> -1 = unused
					// 254 -> -2 = north
					// 253 -> -3 = east
					// 252 -> -4 = south
					// 251 -> -5 = west

				std::vector<int>* nodeLinks = node->getNodeLinks();
				if (std::find(
							nodeLinks->rbegin(),
							nodeLinks->rend(),
							linkId) != nodeLinks->rend())
				{
					linkId = -1;	// <- for Expanded U_BASE, which seems to have multiple identical links on nodes.
				}					// TODO: yeah i'm working on it but things like 'mapView' are involved ....

				nodeLinks->push_back(linkId);
			}

			_battleSave->getNodes()->push_back(node);
		}
	}

	if (mapFile.eof() == false)
	{
		throw Exception("Invalid RMP file");
	}

	mapFile.close();
}

/**
 * Fill power sources with an alien fuel object.
 */
void BattlescapeGenerator::fuelPowerSources() // private.
{
	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		if (   _battleSave->getTiles()[i]->getMapData(O_OBJECT)
			&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE)
		{
			BattleItem* const alienFuel = new BattleItem(
													_rules->getItem(_rules->getAlienFuelType()),
													_battleSave->getNextItemId());

			_battleSave->getItems()->push_back(alienFuel);
			_battleSave->getTiles()[i]->addItem(
											alienFuel,
											_rules->getInventory("STR_GROUND"));
		}
	}
}

/**
 * When a UFO crashes, there is a chance for each powersource to explode.
 */
void BattlescapeGenerator::explodePowerSources() // private.
{
	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		if (   _battleSave->getTiles()[i]->getMapData(O_OBJECT) != NULL
			&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE
			&& RNG::percent(80) == true)
		{
			const Position pos = Position(
									_battleSave->getTiles()[i]->getPosition().x * 16 + 8,
									_battleSave->getTiles()[i]->getPosition().y * 16 + 8,
									_battleSave->getTiles()[i]->getPosition().z * 24 + 12);

			double power = static_cast<double>(_ufo->getDamagePercent());	// range: ~50+ to ~100-
			if (RNG::percent(static_cast<int>(power) / 2) == true)			// chance for full range Explosion (even if crash took low damage)
				power = RNG::generate(1.,100.);

			power *= RNG::generate(0.1,2.);
			power += std::pow(power, 2) / 160.;

			if (power > 0.5)
				_battleSave->getTileEngine()->explode(
													pos,
													static_cast<int>(std::ceil(power)),
													DT_HE,
													21);
		}
	}

	const Tile* tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	while (tile != NULL)
	{
		const Position pos = Position(
									tile->getPosition().x * 16 + 8,
									tile->getPosition().y * 16 + 8,
									tile->getPosition().z * 24 + 10);
		_battleSave->getTileEngine()->explode(
											pos,
											tile->getExplosive(),
											DT_HE,
											tile->getExplosive() / 10);

		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	}
}

/**
 * Creates a mini-battle-save for managing inventory from the Geoscape's CraftEquip or BaseEquip screen.
 * Kids, don't try this at home! yer tellin' me.
 * @param craft		- pointer to Craft to handle
 * @param base		- pointer to Base to handle (default NULL)
 * @param selUnit	- soldier to display in battle pre-equip inventory (default 0)
 */
void BattlescapeGenerator::runInventory(
		Craft* craft,
		Base* base,
		size_t selUnit)
{
	_baseEquipScreen = true;

	int qtySoldiers;
	if (craft != NULL)
		qtySoldiers = craft->getNumSoldiers();
	else
		qtySoldiers = base->getAvailableSoldiers(true);

	_mapsize_x = qtySoldiers; // fake a map for soldier placement
	_mapsize_y =
	_mapsize_z = 1;

	_battleSave->initMap(
					_mapsize_x,
					_mapsize_y,
					_mapsize_z);

	MapDataSet* const dataSet = new MapDataSet(
											"blank",
											_game);
	MapData* const data = new MapData(dataSet);

	for (size_t
			i = 0;
			i != static_cast<size_t>(qtySoldiers);
			++i)
	{
		Tile* const tile = _battleSave->getTiles()[i];
		tile->setMapData(
					data,
					0,0,
					O_FLOOR);
		tile->getMapData(O_FLOOR)->setSpecialType(
														START_POINT,
														O_FLOOR);
		tile->getMapData(O_FLOOR)->setTUWalk(0);
		tile->getMapData(O_FLOOR)->setFlags(
													false,
													false,
													false,
													0,
													false,
													false,
													false,
													false,
													false);
	}

	if (craft != NULL)
		setCraft(craft); // generate the battleitems for inventory
	else
		setBase(base);

	deployXCOM();

	if (craft != NULL
		&& selUnit != 0
		&& static_cast<int>(selUnit) <= qtySoldiers)
	{
		size_t j = 0;
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if (++j == selUnit)
			{
				_battleSave->setSelectedUnit(*i);
				break;
			}
		}
	}

	delete data;
	delete dataSet;
}

/**
 * Generates a map of modules (sets of tiles) for a new Battlescape game.
 * @param script - the scripts to use to build the map
 */
void BattlescapeGenerator::generateMap(const std::vector<MapScript*>* const script) // private.
{
	// set ambient sound
//	_battleSave->setAmbientSound(_terrain->getAmbience());

	// set up map generation vars
	_testBlock = new MapBlock("testBlock");

	init();

	MapBlock
		* craftMap = NULL,
		* ufoMap = NULL;

	int
		mapDataSetIDOffset = 0,
		craftDataSetIDOffset = 0;

	// create an array to track command success/failure
	std::map<int, bool> conditionals;

	for (std::vector<MapDataSet*>::const_iterator
			i = _terrain->getMapDataSets()->begin();
			i != _terrain->getMapDataSets()->end();
			++i)
	{
		(*i)->loadData();

		if (_rules->getMCDPatch((*i)->getName()) != NULL)
			_rules->getMCDPatch((*i)->getName())->modifyData(*i);

		_battleSave->getMapDataSets()->push_back(*i);
		++mapDataSetIDOffset;
	}


	// generate the map now and store it inside the tile objects

	// this mission type is "hard-coded" in terms of map layout
//	if (_missionType == "STR_BASE_DEFENSE")
	if (_battleSave->getTacticalType() == TCT_BASEDEFENSE)
		generateBaseMap();

	// process script
	MapScript* command;
	for (std::vector<MapScript*>::const_iterator
			i = script->begin();
			i != script->end();
			++i)
	{
		command = *i;

		if (command->getLabel() > 0
			&& conditionals.find(command->getLabel()) != conditionals.end())
		{
			throw Exception("Map generator encountered an error: multiple commands are sharing the same label.");
		}

		bool& success = conditionals[command->getLabel()] = false;


		// if this command runs conditionally on the failures or successes of previous commands
		if (command->getConditionals()->empty() == false)
		{
			bool execute = true;

			// compare the corresponding entries in the success/failure vector
			for (std::vector<int>::const_iterator
					condition = command->getConditionals()->begin();
					condition != command->getConditionals()->end();
					++condition)
			{
				// positive numbers indicate conditional on success, negative means conditional on failure
				// ie: [1, -2] means this command only runs if command 1 succeeded and command 2 failed.
				if (conditionals.find(std::abs(*condition)) != conditionals.end())
				{
					if ((*condition > 0
							&& conditionals[*condition] == false)
						|| (*condition < 0
							&& conditionals[std::abs(*condition)] == true))
					{
						execute = false;
						break;
					}
				}
				else
				{
					throw Exception("Map generator encountered an error: conditional command expected a label that did not exist before this command.");
				}
			}

			if (execute == false)
				continue;
		}

		// if there's a chance a command won't execute by design, take that into account here
		if (RNG::percent(command->getChancesOfExecution()) == true)
		{
			// initialize the block selection arrays
			command->init();

			// each command can be attempted multiple times, as randomization within the rects may occur
			for (int
					j = 0;
					j != command->getExecutions();
					++j)
			{
				MapBlock* block = NULL;
				int
					x,y;

				switch (command->getType())
				{
					case MSC_ADDBLOCK:
						block = command->getNextBlock(_terrain);
						// select an X and Y position from within the rects, using an even distribution
						if (block != NULL
							&& selectPosition(
											command->getRects(),
											x,y,
											block->getSizeX(),
											block->getSizeY()) == true)
						{
							success = (addBlock(x,y, block) == true)
									|| success;
						}
					break;

					case MSC_ADDLINE:
						success = (addLine(
										(MapDirection)(command->getDirection()),
										command->getRects()) == true);
					break;

					case MSC_ADDCRAFT:
						if (_craft != NULL)
						{
							craftMap = _craft->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(
																										999,999,
																										0,
																										false);
							if (addCraft(
										craftMap,
										command,
										_craftPos) == true)
							{
								// by default addCraft adds blocks from group 1.
								// this can be overwritten in the command by defining specific groups or blocks
								// or this behaviour can be suppressed by leaving group 1 empty
								// this is intentional to allow for TFTD's cruise liners/etc
								// in this situation, you can end up with ANYTHING under your craft, so be careful
								for (
										x = _craftPos.x;
										x < _craftPos.x + _craftPos.w;
										++x)
								{
									for (
											y = _craftPos.y;
											y < _craftPos.y + _craftPos.h;
											++y)
									{
										if (_blocks[x][y] != NULL)
											loadMAP(
												_blocks[x][y],
												x * 10,
												y * 10,
												_terrain,
												0);
									}
								}

								_craftDeployed = true;
								success = true;
							}
						}
					break;

					case MSC_ADDUFO:
						if (_ufo != NULL)
						{
							// as above, note that the craft and the ufo will never be allowed to overlap.
							// TODO: make _ufopos a vector ;)
							ufoMap = _ufo->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(
																									999,999,
																									0,
																									false);
							if (addCraft(
										ufoMap,
										command,
										_ufoPos) == true)
							{
								for (
										x = _ufoPos.x;
										x < _ufoPos.x + _ufoPos.w;
										++x)
								{
									for (
											y = _ufoPos.y;
											y < _ufoPos.y + _ufoPos.h;
											++y)
									{
										if (_blocks[x][y])
											loadMAP(
												_blocks[x][y],
												x * 10,
												y * 10,
												_terrain,
												0);
									}
								}

								success = true;
							}
						}
					break;

					case MSC_DIGTUNNEL:
						drillModules(
									command->getTunnelData(),
									command->getRects(),
									command->getDirection());
						success = true; // this command is fail-proof
					break;

					case MSC_FILLAREA:
						block = command->getNextBlock(_terrain);
						while (block != NULL)
						{
							if (selectPosition(
											command->getRects(),
											x,y,
											block->getSizeX(),
											block->getSizeY()) == true)
							{
								// fill area will succeed if even one block is added
								success = (addBlock(x,y, block) == true)
										|| success;
							}
							else
								break;

							block = command->getNextBlock(_terrain);
						}
					break;

					case MSC_CHECKBLOCK:
						for (std::vector<SDL_Rect*>::const_iterator
								k = command->getRects()->begin();
								k != command->getRects()->end()
									&& success == false;
								++k)
						{
							for (
									x = (*k)->x;
									x != (*k)->x + (*k)->w
										&& x != _mapsize_x / 10
										&& success == false;
									++x)
							{
								for (
										y = (*k)->y;
										y != (*k)->y + (*k)->h
											&& y != _mapsize_y / 10
											&& success == false;
										++y)
								{
									if (command->getGroups()->empty() == false)
									{
										for (std::vector<int>::const_iterator
												z = command->getGroups()->begin();
												z != command->getGroups()->end()
													&& success == false;
												++z)
										{
											success = (_blocks[x][y]->isInGroup(*z) == true);
										}
									}
									else if (command->getBlocks()->empty() == false)
									{
										for (std::vector<int>::const_iterator
												z = command->getBlocks()->begin();
												z != command->getBlocks()->end()
													&& success == false;
												++z)
										{
											if (*z < static_cast<int>(_terrain->getMapBlocks()->size()))
												success = (_blocks[x][y] == _terrain->getMapBlocks()->at(*z));
										}
									}
									else // wildcard, don't care what block it is, just wanna know if there's a block here
										success = (_blocks[x][y] != NULL);
								}
							}
						}
					break;

					case MSC_REMOVE:
						success = removeBlocks(command);
					break;

					case MSC_RESIZE:
//						if (_battleSave->getMissionType() == "STR_BASE_DEFENSE")
						if (_battleSave->getTacticalType() == TCT_BASEDEFENSE)
						{
							throw Exception("Map Generator encountered an error: Base defense map cannot be resized.");
						}

						if (_blocksToDo < (_mapsize_x / 10) * (_mapsize_y / 10))
						{
							throw Exception("Map Generator encountered an error: The map cannot be resized after adding blocks.");
						}

						if (command->getSizeX() > 0
							&& command->getSizeX() != _mapsize_x / 10)
						{
							_mapsize_x = command->getSizeX() * 10;
						}

						if (command->getSizeY() > 0
							&& command->getSizeY() != _mapsize_y / 10)
						{
							_mapsize_y = command->getSizeY() * 10;
						}

						if (command->getSizeZ() > 0
							&& command->getSizeZ() != _mapsize_z)
						{
							_mapsize_z = command->getSizeZ();
						}

						init();
				}
			}
		}
	}

	if (_blocksToDo != 0)
	{
		throw Exception("Map failed to fully generate.");
	}

	loadNodes();

//	if (_missionType == "STR_ALIEN_BASE_ASSAULT")
	if (_battleSave->getTacticalType() == TCT_BASEASSAULT)
		placeXcomProperty();


	if (ufoMap != NULL)
	{
		for (std::vector<MapDataSet*>::const_iterator
				i = _ufo->getRules()->getBattlescapeTerrainData()->getMapDataSets()->begin();
				i != _ufo->getRules()->getBattlescapeTerrainData()->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getName()))
				_rules->getMCDPatch((*i)->getName())->modifyData(*i);

			_battleSave->getMapDataSets()->push_back(*i);
			++craftDataSetIDOffset;
		}

		// TODO: put ufo positions in a vector rather than a single rect, and iterate here;
		// will probably need to make ufomap a vector too.
		loadMAP(
			ufoMap,
			_ufoPos.x * 10,
			_ufoPos.y * 10,
			_ufo->getRules()->getBattlescapeTerrainData(),
			mapDataSetIDOffset);
		loadRMP(
			ufoMap,
			_ufoPos.x * 10,
			_ufoPos.y * 10,
			Node::SEG_UFO);

		for (int
				i = 0;
				i < ufoMap->getSizeX() / 10;
				++i)
		{
			for (int
					j = 0;
					j < ufoMap->getSizeY() / 10;
					++j)
			{
				_segments[_ufoPos.x + i][_ufoPos.y + j] = Node::SEG_UFO;
			}
		}
	}


	if (craftMap != NULL)
	{
		for (std::vector<MapDataSet*>::const_iterator
				i = _craft->getRules()->getBattlescapeTerrainData()->getMapDataSets()->begin();
				i != _craft->getRules()->getBattlescapeTerrainData()->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getName()))
				_rules->getMCDPatch((*i)->getName())->modifyData(*i);

			_battleSave->getMapDataSets()->push_back(*i);
		}

		loadMAP(
			craftMap,
			_craftPos.x * 10,
			_craftPos.y * 10,
			_craft->getRules()->getBattlescapeTerrainData(),
			mapDataSetIDOffset + craftDataSetIDOffset,
			false, // was true
			true);
		loadRMP(
			craftMap,
			_craftPos.x * 10,
			_craftPos.y * 10,
			Node::SEG_CRAFT);

		for (int
				i = 0;
				i < craftMap->getSizeX() / 10;
				++i)
		{
			for (int
					j = 0;
					j < craftMap->getSizeY() / 10;
					++j)
			{
				_segments[_craftPos.x + i]
						 [_craftPos.y + j] = Node::SEG_CRAFT;
			}
		}

/*kL
		for (int
				i = _craftPos.x * 10 - 1;
				i <= _craftPos.x * 10 + craftMap->getSizeX();
				++i)
		{
			for (int
					j = _craftPos.y * 10 - 1;
					j <= _craftPos.y * 10 + craftMap->getSizeY();
					++j)
			{
				for (int
						k = _mapsize_z - 1;
						k >= _craftZ;
						--k)
				{
					if (_battleSave->getTile(Position(i, j, k)))
						_battleSave->getTile(Position(i, j, k))->setDiscovered(true, 2);
				}
			}
		} */
	}

	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		for (int
				j = 0;
				j != 4;
				++j)
		{
			if (   _battleSave->getTiles()[i]->getMapData(j) != NULL
				&& _battleSave->getTiles()[i]->getMapData(j)->getSpecialType() == MUST_DESTROY)
			{
				_battleSave->addToObjectiveCount();
			}
		}
	}

	delete _testBlock;

	attachNodeLinks();
}

/**
 * Generates a map based on the base's layout.
 * @note This doesn't drill or fill with dirt, the script must do that.
 */
void BattlescapeGenerator::generateBaseMap() // private.
{
	// add modules based on the base's layout
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			const int
				xLimit = (*i)->getX() + (*i)->getRules()->getSize() - 1,
				yLimit = (*i)->getY() + (*i)->getRules()->getSize() - 1;
			int num = 0;

			for (int
					y = (*i)->getY();
					y <= yLimit;
					++y)
			{
				for (int
						x = (*i)->getX();
						x <= xLimit;
						++x)
				{
					// lots of crazy stuff here, which is for the hangars (or other large base facilities one may create)
					// TODO: clean this mess up, make the mapNames a vector in the base module defs
					// also figure out how to do the terrain sets on a per-block basis.
					const std::string mapname = (*i)->getRules()->getMapName();
					std::ostringstream newname;
					newname << mapname.substr(
											0,
											mapname.size() - 2); // strip off last 2 digits

					int mapnum = std::atoi(mapname.substr(
														mapname.size() - 2,
														2).c_str()); // get number
					mapnum += num;
					if (mapnum < 10)
						newname << 0;
					newname << mapnum;

					addBlock(
						x,
						y,
						_terrain->getMapBlock(newname.str()));

					_drillMap[x][y] = MD_NONE;
					++num;

					// general stores - there is where the items are put
					if ((*i)->getRules()->getStorage() > 0) // <- hmm Raises questions about buying and transfering Craft ....
					{
						int groundLevel;
						for (
								groundLevel = _mapsize_z - 1;
								groundLevel > -1;
								--groundLevel)
						{
							if (_battleSave->getTile(Position(
															x * 10,
															y * 10,
															groundLevel))->hasNoFloor(NULL) == false)
							{
								break;
							}
						}
						if (groundLevel < 0)
							groundLevel = 0; // safety.

						for (int
								k = x * 10;
								k != (x + 1) * 10;
								++k)
						{
							for (int
									l = y * 10;
									l != (y + 1) * 10;
									++l)
							{
								// use only every other tile, giving a "checkerboard" pattern
								if ((k + l) % 2 == 0)
								{
									Tile
										* const tile = _battleSave->getTile(Position(
																				k,
																				l,
																				groundLevel)),
										* const tileEast = _battleSave->getTile(Position(
																				k + 1,
																				l,
																				groundLevel)),
										* const tileSouth = _battleSave->getTile(Position(
																				k,
																				l + 1,
																				groundLevel));

									if (tile != NULL
										&& tile->getMapData(O_FLOOR) != NULL
										&& tile->getMapData(O_OBJECT) == NULL
										&& tileEast != NULL
										&& tileEast->getMapData(O_WESTWALL) == NULL
										&& tileSouth != NULL
										&& tileSouth->getMapData(O_NORTHWALL) == NULL)
									{
										_battleSave->getStorageSpace().push_back(Position(k,l, groundLevel));
									}
								}
							}
						}

						// put the inventory tile on the lowest floor, jic.
						if (_tileEquipt == NULL)
							_tileEquipt = _battleSave->getTile(Position(
																	x * 10 + 5,
																	y * 10 + 5,
																	std::max(
																		0,
																		groundLevel - 1)));
					}
				}
			}

			for (int
					x = (*i)->getX();
					x <= xLimit;
					++x)
			{
				_drillMap[x][yLimit] = MD_VERTICAL;
			}

			for (int
					y = (*i)->getY();
					y <= yLimit;
					++y)
			{
				_drillMap[xLimit][y] = MD_HORIZONTAL;
			}

			_drillMap[xLimit][yLimit] = MD_BOTH;
		}
	}

	_battleSave->calculateModuleMap();
}

/**
 * Finds Alien Base start modules and iterates possible positions for Xcom's
 * starting equipment.
 */
void BattlescapeGenerator::placeXcomProperty() // private.
{
	for (size_t
			y = 0;
			y != static_cast<size_t>(_mapsize_y / 10);
			++y)
	{
		for (size_t
				x = 0;
				x != static_cast<size_t>(_mapsize_x / 10);
				++x)
		{
			if (_blocks[x][y]->isInGroup(MBT_START))
			{
				int
					xPos = x * 10 + 5,
					yPos = y * 10 + 5,
					zPos = 1;
				_battleSave->getStorageSpace().push_back(Position(
																xPos,
																yPos,
																zPos));
			}
		}
	}
}

/**
 * Clears a module from the map.
 * @param x		- the x offset
 * @param y		- the y offset
 * @param sizeX	- how far along the x axis to clear
 * @param sizeY	- how far along the y axis to clear
 */
void BattlescapeGenerator::clearModule( // private.
		int x,
		int y,
		int sizeX,
		int sizeY)
{
	for (int
			z = 0;
			z != _mapsize_z;
			++z)
	{
		for (int
				dx = x;
				dx != x + sizeX;
				++dx)
		{
			for (int
					dy = y;
					dy != y + sizeY;
					++dy)
			{
				Tile* const tile = _battleSave->getTile(Position(dx,dy,z));
				for (int
						i = 0;
						i != 4;
						++i)
				{
					tile->setMapData(NULL,-1,-1, i);
				}
			}
		}
	}
}

/**
 * Loads all the nodes from the map modules.
 */
void BattlescapeGenerator::loadNodes() // private.
{
	int segment = 0;

	for (size_t
			y = 0;
			y != static_cast<size_t>(_mapsize_y / 10);
			++y)
	{
		for (size_t
				x = 0;
				x != static_cast<size_t>(_mapsize_x / 10);
				++x)
		{
			_segments[x][y] = segment;

			if (_blocks[x][y] != NULL
				&& _blocks[x][y] != _testBlock)
			{
				if (!(
					_blocks[x][y]->isInGroup(MBT_LANDPAD)
						&& _landingzone[x][y]))
				{
					loadRMP(
						_blocks[x][y],
						static_cast<int>(x) * 10,
						static_cast<int>(y) * 10,
						segment++);
				}
			}
		}
	}
}

/**
 * Attaches all the nodes together in an intimate web of C++.
 */
void BattlescapeGenerator::attachNodeLinks() // private.
{
	for (std::vector<Node*>::const_iterator
			i = _battleSave->getNodes()->begin();
			i != _battleSave->getNodes()->end();
			++i)
	{
		const size_t
			segX = (*i)->getPosition().x / 10,
			segY = (*i)->getPosition().y / 10;
		const int
			borDirs[4]			= {-2,-3,-4,-5},
			borDirs_invert[4]	= {-4,-5,-2,-3};
		int borSegs[4];

		if (static_cast<int>(segX) == (_mapsize_x / 10) - 1)
			borSegs[0] = -1;
		else
			borSegs[0] = _segments[segX + 1]
								  [segY];

		if (static_cast<int>(segY) == (_mapsize_y / 10) - 1)
			borSegs[1] = -1;
		else
			borSegs[1] = _segments[segX]
								  [segY + 1];

		if (segX == 0)
			borSegs[2] = -1;
		else
			borSegs[2] = _segments[segX - 1]
								  [segY];

		if (segY == 0)
			borSegs[3] = -1;
		else
			borSegs[3] = _segments[segX]
								  [segY - 1];

		for (std::vector<int>::iterator
				j = (*i)->getNodeLinks()->begin();
				j != (*i)->getNodeLinks()->end();
				++j)
		{
			for (size_t
					dir = 0;
					dir != 4;
					++dir)
			{
				if (*j == borDirs[dir])
				{
					for (std::vector<Node*>::const_iterator
							k = _battleSave->getNodes()->begin();
							k != _battleSave->getNodes()->end();
							++k)
					{
						if ((*k)->getSegment() == borSegs[dir])
						{
							for (std::vector<int>::iterator
									l = (*k)->getNodeLinks()->begin();
									l != (*k)->getNodeLinks()->end();
									++l )
							{
								if (*l == borDirs_invert[dir])
								{
									*l = (*i)->getID();
									*j = (*k)->getID();
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Selects a position for a MapBlock.
 * @param rects	- pointer to a vector of pointers representing the positions to select from, none meaning the whole map
 * @param X		- reference the x position; for the block gets stored in this variable
 * @param Y		- reference the y position; for the block gets stored in this variable
 * @param sizeX	- the x size of the block to add
 * @param sizeY	- the y size of the block to add
 * @return, true if a valid position was selected
 */
bool BattlescapeGenerator::selectPosition( // private.
		const std::vector<SDL_Rect*>* rects,
		int& X,
		int& Y,
		int sizeX,
		int sizeY)
{
	std::vector<SDL_Rect*> available;

	SDL_Rect wholeMap;
	wholeMap.x =
	wholeMap.y = 0;
	wholeMap.w = static_cast<Uint16>(_mapsize_x / 10);
	wholeMap.h = static_cast<Uint16>(_mapsize_y / 10);

	sizeX = (sizeX / 10);
	sizeY = (sizeY / 10);

	if (rects->empty() == true)
		available.push_back(&wholeMap);
	else
		available = *rects;

	std::vector<std::pair<int, int> > valid;
	for (std::vector<SDL_Rect*>::const_iterator
			i = available.begin();
			i != available.end();
			++i)
	{
		if (   sizeX > static_cast<int>((*i)->w)
			|| sizeY > static_cast<int>((*i)->h))
		{
			continue;
		}

		for (int
				x = static_cast<int>((*i)->x);
				x + sizeX <= static_cast<int>((*i)->x) + static_cast<int>((*i)->w)
					&& x + sizeX <= static_cast<int>(wholeMap.w);
				++x)
		{
			for (int
					y = static_cast<int>((*i)->y);
					y + sizeY <= static_cast<int>((*i)->y) + static_cast<int>((*i)->h)
						&& y + sizeY <= static_cast<int>(wholeMap.h);
					++y)
			{
				if (std::find(
							valid.begin(),
							valid.end(),
							std::make_pair(x,y)) == valid.end())
				{
					bool add = true;

					for (int
							xCheck = x;
							xCheck != x + sizeX;
							++xCheck)
					{
						for (int
								yCheck = y;
								yCheck != y + sizeY;
								++yCheck)
						{
							if (_blocks[static_cast<size_t>(xCheck)]
									   [static_cast<size_t>(yCheck)] != NULL)
							{
								add = false;
							}
						}
					}

					if (add == true)
						valid.push_back(std::make_pair(x,y));
				}
			}
		}
	}

	if (valid.empty() == true)
		return false;

	std::pair<int, int> sel = valid.at(static_cast<size_t>(RNG::generate(
																	0,
																	static_cast<int>(valid.size()) - 1)));
	X = sel.first;
	Y = sel.second;

	return true;
}

/**
 * Adds a craft (or UFO) to the map, and tries to add a landing zone type block underneath it.
 * @param craftMap	- pointer to the MapBlock for the craft in question
 * @param command	- pointer to the script command to pull info from
 * @param craftPos	- reference the position of the craft and store it here
 * @return, true if the craft was placed
 */
bool BattlescapeGenerator::addCraft( // private.
		const MapBlock* const craftMap,
		MapScript* command,
		SDL_Rect& craftPos)
{
	craftPos.w = static_cast<Uint16>(craftMap->getSizeX());
	craftPos.h = static_cast<Uint16>(craftMap->getSizeY());

	int
		x,y;
	bool placed = selectPosition(
							command->getRects(),
							x,y,
							craftPos.w,
							craftPos.h);

	if (placed == true)
	{
		craftPos.x = static_cast<Sint16>(x);
		craftPos.y = static_cast<Sint16>(y);
		craftPos.w /= 10;
		craftPos.h /= 10;

		for (
				x = 0;
				x != craftPos.w;
				++x)
		{
			for (
					y = 0;
					y != craftPos.h;
					++y)
			{
				_landingzone[craftPos.x + x]
							[craftPos.y + y] = true;

				MapBlock* const block = command->getNextBlock(_terrain);
				if (block != NULL
					&& _blocks[craftPos.x + x]
							  [craftPos.y + y] == NULL)
				{
					_blocks[craftPos.x + x]
						   [craftPos.y + y] = block;

					--_blocksToDo;
				}
			}
		}
	}

	return placed;
}

/**
 * Draws a line along the map either horizontally, vertically or both.
 * @param direction	- the direction to draw the line
 * @param rects		- the positions to allow the line to be drawn in
 * @return, true if the blocks were added
 */
bool BattlescapeGenerator::addLine( // private.
		MapDirection direction,
		const std::vector<SDL_Rect*>* rects)
{
	if (direction == MD_BOTH)
	{
		if (addLine(MD_VERTICAL, rects) == true)
		{
			addLine(MD_HORIZONTAL, rects);
			return true;
		}

		return false;
	}

	int tries = 0,
		roadX = 0, // avoid VC++ linker warnings
		roadY = 0,
		* i,
		limit;
	MapBlockType
		comparator,
		typeToAdd;

	if (direction == MD_VERTICAL)
	{
		i = &roadY;
		comparator = MBT_EWROAD;
		typeToAdd = MBT_NSROAD;
		limit = _mapsize_y / 10;
	}
	else
	{
		i = &roadX;
		comparator = MBT_NSROAD;
		typeToAdd = MBT_EWROAD;
		limit = _mapsize_x / 10;
	}

	bool placed = false;
	while (placed == false)
	{
		placed = true;
		selectPosition(
					rects,
					roadX,
					roadY,
					10,10);

		for (
				*i = 0;
				*i != limit;
				*i += 1)
		{
			if (   _blocks[roadX][roadY] != NULL
				&& _blocks[roadX][roadY]->isInGroup(comparator) == false)
			{
				placed = false;
				break;
			}
		}

		if (tries++ > 20)
			return false;
	}

	*i = 0;
	while (*i != limit)
	{
		if (_blocks[roadX][roadY] == NULL)
			addBlock(
					roadX,
					roadY,
					_terrain->getRandomMapBlock(
											10,10,
											typeToAdd));
		else if (_blocks[roadX][roadY]->isInGroup(comparator) == true)
		{
			_blocks[roadX][roadY] = _terrain->getRandomMapBlock(
															10,10,
															MBT_CROSSROAD);
			clearModule(
					roadX * 10,
					roadY * 10,
					10,10);
			loadMAP(
				_blocks[roadX][roadY],
				roadX * 10,
				roadY * 10,
				_terrain,
				0);
		}

		*i += 1;
	}

	return true;
}

/**
 * Adds a single block to the map.
 * @param x		- the x position to add the block
 * @param y		- the y position to add the block
 * @param block	- the MapBlock to add
 * @return, true if the block was added
 */
bool BattlescapeGenerator::addBlock( // private.
		int x,
		int y,
		MapBlock* const block)
{
	const size_t
		xSize = (block->getSizeX() - 1) / 10,
		ySize = (block->getSizeY() - 1) / 10,
		xt = static_cast<size_t>(x),
		yt = static_cast<size_t>(y);

	for (size_t
			xd = 0;
			xd <= xSize;
			++xd)
	{
		for (size_t
				yd = 0;
				yd != ySize;
				++yd)
		{
			if (_blocks[xt + xd]
					   [yt + yd])
			{
				return false;
			}
		}
	}

	for (size_t
			xd = 0;
			xd <= xSize;
			++xd)
	{
		for (size_t
				yd = 0;
				yd <= ySize;
				++yd)
		{
			_blocks[xt + xd]
				   [yt + yd] = _testBlock;

			--_blocksToDo;
		}
	}

	// mark the south edge of the block for drilling
	for (size_t
			xd = 0;
			xd <= xSize;
			++xd)
	{
		_drillMap[xt + xd]
				 [yt + ySize] = MD_VERTICAL;
	}

	// then the east edge
	for (size_t
			yd = 0;
			yd <= ySize;
			++yd)
	{
		_drillMap[xt + xSize]
				 [yt + yd] = MD_HORIZONTAL;
	}

	// then the far corner gets marked for both
	// this also marks 1x1 modules
	_drillMap[xt + xSize]
			 [yt + ySize] = MD_BOTH;
	_blocks[xt][yt] = block;

	const bool visible = (_battleSave->getTacticalType() == TCT_BASEDEFENSE);
//	const bool visible = (_battleSave->getMissionType() == "STR_BASE_DEFENSE");	// yes, i'm hard coding these.
// & I've unhardcoded them, big whop; or perhaps that was just the abortion-in-action. /cheers (aka, Hah! fool)
	loadMAP(
		_blocks[xt][yt],
		x * 10,
		y * 10,
		_terrain,
		0,
		visible);

	return true;
}

/**
 * Drills a tunnel between existing map modules.
 * Note that this drills all modules currently on the map
 * so it should take place BEFORE the dirt is added in base defenses.
 * @param info	- pointer to the wall replacements and level to dig on
 * @param rects	- pointer to a vector of pointers defining the length/width of the tunnels themselves
 * @param dir	- the direction to drill
 */
void BattlescapeGenerator::drillModules( // private.
		TunnelData* info,
		const std::vector<SDL_Rect*>* rects,
		MapDirection dir)
{
	const MCDReplacement
		* const westMCD = info->getMCDReplacement("westWall"),
		* const northMCD = info->getMCDReplacement("northWall"),
		* const cornerMCD = info->getMCDReplacement("corner"),
		* const floorMCD = info->getMCDReplacement("floor");

	SDL_Rect rect;
	rect.x =
	rect.y = 3;
	rect.w =
	rect.h = 3;

	if (rects->empty() == false)
		rect = *rects->front();

	for (int
			i = 0;
			i != (_mapsize_x / 10);
			++i)
	{
		for (int
				j = 0;
				j != (_mapsize_y / 10);
				++j)
		{
			if (_blocks[i][j] == 0)
				continue;

			Tile* tile = NULL;
			MapData* data = NULL;

			if (dir != MD_VERTICAL) // drill east
			{
				if (i < (_mapsize_x / 10) - 1
					&& (_drillMap[i][j] == MD_HORIZONTAL
						|| _drillMap[i][j] == MD_BOTH)
					&& _blocks[i + 1][j] != NULL)
				{
					for (int
							k = rect.y;
							k != rect.y + rect.h;
							++k)
					{
						tile = _battleSave->getTile(Position(
														(i * 10) + 9,
														(j * 10) + k,
														info->level));
						if (tile != NULL)
						{
							tile->setMapData(0,-1,-1, O_WESTWALL);
							tile->setMapData(0,-1,-1, O_OBJECT);

							if (floorMCD != NULL)
							{
								data = _terrain->getMapDataSets()->at(floorMCD->dataSet)->getObjects()->at(floorMCD->entry);
								tile->setMapData(
											data,
											floorMCD->entry,
											floorMCD->dataSet,
											O_FLOOR);
							}

							tile = _battleSave->getTile(Position(
															(i + 1) * 10,
															(j * 10) + k,
															info->level));
							tile->setMapData(NULL,-1,-1, O_WESTWALL);

							const MapData* const obj = tile->getMapData(O_OBJECT);
							if (obj != NULL
								&& obj->getTUCostObject(MT_WALK) == 0)
							{
								tile->setMapData(NULL,-1,-1, O_OBJECT);
							}
						}
					}

					if (northMCD != NULL)
					{
						data = _terrain->getMapDataSets()->at(northMCD->dataSet)->getObjects()->at(northMCD->entry);
						tile = _battleSave->getTile(Position(
														(i * 10) + 9,
														(j * 10) + rect.y,
														info->level));
						tile->setMapData(
									data,
									northMCD->entry,
									northMCD->dataSet,
									O_NORTHWALL);
						tile = _battleSave->getTile(Position(
														(i * 10) + 9,
														(j * 10) + rect.y + rect.h,
														info->level));
						tile->setMapData(
									data,
									northMCD->entry,
									northMCD->dataSet,
									O_NORTHWALL);
					}

					if (cornerMCD != NULL)
					{
						data = _terrain->getMapDataSets()->at(cornerMCD->dataSet)->getObjects()->at(cornerMCD->entry);
						tile = _battleSave->getTile(Position(
														(i + 1) * 10,
														(j * 10) + rect.y,
														info->level));

						if (tile->getMapData(O_NORTHWALL) == NULL)
							tile->setMapData(
										data,
										cornerMCD->entry,
										cornerMCD->dataSet,
										O_NORTHWALL);
					}
				}
			}

			if (dir != MD_HORIZONTAL) // drill south
			{
				if (j < (_mapsize_y / 10) - 1
					&& (_drillMap[i][j] == MD_VERTICAL
						|| _drillMap[i][j] == MD_BOTH)
					&& _blocks[i][j + 1] != NULL)
				{
					for (int
							k = rect.x;
							k != rect.x + rect.w;
							++k)
					{
						tile = _battleSave->getTile(Position(
														(i * 10) + k,
														(j * 10) + 9,
														info->level));
						if (tile != NULL)
						{
							tile->setMapData(NULL,-1,-1, O_NORTHWALL);
							tile->setMapData(NULL,-1,-1, O_OBJECT);

							if (floorMCD != NULL)
							{
								data = _terrain->getMapDataSets()->at(floorMCD->dataSet)->getObjects()->at(floorMCD->entry);
								tile->setMapData(
											data,
											floorMCD->entry,
											floorMCD->dataSet,
											O_FLOOR);
							}

							tile = _battleSave->getTile(Position(
															(i * 10) + k,
															(j + 1) * 10,
															info->level));
							tile->setMapData(NULL,-1,-1, O_NORTHWALL);

							const MapData* const obj = tile->getMapData(O_OBJECT);
							if (obj != NULL
								&& obj->getTUCostObject(MT_WALK) == 0)
							{
								tile->setMapData(NULL,-1,-1, O_OBJECT);
							}
						}
					}

					if (westMCD != NULL)
					{
						data = _terrain->getMapDataSets()->at(westMCD->dataSet)->getObjects()->at(westMCD->entry);
						tile = _battleSave->getTile(Position(
														(i * 10) + rect.x,
														(j * 10) + 9,
														info->level));
						tile->setMapData(
									data,
									westMCD->entry,
									westMCD->dataSet,
									O_WESTWALL);
						tile = _battleSave->getTile(Position(
														(i * 10) + rect.x + rect.w,
														(j * 10) + 9,
														info->level));
						tile->setMapData(
									data,
									westMCD->entry,
									westMCD->dataSet,
									O_WESTWALL);
					}

					if (cornerMCD != NULL)
					{
						data = _terrain->getMapDataSets()->at(cornerMCD->dataSet)->getObjects()->at(cornerMCD->entry);
						tile = _battleSave->getTile(Position(
														(i * 10) + rect.x,
														(j + 1) * 10,
														info->level));

						if (tile->getMapData(O_WESTWALL) == NULL)
							tile->setMapData(
										data,
										cornerMCD->entry,
										cornerMCD->dataSet,
										O_WESTWALL);
					}
				}
			}
		}
	}
}

/**
 * Removes all blocks within a given set of rects, as defined in the command.
 * @param command - contains all the info needed
 * @return, true if success
 * @feel clever & self-important (!)
 * @reality WoT..
 */
bool BattlescapeGenerator::removeBlocks(MapScript* command) // private.
{
	std::vector<std::pair<int, int> > deleted;
	bool success = false;

	for (std::vector<SDL_Rect*>::const_iterator
			k = command->getRects()->begin();
			k != command->getRects()->end();
			++k)
	{
		for (int
				x = (*k)->x;
				x != (*k)->x + (*k)->w
					&& x != _mapsize_x / 10;
				++x)
		{
			for (int
					y = (*k)->y;
					y != (*k)->y + (*k)->h
						&& y != _mapsize_y / 10;
					++y)
			{
				if (_blocks[x][y] != NULL
					&& _blocks[x][y] != _testBlock)
				{
					std::pair<int, int> pos(x, y);

					if (command->getGroups()->empty() == false)
					{
						for (std::vector<int>::const_iterator
								z = command->getGroups()->begin();
								z != command->getGroups()->end();
								++z)
						{
							if (_blocks[x][y]->isInGroup(*z) == true)
							{
								// the deleted vector should only contain unique entries
								if (std::find(
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
								{
									deleted.push_back(pos);
								}
							}
						}
					}
					else if (command->getBlocks()->empty() == false)
					{
						for (std::vector<int>::const_iterator
								z = command->getBlocks()->begin();
								z != command->getBlocks()->end();
								++z)
						{
							if (*z < static_cast<int>(_terrain->getMapBlocks()->size()))
							{
								// the deleted vector should only contain unique entries
								if (std::find(
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
								{
									deleted.push_back(pos);
								}
							}
						}
					}
					else
					{
						// the deleted vector should only contain unique entries
						if (std::find(
									deleted.begin(),
									deleted.end(),
									pos) == deleted.end())
						{
							deleted.push_back(pos);
						}
					}
				}
			}
		}
	}

	for (std::vector<std::pair<int, int> >::const_iterator
			z = deleted.begin();
			z != deleted.end();
			++z)
	{
		const int
			x = (*z).first,
			y = (*z).second;

		clearModule(
				x * 10,
				y * 10,
				_blocks[x][y]->getSizeX(),
				_blocks[x][y]->getSizeY());

		const int
			delx = (_blocks[x][y]->getSizeX() / 10),
			dely = (_blocks[x][y]->getSizeY() / 10);

		for (int
				dx = x;
				dx != x + delx;
				++dx)
		{
			for (int
					dy = y;
					dy != y + dely;
					++dy)
			{
				_blocks[dx][dy] = NULL;
				++_blocksToDo;
			}
		}

		// this command succeeds if even one block is removed.
		success = true;
	}

	return success;
}

}
