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
#include "../Ruleset/RuleTerrain.h"
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
 * @param game pointer to Game object.
 */
BattlescapeGenerator::BattlescapeGenerator(Game* game)
	:
		_game(game),
		_save(game->getSavedGame()->getSavedBattle()),
		_rules(game->getRuleset()),
		_res(_game->getResourcePack()),
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
		_craftX(0),
		_craftY(0),
		_craftZ(0),
		_baseEquipScreen(false),
		_battleOrder(0)
{
	//Log(LOG_INFO) << "Create BattlescapeGenerator";
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
 * @param craft - pointer to XCom craft
 */
void BattlescapeGenerator::setCraft(Craft* craft)
{
	_craft = craft;
	_craft->setInBattlescape(true);
}

/**
 * Sets the ufo involved in the battle.
 * @param ufo - pointer to UFO
 */
void BattlescapeGenerator::setUfo(Ufo* ufo)
{
	_ufo = ufo;
	_ufo->setInBattlescape(true);
}

/**
 * Sets the XCom base involved in the battle.
 * @param base - pointer to XCom base
 */
void BattlescapeGenerator::setBase(Base* base)
{
	_base = base;
	_base->setInBattlescape(true);
}

/**
 * Sets the terror site involved in the battle.
 * @param terror - pointer to terror site
 */
void BattlescapeGenerator::setTerrorSite(TerrorSite* terror)
{
	_terror = terror;
	_terror->setInBattlescape(true);
}

/**
 * Sets the alien base involved in the battle.
 * @param base - pointer to alien base
 */
void BattlescapeGenerator::setAlienBase(AlienBase* base)
{
	_alienBase = base;
	_alienBase->setInBattlescape(true);
}

/**
 * kL. Sets the world terrainRule of where a ufo crashed or landed.
 * @param texture - pointer to a terrainRule as determined in ConfirmLandingState
 */
void BattlescapeGenerator::setWorldTerrain(RuleTerrain* terrain) // kL
{
	_worldTerrain = terrain;
}

/**
 * Sets the world texture where a ufo crashed or landed.
 * This is used to determine the terrain if worldTerrain is "".
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
 * @param alienRace - alien (main) race
 */
void BattlescapeGenerator::setAlienRace(const std::string& alienRace)
{
	//Log(LOG_INFO) << "gen, race = " << alienRace;
	_alienRace = alienRace;

	_game->getSavedGame()->getSavedBattle()->setAlienRace(alienRace);
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
	for (std::vector<BattleUnit*>::iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
			++j)
	{
		if ((_game->getSavedGame()->getSavedBattle()->isAborted()
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

	while (_game->getSavedGame()->getSavedBattle()->getSide() != FACTION_PLAYER)
	{
		_game->getSavedGame()->getSavedBattle()->endBattleTurn();
	}

	_save->resetTurnCounter();

	AlienDeployment* ruleDeploy = _rules->getDeployment(_save->getMissionType());
	ruleDeploy->getDimensions(
							&_mapsize_x,
							&_mapsize_y,
							&_mapsize_z);
	size_t pick = RNG::generate(
							0,
							ruleDeploy->getTerrains().size() - 1);
	_terrain = _rules->getTerrain(ruleDeploy->getTerrains().at(pick));

	_worldShade = ruleDeploy->getShade();

	_save->initMap(
				_mapsize_x,
				_mapsize_y,
				_mapsize_z);

	generateMap(); // <-- BATTLE MAP GENERATION.


	bool selectedFirstSoldier = false;
	int highestSoldierID = 0;

	for (std::vector<BattleUnit*>::iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
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
					_save->setSelectedUnit(*j);
					selectedFirstSoldier = true;
				}

				Node* node = _save->getSpawnNode(
												NR_XCOM,
												*j);
				if (node
					|| placeUnitNearFriend(*j))
				{
					if (node)
						_save->setUnitPosition(
											*j,
											node->getPosition());

					if (_tileCraft == NULL)
					{
						_tileCraft = (*j)->getTile();
						_save->setBattleInventory(_tileCraft);
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
	for (std::vector<BattleItem*>::iterator
			j = _save->getItems()->begin();
			j != _save->getItems()->end();
			++j)
	{
		if ((*j)->getOwner() == NULL
			|| (*j)->getOwner()->getId() > highestSoldierID)
		{
			(*j)->setTile(NULL);
		}
	}

	_unitSequence = _save->getUnits()->back()->getId() + 1;
	deployAliens(_rules->getAlienRace(_alienRace), ruleDeploy);

	deployCivilians(ruleDeploy->getCivilians());

	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		if (_save->getTiles()[i]->getMapData(MapData::O_FLOOR)
			&& (_save->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT
				|| (_save->getTiles()[i]->getPosition().z == 1
					&& _save->getTiles()[i]->getMapData(MapData::O_FLOOR)->isGravLift()
					&& _save->getTiles()[i]->getMapData(MapData::O_OBJECT))))
		{
			_save->getTiles()[i]->setDiscovered(true, 2);
		}
	}

	// set shade (alien bases are a little darker, sites depend on worldshade)
	_save->setGlobalShade(_worldShade);

	_save->getTileEngine()->calculateSunShading();
	_save->getTileEngine()->calculateTerrainLighting();
	_save->getTileEngine()->calculateUnitLighting();
	_save->getTileEngine()->recalculateFOV();
}

/**
 * Starts the generator; this fills up the SavedBattleGame with data.
 */
void BattlescapeGenerator::run()
{
	AlienDeployment* ruleDeploy = NULL;
	if (_ufo)
		ruleDeploy = _rules->getDeployment(_ufo->getRules()->getType());
	else
		ruleDeploy = _rules->getDeployment(_save->getMissionType());

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
	_save->initMap(
				_mapsize_x,
				_mapsize_y,
				_mapsize_z);
	_save->initUtilities(_res);

	// let's generate the map now and store it inside the tile objects
	generateMap(); // <-- BATTLE MAP GENERATION.

	_save->setTerrain(_terrain->getName()); // sza_MusicRules

	// kL_begin: blow up PowerSources after aLiens & civies spawn, but before xCom spawn.
	// (i hope) nope-> CTD
/*	deployAliens(
			_rules->getAlienRace(_alienRace),
			ruleDeploy);
	deployCivilians(ruleDeploy->getCivilians());

	fuelPowerSources();
	if (_save->getMissionType() ==  "STR_UFO_CRASH_RECOVERY")
		explodePowerSources(); */ // kL_end.

	if (_craft != NULL
		|| _base != NULL)
	{
		deployXCOM();
	}

	deployAliens(
			_rules->getAlienRace(_alienRace),
			ruleDeploy);
	deployCivilians(ruleDeploy->getCivilians());

	if (_generateFuel)
		fuelPowerSources();

	if (_save->getMissionType() ==  "STR_UFO_CRASH_RECOVERY")
		explodePowerSources();

	if (_save->getMissionType() == "STR_BASE_DEFENSE")
	{
		for (int
				i = 0;
				i < _save->getMapSizeXYZ();
				++i)
		{
			_save->getTiles()[i]->setDiscovered(true, 2);
		}

		_save->calculateModuleMap();
	}

	if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
		|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
	{
		for (int
				i = 0;
				i < _save->getMapSizeXYZ();
				++i)
		{
			if (_save->getTiles()[i]->getMapData(MapData::O_FLOOR)
				&& (_save->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT
					|| (_save->getTiles()[i]->getPosition().z == 1
						&& _save->getTiles()[i]->getMapData(MapData::O_FLOOR)->isGravLift()
						&& _save->getTiles()[i]->getMapData(MapData::O_OBJECT))))
			{
				_save->getTiles()[i]->setDiscovered(true, 2);
			}
		}
	}

	// set shade (alien bases are a little darker, sites depend on worldshade)
	_save->setGlobalShade(_worldShade);

	_save->getTileEngine()->calculateSunShading();
	_save->getTileEngine()->calculateTerrainLighting();
	_save->getTileEngine()->calculateUnitLighting();
	_save->getTileEngine()->recalculateFOV();
}

/**
* Deploys all the X-COM units and equipment based on the Geoscape base / craft.
*/
void BattlescapeGenerator::deployXCOM()
{
	//Log(LOG_INFO) << "BattlescapeGenerator::deployXCOM()";
	RuleInventory* ground = _rules->getInventory("STR_GROUND");

	if (_craft != NULL)
	{
		_base = _craft->getBase();

		if (_baseEquipScreen == false)
		{
			//Log(LOG_INFO) << ". craft VALID";

			// add all vehicles that are in the craft - a vehicle is actually an item,
			// which you will never see as it is converted to a unit;
			// however the item itself becomes the weapon it "holds".
			for (std::vector<Vehicle*>::iterator
					i = _craft->getVehicles()->begin();
					i != _craft->getVehicles()->end();
					++i)
			{
				//Log(LOG_INFO) << ". . isCraft: addXCOMVehicle " << (int)*i;
				BattleUnit* unit = addXCOMVehicle(*i);
				if (unit
					&& _save->getSelectedUnit() == NULL)
				{
					_save->setSelectedUnit(unit);
				}
			}
		}
	}
	else if (_base != NULL
		&& _baseEquipScreen == false)
	{
		// add vehicles that are in Base inventory.
		for (std::vector<Vehicle*>::iterator
				i = _base->getVehicles()->begin();
				i != _base->getVehicles()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . isBase: addXCOMVehicle " << (int)*i;
			BattleUnit* unit = addXCOMVehicle(*i);
			if (unit
				&& _save->getSelectedUnit() == NULL)
			{
				_save->setSelectedUnit(unit);
			}
		}

		if (_game->getSavedGame()->getMonthsPassed() == -1)
		{
			// add vehicles from Crafts at Base.
			for (std::vector<Craft*>::iterator
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
			{
				for (std::vector<Vehicle*>::iterator
					j = (*i)->getVehicles()->begin();
					j != (*i)->getVehicles()->end();
					++j)
				{
					BattleUnit* unit = addXCOMVehicle(*j);
					if (unit
						&& _save->getSelectedUnit() == NULL)
					{
						_save->setSelectedUnit(unit);
					}
				}
			}
		}
	}

	// add soldiers that are in the Craft or Base.
	for (std::vector<Soldier*>::iterator
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
			BattleUnit* unit = addXCOMUnit(new BattleUnit(
														*i,
														FACTION_PLAYER,
														static_cast<int>(_game->getSavedGame()->getDifficulty()))); // kL_add: For VictoryPts value per death.

			if (unit)
			{
				unit->setBattleOrder(++_battleOrder);

				if (_save->getSelectedUnit() == NULL)
					_save->setSelectedUnit(unit);
			}
		}
	}

	//Log(LOG_INFO) << ". addXCOMUnit(s) DONE";

	// maybe we should assign all units to the first tile of the skyranger before the
	// inventory pre-equip and then reassign them to their correct tile afterwards?
	// Fix: make them invisible, they are made visible afterwards.
	for (std::vector<BattleUnit*>::iterator // pre-battle equip; give all xCom Soldiers access to the inventory tile.
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
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
		for (std::map<std::string, int>::iterator
				i = _craft->getItems()->getContents()->begin();
				i != _craft->getItems()->getContents()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . . *i = _craft->getItems()->getContents()";
			for (int
					count = 0;
					count < i->second;
					count++)
			{
				//Log(LOG_INFO) << ". . . count+";
				_tileCraft->addItem(
									new BattleItem(
												_rules->getItem(i->first),
												_save->getCurrentItemId()),
									ground);
				//Log(LOG_INFO) << ". . . count cycle";
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
				RuleItem* rule = _rules->getItem(i->first);
				if (rule->getBigSprite() > -1
					&& rule->getBattleType() != BT_NONE
					&& rule->getBattleType() != BT_CORPSE
					&& rule->isFixed() == false
					&& _game->getSavedGame()->isResearched(rule->getRequirements()))
				{
					for (int
							count = 0;
							count < i->second;
							count++)
					{
						_tileCraft->addItem(
											new BattleItem(
														_rules->getItem(i->first),
														_save->getCurrentItemId()),
											ground);
					}

					std::map<std::string, int>::iterator mark = i;
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

		for (std::vector<Craft*>::iterator // add items from Crafts at Base.
				i = _base->getCrafts()->begin();
				i != _base->getCrafts()->end();
				++i)
		{
			if ((*i)->getStatus() == "STR_OUT")
				continue;

			for (std::map<std::string, int>::iterator
					j = (*i)->getItems()->getContents()->begin();
					j != (*i)->getItems()->getContents()->end();
					++j)
			{
				for (int
						count = 0;
						count < j->second;
						count++)
				{
					_tileCraft->addItem(
										new BattleItem(
													_rules->getItem(j->first),
													_save->getCurrentItemId()),
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
	for (std::vector<BattleItem*>::iterator
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

	for (std::vector<BattleItem*>::iterator
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
								i = _save->getUnits()->begin();
								i != _save->getUnits()->end();
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
	for (std::vector<BattleItem*>::iterator
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
		if ((*i)->needsAmmo()) // or do it My way.
			loadGroundWeapon(*i);
	}
	//Log(LOG_INFO) << ". loading DONE";

	// clean up moved items
	for (std::vector<BattleItem*>::iterator
			i = _tileCraft->getInventory()->begin();
			i != _tileCraft->getInventory()->end();
			)
	{
		if ((*i)->getSlot() == ground)
		{
			(*i)->setXCOMProperty();

			_save->getItems()->push_back(*i);

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
	std::string vehicle = tank->getRules()->getType();
	Unit* unitRule = _rules->getUnit(vehicle);

	BattleUnit* tankUnit = addXCOMUnit(new BattleUnit( // add Vehicle as a unit.
													unitRule,
													FACTION_PLAYER,
													_unitSequence++,
													_rules->getArmor(unitRule->getArmor()),
													0));
	if (tankUnit)
	{
		tankUnit->setTurretType(tank->getRules()->getTurretType());

		BattleItem* item = new BattleItem( // add Vehicle as an item and assign the unit as its owner.
									_rules->getItem(vehicle),
									_save->getCurrentItemId());
		if (!addItem(
					item,
					tankUnit))
		{
			delete item;
			delete tankUnit;

			return NULL;
		}

		if (tank->getRules()->getCompatibleAmmo()->empty() == false)
		{
			std::string ammo = tank->getRules()->getCompatibleAmmo()->front();
			BattleItem* ammoItem = new BattleItem( // add item(ammo) and assign the Vehicle-ITEM as its owner.
											_rules->getItem(ammo),
											_save->getCurrentItemId());
			if (!addItem(
						ammoItem,
						tankUnit))
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
				RuleItem* itemRule = _rules->getItem(*i);
				if (itemRule)
				{
					BattleItem* item = new BattleItem(
													itemRule,
													_save->getCurrentItemId());

					if (!addItem(
								item,
								tankUnit))
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
			|| _save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
			|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
		&& _baseEquipScreen == false)
	{
		Node* node = _save->getSpawnNode(
									NR_XCOM,
									unit);
		if (node)
		{
			_save->getUnits()->push_back(unit);

			_save->setUnitPosition(
								unit,
								node->getPosition());
			unit->setDirection(RNG::generate(0, 7));
			unit->deriveRank();
//			_save->getTileEngine()->calculateFOV(unit);

			_tileCraft = _save->getTile(node->getPosition());
			_save->setBattleInventory(_tileCraft);

			return unit;
		}
		else if (_save->getMissionType() != "STR_BASE_DEFENSE")
		{
			if (placeUnitNearFriend(unit))
			{
				_save->getUnits()->push_back(unit);

				unit->setDirection(RNG::generate(0, 7));
				unit->deriveRank();
//				_save->getTileEngine()->calculateFOV(unit);

				_tileCraft = _save->getTile(unit->getPosition());
				_save->setBattleInventory(_tileCraft);

				return unit;
			}
		}
	}
	else if (_craft // Transport craft deployments (Lightning & Avenger)
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
			Position pos = Position(
								(*i)[0] + (_craftX * 10),
								(*i)[1] + (_craftY * 10),
								(*i)[2] + _craftZ);
			if (canPlaceXCOMUnit(_save->getTile(pos)))
			{
				//Log(LOG_INFO) << ". canPlaceXCOMUnit()";
				if (_save->setUnitPosition(
										unit,
										pos))
				{
					//Log(LOG_INFO) << ". setUnitPosition()";
					_save->getUnits()->push_back(unit);

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
			if (canPlaceXCOMUnit(_save->getTiles()[i]))
			{
				// kL_begin: BattlescapeGenerator, set tankPosition
				if (unit->getArmor()->getSize() > 1) // is a tank
				{
					if (_save->getTiles()[i]->getPosition().x == _tileCraft->getPosition().x) // and the ground-inventory-tile is on this[i] tile's x-axis|
					{
						if (++tankPos == 3)
						{
							if (_save->setUnitPosition( // set unit position SUCCESS
													unit,
													_save->getTiles()[i]->getPosition()))
							{
								_save->getUnits()->push_back(unit); // add unit to vector of Units.

								return unit;
							}
						}
					}
				}
				else if (_save->setUnitPosition( // set unit position SUCCESS
											unit,
											_save->getTiles()[i]->getPosition()))
				{
					_save->getUnits()->push_back(unit); // add unit to vector of Units.

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
 * @param tile the given tile.
 * @return whether the unit can be placed here.
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
			_save->setBattleInventory(_tileCraft);
		}

		return true;
	}

	return false;
}

/**
 * Loads a weapon on the inventoryTile.
 * @param item, Pointer to the weaponItem.
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
			_save->getItems()->push_back(*i);
		}
	}
	//Log(LOG_INFO) << "BattlescapeGenerator::loadGroundWeapon() EXIT false";
}

/**
 * Places an item on an XCom soldier based on equipment layout.
 * @param item, Pointer to the Item.
 * @return, Pointer to the Item.
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
				i = _save->getUnits()->begin();
				i != _save->getUnits()->end();
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
							_save->getItems()->push_back(*k);

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

					_save->getItems()->push_back(item);

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
		_save->getItems()->push_back(item);
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
					&& unit->getStats()->strength * 0.66 >= weight)
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
			if (unit->getStats()->strength >= weight) // weight is always considered 0 for aliens
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
	if (_save->getDepth() > 0
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
				i++)
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
															_save->getCurrentItemId());
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
											_save->getCurrentItemId());
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
												_save->getCurrentItemId());
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
 * @param rules		- pointer to the Unit which holds info about the alien
 * @param alienRank	- rank of the alien, used for spawn point search
 * @param outside	- true if the alien should spawn outside the UFO
 * @return, pointer to the created unit
 */
BattleUnit* BattlescapeGenerator::addAlien(
		Unit* rules,
		int alienRank,
		bool outside)
{
	int difficulty = static_cast<int>(_game->getSavedGame()->getDifficulty());
	int month = _game->getSavedGame()->getMonthsPassed();
	BattleUnit* unit = new BattleUnit(
									rules,
									FACTION_HOSTILE,
									_unitSequence++,
									_rules->getArmor(rules->getArmor()),
									difficulty,
									month); // kL_add.

	// following data is the order in which certain alien ranks spawn on certain node ranks;
	// note that they all can fall back to rank 0 nodes - which is scout (outside ufo)

	Node* node = NULL;

	if (outside)
		node = _save->getSpawnNode( // Civ-Scout spawnpoints <- 'outside'
								0,
								unit);

	if (node == NULL) // ie. if not spawning on a Civ-Scout node
	{
		for (int
				i = 0;
//kL			i < 7
				i < 8 // kL
					&& node == NULL;
				i++)
		{
			node = _save->getSpawnNode(
									Node::nodeRank[alienRank][i],
									unit);
		}
	}

	if (node
		&& _save->setUnitPosition(
								unit,
								node->getPosition()))
	{
		unit->setAIState(new AlienBAIState(
										_game->getSavedGame()->getSavedBattle(),
										unit,
										node));
		unit->setRankInt(alienRank);

		int dir = _save->getTileEngine()->faceWindow(node->getPosition());

		Position craft = _game->getSavedGame()->getSavedBattle()->getUnits()->at(0)->getPosition();
		if (_save->getTileEngine()->distance(
											node->getPosition(),
											craft)
										< 25
			&& RNG::percent(difficulty * 20))
		{
			dir = unit->directionTo(craft);
		}

		if (dir != -1)
			unit->setDirection(dir);
		else
			unit->setDirection(RNG::generate(0, 7));

//kL	if (difficulty == 0) // kL_note: moved to BattleUnit::adjustStats()
//kL		unit->halveArmor();

		// we only add a unit if it has a node to spawn on. (stops them spawning at 0,0,0)
		_save->getUnits()->push_back(unit);
	}
	else
	{
		delete unit;
		unit = NULL;
	}

	return unit;
}

/**
 * Spawns 1-16 civilians on a terror mission.
 * @param civilians, Maximum number of civilians to spawn.
 */
void BattlescapeGenerator::deployCivilians(int civilians)
{
	if (civilians)
	{
		// inevitably someone will point out that ufopaedia says 0-16 civilians.
		// to that person: i looked at the code and it says otherwise.
		// 0 civilians would only be a possibility if there were already 80 units,
		// or no spawn nodes for civilians.
		int rand = RNG::generate(
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
 * @param rules - pointer to the Unit which holds info about the civilian
 * @return, pointer to the created unit
 */
BattleUnit* BattlescapeGenerator::addCivilian(Unit* rules)
{
	BattleUnit* unit = new BattleUnit(
									rules,
									FACTION_NEUTRAL,
									_unitSequence++,
									_rules->getArmor(rules->getArmor()),
									0);

	Node* node = _save->getSpawnNode(0, unit);
	if (node)
	{
		_save->setUnitPosition(unit, node->getPosition());
		unit->setAIState(new CivilianBAIState(
										_game->getSavedGame()->getSavedBattle(),
										unit,
										node));

		unit->setDirection(RNG::generate(0, 7));

		// we only add a unit if it has a node to spawn on. (stops them spawning at 0,0,0)
		_save->getUnits()->push_back(unit);
	}
	else if (placeUnitNearFriend(unit))
	{
		unit->setAIState(new CivilianBAIState(
										_game->getSavedGame()->getSavedBattle(),
										unit,
										node));

		unit->setDirection(RNG::generate(0, 7));

		_save->getUnits()->push_back(unit);
	}
	else
	{
		delete unit;
		unit = NULL;
	}

	return unit;
}

/**
 * Generates a map (set of tiles) for a new battlescape game.
 */
void BattlescapeGenerator::generateMap()
{
	bool placed = false;
	int
		x = 0,
		y = 0,
		blocksToDo = 0,
		ufoX = 0,
		ufoY = 0,
		mapDataSetIDOffset = 0,
		craftDataSetIDOffset = 0;

	std::vector<std::vector<bool> >
		landingzone,
		storageBlocks;
	std::vector<std::vector<int> > segments;
	std::vector<std::vector<MapBlock*> > blocks;

	MapBlock
		* dummy = new MapBlock(
							"dummy",
							0,
							0,
							MT_DEFAULT),
		* craftMap = NULL,
		* ufoMap = NULL;

	_save->setAmbientSound(_terrain->getAmbience());

	blocks.resize(
			_mapsize_x / 10,
			std::vector<MapBlock*>(_mapsize_y / 10));
	landingzone.resize(
			_mapsize_x / 10,
			std::vector<bool>(_mapsize_y / 10,
			false));
	storageBlocks.resize(
			_mapsize_x / 10,
			std::vector<bool>(_mapsize_y / 10, false));
	segments.resize(
			_mapsize_x / 10,
			std::vector<int>(_mapsize_y / 10, 0));

	blocksToDo = (_mapsize_x / 10) * (_mapsize_y / 10);

	/* Determine UFO landingzone (do this first because ufo is generally bigger) */
	if (_ufo != NULL)
	{
		// pick a random ufo mapblock, can have all kinds of sizes
		ufoMap = _ufo->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(999, MT_DEFAULT);

		int
			minX = 0,
			minY = 0,
			maxX = (_mapsize_x / 10) - (ufoMap->getSizeX() / 10),
			maxY = (_mapsize_y / 10) - (ufoMap->getSizeY() / 10);

		if (_terrain->getUfoPositions()->empty() == false)
		{
			std::vector<LandingSite*> possibleSites;
			for (std::vector<LandingSite*>::const_iterator
					i = _terrain->getUfoPositions()->begin();
					i != _terrain->getUfoPositions()->end();
					++i)
			{
				if ((*i)->sizeX >= ufoMap->getSizeX() / 10
					&& (*i)->sizeY >= ufoMap->getSizeY() / 10)
				{
					possibleSites.push_back(*i);
				}
			}

			if (possibleSites.empty())
			{
				throw Exception("no suitable landing site found for a UFO of that size");
			}

			size_t pick = RNG::generate(0, possibleSites.size() - 1);
			LandingSite* landingSite = possibleSites.at(pick);

			minX = landingSite->x;
			minY = landingSite->y;
			maxX = minX + landingSite->sizeX - (ufoMap->getSizeX() / 10);
			maxY = minY + landingSite->sizeY - (ufoMap->getSizeY() / 10);
		}

		ufoX = RNG::generate(minX, maxX);
		ufoY = RNG::generate(minY, maxY);

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
				landingzone[ufoX + i][ufoY + j] = true;
				blocks[ufoX + i][ufoY + j] = _terrain->getRandomMapBlock(10, MT_LANDINGZONE);

				blocksToDo--;
			}
		}
	}

	// DETERMINE UFO LANDINGZONE
	// - do this first because ufo is generally bigger
/*	if (_ufo != NULL)
	{
		// pick a random ufo mapblock, can have all kinds of sizes
		ufoMap = _ufo->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(999, MT_DEFAULT);

		ufoX = RNG::generate(0, (_mapsize_x / 10) - (ufoMap->getSizeX() / 10));
		ufoY = RNG::generate(0, (_mapsize_y / 10) - (ufoMap->getSizeY() / 10));

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
				landingzone[ufoX + i][ufoY + j] = true;
				blocks[ufoX + i][ufoY + j] = _terrain->getRandomMapBlock(10, MT_LANDINGZONE);

				blocksToDo--;
			}
		}
	} */

	// DETERMINE CRAFT LANDINGZONE
	// - alien base assault has no craft landing zone
/*	if (_craft != NULL
		&& (_save->getMissionType() != "STR_ALIEN_BASE_ASSAULT")
		&& (_save->getMissionType() != "STR_MARS_THE_FINAL_ASSAULT"))
	{
		// pick a random craft mapblock, can have all kinds of sizes
		craftMap = _craft->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(999, MT_DEFAULT);

		while (!placed)
		{
			_craftX = RNG::generate(0, (_mapsize_x / 10) - (craftMap->getSizeX() / 10));
			_craftY = RNG::generate(0, (_mapsize_y / 10) - (craftMap->getSizeY() / 10)); */

	/* Determine Craft landingzone */
	/* alien base assault has no craft landing zone */
	if (_craft != NULL
		&& _save->getMissionType() != "STR_ALIEN_BASE_ASSAULT"
		&& _save->getMissionType() != "STR_MARS_THE_FINAL_ASSAULT")
	{
		// pick a random craft mapblock, can have all kinds of sizes
		craftMap = _craft->getRules()->getBattlescapeTerrainData()->getRandomMapBlock(999, MT_DEFAULT);

		while (!placed)
		{
			int
				minX = 0,
				minY = 0,
				maxX = (_mapsize_x / 10) - (craftMap->getSizeX() / 10),
				maxY = (_mapsize_y / 10) - (craftMap->getSizeY() / 10);

			if (_terrain->getCraftPositions()->empty() == false)
			{
				std::vector<LandingSite*> possibleSites;
				for (std::vector<LandingSite*>::const_iterator
					i = _terrain->getCraftPositions()->begin();
					i != _terrain->getCraftPositions()->end();
					++i)
				{
					if ((*i)->sizeX >= craftMap->getSizeX() / 10
						&& (*i)->sizeY >= craftMap->getSizeY() / 10)
					{
						possibleSites.push_back(*i);
					}
				}

				if (possibleSites.empty())
				{
					throw Exception("no suitable landing site found for a craft of that size");
				}

				size_t pick = RNG::generate(0, possibleSites.size() - 1);
				LandingSite* landingSite = possibleSites.at(pick);

				minX = landingSite->x;
				minY = landingSite->y;
				maxX = minX + landingSite->sizeX - (craftMap->getSizeX() / 10);
				maxY = minY + landingSite->sizeY - (craftMap->getSizeY() / 10);
			}

			_craftX = RNG::generate(minX, maxX);
			_craftY = RNG::generate(minY, maxY); // end_New.


			placed = true;

			for (int // check if this place is ok
					i = 0;
					i < craftMap->getSizeX() / 10;
					++i)
			{
				for (int
						j = 0;
						j < craftMap->getSizeY() / 10;
						++j)
				{
					if (landingzone[_craftX + i][_craftY + j])
						placed = false; // whoops the ufo is already here, try again
				}
			}

			if (placed) // if ok, allocate it
			{
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
						landingzone[_craftX + i][_craftY + j] = true;
						blocks[_craftX + i][_craftY + j] = _terrain->getRandomMapBlock(10, MT_LANDINGZONE);

						blocksToDo--;
					}
				}
			}
		}
	}

	// DETERMINE POSITION OF URBAN TERRAIN ROADS
	if (_save->getMissionType() == "STR_TERROR_MISSION")
	{
		int roadStyle = RNG::generate(0, 99);
		std::vector<int> roadChances = _terrain->getRoadTypeOdds();
		bool EWRoad = roadStyle < roadChances.at(0);
		bool NSRoad = !EWRoad && roadStyle < roadChances.at(0) + roadChances.at(1);
		bool twoRoads = !EWRoad && !NSRoad;

		int // make sure the road(s) are not crossing the craft landing site
			roadX = _craftX,
			roadY = _craftY;
		while ((roadX >= _craftX
				&& roadX < _craftX + (craftMap->getSizeX() / 10))
			|| (roadY >= _craftY
				&& roadY < _craftY + (craftMap->getSizeY() / 10)))
		{
			roadX = RNG::generate(0, (_mapsize_x / 10) - 1);
			roadY = RNG::generate(0, (_mapsize_y / 10) - 1);
		}

		if (twoRoads)
		{
			// put a crossing on the X,Y position and fill the rest with roads
			blocks[roadX][roadY] = _terrain->getRandomMapBlock(10, MT_CROSSING);
			blocksToDo--;

			EWRoad = true;
			NSRoad = true;
		}

		if (EWRoad)
		{
			while (x < _mapsize_x / 10)
			{
				if (blocks[x][roadY] == 0)
				{
					blocks[x][roadY] = _terrain->getRandomMapBlock(10, MT_EWROAD);

					blocksToDo--;
				}

				x++;
			}
		}

		if (NSRoad)
		{
			while (y < _mapsize_y / 10)
			{
				if (blocks[roadX][y] == 0)
				{
					blocks[roadX][y] = _terrain->getRandomMapBlock(10, MT_NSROAD);

					blocksToDo--;
				}

				y++;
			}
		}
	}

	// DETERMINE POSITION OF BASE MODULES
	else if (_save->getMissionType() == "STR_BASE_DEFENSE")
	{
		for (std::vector<BaseFacility*>::const_iterator
				i = _base->getFacilities()->begin();
				i != _base->getFacilities()->end();
				++i)
		{
			if ((*i)->getBuildTime() == 0)
			{
				int num = 0;

				for (int
						y = (*i)->getY();
						y < (*i)->getY() + (*i)->getRules()->getSize();
						++y)
				{
					for (int
							x = (*i)->getX();
							x < (*i)->getX() + (*i)->getRules()->getSize();
							++x)
					{
						// lots of crazy stuff here, which is for the hangars (or other large base facilities one may create)
						std::string mapname = (*i)->getRules()->getMapName();
						std::ostringstream newname;
						newname << mapname.substr(0, mapname.size() - 2); // strip off last 2 digits

						int mapnum = atoi(mapname.substr(mapname.size() - 2, 2).c_str()); // get number
						mapnum += num;

						if (mapnum < 10) newname << 0;
						newname << mapnum;

						blocks[x][y] = _terrain->getMapBlock(newname.str());
						storageBlocks[x][y] = ((*i)->getRules()->getStorage() > 0);

						num++;
					}
				}
			}
		}

		for (int // fill with dirt
				i = 0;
				i < (_mapsize_x / 10);
				++i)
		{
			for (int
					j = 0;
					j < (_mapsize_y / 10);
					++j)
			{
				if (blocks[i][j] == 0)
					blocks[i][j] = _terrain->getRandomMapBlock(
															10,
															MT_DIRT);
			}
		}

		blocksToDo = 0;
	}

	// DETERMINE POSITION OF BASE MODULES
	else if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
		|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
	{
		int randX = RNG::generate(0, (_mapsize_x / 10) - 2);
		int randY = RNG::generate(0, (_mapsize_y / 10) - 2);

		blocks[randX][randY] = _terrain->getRandomMapBlock( // add the command center
														20,
														(_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")? MT_FINALCOMM: MT_UBASECOMM);
		blocksToDo--;

		blocks[randX + 1][randY] = dummy; // mark mapblocks as used
		blocksToDo--;

		blocks[randX + 1][randY + 1] = dummy;
		blocksToDo--;

		blocks[randX][randY + 1] = dummy;
		blocksToDo--;

		for (int // add two lifts (not on top of the command center)
				i = 0;
				i < 2;
				i++)
		{
			while (blocks[randX][randY] != NULL)
			{
				randX = RNG::generate(0, (_mapsize_x / 10) - 1);
				randY = RNG::generate(0, (_mapsize_y / 10) - 1);
			}

			blocks[randX][randY] = _terrain->getRandomMapBlock(10, MT_XCOMSPAWN); // add the lift

			blocksToDo--;
		}
	}
	else if (_save->getMissionType() == "STR_MARS_CYDONIA_LANDING")
	{
		int randX = RNG::generate(0, (_mapsize_x / 10) - 2);
		int randY = RNG::generate(0, (_mapsize_y / 10) - 2);

		while (blocks[randX][randY] != NULL // add one lift
			|| landingzone[randX][randY])
		{
			randX = RNG::generate(0, (_mapsize_x / 10) - 1);
			randY = RNG::generate(0, (_mapsize_y / 10) - 1);
		}

		blocks[randX][randY] = _terrain->getRandomMapBlock(10, MT_XCOMSPAWN); // add the lift

		blocksToDo--;
	}

	x = 0;
	y = 0;

	int maxLarge = _terrain->getLargeBlockLimit();
	int curLarge = 0;

	int tries = 0;
	while (curLarge != maxLarge
		&& tries <= 50)
	{
		int randX = RNG::generate(0, (_mapsize_x / 10) - 2);
		int randY = RNG::generate(0, (_mapsize_y / 10) - 2);
		if (!blocks[randX][randY]
			&& !blocks[randX + 1][randY]
			&& !blocks[randX + 1][randY + 1]
			&& !blocks[randX][randY + 1]
			&& !landingzone[randX][randY]
			&& !landingzone[randX + 1][randY]
			&& !landingzone[randX][randY + 1]
			&& !landingzone[randX + 1][randY + 1])
		{
			blocks[randX][randY] = _terrain->getRandomMapBlock(
															20,
															MT_DEFAULT,
															true);
			blocksToDo--;

			blocks[randX + 1][randY] = dummy; // mark mapblocks as used
			blocksToDo--;

			blocks[randX + 1][randY + 1] = dummy;
			blocksToDo--;

			blocks[randX][randY + 1] = dummy;
			blocksToDo--;

			curLarge++;
		}

		tries++;
	}

	// RANDOM MAP GENERATION - for crashed or landed sites
	while (blocksToDo)
	{
		if (blocks[x][y] == 0)
		{
			if ((_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
					|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
				&& RNG::generate(0, 99) > 59)
			{
				blocks[x][y] = _terrain->getRandomMapBlock(10, MT_CROSSING);
			}
			else
				blocks[x][y] = _terrain->getRandomMapBlock(10, landingzone[x][y]? MT_LANDINGZONE: MT_DEFAULT);

			blocksToDo--;
			x++;
		}
		else
			x++;

		if (x >= _mapsize_x / 10) // reach the end
		{
			x = 0;
			y++;
		}
	}

	_terrain->resetMapBlocks(); // reset the "times used" fields.

	for (std::vector<MapDataSet*>::iterator
			i = _terrain->getMapDataSets()->begin();
			i != _terrain->getMapDataSets()->end();
			++i)
	{
		(*i)->loadData();

		if (_rules->getMCDPatch((*i)->getName()))
			_rules->getMCDPatch((*i)->getName())->modifyData(*i);

		_save->getMapDataSets()->push_back(*i);

		mapDataSetIDOffset++;
	}

	// LOAD it all up.
	int segment = 0;
	for (int
			itY = 0;
			itY < _mapsize_y / 10;
			itY++)
	{
		for (int
				itX = 0;
				itX < _mapsize_x / 10;
				itX++)
		{
			segments[itX][itY] = segment;
			if (blocks[itX][itY] != 0
				&& blocks[itX][itY] != dummy)
			{
				loadMAP(
					blocks[itX][itY],
					itX * 10,
					itY * 10,
					_terrain,
					0);

				if (!landingzone[itX][itY])
				{
					loadRMP(
						blocks[itX][itY],
						itX * 10,
						itY * 10,
						segment++);
				}
			}
		}
	}

	if (_save->getMissionType() == "STR_BASE_DEFENSE" // make passages between blocks in a base map
		|| _save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
		|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
	{
		int ewallfix = 14;
		int swallfix = 13;
		int ewallfixSet = 1;
		int swallfixSet = 1;

		if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
			|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
		{
			ewallfix = 17;	// north wall
			swallfix = 18;	// west wall
			ewallfixSet = 2;
			swallfixSet = 2;
		}

		MapDataSet* mds = _terrain->getMapDataSets()->at(ewallfixSet);
		MapBlock* dirt = _terrain->getRandomMapBlock(10, MT_DIRT);

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
				if (blocks[i][j] == dirt)
					continue;

				if (storageBlocks[i][j]) // general stores - there is where the items are put
				{
					for (int
							k = i * 10;
							k != (i + 1) * 10;
							++k)
					{
						for (int
								l = j * 10;
								l != (j + 1) * 10;
								++l)
						{
							if ((k + l) %2 == 0) // we only want every other tile, giving us a "checkerboard" pattern
							{
								Tile* tile = _save->getTile(Position(k, l, 1));
								Tile* tileEast = _save->getTile(Position(k + 1, l, 1));
								Tile* tileSouth = _save->getTile(Position(k, l + 1, 1));
								if (tile
									&& tile->getMapData(MapData::O_FLOOR)
									&& !tile->getMapData(MapData::O_OBJECT)
									&& tileEast
									&& !tileEast->getMapData(MapData::O_WESTWALL)
									&& tileSouth
									&& !tileSouth->getMapData(MapData::O_NORTHWALL))
								{
									_save->getStorageSpace().push_back(Position(k, l, 1));
								}
							}
						}
					}

					_tileCraft = _save->getTile(Position( // let's put the inventory tile on the lower floor, just to be safe.
														(i * 10) + 5,
														(j * 10) + 5,
														0));
					_save->setBattleInventory(_tileCraft);
				}

				if (i < _mapsize_x / 10 - 1 // drill east
					&& blocks[i + 1][j] != dirt
					&& _save->getTile(Position((i*10)+9,(j*10)+4,0))->getMapData(MapData::O_OBJECT)
					&& (!_save->getTile(Position((i*10)+8,(j*10)+4,0))->getMapData(MapData::O_OBJECT)
						|| (_save->getTile(Position((i*10)+9,(j*10)+4,0))->getMapData(MapData::O_OBJECT)
							!= _save->getTile(Position((i*10)+8,(j*10)+4,0))->getMapData(MapData::O_OBJECT))))
				{
					_save->getTile(Position((i*10)+9,(j*10)+3,0))->setMapData(0,-1,-1, MapData::O_WESTWALL); // remove stuff
					_save->getTile(Position((i*10)+9,(j*10)+3,0))->setMapData(0,-1,-1, MapData::O_OBJECT);
					_save->getTile(Position((i*10)+9,(j*10)+4,0))->setMapData(0,-1,-1, MapData::O_WESTWALL);
					_save->getTile(Position((i*10)+9,(j*10)+4,0))->setMapData(0,-1,-1, MapData::O_OBJECT);
					_save->getTile(Position((i*10)+9,(j*10)+5,0))->setMapData(0,-1,-1, MapData::O_WESTWALL);
					_save->getTile(Position((i*10)+9,(j*10)+5,0))->setMapData(0,-1,-1, MapData::O_OBJECT);

					if (_save->getTile(Position((i*10)+9,(j*10)+2,0))->getMapData(MapData::O_OBJECT)) // wallfix
					{
						_save->getTile(Position((i*10)+9,(j*10)+3,0))->setMapData(
																				mds->getObjects()->at(ewallfix),
																				ewallfix,
																				ewallfixSet,
																				MapData::O_NORTHWALL);
						_save->getTile(Position((i*10)+9,(j*10)+6,0))->setMapData(
																				mds->getObjects()->at(ewallfix),
																				ewallfix,
																				ewallfixSet,
																				MapData::O_NORTHWALL);
					}

					if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
						|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
					{
						if (!_save->getTile(Position((i*10)+10,(j*10)+3,0))->getMapData(MapData::O_NORTHWALL)) // wallcornerfix
						{
							_save->getTile(Position(((i+1)*10),(j*10)+3,0))->setMapData(
																					mds->getObjects()->at(swallfix+1),
																					swallfix+1,
																					swallfixSet,
																					MapData::O_OBJECT);
						}

						_save->getTile(Position((i*10)+9,(j*10)+3,0))->setMapData( // floorfix
																			_terrain->getMapDataSets()->at(1)->getObjects()->at(63),
																			63,
																			1,
																			MapData::O_FLOOR);
						_save->getTile(Position((i*10)+9,(j*10)+4,0))->setMapData(
																			_terrain->getMapDataSets()->at(1)->getObjects()->at(63),
																			63,
																			1,
																			MapData::O_FLOOR);
						_save->getTile(Position((i*10)+9,(j*10)+5,0))->setMapData(
																			_terrain->getMapDataSets()->at(1)->getObjects()->at(63),
																			63, 1,
																			MapData::O_FLOOR);
					}

					_save->getTile(Position(((i+1)*10),(j*10)+3,0))->setMapData(0,-1,-1, MapData::O_WESTWALL); // remove more stuff
					_save->getTile(Position(((i+1)*10),(j*10)+4,0))->setMapData(0,-1,-1, MapData::O_WESTWALL);
					_save->getTile(Position(((i+1)*10),(j*10)+5,0))->setMapData(0,-1,-1, MapData::O_WESTWALL);
				}

				if (j < _mapsize_y / 10 - 1 // drill south
					&& blocks[i][j + 1] != dirt
					&& _save->getTile(Position((i*10)+4,(j*10)+9,0))->getMapData(MapData::O_OBJECT)
					&& (!_save->getTile(Position((i*10)+4,(j*10)+8,0))->getMapData(MapData::O_OBJECT)
					|| (_save->getTile(Position((i*10)+4,(j*10)+9,0))->getMapData(MapData::O_OBJECT)
						!= _save->getTile(Position((i*10)+4,(j*10)+8,0))->getMapData(MapData::O_OBJECT))))
				{
					_save->getTile(Position((i*10)+3,(j*10)+9,0))->setMapData(0,-1,-1, MapData::O_NORTHWALL); // remove stuff
					_save->getTile(Position((i*10)+3,(j*10)+9,0))->setMapData(0,-1,-1, MapData::O_OBJECT);
					_save->getTile(Position((i*10)+4,(j*10)+9,0))->setMapData(0,-1,-1, MapData::O_NORTHWALL);
					_save->getTile(Position((i*10)+4,(j*10)+9,0))->setMapData(0,-1,-1, MapData::O_OBJECT);
					_save->getTile(Position((i*10)+5,(j*10)+9,0))->setMapData(0,-1,-1, MapData::O_NORTHWALL);
					_save->getTile(Position((i*10)+5,(j*10)+9,0))->setMapData(0,-1,-1, MapData::O_OBJECT);

					if (_save->getTile(Position((i*10)+2,(j*10)+9,0))->getMapData(MapData::O_OBJECT)) // wallfix
					{
						_save->getTile(Position((i*10)+3,(j*10)+9,0))->setMapData(
																				mds->getObjects()->at(swallfix),
																				swallfix,
																				swallfixSet,
																				MapData::O_WESTWALL);
						_save->getTile(Position((i*10)+6,(j*10)+9,0))->setMapData(
																				mds->getObjects()->at(swallfix),
																				swallfix,
																				swallfixSet,
																				MapData::O_WESTWALL);
					}

					if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
						|| _save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
					{
						if (!_save->getTile(Position((i*10)+3,(j*10)+10,0))->getMapData(MapData::O_WESTWALL)) // wallcornerfix
						{
							_save->getTile(Position((i*10)+3,((j+1)*10),0))->setMapData(
																					mds->getObjects()->at(swallfix+1),
																					swallfix+1,
																					swallfixSet,
																					MapData::O_OBJECT);
						}

						_save->getTile(Position((i*10)+3,(j*10)+9,0))->setMapData( // floorfix
																			_terrain->getMapDataSets()->at(1)->getObjects()->at(63),
																			63,
																			1,
																			MapData::O_FLOOR);
						_save->getTile(Position((i*10)+4,(j*10)+9,0))->setMapData(
																			_terrain->getMapDataSets()->at(1)->getObjects()->at(63),
																			63,
																			1,
																			MapData::O_FLOOR);
						_save->getTile(Position((i*10)+5,(j*10)+9,0))->setMapData(
																			_terrain->getMapDataSets()->at(1)->getObjects()->at(63),
																			63,
																			1,
																			MapData::O_FLOOR);
					}

					_save->getTile(Position((i*10)+3,(j+1)*10,0))->setMapData(0,-1,-1, MapData::O_NORTHWALL); // remove more stuff
					_save->getTile(Position((i*10)+4,(j+1)*10,0))->setMapData(0,-1,-1, MapData::O_NORTHWALL);
					_save->getTile(Position((i*10)+5,(j+1)*10,0))->setMapData(0,-1,-1, MapData::O_NORTHWALL);
				}
			}
		}
	}


	if (_ufo != NULL)
	{
		for (std::vector<MapDataSet*>::iterator
				i = _ufo->getRules()->getBattlescapeTerrainData()->getMapDataSets()->begin();
				i != _ufo->getRules()->getBattlescapeTerrainData()->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getName()))
				_rules->getMCDPatch((*i)->getName())->modifyData(*i);

			_save->getMapDataSets()->push_back(*i);

			craftDataSetIDOffset++;
		}

		loadMAP(
			ufoMap,
			ufoX * 10,
			ufoY * 10,
			_ufo->getRules()->getBattlescapeTerrainData(),
			mapDataSetIDOffset);
		loadRMP(
			ufoMap,
			ufoX * 10,
			ufoY * 10,
			Node::UFOSEGMENT);

		for (int
				i = 0;
				i < ufoMap->getSizeX() / 10;
				++i)
		{
			for (int
					j = 0;
					j < ufoMap->getSizeY() / 10;
					j++)
			{
				segments[ufoX + i][ufoY + j] = Node::UFOSEGMENT;
			}
		}
	}

	if (_craft != NULL
		&& _save->getMissionType() != "STR_ALIEN_BASE_ASSAULT"
		&& _save->getMissionType() != "STR_MARS_THE_FINAL_ASSAULT")
	{
		for (std::vector<MapDataSet*>::iterator
				i = _craft->getRules()->getBattlescapeTerrainData()->getMapDataSets()->begin();
				i != _craft->getRules()->getBattlescapeTerrainData()->getMapDataSets()->end();
				++i)
		{
			(*i)->loadData();

			if (_rules->getMCDPatch((*i)->getName()))
				_rules->getMCDPatch((*i)->getName())->modifyData(*i);

			_save->getMapDataSets()->push_back(*i);
		}

		loadMAP(
			craftMap,
			_craftX * 10,
			_craftY * 10,
			_craft->getRules()->getBattlescapeTerrainData(),
			mapDataSetIDOffset + craftDataSetIDOffset,
			false, // was true, let calcFOV() do the job.
			true);

		loadRMP(
			craftMap,
			_craftX * 10,
			_craftY * 10,
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
				segments[_craftX + i][_craftY + j] = Node::CRAFTSEGMENT;
			}
		}
	}

	for (std::vector<Node*>::iterator // attach nodelinks to each other
			i = _save->getNodes()->begin();
			i != _save->getNodes()->end();
			++i)
	{
		Node* node = *i;

		int segmentX = node->getPosition().x / 10;
		int segmentY = node->getPosition().y / 10;
		int neighbourSegments[4];
		int neighbourDirections[4] = {-2, -3, -4, -5};
		int neighbourDirectionsInverted[4] = {-4, -5, -2, -3};

		if (segmentX == _mapsize_x / 10 - 1)
			neighbourSegments[0] = -1;
		else
			neighbourSegments[0] = segments[segmentX + 1][segmentY];

		if (segmentY == _mapsize_y / 10 - 1)
			neighbourSegments[1] = -1;
		else
			neighbourSegments[1] = segments[segmentX][segmentY + 1];

		if (segmentX == 0)
			neighbourSegments[2] = -1;
		else
			neighbourSegments[2] = segments[segmentX - 1][segmentY];

		if (segmentY == 0)
			neighbourSegments[3] = -1;
		else
			neighbourSegments[3] = segments[segmentX][segmentY - 1];

		for (std::vector<int>::iterator
				j = node->getNodeLinks()->begin();
				j != node->getNodeLinks()->end();
				++j )
		{
			for (int
					n = 0;
					n < 4;
					n++)
			{
				if (*j == neighbourDirections[n])
				{
					for (std::vector<Node*>::iterator
							k = _save->getNodes()->begin();
							k != _save->getNodes()->end();
							++k)
					{
						if ((*k)->getSegment() == neighbourSegments[n])
						{
							for (std::vector<int>::iterator
									l = (*k)->getNodeLinks()->begin();
									l != (*k)->getNodeLinks()->end();
									++l)
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

	delete dummy;
}

/**
 * Loads an XCom format MAP file into the tiles of the battlegame.
 * @param mapblock		- pointer to MapBlock
 * @param xoff			- Mapblock offset in X direction
 * @param yoff			- Mapblock offset in Y direction
 * @param terrain		- pointer to the Terrain rule
 * @param discovered	- whether or not this mapblock is discovered (eg. landingsite of the XCom plane)
 * @param craft			- true if Craft
 * @return, height of the loaded mapblock (this is needed for spawnpoint calculation...
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 * @note Y-axis is in reverse order.
 */
int BattlescapeGenerator::loadMAP(
		MapBlock* mapblock,
		int xoff,
		int yoff,
		RuleTerrain* terrain,
		int mapDataSetOffset,
		bool discovered,
		bool craft)
{
	int
		sizex,
		sizey,
		sizez,
		x = xoff,
		y = yoff,
		z = 0;

	char size[3];
	unsigned char value[4];

	std::ostringstream filename;
	filename << "MAPS/" << mapblock->getName() << ".MAP";

	unsigned int terrainObjectID;

	std::ifstream mapFile( // Load file
						CrossPlatform::getDataFile(filename.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename.str() + " not found");
	}

	mapFile.read((char*)& size, sizeof(size));
	sizey = static_cast<int>(size[0]);
	sizex = static_cast<int>(size[1]);
	sizez = static_cast<int>(size[2]);

	if (sizez > _save->getMapSizeZ())
	{
		throw Exception("Height of map too big for this mission");
	}

	z += sizez - 1;
	mapblock->setSizeZ(sizez);

	for (int
			i = _mapsize_z - 1;
			i > 0;
			i--)
	{
		// check if there is already a layer - if so, we have to move Z up
		MapData* floor = _save->getTile(Position(x, y, i))->getMapData(MapData::O_FLOOR);
		if (floor != NULL)
		{
			z += i;

			if (craft)
				_craftZ = i;

			break;
		}
	}

	if (z > _save->getMapSizeZ() - 1)
	{
		throw Exception("Something is wrong in your map definitions");
	}

	while (mapFile.read((char*)& value, sizeof(value)))
	{
		for (int
				part = 0;
				part < 4;
				part++)
		{
			terrainObjectID = static_cast<unsigned char>(value[part]);
			if (terrainObjectID > 0)
			{
				int mapDataSetID = mapDataSetOffset;
				unsigned int mapDataID = terrainObjectID;
				MapData* md = terrain->getMapData(
											&mapDataID,
											&mapDataSetID);

				_save->getTile(Position(x, y, z))->setMapData(
															md,
															mapDataID,
															mapDataSetID,
															part);
			}

			// if the part is empty and it's not a floor, remove it;
			// this prevents growing grass in UFOs.
			if (terrainObjectID == 0
				&& part > 0)
			{
				_save->getTile(Position(x, y, z))->setMapData(0,-1,-1, part);
			}
				// kL_note: might want to change this to how it was in UFO:orig.
		}

		if (craft
			&& _craftZ == z)
		{
			for (int
					z2 = _save->getMapSizeZ() - 1;
					z2 >= _craftZ;
					--z2)
			{
				_save->getTile(Position(x, y, z2))->setDiscovered(true, 2);
			}
		}

		_save->getTile(Position(x, y, z))->setDiscovered(discovered, 2);

		x++;

		if (x == sizex + xoff)
		{
			x = xoff;
			y++;
		}

		if (y == sizey + yoff)
		{
			y = yoff;
			z--;
		}
	}

	if (!mapFile.eof())
	{
		throw Exception("Invalid MAP file");
	}

	mapFile.close();

	if (_generateFuel)
	{
		// if one of the mapBlocks has an items array defined, don't deploy fuel algorithmically
		_generateFuel = mapblock->getItems()->empty();
	}

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
											_save->getCurrentItemId());
			_save->getItems()->push_back(item);
			_save->getTile((*j) + Position(xoff, yoff, 0))->addItem(
																item,
																_rules->getInventory("STR_GROUND"));
		}
	}

	return sizez;
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

	size_t nodeOffset = _save->getNodes()->size();
	int
		pos_x,
		pos_y,
		pos_z,
		type,
		rank,
		flags,
		reserved,
		priority;

	while (mapFile.read((char*)& value, sizeof(value)))
	{
		pos_x = static_cast<int>(value[1]);
		pos_y = static_cast<int>(value[0]);
		pos_z = static_cast<int>(value[2]);

		if (pos_x < mapblock->getSizeX()
			&& pos_y < mapblock->getSizeY()
			&& pos_z < _mapsize_z )
		{
			Position pos = Position(
								xoff + pos_x,
								yoff + pos_y,
								mapblock->getSizeZ() - pos_z - 1);

			type		= value[19];
			rank		= value[20];
			flags		= value[21];
			reserved	= value[22];
			priority	= value[23];

			Node* node = new Node(
								_save->getNodes()->size(),
								pos,
								segment,
								type,
								rank,
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

			_save->getNodes()->push_back(node);
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
			i < _save->getMapSizeXYZ();
			++i)
	{
		if (_save->getTiles()[i]->getMapData(MapData::O_OBJECT)
			&& _save->getTiles()[i]->getMapData(MapData::O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE)
		{
			BattleItem* alienFuel = new BattleItem(
											_rules->getItem(_rules->getAlienFuel()),
											_save->getCurrentItemId());

			_save->getItems()->push_back(alienFuel);
			_save->getTiles()[i]->addItem(
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
	//Log(LOG_INFO) << "BattlescapeGenerator::explodePowerSources()";
	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		if (_save->getTiles()[i]->getMapData(MapData::O_OBJECT)
			&& _save->getTiles()[i]->getMapData(MapData::O_OBJECT)->getSpecialType() == UFO_POWER_SOURCE
			&& RNG::percent(80))
		{
			Position pos;
			pos.x = _save->getTiles()[i]->getPosition().x * 16;
			pos.y = _save->getTiles()[i]->getPosition().y * 16;
			pos.z = _save->getTiles()[i]->getPosition().z * 24 + 12;

			int crashPower = _ufo->getCrashPower(); // ( range: 50+ to 100- )
			//Log(LOG_INFO) << ". crashPower = " << crashPower;
			if (RNG::percent(crashPower / 2)) // chance for full range Explosion (even if crash took low damage)
			{
				crashPower = RNG::generate(1, 100);
				//Log(LOG_INFO) << ". . Alt crashPower = " << crashPower;
			}

			double explForce = RNG::generate(0.1, 2.0) * static_cast<double>(crashPower);
			//Log(LOG_INFO) << ". explForce = " << (int)explForce;

//			int power = static_cast<int>(((pow(explForce, 3)) / 32000.0) + (static_cast<double>(crashPower) - 50.0));
			int power = static_cast<int>(((pow(explForce, 2)) / 160.0) + (static_cast<double>(crashPower)));
			//Log(LOG_INFO) << ". power = " << power;

			_save->getTileEngine()->explode(
										pos,
										power,
										DT_HE,
										21);
		}
	}

	Tile* tile = _save->getTileEngine()->checkForTerrainExplosions();
	while (tile)
	{
		Position pos = Position(
							tile->getPosition().x * 16,
							tile->getPosition().y * 16,
							tile->getPosition().z * 24);
		pos += Position(8, 8, 0);
		_save->getTileEngine()->explode(
									pos,
									tile->getExplosive(),
									DT_HE,
									tile->getExplosive() / 10);
		tile = _save->getTileEngine()->checkForTerrainExplosions();
	}
}

/**
 * Places a unit near a friendly unit.
 * @param unit - pointer to the BattleUnit in question
 * @return, true if the unit was successfully placed
 */
bool BattlescapeGenerator::placeUnitNearFriend(BattleUnit* unit)
{
	Position entryPoint = Position(-1,-1,-1);
//kL	int tries = 100;
	int tries = 3; // kL: these guys were getting too cramped up.
	while (entryPoint == Position(-1,-1,-1)
		&& tries > 0)
	{
		BattleUnit* bu = _save->getUnits()->at(RNG::generate(
														0,
														static_cast<int>(_save->getUnits()->size()) - 1));
		if (bu->getFaction() == unit->getFaction()
			&& bu->getPosition() != Position(-1,-1,-1)
			&& bu->getArmor()->getSize() == 1)
		{
			entryPoint = bu->getPosition();
		}

		tries--;
	}

	if (tries
		&& _save->placeUnitNearPosition(
									unit,
									entryPoint))
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
//	RuleTerrain* ret = NULL; // test
	RuleTerrain* terrain = NULL;

	const std::vector<std::string>& terrains = _rules->getTerrainList();
	for (std::vector<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end();
			++i)
	{
		//Log(LOG_INFO) << "Gen:getTerrain() terrain = " << (*i);
		terrain = _rules->getTerrain(*i);

		for (std::vector<int>::iterator
				j = terrain->getTextures()->begin();
				j != terrain->getTextures()->end();
				++j)
		{
			//Log(LOG_INFO) << "Gen:getTerrain() texture = " << (*j);
			if (*j == tex
				&& (terrain->getHemisphere() == 0
					|| (terrain->getHemisphere() < 0
						&& lat < 0.0)
					|| (terrain->getHemisphere() > 0
						&& lat >= 0.0)))
			{
				//Log(LOG_INFO) << "Gen:getTerrain() return = " << (*i);
				return terrain;
//				ret = terrain;	// test
//				break;			// test
			}
		}
	}

	assert(0 && "No matching terrain for globe texture");

	return terrain;
//	return ret; // test
}

/**
 * Creates a mini-battle-save for managing inventory from the Geoscape's CraftEquip or BaseEquip screen.
 * Kids, don't try this at home! yer tellin' me.
 * @param craft	- pointer to craft to handle
 * @param base	- pointer to base to handle (default NULL)
 */
void BattlescapeGenerator::runInventory(
		Craft* craft,
		Base* base)
{
	//Log(LOG_INFO) << "BattlescapeGenerator::runInventory()";
	_baseEquipScreen = true;

	int soldiers = 0;
	if (craft)
		soldiers = craft->getNumSoldiers();
	else
		soldiers = base->getAvailableSoldiers(true);

	// we need to fake a map for soldier placement
	_mapsize_x = soldiers;
	_mapsize_y = 1;
	_mapsize_z = 1;

	_save->initMap(
				_mapsize_x,
				_mapsize_y,
				_mapsize_z);

	MapDataSet* set = new MapDataSet(
									"dummy",
									_game); // kL_add
	MapData* data = new MapData(set);

	for (int
			i = 0;
			i < soldiers;
			++i)
	{
		Tile* tile = _save->getTiles()[i];
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

	if (craft)
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
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			)
	{
		if ((*i)->getGeoscapeSoldier())
		{
			if (_save->getSelectedUnit() == 0)
				_save->setSelectedUnit(*i);

			++i;
		}
		else
		{
			if (_save->getSelectedUnit() == *i)
				_save->setSelectedUnit(0);

			delete *i;
			i = _save->getUnits()->erase(i);
		}
	} */ // kL, they took this out.

	delete data;
	delete set;
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
					_save->getItems()->push_back(*j);
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

}
