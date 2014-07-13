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

#include "MultipleTargetsState.h"

#include <sstream>

#include "ConfirmDestinationState.h"
#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "InterceptState.h"
#include "TargetInfoState.h"
#include "UfoDetectedState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Multiple Targets window.
 * @param game, Pointer to the core game.
 * @param targets, List of targets to display.
 * @param craft, Pointer to craft to retarget (NULL if none).
 * @param state, Pointer to the Geoscape state.
 */
MultipleTargetsState::MultipleTargetsState(
		std::vector<Target*> targets,
		Craft* craft,
		GeoscapeState* state)
	:
		_targets(targets),
		_craft(craft),
		_state(state)
{
	_screen = false;

	if (_targets.size() > 1)
	{
		int
//kL		height	= BUTTON_HEIGHT * _targets.size() + SPACING * (_targets.size() - 1) + MARGIN * 2,
			height	= (BUTTON_HEIGHT * (_targets.size() + 1)) + (SPACING * (_targets.size())) + (MARGIN * 2), // kL
			winY	= (200 - height) / 2,
			btnY	= winY + MARGIN;

		_window = new Window(
							this,
							136,
							height,
							60,
							winY,
							POPUP_VERTICAL);

		setPalette("PAL_GEOSCAPE", 7);

		add(_window);

		_window->setColor(Palette::blockOffset(8)+5);
		_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

		for (size_t
				i = 0;
				i < _targets.size();
				++i)
		{
			TextButton* btn = new TextButton(
										116,
										BUTTON_HEIGHT,
										70,
										btnY);
			btn->setColor(Palette::blockOffset(8)+5);
			btn->setText(_targets[i]->getName(_game->getLanguage()));
			btn->onMouseClick((ActionHandler)& MultipleTargetsState::btnTargetClick);

			add(btn);

			_btnTargets.push_back(btn);

			btnY += BUTTON_HEIGHT + SPACING;
		}

		// kL_begin:
		TextButton* _btnCancel = new TextButton(
											116,
											BUTTON_HEIGHT,
											70,
											btnY);
		_btnCancel->setColor(Palette::blockOffset(8)+5);
		_btnCancel->setText(tr("STR_CANCEL"));
		_btnCancel->onMouseClick((ActionHandler)& MultipleTargetsState::btnCancelClick);
		_btnCancel->onKeyboardPress(
						(ActionHandler)& MultipleTargetsState::btnCancelClick,
						Options::keyCancel);

		add(_btnCancel);
		// kL_end.
/*kL
		_btnTargets[0]->onKeyboardPress(
						(ActionHandler)& MultipleTargetsState::btnCancelClick,
						Options::keyCancel); */

		centerAllSurfaces();
	}
}

/**
 * dTor.
 */
MultipleTargetsState::~MultipleTargetsState()
{
}

/**
 * Resets the palette and ignores the window if there's only one target.
 */
void MultipleTargetsState::init()
{
	if (_targets.size() == 1)
		popupTarget(*_targets.begin());
	else
		State::init();
}

/**
 * Displays the right popup for a specific target.
 * @param target, Pointer to target.
 */
void MultipleTargetsState::popupTarget(Target* target)
{
	//Log(LOG_INFO) << "MultipleTargetsState::popupTarget()";
	_game->popState();

	if (_craft == NULL)
	{
		//Log(LOG_INFO) << ". _craft == 0";
		Craft* c	= dynamic_cast<Craft*>(target);
		Ufo* u		= dynamic_cast<Ufo*>(target);
		Base* b		= dynamic_cast<Base*>(target);
		//Log(LOG_INFO) << ". dynamic_cast's examined";

		if (b != NULL)
		{
			//Log(LOG_INFO) << ". . base";
//			_game->popState(); // kL
			_game->pushState(new InterceptState(
											_state->getGlobe(),
											b,
											NULL,
											_state)); // kL_add.
		}
		else if (c != NULL)
		{
			//Log(LOG_INFO) << ". . craft";
			// kL_begin:
//			bool doublePop = true;
//			if (_targets.size() == 1) // uh This never happens.
//			{
//				doublePop = false;
//				_game->popState();
//			} // kL_end.

//			_game->popState(); // kL
			_game->pushState(new GeoscapeCraftState(
												c,
												_state->getGlobe(),
												NULL,
												_state,
												false)); // kL_add.
//												doublePop)); // kL_add.
		}
		else if (u != NULL)
		{
			//Log(LOG_INFO) << ". . ufo";
//			_game->popState(); // kL
			_game->pushState(new UfoDetectedState(
												u,
												_state,
												false,
												u->getHyperDetected()));
		}
		else
		{
			//Log(LOG_INFO) << ". . else...";
//kL		_game->pushState(new TargetInfoState(_game, target, _state->getGlobe()));
//			_game->popState(); // kL
			_game->pushState(new TargetInfoState(
												target,
												_state->getGlobe(),
												_state)); // kL
		}
	}
	else
	{
		//Log(LOG_INFO) << ". _craft != 0";
//		_game->popState(); // kL
		_game->pushState(new ConfirmDestinationState(
													_craft,
													target));
	}
	//Log(LOG_INFO) << "MultipleTargetsState::popupTarget() EXIT";
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MultipleTargetsState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Pick a target to display.
 * @param action Pointer to an action.
 */
void MultipleTargetsState::btnTargetClick(Action* action)
{
	//Log(LOG_INFO) << "MultipleTargetsState::lstTargetsClick()";
	for (size_t
			i = 0;
			i < _btnTargets.size();
			++i)
	{
		if (action->getSender() == _btnTargets[i])
		{
			popupTarget(_targets[i]);

			break;
		}
	}
	//Log(LOG_INFO) << "MultipleTargetsState::lstTargetsClick() EXIT";
}

}
