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

#include "TargetInfoState.h"

#include "GeoscapeState.h"
#include "InterceptState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Target Info window.
 * @param target	- pointer to a Target to show info about
 * @param state		- pointer to GeoscapeState
 */
TargetInfoState::TargetInfoState(
		Target* const target,
		GeoscapeState* const state)
	:
		_target(target),
		_state(state),
		_aBase(NULL)
{
	_screen = false;

	_window			= new Window(this, 192, 120, 32, 40, POPUP_BOTH);
	_txtTitle		= new Text(182, 17, 37, 54);

	_edtTarget		= new TextEdit(this, 50, 9, 38, 46);

	_txtTargetted	= new Text(182, 9, 37, 71);
	_txtFollowers	= new Text(182, 40, 37, 82);

	_btnIntercept	= new TextButton(160, 16, 48, 119);
	_btnOk			= new TextButton(160, 16, 48, 137);

	setInterface("targetInfo");

	add(_window,		"window",	"targetInfo");
	add(_txtTitle,		"text",		"targetInfo");
	add(_edtTarget,		"text",		"targetInfo");
	add(_txtTargetted,	"text",		"targetInfo");
	add(_txtFollowers,	"text",		"targetInfo");
	add(_btnIntercept,	"button",	"targetInfo");
	add(_btnOk,			"button",	"targetInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnIntercept->setColor(Palette::blockOffset(8)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& TargetInfoState::btnInterceptClick);

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick((ActionHandler)& TargetInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TargetInfoState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	std::wostringstream woststr;
	woststr << L'\x01' << _target->getName(_game->getLanguage());
	_txtTitle->setText(woststr.str().c_str());

	_edtTarget->setVisible(false);
	for (std::vector<AlienBase*>::const_iterator
			aBase = _game->getSavedGame()->getAlienBases()->begin();
			aBase != _game->getSavedGame()->getAlienBases()->end();
			++aBase)
	{
		if (_target == dynamic_cast<Target*>(*aBase))
		{
			_aBase = *aBase;

			const std::wstring edit = Language::utf8ToWstr((*aBase)->getLabel());
			_edtTarget->setText(edit);
			_edtTarget->onChange((ActionHandler)& TargetInfoState::edtTargetChange);
			_edtTarget->setVisible();

			break;
		}
	}

	bool targeted = false;

	_txtFollowers->setAlign(ALIGN_CENTER);
	woststr.str(L"");
	for (std::vector<Target*>::const_iterator
			i = _target->getFollowers()->begin();
			i != _target->getFollowers()->end();
			++i)
	{
		woststr << (*i)->getName(_game->getLanguage()) << L'\n';

		if (targeted == false)
		{
			targeted = true;
			_txtTargetted->setAlign(ALIGN_CENTER);
			_txtTargetted->setText(tr("STR_TARGETTED_BY"));
		}
	}
	_txtFollowers->setText(woststr.str());

	if (targeted == false)
		_txtTargetted->setVisible(false);
}

/**
 * dTor.
 */
TargetInfoState::~TargetInfoState()
{}

/**
 * Edits an aLienBase's name.
 * @param action - pointer to an Action
 */
void TargetInfoState::edtTargetChange(Action*)
{
	_aBase->setLabel(Language::wstrToUtf8(_edtTarget->getText()));
}

/**
 * Picks a craft to intercept the UFO.
 * @param action, Pointer to an action.
 */
void TargetInfoState::btnInterceptClick(Action*)
{
	_state->resetTimer();

	_game->popState();
	_game->pushState(new InterceptState(
									NULL,
									_state));
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void TargetInfoState::btnOkClick(Action*)
{
	_game->popState();
}

}
