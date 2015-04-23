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

#include "AbortMissionState.h"

//#include <sstream>
//#include <vector>

#include "BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Abort Mission window.
 * @param battleGame	- pointer to the SavedBattleGame
 * @param state			- pointer to the BattlescapeState
 */
AbortMissionState::AbortMissionState(
		SavedBattleGame* battleGame,
		BattlescapeState* state)
	:
		_battleGame(battleGame),
		_state(state),
		_inExitArea(0),
		_outExitArea(0)
{
	_screen = false;

	_window			= new Window(this, 320, 144);

	_txtInExit		= new Text(304, 17, 16, 25);
	_txtOutsideExit	= new Text(304, 17, 16, 50);

	_txtAbort		= new Text(320, 17, 0, 84);

	_btnCancel		= new TextButton(134, 18, 16, 116);
	_btnOk			= new TextButton(134, 18, 170, 116);

	_battleGame->setPaletteByDepth(this);

	add(_window,			"messageWindowBorder",	"battlescape");
	add(_txtInExit,			"messageWindows",		"battlescape");
	add(_txtOutsideExit,	"messageWindows",		"battlescape");
	add(_txtAbort,			"messageWindows",		"battlescape");
	add(_btnCancel,			"messageWindowButtons",	"battlescape");
	add(_btnOk,				"messageWindowButtons",	"battlescape");

	centerAllSurfaces();


	std::string nextStage;
	if (_battleGame->getMissionType() != "STR_UFO_GROUND_ASSAULT"
		&& _battleGame->getMissionType() != "STR_UFO_CRASH_RECOVERY")
	{
		nextStage = _game->getRuleset()->getDeployment(_battleGame->getMissionType())->getNextStage();
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleGame->getUnits()->begin();
			i != _battleGame->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->isOut() == false)
		{
			if ((nextStage.empty() == false
					&& (*i)->isInExitArea(END_POINT))
				|| (nextStage == ""
					&& (*i)->isInExitArea() == true))
			{
				++_inExitArea;
			}
			else
				++_outExitArea;
		}
	}


	_window->setBackground(_game->getResourcePack()->getSurface("TAC00.SCR"));
	_window->setHighContrast();

	_txtInExit->setText(tr("STR_UNITS_IN_EXIT_AREA", _inExitArea));
	_txtInExit->setBig();
	_txtInExit->setAlign(ALIGN_CENTER);
	_txtInExit->setHighContrast();

	_txtOutsideExit->setText(tr("STR_UNITS_OUTSIDE_EXIT_AREA", _outExitArea));
	_txtOutsideExit->setBig();
	_txtOutsideExit->setAlign(ALIGN_CENTER);
	_txtOutsideExit->setHighContrast();

	if (_battleGame->getMissionType() == "STR_BASE_DEFENSE")
	{
		_txtInExit->setVisible(false);
		_txtOutsideExit->setVisible(false);
	}

	_txtAbort->setBig();
	_txtAbort->setAlign(ALIGN_CENTER);
	_txtAbort->setHighContrast();
	_txtAbort->setText(tr("STR_ABORT_MISSION_QUESTION"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setHighContrast();
	_btnOk->onMouseClick((ActionHandler)& AbortMissionState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnOkClick,
					Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->setHighContrast();
	_btnCancel->onMouseClick((ActionHandler)& AbortMissionState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnCancelClick,
					Options::keyCancel);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& AbortMissionState::btnCancelClick,
					Options::keyBattleAbort);
}

/**
 * dTor.
 */
AbortMissionState::~AbortMissionState()
{}

/**
 * Confirms mission abort.
 * @param action - pointer to an Action
 */
void AbortMissionState::btnOkClick(Action*)
{
	_game->popState();

	_battleGame->setAborted(true);
	_state->finishBattle(
					true,
					_inExitArea);
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void AbortMissionState::btnCancelClick(Action*)
{
	_game->popState();
}

}
