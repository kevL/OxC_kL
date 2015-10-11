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

#ifndef OPENXCOM_SCROLLBAR_H
#define OPENXCOM_SCROLLBAR_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class TextList;
class Timer;


/**
 * Vertical scrollbar control to select from a range of values.
 */
class ScrollBar
	:
		public InteractiveSurface
{

private:
	bool
		_contrast,
		_pressed;
	int _offset;
	Uint8 _color;

	SDL_Rect _thumbRect;

	Surface
		* _bg,
		* _track,
		* _thumb;
	TextList* _list;
	Timer* _timer;

	/// Draws the scrollbar track.
	void drawTrack();
	/// Draws the scrollbar thumb.
	void drawThumb();


	public:
		/// Creates a new scrollbar with the specified size and position.
		ScrollBar(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the scrollbar.
		~ScrollBar();

		/// Sets the X position of the surface.
		void setX(int x);
		/// Sets the Y position of the surface.
		void setY(int y);
		/// Sets the height of the surface.
		void setHeight(int height);

		/// Sets the scrollbar's color.
		void setColor(Uint8 color);
		/// Gets the scrollbar's color.
		Uint8 getColor() const;
		/// Sets the scrollbar's high contrast color setting.
		void setHighContrast(bool contrast = true);

		/// Sets the scrollbar's list.
		void setTextList(TextList* textList);

		/// Sets the background for the track.
		void setBackground(Surface* bg);
		/// Sets the scrollbar's palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256);

		/// Blits the scrollbar onto another surface.
		void blit(Surface* surface);

		/// Moves the scrollbar.
		void handle(Action* action, State* state);
		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state);
		/// Special handling for mouse releases.
		void mouseRelease(Action* action, State* state);

		/// Gives ticks to keyboard press.
		void think();
		/// Scrolls the list.
		void scroll();
		/// Handles keyboard presses.
		void keyboardPress(Action* action, State* state);
		/// Handles keyboard releases.
		void keyboardRelease(Action* action, State* state);

		/// Draws the scrollbar contents.
		void draw();
};

}

#endif
