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

#include "SaveGameState.h"

//#include <sstream>

#include "ErrorMessageState.h"
#include "MainMenuState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Interface/Text.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Save Game screen.
 * @param origin Game section that originated this state.
 * @param filename Name of the save file without extension.
 * @param palette Parent state palette.
 */
SaveGameState::SaveGameState(
		OptionsOrigin origin,
		const std::string& filename,
		SDL_Color* palette)
	:
		_firstRun(0),
		_origin(origin),
		_filename(filename),
		_type(SAVE_DEFAULT)
{
	buildUi(palette);
}

/**
 * Initializes all the elements in the Save Game screen.
 * @param origin Game section that originated this state.
 * @param type Type of auto-save being used.
 * @param palette Parent state palette.
 */
SaveGameState::SaveGameState(
		OptionsOrigin origin,
		SaveType type,
		SDL_Color* palette)
	:
		_firstRun(0),
		_origin(origin),
		_type(type)
{
	switch (type)
	{
		case SAVE_QUICK:
			_filename = SavedGame::QUICKSAVE;
		break;
		case SAVE_AUTO_GEOSCAPE:
			_filename = SavedGame::AUTOSAVE_GEOSCAPE;
		break;
		case SAVE_AUTO_BATTLESCAPE:
			_filename = SavedGame::AUTOSAVE_BATTLESCAPE;
		break;
		case SAVE_IRONMAN:
		case SAVE_IRONMAN_END:
			_filename = CrossPlatform::sanitizeFilename(Language::wstrToFs(_game->getSavedGame()->getName())) + ".sav";
		break;

		default:
		break;
	}

	buildUi(palette);
}

/**
 * dTor.
 */
SaveGameState::~SaveGameState()
{}

/**
 * Builds the interface.
 * @param palette Parent state palette.
 */
void SaveGameState::buildUi(SDL_Color* palette)
{
	_screen = false;

	_txtStatus = new Text(320, 17, 0, 92);

	setPalette(palette);

	if (_origin == OPT_BATTLESCAPE)
	{
		add(_txtStatus, "textLoad", "battlescape");
//		_txtStatus->setColor(Palette::blockOffset(1)-1);
		_txtStatus->setHighContrast();
	}
	else
		add(_txtStatus, "textLoad", "geoscape");

	centerAllSurfaces();


//	_txtStatus->setColor(Palette::blockOffset(8)+5);
	_txtStatus->setBig();
	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_SAVING_GAME"));
}

/**
 * Saves the current save.
 */
void SaveGameState::think()
{
	State::think();

	// Make sure it gets drawn properly
	if (_firstRun < 10)
		_firstRun++;
	else
	{
		_game->popState();

		switch (_type)
		{
			case SAVE_DEFAULT: // manual save, close the save screen
				_game->popState();

				if (!_game->getSavedGame()->isIronman()) // and pause screen too
					_game->popState();
			break;

			case SAVE_QUICK: // automatic save, give it a default name
			case SAVE_AUTO_GEOSCAPE:
			case SAVE_AUTO_BATTLESCAPE:
				_game->getSavedGame()->setName(Language::fsToWstr(_filename));

			default:
			break;
		}


		try // Save the game
		{
			std::string backup = _filename + ".bak";
			_game->getSavedGame()->save(backup);
			std::string fullPath = Options::getUserFolder() + _filename;
			std::string bakPath = Options::getUserFolder() + backup;
			if (!CrossPlatform::moveFile(bakPath, fullPath))
			{
				throw Exception("Save backed up in " + backup);
			}

			if (_type == SAVE_IRONMAN_END)
			{
				// This uses baseX/Y options for Geoscape & Basescape:
//				Options::baseXResolution = Options::baseXGeoscape; // kL
//				Options::baseYResolution = Options::baseYGeoscape; // kL
				// This sets Geoscape and Basescape to default (320x200) IG and the config.
/*kL			Screen::updateScale(
								Options::geoscapeScale,
								Options::geoscapeScale,
								Options::baseXGeoscape,
								Options::baseYGeoscape,
								true); */
//				_game->getScreen()->resetDisplay(false);

				_game->setState(new MainMenuState());
				_game->setSavedGame(0);
			}
		}
		catch (Exception &e)
		{
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_SAVE_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapeColor")->color, //Palette::blockOffset(8)+10,
													"BACK01.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapePalette")->color)); //6
			else
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapeColor")->color, //Palette::blockOffset(0),
													"TAC00.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapePalette")->color)); //-1
		}
		catch (YAML::Exception &e)
		{
			Log(LOG_ERROR) << e.what();
			std::wostringstream error;
			error << tr("STR_SAVE_UNSUCCESSFUL") << L'\x02' << Language::fsToWstr(e.what());
			if (_origin != OPT_BATTLESCAPE)
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapeColor")->color, //Palette::blockOffset(8)+10,
													"BACK01.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("geoscapePalette")->color)); //6
			else
				_game->pushState(new ErrorMessageState(
													error.str(),
													_palette,
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapeColor")->color, //Palette::blockOffset(0),
													"TAC00.SCR",
													_game->getRuleset()->getInterface("errorMessages")->getElement("battlescapePalette")->color)); //-1
		}
	}
}

}
