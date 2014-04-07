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
//#include "../Interface/TextList.h"
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
		Game* game,
		std::vector<Target*> targets,
		Craft* craft,
		GeoscapeState* state)
	:
		State(game),
		_targets(targets),
		_craft(craft),
		_state(state)
{
	_screen = false;

	if (_targets.size() > 1)
	{
		int winHeight = BUTTON_HEIGHT * _targets.size() + SPACING * (_targets.size() - 1) + MARGIN * 2;
		int winY = (200 - winHeight) / 2;
		int btnY = winY + MARGIN;

		// Create objects
		_window = new Window(this, 136, winHeight, 60, winY, POPUP_VERTICAL);

		// Set palette
		_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)), Palette::backPos, 16);

		add(_window);

		// Set up objects
		_window->setColor(Palette::blockOffset(8) + 5);
		_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

		int y = btnY;
		for (size_t i = 0; i < _targets.size(); ++i)
		{
			TextButton *button = new TextButton(116, BUTTON_HEIGHT, 70, y);
			button->setColor(Palette::blockOffset(8) + 5);
			button->setText(_targets[i]->getName(_game->getLanguage()));
			button->onMouseClick((ActionHandler)&MultipleTargetsState::btnTargetClick);
			add(button);

			_btnTargets.push_back(button);

			y += button->getHeight() + SPACING;
		}
		_btnTargets[0]->onKeyboardPress((ActionHandler)&MultipleTargetsState::btnCancelClick, Options::keyCancel);

		centerAllSurfaces();

//	static const int OUTER_MARGIN = 3;
//	static const int INNER_MARGIN = 4;
//	static const int BORDER = 5;
//	static const int BUTTON_HEIGHT = 16;

/*		size_t listHeight = _targets.size() * 8; */
//		int winHeight = listHeight + BUTTON_HEIGHT + OUTER_MARGIN * 2 + INNER_MARGIN + BORDER * 2;
//		int winY = (200 - winHeight) / 2;
//		int listY = winY + BORDER + OUTER_MARGIN;
//		int btnY = listY + listHeight + INNER_MARGIN;

//		_window = new Window(this, 136, winHeight, 60, winY);
//		_btnCancel = new TextButton(116, BUTTON_HEIGHT, 70, btnY);
//		_lstTargets = new TextList(116, listHeight, 70, listY);

//		int winHeight = listHeight + 36;
//		int winY = (236 - listHeight) / 2;
//		int listY = ((236 - listHeight) / 2) + 8;
//		int btnY = ((236 - listHeight) / 2) + listHeight + 12;

/*		_window		= new Window(this, 136, listHeight + 36, 60, 118 - listHeight / 2);
		_lstTargets	= new TextList(116, listHeight, 70, 126 - listHeight / 2);
		_btnCancel	= new TextButton(116, 16, 70, 130 + listHeight / 2);


		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
					Palette::backPos,
					16);

		add(_window);
		add(_lstTargets);
		add(_btnCancel);

		centerAllSurfaces();

		_window->setColor(Palette::blockOffset(8)+5);
		_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

		_btnCancel->setColor(Palette::blockOffset(8)+5);
		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onMouseClick((ActionHandler)& MultipleTargetsState::btnCancelClick);
		_btnCancel->onKeyboardPress(
						(ActionHandler)& MultipleTargetsState::btnCancelClick,
						Options::keyCancel);

		_lstTargets->setColor(Palette::blockOffset(8)+5);
		_lstTargets->setColumns(1, 116);
		_lstTargets->setAlign(ALIGN_CENTER);
		_lstTargets->setSelectable(true);
		_lstTargets->setBackground(_window);
		_lstTargets->onMouseClick((ActionHandler)& MultipleTargetsState::lstTargetsClick);
		for (std::vector<Target*>::iterator
				i = _targets.begin();
				i != _targets.end();
				++i)
		{
			_lstTargets->addRow(
							1,
							(*i)->getName(_game->getLanguage()).c_str());
		} */
	}
}

/**
 *
 */
MultipleTargetsState::~MultipleTargetsState()
{
}

/**
 * Resets the palette and ignores the window if there's only one target.
 */
void MultipleTargetsState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);

	if (_targets.size() == 1)
		popupTarget(*_targets.begin());
}

/**
 * Displays the right popup for a specific target.
 * @param target, Pointer to target.
 */
void MultipleTargetsState::popupTarget(Target* target)
{
	//Log(LOG_INFO) << "MultipleTargetsState::popupTarget()";
//kL	_game->popState();

	if (_craft == 0)
	{
		//Log(LOG_INFO) << ". _craft == 0";
		Craft* c = dynamic_cast<Craft*>(target);
		Ufo* u = dynamic_cast<Ufo*>(target);
		Base* b = dynamic_cast<Base*>(target);
		//Log(LOG_INFO) << ". dynamic_cast's examined";

		if (b != 0)
		{
			//Log(LOG_INFO) << ". . base";
			_game->popState(); // kL
			_game->pushState(new InterceptState(
											_game,
											_state->getGlobe(),
											b));
		}
		else if (c != 0)
		{
			//Log(LOG_INFO) << ". . craft";
			// kL_begin:
			bool doublePop = true;

			if (_targets.size() == 1)
			{
				doublePop = false;
				_game->popState();
			} // kL_end.

			_game->pushState(new GeoscapeCraftState(
												_game,
												c,
												_state->getGlobe(),
												0,
												doublePop)); // kL_add.
		}
		else if (u != 0)
		{
			//Log(LOG_INFO) << ". . ufo";
			_game->popState(); // kL
			_game->pushState(new UfoDetectedState(
												_game,
												u,
												_state,
												false,
												u->getHyperDetected()));
		}
		else
		{
			//Log(LOG_INFO) << ". . else...";
//kL			_game->pushState(new TargetInfoState(_game, target, _state->getGlobe()));
			_game->popState(); // kL
			_game->pushState(new TargetInfoState(
												_game,
												target,
												_state->getGlobe(),
												_state)); // kL
		}
	}
	else
	{
		//Log(LOG_INFO) << ". _craft != 0";
		_game->popState(); // kL
		_game->pushState(new ConfirmDestinationState(
													_game,
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
void MultipleTargetsState::btnTargetClick(Action*)
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

//	Target* targ = _targets[_lstTargets->getSelectedRow()];
//	popupTarget(targ);
	//Log(LOG_INFO) << "MultipleTargetsState::lstTargetsClick() EXIT";
}

}
