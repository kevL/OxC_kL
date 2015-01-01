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

#ifndef OPENXCOM_STARTSTATE_H
#define OPENXCOM_STARTSTATE_H

#include <sstream>

#include "../Engine/State.h"


namespace OpenXcom
{

class Font;
class Language;
class Text;
class Timer;


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
	size_t _anim;

	Font* _font;
	Language* _lang;
	Text
		* _cursor,
		* _text;
	Timer* _timer;

	SDL_Thread* _thread;

	std::wostringstream _output;
	std::wstring _dosart; // kL


	public:
		static LoadingPhase loading;
		static std::string error;
		static bool kL_ready; // kL

		/// Creates the Start state.
		StartState();
		/// Cleans up the Start state.
		~StartState();

		/// Reset everything.
		void init();
		/// Displays messages.
		void think();

		/// Handles key clicks.
		void handle(Action* action);

		/// Animates the terminal.
		void animate();
		/// Adds a line of text.
		void addLine(const std::wstring& line);
		/// kL.
		void addLine_kL(); // kL
		/// kL.
		void addChar_kL(const size_t nextChar); // kL
		/// kL.
		void addCursor_kL(); // kL

		/// Loads the game resources.
		static int load(void* game_ptr);
};

}

#endif
