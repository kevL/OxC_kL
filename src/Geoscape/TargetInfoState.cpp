/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#include "TargetInfoState.h"
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Savegame/Target.h"
#include "../Engine/Options.h"
#include "InterceptState.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Target Info window.
 * @param game Pointer to the core game.
 * @param target Pointer to the target to show info from.
 * @param globe Pointer to the Geoscape globe.
 */
TargetInfoState::TargetInfoState(Game* game, Target* target, Globe* globe)
	:
		State(game),
		_target(target),
		_globe(globe)
{
	_screen = false;

	_window			= new Window(this, 192, 120, 32, 40, POPUP_BOTH);


	_txtTargetted	= new Text(182, 9, 37, 78);
	_txtFollowers	= new Text(182, 40, 37, 88);

	_btnIntercept	= new TextButton(160, 14, 48, 124); // new

//	_btnOk			= new TextButton(160, 16, 48, 135);
	_btnOk			= new TextButton(160, 14, 48, 140); // new


	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)), Palette::backPos, 16);

	add(_window);
	add(_btnIntercept);
	add(_txtTitle);
	add(_txtTargetted);
	add(_txtFollowers);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnIntercept->setColor(Palette::blockOffset(8)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& TargetInfoState::btnInterceptClick);

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& TargetInfoState::btnOkClick);
//	_btnOk->onKeyboardPress((ActionHandler)& TargetInfoState::btnOkClick, (SDLKey)Options::getInt("keyOk")); // removed by Sup.
	_btnOk->onKeyboardPress((ActionHandler)& TargetInfoState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(8)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
//kL	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
//kL	_txtTitle->setWordWrap(true);
	_txtTitle->setText(_target->getName(_game->getLanguage()));

	_txtTargetted->setColor(Palette::blockOffset(15)-1);
	_txtTargetted->setAlign(ALIGN_CENTER);
	_txtTargetted->setText(tr("STR_TARGETTED_BY"));

	_txtFollowers->setColor(Palette::blockOffset(15)-1);
	_txtFollowers->setAlign(ALIGN_CENTER);
	std::wostringstream ss;
	for (std::vector<Target*>::iterator i = _target->getFollowers()->begin(); i != _target->getFollowers()->end(); ++i)
	{
		ss << (*i)->getName(_game->getLanguage()) << L'\n';
	}
	_txtFollowers->setText(ss.str());
}

/**
 *
 */
TargetInfoState::~TargetInfoState()
{
}

/**
 * Pick a craft to intercept the UFO.
 * @param action, Pointer to an action.
 */
void TargetInfoState::btnInterceptClick(Action*)
{
	_game->pushState(new InterceptState(_game, _globe, 0, _target));
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void TargetInfoState::btnOkClick(Action*)
{
	_game->popState();
}

}
