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
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
#include "../Engine/Surface.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/City.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/RuleTerrain.h"
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
// * @param texture	- texture of the landing site
 * @param texture	- pointer to the Texture of the landing site
 * @param shade		- shade of the landing site
 */
ConfirmLandingState::ConfirmLandingState(
		Craft* const craft,
//		const int texture,
		Texture* texture,
		const int shade)
	:
		_craft(craft),
		_texture(texture),
		_shade(shade),
		_terrain(NULL),
		_city(false)
{
	//Log(LOG_INFO) << "Create ConfirmLandingState()";
	// TODO: show Country & Region
	// TODO: should do buttons: Patrol (isCancel already) or GeoscapeCraftState or Return to base.
	_screen = false;

	_window			= new Window(this, 230, 160, 13, 20, POPUP_BOTH);

	_txtBase		= new Text(100, 9, 23, 29);
	_txtTexture		= new Text(130, 9, 103, 29);
	_txtShade		= new Text(60, 9, 173, 39);

	_txtMessage		= new Text(206, 40, 25, 47);
	_txtMessage2	= new Text(206, 43, 25, 87);

	_txtBegin		= new Text(206, 17, 25, 130);

	_btnNo			= new TextButton(80, 18, 40, 152);
	_btnYes			= new TextButton(80, 18, 136, 152);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("confirmLanding")->getElement("palette")->color);

	add(_window, "window", "confirmLanding");
	add(_txtBase, "text", "confirmLanding");
	add(_txtTexture, "text", "confirmLanding");
	add(_txtShade, "text", "confirmLanding");
	add(_txtMessage, "text", "confirmLanding");
	add(_txtMessage2, "text", "confirmLanding");
	add(_txtBegin, "text", "confirmLanding");
	add(_btnNo, "button", "confirmLanding");
	add(_btnYes, "button", "confirmLanding");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_txtBase->setAlign(ALIGN_LEFT);
	_txtBase->setText(craft->getBase()->getName(_game->getLanguage()));

	_txtShade->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtShade->setAlign(ALIGN_RIGHT);
	_txtShade->setText(tr("STR_SHADE_").arg(shade));

	_txtTexture->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtTexture->setAlign(ALIGN_RIGHT);

	// NOTE: the following terrain stuff can and may* fall through to BattlescapeGenerator.
	// *for Base assault/defense and Cydonia missions; here is concerned only with UFOs and TerrorSites

	Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	MissionSite* const site = dynamic_cast<MissionSite*>(_craft->getDestination());

	if (ufo != NULL
		|| site != NULL)
	{
		double lat;
		std::string terrain;

		if (ufo != NULL)
		{
			Log(LOG_INFO) << ". ufo VALID, tex " << _texture;
			terrain = ufo->getTerrain(); // Ufo-object stores the terrain value.
			if (terrain.empty() == true)
			{
				const double lon = ufo->getLongitude();
				lat = ufo->getLatitude();

				for (std::vector<Region*>::const_iterator
						i = _game->getSavedGame()->getRegions()->begin();
						i != _game->getSavedGame()->getRegions()->end()
							&& _city == false;
						++i)
				{
					if ((*i)->getRules()->insideRegion(
													lon,
													lat) == true)
					{
						for (std::vector<City*>::const_iterator
								j = (*i)->getRules()->getCities()->begin();
								j != (*i)->getRules()->getCities()->end()
									&& _city == false;
								++j)
						{
							if (AreSame(lon, (*j)->getLongitude())
								&& AreSame(lat, (*j)->getLatitude()))
							{
								_city = true;
							}
						}
					}
				}
			}
			else // ufo's terrainType has already been set before
			{
				Log(LOG_INFO) << ". . terrain VALID: " << terrain;
				_terrain = _game->getRuleset()->getTerrain(terrain);
			}
		}
		else // site != NULL
		{
			Log(LOG_INFO) << ". site VALID, tex " << _texture;
			terrain = site->getTerrain(); // TerrorSite-object stores the terrain value.
			if (terrain.empty() == true)
			{
				lat = site->getLatitude();
				_city = true;
			}
			else // terrorSite's terrainType has already been set before
			{
				Log(LOG_INFO) << ". . terrain VALID: " << terrain;
				_terrain = _game->getRuleset()->getTerrain(terrain);
			}
		}

		if (_terrain == NULL)
		{
			if (_city == true) // use TerrorSite terrains for UFOs' _city missions.
			{
				Log(LOG_INFO) << ". . . TS or city VALID";
//->			_terrain = selectCityTerrain(lat);
			}
			else // ufo
			{
				Log(LOG_INFO) << ". . . UFO is NOT at a City";
//->			_terrain = selectTerrain(lat);
			}

			if (ufo != NULL)
				ufo->setTerrain(_terrain->getName());
			else // terrorSite
				site->setTerrain(_terrain->getName());
		}

		_txtTexture->setText(tr("STR_TEXTURE_").arg(tr(_terrain->getName())));
	}
	else
	{
		Log(LOG_INFO) << ". ufo/terrorsite NOT valid";
		_txtTexture->setVisible(false);
		_txtShade->setVisible(false);
	}

//	_txtMessage->setColor(Palette::blockOffset(8)+10);
//	_txtMessage->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setText(tr("STR_CRAFT_READY_TO_LAND_AT")
						 .arg(_craft->getName(_game->getLanguage())));

//	_txtMessage2->setColor(Palette::blockOffset(8)+10);
//	_txtMessage2->setSecondaryColor(Palette::blockOffset(5)+4);
	_txtMessage2->setBig();
	_txtMessage2->setAlign(ALIGN_CENTER);

	std::wostringstream wosts;
	wosts << L""; // blank if no UFO.
	if (ufo != NULL)
	{
		const RuleUfo* const ufoRule = ufo->getRules();
		if (ufoRule != NULL)
			wosts << tr(ufoRule->getType()); // only ufoType shows if not hyperdetected.

		if (ufo->getHyperDetected() == true)
			wosts << L" : " << tr(ufo->getAlienRace());
	}
	_txtMessage2->setText(tr("STR_CRAFT_DESTINATION")
						 .arg(_craft->getDestination()->getName(_game->getLanguage()))
						 .arg(wosts.str()));


	_txtBegin->setBig();
	_txtBegin->setAlign(ALIGN_CENTER);
	_txtBegin->setText(tr("STR_BEGIN_MISSION"));

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
 * Make sure we aren't returning to base.
 */
void ConfirmLandingState::init()
{
	State::init();

	const Base* const base = dynamic_cast<Base*>(_craft->getDestination());
	if (base == _craft->getBase())
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
														static_cast<int>(ruleDeploy->getTerrains().size()) - 1));
	RuleTerrain* const terrainRule = _game->getRuleset()->getTerrain(ruleDeploy->getTerrains().at(pick));
	Log(LOG_INFO) << "cityTerrain = " << ruleDeploy->getTerrains().at(pick);

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
	AlienBase* const abase = dynamic_cast<AlienBase*>(_craft->getDestination());

	SavedBattleGame* battle = new SavedBattleGame(&_game->getRuleset()->getOperations());
	_game->getSavedGame()->setBattleGame(battle);

	BattlescapeGenerator bGen (_game); // init.

	bGen.setSiteTerrain(_terrain); // kL
	bGen.setSiteTexture(_texture);
	bGen.setSiteShade(_shade);
	bGen.setCraft(_craft);

	if (ufo != NULL)
	{
		if (ufo->getStatus() == Ufo::CRASHED)
			battle->setMissionType("STR_UFO_CRASH_RECOVERY");
		else
			battle->setMissionType("STR_UFO_GROUND_ASSAULT");

		bGen.setUfo(ufo);
		bGen.setAlienRace(ufo->getAlienRace());
		bGen.setIsCity(_city); // kL
	}
	else if (site != NULL)
	{
		battle->setMissionType(site->getDeployment()->getType());
		bGen.setMissionSite(site);
		bGen.setAlienRace(site->getAlienRace());
	}
	else if (abase != NULL)
	{
		battle->setMissionType("STR_ALIEN_BASE_ASSAULT");
		bGen.setAlienBase(abase);
		bGen.setAlienRace(abase->getAlienRace());

		bGen.setSiteTexture(NULL);
	}
	else
	{
		throw Exception("No mission available!");
	}

	bGen.run(); // <- DETERMINE ALL TACTICAL DATA.

	_game->pushState(new BriefingState(_craft));
}

/**
 * Returns the craft to base and closes the window.
 * kL CHANGE: the craft goes into Patrol mode.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnNoClick(Action*)
{
//	_craft->returnToBase();
	_craft->setDestination(NULL); // kL
	_game->popState();
}

}
