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

#include "MediTargetState.h"

#include "MediKitState.h"
#include "TileEngine.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the MediTargetState screen.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 */
MediTargetState::MediTargetState(BattleAction* action)
	:
		_action(action)
{
	_screen = false;

	_window		= new Window(this, 270, 96, 25, 50);
	_txtTitle	= new Text(200, 9, 60, 58);

	_txtWounds	= new Text(13, 9, 156, 70);
	_txtHealth	= new Text(42, 9, 174, 70);
	_txtEnergy	= new Text(32, 9, 221, 70);
	_txtMorale	= new Text(18, 9, 258, 70);

	_lstTarget	= new TextList(234, 33, 42, 80);

	_btnCancel	= new TextButton(120, 14, 100, 125);

	setPalette("PAL_BATTLESCAPE");
//			_game->getRuleset()->getInterface("soldierList")->getElement("palette")->color);

	add(_window,	"messageWindowBorder",	"battlescape");
	add(_txtTitle,	"messageWindows",		"battlescape");
	add(_txtWounds,	"messageWindows",		"battlescape");
	add(_txtHealth,	"messageWindows",		"battlescape");
	add(_txtEnergy,	"messageWindows",		"battlescape");
	add(_txtMorale,	"messageWindows",		"battlescape");
	add(_lstTarget,	"messageWindows",		"battlescape");
	add(_btnCancel,	"messageWindowButtons",	"battlescape");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));
	_window->setHighContrast();

	_txtTitle->setText(tr("STR_MEDITARGET"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast();

//	_txtWounds->setText(tr("STR_FATAL_WOUNDS"));
	_txtWounds->setText(L"fw");
	_txtWounds->setHighContrast();

//	_txtHealth->setText(tr("STR_HEALTH"));
	_txtHealth->setText(L"health");
	_txtHealth->setHighContrast();

//	_txtEnergy->setText(tr("STR_STAMINA"));
	_txtEnergy->setText(L"sta");
	_txtEnergy->setHighContrast();

//	_txtMorale->setText(tr("STR_MORALE"));
	_txtMorale->setText(L"rl");
	_txtMorale->setHighContrast();

	_lstTarget->setColumns(5, 108, 18, 47, 37, 18);
	_lstTarget->setBackground(_window);
	_lstTarget->setSelectable();
	_lstTarget->setMargin(6);
	_lstTarget->setHighContrast();
	_lstTarget->onMousePress((ActionHandler)& MediTargetState::lstTargetPress);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->setHighContrast();
	_btnCancel->onMouseClick((ActionHandler)& MediTargetState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& MediTargetState::btnCancelClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
MediTargetState::~MediTargetState()
{}

/**
 * Resets the palette and adds targets to the TextList.
 */
void MediTargetState::init()
{
	State::init();

	bool
		addToList,
		firstPass = true;

	const std::vector<BattleUnit*>* const units = _game->getSavedGame()->getSavedBattle()->getUnits();
	for (std::vector<BattleUnit*>::const_iterator
			i = units->begin();
			i != units->end();
			++i)
	{
		if ((*i)->isFearable() == true) // isWoundable()
		{
			addToList = false;

			if (firstPass == true
				&& *i == _action->actor)
			{
				addToList = true;
			}

			if (firstPass == false
				&& *i != _action->actor)
			{
				if ((*i)->getStatus() == STATUS_UNCONSCIOUS
					&& (*i)->getPosition() == _action->actor->getPosition())
				{
					addToList = true;;
				}
				else if (_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
																								_action->actor,
																								*i,
																								_action->actor->getDirection()) == true)
				{
					addToList = true;
				}
			}


			if (addToList == true)
			{
				_targetUnits.push_back(*i);

				std::wostringstream
					woststr1,
					woststr2;

				woststr1	<< _targetUnits.back()->getHealth()
							<< L"|"
							<< _targetUnits.back()->getBaseStats()->health
							<< L"|"
							<< _targetUnits.back()->getStun();

				woststr2	<< _targetUnits.back()->getEnergy()
							<< L"|"
							<< _targetUnits.back()->getBaseStats()->stamina;

				_lstTarget->addRow(
								5,
								(*i)->getName(_game->getLanguage()).c_str(),
								Text::formatNumber(_targetUnits.back()->getFatalWounds()).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str(),
								Text::formatNumber(_targetUnits.back()->getMorale()).c_str());

				if (firstPass == true)
				{
					firstPass = false;
					i = units->begin();

					_lstTarget->setRowColor(
										0,
										Palette::blockOffset(6), // orange
										true);
				}
			}
		}

		if (firstPass == true // in case medikit user is not 'Fearable'
			&& *i == units->back())
		{
			firstPass = false;
			i = units->begin();
		}
	}

	if (_targetUnits.size() == 1)
	{
		_action->target = _targetUnits[0]->getPosition(); // jic.
		_action->targetUnit = _targetUnits[0];

		_game->popState();
		_game->pushState(new MedikitState(_action));
	}
	else if (_targetUnits.empty() == true)
		_game->popState();
}

/**
 * Chooses a unit to apply Medikit to.
 * @param action - pointer to an Action
 */
void MediTargetState::lstTargetPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		BattleUnit* const targetUnit = _targetUnits[_lstTarget->getSelectedRow()];
		_action->target = targetUnit->getPosition(); // jic.
		_action->targetUnit = targetUnit;

		_game->popState();
		_game->pushState(new MedikitState(_action));
	}
}

/**
 * Returns to the previous screen.
 */
void MediTargetState::btnCancelClick(Action*)
{
	_game->popState();
}

}
