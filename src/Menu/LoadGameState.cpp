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

#include "LoadGameState.h"

//#include <sstream>

#include "ErrorMessageState.h"

#include "../Battlescape/BattlescapeState.h"

//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Load Game screen.
 * @param origin	- game section that originated this state
 * @param file		- reference the name of the save file without extension
 * @param palette	- pointer to parent state palette
 */
LoadGameState::LoadGameState(
		OptionsOrigin origin,
		const std::string& file,
		SDL_Color* const palette)
	:
		_origin(origin),
		_file(file),
		_firstRun(0)
{
	buildUi(palette);
}

/**
 * Initializes all the elements in the Load Game screen.
 * @param origin	- game section that originated this state
 * @param type		- type of auto-load being used
 * @param palette	- pointer to parent state palette
 */
LoadGameState::LoadGameState(
		OptionsOrigin origin,
		SaveType type,
		SDL_Color* const palette)
	:
		_origin(origin),
		_firstRun(0)
{
	switch (type) // can't auto-load ironman games
	{
		case SAVE_QUICK:
			_file = SavedGame::QUICKSAVE;
		break;

		case SAVE_AUTO_GEOSCAPE:
			_file = SavedGame::AUTOSAVE_GEOSCAPE;
		break;

		case SAVE_AUTO_BATTLESCAPE:
			_file = SavedGame::AUTOSAVE_BATTLESCAPE;
	}

	buildUi(palette, true);
}

/**
 * dTor.
 */
LoadGameState::~LoadGameState()
{
#ifdef _WIN32
	MessageBeep(MB_ICONASTERISK);
#endif
}

/**
 * Builds the interface.
 * @param palette	- pointer to parent state palette
 * @param dropText	- true if loading without a window (eg. quickload)
 */
void LoadGameState::buildUi(
		SDL_Color* const palette,
		bool dropText)
{
#ifdef _WIN32
//	MessageBeep(MB_OK); // <- done in BattlescapeState::handle() for Fkeys
#endif
	_screen = false;

	int y;
	if (dropText == true)
		y = 92;
	else
		y = -18;

	_txtStatus = new Text(320, 17, 0, y);

	setPalette(palette);

	if (_origin == OPT_BATTLESCAPE)
	{
		add(_txtStatus, "textLoad", "battlescape");
		_txtStatus->setHighContrast();
	}
	else
		add(_txtStatus, "textLoad", "geoscape");

	centerAllSurfaces();


	_txtStatus->setText(tr("STR_LOADING_GAME"));
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setBig();

	_game->getCursor()->setVisible(false);
}

/**
 * Ignore quick loads without a save available.
 */
void LoadGameState::init()
{
	State::init();

	if (_file == SavedGame::QUICKSAVE
		&& CrossPlatform::fileExists(Options::getUserFolder() + _file) == false)
	{
		_game->popState();
		_game->getCursor()->setVisible();
	}
}

/**
 * Loads the specified save.
 */
void LoadGameState::think()
{
	State::think();

	if (_firstRun < 10) // pause to Ensure this gets drawn properly
		++_firstRun;
	else
	{
		_game->popState();
		_game->getCursor()->setVisible();

		SavedGame* const gameSave = new SavedGame(_game->getRuleset());
		try
		{
			Log(LOG_INFO) << "LoadGameState: loading";
			gameSave->load(
						_file,
						_game->getRuleset());
			_game->setSavedGame(gameSave);

			Options::baseXResolution = Options::baseXGeoscape;
			Options::baseYResolution = Options::baseYGeoscape;

			_game->getScreen()->resetDisplay(false);
			_game->setState(new GeoscapeState());

			if (_game->getSavedGame()->getBattleSave() != NULL)
			{
				Log(LOG_INFO) << "LoadGameState: loading battlescape map";
				_game->getSavedGame()->getBattleSave()->loadMapResources(_game);

				Options::baseXResolution = Options::baseXBattlescape;
				Options::baseYResolution = Options::baseYBattlescape;

				_game->getScreen()->resetDisplay(false);

				BattlescapeState* const bs = new BattlescapeState();
				_game->pushState(bs);
				_game->getSavedGame()->getBattleSave()->setBattleState(bs);
			}
		}
		catch (Exception& e)
		{
			Log(LOG_INFO) << "LoadGame error";
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_LOAD_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapeColor")->color,
													"BACK01.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapePalette")->color));
			else
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapeColor")->color,
													"TAC00.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapePalette")->color));

			if (_game->getSavedGame() == gameSave)
				_game->setSavedGame(NULL);
			else
				delete gameSave;
		}
		catch (YAML::Exception& e)
		{
			Log(LOG_INFO) << "LoadGame error YAML";
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_LOAD_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapeColor")->color,
													"BACK01.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapePalette")->color));
			else
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapeColor")->color,
													"TAC00.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapePalette")->color));

			if (_game->getSavedGame() == gameSave)
				_game->setSavedGame(NULL);
			else
				delete gameSave;
		}

		CrossPlatform::flashWindow();
	}
}

}
