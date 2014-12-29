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

#include "Game.h"

// kL_begin: Old
/* #ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <SDL_syswm.h>
#endif */ // kL_end.


//#include <cmath>
//#include <sstream>
//#include <SDL_mixer.h>

//#include "Adlib/adlplayer.h"

#include "Action.h"
//#include "CrossPlatform.h"
//#include "Exception.h"
#include "InteractiveSurface.h"
#include "Language.h"
//#include "Logger.h"
#include "Music.h"
//#include "Options.h"
//#include "Palette.h"
//#include "Screen.h"
#include "Sound.h"
#include "State.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"

#include "../Menu/TestState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

const double Game::VOLUME_GRADIENT = 10.;


/**
 * Starts up SDL with all the subsystems and SDL_mixer for audio processing,
 * creates the display screen and sets up the cursor.
 * @param title - reference the title of the game window
 */
Game::Game(const std::string& title)
	:
		_screen(NULL),
		_cursor(NULL),
		_lang(NULL),
		_res(NULL),
		_save(NULL),
		_rules(NULL),
		_quit(false),
		_init(false),
		_inputActive(true),
		_timeUntilNextFrame(0),
		_debugCycle(-1),
		_debugCycle_b(-1)
{
	//Log(LOG_INFO) << "Create Game";
	Options::reload = false;
	Options::mute = false;

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		throw Exception(SDL_GetError());
	}
	Log(LOG_INFO) << "SDL initialized.";

	// Initialize SDL_mixer
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		Log(LOG_ERROR) << SDL_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
	}
	else
		initAudio();

	// trap the mouse inside the window
	SDL_WM_GrabInput(Options::captureMouse);

	// Set the window icon
	CrossPlatform::setWindowIcon(103, "openxcom.png");

	// Set the window caption
	SDL_WM_SetCaption(title.c_str(), NULL);

	SDL_EnableUNICODE(1);

	// Create display
	_screen = new Screen();
/*	Screen::BASE_WIDTH = Options::getInt("baseXResolution");
	Screen::BASE_HEIGHT = Options::getInt("baseYResolution");
	_screen = new Screen(
			Options::getInt("displayWidth"),
			Options::getInt("displayHeight"), 0,
			Options::getBool("fullscreen"),
			Options::getInt("windowedModePositionX"),
			Options::getInt("windowedModePositionY")); */ // kL

	// Create cursor
	_cursor = new Cursor(9, 13);
	_cursor->setColor(Palette::blockOffset(15)+12);

	// Create invisible hardware cursor to workaround bug with absolute positioning pointing devices
//	SDL_ShowCursor(SDL_ENABLE);
	SDL_ShowCursor(SDL_DISABLE); // kL
	Uint8 cursor = 0;
	SDL_SetCursor(SDL_CreateCursor(&cursor, &cursor, 1, 1, 0, 0));

	// Create fps counter
	_fpsCounter = new FpsCounter(15, 5, 0, 0);

	// Create blank language
	_lang = new Language();

	_timeOfLastFrame = 0;
}

/**
 * Deletes the display screen, cursor, states and shuts down all the SDL subsystems.
 */
Game::~Game()
{
	//Log(LOG_INFO) << "Delete Game";
	Sound::stop();
	Music::stop();

	for (std::list<State*>::const_iterator
			i = _states.begin();
			i != _states.end();
			++i)
	{
		delete *i;
	}

	SDL_FreeCursor(SDL_GetCursor());

	delete _cursor;
	delete _lang;
	delete _res;
	delete _save;
	delete _rules;
	delete _screen;
	delete _fpsCounter;

	Mix_CloseAudio();

	SDL_Quit();
}

/**
 * The state machine takes care of passing all the events from SDL to the
 * active state, running any code within and blitting all the states and
 * cursor to the screen. This is run indefinitely until the game quits.
 */
void Game::run()
{
	enum ApplicationState
	{
		RUNNING	= 0,
		SLOWED	= 1,
		PAUSED	= 2
	}
	runningState = RUNNING;

	static const ApplicationState kbFocusRun[4] =
	{
		RUNNING,	// 0
		RUNNING,	// 0
		SLOWED,		// 1
		PAUSED		// 2
	};

	static const ApplicationState stateRun[4] =
	{
		SLOWED,		// 1
		PAUSED,		// 2
		PAUSED,		// 2
		PAUSED		// 2
	};

	// this will avoid processing SDL's resize event on startup, workaround for the heap allocation error it causes.
	bool startupEvent = Options::allowResize;


	while (_quit == false)
	{
		while (_deleted.empty() == false) // Clean up states
		{
			delete _deleted.back();
			_deleted.pop_back();
		}

		if (_init == false) // Initialize active state
		{
			_init = true;
			_states.back()->init();

			_states.back()->resetAll(); // Unpress buttons

			SDL_Event ev; // Refresh mouse position
			int
				x,
				y;
			SDL_GetMouseState(&x, &y);
			ev.type = SDL_MOUSEMOTION;
			ev.motion.x = x;
			ev.motion.y = y;

			Action action = Action(
								&ev,
								_screen->getXScale(),
								_screen->getYScale(),
								_screen->getCursorTopBlackBand(),
								_screen->getCursorLeftBlackBand());
			_states.back()->handle(&action);
		}

		while (SDL_PollEvent(&_event) == 1) // Process SDL input events
		{
			if (_inputActive == false // kL->
				&& _event.type != SDL_MOUSEMOTION)
			{
//				_event.type = SDL_IGNORE; // discard buffered events
				continue;
			}

			if (CrossPlatform::isQuitShortcut(_event) == true)
				_event.type = SDL_QUIT;

			switch (_event.type)
			{
//				case SDL_IGNORE:
//				break;
				case SDL_QUIT:
					quit();
				break;

				case SDL_ACTIVEEVENT:
					switch (reinterpret_cast<SDL_ActiveEvent*>(&_event)->state)
					{
						case SDL_APPACTIVE:
							runningState = reinterpret_cast<SDL_ActiveEvent*>(&_event)->gain? RUNNING: stateRun[Options::pauseMode];
						break;
						case SDL_APPMOUSEFOCUS: // Consciously ignore it.
						break;
						case SDL_APPINPUTFOCUS:
							runningState = reinterpret_cast<SDL_ActiveEvent*>(&_event)->gain? RUNNING: kbFocusRun[Options::pauseMode];
						break;
					}
				break;

				case SDL_VIDEORESIZE:
					if (Options::allowResize)
					{
						if (startupEvent == false)
						{
							Options::newDisplayWidth = Options::displayWidth = std::max(
																					Screen::ORIGINAL_WIDTH,
																					_event.resize.w);
							Options::newDisplayHeight = Options::displayHeight = std::max(
																					Screen::ORIGINAL_HEIGHT,
																					_event.resize.h);
							int
								dX = 0,
								dY = 0;

							Screen::updateScale(
											Options::battlescapeScale,
											Options::battlescapeScale,
											Options::baseXBattlescape,
											Options::baseYBattlescape,
											false);
							Screen::updateScale(
											Options::geoscapeScale,
											Options::geoscapeScale,
											Options::baseXGeoscape,
											Options::baseYGeoscape,
											false);

							for (std::list<State*>::const_iterator
									i = _states.begin();
									i != _states.end();
									++i)
							{
								(*i)->resize(dX, dY);
							}

							_screen->resetDisplay();
						}
						else
							startupEvent = false;
					}
				break;

				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
//kL				if (_inputActive == false) // Skip [ie. postpone] mouse events if they're disabled
//kL					continue;

					runningState = RUNNING;	// re-gain focus on mouse-over or keypress.
											// Go on, feed the event to others
				default:
					Action action = Action(
										&_event,
										_screen->getXScale(),
										_screen->getYScale(),
										_screen->getCursorTopBlackBand(),
										_screen->getCursorLeftBlackBand());
					_screen->handle(&action);
					_cursor->handle(&action);
					_fpsCounter->handle(&action);
					_states.back()->handle(&action);

					if (action.getDetails()->type == SDL_KEYDOWN)
					{
						if (action.getDetails()->key.keysym.sym == SDLK_g // "ctrl-g" grab input
							&& (SDL_GetModState() & KMOD_CTRL) != 0)
						{
							Options::captureMouse = (SDL_GrabMode)(Options::captureMouse == SDL_GRAB_QUERY);
							SDL_WM_GrabInput(Options::captureMouse);
						}
						else if (Options::debug == true)
						{
							if (action.getDetails()->key.keysym.sym == SDLK_t // "ctrl-t" engage TestState
								&& (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								setState(new TestState());
							}
							else if (action.getDetails()->key.keysym.sym == SDLK_u // "ctrl-u" debug UI
								&& (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								Options::debugUi = !Options::debugUi;
								_states.back()->redrawText();
							}
							else if (_save->getDebugMode() == true
								&& action.getDetails()->key.keysym.sym == SDLK_c // kL-> note: 'c' doubles as CreateInventoryTemplate (remarked @ InventoryState)
								&& (SDL_GetModState() & KMOD_CTRL) != 0)
							{
								// "ctrl-c"			- increment to show next area's boundaries
								// "ctrl-shift-c"	- decrement to show previous area's boundaries
								// "ctrl-alt-c"		- toggles between show all areas' boundaries & show current area's boundaries (roughly)
								if ((SDL_GetModState() & KMOD_ALT) != 0)
									std::swap(_debugCycle, _debugCycle_b);
								else if ((SDL_GetModState() & KMOD_SHIFT) != 0)
								{
									if (_debugCycle != -1)
										--_debugCycle;
									else
										_debugCycle_b = -1; // semi-convenient reset for Cycle_b .... hey, at least there *is* one.
								}
								else
									++_debugCycle;
							}
						}
					}
				break;
			}
		}

		if (_inputActive == false)	// kL
			_inputActive = true;	// kL


		if (runningState != PAUSED) // Process rendering
		{
			_states.back()->think(); // Process logic
			_fpsCounter->think();

			if (Options::FPS > 0
				&& !(
					Options::useOpenGL && Options::vSyncForOpenGL))
			{
				// Update our FPS delay time based on the time of the last draw.
				int fps = SDL_GetAppState() & SDL_APPINPUTFOCUS? Options::FPS: Options::FPSInactive;
				if (fps < 1)
					fps = 1;
				_timeUntilNextFrame = static_cast<int>(
											(1000.f / static_cast<float>(fps))
											- static_cast<float>(SDL_GetTicks() - static_cast<Uint32>(_timeOfLastFrame)));
			}
			else
				_timeUntilNextFrame = 0;

			if (_init == true
				&& _timeUntilNextFrame < 1)
			{
				// make a note of when this frame update occured.
				_timeOfLastFrame = SDL_GetTicks();
				_fpsCounter->addFrame();

				_screen->clear();

				std::list<State*>::const_iterator i = _states.end();
				do
				{
					--i;
				}
				while (i != _states.begin()
					&& (*i)->isScreen() == false);

				for (
						;
						i != _states.end();
						++i)
				{
					(*i)->blit();
				}

				_fpsCounter->blit(_screen->getSurface());
				_cursor->blit(_screen->getSurface());

				_screen->flip();
			}
		}

		switch (runningState)	// Save on CPU
		{
			case RUNNING:
				SDL_Delay(1);	// Save CPU from going 100%
			break;
			case SLOWED:
			case PAUSED:
				SDL_Delay(100);	// More slowing down.
			break;
		}
	}

//kL	Options::save();	// kL_note: why this work here but not at main() EXIT,
							// where it clears & rewrites my options.cfg
							// Ps. why are they even doing Options::save() twice
							// ... now they both fuck up.
}

/**
 * Stops the state machine and the game is shut down.
 */
void Game::quit()
{
	if (_save != NULL // Always save ironman
		&& _save->isIronman() == true
		&& _save->getName().empty() == false)
	{
		const std::string filename = CrossPlatform::sanitizeFilename(Language::wstrToFs(_save->getName())) + ".sav";
		_save->save(filename);
	}

	_quit = true;
}

/**
 * Changes the audio volume of the music and sound effect channels.
 * @param sound	- sound volume, from 0 to MIX_MAX_VOLUME
 * @param music	- music volume, from 0 to MIX_MAX_VOLUME
 * @param ui	- ui volume, from 0 to MIX_MAX_VOLUME
 */
void Game::setVolume(
		int sound,
		int music,
		int ui)
{
	if (Options::mute == false)
	{
		if (music > -1)
		{
			music = static_cast<int>(volumeExponent(music) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_VolumeMusic(music);
//			func_set_music_volume(music);
		}

		if (sound > -1)
		{
			sound = static_cast<int>(volumeExponent(sound) * static_cast<double>(SDL_MIX_MAXVOLUME));
			Mix_Volume(-1, sound); // kL_note: this, supposedly, sets volume on *all channels*

			// channel 3: reserved for ambient sound effect.
			Mix_Volume(3, sound / 2);
		}

		if (ui > -1)
		{
			ui = static_cast<int>(volumeExponent(ui) * static_cast<double>(SDL_MIX_MAXVOLUME));
			// ... they use channels #1 & #2 btw. and group them accordingly in initAudio() below_
			Mix_Volume(0, ui); // kL_note: then this sets channel-0 to ui-Volume
			Mix_Volume(1, ui); // and this sets channel-1 to ui-Volume!
			Mix_Volume(2, ui); // and this sets channel-2 to ui-Volume!
		}
	}
}

/**
 *
 */
double Game::volumeExponent(int volume)
{
	return (exp(
				std::log(Game::VOLUME_GRADIENT + 1.) * static_cast<double>(volume) / static_cast<double>(SDL_MIX_MAXVOLUME))
				- 1.)
			/ Game::VOLUME_GRADIENT;
}

/**
 * Returns the display screen used by the game.
 * @return, pointer to the Screen
 */
Screen* Game::getScreen() const
{
	return _screen;
}

/**
 * Returns the mouse cursor used by the game.
 * @return, pointer to the Cursor
 */
Cursor* Game::getCursor() const
{
	return _cursor;
}

/**
 * Returns the FpsCounter used by the game.
 * @return, pointer to the FpsCounter
 */
FpsCounter* Game::getFpsCounter() const
{
	return _fpsCounter;
}

/**
 * Pops all the states currently in stack and pushes in the new state.
 * A shortcut for cleaning up all the old states when they're not necessary
 * like in one-way transitions.
 * @param state - pointer to the new State
 */
void Game::setState(State* state)
{
	while (_states.empty() == false)
		popState();

	pushState(state);
	_init = false;
}

/**
 * Pushes a new state into the top of the stack and initializes it.
 * The new state will be used once the next game cycle starts.
 * @param state - pointer to the new State
 */
void Game::pushState(State* state)
{
	_states.push_back(state);
	_init = false;
}

/**
 * Pops the last state from the top of the stack.
 * Since states can't actually be deleted mid-cycle it's moved into a separate queue
 * which is cleared at the start of every cycle so the transition is seamless.
 */
void Game::popState()
{
	_deleted.push_back(_states.back());
	_states.pop_back();

	_init = false;
}

/**
 * Returns the language currently in use by the game.
 * @return, pointer to the Language
 */
Language* Game::getLanguage() const
{
	return _lang;
}

/**
* Changes the language currently in use by the game.
* @param filename - reference the filename of the language file
*/
void Game::loadLanguage(const std::string& filename)
{
	std::ostringstream ss;
	ss << "Language/" << filename << ".yml";

	ExtraStrings* strings = NULL;
	std::map<std::string, ExtraStrings*> extraStrings = _rules->getExtraStrings();
	if (extraStrings.empty() == false)
	{
		if (extraStrings.find(filename) != extraStrings.end())
			strings = extraStrings[filename];
		else if (extraStrings.find("en-US") != extraStrings.end()) // Fallback
			strings = extraStrings["en-US"];
		else if (extraStrings.find("en-GB") != extraStrings.end())
			strings = extraStrings["en-GB"];
		else
			strings = extraStrings.begin()->second;
	}

	_lang->load(
			CrossPlatform::getDataFile(ss.str()),
			strings);

	Options::language = filename;
}

/**
 * Returns the resource pack currently in use by the game.
 * @return, pointer to the ResourcePack
 */
ResourcePack* Game::getResourcePack() const
{
	return _res;
}

/**
 * Sets a new resource pack for the game to use.
 * @param res - pointer to the ResourcePack
 */
void Game::setResourcePack(ResourcePack* const res)
{
	delete _res;
	_res = res;
}

/**
 * Returns the saved game currently in use by the game.
 * @return, pointer to the SavedGame
 */
SavedGame* Game::getSavedGame() const
{
	return _save;
}

/**
 * Sets a new saved game for the game to use.
 * @param save - pointer to the SavedGame
 */
void Game::setSavedGame(SavedGame* const save)
{
	delete _save;
	_save = save;
}

/**
 * Returns the ruleset currently in use by the game.
 * @return, pointer to the Ruleset
 */
Ruleset* Game::getRuleset() const
{
	return _rules;
}

/**
 * Loads the rulesets specified in the game options.
 */
void Game::loadRuleset()
{
	Options::badMods.clear();

	_rules = new Ruleset(this);

	if (Options::rulesets.empty() == true)
		Options::rulesets.push_back("Xcom1Ruleset");

	for (std::vector<std::string>::const_iterator
			i = Options::rulesets.begin();
			i != Options::rulesets.end();
			)
	{
		try
		{
			_rules->load(*i);
			++i;
		}
		catch (YAML::Exception &e)
		{
			Log(LOG_WARNING) << e.what();

			Options::badMods.push_back(*i);
			Options::badMods.push_back(e.what());

			i = Options::rulesets.erase(i);
		}
	}

	if (Options::rulesets.empty() == true)
	{
		throw Exception("Failed to load ruleset");
	}

	_rules->sortLists();
}

/**
 * Sets whether SDL input is activated.
 * All input events are processed if true, otherwise only mouse motion is handled.
 * @param active - true if SDL input is active
 */
void Game::setInputActive(bool active)
{
	_inputActive = active;
//	_cursor->setVisible(active);
}

/**
 * Gets the quantity of currently running states.
 * @return, qty of states
 */
int Game::getQtyStates() const
{
	return _states.size();
}

/**
 * Returns whether current state is *state
 * @param state - pointer to a State to test against the stack state
 * @return, true if *state is the current state
 */
bool Game::isState(State* state) const
{
	return _states.empty() == false
		&& _states.back() == state;
}

/**
 * Checks if the game is currently quitting.
 * @return, true if the game is in the process of shutting down
 */
bool Game::isQuitting() const
{
	return _quit;
}

/**
 * Loads the most appropriate language given current system and game options.
 */
void Game::defaultLanguage()
{
	std::string defaultLang = "en-US";

	if (Options::language.empty()) // No language set, detect based on system
	{
		std::string locale = CrossPlatform::getLocale();
		std::string lang = locale.substr(0, locale.find_first_of('-'));

		try // Try to load full locale
		{
			loadLanguage(locale);
		}
		catch (std::exception)
		{
			try // Try to load language locale
			{
				loadLanguage(lang);
			}
			catch (std::exception) // Give up, use default
			{
				loadLanguage(defaultLang);
			}
		}
	}
	else
	{
		try // Use options language
		{
			loadLanguage(Options::language);
		}
		catch (std::exception) // Language not found, use default
		{
			loadLanguage(defaultLang);
		}
	}
}

/**
 * Initializes the audio subsystem.
 */
void Game::initAudio()
{
	Uint16 format;
	if (Options::audioBitDepth == 8)
		format = AUDIO_S8;
	else
		format = AUDIO_S16SYS;

	if (Mix_OpenAudio(Options::audioSampleRate, format, 2, 1024) != 0)
	{
		Log(LOG_ERROR) << Mix_GetError();
		Log(LOG_WARNING) << "No sound device detected, audio disabled.";
		Options::mute = true;
	}
	else
	{
		Mix_AllocateChannels(16);

		// Set up UI channels
		Mix_ReserveChannels(4); // 4th channel (#3) is for ambient sFx
		Mix_GroupChannels(0, 2, 0);
		Log(LOG_INFO) << "SDL_mixer initialized.";

		setVolume(
				Options::soundVolume,
				Options::musicVolume,
				Options::uiVolume);
	}
}

/**
 * Gets the country cycle for debugging country regions.
 * @return, the current country cycle setting
 */
int Game::getDebugCycle() const
{
	return _debugCycle;
}

/**
 * Sets the country cycle for debugging country regions.
 * @param cycle - the country cycle to set
 */
void Game::setDebugCycle(const int cycle)
{
	_debugCycle = cycle;
}

}
