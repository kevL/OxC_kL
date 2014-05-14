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

#include "StartState.h"

#include <SDL_mixer.h>
#include <SDL_syswm.h>
#include <SDL_thread.h>

#include "ErrorMessageState.h"
#include "IntroState.h"
#include "MainMenuState.h"
//kL #include "OptionsBaseState.h"

#include "../Engine/Action.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"

#include "../Interface/FpsCounter.h"
#include "../Interface/Cursor.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

LoadingPhase StartState::loading;
std::string StartState::error;


/**
 * Initializes all the elements in the Loading screen.
 * @param game Pointer to the core game.
 */
StartState::StartState(Game* game)
	:
		State(game)
{
	int
		dx = (Options::baseXResolution - 320) / 2,
		dy = (Options::baseYResolution - 200) / 2;

	Options::newDisplayWidth = Options::displayWidth;
	Options::newDisplayHeight = Options::displayHeight;

	_thread = 0;
	loading = LOADING_STARTED;
	error = "";

	_surface = new Surface(320, 200, dx, dy);

	// Set palette (set to {0} here to ensure all fields are initialized)
	SDL_Color bnw[3] = {{0}};
	bnw[0].r = 0;
	bnw[0].g = 0;
	bnw[0].b = 0;
	bnw[1].r = 255;
	bnw[1].g = 255;
	bnw[1].b = 255;
	bnw[2].r = 255;
	bnw[2].g = 255;
	bnw[2].b = 0;
	setPalette(bnw, 0, 3);

	add(_surface);

	_surface->drawString(120, 96, "Loading...", 1);

	_game->getCursor()->setVisible(false);
	_game->getFpsCounter()->setVisible(false);
}

/**
 * Kill the thread in case the game is quit early.
 */
StartState::~StartState()
{
	if (_thread != 0)
		SDL_KillThread(_thread);
}

/**
 * Reset and reload data.
 */
void StartState::init()
{
	State::init();

	// Silence!
	Sound::stop();
	Music::stop();

	_game->setResourcePack(0);
	if (!Options::mute
		&& Options::reload)
	{
		Mix_CloseAudio();
		_game->initAudio();
	}

	// Load the game data in a separate thread
	_thread = SDL_CreateThread(
							load,
							(void*)_game);

	if (_thread == 0)
	{
		// If we can't create the thread, just load it as usual
		load((void*)_game);
	}
}

/**
 * If the loading fails, it shows an error, otherwise moves on to the game.
 */
void StartState::think()
{
	State::think();

	switch (loading)
	{
		case LOADING_FAILED:
			flash();

			_surface->clear();
			_surface->drawString(1, 9, "ERROR:", 2);
			_surface->drawString(1, 17, error.c_str(), 2);
			_surface->drawString(1, 49, "Make sure you installed OpenXcom", 1);
			_surface->drawString(1, 57, "correctly.", 1);
			_surface->drawString(1, 73, "Check the requirements and", 1);
			_surface->drawString(1, 81, "documentation for more details.", 1);
			_surface->drawString(75, 183, "Press any key to quit", 1);

			loading = LOADING_DONE;
		break;
		case LOADING_SUCCESSFUL:
			flash();

			Log(LOG_INFO) << "OpenXcom started!";
			if (!Options::reload
				&& Options::playIntro)
			{
				bool letterbox = Options::keepAspectRatio;
				Options::keepAspectRatio = true;

				_game->getScreen()->resetDisplay(false);

				_game->setState(new IntroState(
											_game,
											letterbox));
			}
			else
			{
				// This uses baseX/Y options for Geoscape & Basescape:
				Options::baseXResolution = Options::baseXGeoscape; // kL
				Options::baseYResolution = Options::baseYGeoscape; // kL
				// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*kL			OptionsBaseState::updateScale(
									Options::geoscapeScale,
									Options::geoscapeScale,
									Options::baseXGeoscape,
									Options::baseYGeoscape,
									true); */
				_game->getScreen()->resetDisplay(false);

				State* state = new MainMenuState(_game);
				_game->setState(state);

				if (!Options::badMods.empty()) // Check for mod loading errors
				{
					std::wostringstream error;
					error << tr("STR_MOD_UNSUCCESSFUL") << L'\x02';
					for (std::vector<std::string>::iterator
							i = Options::badMods.begin();
							i != Options::badMods.end();
							++i)
					{
						error << Language::fsToWstr(*i) << L'\n';
					}

					Options::badMods.clear();

					_game->pushState(new ErrorMessageState(
														_game,
														error.str(),
														state->getPalette(),
														Palette::blockOffset(8)+10,
														"BACK01.SCR",
														6));
				}

				Options::reload = false;
			}

			_game->getCursor()->setVisible(true);
			_game->getFpsCounter()->setVisible(Options::fpsCounter);
		break;

		default:
		break;
	}
}

/**
 * The game quits if the player presses any key when an error
 * message is on display.
 * @param action Pointer to an action.
 */
void StartState::handle(Action *action)
{
	State::handle(action);
	if (loading == LOADING_DONE)
	{
		if (action->getDetails()->type == SDL_KEYDOWN)
			_game->quit();
	}
}

/**
 * Notifies the user that maybe he should have a look.
 */
void StartState::flash()
{
#ifdef _WIN32
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWMInfo(&wminfo))
	{
		HWND hwnd = wminfo.window;
		FlashWindow(hwnd, true);
	}
#endif
}

/**
 * Loads game data and updates status accordingly.
 * @param game_ptr Pointer to the game.
 */
int StartState::load(void* game_ptr)
{
	Game* game = (Game*)game_ptr;
	try
	{
		Log(LOG_INFO) << "Loading ruleset...";
		game->loadRuleset();
		Log(LOG_INFO) << "Ruleset loaded.";

		Log(LOG_INFO) << "Loading resources...";
		Ruleset* ruleset = game->getRuleset();		// kL
		game->setResourcePack(new XcomResourcePack(	// kL sza_MusicRules, sza_ExtraMusic
												ruleset->getMusic(),
												ruleset->getExtraSprites(),
												ruleset->getExtraSounds(),
												ruleset->getExtraMusic()));
//kL		game->setResourcePack(new XcomResourcePack(
//kL												game->getRuleset()->getExtraSprites(),
//kL												game->getRuleset()->getExtraSounds()));
		Log(LOG_INFO) << "Resources loaded.";

		Log(LOG_INFO) << "Loading language...";
		game->defaultLanguage();
		Log(LOG_INFO) << "Language loaded successfully.";

		loading = LOADING_SUCCESSFUL;
	}
	catch (Exception &e)
	{
		error = e.what();
		Log(LOG_ERROR) << error;

		loading = LOADING_FAILED;
	}

	return 0;
}

}
