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

#ifndef OPENXCOM_TEXTEDIT_H
#define OPENXCOM_TEXTEDIT_H

#include "Text.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Timer;


/**
 * Editable version of Text.
 * Receives keyboard input to allow the player to change the text.
 */
class TextEdit
	:
		public InteractiveSurface
{

private:
	bool
		_blink,
		_modal,
		_numerical;
	size_t _caretPos;
	wchar_t _ascii;

	std::wstring _value;

	ActionHandler _change;
	State* _state;
	Text
		* _caret,
		* _text;
	Timer * _timer;

	/// Checks if a character will exceed the maximum width.
	bool exceedsMaxWidth(wchar_t c);


	public:
		/// Creates a new text edit with the specified size and position.
		TextEdit(
				State* state,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the text edit.
		~TextEdit();

		/// Handle focus.
		void handle(Action* action, State* state);
		/// Sets focus on this text edit.
		void setFocus(
				bool focus,
				bool modal = true);
		/// kL. Check if the player is currently typing in this box.
//		bool isFocused(); // kL, is this still needed.

		/// Sets the text size to big.
		void setBig();
		/// Sets the text size to small.
		void setSmall();

		/// Initializes the text edit's resources.
		void initText(
				Font* big,
				Font* small,
				Language* lang);

		/// Sets the text's string.
		void setText(const std::wstring& text);
		/// Gets the text edit's string.
		std::wstring getText() const;

		/// Sets the text edit's wordwrap setting.
		void setWordWrap(bool wrap = true);

		/// Sets the text edit's color invert setting.
		void setInvert(bool invert = true);
		/// Sets the text edit's high contrast color setting.
		void setHighContrast(bool contrast = true);

		/// Sets the text edit's horizontal alignment.
		void setAlign(TextHAlign align);
		/// Sets the text edit's vertical alignment.
		void setVerticalAlign(TextVAlign valign);

		/// Sets the text edit to numerical input.
		void setNumerical(bool numerical = true);

		/// Sets the text edit's color.
		void setColor(Uint8 color);
		/// Gets the text edit's color.
		Uint8 getColor() const;
		/// Sets the text edit's secondary color.
		void setSecondaryColor(Uint8 color);
		/// Gets the text edit's secondary color.
		Uint8 getSecondaryColor() const;
		/// Sets the text edit's palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256);

		/// Handles the timers.
		void think();
		/// Plays the blinking animation.
		void blink();
		/// Draws the text edit.
		void draw();

		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state);
		/// Special handling for keyboard presses.
		void keyboardPress(Action* action, State* state);
		/// Hooks an action handler to when the slider changes.
		void onChange(ActionHandler handler);
};

}

#endif
