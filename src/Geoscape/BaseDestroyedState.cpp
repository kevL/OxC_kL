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

#include "BaseDestroyedState.h"
#include "Globe.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

BaseDestroyedState::BaseDestroyedState(
		Base* base,
		Globe* globe)
	:
		_base(base),
		_globe(globe)
{
	_screen = false;

	_window		= new Window(this, 246, 160, 35, 20); // note, these are offset a few px left.
	_txtMessage	= new Text(224, 106, 46, 26);
	_btnCenter	= new TextButton(140, 16, 88, 133);
	_btnOk		= new TextButton(140, 16, 88, 153);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("UFOInfo")->getElement("palette")->color); //7

	add(_window, "window", "UFOInfo");
	add(_txtMessage, "text", "UFOInfo");
	add(_btnCenter, "button", "UFOInfo");
	add(_btnOk, "button", "UFOInfo");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(
						_game->getResourcePack()->getSurface("BACK15.SCR"),
						37); // kL: x-offset

//	_txtMessage->setColor(Palette::blockOffset(8)+5);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
	_txtMessage->setText(tr("STR_THE_ALIENS_HAVE_DESTROYED_THE_UNDEFENDED_BASE")
							.arg(_base->getName()));

//	_btnCenter->setColor(Palette::blockOffset(8)+5);
	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& BaseDestroyedState::btnCenterClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDestroyedState::btnCenterClick,
					Options::keyOk);

//	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseDestroyedState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDestroyedState::btnOkClick,
					Options::keyCancel);


	std::vector<Region*>::iterator region = _game->getSavedGame()->getRegions()->begin();
	for (
			;
			region != _game->getSavedGame()->getRegions()->end();
			++region)
	{
		if ((*region)->getRules()->insideRegion(
										base->getLongitude(),
										base->getLatitude()))
		{
			break;
		}
	}

	AlienMission* am = _game->getSavedGame()->getAlienMission(
														(*region)->getRules()->getType(),
														"STR_ALIEN_RETALIATION");
	for (std::vector<Ufo*>::iterator
			ufo = _game->getSavedGame()->getUfos()->begin();
			ufo != _game->getSavedGame()->getUfos()->end();)
	{
		if ((*ufo)->getMission() == am)
		{
			delete *ufo;
			ufo = _game->getSavedGame()->getUfos()->erase(ufo);
		}
		else
			++ufo;
	}

	for (std::vector<AlienMission*>::iterator
			mission = _game->getSavedGame()->getAlienMissions().begin();
			mission != _game->getSavedGame()->getAlienMissions().end();
			++mission)
	{
		if (dynamic_cast<AlienMission*>(*mission) == am)
		{
			delete *mission;
			_game->getSavedGame()->getAlienMissions().erase(mission);

			break;
		}
	}
}

/**
 * dTor.
 */
BaseDestroyedState::~BaseDestroyedState()
{}

/**
 *
 */
void BaseDestroyedState::finish()
{
	_game->popState();

	for (std::vector<Base*>::iterator
			base = _game->getSavedGame()->getBases()->begin();
			base != _game->getSavedGame()->getBases()->end();
			++base)
	{
		if (*base == _base)
		{
			delete *base;
			_game->getSavedGame()->getBases()->erase(base);

			// SHOULD PUT IN A SECTION FOR TRANSFERRING AIRBORNE CRAFT TO ANOTHER BASE/S
			// if Option:: transfer airborne craft == true
			// & stores available (else sell)
			// & living quarters available (if soldiers true) else Sack.

			break;
		}
	}


//	pts = (_game->getRuleset()->getAlienMission("STR_ALIEN_TERROR")->getPoints() * 50) + (diff * 200);
	const int aLienPts = 200 + (_game->getSavedGame()->getDifficulty() * 200);

	const double
		lon = _base->getLongitude(),
		lat = _base->getLatitude();

	for (std::vector<Region*>::iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										lon,
										lat))
		{
			(*i)->addActivityAlien(aLienPts);
			(*i)->recentActivity();

			break;
		}
	}

	for (std::vector<Country*>::iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(
										lon,
										lat))
		{
			(*i)->addActivityAlien(aLienPts);
			(*i)->recentActivity();

			break;
		}
	}
}

/**
 * Deletes the base and scores aLien victory points.
 * @param action - pointer to an Action
 */
void BaseDestroyedState::btnOkClick(Action*)
{
	finish();
}

/**
 * Deletes the base and scores aLien victory points, and centers the Globe.
 * @param action - pointer to an Action
 */
void BaseDestroyedState::btnCenterClick(Action*)
{
	_globe->center(
				_base->getLongitude(),
				_base->getLatitude());

	finish();
}

}
