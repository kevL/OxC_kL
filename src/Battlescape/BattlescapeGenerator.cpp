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

#include "BattlescapeGenerator.h"

#include <fstream>
#include <sstream>

#include <assert.h>

#include "AlienBAIState.h"
#include "CivilianBAIState.h"
#include "Inventory.h"
#include "Pathfinding.h"
#include "TileEngine.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

//kL #include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/AlienRace.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/MapBlock.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/MCDPatch.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"
//#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/TerrorSite.h"
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
		_battleSave(game->getSavedGame()->getSavedBattle()),
		_rules(game->getRuleset()),
		_res(game->getResourcePack()),
		_craft(NULL),
		_ufo(NULL),
		_base(NULL),
		_terror(NULL),
		_alienBase(NULL),
		_terrain(NULL),
		_mapsize_x(0),
		_mapsize_y(0),
		_mapsize_z(0),
		_worldTerrain(NULL),
		_worldTexture(0),
		_worldShade(0),
		_unitSequence(0),
		_tileCraft(NULL),
		_alienItemLevel(0),
		_generateFuel(true),
		_craftDeployed(false),
		_craftZ(0),
		_baseEquipScreen(false),
		_battleOrder(0),
		_isCity(false)
{
	//Log(LOG_INFO) << "Run BattlescapeGenerator";
	_allowAutoLoadout = !Options::disableAutoEquip;
}

/**
 * Deletes the BattlescapeGenerator.
 */
BattlescapeGenerator::~BattlescapeGenerator()
{
	//Log(LOG_INFO) << "Delete BattlescapeGenerator";
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
 * Sets the terror site involved in the battle.
 * @param terror - pointer to TerrorSite
 */
void BattlescapeGenerator::setTerrorSite(TerrorSite* terror)
{
	_terror = terror;
	_terror->setInBattlescape(true);
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
 */
void BattlescapeGenerator::setIsCity(const bool isCity)
{
	_isCity = isCity;
}

/**
 * Sets the terrain of where a ufo crashed/landed as per ConfirmLandingState.
 * @param texture - pointer to RuleTerrain
 */
void BattlescapeGenerator::setWorldTerrain(RuleTerrain* terrain)
{
	_worldTerrain = terrain;
}

/**
 * Sets the world texture where a ufo crashed or landed.
 * This is used to determine the terrain if worldTerrain is ""/ NULL.
 * @param texture - texture id of the polygon on the globe
 */
void BattlescapeGenerator::setWorldTexture(int texture)
{
	if (texture < 0)
		texture = 0;

	_worldTexture = texture;
}

/**
 * Sets the world shade where a ufo crashed or landed.
 * This is used to determine the battlescape light level.
 * @param shade - shade of the polygon on the globe
 */
void BattlescapeGenerator::setWorldShade(int shade)
{
	if (shade > 15)
		shade = 15;

	if (shade < 0)
		shade = 0;

	_worldShade = shade;
}

/**
 * Sets the alien race on the mission. This is used to determine the various alien types to spawn.
 * @param alienRace - reference the (main) alien race
 */
void BattlescapeGenerator::setAlienRace(const std::string& alienRace)
{
	//Log(LOG_INFO) << "gen, race = " << alienRace;
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
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((_battleSave->isAborted()
				&& !(*j)->isInExitArea(END_POINT))
			|| (*j)->getOriginalFaction() == FACTION_HOSTILE)
		{
			(*j)->instaKill();
		}

		if ((*j)->getTile())
			(*j)->getTile()->setUnit(NULL);

		(*j)->setTile(NULL);
		(*j)->setPosition(Position(-1,-1,-1), false);
	}

	while (_battleSave->getSide() != FACTION_PLAYER)
	{
		_battleSave->endBattleTurn();
	}

	_battleSave->resetTurnCounter();

	_mission = _battleSave->getMissionType();
	AlienDeployment* ruleDeploy = _rules->getDeployment(_mission);
	ruleDeploy->getDimensions(
							&_mapsize_x,
							&_mapsize_y,
							&_mapsize_z);
	size_t pick = RNG::generate(
							0,
							ruleDeploy->getTerrains().size() - 1);
	_terrain = _rules->getTerrain(ruleDeploy->getTerrains().at(pick));

	_worldShade = ruleDeploy->getShade();

	_battleSave->initMap(
				_mapsize_x,
				_mapsize_y,
				_mapsize_z);

	const std::vector<MapScript*>* script = _rules->getMapScript(_terrain->getScript());
	if (_rules->getMapScript(ruleDeploy->getScript()))
		script = _rules->getMapScript(ruleDeploy->getScript());
	else if (ruleDeploy->getScript() != "")
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
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((*j)->getOriginalFaction() == FACTION_PLAYER)
		{
			if ((*j)->isOut() == false)
			{
				(*j)->convertToFaction(FACTION_PLAYER);
				(*j)->setTurnsExposed(255);
				(*j)->getVisibleTiles()->clear();

				if (selectedFirstSoldier == false
					&& (*j)->getGeoscapeSoldier())
				{
					_battleSave->setSelectedUnit(*j);
					selectedFirstSoldier = true;
				}

				Node* node = _battleSave->getSpawnNode(
												NR_XCOM,
												*j);
				if (node
					|| placeUnitNearFriend(*j))
				{
					if (node)
						_battleSave->setUnitPosition(
													*j,
													node->getPosition());

					if (_tileCraft == NULL)
					{
						_tileCraft = (*j)->getTile();
						_battleSave->setBattleInventory(_tileCraft);
					}

					_tileCraft->setUnit(*j);
					(*j)->setVisible(false);

					if ((*j)->getId() > highestSoldierID)
						highestSoldierID = (*j)->getId();

					(*j)->prepareUnitTurn();
				}
			}
		}
	}

	// remove all items not belonging to our soldiers from the map.
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
	}

	_unitSequence = _battleSave->getUnits()->back()->getId() + 1;
	size_t unitCount = _battleSave->getUnits()->size();

	deployAliens(
			_rules->getAlienRace(_alienRace),
			ruleDeploy);

	if (unitCount == _battleSave->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}

	deployCivilians(ruleDeploy->getCivilians());

	for (int
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
/*		if (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)
			&& (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT
				|| (_battleSave->getTiles()[i]->getPosition().z == 1
					&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->isGravLift()
					&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)))) */

		if (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT)
//				|| (_battleSave->getTiles()[i]->getPosition().z == 1
//					&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->isGravLift()
//					&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT))
		{
			_battleSave->getTiles()[i]->setDiscovered(true, 2);
		}
	}

	// set shade (alien bases are a little darker, sites depend on worldshade)
	_battleSave->setGlobalShade(_worldShade);

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
	_mission = _battleSave->getMissionType();
	AlienDeployment* ruleDeploy = NULL;
	if (_ufo != NULL)
		ruleDeploy = _rules->getDeployment(_ufo->getRules()->getType());
	else
		ruleDeploy = _rules->getDeployment(_mission);

	ruleDeploy->getDimensions(
						&_mapsize_x,
						&_mapsize_y,
						&_mapsize_z);

	_unitSequence = BattleUnit::MAX_SOLDIER_ID; // geoscape soldier IDs should stay below this number

	if (ruleDeploy->getTerrains().empty()) // UFO crashed/landed
	{
		//Log(LOG_INFO) << "Generator run() terrains NOT available: worldTexture = " << _worldTexture;
		//Log(LOG_INFO) << "Generator run() terrains NOT available: worldTerrain = " << _worldTerrain->getName();
		if (_worldTerrain == NULL) // kL
		{
			double lat = 0.0;
			if (_ufo)
				lat = _ufo->getLatitude();

			_terrain = getTerrain(
								_worldTexture,
								lat);
		}
		else
			_terrain = _worldTerrain; // kL
	}
	else // set-piece battle like Cydonia or Terror site or Base assault/defense
	{
		//Log(LOG_INFO) << "Generator run() terrains available = " << ruleDeploy->getTerrains().size();
		size_t pick = RNG::generate(
								0,
								ruleDeploy->getTerrains().size() - 1);
		_terrain = _rules->getTerrain(ruleDeploy->getTerrains().at(pick));
	}


	if (ruleDeploy->getShade() != -1)
		_worldShade = ruleDeploy->getShade();

	// creates the tile objects
	_battleSave->initMap(
						_mapsize_x,
						_mapsize_y,
						_mapsize_z);
	_battleSave->initUtilities(_res);


/*	// generate the map now and store it inside the tile objects
	generateMap(); // <-- BATTLE MAP GENERATION.

	_battleSave->setTerrain(_terrain->getName()); // sza_MusicRules

	// kL_begin: blow up PowerSources after aLiens & civies spawn, but before xCom spawn.
	// (i hope) nope-> CTD
//	deployAliens(
//			_rules->getAlienRace(_alienRace),
//			ruleDeploy);
//	deployCivilians(ruleDeploy->getCivilians());

//	fuelPowerSources();
//	if (_mission ==  "STR_UFO_CRASH_RECOVERY")
//		explodePowerSources(); // kL_end.

	if (_craft != NULL
		|| _base != NULL)
	{
		setTacticalSprites(); // kL
		deployXCOM(); // <-- XCOM DEPLOYMENT.
	}

	deployAliens( // <-- ALIEN DEPLOYMENT.
			_rules->getAlienRace(_alienRace),
			ruleDeploy); */


	const std::vector<MapScript*>* script = _rules->getMapScript(_terrain->getScript());
	if (_rules->getMapScript(ruleDeploy->getScript()))
		script = _rules->getMapScript(ruleDeploy->getScript());
	else if (ruleDeploy->getScript() != "")
	{
		throw Exception("Map generator encountered an error: " + ruleDeploy->getScript() + " script not found.");
	}

	if (script == NULL)
	{
		throw Exception("Map generator encountered an error: " + _terrain->getScript() + " script not found.");
	}

	generateMap(script);

	setTacticalSprites(); // kL
	deployXCOM(); // <-- XCOM DEPLOYMENT.

	const size_t unitCount = _battleSave->getUnits()->size();

	deployAliens( // <-- ALIEN DEPLOYMENT.
			_rules->getAlienRace(_alienRace),
			ruleDeploy);

	if (unitCount == _battleSave->getUnits()->size())
	{
		throw Exception("Map generator encountered an error: no alien units could be placed on the map.");
	}



	deployCivilians(ruleDeploy->getCivilians());

	if (_generateFuel == true)
		fuelPowerSources();

//	if (_mission == "STR_UFO_CRASH_RECOVERY")
	if (_ufo != NULL
		&& _ufo->getStatus() == Ufo::CRASHED)
	{
		explodePowerSources();
	}

/*	if (_mission == "STR_BASE_DEFENSE")
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

	if (_mission == "STR_ALIEN_BASE_ASSAULT"
		|| _mission == "STR_MARS_THE_FINAL_ASSAULT")
	{
		for (int
				i = 0;
				i < _battleSave->getMapSizeXYZ();
				++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)
				&& (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT
					|| (_battleSave->getTiles()[i]->getPosition().z == 1
						&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->isGravLift()
						&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT))))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	} */

	if (_craftDeployed == false)
	{
		for (int
				i = 0;
				i < _battleSave->getMapSizeXYZ();
				++i)
		{
			if (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR) != NULL
				&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT)
//					|| (_battleSave->getTiles()[i]->getPosition().z == _mapsize_z - 1
//						&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->isGravLift()
//						&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT))
			{
				_battleSave->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	}



	// set shade (alien bases are a little darker, sites depend on worldshade)
	_battleSave->setGlobalShade(_worldShade);

	_battleSave->getTileEngine()->calculateSunShading();
	_battleSave->getTileEngine()->calculateTerrainLighting();
	_battleSave->getTileEngine()->calculateUnitLighting();
	_battleSave->getTileEngine()->recalculateFOV();
}

/**
* Deploys all the X-COM units and equipment based on the Geoscape base / craft.
*/
void BattlescapeGenerator::deployXCOM()
{
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM()";
	RuleInventory* const ground = _rules->getInventory("STR_GROUND");

	if (_craft != NULL)
	{
		_base = _craft->getBase();

		if (_baseEquipScreen == false)
		{
			//Log(LOG_INFO) << ". craft VALID";

			// add all vehicles that are in the craft - a vehicle is actually an item,
			// which you will never see as it is converted to a unit;
			// however the item itself becomes the weapon it "holds".
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
		if (_game->getSavedGame()->getMonthsPassed() == -1)
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
				&& (*i)->getWoundRecovery() == 0
				&& ((*i)->getCraft() == NULL
					|| (*i)->getCraft()->getStatus() != "STR_OUT")))
		{
			//Log(LOG_INFO) << ". . addXCOMUnit " << (*i)->getId();
			BattleUnit* const unit = addXCOMUnit(new BattleUnit(
															*i,
															_battleSave->getDepth(),
															static_cast<int>(_game->getSavedGame()->getDifficulty()))); // kL_add: For VictoryPts value per death.

			if (unit != NULL)
			{
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
			_tileCraft->setUnit(*i);
			(*i)->setVisible(false);
		}
	}
	//Log(LOG_INFO) << ". setUnit(s) DONE";

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
					j < i->second;
					++j)
			{
				//Log(LOG_INFO) << ". . . j+";
				_tileCraft->addItem(
								new BattleItem(
											_rules->getItem(i->first),
											_battleSave->getCurrentItemId()),
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
		if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			//Log(LOG_INFO) << ". . addBaseItems";
			for (std::map<std::string, int>::iterator // add items from stores in base
					i = _base->getItems()->getContents()->begin();
					i != _base->getItems()->getContents()->end();
					)
			{
				// add only items in the battlescape that make sense
				// (when the item has a sprite, it's probably ok)
				const RuleItem* const rule = _rules->getItem(i->first);
				if (rule->getBigSprite() > -1
					&& rule->getBattleType() != BT_NONE
					&& rule->getBattleType() != BT_CORPSE
					&& rule->isFixed() == false
					&& _game->getSavedGame()->isResearched(rule->getRequirements()) == true)
				{
					for (int
							j = 0;
							j < i->second;
							++j)
					{
						_tileCraft->addItem(
										new BattleItem(
													_rules->getItem(i->first),
													_battleSave->getCurrentItemId()),
										ground);
					}

					const std::map<std::string, int>::iterator mark = i;
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
						k < j->second;
						++k)
				{
					_tileCraft->addItem(
									new BattleItem(
												_rules->getItem(j->first),
												_battleSave->getCurrentItemId()),
									ground);
				}
			}
		}
		//Log(LOG_INFO) << ". . addBaseCraftItems DONE";
	}
	//Log(LOG_INFO) << ". addItem(s) DONE";


	// kL_note: ALL ITEMS SEEM TO STAY ON THE GROUNDTILE, _tileCraft,
	// IN THAT INVENTORY(vector) UNTIL EVERYTHING IS EQUIPPED & LOADED. Then
	// the inventory-tile is cleaned up at the end of this function....
	//
	// equip soldiers based on equipment-layout
	//Log(LOG_INFO) << ". placeItemByLayout Start";
	for (std::vector<BattleItem*>::const_iterator
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
			++i)
	{
		// don't let the soldiers take extra ammo yet
		if ((*i)->getRules()->getBattleType() == BT_AMMO)
			continue;

		placeItemByLayout(*i);
	}

	// load weapons before loadouts take extra clips.
//kL	loadWeapons();

	for (std::vector<BattleItem*>::const_iterator
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". placeItemByLayout(*item)";
		// we only need to distribute extra ammo at this point.
		if ((*i)->getRules()->getBattleType() != BT_AMMO)
			continue;

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
					j = _tileCraft->getInventory()->begin();
					j != _tileCraft->getInventory()->end();
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
										|| _worldShade >= 9);
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
								j = _tileCraft->getInventory()->erase(j);
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
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
			++i)
	{
		//Log(LOG_INFO) << ". loading."; // from loadWeapons() ->
/*		if ((*i)->getRules()->isFixed() == false
			&& (*i)->getRules()->getCompatibleAmmo()->empty() == false
			&& (*i)->getAmmoItem() == NULL
			&& ((*i)->getRules()->getBattleType() == BT_FIREARM
				|| (*i)->getRules()->getBattleType() == BT_MELEE)) */
		if ((*i)->needsAmmo() == true) // or do it My way.
			loadGroundWeapon(*i);
	}
	//Log(LOG_INFO) << ". loading DONE";

	// clean up moved items
	for (std::vector<BattleItem*>::const_iterator
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
			)
	{
		if ((*i)->getSlot() == ground)
		{
			(*i)->setXCOMProperty();
			_battleSave->getItems()->push_back(*i);

			++i;
		}
		else
			i = _tileCraft->getInventory()->erase(i);
	}
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM() EXIT";
}

/**
 * Adds an XCom vehicle to the game.
 * Sets the correct turret depending on the ammo type and adds auxilliary weapons if any.
 * @param tank - pointer to Vehicle
 * @return, pointer to the spawned unit; NULL if unable to create and equip
 */
BattleUnit* BattlescapeGenerator::addXCOMVehicle(Vehicle* tank)
{
	const std::string vehicle = tank->getRules()->getType();
	Unit* const unitRule = _rules->getUnit(vehicle);

	BattleUnit* const tankUnit = addXCOMUnit(new BattleUnit( // add Vehicle as a unit.
														unitRule,
														FACTION_PLAYER,
														_unitSequence++,
														_rules->getArmor(unitRule->getArmor()),
														0,
														_battleSave->getDepth()));
	if (tankUnit != NULL)
	{
		tankUnit->setTurretType(tank->getRules()->getTurretType());

		BattleItem* const item = new BattleItem( // add Vehicle as an item and assign the unit as its owner.
											_rules->getItem(vehicle),
											_battleSave->getCurrentItemId());
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
											_battleSave->getCurrentItemId());
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
				RuleItem* const itemRule = _rules->getItem(*i);
				if (itemRule != NULL)
				{
					BattleItem* const item = new BattleItem(
													itemRule,
													_battleSave->getCurrentItemId());

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
	else
		return NULL;

	return tankUnit;
}

/**
 * Adds a soldier to the game and places him on a free spawnpoint.
 * Spawnpoints are either tiles in case of an XCom craft that landed
 * or they are mapnodes in case there's no craft.
 * @param unit - pointer to an xCom BattleUnit
 * @return, pointer to the spawned unit if successful, else NULL
 */
BattleUnit* BattlescapeGenerator::addXCOMUnit(BattleUnit* unit)
{
//	unit->setId(_unitCount++);
	if ((_craft == NULL
			|| _craftDeployed == false)
		&& _baseEquipScreen == false)
/*	if ((_craft == NULL
			|| _mission == "STR_ALIEN_BASE_ASSAULT"
			|| _mission == "STR_MARS_THE_FINAL_ASSAULT")
		&& _baseEquipScreen == false) */
	{
		Node* node = _battleSave->getSpawnNode(
									NR_XCOM,
									unit);
		if (node)
		{
			_battleSave->getUnits()->push_back(unit);

			_battleSave->setUnitPosition(
										unit,
										node->getPosition());
			unit->setDirection(RNG::generate(0, 7));
			unit->deriveRank();
//			_battleSave->getTileEngine()->calculateFOV(unit);

			_tileCraft = _battleSave->getTile(node->getPosition());
			_battleSave->setBattleInventory(_tileCraft);

			return unit;
		}
		else if (_mission != "STR_BASE_DEFENSE")
		{
			if (placeUnitNearFriend(unit))
			{
				_battleSave->getUnits()->push_back(unit);

				unit->setDirection(RNG::generate(0, 7));
				unit->deriveRank();
//				_battleSave->getTileEngine()->calculateFOV(unit);

				_tileCraft = _battleSave->getTile(unit->getPosition());
				_battleSave->setBattleInventory(_tileCraft);

				return unit;
			}
		}
	}
	else if (_craft != NULL // Transport craft deployments (Lightning & Avenger)
		&& _craft->getRules()->getDeployment().empty() == false
		&& _baseEquipScreen == false)
	{
		//Log(LOG_INFO) << "addXCOMUnit() - use Deployment rule";
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
					x < unit->getArmor()->getSize();
					++x)
			{
				for (int
						y = 0;
						y < unit->getArmor()->getSize();
						++y)
				{
					canPlace = canPlace
							&& canPlaceXCOMUnit(_battleSave->getTile(pos + Position(x, y, 0)));
				}
			}

			if (canPlace == true)
			{
				//Log(LOG_INFO) << ". canPlaceXCOMUnit()";
				if (_battleSave->setUnitPosition(
												unit,
												pos))
				{
					//Log(LOG_INFO) << ". setUnitPosition()";
					_battleSave->getUnits()->push_back(unit);

					unit->setDirection((*i)[3]);
					unit->deriveRank();

					return unit;
				}
			}
		}
	}
	else // mission w/ transport craft that does not have ruleset Deployments. Or craft/base Equip.
	{
		//Log(LOG_INFO) << "addXCOMUnit() - NO Deployment rule";
		int tankPos = 0;

		for (int // iterate through *all* tiles
				i = 0;
				i < _mapsize_x * _mapsize_y * _mapsize_z;
				++i)
		{
			if (canPlaceXCOMUnit(_battleSave->getTiles()[i]))
			{
				// kL_begin: BattlescapeGenerator, set tankPosition
				if (unit->getArmor()->getSize() > 1) // is a tank
				{
					if (_battleSave->getTiles()[i]->getPosition().x == _tileCraft->getPosition().x) // and the ground-inventory-tile is on this[i] tile's x-axis|
					{
						if (++tankPos == 3)
						{
							if (_battleSave->setUnitPosition( // set unit position SUCCESS
															unit,
															_battleSave->getTiles()[i]->getPosition()))
							{
								_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

								return unit;
							}
						}
					}
				}
				else if (_battleSave->setUnitPosition( // set unit position SUCCESS
													unit,
													_battleSave->getTiles()[i]->getPosition()))
				{
					_battleSave->getUnits()->push_back(unit); // add unit to vector of Units.

					unit->deriveRank();

					return unit;
				} // kL_end.
			}
		}
	}

	delete unit; // fallthrough, if above fails.

	return NULL;
}

/**
 * Checks if a soldier/tank can be placed on a given tile.
 * @param tile - the given tile
 * @return, true if a unit can be placed there
 */
bool BattlescapeGenerator::canPlaceXCOMUnit(Tile* tile)
{
	// to spawn an xcom soldier, there has to be a tile, with a floor,
	// with the starting point attribute and no objects in the way
	if (tile																		// is a tile
		&& tile->getMapData(MapData::O_FLOOR)										// has a floor
		&& tile->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT		// is a 'start point', ie. cargo tile
		&& tile->getMapData(MapData::O_OBJECT) == NULL								// no object content
		&& tile->getMapData(MapData::O_FLOOR)->getTUCost(MT_WALK) < 255				// is walkable.
		&& tile->getUnit() == NULL)													// kL, and no unit on Tile.
	{
		// kL_note: ground inventory goes where the first xCom unit spawns
		if (_tileCraft == NULL)
		{
			_tileCraft = tile;
			_battleSave->setBattleInventory(_tileCraft);
		}

		return true;
	}

	return false;
}

/**
 * Loads a weapon on the inventoryTile.
 * @param item - pointer to a BattleItem
 */
void BattlescapeGenerator::loadGroundWeapon(BattleItem* item)
{
	//Log(LOG_INFO) << "BattlescapeGenerator::loadGroundWeapon()";
	RuleInventory* ground = _rules->getInventory("STR_GROUND");
	RuleInventory* righthand = _rules->getInventory("STR_RIGHT_HAND");
	//Log(LOG_INFO) << ". ground & righthand RuleInventory are set";

	for (std::vector<BattleItem*>::iterator
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
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
 * Places an item on an XCom soldier based on equipment layout.
 * @param item - pointer to a BattleItem
 * @return, true if item is placed successfully
 */
bool BattlescapeGenerator::placeItemByLayout(BattleItem* item)
{
	RuleInventory* ground = _rules->getInventory("STR_GROUND");
	if (item->getSlot() == ground)
	{
		RuleInventory* righthand = _rules->getInventory("STR_RIGHT_HAND");
		bool loaded = false;

		// find the first soldier with a matching layout-slot
		for (std::vector<BattleUnit*>::iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			// skip the vehicles, we need only X-Com soldiers WITH equipment-layout
			if ((*i)->getGeoscapeSoldier() == NULL
//kL			|| (*i)->getArmor()->getSize() > 1
				|| (*i)->getGeoscapeSoldier()->getEquipmentLayout()->empty())
			{
				continue;
			}


			// find the first matching layout-slot which is not already occupied
			std::vector<EquipmentLayoutItem*>* layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();
			for (std::vector<EquipmentLayoutItem*>::iterator
					j = layoutItems->begin();
					j != layoutItems->end();
					++j)
			{
				if (item->getRules()->getType() != (*j)->getItemType()
					|| (*i)->getItem(
									(*j)->getSlot(),
									(*j)->getSlotX(),
									(*j)->getSlotY()))	// HOW can something other than the layout item be
														// in the item-slot?? (depends when Layout is saved)
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
					for (std::vector<BattleItem*>::iterator
							k = _tileCraft->getInventory()->begin();
							k != _tileCraft->getInventory()->end()
								&& loaded == false;
							++k)
					{
						if ((*k)->getRules()->getType() == (*j)->getAmmoItem()
							&& (*k)->getSlot() == ground		// why the redundancy?
																// WHAT OTHER _tileCraft IS THERE BUT THE GROUND TILE!!??!!!1
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
				if (loaded)
				{
					item->setXCOMProperty();

					item->moveToOwner(*i);

					item->setSlot(_rules->getInventory((*j)->getSlot()));
					item->setSlotX((*j)->getSlotX());
					item->setSlotY((*j)->getSlotY());

					if (Options::includePrimeStateInSavedLayout
						&& (item->getRules()->getBattleType() == BT_GRENADE
							|| item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
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
 * Adds an item to an XCom soldier (auto-equip ONLY). kL_note: I don't use this part.
 * Or an XCom tank, also adds items & terrorWeapons to aLiens, deployAliens()!
 * @param item				- pointer to the Item
 * @param unit				- pointer to the Unit
 * @param allowSecondClip	- true to allow the unit to take a second clip
 *							(only applies to xcom soldiers, aliens are allowed regardless)
 * @return, true if the item was placed
 */
bool BattlescapeGenerator::addItem(
		BattleItem* item,
		BattleUnit* unit)
//		bool allowSecondClip
{
	// kL_note: Old code that does what I want:
	RuleInventory
		* rightHand = _rules->getInventory("STR_RIGHT_HAND"),
		* leftHand = _rules->getInventory("STR_LEFT_HAND");
	BattleItem
		* rhWeapon = unit->getItem("STR_RIGHT_HAND"),
		* lhWeapon = unit->getItem("STR_LEFT_HAND");

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
			if (rhWeapon
				&& (rhWeapon->getRules()->isFixed()
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
			if (lhWeapon
				&& (lhWeapon->getRules()->isFixed()
					|| unit->getFaction() != FACTION_PLAYER)
				&& lhWeapon->getAmmoItem() == NULL
				&& lhWeapon->setAmmoItem(item) == 0)
			{
				item->setSlot(leftHand);

				placed = true;
				break;
			}

			// else put the clip in Belt or Backpack
			RuleItem* itemRule = item->getRules();

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
		case BT_PROXIMITYGRENADE:
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

	if (placed)
	{
		item->setXCOMProperty(unit->getFaction() == FACTION_PLAYER);
		_battleSave->getItems()->push_back(item);
	}

	// kL_note: If not placed, the items are deleted from wherever this function was called.
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
 * Deploys the aliens, according to the alien deployment rules.
 * @param race 			- pointer to the alien race
 * @param deployment 	- pointer to the deployment rules
 */
void BattlescapeGenerator::deployAliens(
		AlienRace* race,
		AlienDeployment* deployment)
{
	if (_battleSave->getDepth() > 0
		&& _alienRace.find("_UNDERWATER") == std::string::npos)
	{
		std::stringstream ss;
		ss << _alienRace << "_UNDERWATER";

		if (_rules->getAlienRace(ss.str()))
			race = _rules->getAlienRace(ss.str());
	}

	int month = _game->getSavedGame()->getMonthsPassed();
	if (month != -1)
	{
		int aiLevel_maximum = static_cast<int>(_rules->getAlienItemLevels().size()) - 1;
		if (month > aiLevel_maximum)
			month = aiLevel_maximum;
	}
	else
		month = _alienItemLevel;


	std::string alienName;
	bool outside;
	int
		itemLevel,
		qty;

	BattleItem* item = NULL;
	BattleUnit* unit = NULL;
	Unit* unitRule = NULL;

	for (std::vector<DeploymentData>::iterator
			data = deployment->getDeploymentData()->begin();
			data != deployment->getDeploymentData()->end();
			++data)
	{
		alienName = race->getMember((*data).alienRank);

		if (_game->getSavedGame()->getDifficulty() < DIFF_VETERAN)
			qty = (*data).lowQty + RNG::generate( // beginner/experienced
											0,
											(*data).dQty);
		else if (_game->getSavedGame()->getDifficulty() < DIFF_SUPERHUMAN)
			qty = (*data).lowQty + (((*data).highQty - (*data).lowQty) / 2) + RNG::generate( // veteran/genius
																						0,
																						(*data).dQty);
		else
			qty = (*data).highQty + RNG::generate( // super (and beyond)
												0,
												(*data).dQty);

		if (_base != NULL
			&& _base->getDefenseResult() > 0)
		{
			Log(LOG_INFO) << "BattlescapeGenerator::deployAliens()";
			Log(LOG_INFO) << ". defenseEffect = " << _base->getDefenseResult();
			Log(LOG_INFO) << ". qty_init = " << qty;
			qty = std::max(
						qty / 2,
						qty - (qty * _base->getDefenseResult() / 100));
			Log(LOG_INFO) << ". qty_postDefense = " << qty;

			_base->setDefenseResult(0);
		}

		for (int
				i = 0;
				i < qty;
				++i)
		{
			outside = false;
			if (_ufo)
				outside = RNG::percent((*data).percentageOutsideUfo);

			unitRule = _rules->getUnit(alienName);
			unit = addAlien(
						unitRule,
						(*data).alienRank,
						outside);

			if (unit != NULL)
			{
				RuleItem* itemRule = NULL;

				// Built in weapons: the unit has this weapon regardless of loadout or what have you.
				if (unitRule->getBuiltInWeapons().empty() == false)
				{
					for (std::vector<std::string>::const_iterator
							j = unitRule->getBuiltInWeapons().begin();
							j != unitRule->getBuiltInWeapons().end();
							++j)
					{
						itemRule = _rules->getItem(*j);
						if (itemRule != NULL)
						{
							BattleItem* item = new BattleItem(
															itemRule,
															_battleSave->getCurrentItemId());
							if (!addItem(
										item,
										unit))
							{
								delete item;
							}
						}
					}
				}

				// terrorist aliens' equipment is a special case - they are fitted
				// with a weapon which is the alien's name with suffix _WEAPON
				if (unitRule->isLivingWeapon())
				{
					std::string terroristWeapon = unitRule->getRace().substr(4);
					terroristWeapon += "_WEAPON";

					itemRule = _rules->getItem(terroristWeapon);
					if (itemRule != NULL)
					{
						item = new BattleItem( // terror aLiens add their weapons
											itemRule,
											_battleSave->getCurrentItemId());
						if (!addItem(
									item,
									unit))
						{
							delete item;
						}
						else
							unit->setTurretType(item->getRules()->getTurretType());
					}
				}
				else
				{
					itemLevel = _rules->getAlienItemLevels().at(month).at(RNG::generate(0, 9));

					for (std::vector<std::string>::iterator
							setItem = (*data).itemSets.at(itemLevel).items.begin();
							setItem != (*data).itemSets.at(itemLevel).items.end();
							++setItem)
					{
						itemRule = _rules->getItem(*setItem);
						if (itemRule != NULL)
						{
							item = new BattleItem( // aLiens add items
												itemRule,
												_battleSave->getCurrentItemId());
							if (!addItem(
										item,
										unit))
							{
								delete item;
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Adds an alien to the game and places him on a free spawnpoint.
 * @param rules		- pointer to the Unit rule which holds info about aLiens
 * @param alienRank	- rank of the alien, used for spawn point search
 * @param outside	- true if the alien should spawn outside the UFO
 * @return, pointer to the created BattleUnit
 */
BattleUnit* BattlescapeGenerator::addAlien(
		Unit* rules,
		int alienRank,
		bool outside)
{
	int
		difficulty = static_cast<int>(_game->getSavedGame()->getDifficulty()),
		month = _game->getSavedGame()->getMonthsPassed();

	BattleUnit* unit = new BattleUnit(
									rules,
									FACTION_HOSTILE,
									_unitSequence++,
									_rules->getArmor(rules->getArmor()),
									difficulty,
									_battleSave->getDepth(),
									month); // kL_add.

	// following data is the order in which certain alien ranks spawn on certain node ranks;
	// note that they all can fall back to rank 0 nodes - which is scout (outside ufo)

	Node* node = NULL;

	if (outside)
		node = _battleSave->getSpawnNode( // Civ-Scout spawnpoints <- 'outside'
									0,
									unit);

	if (node == NULL) // ie. if not spawning on a Civ-Scout node
	{
		for (int
				i = 0;
				i < 8
					&& node == NULL;
				++i)
		{
			node = _battleSave->getSpawnNode(
									Node::nodeRank[alienRank][i],
									unit);
		}
	}

	if ((node
			&& _battleSave->setUnitPosition(
										unit,
										node->getPosition()))
		|| placeUnitNearFriend(unit))
	{
		unit->setAIState(new AlienBAIState(
										_battleSave,
										unit,
										node));
		unit->setRankInt(alienRank);

		int dir = -1;

		Position craft = _battleSave->getUnits()->at(0)->getPosition();
		if (RNG::percent(difficulty * 20
			&& _battleSave->getTileEngine()->distance(
													node->getPosition(),
													craft)
												< 25))
		{
			dir = unit->directionTo(craft);
		}
		else
			dir = _battleSave->getTileEngine()->faceWindow(node->getPosition());

		if (dir == -1)
			dir = RNG::generate(0, 7);
		unit->setDirection(dir);

//		if (difficulty == 0) // kL_note: moved to BattleUnit::adjustStats()
//			unit->halveArmor();

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
 * Spawns civilians on a terror mission.
 * @param civilians - maximum number of civilians to spawn
 */
void BattlescapeGenerator::deployCivilians(int civilians)
{
	if (civilians > 0)
	{
		const int rand = RNG::generate(
									civilians / 2,
									civilians);
		if (rand > 0)
		{
			for (int
					i = 0;
					i < rand;
					++i)
			{
				size_t pick = RNG::generate(
										0,
										_terrain->getCivilianTypes().size() -1);
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
BattleUnit* BattlescapeGenerator::addCivilian(Unit* rules)
{
	BattleUnit* unit = new BattleUnit(
									rules,
									FACTION_NEUTRAL,
									_unitSequence++,
									_rules->getArmor(rules->getArmor()),
									0,
									_battleSave->getDepth());

	Node* node = _battleSave->getSpawnNode(
								0,
								unit);
	if ((node != NULL
			&& _battleSave->setUnitPosition(
									unit,
									node->getPosition()))
		|| placeUnitNearFriend(unit))
	{
		unit->setAIState(new CivilianBAIState(
										_battleSave,
										unit,
										node));

		unit->setDirection(RNG::generate(0, 7));

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
 * @param terrain		- pointer to RuleTerrain
 * @param dataSetOffset	-
 * @param discovered	- true if this MapBlock is discovered (eg. landingsite of the Skyranger)
 * @param craft			- true if xCom Craft has landed on the MAP
 * @return, height of the loaded mapblock (needed for spawnpoint calculation)
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 * @note Y-axis is in reverse order.
 */
int BattlescapeGenerator::loadMAP(
		MapBlock* mapblock,
		int offset_x,
		int offset_y,
		RuleTerrain* terrain,
		int dataSetOffset,
		bool discovered,
		bool craft)
{
	std::ostringstream filename;
	filename << "MAPS/" << mapblock->getName() << ".MAP";
	std::ifstream mapFile( // Load file
						CrossPlatform::getDataFile(filename.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	int
		size_x,
		size_y,
		size_z;
	char mapSize[3];
	mapFile.read(
			(char*)&mapSize,
			sizeof(mapSize));
	size_y = static_cast<int>(mapSize[0]); // note X-Y switch!
	size_x = static_cast<int>(mapSize[1]); // note X-Y switch!
	size_z = static_cast<int>(mapSize[2]);

	mapblock->setSizeZ(size_z);

	std::stringstream ss;
	if (size_z > _battleSave->getMapSizeZ())
	{
		ss <<"Height of map " + filename.str() + " too big for this mission, block is " << size_z << ", expected: " << _battleSave->getMapSizeZ();
		throw Exception(ss.str());
	}


	if (size_x != mapblock->getSizeX()
		|| size_y != mapblock->getSizeY())
	{
		ss <<"Map block is not of the size specified " + filename.str() + " is " << size_x << "x" << size_y << " , expected: " << mapblock->getSizeX() << "x" << mapblock->getSizeY();
		throw Exception(ss.str());
	}

	int
		x = offset_x,
		y = offset_y,
		z = size_z - 1;

	for (int
			i = _mapsize_z - 1;
			i > 0;
			--i)
	{
		// check if there is already a layer - if so, we have to move Z up
		MapData* floor = _battleSave->getTile(Position(x, y, i))->getMapData(MapData::O_FLOOR);
		if (floor != NULL)
		{
			z += i;

			if (craft == true)
				_craftZ = i;

			break;
		}
	}

	if (z > _battleSave->getMapSizeZ() - 1)
	{
		throw Exception("Something is wrong in your map definitions, craft/ufo map is too tall?");
	}

	unsigned int objectID;
	unsigned char value[4];
	while (mapFile.read(
					(char*)&value,
					sizeof(value)))
	{
		for (int
				part = 0;
				part < 4;
				++part)
		{
			objectID = static_cast<unsigned int>(value[part]);

			// kL_begin: Remove natural terrain that is inside Craft or Ufo. This mimics UFO:orig behavior.
			if (part != 0			// not if it's a floor since Craft/Ufo part will overwrite it anyway
				&& objectID == 0	// and only if no Craft/Ufo part would overwrite the part
				&& value[0] != 0)	// but only if there *is* a floor-part to the Craft/Ufo, so it would (have) be(en) inside the Craft/Ufo
			{
				_battleSave->getTile(Position(x, y, z))->setMapData(NULL,-1,-1, part);
			} // kL_end.

			// Then overwrite previous terrain with Craft or Ufo terrain.
			// nb. See sequence of map-loading in generateMap() above^ (1st terrain, 2nd Ufo, 3rd Craft)
			if (objectID > 0)
			{
				unsigned int dataID = objectID;
				int dataSetID = dataSetOffset;

				MapData* data = terrain->getMapData(
												&dataID,
												&dataSetID);
				_battleSave->getTile(Position(x, y, z))->setMapData(
																data,
																static_cast<int>(dataID),
																dataSetID,
																part);
			}

/*			// If the part is not a floor and is empty, remove it; this prevents growing grass in UFOs.
			if (part == 3
				&& objectID == 0)
			{
				_battleSave->getTile(Position(x, y, z))->setMapData(NULL,-1,-1, part);
			} */

			// kL_begin: Reveal only tiles inside the Craft.
			if (craft == true
				&& objectID != 0
				&& z != _craftZ)
			{
				_battleSave->getTile(Position(x, y, z))->setDiscovered(true, 2);
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
				_battleSave->getTile(Position(x, y, z2))->setDiscovered(true, 2);
			}
		} */

//		_battleSave->getTile(Position(x, y, z))->setDiscovered(discovered, 2);

		x++;

		if (x == size_x + offset_x)
		{
			x = offset_x;
			y++;
		}

		if (y == size_y + offset_y)
		{
			y = offset_y;
			z--;
		}
	}

	if (mapFile.eof() == false)
	{
		throw Exception("Invalid MAP file: " + filename.str());
	}

	mapFile.close();

	if (_generateFuel) // if one of the mapBlocks has an items array defined, don't deploy fuel algorithmically
		_generateFuel = (mapblock->getItems()->empty() == true);

	for (std::map<std::string, std::vector<Position> >::const_iterator
			i = mapblock->getItems()->begin();
			i != mapblock->getItems()->end();
			++i)
	{
		RuleItem* rule = _rules->getItem((*i).first);
		for (std::vector<Position>::const_iterator
				j = (*i).second.begin();
				j != (*i).second.end();
				++j)
		{
			BattleItem* item = new BattleItem(
											rule,
											_battleSave->getCurrentItemId());
			_battleSave->getItems()->push_back(item);
			_battleSave->getTile((*j) + Position(offset_x, offset_y, 0))->addItem(
																				item,
																				_rules->getInventory("STR_GROUND"));
		}
	}

	return size_z;
}

/**
 * Loads an XCom format RMP file into the spawnpoints of the battlegame.
 * @param mapblock	- pointer to MapBlock
 * @param xoff		- Mapblock offset in X direction
 * @param yoff		- Mapblock offset in Y direction
 * @param segment	- Mapblock segment
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
void BattlescapeGenerator::loadRMP(
		MapBlock* mapblock,
		int xoff,
		int yoff,
		int segment)
{
	char value[24];

	std::ostringstream filename;
	filename << "ROUTES/" << mapblock->getName() << ".RMP";

	std::ifstream mapFile( // Load file
					CrossPlatform::getDataFile(filename.str()).c_str(),
					std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	const int nodeOffset = static_cast<int>(_battleSave->getNodes()->size());
	int
		pos_x,
		pos_y,
		pos_z,

		type,
		nodeRank,
		flags,
		reserved,
		priority;

	while (mapFile.read(
					(char*)& value,
					sizeof(value)))
	{
		pos_x = static_cast<int>(value[1]);
		pos_y = static_cast<int>(value[0]);
		pos_z = static_cast<int>(value[2]);

		if (   pos_x < mapblock->getSizeX()
			&& pos_y < mapblock->getSizeY()
			&& pos_z < _mapsize_z)
		{
			const Position pos = Position(
										xoff + pos_x,
										yoff + pos_y,
										mapblock->getSizeZ() - pos_z - 1);

			type		= static_cast<int>(value[19]);
			nodeRank	= static_cast<int>(value[20]);
			flags		= static_cast<int>(value[21]);
			reserved	= static_cast<int>(value[22]);
			priority	= static_cast<int>(value[23]);

			Node* const node = new Node(
									_battleSave->getNodes()->size(),
									pos,
									segment,
									type,
									nodeRank,
									flags,
									reserved,
									priority);

			for (int
					j = 0;
					j < 5;
					++j)
			{
				int connectID = static_cast<int>(value[j * 3 + 4]);

				if (connectID < 251) // don't touch special values
					connectID += nodeOffset;
				else // 255/-1 = unused, 254/-2 = north, 253/-3 = east, 252/-4 = south, 251/-5 = west
					connectID -= 256;

				node->getNodeLinks()->push_back(connectID);
			}

			_battleSave->getNodes()->push_back(node);
		}
	}

	if (!mapFile.eof())
	{
		throw Exception("Invalid RMP file");
	}

	mapFile.close();
}

/**
 * Fill power sources with an alien fuel object.
 */
void BattlescapeGenerator::fuelPowerSources()
{
	for (int
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE)
		{
			BattleItem* const alienFuel = new BattleItem(
													_rules->getItem(_rules->getAlienFuel()),
													_battleSave->getCurrentItemId());

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
void BattlescapeGenerator::explodePowerSources()
{
	for (int
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT) != NULL
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE
			&& RNG::percent(80) == true)
		{
			const Position pos = Position(
									_battleSave->getTiles()[i]->getPosition().x * 16,
									_battleSave->getTiles()[i]->getPosition().y * 16,
									_battleSave->getTiles()[i]->getPosition().z * 24 + 12);

			double power = static_cast<double>(_ufo->getCrashPower());	// range: ~50+ to ~100-
			if (RNG::percent(static_cast<int>(power) / 2) == true)		// chance for full range Explosion (even if crash took low damage)
				power = RNG::generate(1., 100.);

			power *= RNG::generate(0.1, 2.);
			power += std::pow(power, 2) / 160.0;

			if (power > 0.5)
				_battleSave->getTileEngine()->explode(
													pos,
													static_cast<int>(power),
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
									tile->getPosition().z * 24);
		_battleSave->getTileEngine()->explode(
											pos,
											tile->getExplosive(),
											DT_HE,
											tile->getExplosive() / 10);
		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	}
}

/**
 * Places a unit near a friendly unit.
 * @param unit - pointer to the BattleUnit in question
 * @return, true if the unit was successfully placed
 */
bool BattlescapeGenerator::placeUnitNearFriend(BattleUnit* unit)
{
	if (unit == NULL
		|| _battleSave->getUnits()->empty() == true)
	{
		return false;
	}

	Position posEntry = Position(-1,-1,-1);

	int tries = 100;
	while (posEntry == Position(-1,-1,-1)
		&& tries > 0)
	{
		const BattleUnit* const bu = _battleSave->getUnits()->at(RNG::generate(
																			0,
																			static_cast<int>(_battleSave->getUnits()->size()) - 1));
		if (bu->getFaction() == unit->getFaction()
			&& bu->getPosition() != Position(-1,-1,-1)
			&& bu->getArmor()->getSize() == 1)
		{
			posEntry = bu->getPosition();
		}

		tries--;
	}

	if (tries > 0
		&& _battleSave->placeUnitNearPosition(
											unit,
											posEntry) == true)
	{
		return true;
	}

	return false;
}

/**
 * Gets battlescape terrain using globe texture and latitude.
 * @param tex - Globe texture
 * @param lat - latitude
 * @return, pointer to RuleTerrain
 */
RuleTerrain* BattlescapeGenerator::getTerrain(
		int tex,
		double lat)
{
	RuleTerrain* terrain = NULL;

	const std::vector<std::string>& terrains = _rules->getTerrainList();
	for (std::vector<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end();
			++i)
	{
		terrain = _rules->getTerrain(*i);

		for (std::vector<int>::const_iterator
				j = terrain->getTextures()->begin();
				j != terrain->getTextures()->end();
				++j)
		{
			if (*j == tex
				&& (terrain->getHemisphere() == 0
					|| (terrain->getHemisphere() < 0
						&& lat < 0.0)
					|| (terrain->getHemisphere() > 0
						&& lat >= 0.0)))
			{
				return terrain;
			}
		}
	}

	assert(0 && "No matching terrain for globe texture");

	return NULL;
}

/**
 * Sets xCom soldiers' combat clothing style - spritesheets & paperdolls.
 * Uses EqualTerms v1 graphics to replace stock resources. Affects soldiers
 * wearing pyjamas (STR_ARMOR_NONE_UC) only.
 * This is done by switching in/out equivalent Armors.
 */
void BattlescapeGenerator::setTacticalSprites()
{
// base defense, craft NULL "STR_BASE_DEFENSE"
// ufo, base NULL "STR_UFO_CRASH_RECOVERY" "STR_UFO_GROUND_ASSAULT" "STR_TERROR_MISSION" "STR_ALIEN_BASE_ASSAULT"
// cydonia "STR_MARS_CYDONIA_LANDING" "STR_MARS_THE_FINAL_ASSAULT"

	if ((_craft == NULL // both Craft & Base are NULL for the 2nd of a 2-part mission.
			&& _base == NULL)
		|| _mission == "STR_BASE_DEFENSE")
	{
		return;
	}


	std::string strArmor = "STR_ARMOR_NONE_UC";

	if (_isCity == true)
		strArmor = "STR_STREET_URBAN_UC";
	else if (_mission == "STR_MARS_CYDONIA_LANDING"
		|| _mission == "STR_MARS_THE_FINAL_ASSAULT")
	{
		strArmor = "STR_STREET_ARCTIC_UC";
	}
	else if (_mission == "STR_TERROR_MISSION"
		|| _mission == "STR_ALIEN_BASE_ASSAULT")
	{
		strArmor = "STR_STREET_URBAN_UC";
	}
	else
	{
		if ((-1 < _worldTexture && _worldTexture < 7)
			|| (9 < _worldTexture && _worldTexture < 12))
		{
			strArmor = "STR_STREET_JUNGLE_UC";
		}
		else if (6 < _worldTexture && _worldTexture < 10
			|| _worldTexture == 12)
		{
			strArmor = "STR_STREET_ARCTIC_UC";
		}
	}


	Armor* const armorRule = _rules->getArmor(strArmor);

	Base* base = _base; // just don't muck w/ _base here ... heh.
	if (_craft != NULL)
		base = _craft->getBase();

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

/**
 * Creates a mini-battle-save for managing inventory from the Geoscape's CraftEquip or BaseEquip screen.
 * Kids, don't try this at home! yer tellin' me.
 * @param craft	- pointer to Craft to handle
 * @param base	- pointer to Base to handle (default NULL)
 */
void BattlescapeGenerator::runInventory(
		Craft* craft,
		Base* base)
{
	//Log(LOG_INFO) << "BattlescapeGenerator::runInventory()";
	_baseEquipScreen = true;

	int soldiers = 0;
	if (craft != NULL)
		soldiers = craft->getNumSoldiers();
	else
		soldiers = base->getAvailableSoldiers(true);

	// we need to fake a map for soldier placement
	_mapsize_x = soldiers;
	_mapsize_y = 1;
	_mapsize_z = 1;

	_battleSave->initMap(
				_mapsize_x,
				_mapsize_y,
				_mapsize_z);

	MapDataSet* const dataSet = new MapDataSet(
									"dummy",
									_game); // kL_add
	MapData* const data = new MapData(dataSet);

	for (int
			i = 0;
			i < soldiers;
			++i)
	{
		Tile* const tile = _battleSave->getTiles()[i];
		tile->setMapData(
					data,
					0,
					0,
					MapData::O_FLOOR);
		tile->getMapData(MapData::O_FLOOR)->setSpecialType(
														START_POINT,
														0);
		tile->getMapData(MapData::O_FLOOR)->setTUWalk(0);
		tile->getMapData(MapData::O_FLOOR)->setFlags(
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
	{
		//Log(LOG_INFO) << ". setCraft()";
		setCraft(craft); // generate the battleitems for inventory
	}
	else
		setBase(base);

	//Log(LOG_INFO) << ". setCraft() DONE, deployXCOM()";
	deployXCOM();
	//Log(LOG_INFO) << ". deployXCOM() DONE";

	// ok, now remove any vehicles that may have somehow ended up in our battlescape
/*	for (std::vector<BattleUnit*>::iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			)
	{
		if ((*i)->getGeoscapeSoldier())
		{
			if (_battleSave->getSelectedUnit() == 0)
				_battleSave->setSelectedUnit(*i);

			++i;
		}
		else
		{
			if (_battleSave->getSelectedUnit() == *i)
				_battleSave->setSelectedUnit(0);

			delete *i;
			i = _battleSave->getUnits()->erase(i);
		}
	} */ // kL, they took this out.

	delete data;
	delete dataSet;
	//Log(LOG_INFO) << "BattlescapeGenerator::runInventory() EXIT";
}

/**
 * Loads all XCom weaponry before anything else is distributed.
 */
/* void BattlescapeGenerator::loadWeapons()
{
	// let's try to load this weapon, whether we equip it or not.
	for (std::vector<BattleItem*>::iterator
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
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
					j = _tileCraft->getInventory()->begin();
					j != _tileCraft->getInventory()->end()
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
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
			)
	{
		if ((*i)->getSlot() != _rules->getInventory("STR_GROUND"))
		{
			i = _tileCraft->getInventory()->erase(i);
			continue;
		}

		++i;
	}
} */

/**
 * Generates a map of modules (sets of tiles) for a new Battlescape game.
 * @param script - the scripts to use to build the map
 */
void BattlescapeGenerator::generateMap(const std::vector<MapScript*>* script)
{
	// set ambient sound
	_battleSave->setAmbientSound(_terrain->getAmbience());

	// set up map generation vars
	_dummy = new MapBlock("dummy");
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

	bool placed = false;
	int
		x = 0,
		y = 0;

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

		if (_rules->getMCDPatch((*i)->getName()))
			_rules->getMCDPatch((*i)->getName())->modifyData(*i);

		_battleSave->getMapDataSets()->push_back(*i);
		mapDataSetIDOffset++;
	}


	// generate the map now and store it inside the tile objects

	// this mission type is "hard-coded" in terms of map layout
	if (_battleSave->getMissionType() == "STR_BASE_DEFENSE")
		generateBaseMap();

	// process script
	for (std::vector<MapScript*>::const_iterator
			i = script->begin();
			i != script->end();
			++i)
	{
		MapScript* const command = *i;

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
							&& !conditionals[*condition])
						|| (*condition < 0
							&& conditionals[std::abs(*condition)]))
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
					j < command->getExecutions();
					++j)
			{
				MapBlock* block = NULL;
				int
					x,
					y;

				switch (command->getType())
				{
					case MSC_ADDBLOCK:
						block = command->getNextBlock(_terrain);
						// select an X and Y position from within the rects, using an even distribution
						if (block != NULL
							&& selectPosition(
											command->getRects(),
											x,
											y,
											block->getSizeX(),
											block->getSizeY()) == true)
						{
							success = (addBlock(x, y, block) == true)
									|| success;
						}
					break;

					case MSC_ADDLINE:
						success = (addLine(
										(MapDirection)(command->getDirection()),
										command->getRects()) == true);
					break;

					case MSC_ADDCRAFT:
						craftMap = _craft->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(
																									999,
																									999,
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
					break;

					case MSC_ADDUFO:
						// as above, note that the craft and the ufo will never be allowed to overlap.
						// TODO: make _ufopos a vector ;)
						ufoMap = _ufo->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(
																								999,
																								999,
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
											x,
											y,
											block->getSizeX(),
											block->getSizeY()) == true)
							{
								// fill area will succeed if even one block is added
								success = (addBlock(x, y, block) == true)
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
									else
									{
										// wildcard, we don't care what block it is, we just wanna know if there's a block here
										success = (_blocks[x][y] != NULL);
									}
								}
							}
						}
					break;

					case MSC_REMOVE:
						success = removeBlocks(command);
					break;

					default:
					break;
				}
			}
		}
	}

	if (_blocksToDo != 0)
	{
		throw Exception("Map failed to fully generate.");
	}

	loadNodes();

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
			craftDataSetIDOffset++;
		}

		// TODO: put ufo positions in a vector rather than a single rect, and iterate here?
		// will probably need to make ufomap a vector too i suppose.
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
				Node::UFOSEGMENT);

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
				_segments[_ufoPos.x + i][_ufoPos.y + j] = Node::UFOSEGMENT;
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
				true,
				true);
		loadRMP(
				craftMap,
				_craftPos.x * 10,
				_craftPos.y * 10,
				Node::CRAFTSEGMENT);

		for (int
				i = 0;
				i < craftMap->getSizeX() / 10;
				++i)
		{
			for (int
					j = 0;
					j < craftMap->getSizeY() / 10;
					j++)
			{
				_segments[_craftPos.x + i][_craftPos.y + j] = Node::CRAFTSEGMENT;
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

	delete _dummy;

	attachNodeLinks();
}

/**
 * Generates a map based on the base's layout.
 * This doesn't drill or fill with dirt, the script must do that.
 */
void BattlescapeGenerator::generateBaseMap()
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

					int mapnum = atoi(mapname.substr(
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
					num++;

					if ((*i)->getRules()->getStorage() > 0)
					{
						int groundLevel;
						for (
								groundLevel = _mapsize_z - 1;
								groundLevel >= 0;
								--groundLevel)
						{
							if (_battleSave->getTile(Position(
															x * 10,
															y * 10,
															groundLevel))
														->hasNoFloor(NULL) == false)

								break;
						}

						// general stores - there is where the items are put
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
								// we only want every other tile, giving us a "checkerboard" pattern
								if ((k + l) %2 == 0)
								{
									Tile
										* const t = _battleSave->getTile(Position(
																				k,
																				l,
																				groundLevel)),
										* const tEast = _battleSave->getTile(Position(
																				k + 1,
																				l,
																				groundLevel)),
										* const tSouth = _battleSave->getTile(Position(
																				k,
																				l + 1,
																				groundLevel));

									if (t != NULL
										&& t->getMapData(MapData::O_FLOOR)
										&& t->getMapData(MapData::O_OBJECT) == NULL
										&& tEast != NULL
										&& tEast->getMapData(MapData::O_WESTWALL) == NULL
										&& tSouth != NULL
										&& tSouth->getMapData(MapData::O_NORTHWALL) == NULL)
									{
										_battleSave->getStorageSpace().push_back(Position(k, l, groundLevel));
									}
								}
							}
						}

						// put the inventory tile on the lower floor, just to be safe.
						if (_tileCraft == NULL)
							_tileCraft = _battleSave->getTile(Position(
																	x * 10 + 5,
																	y * 10 + 5,
																	groundLevel - 1));
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
 * Clears a module from the map.
 * @param x		- the x offset
 * @param y		- the y offset
 * @param sizeX	- how far along the x axis to clear
 * @param sizeY	- how far along the y axis to clear
 */
void BattlescapeGenerator::clearModule(
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
				Tile* const tile = _battleSave->getTile(Position(dx, dy, z));
				for (int
						i = 0;
						i < 4;
						++i)
				{
					tile->setMapData(NULL, -1, -1, i);
				}
			}
		}
	}
}

/**
 * Loads all the nodes from the map modules.
 */
void BattlescapeGenerator::loadNodes()
{
	int segment = 0;

	for (int
			itY = 0;
			itY < (_mapsize_y / 10);
			++itY)
	{
		for (int
				itX = 0;
				itX < (_mapsize_x / 10);
				++itX)
		{
			_segments[itX][itY] = segment;

			if (_blocks[itX][itY] != 0
				&& _blocks[itX][itY] != _dummy)
			{
				if (!(
						_blocks[itX][itY]->isInGroup(MT_LANDINGZONE)
							&& _landingzone[itX][itY]))
				{
					loadRMP(
							_blocks[itX][itY],
							itX * 10,
							itY * 10,
							segment++);
				}
			}
		}
	}
}

/**
 * Attaches all the nodes together in an intimate web of C++.
 */
void BattlescapeGenerator::attachNodeLinks()
{
	for (std::vector<Node*>::const_iterator
			i = _battleSave->getNodes()->begin();
			i != _battleSave->getNodes()->end();
			++i)
	{
		Node* const node = *i;

		const int
			segmentX = node->getPosition().x / 10,
			segmentY = node->getPosition().y / 10,
			neighbourDirections[4]			= {-2, -3, -4, -5},
			neighbourDirectionsInverted[4]	= {-4, -5, -2, -3};
		int neighbourSegments[4];

		if (segmentX == (_mapsize_x / 10) - 1)
			neighbourSegments[0] = -1;
		else
			neighbourSegments[0] = _segments[segmentX + 1][segmentY];

		if (segmentY == (_mapsize_y / 10) - 1)
			neighbourSegments[1] = -1;
		else
			neighbourSegments[1] = _segments[segmentX][segmentY + 1];

		if (segmentX == 0)
			neighbourSegments[2] = -1;
		else
			neighbourSegments[2] = _segments[segmentX - 1][segmentY];

		if (segmentY == 0)
			neighbourSegments[3] = -1;
		else
			neighbourSegments[3] = _segments[segmentX][segmentY - 1];

		for (std::vector<int>::iterator
				j = node->getNodeLinks()->begin();
				j != node->getNodeLinks()->end();
				++j)
		{
			for (int
					n = 0;
					n < 4;
					++n)
			{
				if (*j == neighbourDirections[n])
				{
					for (std::vector<Node*>::const_iterator
							k = _battleSave->getNodes()->begin();
							k != _battleSave->getNodes()->end();
							++k)
					{
						if ((*k)->getSegment() == neighbourSegments[n])
						{
							for (std::vector<int>::iterator
									l = (*k)->getNodeLinks()->begin();
									l != (*k)->getNodeLinks()->end();
									++l )
							{
								if (*l == neighbourDirectionsInverted[n])
								{
									*l = node->getID();
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
bool BattlescapeGenerator::selectPosition(
		const std::vector<SDL_Rect*>* rects,
		int& X,
		int& Y,
		int sizeX,
		int sizeY)
{
	std::vector<SDL_Rect*> available;
	std::vector<std::pair<int, int> > valid;

	SDL_Rect wholeMap;
	wholeMap.x = 0;
	wholeMap.y = 0;
	wholeMap.w = (_mapsize_x / 10);
	wholeMap.h = (_mapsize_y / 10);

	sizeX = (sizeX / 10);
	sizeY = (sizeY / 10);

	if (rects->empty() == true)
		available.push_back(&wholeMap);
	else
		available = *rects;

	for (std::vector<SDL_Rect*>::const_iterator
			i = available.begin();
			i != available.end();
			++i)
	{
		if (sizeX > (*i)->w
			|| sizeY > (*i)->h)
		{
			continue;
		}

		for (int
				x = (*i)->x;
				x + sizeX <= (*i)->x + (*i)->w
					&& x + sizeX <= wholeMap.w;
				++x)
		{
			for (int
					y = (*i)->y;
					y + sizeY <= (*i)->y + (*i)->h
						&& y + sizeY <= wholeMap.h;
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
							if (_blocks[xCheck][yCheck])
								add = false;
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

	std::pair<int, int> selection = valid.at(RNG::generate(
														0,
														valid.size() - 1));
	X = selection.first;
	Y = selection.second;

	return true;
}

/**
 * Adds a craft (or UFO) to the map, and tries to add a landing zone type block underneath it.
 * @param craftMap	- pointer to the MapBlock for the craft in question
 * @param command	- pointer to the script command to pull info from
 * @param craftPos	- reference the position of the craft and store it here
 * @return, true if the craft was placed
 */
bool BattlescapeGenerator::addCraft(
		MapBlock* craftMap,
		MapScript* command,
		SDL_Rect& craftPos)
{
	craftPos.w = craftMap->getSizeX();
	craftPos.h = craftMap->getSizeY();
	bool placed = false;
	int
		x,
		y;

	placed = selectPosition(
						command->getRects(),
						x,
						y,
						craftPos.w,
						craftPos.h);

	// if ok, allocate it
	if (placed == true)
	{
		craftPos.x = x;
		craftPos.y = y;
		craftPos.w /= 10;
		craftPos.h /= 10;

		for (
				x = 0;
				x < craftPos.w;
				++x)
		{
			for (
					y = 0;
					y < craftPos.h;
					++y)
			{
				_landingzone[craftPos.x + x][craftPos.y + y] = true;

				MapBlock* block = command->getNextBlock(_terrain);
				if (block != NULL
					&& _blocks[craftPos.x + x][craftPos.y + y] == NULL)
				{
					_blocks[craftPos.x + x][craftPos.y + y] = block;

					_blocksToDo--;
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
bool BattlescapeGenerator::addLine(
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

	bool placed = false;
	int tries = 0,
		roadX,
		roadY,
		* iteratorValue = &roadX,
		* staticValue = &roadY,
		limit = _mapsize_x / 10;
	MapBlockType
		comparator = MT_NSROAD,
		typeToAdd = MT_EWROAD;

	if (direction == MD_VERTICAL)
	{
		iteratorValue = &roadY;
		staticValue = &roadX;
		comparator = MT_EWROAD;
		typeToAdd = MT_NSROAD;
		limit = _mapsize_y / 10;
	}

	while (placed == false)
	{
		selectPosition(
					rects,
					roadX,
					roadY,
					10,
					10);
		placed = true;

		for (
				*iteratorValue = 0;
				*iteratorValue < limit;
				*iteratorValue += 1)
		{
			if (_blocks[roadX][roadY] != NULL
				&& _blocks[roadX][roadY]->isInGroup(comparator) == false)
			{
				placed = false;
				break;
			}
		}

		if (tries++ > 20)
			return false;
	}

	*iteratorValue = 0;
	while (*iteratorValue < limit)
	{
		if (_blocks[roadX][roadY] == 0)
			addBlock(
					roadX,
					roadY,
					_terrain->getRandomMapBlock(
											10,
											10,
											typeToAdd));
		else if (_blocks[roadX][roadY]->isInGroup(comparator))
		{
			_blocks[roadX][roadY] = _terrain->getRandomMapBlock(
															10,
															10,
															MT_CROSSING);
			clearModule(
					roadX * 10,
					roadY * 10,
					10,
					10);
			loadMAP(
					_blocks[roadX][roadY],
					roadX * 10,
					roadY * 10,
					_terrain,
					0);
		}

		*iteratorValue += 1;
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
bool BattlescapeGenerator::addBlock(
		int x,
		int y,
		MapBlock* block)
{
	const int
		xSize = (block->getSizeX() - 1) / 10,
		ySize = (block->getSizeY() - 1) / 10;

	for (int
			xd = 0;
			xd <= xSize;
			++xd)
	{
		for (int
				yd = 0;
				yd != ySize;
				++yd)
		{
			if (_blocks[x + xd][y + yd])
				return false;
		}
	}

	int drillType = 0;
	for (int
			xd = 0;
			xd <= xSize;
			++xd)
	{
		for (int
				yd = 0;
				yd <= ySize;
				++yd)
		{
			_blocks[x + xd][y + yd] = _dummy;

			_blocksToDo--;
		}
	}

	// mark the south edge of the block for drilling
	for (int
			xd = 0;
			xd <= xSize;
			++xd)
	{
		_drillMap[x+xd][y + ySize] = MD_VERTICAL;
	}

	// then the east edge
	for (int
			yd = 0;
			yd <= ySize;
			++yd)
	{
		_drillMap[x + xSize][y+yd] = MD_HORIZONTAL;
	}

	// then the far corner gets marked for both
	// this also marks 1x1 modules
	_drillMap[x + xSize][y + ySize] = MD_BOTH;

	_blocks[x][y] = block;

	bool visible = (_battleSave->getMissionType() == "STR_BASE_DEFENSE"); // yes, i'm hard coding these, big whoop

	loadMAP(
			_blocks[x][y],
			x * 10,
			y * 10,
			_terrain,
			0,
			visible);

	return true;
}

/**
 * Drills a tunnel between existing map modules.
 * note that this drills all modules currently on the map,
 * so it should take place BEFORE the dirt is added in base defenses.
 * @param tunnelInfo	- pointer to the wall replacements and level to dig on
 * @param rects	- pointer to a vector of pointers defining the length/width of the tunnels themselves
 * @param dir	- the direction to drill
 */
void BattlescapeGenerator::drillModules(
		TunnelData* tunnelInfo,
		const std::vector<SDL_Rect*>* rects,
		MapDirection dir)
{
	const MCDReplacement
		* wWall = tunnelInfo->getMCDReplacement("westWall"),
		* nWall = tunnelInfo->getMCDReplacement("northWall"),
		* corner = tunnelInfo->getMCDReplacement("corner"),
		* floor = tunnelInfo->getMCDReplacement("floor");

	SDL_Rect rect;
	rect.x =
	rect.y =
	rect.w =
	rect.h = 3;

	if (rects->empty() == false)
		rect = *rects->front();

	for (int
			i = 0;
			i < (_mapsize_x / 10);
			++i)
	{
		for (int
				j = 0;
				j < (_mapsize_y / 10);
				++j)
		{
			if (_blocks[i][j] == 0)
				continue;

			Tile* tile = NULL;
			MapData* data = NULL;

			if (dir != MD_VERTICAL)
			{
				// drill east
				if (i < (_mapsize_x / 10) - 1
					&& (_drillMap[i][j] == MD_HORIZONTAL
						|| _drillMap[i][j] == MD_BOTH)
					&& _blocks[i + 1][j] != NULL)
				{
					// remove stuff
					for (int
							k = rect.y;
							k != rect.y + rect.h;
							++k)
					{
						tile = _battleSave->getTile(Position(
															i * 10 + 9,
															j * 10 + k,
															tunnelInfo->level));
						if (tile != NULL)
						{
							tile->setMapData(0,-1,-1, MapData::O_WESTWALL);
							tile->setMapData(0,-1,-1, MapData::O_OBJECT);

							if (floor != NULL)
							{
								data = _terrain->getMapDataSets()->at(floor->dataSet)->getObjects()->at(floor->entry);
								tile->setMapData(
											data,
											floor->entry,
											floor->dataSet,
											MapData::O_FLOOR);
							}

							tile = _battleSave->getTile(Position(
																(i + 1) * 10,
																j * 10 + k,
																tunnelInfo->level));
							tile->setMapData(NULL,-1,-1, MapData::O_WESTWALL);

							MapData* obj = tile->getMapData(MapData::O_OBJECT);
							if (obj != NULL
								&& obj->getTUCost(MT_WALK) == 0)
							{
								tile->setMapData(NULL,-1,-1, MapData::O_OBJECT);
							}
						}
					}

					if (nWall != NULL)
					{
						data = _terrain->getMapDataSets()->at(nWall->dataSet)->getObjects()->at(nWall->entry);
						tile = _battleSave->getTile(Position(
															i * 10 + 9,
															j * 10 + rect.y,
															tunnelInfo->level));
						tile->setMapData(
										data,
										nWall->entry,
										nWall->dataSet,
										MapData::O_NORTHWALL);
						tile = _battleSave->getTile(Position(
															i * 10 + 9,
															j * 10 + rect.y + rect.h,
															tunnelInfo->level));
						tile->setMapData(
										data,
										nWall->entry,
										nWall->dataSet,
										MapData::O_NORTHWALL);
					}

					if (corner != NULL)
					{
						data = _terrain->getMapDataSets()->at(corner->dataSet)->getObjects()->at(corner->entry);
						tile = _battleSave->getTile(Position(
															(i + 1) * 10,
															j * 10 + rect.y,
															tunnelInfo->level));

						if (tile->getMapData(MapData::O_NORTHWALL) == NULL)
							tile->setMapData(
											data,
											corner->entry,
											corner->dataSet,
											MapData::O_NORTHWALL);
					}
				}
			}

			if (dir != MD_HORIZONTAL)
			{
				// drill south
				if (j < (_mapsize_y / 10) - 1
					&& (_drillMap[i][j] == MD_VERTICAL
						|| _drillMap[i][j] == MD_BOTH)
					&& _blocks[i][j + 1] != NULL)
				{
					// remove stuff
					for (int
							k = rect.x;
							k != rect.x + rect.w;
							++k)
					{
						Tile* tile = _battleSave->getTile(Position(
																i * 10 + k,
																j * 10 + 9,
																tunnelInfo->level));
						if (tile != NULL)
						{
							tile->setMapData(NULL,-1,-1, MapData::O_NORTHWALL);
							tile->setMapData(NULL,-1,-1, MapData::O_OBJECT);

							if (floor != NULL)
							{
								data = _terrain->getMapDataSets()->at(floor->dataSet)->getObjects()->at(floor->entry);
								tile->setMapData(
												data,
												floor->entry,
												floor->dataSet,
												MapData::O_FLOOR);
							}

							tile = _battleSave->getTile(Position(
																i * 10 + k,
																(j + 1) * 10,
																tunnelInfo->level));
							tile->setMapData(NULL, -1, -1, MapData::O_NORTHWALL);

							MapData* obj = tile->getMapData(MapData::O_OBJECT);
							if (obj != NULL
								&& obj->getTUCost(MT_WALK) == 0)
							{
								tile->setMapData(NULL,-1,-1, MapData::O_OBJECT);
							}
						}
					}

					if (wWall != NULL)
					{
						data = _terrain->getMapDataSets()->at(wWall->dataSet)->getObjects()->at(wWall->entry);
						tile = _battleSave->getTile(Position(
															i * 10 + rect.x,
															j * 10 + 9,
															tunnelInfo->level));
						tile->setMapData(
										data,
										wWall->entry,
										wWall->dataSet,
										MapData::O_WESTWALL);
						tile = _battleSave->getTile(Position(
															i * 10 + rect.x + rect.w,
															j * 10 + 9,
															tunnelInfo->level));
						tile->setMapData(
										data,
										wWall->entry,
										wWall->dataSet,
										MapData::O_WESTWALL);
					}

					if (corner != NULL)
					{
						data = _terrain->getMapDataSets()->at(corner->dataSet)->getObjects()->at(corner->entry);
						tile = _battleSave->getTile(Position(
															i * 10 + rect.x,
															(j + 1) * 10,
															tunnelInfo->level));

						if (tile->getMapData(MapData::O_WESTWALL) == NULL)
							tile->setMapData(
											data,
											corner->entry,
											corner->dataSet,
											MapData::O_WESTWALL);
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
bool BattlescapeGenerator::removeBlocks(MapScript* command)
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
					&& _blocks[x][y] != _dummy)
				{
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
								const std::pair<int, int> pos = std::make_pair<int, int>(x, y);
								if (std::find(
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
								{
									deleted.push_back(std::make_pair<int, int>(x, y));
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
								const std::pair<int, int> pos = std::make_pair<int, int>(x, y);
								if (std::find(
											deleted.begin(),
											deleted.end(),
											pos) == deleted.end())
								{
									deleted.push_back(std::make_pair<int, int>(x, y));
								}
							}
						}
					}
					else
					{
						// the deleted vector should only contain unique entries
						const std::pair<int, int> pos = std::make_pair<int, int>(x, y);
						if (std::find(
									deleted.begin(),
									deleted.end(),
									pos) == deleted.end())
						{
							deleted.push_back(std::make_pair<int, int>(x, y));
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

		for (int
				dx = x;
				dx != x + (_blocks[x][y]->getSizeX() / 10);
				++dx)
		{
			for (int
					dy = y;
					dy != y + (_blocks[x][y]->getSizeY() / 10);
					++dy)
			{
				_blocks[dx][dy] = 0;

				_blocksToDo++;
			}
		}

		// this command succeeds if even one block is removed.
		success = true;
	}

	return success;
}

}
