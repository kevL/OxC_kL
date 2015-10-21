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

#include "MissionDetectedState.h"

#include "GeoscapeState.h"
#include "Globe.h"
#include "InterceptState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/AlienDeployment.h"

#include "../Savegame/MissionSite.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Mission Detected window.
 * @param site	- pointer to the respective Mission Site
 * @param geo	- pointer to GeoscapeState
 */
MissionDetectedState::MissionDetectedState(
		const MissionSite* const site,
		GeoscapeState* const geo)
	:
		_site(site),
		_geo(geo)
{
	_screen = false;

	_window			= new Window(this, 256, 200, 0, 0, POPUP_BOTH);
	_txtTitle		= new Text(246, 32, 5, 48);

	_txtCity		= new Text(246, 17, 5, 80);

	_btnIntercept	= new TextButton(200, 16, 28, 130);
	_btnCenter		= new TextButton(200, 16, 28, 150);
	_btnCancel		= new TextButton(200, 16, 28, 170);

	setInterface("terrorSite");

	add(_window,		"window",	"terrorSite");
	add(_txtTitle,		"text",		"terrorSite");
	add(_txtCity,		"text",		"terrorSite");
	add(_btnIntercept,	"button",	"terrorSite");
	add(_btnCenter,		"button",	"terrorSite");
	add(_btnCancel,		"button",	"terrorSite");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface(_site->getSiteDeployment()->getAlertBackground()));

	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& MissionDetectedState::btnInterceptClick);

	_btnCenter->setText(tr("STR_CENTER_ON_SITE_TIME_5_SECONDS"));
	_btnCenter->onMouseClick((ActionHandler)& MissionDetectedState::btnCenterClick);
	_btnCenter->onKeyboardPress(
					(ActionHandler)& MissionDetectedState::btnCenterClick,
					Options::keyOk);
	_btnCenter->onKeyboardPress(
					(ActionHandler)& MissionDetectedState::btnCenterClick,
					Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& MissionDetectedState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& MissionDetectedState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setWordWrap();
	_txtTitle->setText(tr(_site->getSiteDeployment()->getAlertMessage()));

	_txtCity->setBig();
	_txtCity->setAlign(ALIGN_CENTER);
	_txtCity->setText(tr(_site->getCity()));

	_geo->resetTimer();

	_game->getResourcePack()->fadeMusic(_game, 325);
	_game->getResourcePack()->playMusic(res_MUSIC_GEO_TERROR_SPLASH);

//	kL_geoMusicPlaying = false;
//	kL_geoMusicReturnState = true;
}

/**
 * dTor.
 */
MissionDetectedState::~MissionDetectedState()
{}

/**
 * Picks a craft to intercept the mission site.
 * @param action - pointer to an Action
 */
void MissionDetectedState::btnInterceptClick(Action*)
{
	_game->popState();
	_game->pushState(new InterceptState(NULL, _geo));
}

/**
 * Centers on the mission site and returns to the previous screen.
 * @param action - pointer to an Action
 */
void MissionDetectedState::btnCenterClick(Action*)
{
	_geo->getGlobe()->center(
						_site->getLongitude(),
						_site->getLatitude());
	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void MissionDetectedState::btnCancelClick(Action*)
{
	_game->popState();
}

}
