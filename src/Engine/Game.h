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

#ifndef OPENXCOM_GAME_H
#define OPENXCOM_GAME_H

//#include <list>
//#include <string>

//#include <SDL.h>


namespace OpenXcom
{

class Cursor;
class FpsCounter;
class Language;
class ResourcePack;
class Ruleset;
class SavedGame;
class Screen;
class State;


/**
 * The core of the game engine, manages the game's entire contents and structure.
 * Takes care of encapsulating all the core SDL commands, provides access to all
 * the game's resources and contains a stack state machine to handle all the
 * initializations, events and blits of each state, as well as transitions.
 */
class Game
{

private:
	static const double VOLUME_GRADIENT;

	bool
		_init,
		_inputActive,
		_quit;
	int
		_delaytime,
		_timeUntilNextFrame;
	unsigned int _timeOfLastFrame;

	Cursor		* _cursor;
	FpsCounter	* _fpsCounter;
	Language	* _lang;
	ResourcePack* _res;
	Ruleset		* _rules;
	SavedGame	* _save;
	Screen		* _screen;

	SDL_Event _event;

	std::list<State*>
		_deleted,
		_states;


	public:
		/// Creates a new game and initializes SDL.
		Game(const std::string& title);
		/// Cleans up all the game's resources and shuts down SDL.
		~Game();

		/// Starts the game's state machine.
		void run();
		/// Quits the game.
		void quit();

		/// Sets the game's audio volume.
		void setVolume(
				int sound,
				int music,
				int ui);
		/// Adjusts a linear volume level to an exponential one.
		static double volumeExponent(int volume);

		/// Gets the game's display screen.
		Screen* getScreen() const;
		/// Gets the game's cursor.
		Cursor* getCursor() const;
		/// Gets the FpsCounter.
		FpsCounter* getFpsCounter() const;

		/// Resets the state stack to a new state.
		void setState(State* state);
		/// Pushes a new state into the state stack.
		void pushState(State* state);
		/// Pops the last state from the state stack.
		void popState();

		/// Gets the currently loaded language.
		Language* getLanguage() const;
		/// Loads a new language for the game.
		void loadLanguage(const std::string& filename);

		/// Gets the currently loaded resource pack.
		ResourcePack* getResourcePack() const;
		/// Sets a new resource pack for the game.
		void setResourcePack(ResourcePack* const res);

		/// Gets the currently loaded saved game.
		SavedGame* getSavedGame() const;
		/// Sets a new saved game for the game.
		void setSavedGame(SavedGame* const save);

		/// Gets the currently loaded ruleset.
		Ruleset* getRuleset() const;
		/// Loads a new ruleset for the game.
		void loadRuleset();

		/// Sets whether the mouse cursor is activated.
		void setInputActive(bool active);

		/// Gets the quantity of currently running states.
		int getQtyStates() const;
		/// Returns whether current state is the param state
		bool isState(State* state) const;

		/// Returns whether the game is shutting down.
		bool isQuitting() const;

		/// Sets up the default language.
		void defaultLanguage();

		/// Sets up the audio.
		void initAudio();
};

}

#endif
