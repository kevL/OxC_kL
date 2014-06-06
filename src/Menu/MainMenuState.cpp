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

#include <sstream>

#include "ListLoadState.h"
#include "NewBattleState.h"
#include "NewGameState.h"
#include "OptionsVideoState.h"

#include "../version.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"
#include "../Resource/XcomResourcePack.h" // sza_MusicRules


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
	// kL_note: These screen calls were displaced to IntroState &
	// AbandonGameState & StartState & SaveGameState & MainMenuState::resize()
	//
	// This uses baseX/Y options for Geoscape & Basescape:
	Options::baseXResolution = Options::baseXGeoscape; // kL
	Options::baseYResolution = Options::baseYGeoscape; // kL
	// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*	Screen::updateScale(
					Options::geoscapeScale,
					Options::geoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					true); kL */
	_game->getScreen()->resetDisplay(false); // kL

	_window			= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtTitle		= new Text(256, 30, 32, 56);

	_btnNewGame		= new TextButton(92, 20, 64, 88);
	_btnNewBattle	= new TextButton(92, 20, 164, 88);

	_btnLoad		= new TextButton(92, 20, 64, 116);
	_btnOptions		= new TextButton(92, 20, 164, 116);

	_btnQuit		= new TextButton(192, 20, 64, 144);

	setPalette("PAL_GEOSCAPE", 0);

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
	_btnOptions->setVisible(false); // kL

	_btnQuit->setColor(Palette::blockOffset(8)+5);
	_btnQuit->setText(tr("STR_QUIT"));
	_btnQuit->onMouseClick((ActionHandler)& MainMenuState::btnQuitClick);

	_txtTitle->setColor(Palette::blockOffset(8)+10);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	std::wostringstream title;
	title << tr("STR_OPENXCOM"); //kL << L"\x02";
//kL	title << Language::utf8ToWstr(OPENXCOM_VERSION_SHORT) << Language::utf8ToWstr(OPENXCOM_VERSION_GIT);
//	title << Language::utf8ToWstr(OPENXCOM_VERSION_GIT); // kL
	_txtTitle->setText(title.str());


//	_game->getResourcePack()->playMusic("GMSTORY");
//	_game->getResourcePack()->getMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMSTORY)->play(); // sza_MusicRules
	_game->getResourcePack()->playMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMSTORY); // kL, sza_MusicRules

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
	_game->pushState(new ListLoadState(
									_game,
									OPT_MENU));
}

/**
 * Opens the Options screen.
 * @param action Pointer to an action.
 */
void MainMenuState::btnOptionsClick(Action*)
{
	Options::backupDisplay();

	_game->pushState(new OptionsVideoState(
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

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void MainMenuState::resize(
		int& dX,
		int& dY)
{
/*	dX = Options::baseXResolution;
	dY = Options::baseYResolution; */

	// This uses baseX/Y options for Geoscape & Basescape:
//	Options::baseXResolution = Options::baseXGeoscape; // kL
//	Options::baseYResolution = Options::baseYGeoscape; // kL
	// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*	Screen::updateScale(
					Options::geoscapeScale,
					Options::geoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					true); */
//	_game->getScreen()->resetDisplay(false); // kL: this resets options.cfg!

/*	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;

	State::resize(dX, dY); */
}

}
