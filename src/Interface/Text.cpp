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

#include "Text.h"

#include <cctype>
#include <cmath>
#include <sstream>

#include "../Engine/Font.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"


namespace OpenXcom
{
/**
 * Sets up a blank text with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
Text::Text(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,
			y),
		_big(0),
		_small(0),
		_font(0),
		_lang(0),
		_text(L""),
		_wrap(false),
		_invert(false),
		_contrast(false),
		_indent(false),
		_align(ALIGN_LEFT),
		_valign(ALIGN_TOP),
		_color(0),
		_color2(0)
{
}

/**
 *
 */
Text::~Text()
{
}

/**
 * Takes an integer value and formats it as number with separators (spacing the thousands).
 * @param value, The value.
 * @return, The formatted string.
 */
std::wstring Text::formatNumber(
		int value,
		std::wstring currency,
		bool space)
{
	// In the future, the whole setlocale thing should be removed from here.
	// It is inconsistent with the in-game language selection: locale-specific
	// symbols, such as thousands separators, should be determined by the game
	// language, not by system locale.
	//setlocale(LC_MONETARY, ""); // see http://www.cplusplus.com/reference/clocale/localeconv/
	//setlocale(LC_CTYPE, ""); // this is necessary for mbstowcs to work correctly
	//struct lconv* lc = localeconv();

	std::wostringstream ss;

	bool negative = (value < 0);
	ss << (negative? -value: value);

	std::wstring ret = ss.str();

	if (space)
	{
		std::wstring thousands = L"\xA0"; // Language::cpToWstr(lc->mon_thousands_sep);

		size_t spacer = ret.size() - 3;
		while (spacer > 0
			&& spacer < ret.size())
		{
			ret.insert(
					spacer,
					thousands);

			spacer -= 3;
		}
	}

	if (!currency.empty())
		ret.insert(
				0,
				currency);

	if (negative)
		ret.insert(
				0,
				L"-");

	return ret;
}

/**
 * Takes an integer value and formats it as currency,
 * spacing the thousands and adding a $ sign to the front.
 * @param funds The funding value.
 * @return The formatted string.
 */
std::wstring Text::formatFunding(int funds)
{
	return formatNumber(funds, L"$");
}

/**
 * Takes an integer value and formats it as percentage by adding a % sign.
 * @param value The percentage value.
 * @return The formatted string.
 */
std::wstring Text::formatPercentage(int value)
{
	std::wostringstream ss;
	ss << value << "%";

	return ss.str();
}

/**
 * Changes the text to use the big-size font.
 */
void Text::setBig()
{
	_font = _big;

	processText();
}

/**
 * Changes the text to use the small-size font.
 */
void Text::setSmall()
{
	_font = _small;

	processText();
}

/**
 * Returns the font currently used by the text.
 * @return Pointer to font.
 */
Font* Text::getFont() const
{
	return _font;
}

/**
 * Changes the various resources needed for text rendering.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text, and the language affects
 * how the text is rendered.
 * @param big, Pointer to large-size font.
 * @param small, Pointer to small-size font.
 * @param lang, Pointer to current language.
 */
void Text::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_big	= big;
	_small	= small;
	_lang	= lang;
	_font	= _small;

	processText();
}

/**
 * Changes the string displayed on screen.
 * @param text Text string.
 */
void Text::setText(const std::wstring& text)
{
	_text = text;

	processText();

	// If big text won't fit the space, try small text
	if (_font == _big
		&& (getTextWidth() > getWidth() || getTextHeight() > getHeight())
		&& _text[_text.size() - 1] != L'.')
	{
		setSmall();
	}
}

/**
 * Returns the string displayed on screen.
 * @return Text string.
 */
std::wstring Text::getText() const
{
	return _text;
}

/**
 * Enables/disables text wordwrapping. When enabled, lines of
 * text are automatically split to ensure they stay within the
 * drawing area, otherwise they simply go off the edge.
 * @param wrap Wordwrapping setting.
 * @param indent Indent wrapped text.
 */
void Text::setWordWrap(
		bool wrap,
		bool indent)
{
	if (_wrap != wrap
		|| indent != _indent)
	{
		_wrap = wrap;
		_indent = indent;

		processText();
	}
}

/**
 * Enables/disables color inverting. Mostly used to make
 * button text look pressed along with the button.
 * @param invert Invert setting.
 */
void Text::setInvert(bool invert)
{
	_invert = invert;
	_redraw = true;
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape UI.
 * @param contrast, High contrast setting.
 */
void Text::setHighContrast(bool contrast)
{
	_contrast = contrast;
	_redraw = true;
}

/**
 * Changes the way the text is aligned horizontally
 * relative to the drawing area.
 * @param align Horizontal alignment.
 */
void Text::setAlign(TextHAlign align)
{
	_align = align;
	_redraw = true;
}

/**
 * Returns the way the text is aligned horizontally
 * relative to the drawing area.
 * @return Horizontal alignment.
 */
TextHAlign Text::getAlign() const
{
	return _align;
}

/**
 * Changes the way the text is aligned vertically
 * relative to the drawing area.
 * @param valign Vertical alignment.
 */
void Text::setVerticalAlign(TextVAlign valign)
{
	_valign = valign;
	_redraw = true;
}

/**
 * Changes the color used to render the text. Unlike regular graphics,
 * fonts are greyscale so they need to be assigned a specific position
 * in the palette to be displayed.
 * @param color Color value.
 */
void Text::setColor(Uint8 color)
{
	_color = color;
	_color2 = color;
	_redraw = true;
}

/**
 * Returns the color used to render the text.
 * @return Color value.
 */
Uint8 Text::getColor() const
{
	return _color;
}

/**
 * Changes the secondary color used to render the text. The text
 * switches between the primary and secondary color whenever there's
 * a 0x01 in the string.
 * @param color Color value.
 */
void Text::setSecondaryColor(Uint8 color)
{
	_color2 = color;
	_redraw = true;
}

/**
 * Returns the secondary color used to render the text.
 * @return Color value.
 */
Uint8 Text::getSecondaryColor() const
{
	return _color2;
}

/**
 * Returns the rendered text's height. Useful to check if wordwrap applies.
 * @return Height in pixels.
 */
int Text::getTextHeight() const
{
	int height = 0;
	for (std::vector<int>::const_iterator
			i = _lineHeight.begin();
			i != _lineHeight.end();
			++i)
	{
		height += *i;
	}

	return height;
}

/**
  * Returns the rendered text's width.
  * @return Width in pixels.
  */
int Text::getTextWidth() const
{
	int width = 0;
	for (std::vector<int>::const_iterator
			i = _lineWidth.begin();
			i != _lineWidth.end();
			++i)
	{
		if (*i > width)
			width = *i;
	}

	return width;
}

/**
 * Takes care of any text post-processing like calculating
 * line metrics for alignment and wordwrapping if necessary.
 */
void Text::processText()
{
	if (_font == 0 || _lang == 0)
		return;

	std::wstring* str = &_text;

	if (_wrap) // Use a separate string for wordwrapping text
	{
		_wrappedText = _text;
		str = &_wrappedText;
	}

	_lineWidth.clear();
	_lineHeight.clear();

	bool start = true;
	int
		width = 0,
		word = 0;
	size_t space = 0;

	Font* font = _font;

	for (size_t // Go through the text character by character
		c = 0;
		c <= str->size();
		++c)
	{
		if (c == str->size() // End of the line
			|| Font::isLinebreak((*str)[c]))
		{
			// Add line measurements for alignment later
			_lineWidth.push_back(width);
			_lineHeight.push_back(font->getCharSize(L'\n').h);

			width = 0;
			word = 0;
			start = true;

			if (c == str->size())
				break;

			else if ((*str)[c] == 2) // \x02 marks start of small text
				font = _small;
		}
		else if (Font::isSpace((*str)[c]) // Keep track of spaces for wordwrapping
			|| Font::isSeparator((*str)[c]))
		{
			space = c;
			width += font->getCharSize((*str)[c]).w;
			word = 0;
			start = false;
		}
		else if ((*str)[c] != 1) // Keep track of the width of the last line and word
		{
			if (font->getChar((*str)[c]) == 0)
				(*str)[c] = L'?';

			int charWidth = font->getCharSize((*str)[c]).w;
			width += charWidth;
			word += charWidth;

			if (_wrap // Wordwrap if the last word doesn't fit the line
				&& width >= getWidth()
				&& !start)
			{
				if (_lang->getTextWrapping() == WRAP_WORDS
					|| Font::isSpace((*str)[c]))
				{
					width -= word; // Go back to the last space and put a linebreak there

					size_t indent = space;
					if (Font::isSpace((*str)[space]))
					{
						width -= font->getCharSize((*str)[space]).w;
						(*str)[space] = L'\n';
					}
					else
					{
						str->insert(space + 1, L"\n");
						indent++;
					}

					if (_indent)
					{
						str->insert(indent + 1, L" \xA0");
						width += font->getCharSize(L' ').w + font->getCharSize(L'\xA0').w;
					}
				}
				else if (_lang->getTextWrapping() == WRAP_LETTERS) // Go back to the last letter and put a linebreak there
				{
					str->insert(c, L"\n");
					width -= charWidth;
				}

				_lineWidth.push_back(width);
				_lineHeight.push_back(font->getCharSize(L'\n').h);

				if (_lang->getTextWrapping() == WRAP_WORDS)
					width = word;
				else if (_lang->getTextWrapping() == WRAP_LETTERS)
					width = 0;

				start = true;
			}
		}
	}

	_redraw = true;
}

/**
 * Calculates the starting X position for a line of text.
 * @param line, The line number (0 = first, etc).
 * @return, The X position in pixels.
 */
int Text::getLineX(int line) const
{
	int x = 0;

	switch (_lang->getTextDirection())
	{
		case DIRECTION_LTR:
			switch (_align)
			{
				case ALIGN_LEFT:
				break;
				case ALIGN_CENTER:
					x = static_cast<int>(
							ceil(static_cast<double>(getWidth() + _font->getSpacing() - _lineWidth[line]) / 2.0));
				break;
				case ALIGN_RIGHT:
					x = getWidth() - 1 - _lineWidth[line];
				break;
			}
		break;
		case DIRECTION_RTL:
			switch (_align)
			{
				case ALIGN_LEFT:
					x = getWidth() - 1;
				break;
				case ALIGN_CENTER:
					x = getWidth() - static_cast<int>(
							ceil(static_cast<double>(getWidth() + _font->getSpacing() - _lineWidth[line]) / 2.0));
				break;
				case ALIGN_RIGHT:
					x = _lineWidth[line];
				break;
			}
		break;
	}

	return x;
}


namespace
{

struct PaletteShift
{
	static inline void func(
			Uint8& dest,
			Uint8& src,
			int off,
			int mult,
			int mid)
	{
		if (src)
		{
			int inverseOffset = mid? 2 * (mid - src): 0;
			dest = off + (src * mult) + inverseOffset;
		}
	}
};

}


/**
 * Draws all the characters in the text with a really
 * nasty complex gritty text rendering algorithm logic stuff.
 */
void Text::draw()
{
	Surface::draw();

	if (_text.empty() || _font == 0)
		return;

	// Show text borders for debugging
	if (Options::debugUi)
	{
		SDL_Rect r;
		r.w = getWidth();
		r.h = getHeight();
		r.x = 0;
		r.y = 0;
		this->drawRect(&r, 5);

		r.w -= 2;
		r.h -= 2;
		r.x++;
		r.y++;
		this->drawRect(&r, 0);
	}

	int
		x = 0,
		y = 0,
		line = 0,
		height = 0,
		color = _color;

	Font* font = _font;
	std::wstring* s = &_text;

	for (std::vector<int>::iterator
			i = _lineHeight.begin();
			i != _lineHeight.end();
			++i)
	{
		height += *i;
	}

	switch (_valign)
	{
		case ALIGN_TOP:
			y = 0;
		break;
		case ALIGN_MIDDLE:
			y = static_cast<int>(
					ceil(static_cast<double>(getHeight() - height) / 2.0));
		break;
		case ALIGN_BOTTOM:
			y = getHeight() - height;
		break;
	}

	x = getLineX(line);

	if (_wrap)
		s = &_wrappedText;

	// Set up text color
	int mult = 1;
	if (_contrast)
		mult = 3;
//		mult = 4; // kL

	// Set up text direction
	int dir = 1;
	if (_lang->getTextDirection() == DIRECTION_RTL)
		dir = -1;

	// Invert text by inverting the font palette on index 3 (font palettes use indices 1-5)
	int mid = _invert? 3: 0;

	// Draw each letter one by one
	for (std::wstring::iterator
			c = s->begin();
			c != s->end();
			++c)
	{
		if (Font::isSpace(*c))
			x += dir * font->getCharSize(*c).w;
		else if (Font::isLinebreak(*c))
		{
			line++;

			y += font->getCharSize(*c).h;
			x = getLineX(line);

			if (*c == L'\x02')
			{
				font = _small;
			}
		}
		else if (*c == L'\x01')
			color = (color == _color? _color2: _color);
		else
		{
			if (dir < 0)
				x += dir * font->getCharSize(*c).w;

			Surface* chr = font->getChar(*c);
			chr->setX(x);
			chr->setY(y);
			ShaderDraw<PaletteShift>(
								ShaderSurface(this, 0, 0),
								ShaderCrop(chr),
								ShaderScalar(color),
								ShaderScalar(mult),
								ShaderScalar(mid));

			if (dir > 0)
				x += dir * font->getCharSize(*c).w;
		}
	}
}

}
