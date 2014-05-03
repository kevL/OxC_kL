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

#include "NewGameState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"

#include "../Geoscape/BuildNewBaseState.h"
#include "../Geoscape/GeoscapeState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Difficulty window.
 * @param game Pointer to the core game.
 */
NewGameState::NewGameState(Game* game)
	:
		State(game)
{
	_window			= new Window(this, 192, 180, 64, 10, POPUP_VERTICAL);

	_btnBeginner	= new TextButton(160, 18, 80, 32);
	_btnExperienced	= new TextButton(160, 18, 80, 52);
	_btnVeteran		= new TextButton(160, 18, 80, 72);
	_btnGenius		= new TextButton(160, 18, 80, 92);
	_btnSuperhuman	= new TextButton(160, 18, 80, 112);
	_btnIronman		= new ToggleTextButton(78, 18, 80, 138);
	_btnOk			= new TextButton(78, 16, 80, 164);
	_btnCancel		= new TextButton(78, 16, 162, 164);
	_txtTitle		= new Text(192, 9, 64, 20);
	_txtIronman		= new Text(90, 16, 162, 138);

	_difficulty = _btnBeginner;
/*	_txtTitle		= new Text(192, 17, 64, 21);

	_btnBeginner	= new TextButton(160, 18, 80, 42);
	_btnExperienced	= new TextButton(160, 18, 80, 64);
	_btnVeteran		= new TextButton(160, 18, 80, 86);
	_btnGenius		= new TextButton(160, 18, 80, 108);
	_btnSuperhuman	= new TextButton(160, 18, 80, 130);
	_btnCancel		= new TextButton(160, 18, 80, 158); */ // kL

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_btnBeginner);
	add(_btnExperienced);
	add(_btnVeteran);
	add(_btnGenius);
	add(_btnSuperhuman);
	add(_btnIronman);
	add(_txtIronman);
	add(_btnOk);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnBeginner->setColor(Palette::blockOffset(8)+5);
	_btnBeginner->setText(tr("STR_1_BEGINNER"));
	_btnBeginner->setGroup(&_difficulty);

	_btnExperienced->setColor(Palette::blockOffset(8)+5);
	_btnExperienced->setText(tr("STR_2_EXPERIENCED"));
	_btnExperienced->setGroup(&_difficulty);

	_btnVeteran->setColor(Palette::blockOffset(8)+5);
	_btnVeteran->setText(tr("STR_3_VETERAN"));
	_btnVeteran->setGroup(&_difficulty);

	_btnGenius->setColor(Palette::blockOffset(8)+5);
	_btnGenius->setText(tr("STR_4_GENIUS"));
	_btnGenius->setGroup(&_difficulty);

	_btnSuperhuman->setColor(Palette::blockOffset(8)+5);
	_btnSuperhuman->setText(tr("STR_5_SUPERHUMAN"));
	_btnSuperhuman->setGroup(&_difficulty);

	_btnIronman->setColor(Palette::blockOffset(8)+10);
	_btnIronman->setText(tr("STR_IRONMAN"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& NewGameState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewGameState::btnOkClick,
					Options::keyOk);

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewGameState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewGameState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+10);
	_txtTitle->setAlign(ALIGN_CENTER);
//	_txtTitle->setBig(); // kL
	_txtTitle->setText(tr("STR_SELECT_DIFFICULTY_LEVEL"));

	_txtIronman->setColor(Palette::blockOffset(8)+10);
	_txtIronman->setWordWrap(true);
	_txtIronman->setVerticalAlign(ALIGN_MIDDLE);
	_txtIronman->setText(tr("STR_IRONMAN_DESC"));
}

/**
 *
 */
NewGameState::~NewGameState()
{
}

/**
 * Sets up a new saved game and jumps to the Geoscape.
 * @param action Pointer to an action.
 */
void NewGameState::btnOkClick(Action*)
{
	GameDifficulty diff;
	if (_difficulty == _btnBeginner)
	{
		diff = DIFF_BEGINNER;
	}
	else if (_difficulty == _btnExperienced)
	{
		diff = DIFF_EXPERIENCED;
	}
	else if (_difficulty == _btnVeteran)
	{
		diff = DIFF_VETERAN;
	}
	else if (_difficulty == _btnGenius)
	{
		diff = DIFF_GENIUS;
	}
	else if (_difficulty == _btnSuperhuman)
	{
		diff = DIFF_SUPERHUMAN;
	}

	SavedGame* save = _game->getRuleset()->newSave();
	save->setDifficulty(diff);
	save->setIronman(_btnIronman->getPressed());
	_game->setSavedGame(save);

	GeoscapeState* gs = new GeoscapeState(_game);
	_game->setState(gs);
	gs->init();

	_game->pushState(new BuildNewBaseState(
										_game,
										_game->getSavedGame()->getBases()->back(),
										gs->getGlobe(),
										true));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void NewGameState::btnCancelClick(Action*)
{
	_game->setSavedGame(0);
	_game->popState();
}

}
