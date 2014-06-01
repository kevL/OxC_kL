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

//kL class Font;
//kL class Language;
//kL class Text;
//kL class Timer;
class Surface; // kL


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
//kL	int _anim;

//kL	Font* _font;
//kL	Language* _lang;
//kL	Text
//kL		* _cursor,
//kL		* _text;
//kL	Timer* _timer;

	SDL_Thread* _thread;
	Surface* _surface; // kL

//kL	std::wostringstream _output;


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
//kL	void animate();

		/// Adds a line of text.
//kL	void addLine(const std::wstring& str);

		/// Loads the game resources.
		static int load(void* game_ptr);
};

}

#endif
