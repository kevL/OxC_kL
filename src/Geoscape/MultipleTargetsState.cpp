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

#include "MultipleTargetsState.h"

#include "ConfirmDestinationState.h"
#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "InterceptState.h"
#include "TargetInfoState.h"
#include "UfoDetectedState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

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
 * @param targets	- vector of pointers to Target for display
 * @param craft		- pointer to Craft to retarget (NULL if none)
 * @param state		- pointer to the GeoscapeState
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
			height = (BUTTON_HEIGHT * (_targets.size() + 1)) + (SPACING * (_targets.size())) + (MARGIN * 2),
			window_y = (200 - height) / 2,
			btn_y = window_y + MARGIN;

		_window = new Window(
							this,
							136,
							height,
							60,
							window_y,
							POPUP_VERTICAL);

		setInterface("UFOInfo");

		add(_window, "window", "UFOInfo");

		_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

		for (size_t
				i = 0;
				i != _targets.size();
				++i)
		{
			TextButton* btn = new TextButton(
										116,
										BUTTON_HEIGHT,
										70,
										btn_y);
			btn->setText(_targets[i]->getName(_game->getLanguage()));
			btn->onMouseClick((ActionHandler)& MultipleTargetsState::btnTargetClick);
			add(btn, "button", "UFOInfo");

			_btnTargets.push_back(btn);

			btn_y += BUTTON_HEIGHT + SPACING;
		}

		_btnCancel = new TextButton(
								116,
								BUTTON_HEIGHT,
								70,
								btn_y);
		_btnCancel->setText(tr("STR_CANCEL"));
		_btnCancel->onMouseClick((ActionHandler)& MultipleTargetsState::btnCancelClick);
		_btnCancel->onKeyboardPress(
						(ActionHandler)& MultipleTargetsState::btnCancelClick,
						Options::keyCancel);
		add(_btnCancel, "button", "UFOInfo");

		centerAllSurfaces();
	}
}

/**
 * dTor.
 */
MultipleTargetsState::~MultipleTargetsState()
{}

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
 * @param target - pointer to a Target
 */
void MultipleTargetsState::popupTarget(Target* target)
{
	_game->popState();

	if (_craft == NULL)
	{
		Base* const base = dynamic_cast<Base*>(target);
		Craft* const craft = dynamic_cast<Craft*>(target);
		Ufo* const ufo = dynamic_cast<Ufo*>(target);

		if (base != NULL)
			_game->pushState(new InterceptState(
											base,
											_state));
		else if (craft != NULL)
			_game->pushState(new GeoscapeCraftState(
											craft,
											_state));
		else if (ufo != NULL)
			_game->pushState(new UfoDetectedState(
											ufo,
											_state,
											false,
											ufo->getHyperDetected()));
		else
			_game->pushState(new TargetInfoState(
											target,
											_state));
	}
	else
		_game->pushState(new ConfirmDestinationState(
											_craft,
											target));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void MultipleTargetsState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Pick a target to display.
 * @param action - pointer to an Action
 */
void MultipleTargetsState::btnTargetClick(Action* action)
{
	for (size_t
			i = 0;
			i != _btnTargets.size();
			++i)
	{
		if (action->getSender() == _btnTargets[i])
		{
			popupTarget(_targets[i]);
			break;
		}
	}
}

}
