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
#ifndef OPENXCOM_TESTSTATE_H
#define OPENXCOM_TESTSTATE_H

#include "../Engine/State.h"

namespace OpenXcom
{

class NumberText;
class Slider;
class Surface;
class SurfaceSet;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * A state purely for testing game functionality.
 * Fun fact, this was the project's original main(),
 * used for testing and implementing basic engine
 * features until it grew a proper structure and was
 * put aside for actual game states. Useful as a
 * sandbox / testing ground.
 */
class TestState
	:
		public State
{

private:
	int _i;

	NumberText* _number;
	Slider* _slider;
	SurfaceSet* _set;
	Text* _text;
	TextButton* _button;
	TextList* _list;
	Window* _window;

	/// Creates a surface with every color in the palette.
	SDL_Surface* testSurface();


	public:
		/// Creates the Test state.
		TestState(Game* game);
		/// Cleans up the Test state.
		~TestState();

		///
		void think();
		///
		void blit();
};

}

#endif
