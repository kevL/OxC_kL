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

#include "VictoryState.h"

//#include <sstream>

//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Screen.h"
//#include "../Engine/Timer.h"

#include "../Interface/Text.h"

#include "../Menu/MainMenuState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Victory screen.
 */
VictoryState::VictoryState()
	:
		_curScreen(std::numeric_limits<size_t>::max())
{
/*	Options::baseXResolution = Screen::ORIGINAL_WIDTH;
	Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
	_game->getScreen()->resetDisplay(false); */

	const char* files[] =
	{
		"PICT1.LBM",
		"PICT2.LBM",
		"PICT3.LBM",
		"PICT6.LBM",
		"PICT7.LBM"
	};

//	_timer = new Timer(30000);

	_text[0] = new Text(195, 56, 5, 0);
	_text[1] = new Text(232, 64, 88, 136);
	_text[2] = new Text(254, 48, 66, 152);
	_text[3] = new Text(300, 200, 5, 0);
	_text[4] = new Text(310, 42, 5, 158);

	for (size_t
			i = 0;
			i < SCREENS;
			++i)
	{
		Surface* const screen = _game->getResourcePack()->getSurface(files[i]);

		_bg[i] = new InteractiveSurface(320, 200);

		setPalette(screen->getPalette());

		add(_bg[i]);
		add(_text[i], "victoryText", "gameOver");

		screen->blit(_bg[i]);
		_bg[i]->setVisible(false);
		_bg[i]->onMousePress((ActionHandler)& VictoryState::screenPress);
		_bg[i]->onKeyboardPress((ActionHandler)& VictoryState::screenPress);

		std::ostringstream oststr;
		oststr << "STR_VICTORY_" << static_cast<int>(i + 1);
		_text[i]->setText(tr(oststr.str()));
		_text[i]->setWordWrap();
		_text[i]->setVisible(false);
	}

	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_WIN);

	centerAllSurfaces();


//	_timer->onTimer((StateHandler)& VictoryState::screenTimer);
//	_timer->start();

	screenPress(NULL);

	if (_game->getSavedGame()->isIronman() == true) // Ironman is over, rambo
	{
		const std::string filename = CrossPlatform::sanitizeFilename(Language::wstrToFs(_game->getSavedGame()->getName())) + ".sav";
		CrossPlatform::deleteFile(Options::getUserFolder() + filename);
	}
}

/**
 * dTor.
 */
VictoryState::~VictoryState()
{
//	delete _timer;
}

/**
 * Shows the next screen on a timed basis.
 */
/*void VictoryState::screenTimer()
{
	screenPress(NULL);
} */

/**
 * Handle timers.
 */
/*void VictoryState::think()
{
	_timer->think(this, NULL);
} */

/**
 * Shows the next screen in the slideshow or goes back to the Main Menu.
 * @param action - pointer to an Action
 */
void VictoryState::screenPress(Action*)
{
//	_timer->start();

	if (_curScreen != std::numeric_limits<size_t>::max())
	{
		_bg[_curScreen]->setVisible(false);
		_text[_curScreen]->setVisible(false);
	}

	++_curScreen;

	if (_curScreen < SCREENS) // next screen
	{
		setPalette(_bg[_curScreen]->getPalette());
		_bg[_curScreen]->setVisible();
		_text[_curScreen]->setVisible();

		init();
	}
	else // quit game
	{
		_game->popState();
/*		Screen::updateScale(
						Options::geoscapeScale,
						Options::geoscapeScale,
						Options::baseXGeoscape,
						Options::baseYGeoscape,
						true);
		_game->getScreen()->resetDisplay(false); */

		_game->setState(new MainMenuState());
		_game->setSavedGame(NULL);
	}
}

}
