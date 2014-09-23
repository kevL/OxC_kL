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

#include "ConfirmLandingState.h"

#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Surface.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h" // kL
#include "../Ruleset/RuleUfo.h" // kL

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Landing window.
 * @param game		- pointer to the core game
 * @param craft 	- pointer to the craft to confirm
 * @param texture	- texture of the landing site
 * @param shade		- shade of the landing site
 */
ConfirmLandingState::ConfirmLandingState(
		Craft* craft,
		int texture,
		int shade)
	:
		_craft(craft),
		_texture(texture),
		_shade(shade),
		_terrain(NULL)
{
	//Log(LOG_INFO) << "Create ConfirmLandingState()";
	// TODO: show Country & Region
	// TODO: should do buttons: Patrol or GeoscapeCraftState or Return to base.
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

	setPalette("PAL_GEOSCAPE", 3);

	add(_window);
	add(_txtBase);
	add(_txtTexture);
	add(_txtShade);
	add(_txtMessage);
	add(_txtMessage2);
	add(_txtBegin);
	add(_btnNo);
	add(_btnYes);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_txtBase->setColor(Palette::blockOffset(8)+10);
	_txtBase->setAlign(ALIGN_LEFT);
	_txtBase->setText(craft->getBase()->getName(_game->getLanguage()));

	_txtShade->setColor(Palette::blockOffset(8)+10);
	_txtShade->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtShade->setAlign(ALIGN_RIGHT);
	_txtShade->setText(tr("STR_SHADE_").arg(shade));

	_txtTexture->setColor(Palette::blockOffset(8)+10);
	_txtTexture->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtTexture->setAlign(ALIGN_RIGHT);

	Ufo* ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	if (ufo != NULL) // NOTE: all this terrain stuff can and may fall through to BattlescapeGenerator.
	{
		std::string terrain = ufo->getTerrain(); // Ufo-object stores the terrain value.
		//Log(LOG_INFO) << ". ufo VALID, terrain = " << terrain;

		if (terrain == "")
		{
			//Log(LOG_INFO) << ". . terrain NOT valid";
			std::vector<RuleTerrain*> choice;
			double lat = craft->getLatitude();
			if (texture < 0)
				texture = 0;

			const std::vector<std::string>& terrains = _game->getRuleset()->getTerrainList();
			for (std::vector<std::string>::const_iterator
					i = terrains.begin();
					i != terrains.end();
					++i)
			{
				//Log(LOG_INFO) << ". . . terrain = " << *i;
				_terrain = _game->getRuleset()->getTerrain(*i);
				for (std::vector<int>::iterator
						j = _terrain->getTextures()->begin();
						j != _terrain->getTextures()->end();
						++j)
				{
					//Log(LOG_INFO) << ". . . . texture = " << *j;
					if (*j == texture
						&& (_terrain->getHemisphere() == 0
							|| (_terrain->getHemisphere() < 0
								&& lat < 0.0)
							|| (_terrain->getHemisphere() > 0
								&& lat >= 0.0)))
					{
						//Log(LOG_INFO) << ". . . . . _terrain = " << *i;
						choice.push_back(_terrain);
					}
				}
			}

			size_t pick = static_cast<size_t>(RNG::generate(
														0,
														static_cast<int>(choice.size()) - 1));
			_terrain = choice.at(pick);
			//Log(LOG_INFO) << ". . pick = " << pick << ", choice = " << _terrain->getName();

			ufo->setTerrain(_terrain->getName());
			_txtTexture->setText(tr("STR_TEXTURE_").arg(tr(_terrain->getName())));
		}
		else
		{
			//Log(LOG_INFO) << ". . terrain VALID";
			_terrain = _game->getRuleset()->getTerrain(terrain);
			_txtTexture->setText(tr("STR_TEXTURE_").arg(tr(terrain)));
		}
	}
	else // TODO: TerrorSites will have to go in here ... they choose terrain/textures differently.
	{
		//Log(LOG_INFO) << ". ufo NOT valid";
		_txtTexture->setVisible(false);
	}

/*kL
	_txtMessage->setColor(Palette::blockOffset(8)+10);
	_txtMessage->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setWordWrap();
	_txtMessage->setText(tr("STR_CRAFT_READY_TO_LAND_NEAR_DESTINATION")
						 .arg(_craft->getName(_game->getLanguage()))
						 .arg(_craft->getDestination()->getName(_game->getLanguage()))); */
	_txtMessage->setColor(Palette::blockOffset(8)+10);
	_txtMessage->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtMessage->setBig();
	_txtMessage->setAlign(ALIGN_CENTER);
//	_txtMessage->setWordWrap();
	_txtMessage->setText(tr("STR_CRAFT_READY_TO_LAND_AT")
						 .arg(_craft->getName(_game->getLanguage())));

	_txtMessage2->setColor(Palette::blockOffset(8)+10);
	_txtMessage2->setSecondaryColor(Palette::blockOffset(5)+4);
	_txtMessage2->setBig();
	_txtMessage2->setAlign(ALIGN_CENTER);
//	_txtMessage2->setWordWrap();

	std::wostringstream ss;
	ss << L""; // blank if no UFO.
	if (ufo != NULL)
	{
		RuleUfo* ufoRule = ufo->getRules();
		if (ufoRule != NULL)
			ss << tr(ufoRule->getType()); // only ufoType shows if not hyperdetected.

		if (ufo->getHyperDetected())
			ss << L" : " << tr(ufo->getAlienRace());
	}
	_txtMessage2->setText(tr("STR_CRAFT_DESTINATION")
						 .arg(_craft->getDestination()->getName(_game->getLanguage()))
						 .arg(ss.str()));

	_txtBegin->setColor(Palette::blockOffset(8)+5);
	_txtBegin->setBig();
	_txtBegin->setAlign(ALIGN_CENTER);
	_txtBegin->setText(tr("STR_BEGIN_MISSION"));


	_btnYes->setColor(Palette::blockOffset(8)+5);
	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& ConfirmLandingState::btnYesClick);
	_btnYes->onKeyboardPress(
					(ActionHandler)& ConfirmLandingState::btnYesClick,
					Options::keyOk);

	_btnNo->setColor(Palette::blockOffset(8)+5);
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
{
}

/**
 * Make sure we aren't returning to base.
 */
void ConfirmLandingState::init()
{
	State::init();

	Base* base = dynamic_cast<Base*>(_craft->getDestination());
	if (base == _craft->getBase())
		_game->popState();
}

/**
 * Enters the mission.
 * @param action Pointer to an action.
 */
void ConfirmLandingState::btnYesClick(Action*)
{
	_game->popState();

	Ufo* ufo = dynamic_cast<Ufo*>(_craft->getDestination());
	TerrorSite* ts = dynamic_cast<TerrorSite*>(_craft->getDestination());
	AlienBase* ab = dynamic_cast<AlienBase*>(_craft->getDestination());

	SavedBattleGame* battleGame = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(battleGame);

	BattlescapeGenerator battleGen (_game); // init.

	battleGen.setWorldTerrain(_terrain); // kL
	battleGen.setWorldTexture(_texture);
	battleGen.setWorldShade(_shade);
	battleGen.setCraft(_craft);

	if (ufo != NULL)
	{
		if (ufo->getStatus() == Ufo::CRASHED)
			battleGame->setMissionType("STR_UFO_CRASH_RECOVERY");
		else
			battleGame->setMissionType("STR_UFO_GROUND_ASSAULT");

		battleGen.setUfo(ufo);
		battleGen.setAlienRace(ufo->getAlienRace());
	}
	else if (ts != NULL)
	{
		battleGame->setMissionType("STR_TERROR_MISSION");
		battleGen.setTerrorSite(ts);
		battleGen.setAlienRace(ts->getAlienRace());
	}
	else if (ab != NULL)
	{
		battleGame->setMissionType("STR_ALIEN_BASE_ASSAULT");
		battleGen.setAlienBase(ab);
		battleGen.setAlienRace(ab->getAlienRace());
	}
	else
	{
		throw Exception("No mission available!");
	}

	battleGen.run();

	_game->pushState(new BriefingState(_craft));
}

/**
 * Returns the craft to base and closes the window.
 * kL: CHANGE the craft goes into Patrol mode.
 * @param action Pointer to an action.
 */
void ConfirmLandingState::btnNoClick(Action*)
{
//kL	_craft->returnToBase();
	_craft->setDestination(NULL); // kL

	_game->popState();
}

}
