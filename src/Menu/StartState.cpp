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

#include "../version.h"

#include "ErrorMessageState.h"
#include "IntroState.h"
#include "MainMenuState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
//kL #include "../Engine/Surface.h"
#include "../Engine/Timer.h"

#include "../Interface/FpsCounter.h"
#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include <SDL_mixer.h>
#include <SDL_thread.h>


namespace OpenXcom
{

LoadingPhase StartState::loading;

std::string StartState::error;

bool StartState::kL_ready; // kL


/**
 * Initializes all the elements in the Loading screen.
 */
StartState::StartState()
	:
		_anim(0)
{
	// updateScale() uses newDisplayWidth/Height and needs to be set ahead of time
//kL	Options::newDisplayWidth	= Options::displayWidth;
//kL	Options::newDisplayHeight	= Options::displayHeight;

//kL	Options::baseXResolution = Options::displayWidth;
//kL	Options::baseYResolution = Options::displayHeight;
	Options::baseXResolution = 640;	// kL
	Options::baseYResolution = 400;	// kL
	_game->getScreen()->resetDisplay(false);

	const int
//		dx = (Options::baseXResolution - 320) / 2,	// kL
//		dy = (Options::baseYResolution - 200) / 2;	// kL
		dx = 10,	// kL
		dy = 20;	// kL
//	_surface = new Surface(320, 200, dx, dy);		// kL

	loading = LOADING_STARTED;
	_thread = NULL;
	error = "";

	// kL_begin: Old Loading screen ... pre 2014 may 26.
	// Set palette (set to {0} here to ensure all fields are initialized)
/*	SDL_Color bnw[3] = {{0}};
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
	_game->getFpsCounter()->setVisible(false); */
	// kL_end.

	_font	= new Font();
	_font->loadTerminal();

	_lang	= new Language();
	_text	= new Text(
//kL				Options::baseXResolution,
//kL				Options::baseYResolution,
//kL				0,
//kL				0);
					630,
					380,
					dx,
					dy);
	_cursor	= new Text(
					_font->getWidth(),
					_font->getHeight(),
					0,
					0);

	_timer	= new Timer(1); // for HQ graphic filters
//	_timer	= new Timer(9);

	setPalette(_font->getSurface()->getPalette(), 0, 2);

	add(_text);
	add(_cursor);

	_text->initText(_font, _font, _lang);
	_text->setColor(0);
	_text->setWordWrap(true);
//	_text->setText(L"Why hello there, I am a custom monoscaped bitmap font, yet I can still take advantage of wordwrapping and all the bells and whistles that regular text does, because I am actually being generated by a regular Text, golly gee whiz, we sure are living in the future now!\n\n!\"#$%&'()*+,-./0123456789:;<=>?\n@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n`abcdefghijklmnopqrstuvwxyz{|}~\n\nLoading OpenXcom 0.9...");

	_cursor->initText(_font, _font, _lang);
	_cursor->setColor(0);
	_cursor->setText(L"_");

	_timer->onTimer((StateHandler)& StartState::animate);
	_timer->start();

	_game->getCursor()->setVisible(false);
	_game->getFpsCounter()->setVisible(false);

	if (Options::reload == true)
	{
		addLine(L"Restarting...");
		addLine(L"");
	}
	else
	{
		addLine(Language::utf8ToWstr(CrossPlatform::getDosPath()) + L">openxcom");
//		addLine(L"");
//		addLine(L"OpenXcom initialisation");
//		addLine(L"");

//		if (Options::mute)
//			addLine(L"No Sound Detected");
//		else
//		{
//			addLine(L"SoundBlaster Sound Effects");
//			addLine(L"SoundBlaster Music");
//			addLine(L"Base Port 220  Irq 5  Dma 1");
//		}

//		addLine(L"");
	}
}

/**
 * Kill the thread in case the game is quit early.
 */
StartState::~StartState()
{
	if (_thread != NULL)
		SDL_KillThread(_thread);

	delete _font;
	delete _timer;
	delete _lang;
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

	_game->setResourcePack(NULL);
	if (Options::mute == false
		&& Options::reload == true)
	{
		Mix_CloseAudio();
		_game->initAudio();
	}

/*	std::wostringstream ss;
	ss << L"Loading OpenXcom " << Language::utf8ToWstr(OPENXCOM_VERSION_SHORT) << Language::utf8ToWstr(OPENXCOM_VERSION_GIT) << "...";
	addLine(ss.str());
*/ // kL

	_thread = SDL_CreateThread( // Load the game data in a separate thread
							load,
							(void*)_game);
	if (_thread == NULL) // If we can't create the thread, just load it as usual
		load((void*)_game);
}

/**
 * If the loading fails, it shows an error, otherwise moves on to the game.
 */
void StartState::think()
{
	State::think();
	_timer->think(this, NULL);

	switch (loading)
	{
		case LOADING_FAILED:
			CrossPlatform::flashWindow();

// load CTD_begin
			addLine(L"");
			addLine(L"ERROR:\n" + Language::utf8ToWstr(error));
//			addLine(L"Make sure you installed OpenXcom correctly.");
//			addLine(L"Check the wiki documentation for more details.");
//			addLine(L"");
//			addLine(L"Press any key to continue.");
// load CTD_end.

			loading = LOADING_DONE;
		break;
		case LOADING_SUCCESSFUL:
			CrossPlatform::flashWindow();

			Log(LOG_INFO) << "OpenXcom started!";
			if (Options::reload == false
				&& Options::playIntro)
			{
				const bool letterbox = Options::keepAspectRatio;
				Options::keepAspectRatio = true;

				Options::baseXResolution = Screen::ORIGINAL_WIDTH;
				Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
				_game->getScreen()->resetDisplay(false);

				_game->setState(new IntroState(letterbox));
			}
			else
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

				State* const state = new MainMenuState();
				_game->setState(state);

				if (Options::badMods.empty() == false) // Check for mod loading errors
				{
					std::wostringstream error;
					error << tr("STR_MOD_UNSUCCESSFUL") << L'\x02';
					for (std::vector<std::string>::const_iterator
							i = Options::badMods.begin();
							i != Options::badMods.end();
							++i)
					{
						error << Language::fsToWstr(*i) << L'\n';
					}

					Options::badMods.clear();

					_game->pushState(new ErrorMessageState(
														error.str(),
														state->getPalette(),
														Palette::blockOffset(8)+10,
														"BACK01.SCR",
														6));
				}

				Options::reload = false;
			}

			_game->getCursor()->setVisible();
			_game->getFpsCounter()->setVisible(Options::fpsCounter);
		break;

		default:
		break;
	}
}

/**
 * The game quits if the player presses any key when an error message is on display.
 * @param action - pointer to an Action
 */
void StartState::handle(Action* action)
{
	State::handle(action);

	if (loading == LOADING_DONE)
	{
		if (action->getDetails()->type == SDL_KEYDOWN)
			_game->quit();
	}
}

/**
 * Blinks the cursor and spreads out terminal output.
 */
void StartState::animate()
{
	_anim++;

	if (_anim %15 == 0) // kL
		_cursor->setVisible(!_cursor->getVisible());

	if (loading == LOADING_STARTED)
	{
		std::wostringstream ss;
		ss << L"Loading " << Language::utf8ToWstr(OPENXCOM_VERSION_GIT) << "..."; // kL
//kL	ss << L"Loading OpenXcom " << Language::utf8ToWstr(OPENXCOM_VERSION_SHORT) << Language::utf8ToWstr(OPENXCOM_VERSION_GIT) << "...";
		if (Options::reload == true)
		{
			if (_anim == 2)
				addLine(ss.str());

			// NOTE: may need to set this:
//			if (kL_ready)
//				loading = LOADING_SUCCESSFUL;
//			else
//				kL_ready = true;
		}
/*		if (Options::reload)
		{
			if (_anim == 1)
				addLine_kL();

			if (_anim < 20)
			{
				addLine_kL();
				_dosart = L"Loading oXc_kL ..."; // 18 chars
				addChar_kL(_anim - 2);
			}
			else
				addCursor();

			// NOTE: may need to set this:
//			if (kL_ready)
//				loading = LOADING_SUCCESSFUL;
//			else
//				kL_ready = true;
		} */
		else
		{
			if (_anim == 1 // start.
				|| _anim == 44
				|| _anim == 91
				|| _anim == 115
				|| _anim == 142
				|| _anim == 161
				|| _anim == 187)
			{
				addLine_kL();
			}

			if (_anim == 1)
				addCursor_kL();

			if (_anim < 44) // 1..43
			{
				_dosart = L"DOS/4GW Protected Mode Run-time Version 1.9"; // 43 chars
				addChar_kL(_anim - 1);
			}
			else if (_anim == 44)
				addCursor_kL();

			else if (_anim < 91) // 45..90
			{
				_dosart = L"Copyright (c) Rational Systems, Inc. 1990-1993"; // 46 chars
				addChar_kL(_anim - 45);
			}
			else if (_anim == 91)
			{
				addLine_kL();
				addCursor_kL();
			}

			else if (_anim < 115) // 92..114
			{
				_dosart = L"OpenXcom initialisation"; // 23 chars <-
				addChar_kL(_anim - 92);
			}
			else if (_anim == 115)
			{
				addLine_kL();
				addCursor_kL();
			}

			else if (_anim < 142) // 116..141
			{
/*				if (Options::mute)
					_dosart = L"No Sound Detected";
				else */
				_dosart = L"SoundBlaster Sound Effects"; // 26 chars
				addChar_kL(_anim - 116);
			}
			else if (_anim == 142)
				addCursor_kL();

			else if (_anim < 161) // 143..160
			{
/*				if (Options::preferredMusic == MUSIC_MIDI)
					_dosart = L"General MIDI Music";
				else */
				_dosart = L"SoundBlaster Music"; // 18 chars
				addChar_kL(_anim - 143);
			}
			else if (_anim == 161)
				addCursor_kL();

			else if (_anim < 187) // 162..186
			{
/*				if (Options::preferredMusic != MUSIC_MIDI)
					_dosart = L"Base Port 220 Irq 5 Dma 1";
				else */
				_dosart = L"Base Port 220 Irq 5 Dma 1"; // 25 chars
				addChar_kL(_anim - 162);
			}
			else if (_anim == 187)
			{
				addLine_kL();
				addCursor_kL();
			}

//			else if (_anim < 206) // 188..205
			else if (_anim < 240) // 188..239
			{
//				_dosart = L"Loading oXc_kL ..."; // 18 chars
				_dosart = L"Loading oXc_kL ... .. ..... . ... ..... ... ... . .."; // 52 chars
				addChar_kL(_anim - 188);
			}
//			else if (_anim == 206)
//				addCursor_kL();

			else if (_anim == 240)
			{
				addCursor_kL();

				if (kL_ready == true)
				{
					kL_ready = false;
					loading = LOADING_SUCCESSFUL;
				}
				else
					kL_ready = true;
			}
		}
	}
}

/**
 * Adds a line of text to the terminal and moves the cursor appropriately.
 * @param line - reference a line of text to add
 */
void StartState::addLine(const std::wstring& line)
{
	_output << L"\n" << line;
	_text->setText(_output.str());

	const int
//kL	y = _text->getTextHeight() - _font->getHeight(),
//kL	x = _text->getTextWidth(y / _font->getHeight());
		x = _text->getTextWidth((_text->getTextHeight() - _font->getHeight()) / _font->getHeight()) + 20,	// kL
		y = _text->getTextHeight() - _font->getHeight() + 20;												// kL
	_cursor->setX(x);
	_cursor->setY(y);
}

/**
 * kL.
 */
void StartState::addLine_kL() // kL
{
	_output << L"\n";
}

/**
 * kL.
 */
void StartState::addChar_kL(const size_t nextChar) // kL
{
//	substr(size_type pos = 0, size_type len = npos) const;
/*	for (size_t
			i = 0;
			i < _dosart.length();
			++i) */

	_output << _dosart.at(nextChar);
	_text->setText(_output.str());
}

/**
 * kL.
 */
void StartState::addCursor_kL() // kL
{
	const int
		x = _text->getTextWidth((_text->getTextHeight() - _font->getHeight()) / _font->getHeight()) + 20,
		y = _text->getTextHeight() - _font->getHeight() + 20;

	_cursor->setX(x);
	_cursor->setY(y);
}

/**
 * Loads game data and updates status accordingly.
 * @param game_ptr - pointer to Game!
 * @return, thread status 0 = ok
 */
int StartState::load(void* game_ptr)
{
	Game* const game = (Game*)game_ptr;
	try
	{
		Log(LOG_INFO) << "Loading ruleset...";
		game->loadRuleset();
		Log(LOG_INFO) << "Ruleset loaded.";

		Log(LOG_INFO) << "Loading resources...";
		game->setResourcePack(new XcomResourcePack(game->getRuleset()));
		Log(LOG_INFO) << "Resources loaded.";

		Log(LOG_INFO) << "Loading language...";
		game->defaultLanguage();
		Log(LOG_INFO) << "Language loaded.";

		if (kL_ready == true)				// kL
		{
			kL_ready = false;				// kL
			loading = LOADING_SUCCESSFUL;
		}
		else								// kL
			kL_ready = true;				// kL
	}
	catch (Exception &e)
	{
		error = e.what();
		Log(LOG_ERROR) << error;

		loading = LOADING_FAILED;
	}
	catch (YAML::Exception &e)
	{
		error = e.what();
		Log(LOG_ERROR) << error;

		loading = LOADING_FAILED;
	}

	return 0;
}

}
