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

#include "MainMenuState.h"

#include "LoadState.h"
#include "NewBattleState.h"
#include "NewGameState.h"
#include "OptionsState.h"

#include "../version.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
#include "../Engine/Palette.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Main Menu window.
 * @param game Pointer to the core game.
 */
MainMenuState::MainMenuState(Game* game)
	:
		State(game)
{
	_window			= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtTitle		= new Text(256, 30, 32, 56);

	_btnNewGame		= new TextButton(92, 20, 64, 88);
	_btnNewBattle	= new TextButton(92, 20, 164, 88);

	_btnLoad		= new TextButton(92, 20, 64, 116);
	_btnOptions		= new TextButton(92, 20, 164, 116);

	_btnQuit		= new TextButton(192, 20, 64, 144);


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_0")->getColors());
	_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
					Palette::backPos,
					16);

	add(_window);
	add(_txtTitle);
	add(_btnNewGame);
	add(_btnNewBattle);
	add(_btnLoad);
	add(_btnOptions);
	add(_btnQuit);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnNewGame->setColor(Palette::blockOffset(8)+5);
	_btnNewGame->setText(tr("STR_NEW_GAME"));
	_btnNewGame->onMouseClick((ActionHandler)& MainMenuState::btnNewGameClick);

	_btnNewBattle->setColor(Palette::blockOffset(8)+5);
	_btnNewBattle->setText(tr("STR_NEW_BATTLE"));
	_btnNewBattle->onMouseClick((ActionHandler)& MainMenuState::btnNewBattleClick);

	_btnLoad->setColor(Palette::blockOffset(8)+5);
	_btnLoad->setText(tr("STR_LOAD_SAVED_GAME"));
	_btnLoad->onMouseClick((ActionHandler)& MainMenuState::btnLoadClick);

	_btnOptions->setColor(Palette::blockOffset(8)+5);
	_btnOptions->setText(tr("STR_OPTIONS"));
	_btnOptions->onMouseClick((ActionHandler)& MainMenuState::btnOptionsClick);

	_btnQuit->setColor(Palette::blockOffset(8)+5);
	_btnQuit->setText(tr("STR_QUIT"));
	_btnQuit->onMouseClick((ActionHandler)& MainMenuState::btnQuitClick);

	_txtTitle->setColor(Palette::blockOffset(8)+10);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	std::wstringstream title;
	title << tr("STR_OPENXCOM"); //kL << L"\x02";
//kL	title << Language::utf8ToWstr(OPENXCOM_VERSION_SHORT) << Language::utf8ToWstr(OPENXCOM_VERSION_GIT);
//	title << Language::utf8ToWstr(OPENXCOM_VERSION_GIT); // kL
	_txtTitle->setText(title.str());


	_game->getResourcePack()->getMusic("GMSTORY")->play();

	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
}

/**
 *
 */
MainMenuState::~MainMenuState()
{
}

/**
 * Resets the palette
 * since it's bound to change on other screens.
 */
void MainMenuState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);
}

/**
 * Opens the New Game window.
 * @param action Pointer to an action.
 */
void MainMenuState::btnNewGameClick(Action*)
{
	_game->pushState(new NewGameState(_game));
}

/**
 * Opens the New Battle screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnNewBattleClick(Action*)
{
	_game->pushState(new NewBattleState(_game));
}

/**
 * Opens the Load Game screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnLoadClick(Action*)
{
	_game->pushState(new LoadState(
								_game,
								OPT_MENU));
}

/**
 * Opens the Options screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnOptionsClick(Action*)
{
	_game->pushState(new OptionsState(
									_game,
									OPT_MENU));
}

/**
 * Quits the game.
 * @param action Pointer to an action.
 */
void MainMenuState::btnQuitClick(Action*)
{
	_game->quit();
}

}
