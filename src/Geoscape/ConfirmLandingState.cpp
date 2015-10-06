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

#include "ConfirmLandingState.h"

//#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleCity.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleTexture.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Landing window.
 * @param craft 	- pointer to the Craft to confirm
 * @param texRule	- pointer to the RuleTexture of the landing site (default NULL)
 * @param shade		- shade of the landing site (default -1)
 */
ConfirmLandingState::ConfirmLandingState(
		Craft* const craft,
		RuleTexture* texRule, // always passes in the vector of eligible Globe Terrains for the land-poly's textureInt.
		const int shade)
	:
		_craft(craft),
//		_texRule(texRule),
		_shade(shade),
		_terrainRule(NULL)
//		_city(NULL)
{
	Log(LOG_INFO) << "Create ConfirmLandingState()";
	// TODO: show Country & Region
	// TODO: should do buttons: Patrol (isCancel already) or GeoscapeCraftState or Return to base.
	_screen = false;

	_window			= new Window(this, 230, 160, 13, 20, POPUP_BOTH);

	_txtBase		= new Text( 80, 9,  23, 29);
	_txtTexture		= new Text(150, 9,  83, 29);
	_txtShade		= new Text( 60, 9, 173, 39);

	_txtMessage		= new Text(206, 40, 25, 47);
	_txtMessage2	= new Text(206, 43, 25, 87);

	_txtBegin		= new Text(206, 17, 25, 130);

	_btnNo			= new TextButton(80, 18,  40, 152);
	_btnYes			= new TextButton(80, 18, 136, 152);

	setInterface("confirmLanding");

	add(_window,		"window",	"confirmLanding");
	add(_txtBase,		"text",		"confirmLanding");
	add(_txtTexture,	"text",		"confirmLanding");
	add(_txtShade,		"text",		"confirmLanding");
	add(_txtMessage,	"text",		"confirmLanding");
	add(_txtMessage2,	"text",		"confirmLanding");
	add(_txtBegin,		"text",		"confirmLanding");
	add(_btnNo,			"button",	"confirmLanding");
	add(_btnYes,		"button",	"confirmLanding");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_txtBase->setText(craft->getBase()->getName(_game->getLanguage()));

	_txtShade->setText(tr("STR_SHADE_").arg(shade));
	_txtShade->setAlign(ALIGN_RIGHT);

	// NOTE: the following terrain determination will fall through to
	// BattlescapeGenerator for Base assault/defense and Cydonia missions;
	// concerned only with UFOs and MissionSites here.
	Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	MissionSite* const site = dynamic_cast<MissionSite*>(_craft->getDestination());

	if (ufo != NULL
		|| site != NULL) // ... else it's an aLienBase assault (NOT defense nor Cydonia).
	{
		double // Determine if Craft is landing at a City.
			lon = _craft->getLongitude(),
			lat = _craft->getLatitude();
		bool city = false;

		for (std::vector<Region*>::const_iterator
				i = _game->getSavedGame()->getRegions()->begin();
				i != _game->getSavedGame()->getRegions()->end()
					&& city == false;
//					&& _city == NULL;
				++i)
		{
			if ((*i)->getRules()->insideRegion(
											lon,
											lat) == true)
			{
				for (std::vector<RuleCity*>::const_iterator
						j = (*i)->getRules()->getCities()->begin();
						j != (*i)->getRules()->getCities()->end()
							&& city == false;
//							&& _city == NULL;
						++j)
				{
					if (AreSame((*j)->getLongitude(), lon)
						&& AreSame((*j)->getLatitude(), lat))
					{
//						_city = *j;
						city = true;
						Log(LOG_INFO) << ". . . city found = " << (*j)->getName();
					}
				}
			}
		}

		std::string terrainType;

		if (ufo != NULL) // UFO crashed/landed
			terrainType = ufo->getUfoTerrainType();		// Ufo-object stores the terrainType value.
		else // missionSite
			terrainType = site->getSiteTerrainType();	// missionSite-object stores the terrainType value.


		if (terrainType.empty() == true) // Determine terrainType/RuleTerrain and store it.
		{
			Log(LOG_INFO) << ". determine Terrain";
			if (site != NULL) // missionSite
			{
				Log(LOG_INFO) << ". . missionSite";
//				if (_city != NULL) // missionSite is at a City.
				{
					std::vector<std::string> terrainList;
					// terrains for Missions can be/are defined in both AlienDeployment AND through RuleGlobe(Textures)
					// Options:
					// 1. choose from both aspects
					// 2. choose Deployment preferentially
					// 3. choose Globe-texture preferentially
					// ...
					// PROFIT!!
					// conclusion: choose among Globe-Texture's def'd deployments first;
					// if none found, choose among Deployment def'd terrains ....
					// Note: cf. NewBattleState::cbxMissionChange()

					// BZZZT. Do it the opposite way; check deployTerrains first, then textureTerrains.

					// TODO: tie all this into WeightedOptions
					// check for Terrains in Globe-Texture(INT) first
/*					const RuleGlobe* const globeRule = _game->getRuleset()->getGlobe();
					const RuleTexture* const texRule = globeRule->getTextureRule(_city->getTextureInt());
					terrainList = globeRule->getGlobeTerrains(texRule->getTextureDeployment());

					// second, check for Terrains in AlienDeployment ...
					if (terrainList.empty() == true)
					{
						// get a Terrain from AlienDeployment
						const AlienDeployment* const deployRule = site->getDeployment();
						terrainList = deployRule->getDeployTerrains();
					} */

					// get a Terrain from AlienDeployment first
					Log(LOG_INFO) << ". . . finding eligibleTerrain for AlienDeployment";
					const AlienDeployment* const deployRule = site->getDeployment();
					terrainList = deployRule->getDeployTerrains();

					// second, check for Terrains in Globe-Texture(INT) ...
					if (terrainList.empty() == true)
					{
						Log(LOG_INFO) << ". . . finding eligibleTerrain for RuleGlobe";
						const RuleGlobe* const globeRule = _game->getRuleset()->getGlobe();
//						const RuleTexture* const texRule = globeRule->getTextureRule(_city->getTextureInt());
						terrainList = globeRule->getGlobeTerrains(); //texRule->getTextureDeployments() -> now uses a weighted system ....
					}

					if (terrainList.empty() == false) // SAFETY.
					{
						const size_t pick (RNG::pick(terrainList.size()));
						Log(LOG_INFO) << ". . . . size = " << (int)terrainList.size() << " pick = " << (int)pick;
						Log(LOG_INFO) << ". . . . terrain = " << terrainList.at(pick) << " - Not Weighted";
						_terrainRule = _game->getRuleset()->getTerrain(terrainList.at(pick));
					}
//					else fuck off. Thanks!
					else Log(LOG_INFO) << ". . . . eligibleTerrain NOT Found. Must be Cydonia, Base assault/defense ...";

					terrainType = _terrainRule->getType();
					Log(LOG_INFO) << ". . . using terrainType: " << terrainType;
				}
//				else	// SAFETY: for missionSite that's not at a City.
						// This should be the same as for NOT City!!!
//					terrainType = _texRule->getRandomTerrain(_craft->getDestination());
					// note: that should crash if on Water tex

				site->setSiteTerrainType(terrainType);
//->			_terrainRule = selectCityTerrain(lat);
			}
			else // is UFO
			{
//				if (_city != NULL) // UFO at a City (eg. Battleship on Infiltration trajectory)
				if (city == true)
				{
					Log(LOG_INFO) << ". . UFO at City";
					// choose from texture(INT) #10, Urban w/ UFO types
					// note that differences between tex -1 & tex -2 have not been implemented yet,
					// so treat them both the same -> texture(INT) #10
					// INDUSTRIALUFO, MADURBANUFO, NATIVEUFO
					const RuleGlobe* const globeRule = _game->getRuleset()->getGlobe();
					const RuleTexture* const texRule = globeRule->getTextureRule(OpenXcom::TT_URBAN);

					terrainType = texRule->getRandomTerrain(ufo);
					// NOTE that inputting coordinates can screw getRandomTerrain() if & when 'target'
					// is not contained within any of the Texture's Terrain's TerrainCriteria coordinates.
					// I don't believe the function has a viable fallback mechanism
					// ... instead it would merely return an empty string.

//					if (_city->getTextureInt() == -1) // Texture ID -1
//					{}
//					else if (_city->getTextureInt() == -2) // Texture ID -2
//					{}
//					else SAFETY!
				}
				else // UFO not at City
				{
					Log(LOG_INFO) << ". . UFO not at City";
//					terrainType = _texRule->getRandomTerrain(_craft->getDestination());
					terrainType = texRule->getRandomTerrain(_craft->getDestination());
//->				_terrainRule = selectTerrain(lat);
				}

				ufo->setUfoTerrainType(terrainType);
			}
		}
		Log(LOG_INFO) << ". chosen terrainType = " << terrainType;

		if (_terrainRule == NULL) // '_terrainRule' can be set above^ if missionSite <-
			_terrainRule = _game->getRuleset()->getTerrain(terrainType);

		_txtTexture->setText(tr("STR_TEXTURE_").arg(tr(terrainType)));
		_txtTexture->setAlign(ALIGN_RIGHT);
	}
	else // aLienBase assault (NOT defense nor Cydonia)
	{
		Log(LOG_INFO) << ". ufo/terrorsite NOT valid";
		_txtTexture->setVisible(false);
		_txtShade->setVisible(false);
	}

	_txtMessage->setText(tr("STR_CRAFT_READY_TO_LAND_AT")
						 .arg(_craft->getName(_game->getLanguage())));
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setBig();

	std::wostringstream woststr;
	if (ufo != NULL)
	{
//		const RuleUfo* const ufoRule = ufo->getRules(); if (ufoRule != NULL)
		woststr << tr(ufo->getRules()->getType());

		if (ufo->getHyperDetected() == true) // only ufoType shows if not hyperdetected.
			woststr << L" : " << tr(ufo->getAlienRace());
	}
	_txtMessage2->setText(tr("STR_CRAFT_DESTINATION")
						 .arg(_craft->getDestination()->getName(_game->getLanguage()))
						 .arg(woststr.str()));
	_txtMessage2->setBig();
	_txtMessage2->setAlign(ALIGN_CENTER);

	_txtBegin->setText(tr("STR_BEGIN_MISSION"));
	_txtBegin->setAlign(ALIGN_CENTER);
	_txtBegin->setBig();

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& ConfirmLandingState::btnYesClick);
	_btnYes->onKeyboardPress(
					(ActionHandler)& ConfirmLandingState::btnYesClick,
					Options::keyOk);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)& ConfirmLandingState::btnNoClick);
	_btnNo->onKeyboardPress(
					(ActionHandler)& ConfirmLandingState::btnNoClick,
					Options::keyCancel);
}

/**
 * dTor
 */
ConfirmLandingState::~ConfirmLandingState()
{}

/**
 * Make sure Craft isn't simply returning to base.
 */
void ConfirmLandingState::init()
{
	State::init();

//	- note that Craft arriving at an xCom Base do not invoke this state, see GeoscapeState::time5Seconds()
//	const Base* const base = dynamic_cast<Base*>(_craft->getDestination());
//	if (base == _craft->getBase())
//		_game->popState();
}

/**
 * Enters the mission.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnYesClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 335);
	_game->popState();

	Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	MissionSite* const site = dynamic_cast<MissionSite*>(_craft->getDestination());
	AlienBase* const alienBase = dynamic_cast<AlienBase*>(_craft->getDestination());

	SavedBattleGame* battleSave = new SavedBattleGame(&_game->getRuleset()->getOperations());
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen (_game); // init.
	bGen.setCraft(_craft);

	if (ufo != NULL)
	{
		if (ufo->getUfoStatus() == Ufo::CRASHED)
			battleSave->setTacticalType("STR_UFO_CRASH_RECOVERY");
		else
			battleSave->setTacticalType("STR_UFO_GROUND_ASSAULT");

		bGen.setUfo(ufo);
		bGen.setAlienRace(ufo->getAlienRace());
		bGen.setTerrain(_terrainRule); // kL
		bGen.setShade(_shade);
//		bGen.setTacTexture(_texRule); // was an INT <- !!!
//		bGen.setIsCity(_city != NULL); // kL
	}
	else if (site != NULL)
	{
		battleSave->setTacticalType(site->getDeployment()->getType());

		bGen.setMissionSite(site);
		bGen.setAlienRace(site->getAlienRace());
		bGen.setTerrain(_terrainRule); // kL
		bGen.setShade(_shade);
//		bGen.setTacTexture(_texRule); // was an INT <- !!!
	}
	else if (alienBase != NULL)
	{
		battleSave->setTacticalType("STR_ALIEN_BASE_ASSAULT");

		bGen.setAlienBase(alienBase);
		bGen.setAlienRace(alienBase->getAlienRace());
//		bGen.setTacTexture(NULL); // was an INT <- !!! bGen default NULL
	}
	else
	{
		throw Exception("No mission available!");
	}

	bGen.run(); // <- DETERMINE ALL TACTICAL DATA. |<--

	_game->pushState(new BriefingState(_craft));
}

/**
 * Returns the craft to base and closes the window.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnNoClick(Action*)
{
//	_craft->returnToBase();
	_craft->setDestination(NULL);
	_game->popState();
}

/**
 * Selects a terrain type for crashed or landed UFOs.
 * @param lat - latitude of the UFO
 */
/*
RuleTerrain* ConfirmLandingState::selectTerrain(const double lat)
{
	std::vector<RuleTerrain*> terrains;

	const std::vector<std::string>& terrainList = _game->getRuleset()->getTerrainList();
	for (std::vector<std::string>::const_iterator
			i = terrainList.begin();
			i != terrainList.end();
			++i)
	{
		Log(LOG_INFO) << ". . . terrain = " << *i;
		RuleTerrain* const terrainRule = _game->getRuleset()->getTerrain(*i);
		for (std::vector<int>::const_iterator
				j = terrainRule->getTextures()->begin();
				j != terrainRule->getTextures()->end();
				++j)
		{
			Log(LOG_INFO) << ". . . . texture = " << *j;
			if (*j == _texture
				&& (terrainRule->getHemisphere() == 0
					|| (terrainRule->getHemisphere() < 0
						&& lat < 0.)
					|| (terrainRule->getHemisphere() > 0
						&& lat >= 0.)))
			{
				Log(LOG_INFO) << ". . . . . terrainRule = " << *i;
				terrains.push_back(terrainRule);
			}
		}
	}

	if (terrains.empty() == false)
	{
		const size_t pick = static_cast<size_t>(RNG::generate(
														0,
														static_cast<int>(terrains.size()) - 1));
		Log(LOG_INFO) << ". . selected terrain = " << terrains.at(pick)->getName();
		return terrains.at(pick);
	}

	// else, mission is on water ... pick a city terrain
	// This should actually never happen if AlienMission zone3 globe data is correct.
	// But do this as a safety:
	Log(LOG_INFO) << ". WARNING: terrain NOT Valid - selecting City terrain";
	// note that the URBAN MapScript, spec'd for all city terrains, will not add the UFO-dropship.
	// ... could be cool. Postnote: yeh, was cool!!!!
	_city = true;
	return selectCityTerrain(lat);
} */

/**
 * Selects a terrain type for missions at cities.
 * @param lat - latitude of the city
 */
/*
RuleTerrain* ConfirmLandingState::selectCityTerrain(const double lat)
{
	const AlienDeployment* const ruleDeploy = _game->getRuleset()->getDeployment("STR_TERROR_MISSION");
	const size_t pick = static_cast<size_t>(RNG::generate(
														0,
														static_cast<int>(ruleDeploy->getDeployTerrains().size()) - 1));
	RuleTerrain* const terrainRule = _game->getRuleset()->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
	Log(LOG_INFO) << "cityTerrain = " << ruleDeploy->getDeployTerrains().at(pick);

	return terrainRule;
} */
/*	if (lat < 0. // northern hemisphere
		&& terrainRule->getName() == "NATIVEURBAN")
	{
		Log(LOG_INFO) << ". north: switching from Native to Dawn A";
		terrainRule = _game->getRuleset()->getTerrain("DAWNURBANA");
	}
	else if (lat > 0. // southern hemisphere
		&& terrainRule->getName() == "DAWNURBANA")
	{
		Log(LOG_INFO) << ". south: switching from Dawn A to Native";
		terrainRule = _game->getRuleset()->getTerrain("NATIVEURBAN");
	} */

}
