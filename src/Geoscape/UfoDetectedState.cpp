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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UfoDetectedState.h"

#include <sstream>

#include "GeoscapeState.h"
#include "Globe.h"
#include "InterceptState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleUfo.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Ufo Detected window.
 * @param game Pointer to the core game.
 * @param ufo Pointer to the UFO to get info from.
 * @param state Pointer to the Geoscape.
 * @param detected Was the UFO detected?
 * @param hyper Was it a hyperwave radar?
 */
UfoDetectedState::UfoDetectedState(
		Game* game,
		Ufo* ufo,
		GeoscapeState* state,
		bool detected,
		bool hyper)
	:
		State(game),
		_ufo(ufo),
		_state(state),
		_hyperwave(hyper)
{
	// Generate UFO ID
	if (_ufo->getId() == 0)
	{
		_ufo->setId(_game->getSavedGame()->getId("STR_UFO"));
	}

	if (_ufo->getAltitude() == "STR_GROUND"
		&& _ufo->getLandId() == 0)
	{
		_ufo->setLandId(_game->getSavedGame()->getId("STR_LANDING_SITE"));
	}

	_screen = false;

	if (hyper)
	{
		_window			= new Window(this, 224, 170, 16, 10, POPUP_BOTH);

		_txtHyperwave	= new Text(216, 17, 20, 45);
		_lstInfo2		= new TextList(192, 33, 32, 98);
	}
	else
	{
		_window		= new Window(this, 224, 120, 16, 48, POPUP_BOTH);
	}

	_txtUfo			= new Text(208, 17, 32, 56);

	_txtDetected	= new Text(80, 9, 32, 73);

	_lstInfo		= new TextList(192, 33, 32, 85);

	_btnCentre		= new TextButton(192, 16, 32, 124);

	_btnIntercept	= new TextButton(88, 16, 32, 144);
	_btnCancel		= new TextButton(88, 16, 136, 144);

	if (hyper)
	{
		_txtUfo->setY(19);
		_txtDetected->setY(36);
		_lstInfo->setY(60);
		_btnCentre->setY(135);
		_btnIntercept->setY(155);
		_btnCancel->setY(155);

		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(2)),
					Palette::backPos,
					16);
	}
	else
	{
		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
					Palette::backPos,
					16);
	}


	add(_window);
	add(_txtUfo);
	add(_txtDetected);
	add(_lstInfo);
	add(_btnIntercept);
	add(_btnCentre);
	add(_btnCancel);

	if (hyper)
	{
		add(_txtHyperwave);
		add(_lstInfo2);
	}

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_txtUfo->setColor(Palette::blockOffset(8)+5);
	_txtUfo->setBig();
	_txtUfo->setText(_ufo->getName(_game->getLanguage()));

	_txtDetected->setColor(Palette::blockOffset(8)+5);
	if (detected)
	{
		_txtDetected->setText(tr("STR_DETECTED"));
	}
	else
	{
		_txtDetected->setText(L"");
	}

	_lstInfo->setColor(Palette::blockOffset(8)+5);
	_lstInfo->setColumns(2, 80, 112);
	_lstInfo->setDot(true);
	_lstInfo->addRow(
					2,
					tr("STR_SIZE_UC").c_str(),
					tr(_ufo->getRules()->getSize()).c_str());
	_lstInfo->setCellColor(0, 1, Palette::blockOffset(8)+10);

	std::string alt = _ufo->getAltitude();
	if (alt == "STR_GROUND")
		alt = "STR_GROUNDED";
	_lstInfo->addRow(
					2,
					tr("STR_ALTITUDE").c_str(),
					tr(alt).c_str());
	_lstInfo->setCellColor(1, 1, Palette::blockOffset(8)+10);

	std::string heading = _ufo->getDirection();
	if (_ufo->getStatus() != Ufo::FLYING)
	{
		heading = "STR_NONE_UC";
	}
	_lstInfo->addRow(
					2,
					tr("STR_HEADING").c_str(),
					tr(heading).c_str());
	_lstInfo->setCellColor(2, 1, Palette::blockOffset(8)+10);

	_lstInfo->addRow(
					2,
					tr("STR_SPEED").c_str(),
					Text::formatNumber(_ufo->getSpeed()).c_str());
	_lstInfo->setCellColor(3, 1, Palette::blockOffset(8)+10);

	_btnIntercept->setColor(Palette::blockOffset(8)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& UfoDetectedState::btnInterceptClick);

	_btnCentre->setColor(Palette::blockOffset(8)+5);
	_btnCentre->setText(tr("STR_CENTER_ON_UFO_TIME_5_SECONDS"));
	_btnCentre->onMouseClick((ActionHandler)& UfoDetectedState::btnCentreClick);
	_btnCentre->onKeyboardPress(
					(ActionHandler)& UfoDetectedState::btnCentreClick,
					(SDLKey)Options::getInt("keyOk"));

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& UfoDetectedState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& UfoDetectedState::btnCancelClick,
					(SDLKey)Options::getInt("keyCancel"));

	if (hyper)
	{
		_txtHyperwave->setColor(Palette::blockOffset(8)+5);
		_txtHyperwave->setAlign(ALIGN_CENTER);
		_txtHyperwave->setWordWrap(true);
		_txtHyperwave->setText(tr("STR_HYPER_WAVE_TRANSMISSIONS_ARE_DECODED"));

		_lstInfo2->setColor(Palette::blockOffset(8)+5);
		_lstInfo2->setColumns(2, 80, 112);
		_lstInfo2->setDot(true);
		_lstInfo2->addRow(
						2,
						tr("STR_CRAFT_TYPE").c_str(),
						tr(_ufo->getRules()->getType()).c_str());
		_lstInfo2->setCellColor(0, 1, Palette::blockOffset(8)+10);
		_lstInfo2->addRow(
						2,
						tr("STR_RACE").c_str(),
						tr(_ufo->getAlienRace()).c_str());
		_lstInfo2->setCellColor(1, 1, Palette::blockOffset(8)+10);
		_lstInfo2->addRow(
						2,
						tr("STR_MISSION").c_str(),
						tr(_ufo->getMissionType()).c_str());
		_lstInfo2->setCellColor(2, 1, Palette::blockOffset(8)+10);
		_lstInfo2->addRow(
						2,
						tr("STR_ZONE").c_str(),
						tr(_ufo->getMission()->getRegion()).c_str());
		_lstInfo2->setCellColor(3, 1, Palette::blockOffset(8)+10);
	}
}

/**
 *
 */
UfoDetectedState::~UfoDetectedState()
{
}

/**
 * Resets the palette.
 */
void UfoDetectedState::init()
{
	if (_hyperwave)
	{
		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
					Palette::backPos,
					16);
	}
	else
	{
		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
					Palette::backPos,
					16);
	}
}

/**
 * Pick a craft to intercept the UFO.
 * @param action Pointer to an action.
 */
void UfoDetectedState::btnInterceptClick(Action*)
{
	_state->timerReset();
//kL	_state->getGlobe()->center(_ufo->getLongitude(), _ufo->getLatitude());

	_game->popState();
	_game->pushState(new InterceptState(
									_game,
									_state->getGlobe(),
									0,
									_ufo));
}

/**
 * Centers the globe on the UFO and returns to the previous screen.
 * @param action Pointer to an action.
 */
void UfoDetectedState::btnCentreClick(Action*)
{
	_state->timerReset();
	_state->getGlobe()->center(
							_ufo->getLongitude(),
							_ufo->getLatitude());

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void UfoDetectedState::btnCancelClick(Action*)
{
	_game->popState();
}

}
