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

#ifndef OPENXCOM_STARTSTATE_H
#define OPENXCOM_STARTSTATE_H

#include <sstream>

#include "../Engine/State.h"


namespace OpenXcom
{

class Font; // load CTD
class Language; // load CTD
class Text; // load CTD
class Timer; // load CTD
//class Surface; // kL


enum LoadingPhase
{
	LOADING_STARTED,	// 0
	LOADING_FAILED,		// 1
	LOADING_SUCCESSFUL,	// 2
	LOADING_DONE		// 3
};


/**
 * Initializes the game and loads all required content.
 */
class StartState
	:
		public State
{

private:
	// load CTD
	int _anim;

	Font* _font;
	Language* _lang;
	Text
		* _cursor,
		* _text;
	Timer* _timer;
	// load CTD_end.

	SDL_Thread* _thread;
//	Surface* _surface; // kL

	std::wostringstream _output; // load CTD


	public:
		static LoadingPhase loading;
		static std::string error;

		/// Creates the Start state.
		StartState(Game* game);
		/// Cleans up the Start state.
		~StartState();

		/// Reset everything.
		void init();
		/// Displays messages.
		void think();

		/// Handles key clicks.
		void handle(Action* action);

		/// Animates the terminal.
		void animate(); // load CTD
		/// Adds a line of text.
		void addLine(const std::wstring& line); // load CTD

		/// Loads the game resources.
		static int load(void* game_ptr);
};

}

#endif
