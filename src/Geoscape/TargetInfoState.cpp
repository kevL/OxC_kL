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

#include "TargetInfoState.h"

#include "GeoscapeState.h"
#include "InterceptState.h"

#include "../Engine/Action.h" // kL
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h" // kL
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/AlienBase.h" // kL
#include "../Savegame/SavedGame.h" // kL
#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Target Info window.
 * @param game, Pointer to the core game.
 * @param target, Pointer to the target to show info from.
 * @param globe, Pointer to the Geoscape globe.
 */
TargetInfoState::TargetInfoState(
		Target* target,
		Globe* globe,
		GeoscapeState* state)
	:
		_target(target),
		_globe(globe),
		_state(state),
		_ab(NULL)
{
	//Log(LOG_INFO) << "Create TargetInfoState";
	_screen = false;

	_window			= new Window(this, 192, 120, 32, 40, POPUP_BOTH);
	_txtTitle		= new Text(182, 17, 37, 54);

	_edtTarget		= new TextEdit(this, 50, 9, 38, 46); // kL

	_txtTargetted	= new Text(182, 9, 37, 71);
	_txtFollowers	= new Text(182, 40, 37, 82);

	_btnIntercept	= new TextButton(160, 16, 48, 119);
	_btnOk			= new TextButton(160, 16, 48, 137);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_edtTarget); // kL
	add(_txtTargetted);
	add(_txtFollowers);
	add(_btnIntercept);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(8)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnIntercept->setColor(Palette::blockOffset(8)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& TargetInfoState::btnInterceptClick);

	_btnOk->setColor(Palette::blockOffset(8)+5);
//kL	_btnOk->setText(tr("STR_CANCEL_UC"));
	_btnOk->setText(tr("STR_OK")); // kL
	_btnOk->onMouseClick((ActionHandler)& TargetInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TargetInfoState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
//kL	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
//kL	_txtTitle->setWordWrap();
	_txtTitle->setText(_target->getName(_game->getLanguage()));

	_edtTarget->setVisible(false);
	for (std::vector<AlienBase*>::const_iterator
			ab = _game->getSavedGame()->getAlienBases()->begin();
			ab != _game->getSavedGame()->getAlienBases()->end();
			++ab)
	{
		if (_target->getName(_game->getLanguage()) == (*ab)->getName(_game->getLanguage()))
		{
			_ab = *ab;

			std::wstring edit = Language::utf8ToWstr((*ab)->getLabel());
			_edtTarget->setText(edit);

			_edtTarget->setColor(Palette::blockOffset(15)+1);
			_edtTarget->onChange((ActionHandler)& TargetInfoState::edtTargetChange);

			_edtTarget->setVisible();

			break;
		}
	}

	bool targeted = false;

	_txtFollowers->setColor(Palette::blockOffset(15)+5);
	_txtFollowers->setAlign(ALIGN_CENTER);
	std::wostringstream ss;
	for (std::vector<Target*>::iterator
			i = _target->getFollowers()->begin();
			i != _target->getFollowers()->end();
			++i)
	{
		ss << (*i)->getName(_game->getLanguage()) << L'\n';

		if (!targeted)
		{
			targeted = true;

			_txtTargetted->setColor(Palette::blockOffset(15)-1);
			_txtTargetted->setAlign(ALIGN_CENTER);
			_txtTargetted->setText(tr("STR_TARGETTED_BY"));
		}
	}
	_txtFollowers->setText(ss.str());

	if (!targeted)
		_txtTargetted->setVisible(false);

	//Log(LOG_INFO) << "Create TargetInfoState EXIT";
}

/**
 * dTor.
 */
TargetInfoState::~TargetInfoState()
{
}

/**
 * Changes the base name.
 * @param action Pointer to an action.
 */
void TargetInfoState::edtTargetChange(Action* action)
{
//	_edtTarget->setText(_editTarget->getText()); // NAH. This should be set auto via TextEdit

	std::string edit = Language::wstrToUtf8(_edtTarget->getText());
	_ab->setLabel(edit);


/*	if (action->getDetails()->key.keysym.sym == SDLK_RETURN
		|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		std::string s = Language::wstrToUtf8(_edtTarget->getText());
		_ab->setEdit(s);
	} */
}
/*
 * Changes the soldier's name.
 * @param action Pointer to an action.
void SoldierInfoState::edtSoldierChange(Action* action)
{
	_soldier->setName(_edtSoldier->getText());
}

 * Changes the Craft name.
 * @param action Pointer to an action.
void CraftInfoState::edtCraftChange(Action* action)
{
	_craft->setName(_edtCraft->getText());

	if (_craft->getName(_game->getLanguage()) == _defaultName)
		_craft->setName(L"");

	if (action->getDetails()->key.keysym.sym == SDLK_RETURN
		|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		_edtCraft->setText(_craft->getName(_game->getLanguage()));
	}
}

 * Updates the base name and disables the OK button if no name is entered.
 * @param action Pointer to an action.
void BaseNameState::edtNameChange(Action* action)
{
	_base->setName(_edtName->getText());

	if (action->getDetails()->key.keysym.sym == SDLK_RETURN
		|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		if (!_edtName->getText().empty())
			btnOkClick(action);
	}
	else
		_btnOk->setVisible(!_edtName->getText().empty());
} */

/**
 * Pick a craft to intercept the UFO.
 * @param action, Pointer to an action.
 */
void TargetInfoState::btnInterceptClick(Action*)
{
/*	if (_ab)
	{
		std::string s = Language::wstrToUtf8(_edtTarget->getText());
		_ab->setEdit(s);
	} */ // kL

	_state->timerReset();

	_game->popState();
	_game->pushState(new InterceptState(
									_globe,
									NULL,
									_target,
									_state)); // kL_add.
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void TargetInfoState::btnOkClick(Action*)
{
/*	if (_ab)
	{
		std::string s = Language::wstrToUtf8(_edtTarget->getText());
		_ab->setEdit(s);
	} */ // kL

	_game->popState();
}

}
