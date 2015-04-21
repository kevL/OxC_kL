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

#include "PsiTrainingState.h"

//#include <sstream>

#include "AllocatePsiTrainingState.h"
#include "GeoscapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Psi Training screen.
 */
PsiTrainingState::PsiTrainingState()
{
	_window		= new Window(this, 320, 200);
	_txtTitle	= new Text(300, 17, 10, 16);
	_btnOk		= new TextButton(160, 14, 80, 174);

	setInterface("psiTraining");

	add(_window,	"window",	"psiTraining");
	add(_txtTitle,	"text",		"psiTraining");
	add(_btnOk,		"button2",	"psiTraining");


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& PsiTrainingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PsiTrainingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PSIONIC_TRAINING"));


	int buttons = 0;

	for(std::vector<Base*>::const_iterator
			b = _game->getSavedGame()->getBases()->begin();
			b != _game->getSavedGame()->getBases()->end();
			++b)
	{
		if ((*b)->getAvailablePsiLabs())
		{
			TextButton* btnBase = new TextButton(
											160,14,
											80,
											40 + 16 * buttons);
			btnBase->onMouseClick((ActionHandler)& PsiTrainingState::btnBaseXClick);
			btnBase->setText((*b)->getName());
			add(btnBase, "button1", "psiTraining");

			_bases.push_back(*b);
			_btnBases.push_back(btnBase);

			if (++buttons > 7)
				break;
		}
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
PsiTrainingState::~PsiTrainingState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void PsiTrainingState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the allocation screen for the corresponding base.
 * @param action - pointer to an Action
 */
void PsiTrainingState::btnBaseXClick(Action* action)
{
	for (size_t
			i = 0;
			i != _btnBases.size();
			++i)
	{
		if (action->getSender() == _btnBases[i])
		{
			_game->pushState(new AllocatePsiTrainingState(_bases.at(i)));
			break;
		}
	}
}

}
