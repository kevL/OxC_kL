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

#ifndef OPENXCOM_ARROWBUTTON_H
#define OPENXCOM_ARROWBUTTON_H

#include "ImageButton.h"


namespace OpenXcom
{

enum ArrowShape
{
	ARROW_NONE,			// 0
	ARROW_BIG_UP,		// 1
	ARROW_BIG_DOWN,		// 2
	ARROW_SMALL_UP,		// 3
	ARROW_SMALL_DOWN,	// 4
	ARROW_SMALL_LEFT,	// 5
	ARROW_SMALL_RIGHT	// 6
};


class TextList;
class Timer;


/**
 * Button with an arrow on it.
 * @note Can be used for scrolling lists, spinners, etc. Contains various
 * arrow shapes.
 */
class ArrowButton
	:
		public ImageButton
{

private:
	ArrowShape _shape;
	TextList* _list;
	Timer* _timer;


protected:
	bool isButtonHandled(Uint8 btn = 0);


	public:
		/// Creates a new arrow button with the specified size and position.
		ArrowButton(
				ArrowShape shape,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the arrow button.
		~ArrowButton();

		/// Sets the arrow button's color.
		void setColor(Uint8 color);
		/// Sets the arrow button's shape.
		void setShape(ArrowShape shape);

		/// Sets the arrow button's list.
		void setTextList(TextList* textList);

		/// Handles the timers.
		void think();
		/// Scrolls the list.
		void scroll();
		/// Draws the arrow button.
		void draw();

		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state);
		/// Special handling for mouse releases.
		void mouseRelease(Action* action, State* state);
		/// Special handling for mouse clicks.
		void mouseClick(Action* action, State* state);
};

}

#endif
