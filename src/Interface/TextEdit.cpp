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

#include "TextEdit.h"

//#include <sstream>

#include "../Engine/Action.h"
//#include "../Engine/Font.h"
//#include "../Engine/Options.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

/**
 * Sets up a blank text edit with the specified size and position.
 * @param state		- pointer to state the text edit belongs to
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
TextEdit::TextEdit(
		State* state,
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_blink(true),
		_modal(true),
		_ascii(L'A'),
		_caretPos(0),
		_numerical(false),
		_change(0),
		_state(state)
{
	_isFocused = false;

	_text	= new Text(width, height, 0, 0);
	_timer	= new Timer(100);
	_timer->onTimer((SurfaceHandler)& TextEdit::blink);

	_caret	= new Text(17, 17, 0, 0);
	_caret->setText(L"|");
}

/**
 * Deletes contents.
 */
TextEdit::~TextEdit()
{
	delete _text;
	delete _caret;
	delete _timer;

	SDL_EnableKeyRepeat( // in case it was left focused
					0,
					SDL_DEFAULT_REPEAT_INTERVAL);

	_state->setModal(NULL);
}

/**
 * Passes events to internal components.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void TextEdit::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state);

	if (_isFocused == true
		&& _modal == true
		&& action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& (   action->getAbsoluteXMouse() < getX()
			|| action->getAbsoluteXMouse() >= getX() + getWidth()
			|| action->getAbsoluteYMouse() < getY()
			|| action->getAbsoluteYMouse() >= getY() + getHeight()))
	{
		setFocus(false);
	}
}

/**
 * Controls the blinking animation when the text edit is focused.
 * @param focus - true if focused
 * @param modal - true to lock input to this control (default true)
 */
void TextEdit::setFocus(
		bool focus,
		bool modal)
{
	_modal = modal;

	if (focus != _isFocused)
	{
		_redraw = true;

		InteractiveSurface::setFocus(focus);

		if (_isFocused == true)
		{
			SDL_EnableKeyRepeat(
							SDL_DEFAULT_REPEAT_DELAY,
							SDL_DEFAULT_REPEAT_INTERVAL);

			_caretPos = _value.length();
			_blink = true;
			_timer->start();

			if (_modal == true)
				_state->setModal(this);
		}
		else
		{
			_blink = false;
			_timer->stop();

			SDL_EnableKeyRepeat(
							0,
							SDL_DEFAULT_REPEAT_INTERVAL);

			if (_modal == true)
				_state->setModal(NULL);
		}
	}
}

/** kL_note: Is this still needed....
 * Check if the player is currently typing in this box.
 * @return, true if the player is typing in this box
 */
/* bool TextEdit::isFocused()
{
	return _isFocused;
} */

/**
 * Changes the text edit to use the big-size font.
 */
void TextEdit::setBig()
{
	_text->setBig();
	_caret->setBig();
}

/**
 * Changes the text edit to use the small-size font.
 */
void TextEdit::setSmall()
{
	_text->setSmall();
	_caret->setSmall();
}

/**
 * Changes the various fonts for the text edit to use.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void TextEdit::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_text->initText(big, small, lang);
	_caret->initText(big, small, lang);
}

/**
 * Changes the string displayed on screen.
 * @param text - reference the text string
 */
void TextEdit::setText(const std::wstring& text)
{
	_value = text;
	_caretPos = _value.length();
	_redraw = true;
}

/**
 * Returns the string displayed on screen.
 * @return, text string
 */
std::wstring TextEdit::getText() const
{
	return _value;
}

/**
 * Sets the previous string value (before editing) so that it may be retrieved
 * if/when an edit-operation is cancelled.
 * @param text - reference the text string
 */
void TextEdit::setTextStored(const std::wstring text)
{
	_valueStored = text;
}

/**
 * Gets the previous string value so that it may be reinstated if an editing
 * operation is cancelled.
 * @return, text string
 */
std::wstring TextEdit::getTextStored() const
{
	return _valueStored;
}

/**
 * Enables/disables text wordwrapping. When enabled, lines of
 * text are automatically split to ensure they stay within the
 * drawing area, otherwise they simply go off the edge.
 * @param wrap - wordwrapping setting (default true)
 */
void TextEdit::setWordWrap(bool wrap)
{
	_text->setWordWrap(wrap);
}

/**
 * Enables/disables color inverting. Mostly used to make
 * button text look pressed along with the button.
 * @param invert - invert setting (default true)
 */
void TextEdit::setInvert(bool invert)
{
	_text->setInvert(invert);
	_caret->setInvert(invert);
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape text.
 * @param contrast - high contrast setting (default true)
 */
void TextEdit::setHighContrast(bool contrast)
{
	_text->setHighContrast(contrast);
	_caret->setHighContrast(contrast);
}

/**
 * Changes the way the text is aligned horizontally relative to the drawing area.
 * @param align - horizontal alignment
 */
void TextEdit::setAlign(TextHAlign align)
{
	_text->setAlign(align);
}

/**
 * Changes the way the text is aligned vertically relative to the drawing area.
 * @param valign - vertical alignment
 */
void TextEdit::setVerticalAlign(TextVAlign valign)
{
	_text->setVerticalAlign(valign);
}

/**
 * Restricts the text to only numerical input.
 * @param numerical - numerical restriction (default true)
 */
void TextEdit::setNumerical(bool numerical)
{
	_numerical = numerical;
}

/**
 * Changes the color used to render the text. Unlike regular graphics,
 * fonts are greyscale so they need to be assigned a specific position
 * in the palette to be displayed.
 * @param color - color value
 */
void TextEdit::setColor(Uint8 color)
{
	_text->setColor(color);
	_caret->setColor(color);
}

/**
 * Returns the color used to render the text.
 * @return, color value
 */
Uint8 TextEdit::getColor() const
{
	return _text->getColor();
}

/**
 * Changes the secondary color used to render the text. The text
 * switches between the primary and secondary color whenever there's
 * a 0x01 in the string.
 * @param color - color value
 */
void TextEdit::setSecondaryColor(Uint8 color)
{
	_text->setSecondaryColor(color);
}

/**
 * Returns the secondary color used to render the text.
 * @return, color value
 */
Uint8 TextEdit::getSecondaryColor() const
{
	return _text->getSecondaryColor();
}

/**
 * Replaces a certain amount of colors in the text edit's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void TextEdit::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_text->setPalette(colors, firstcolor, ncolors);
	_caret->setPalette(colors, firstcolor, ncolors);
}

/**
 * Keeps the animation timers running.
 */
void TextEdit::think()
{
	_timer->think(NULL, this);
}

/**
 * Plays the blinking animation when the text edit is focused.
 */
void TextEdit::blink()
{
	_blink = !_blink;
	_redraw = true;
}

/**
 * Adds a flashing | caret to the text to show when this is focused and editable.
 */
void TextEdit::draw()
{
	Surface::draw();

	_text->setText(_value);

	if (Options::keyboardMode == KEYBOARD_OFF)
	{
		std::wstring newValue = _value;

		if (_isFocused == true
			&& _blink == true)
		{
			newValue += _ascii;
			_text->setText(newValue);
		}
	}

	clear();
	_text->blit(this);

	if (Options::keyboardMode == KEYBOARD_ON)
	{
		if (_isFocused == true
			&& _blink == true)
		{
			int x = 0;

			switch (_text->getAlign())
			{
				case ALIGN_LEFT:
					x = 0;
				break;

				case ALIGN_CENTER:
					x = (_text->getWidth() - _text->getTextWidth()) / 2;
				break;

				case ALIGN_RIGHT:
					x = _text->getWidth() - _text->getTextWidth();
			}

			for (size_t
					i = 0;
					i < _caretPos;
					++i)
			{
				x += _text->getFont()->getCharSize(_value[i]).w;
			}

			_caret->setX(x);
			_caret->blit(this);
		}
	}
}

/**
 * Checks if adding a certain character to
 * the text edit will exceed the maximum width.
 * Used to make sure user input stays within bounds.
 * @param fontChar - character to add
 * @return, true if it exceeds
 */
bool TextEdit::exceedsMaxWidth(wchar_t fontChar)
{
	int width = 0;
	std::wstring wst = _value;

	wst += fontChar;
	for (std::wstring::const_iterator
			i = wst.begin();
			i != wst.end();
			++i)
	{
		width += _text->getFont()->getCharSize(*i).w;
	}

	return (width > getWidth());
}

/**
 * Focuses the text edit when it's pressed on.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void TextEdit::mousePress(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_isFocused == false)
			setFocus(true);
		else
		{
			const double
				mouseX = action->getRelativeXMouse(),
				scaleX = action->getXScale();
			double width = 0.;
			size_t caret = 0;

			for (std::wstring::const_iterator
					i = _value.begin();
					i != _value.end();
					++i)
			{
				if (mouseX <= width)
					break;

				width += static_cast<double>(_text->getFont()->getCharSize(*i).w / 2) * scaleX;
				if (mouseX <= width)
					break;

				++caret;
				width += static_cast<double>(_text->getFont()->getCharSize(*i).w / 2) * scaleX;
			}

			_caretPos = caret;
		}
	}

	InteractiveSurface::mousePress(action, state);
}

/**
 * Changes the text edit according to keyboard input,
 * and unfocuses the text if Enter is pressed.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void TextEdit::keyboardPress(Action* action, State* state)
{
	if (Options::keyboardMode == KEYBOARD_OFF)
	{
		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_UP:
				++_ascii;
				if (_ascii > L'~')
					_ascii = L' ';
			break;
			case SDLK_DOWN:
				--_ascii;
				if (_ascii < L' ')
					_ascii = L'~';
			break;
			case SDLK_LEFT:
				if (_value.length() > 0)
					_value.resize(_value.length() - 1);
			break;
			case SDLK_RIGHT:
				if (exceedsMaxWidth(_ascii) == false)
					_value += _ascii;
		}
	}
	else if (Options::keyboardMode == KEYBOARD_ON)
	{
		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_LEFT:
				if (_caretPos > 0)
					--_caretPos;
			break;
			case SDLK_RIGHT:
				if (_caretPos < _value.length())
					++_caretPos;
			break;
			case SDLK_HOME:
				_caretPos = 0;
			break;
			case SDLK_END:
				_caretPos = _value.length();
			break;
			case SDLK_BACKSPACE:
				if (_caretPos > 0)
				{
					_value.erase(_caretPos - 1, 1);
					--_caretPos;
				}
			break;
			case SDLK_DELETE:
				if (_caretPos < _value.length())
					_value.erase(_caretPos, 1);
			break;
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (_value.empty() == false)
					setFocus(false);
			break;

			default:
				const Uint16 key = action->getDetails()->key.keysym.unicode;

				if (((_numerical == true
						&& key >= L'0'
						&& key <= L'9')
					|| (_numerical == false
						&& ((key >= L' '
								&& key <= L'~')
							|| key >= 160)))
					&& exceedsMaxWidth(static_cast<wchar_t>(key)) == false)
				{
					_value.insert(
								_caretPos,
								1,
								static_cast<wchar_t>(action->getDetails()->key.keysym.unicode));
					++_caretPos;
				}
		}
	}

	_redraw = true;

	if (_change)
		(state->*_change)(action);

	InteractiveSurface::keyboardPress(action, state);
}

/**
 * Sets a function to be called every time the text changes.
 * @param handler Action handler.
 */
void TextEdit::onChange(ActionHandler handler)
{
	_change = handler;
}

}
