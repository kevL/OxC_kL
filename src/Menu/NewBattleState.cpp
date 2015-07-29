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

#include "NewBattleState.h"

//#include <algorithm>
//#include <cmath>
//#include <fstream>
//#include <yaml-cpp/yaml.h>

#include "../Basescape/CraftInfoState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Frame.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the New Battle window.
 */
NewBattleState::NewBattleState()
	:
		_craft(NULL)
{
	_rules = _game->getRuleset();

	_window				= new Window(this, 320, 200, 0,0, POPUP_BOTH);
	_txtTitle			= new Text(320, 17, 0, 9);

	_txtMapOptions		= new Text(148, 9, 8, 68);
	_frameLeft			= new Frame(148, 96, 8, 78);
	_txtAlienOptions	= new Text(148, 9, 164, 68);
	_frameRight			= new Frame(148, 96, 164, 78);

	_txtMission			= new Text(100, 9, 8, 30);
	_cbxMission			= new ComboBox(this, 214, 16, 98, 26);

	_txtCraft			= new Text(100, 9, 8, 50);
	_cbxCraft			= new ComboBox(this, 106, 16, 98, 46);
	_btnEquip			= new TextButton(106, 16, 206, 46);

	_txtDarkness		= new Text(120, 9, 22, 83);
	_slrDarkness		= new Slider(120, 16, 22, 93);

	_txtTerrain			= new Text(120, 9, 22, 113);
	_cbxTerrain			= new ComboBox(this, 120, 16, 22, 123);

//	_txtDepth			= new Text(120, 9, 22, 143);
//	_slrDepth			= new Slider(120, 16, 22, 153);

	_txtDifficulty		= new Text(120, 9, 178, 83);
	_cbxDifficulty		= new ComboBox(this, 120, 16, 178, 93);

	_txtAlienRace		= new Text(120, 9, 178, 113);
	_cbxAlienRace		= new ComboBox(this, 120, 16, 178, 123);

	_txtAlienTech		= new Text(120, 9, 178, 143);
	_slrAlienTech		= new Slider(120, 16, 178, 153);

	_btnCancel			= new TextButton(100, 16,   8, 176);
	_btnRandom			= new TextButton(100, 16, 110, 176);
	_btnOk				= new TextButton(100, 16, 212, 176);

	setInterface("newBattleMenu");

	add(_window,			"window",	"newBattleMenu");
	add(_txtTitle,			"heading",	"newBattleMenu");
	add(_txtMapOptions,		"heading",	"newBattleMenu");
	add(_frameLeft,			"frames",	"newBattleMenu");
	add(_txtAlienOptions,	"heading",	"newBattleMenu");
	add(_frameRight,		"frames",	"newBattleMenu");

	add(_txtMission,		"text",		"newBattleMenu");
	add(_txtCraft,			"text",		"newBattleMenu");
	add(_btnEquip,			"button1",	"newBattleMenu");

	add(_txtDarkness,		"text",		"newBattleMenu");
	add(_slrDarkness,		"button1",	"newBattleMenu");
//	add(_txtDepth,			"text",		"newBattleMenu");
//	add(_slrDepth,			"button1",	"newBattleMenu");
	add(_txtTerrain,		"text",		"newBattleMenu");
	add(_txtDifficulty,		"text",		"newBattleMenu");
	add(_txtAlienRace,		"text",		"newBattleMenu");
	add(_txtAlienTech,		"text",		"newBattleMenu");
	add(_slrAlienTech,		"button1",	"newBattleMenu");

	add(_btnOk,				"button2",	"newBattleMenu");
	add(_btnCancel,			"button2",	"newBattleMenu");
	add(_btnRandom,			"button2",	"newBattleMenu");

	add(_cbxTerrain,		"button1",	"newBattleMenu");
	add(_cbxAlienRace,		"button1",	"newBattleMenu");
	add(_cbxDifficulty,		"button1",	"newBattleMenu");
	add(_cbxCraft,			"button1",	"newBattleMenu");
	add(_cbxMission,		"button1",	"newBattleMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setText(tr("STR_MISSION_GENERATOR"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_frameLeft->setThickness(3);
	_frameRight->setThickness(3);

	_txtMapOptions->setText(tr("STR_MAP_OPTIONS"));
	_txtAlienOptions->setText(tr("STR_ALIEN_OPTIONS"));
	_txtMission->setText(tr("STR_MISSION"));
	_txtCraft->setText(tr("STR_CRAFT"));
	_txtDarkness->setText(tr("STR_MAP_DARKNESS"));
//	_txtDepth->setText(tr("STR_MAP_DEPTH"));
	_txtTerrain->setText(tr("STR_MAP_TERRAIN"));
	_txtDifficulty->setText(tr("STR_ALIEN_DIFFICULTY"));
	_txtAlienRace->setText(tr("STR_ALIEN_RACE"));
	_txtAlienTech->setText(tr("STR_ALIEN_TECH_LEVEL"));

	_missionTypes = _rules->getDeploymentsList();
	_cbxMission->setOptions(_missionTypes);
	_cbxMission->onChange((ActionHandler)& NewBattleState::cbxMissionChange);

	const std::vector<std::string>& craftsList = _rules->getCraftsList();
	for (std::vector<std::string>::const_iterator
			i = craftsList.begin();
			i != craftsList.end();
			++i)
	{
		if (_rules->getCraft(*i)->getSoldiers() > 0)
			_crafts.push_back(*i);
	}
	_cbxCraft->setOptions(_crafts);
	_cbxCraft->onChange((ActionHandler)& NewBattleState::cbxCraftChange);

	_slrDarkness->setRange(0,15);

//	_slrDepth->setRange(1,3);

//	_cbxTerrain->onChange((ActionHandler)& NewBattleState::cbxTerrainChange);

	std::vector<std::string> difficulty;
	difficulty.push_back("STR_1_BEGINNER");
	difficulty.push_back("STR_2_EXPERIENCED");
	difficulty.push_back("STR_3_VETERAN");
	difficulty.push_back("STR_4_GENIUS");
	difficulty.push_back("STR_5_SUPERHUMAN");
	_cbxDifficulty->setOptions(difficulty);

	_alienRaces = _rules->getAlienRacesList();
	for (std::vector<std::string>::const_iterator
			i = _alienRaces.begin();
			i != _alienRaces.end();
			)
	{
		if ((*i).find("_UNDERWATER") != std::string::npos)
			i = _alienRaces.erase(i);
		else
			++i;
	}
	_cbxAlienRace->setOptions(_alienRaces);

	_slrAlienTech->setRange(
						0,
						static_cast<int>(_rules->getAlienItemLevels().size()) - 1);

	_btnEquip->setText(tr("STR_EQUIP_CRAFT"));
	_btnEquip->onMouseClick((ActionHandler)& NewBattleState::btnEquipClick);

	_btnRandom->setText(tr("STR_RANDOMIZE"));
	_btnRandom->onMouseClick((ActionHandler)& NewBattleState::btnRandomClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& NewBattleState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewBattleState::btnOkClick,
					Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewBattleState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewBattleState::btnCancelClick,
					Options::keyCancel);

	load();
}

/**
 * dTor.
 */
NewBattleState::~NewBattleState()
{}

/**
 * Resets the menu music and savegame when coming back from the battlescape.
 */
void NewBattleState::init()
{
	State::init();

	if (_craft == NULL)
		load();
}

/**
 * Loads new battle data from a YAML file.
 * @param filename - reference a YAML filename (default "battle")
 */
void NewBattleState::load(const std::string& filename)
{
	const std::string config = Options::getConfigFolder() + filename + ".cfg";

	if (CrossPlatform::fileExists(config) == false)
		initPlay();
	else
	{
		try
		{
			YAML::Node doc = YAML::LoadFile(config);

			_cbxMission->setSelected(std::min(
									doc["mission"].as<size_t>(0),
									_missionTypes.size() - 1));
			cbxMissionChange(NULL);

			_cbxCraft->setSelected(std::min(
									doc["craft"].as<size_t>(0),
									_crafts.size() - 1));
			_slrDarkness->setValue(doc["darkness"].as<size_t>(0));
			_cbxTerrain->setSelected(
									std::min(doc["terrain"].as<size_t>(0),
									_terrainTypes.size() - 1));
			_cbxAlienRace->setSelected(std::min(
									doc["alienRace"].as<size_t>(0),
									_alienRaces.size() - 1));
			_cbxDifficulty->setSelected(doc["difficulty"].as<size_t>(0));
			_slrAlienTech->setValue(doc["alienTech"].as<size_t>(0));

			if (doc["base"])
			{
				SavedGame* const savedGame = new SavedGame(_rules);

				Base* const base = new Base(_rules);
				base->load(
						doc["base"],
						savedGame,
						false);
				savedGame->getBases()->push_back(base);

				// Add research
				const std::vector<std::string> &research = _rules->getResearchList();
				for (std::vector<std::string>::const_iterator
						i = research.begin();
						i != research.end();
						++i)
				{
					savedGame->addFinishedResearch(_rules->getResearch(*i));
				}

				// Generate items
				base->getItems()->getContents()->clear();
				const RuleItem* itRule;
				const std::vector<std::string>& items = _rules->getItemsList();
				for (std::vector<std::string>::const_iterator
						i = items.begin();
						i != items.end();
						++i)
				{
					itRule = _rules->getItem(*i);
					if (itRule->getBattleType() != BT_CORPSE
						&& itRule->isRecoverable() == true)
					{
						base->getItems()->addItem(*i);
					}
				}

				// Fix invalid contents
				if (base->getCrafts()->empty() == true)
				{
					const std::string craftType = _crafts[_cbxCraft->getSelected()];
					_craft = new Craft(
									_rules->getCraft(craftType),
									base,
									savedGame->getId(craftType));
					base->getCrafts()->push_back(_craft);
				}
				else
				{
					_craft = base->getCrafts()->front();
					for (std::map<std::string, int>::iterator
							i = _craft->getItems()->getContents()->begin();
							i != _craft->getItems()->getContents()->end();
							++i)
					{
						if (_rules->getItem(i->first) == NULL)
							i->second = 0;
					}
				}

				_game->setSavedGame(savedGame);
			}
			else
				initPlay();
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_WARNING) << e.what();
			initPlay();
		}
	}
}

/**
 * Saves new battle data to a YAML file.
 * @param filename - reference a YAML filename (default "battle")
 */
void NewBattleState::save(const std::string& filename)
{
	const std::string config = Options::getConfigFolder() + filename + ".cfg";

	std::ofstream save (config.c_str()); // init
	if (save.fail() == true)
	{
		Log(LOG_WARNING) << "Failed to save " << filename << ".cfg";
		return;
	}

	YAML::Emitter emit;

	YAML::Node node;
	node["mission"]		= _cbxMission->getSelected();
	node["craft"]		= _cbxCraft->getSelected();
	node["darkness"]	= _slrDarkness->getValue();
	node["terrain"]		= _cbxTerrain->getSelected();
	node["alienRace"]	= _cbxAlienRace->getSelected();
	node["difficulty"]	= _cbxDifficulty->getSelected();
	node["alienTech"]	= _slrAlienTech->getValue();
	node["base"]		= _game->getSavedGame()->getBases()->front()->save();
	emit << node;

	save << emit.c_str();
	save.close();
}

/**
 * Initializes a new savegame with everything available.
 */
void NewBattleState::initPlay()
{
	RNG::setSeed(0);

	SavedGame* const savedGame = new SavedGame(_rules); // uh do these get deleted anywhere
	Base* const base = new Base(_rules);

	const YAML::Node& startBase = _rules->getStartingBase();
	base->load(
			startBase,
			savedGame,
			true,
			true);
	savedGame->getBases()->push_back(base);

	// kill everything we don't want in this base
	for (std::vector<Soldier*>::const_iterator
			i = base->getSoldiers()->begin();
			i != base->getSoldiers()->end();
			++i)
	{
		delete (*i);
	}
	base->getSoldiers()->clear();

	for (std::vector<Craft*>::const_iterator
			i = base->getCrafts()->begin();
			i != base->getCrafts()->end();
			++i)
	{
		delete (*i);
	}
	base->getCrafts()->clear();

	base->getItems()->getContents()->clear();

	_craft = new Craft(
					_rules->getCraft(_crafts[_cbxCraft->getSelected()]),
					base,
					1);
	base->getCrafts()->push_back(_craft);

	// Generate soldiers
	for (int
			i = 0;
			i != 30;
			++i)
	{
		Soldier* const soldier = _rules->genSoldier(savedGame);

		for (int
				j = 0;
				j != 5;
				++j)
		{
			if (RNG::percent(30) == true)
			{
				soldier->promoteRank();

				UnitStats* const stats = soldier->getCurrentStats();
				stats->tu			+= RNG::generate(0,5);
				stats->stamina		+= RNG::generate(0,5);
				stats->health		+= RNG::generate(0,5);
				stats->bravery		+= RNG::generate(0,5);
				stats->reactions	+= RNG::generate(0,5);
				stats->firing		+= RNG::generate(0,5);
				stats->throwing		+= RNG::generate(0,5);
				stats->strength		+= RNG::generate(0,5);
				stats->melee		+= RNG::generate(0,5);
				stats->psiStrength	+= RNG::generate(0,5);
				stats->psiSkill		+= RNG::generate(0,20);
			}
		}

		UnitStats* const stats = soldier->getCurrentStats();
		stats->bravery = static_cast<int>(std::floor(
						(static_cast<double>(stats->bravery) / 10.) + 0.5)) * 10; // lulzor

		base->getSoldiers()->push_back(soldier);
//		if (i < _craft->getRules()->getSoldiers())
//			soldier->setCraft(_craft);
	}

	// Generate items
	const RuleItem* itRule;
	const std::vector<std::string>& items = _rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		itRule = _rules->getItem(*i);
		if (itRule->getBattleType() != BT_CORPSE
			&& itRule->isRecoverable() == true)
		{
			base->getItems()->addItem(*i);
//			if (itRule->getBattleType() != BT_NONE
//				&& itRule->isFixed() == false
//				&& itRule->getBigSprite() > -1)
//			{
//				_craft->getItems()->addItem(*i);
//			}
		}
	}

	// Add research
	const std::vector<std::string>& research = _rules->getResearchList();
	for (std::vector<std::string>::const_iterator
			i = research.begin();
			i != research.end();
			++i)
	{
		savedGame->addFinishedResearch(_rules->getResearch(*i));
	}

	_game->setSavedGame(savedGame);
	cbxMissionChange(NULL);
}

/**
 * Starts the battle.
 * @param action - pointer to an Action
 */
void NewBattleState::btnOkClick(Action*)
{
	save();

	if (_missionTypes[_cbxMission->getSelected()] != "STR_BASE_DEFENSE"
		&& _craft->getNumSoldiers() == 0
		&& _craft->getNumVehicles() == 0)
	{
		return;
	}

	SavedBattleGame* const battleSave = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(battleSave);
	battleSave->setMissionType(_missionTypes[_cbxMission->getSelected()]);
	BattlescapeGenerator bGen = BattlescapeGenerator(_game);
	Base* base = NULL;

	bGen.setTacTerrain(_rules->getTerrain(_terrainTypes[_cbxTerrain->getSelected()]));

	if (_missionTypes[_cbxMission->getSelected()] == "STR_BASE_DEFENSE") // base defense
	{
		base = _craft->getBase();
		bGen.setBase(base);
		_craft = NULL; // kL_note: may need to remove this for .. some reason.
	}
	else if (_missionTypes[_cbxMission->getSelected()] == "STR_ALIEN_BASE_ASSAULT") // alien base
	//_missionTypes[_cbxMission->getSelected()].find("STR_ALIEN_BASE") != std::string::npos
	{
		AlienBase* const alienBase = new AlienBase();
		alienBase->setId(1);
		alienBase->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
		_craft->setDestination(alienBase);
		bGen.setAlienBase(alienBase);

		_game->getSavedGame()->getAlienBases()->push_back(alienBase);
	}
	else if (_craft != NULL
		&& _rules->getUfo(_missionTypes[_cbxMission->getSelected()]) != NULL) // ufo assault
	{
		Ufo* const ufo = new Ufo(_rules->getUfo(_missionTypes[_cbxMission->getSelected()]));
		ufo->setId(1);
		_craft->setDestination(ufo);
		bGen.setUfo(ufo);

		if (RNG::percent(50) == true) // either ground assault or ufo crash
			battleSave->setMissionType("STR_UFO_GROUND_ASSAULT");
		else
			battleSave->setMissionType("STR_UFO_CRASH_RECOVERY");

		_game->getSavedGame()->getUfos()->push_back(ufo);
	}
	else // mission site
	{
		const AlienDeployment* const deployment = _rules->getDeployment(battleSave->getMissionType());
		const RuleAlienMission* const mission = _rules->getAlienMission(_rules->getAlienMissionList().front()); // doesn't matter
		MissionSite* const missionSite = new MissionSite(
													mission,
													deployment);
		missionSite->setId(1);
		missionSite->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);

		_craft->setDestination(missionSite);

		bGen.setMissionSite(missionSite);
		_game->getSavedGame()->getMissionSites()->push_back(missionSite);
	}

	if (_craft != NULL)
	{
		_craft->setSpeed(0);
		bGen.setCraft(_craft);
	}

	_game->getSavedGame()->setDifficulty(static_cast<GameDifficulty>(_cbxDifficulty->getSelected()));

	bGen.setTacShade(_slrDarkness->getValue());
	bGen.setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
	bGen.setAlienItemlevel(_slrAlienTech->getValue());
//	battleSave->setDepth(_slrDepth->getValue());

	bGen.run();


	_game->popState();
	_game->popState();

	_game->pushState(new BriefingState(
									_craft,
									base));

	_craft = NULL;
//	_game->getResourcePack()->fadeMusic(_game, 335); // TODO: sort out musicFade from NewBattleState to Briefing ...
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void NewBattleState::btnCancelClick(Action*)
{
	save();

	_game->setSavedGame(NULL);
	_game->popState();
}

/**
 * Randomize the state
 * @param action - pointer to an Action
 */
void NewBattleState::btnRandomClick(Action*)
{
	initPlay();

	_cbxMission->setSelected(RNG::generate(
										0,
										_missionTypes.size() - 1));
	cbxMissionChange(NULL);

	_cbxCraft->setSelected(RNG::generate(
										0,
										_crafts.size() - 1));
	cbxCraftChange(NULL);

	_slrDarkness->setValue(RNG::generate(0,15));

	_cbxTerrain->setSelected(RNG::generate(
										0,
										_terrainTypes.size() - 1));
//	cbxTerrainChange(NULL);

	_cbxAlienRace->setSelected(RNG::generate(
										0,
										_alienRaces.size() - 1));

	_cbxDifficulty->setSelected(RNG::generate(0,4));

	_slrAlienTech->setValue(RNG::generate(
										0,
										_rules->getAlienItemLevels().size() - 1));
}

/**
 * Shows the Craft Info screen.
 * @param action - pointer to an Action
 */
void NewBattleState::btnEquipClick(Action*)
{
	_game->pushState(new CraftInfoState(
									_game->getSavedGame()->getBases()->front(),
									0));
}

/**
 * Updates Map Options based on the current Mission type.
 * @param action - pointer to an Action
 */
void NewBattleState::cbxMissionChange(Action*)
{
	Log(LOG_INFO) << "NewBattleState::cbxMissionChange()";
	const AlienDeployment* const ruleDeploy = _rules->getDeployment(_missionTypes[_cbxMission->getSelected()]);
	Log(LOG_INFO) << ". ruleDeploy = " << ruleDeploy->getType();

	std::vector<std::string> // Get terrains associated with this mission
		deployTerrains = ruleDeploy->getDeployTerrains(),
		globeTerrains;

	// debug:
	for (std::vector<std::string>::const_iterator
			i = deployTerrains.begin();
			i != deployTerrains.end();
			++i)
		Log(LOG_INFO) << ". . deployTerrain = " << *i;


	if (deployTerrains.empty() == true)
	{
		Log(LOG_INFO) << ". . deployTerrains invalid; get globeTerrains w/out Deployment rule";
		globeTerrains = _rules->getGlobe()->getGlobeTerrains("");

		// debug:
		for (std::vector<std::string>::const_iterator
				i = globeTerrains.begin();
				i != globeTerrains.end();
				++i)
			Log(LOG_INFO) << ". . . globeTerrain = " << *i;
	}
	else
	{
		Log(LOG_INFO) << ". . deployTerrains valid; get globeTerrains w/ Deployment rule";
		globeTerrains = _rules->getGlobe()->getGlobeTerrains(ruleDeploy->getType());

		// debug:
		for (std::vector<std::string>::const_iterator
				i = globeTerrains.begin();
				i != globeTerrains.end();
				++i)
			Log(LOG_INFO) << ". . . globeTerrain = " << *i;
	}

	std::set<std::string> terrains;

	for (std::vector<std::string>::const_iterator
			i = deployTerrains.begin();
			i != deployTerrains.end();
			++i)
	{
		Log(LOG_INFO) << ". . insert deployTerrain = " << *i;
		terrains.insert(*i);
	}

	for (std::vector<std::string>::const_iterator
		i = globeTerrains.begin();
		i != globeTerrains.end();
		++i)
	{
		Log(LOG_INFO) << ". . insert globeTerrain = " << *i;
		terrains.insert(*i);
	}

	_terrainTypes.clear();

	std::vector<std::string> terrainOptions;
	for (std::set<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end();
			++i)
	{
		Log(LOG_INFO) << ". . insert to _terrainTypes Option = " << *i;
		_terrainTypes.push_back(*i);
		terrainOptions.push_back("MAP_" + *i);
	}

	bool vis = ruleDeploy->getShade() == -1; // Hide controls that don't apply to mission
	_txtDarkness->setVisible(vis);
	_slrDarkness->setVisible(vis);

	vis = _terrainTypes.size() > 1;
	_txtTerrain->setVisible(vis);
	_cbxTerrain->setVisible(vis);

	_cbxTerrain->setOptions(terrainOptions);
	_cbxTerrain->setSelected(0);

//	cbxTerrainChange(NULL);
	Log(LOG_INFO) << "NewBattleState::cbxMissionChange() EXIT";
}

/**
 * Updates craft accordingly.
 * @param action - pointer to an Action
 */
void NewBattleState::cbxCraftChange(Action*)
{
	_craft->changeRules(_rules->getCraft(_crafts[_cbxCraft->getSelected()]));

	const int maxSoldiers = _craft->getRules()->getSoldiers();
	int curSoldiers = _craft->getNumSoldiers();
	if (curSoldiers > maxSoldiers)
	{
		for (std::vector<Soldier*>::const_reverse_iterator
				i = _craft->getBase()->getSoldiers()->rbegin();
				i != _craft->getBase()->getSoldiers()->rend()
					&& curSoldiers > maxSoldiers;
				++i)
		{
			if ((*i)->getCraft() == _craft)
			{
				(*i)->setCraft(NULL);
				--curSoldiers;
			}
		}
	}
}

/**
 * Updates the depth slider accordingly when terrain selection changes.
 * @param action - pointer to an Action
 */
/* void NewBattleState::cbxTerrainChange(Action*)
{
	const AlienDeployment* const ruleDeploy = _rules->getDeployment(_missionTypes[_cbxMission->getSelected()]);

	int
		minDepth,
		maxDepth;

	if (ruleDeploy->getMaxDepth() > 0)
	{
		minDepth = ruleDeploy->getMinDepth();
		maxDepth = ruleDeploy->getMaxDepth();
	}
	else if (ruleDeploy->getDeployTerrains().empty() == false)
	{
		minDepth = _rules->getTerrain(ruleDeploy->getDeployTerrains().front())->getMinDepth();
		maxDepth = _rules->getTerrain(ruleDeploy->getDeployTerrains().front())->getMaxDepth();
	}
	else
	{
		minDepth = _rules->getTerrain(_terrainTypes.at(_cbxTerrain->getSelected()))->getMinDepth();
		maxDepth = _rules->getTerrain(_terrainTypes.at(_cbxTerrain->getSelected()))->getMaxDepth();
	}

	_txtDepth->setVisible(minDepth != maxDepth
						  && maxDepth != 0);
	_slrDepth->setVisible(minDepth != maxDepth
						  && maxDepth != 0);
	_slrDepth->setRange(minDepth, maxDepth);
	_slrDepth->setValue(minDepth);
} */

}
