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
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
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
	_window				= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
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

	_txtDepth			= new Text(120, 9, 22, 143);
	_slrDepth			= new Slider(120, 16, 22, 153);

	_txtDifficulty		= new Text(120, 9, 178, 83);
	_cbxDifficulty		= new ComboBox(this, 120, 16, 178, 93);

	_txtAlienRace		= new Text(120, 9, 178, 113);
	_cbxAlienRace		= new ComboBox(this, 120, 16, 178, 123);

	_txtAlienTech		= new Text(120, 9, 178, 143);
	_slrAlienTech		= new Slider(120, 16, 178, 153);

	_btnCancel			= new TextButton(100, 16, 8, 176);
	_btnRandom			= new TextButton(100, 16, 110, 176);
	_btnOk				= new TextButton(100, 16, 212, 176);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("newBattleMenu")->getElement("palette")->color);

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
	add(_txtDepth,			"text",		"newBattleMenu");
	add(_slrDepth,			"button1",	"newBattleMenu");
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

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MISSION_GENERATOR"));

	_txtMapOptions->setText(tr("STR_MAP_OPTIONS"));

	_frameLeft->setThickness(3);

	_txtAlienOptions->setText(tr("STR_ALIEN_OPTIONS"));

	_frameRight->setThickness(3);

	_txtMission->setText(tr("STR_MISSION"));

	_txtCraft->setText(tr("STR_CRAFT"));

	_txtDarkness->setText(tr("STR_MAP_DARKNESS"));

	_txtDepth->setText(tr("STR_MAP_DEPTH"));

	_txtTerrain->setText(tr("STR_MAP_TERRAIN"));

	_txtDifficulty->setText(tr("STR_ALIEN_DIFFICULTY"));

	_txtAlienRace->setText(tr("STR_ALIEN_RACE"));

	_txtAlienTech->setText(tr("STR_ALIEN_TECH_LEVEL"));

	_missionTypes = _game->getRuleset()->getDeploymentsList();
	_cbxMission->setOptions(_missionTypes);
	_cbxMission->onChange((ActionHandler)& NewBattleState::cbxMissionChange);

	const std::vector<std::string>& crafts = _game->getRuleset()->getCraftsList();
	for (std::vector<std::string>::const_iterator
			i = crafts.begin();
			i != crafts.end();
			++i)
	{
		const RuleCraft* const rule = _game->getRuleset()->getCraft(*i);
		if (rule->getSoldiers() > 0)
			_crafts.push_back(*i);
	}
	_cbxCraft->setOptions(_crafts);
	_cbxCraft->onChange((ActionHandler)& NewBattleState::cbxCraftChange);

	_slrDarkness->setRange(0,15);

	_slrDepth->setRange(1,3);

	_cbxTerrain->onChange((ActionHandler)& NewBattleState::cbxTerrainChange);

	std::vector<std::string> difficulty;
	difficulty.push_back("STR_1_BEGINNER");
	difficulty.push_back("STR_2_EXPERIENCED");
	difficulty.push_back("STR_3_VETERAN");
	difficulty.push_back("STR_4_GENIUS");
	difficulty.push_back("STR_5_SUPERHUMAN");
	_cbxDifficulty->setOptions(difficulty);

	_alienRaces = _game->getRuleset()->getAlienRacesList();
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
						_game->getRuleset()->getAlienItemLevels().size() - 1);

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
	const std::string st = Options::getConfigFolder() + filename + ".cfg";
	if (CrossPlatform::fileExists(st) == false)
		initSave();
	else
	{
		try
		{
			YAML::Node doc = YAML::LoadFile(st);

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
				const Ruleset* const rule = _game->getRuleset();
				SavedGame* const save = new SavedGame(rule);

				Base* const base = new Base(rule);
				base->load(
						doc["base"],
						save,
						false);
				save->getBases()->push_back(base);

				// Add research
				const std::vector<std::string> &research = rule->getResearchList();
				for (std::vector<std::string>::const_iterator
						i = research.begin();
						i != research.end();
						++i)
				{
					save->addFinishedResearch(rule->getResearch(*i));
				}

				// Generate items
				base->getItems()->getContents()->clear();
				const std::vector<std::string>& items = rule->getItemsList();
				for (std::vector<std::string>::const_iterator
						i = items.begin();
						i != items.end();
						++i)
				{
					const RuleItem* const rule = _game->getRuleset()->getItem(*i);
					if (rule->getBattleType() != BT_CORPSE
						&& rule->isRecoverable() == true)
					{
						base->getItems()->addItem(*i, 1);
					}
				}

				// Fix invalid contents
				if (base->getCrafts()->empty() == true)
				{
					const std::string craftType = _crafts[_cbxCraft->getSelected()];
					_craft = new Craft(
									_game->getRuleset()->getCraft(craftType),
									base,
									save->getId(craftType));
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
						const RuleItem* const rule = _game->getRuleset()->getItem(i->first);
						if (rule == NULL)
							i->second = 0;
					}
				}

				_game->setSavedGame(save);
			}
			else
				initSave();
		}
		catch (YAML::Exception e)
		{
			Log(LOG_WARNING) << e.what();
			initSave();
		}
	}
}

/**
 * Saves new battle data to a YAML file.
 * @param filename - reference a YAML filename (default "battle")
 */
void NewBattleState::save(const std::string& filename)
{
	const std::string st = Options::getConfigFolder() + filename + ".cfg";
	std::ofstream save (st.c_str()); // init
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
void NewBattleState::initSave()
{
	const Ruleset* const rule = _game->getRuleset();
	SavedGame* const savedGame = new SavedGame(rule);
	Base* const base = new Base(rule);
	const YAML::Node& startBase = _game->getRuleset()->getStartingBase();
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
					rule->getCraft(_crafts[_cbxCraft->getSelected()]),
					base,
					1);
	base->getCrafts()->push_back(_craft);

	// Generate soldiers
	for (int
			i = 0;
			i < 30;
			++i)
	{
		Soldier* const soldier = rule->genSoldier(savedGame);

		for (int
				n = 0;
				n < 5;
				++n)
		{
			if (RNG::percent(70) == true)
				continue;

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

		UnitStats* const stats = soldier->getCurrentStats();
		stats->bravery = static_cast<int>(std::floor(
						(static_cast<double>(stats->bravery) / 10.) + 0.5)) * 10; // kL, lulzor

		base->getSoldiers()->push_back(soldier);
		if (i < _craft->getRules()->getSoldiers())
			soldier->setCraft(_craft);
	}

	// Generate items
	const std::vector<std::string>& items = rule->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		const RuleItem* const rule = _game->getRuleset()->getItem(*i);
		if (rule->getBattleType() != BT_CORPSE
			&& rule->isRecoverable() == true)
		{
			base->getItems()->addItem(*i, 1);

			if (rule->getBattleType() != BT_NONE
				&& rule->isFixed() == false
				&& rule->getBigSprite() > -1)
			{
				_craft->getItems()->addItem(*i, 1);
			}
		}
	}

	// Add research
	const std::vector<std::string>& research = rule->getResearchList();
	for (std::vector<std::string>::const_iterator
			i = research.begin();
			i != research.end();
			++i)
	{
		savedGame->addFinishedResearch(rule->getResearch(*i));
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

	SavedBattleGame* const bgame = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(bgame);
	bgame->setMissionType(_missionTypes[_cbxMission->getSelected()]);
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);
	Base* base = NULL;

	bgen.setTerrain(_game->getRuleset()->getTerrain(_terrainTypes[_cbxTerrain->getSelected()]));

	if (_missionTypes[_cbxMission->getSelected()] == "STR_BASE_DEFENSE") // base defense
	{
		base = _craft->getBase();
		bgen.setBase(base);
		_craft = NULL; // kL_note: may need to remove this for .. some reason.
	}
	else if (_missionTypes[_cbxMission->getSelected()] == "STR_ALIEN_BASE_ASSAULT") // alien base
	{
		AlienBase* const ab = new AlienBase();
		ab->setId(1);
		ab->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
		_craft->setDestination(ab);
		bgen.setAlienBase(ab);

		_game->getSavedGame()->getAlienBases()->push_back(ab);
	}
	else if (_craft != NULL
		&& _game->getRuleset()->getUfo(_missionTypes[_cbxMission->getSelected()]) != NULL) // ufo assault
	{
		Ufo* const ufo = new Ufo(_game->getRuleset()->getUfo(_missionTypes[_cbxMission->getSelected()]));
		ufo->setId(1);
		_craft->setDestination(ufo);
		bgen.setUfo(ufo);

		// either ground assault or ufo crash
		if (RNG::percent(50) == true)
			bgame->setMissionType("STR_UFO_GROUND_ASSAULT");
		else
			bgame->setMissionType("STR_UFO_CRASH_RECOVERY");

		_game->getSavedGame()->getUfos()->push_back(ufo);
	}
	else // mission site
	{
		const AlienDeployment* deployment = _game->getRuleset()->getDeployment(bgame->getMissionType());
		const RuleAlienMission* mission = _game->getRuleset()->getAlienMission(_game->getRuleset()->getAlienMissionList().front()); // doesn't matter
		MissionSite* ms = new MissionSite(
										mission,
										deployment);
		ms->setId(1);
		ms->setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);

		_craft->setDestination(ms);

		bgen.setMissionSite(ms);
		_game->getSavedGame()->getMissionSites()->push_back(ms);
	}

	if (_craft != NULL)
	{
		_craft->setSpeed(0);
		bgen.setCraft(_craft);
	}

	_game->getSavedGame()->setDifficulty(static_cast<GameDifficulty>(_cbxDifficulty->getSelected()));

	bgen.setSiteShade(_slrDarkness->getValue());
	bgen.setAlienRace(_alienRaces[_cbxAlienRace->getSelected()]);
	bgen.setAlienItemlevel(_slrAlienTech->getValue());
	bgame->setDepth(_slrDepth->getValue());

	bgen.run();


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
	initSave();

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
	cbxTerrainChange(NULL);

	_cbxAlienRace->setSelected(RNG::generate(
										0,
										_alienRaces.size() - 1));

	_cbxDifficulty->setSelected(RNG::generate(0,4));

	_slrAlienTech->setValue(RNG::generate(
										0,
										_game->getRuleset()->getAlienItemLevels().size() - 1));
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
	const AlienDeployment* const ruleDeploy = _game->getRuleset()->getDeployment(_missionTypes[_cbxMission->getSelected()]);

	// Get terrains associated with this mission
	std::vector<std::string>
		deployTerrains = ruleDeploy->getTerrains(),
		globeTerrains;

	if (deployTerrains.empty() == true)
		globeTerrains = _game->getRuleset()->getGlobe()->getTerrains("");
	else
		globeTerrains = _game->getRuleset()->getGlobe()->getTerrains(ruleDeploy->getType());

	std::set<std::string> terrains;

	for (std::vector<std::string>::const_iterator
			i = deployTerrains.begin();
			i != deployTerrains.end();
			++i)
	{
		terrains.insert(*i);
	}

	for (std::vector<std::string>::const_iterator
		i = globeTerrains.begin();
		i != globeTerrains.end();
		++i)
	{
		terrains.insert(*i);
	}

	_terrainTypes.clear();

	std::vector<std::string> terrainStrings;
	for (std::set<std::string>::const_iterator
			i = terrains.begin();
			i != terrains.end();
			++i)
	{
		_terrainTypes.push_back(*i);
		terrainStrings.push_back("STR_" + *i);
	}

	// Hide controls that don't apply to mission
	bool vis = ruleDeploy->getShade() == -1;
	_txtDarkness->setVisible(vis);
	_slrDarkness->setVisible(vis);

	vis = _terrainTypes.size() > 1;
	_txtTerrain->setVisible(vis);
	_cbxTerrain->setVisible(vis);

	_cbxTerrain->setOptions(terrainStrings);
	_cbxTerrain->setSelected(0);

	cbxTerrainChange(NULL);
}

/**
 * Updates craft accordingly.
 * @param action - pointer to an Action
 */
void NewBattleState::cbxCraftChange(Action*)
{
	_craft->changeRules(_game->getRuleset()->getCraft(_crafts[_cbxCraft->getSelected()]));

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
void NewBattleState::cbxTerrainChange(Action*)
{
	const AlienDeployment* const ruleDeploy = _game->getRuleset()->getDeployment(_missionTypes[_cbxMission->getSelected()]);

	int
		minDepth = _game->getRuleset()->getTerrain(_terrainTypes.at(_cbxTerrain->getSelected()))->getMinDepth(),
		maxDepth = _game->getRuleset()->getTerrain(_terrainTypes.at(_cbxTerrain->getSelected()))->getMaxDepth();

	if (ruleDeploy->getTerrains().empty() == false)
	{
		minDepth = _game->getRuleset()->getTerrain(ruleDeploy->getTerrains().front())->getMinDepth();
		maxDepth = _game->getRuleset()->getTerrain(ruleDeploy->getTerrains().front())->getMaxDepth();
	}

	_txtDepth->setVisible(minDepth != maxDepth && maxDepth != 0);
	_slrDepth->setVisible(minDepth != maxDepth && maxDepth != 0);
	_slrDepth->setRange(minDepth, maxDepth);
	_slrDepth->setValue(minDepth);
}

}
