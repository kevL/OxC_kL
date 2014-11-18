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

#include "AbandonGameState.h"

#include "MainMenuState.h"
#include "SaveGameState.h"

#include "../Engine/Game.h"

#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
//kL #include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Abandon Game screen.
 * @param origin - game section that originated this state
 */
AbandonGameState::AbandonGameState(OptionsOrigin origin)
	:
		_origin(origin)
{
	_screen = false;

	int x = 52;
	if (_origin == OPT_GEOSCAPE)
		x = 20;

	_window		= new Window(this, 216, 160, x, 20, POPUP_BOTH);
	_txtTitle	= new Text(206, 33, x + 5, 73);
//	_txtTitle	= new Text(206, 17, x + 5, 73);

	_btnNo		= new TextButton(55, 20, x + 30, 132);
	_btnYes		= new TextButton(55, 20, x + 131, 132);

	if (_origin == OPT_BATTLESCAPE)
		setPalette("PAL_BATTLESCAPE");
	else
		setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_btnNo);
	add(_btnYes);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnYes->setColor(Palette::blockOffset(15)-1);
	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick((ActionHandler)& AbandonGameState::btnYesClick);
	_btnYes->onKeyboardPress(
					(ActionHandler)& AbandonGameState::btnYesClick,
					Options::keyOk);

	_btnNo->setColor(Palette::blockOffset(15)-1);
	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick((ActionHandler)& AbandonGameState::btnNoClick);
	_btnNo->onKeyboardPress(
					(ActionHandler)& AbandonGameState::btnNoClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_ABANDON_GAME_QUESTION"));

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeTheme();
}

/**
 * dTor.
 */
AbandonGameState::~AbandonGameState()
{
}

/**
 * Goes back to the Main Menu.
 * @param action - pointer to an Action
 */
void AbandonGameState::btnYesClick(Action*)
{
/* #ifndef __NO_MUSIC
	if (Mix_GetMusicType(NULL) != MUS_MID) // fade out!
	{
		_game->setInputActive(false);

		Mix_FadeOutMusic(900);
		func_fade();

		while (Mix_PlayingMusic() == 1)
		{
		}
	}
	else
		Mix_HaltMusic();
#endif */
	_game->getResourcePack()->fadeMusic(_game, 900);


	if (_game->getSavedGame()->isIronman() == false)
	{
		// This uses baseX/Y options for Geoscape & Basescape:
//		Options::baseXResolution = Options::baseXGeoscape; // kL
//		Options::baseYResolution = Options::baseYGeoscape; // kL
		// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*kL		Screen::updateScale(
						Options::geoscapeScale,
						Options::geoscapeScale,
						Options::baseXGeoscape,
						Options::baseYGeoscape,
						true); */
//		_game->getScreen()->resetDisplay(false);

		_game->setState(new MainMenuState());
		_game->setSavedGame(NULL);
	}
	else
		_game->pushState(new SaveGameState(
										OPT_GEOSCAPE,
										SAVE_IRONMAN_END,
										_palette));
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void AbandonGameState::btnNoClick(Action*)
{
	_game->popState();
}

}
