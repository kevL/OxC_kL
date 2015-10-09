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

#include "Text.h"

//#include <cmath>
//#include <sstream>

//#include "../Engine/Font.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/ShaderDraw.h"
//#include "../Engine/ShaderMove.h"


namespace OpenXcom
{
/**
 * Sets up a blank text with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
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
			x,y),
		_big(NULL),
		_small(NULL),
		_font(NULL),
		_lang(NULL),
		_wrap(false),
		_invert(false),
		_contrast(1),
		_indent(false),
		_align(ALIGN_LEFT),
		_valign(ALIGN_TOP),
		_color(0),
		_color2(0)
{}

/**
 * dTor.
 */
Text::~Text()
{}

/**
 * Takes an integer value and formats it as number with separators spacing the
 * thousands.
 * @param value		- a value
 * @param space		- true to insert a space every 3 digits (default false)
 * @param currency	- reference a currency symbol (default L"")
 * @return, formatted string
 */
std::wstring Text::formatNumber(
		int64_t value,
		const bool space,
		const std::wstring& currency)
{
	// In the future, the whole setlocale thing should be removed from here.
	// It is inconsistent with the in-game language selection: locale-specific
	// symbols, such as thousands separators, should be determined by the game
	// language, not by system locale.
	//setlocale(LC_MONETARY, ""); // see http://www.cplusplus.com/reference/clocale/localeconv/
	//setlocale(LC_CTYPE, ""); // this is necessary for mbstowcs to work correctly
	//struct lconv* lc = localeconv();

	std::wostringstream woststr;

	const bool neg = (value < 0);
	woststr << (neg ? -value : value);

	std::wstring ret = woststr.str();

	if (space == true)
	{
		const std::wstring thousands = L"\xA0"; // Language::cpToWstr(lc->mon_thousands_sep);

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

	if (currency.empty() == false)
		ret.insert(
				0,
				currency);

	if (neg == true)
		ret.insert(
				0,
				L"-");

	return ret;
}

/**
 * Takes an integer value and formats it as currency by spacing the thousands
 * and adding a $ sign to the front.
 * @param funds - the funding value
 * @return, the formatted string
 */
std::wstring Text::formatFunding(int64_t funds)
{
	return formatNumber(
					funds,
					true,
					L"$");
}

/**
 * Takes an integer value and formats it as percentage by adding a % sign.
 * @param value - the percentage value
 * @return, the formatted string
 */
std::wstring Text::formatPct(int value)
{
	std::wostringstream woststr;
	woststr << value << L"%";

	return woststr.str();
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
 * @return, pointer to Font
 */
Font* Text::getFont() const
{
	return _font;
}

/**
 * Changes the various resources needed for text rendering.
 * @note The different fonts need to be passed in advance since the text size
 * can change mid-text and the language affects how the text is rendered.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void Text::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_big = big;
	_small = small;
	_lang = lang;
	_font = _small;

	processText();
}

/**
 * Changes the string displayed on screen.
 * @param text - reference a text string
 */
void Text::setText(const std::wstring& text)
{
	_text = text;

	processText();

	// if big text won't fit the space, try small text
	if (_font == _big
		&& (getTextWidth() > getWidth() || getTextHeight() > getHeight())
		&& _text[_text.size() - 1] != L'.')
	{
		setSmall();
	}
}

/**
 * Returns the string displayed on screen.
 * @return, text string
 */
std::wstring Text::getText() const
{
	return _text;
}

/**
 * Enables/disables text wordwrapping.
 * @note If enabled lines of text are automatically split to ensure they stay
 * within the drawing area - otherwise they simply go off the edge.
 * @param wrap		- true to wrap text (default true)
 * @param indent	- true to indent wrapped text (default false)
 */
void Text::setWordWrap(
		const bool wrap,
		const bool indent)
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
 * Enables/disables color inverting.
 * @note Mostly used to make button text look pressed along with the button.
 * @param invert - true to invert text (default true)
 */
void Text::setInvert(const bool invert)
{
	_invert = invert;
	_redraw = true;
}

/**
 * Enables/disables high contrast color.
 * @note Mostly used for Battlescape UI.
 * @param contrast - true to set high contrast (default true)
 */
void Text::setHighContrast(const bool contrast)
{
	_contrast = contrast ? 3 : 1;
	_redraw = true;
}

/**
 * Gets if this Text is using high contrast color.
 * @return, true if high contrast
 */
bool Text::getHighContrast() const
{
	return (_contrast != 1);
}

/**
 * Changes the way the text is aligned horizontally relative to the drawing area.
 * @param align - horizontal alignment (enum Text.h)
 */
void Text::setAlign(TextHAlign align)
{
	_align = align;
	_redraw = true;
}

/**
 * Returns the way the text is aligned horizontally relative to the drawing area.
 * @return, horizontal alignment (enum Text.h)
 */
TextHAlign Text::getAlign() const
{
	return _align;
}

/**
 * Changes the way the text is aligned vertically relative to the drawing area.
 * @param valign - vertical alignment (enum Text.h)
 */
void Text::setVerticalAlign(TextVAlign valign)
{
	_valign = valign;
	_redraw = true;
}

/**
 * Changes the color used to render the text.
 * @note Unlike regular graphics fonts are greyscale so they need to be
 * assigned a specific position in the palette to be displayed.
 * @param color - color value
 */
void Text::setColor(Uint8 color)
{
	_color = color;
	_color2 = color;
	_redraw = true;
}

/**
 * Returns the color used to render the text.
 * @return, color value
 */
Uint8 Text::getColor() const
{
	return _color;
}

/**
 * Changes the secondary color used to render the text.
 * @note The text switches between the primary and secondary color whenever
 * there's a 0x01 in the string.
 * @param color - color value
 */
void Text::setSecondaryColor(Uint8 color)
{
	_color2 = color;
	_redraw = true;
}

/**
 * Returns the secondary color used to render the text.
 * @return, color value
 */
Uint8 Text::getSecondaryColor() const
{
	return _color2;
}

/**
 * Gets the number of lines in this Text including wrapping.
 * @return, number of lines
 */
int Text::getNumLines() const
{
	if (_wrap == true)
		return static_cast<int>(_lineHeight.size());

	return 1;
}

/**
 * Returns the rendered text's width.
 * @param line - line to get the width of or -1 to get whole text width (default -1)
 * @return, width in pixels
 */
int Text::getTextWidth(int line) const
{
	if (line == -1)
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

	return _lineWidth[line];
}

/**
 * Returns the rendered text's height.
 * @note Useful to check if wordwrap applies.
 * @param line - line to get the height of or -1 to get whole text height (default -1)
 * @return, height in pixels
 */
int Text::getTextHeight(int line) const
{
	if (line == -1)
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

	return _lineHeight[line];
}

/**
 * Adds to the text's height.
 * @param pad - pixels to add to height of a textline (default 1)
 */
void Text::addTextHeight(int pad)
{
	if (_lineHeight.empty() == false)
		_lineHeight[_lineHeight.size() - 1] += pad;
}

/**
 * Takes care of any text post-processing like calculating line metrics for
 * alignment and wordwrapping if necessary.
 */
void Text::processText() // private.
{
	if (_font != NULL && _lang != NULL)
	{
		std::wstring* wst = &_text;

		if (_wrap == true) // use a separate string for wordwrapping text
		{
			_wrappedText = _text;
			wst = &_wrappedText;
		}

		_lineWidth.clear();
		_lineHeight.clear();

		bool start = true;
		int
			width = 0,
			word = 0;
		size_t
			space = 0,
			textIndentation = 0;

		Font* font = _font;


		for (size_t // go through the text character by character
			i = 0;
			i <= wst->size();
			++i)
		{
			if (i == wst->size() // end of the line
				|| Font::isLinebreak((*wst)[i]) == true)
			{
				// add line measurements for alignment later
				_lineWidth.push_back(width);
				_lineHeight.push_back(font->getCharSize(L'\n').h);

				width = 0;
				word = 0;
				start = true;

				if (i == wst->size())
					break;

				else if ((*wst)[i] == 2) // \x02 marks start of small text
					font = _small;
			}
			else if (Font::isSpace((*wst)[i]) == true // keep track of spaces for word-wrapping
				|| Font::isSeparator((*wst)[i]) == true)
			{
				if (i == textIndentation) // store existing indentation
					++textIndentation;

				space = i;
				width += font->getCharSize((*wst)[i]).w;
				word = 0;
				start = false;
			}
			else if ((*wst)[i] != 1) // keep track of the width of the last line and word
			{
				if (font->getChar((*wst)[i]) == 0)
					(*wst)[i] = L'?';

				const int charWidth = font->getCharSize((*wst)[i]).w;
				width += charWidth;
				word += charWidth;

				if (_wrap == true // word-wrap if the last word doesn't fit the line
					&& width >= getWidth()
					&& start == false)
				{
					size_t indentLocation = i;

					if (_lang->getTextWrapping() == WRAP_WORDS
						|| Font::isSpace((*wst)[i]) == true)
					{
						width -= word; // go back to the last space and put a linebreak there

						indentLocation = space;
						if (Font::isSpace((*wst)[space]) == true)
						{
							width -= font->getCharSize((*wst)[space]).w;
							(*wst)[space] = L'\n';
						}
						else
						{
							wst->insert(space + 1, L"\n");
							++indentLocation;
						}
					}
					else if (_lang->getTextWrapping() == WRAP_LETTERS) // go back to the last letter and put a linebreak there
					{
						wst->insert(i, L"\n");
						width -= charWidth;
					}

					if (textIndentation > 0) // keep initial indentation of text
					{
						wst->insert(indentLocation + 1, L" \xA0", textIndentation);
						indentLocation += textIndentation;
					}

					if (_indent == true) // indent due to word wrap
					{
						wst->insert(indentLocation + 1, L" \xA0");
						width += font->getCharSize(L' ').w + font->getCharSize(L'\xA0').w;
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
}

/**
 * Calculates the starting X position for a line of text.
 * @param line - the line number (0 = first, etc)
 * @return, the X position in pixels
 */
int Text::getLineX(int line) const // private.
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
					x = static_cast<int>(std::ceil(
						static_cast<double>(getWidth() + _font->getSpacing() - _lineWidth[line]) / 2.));
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
					x = getWidth() - static_cast<int>(std::ceil(
									 static_cast<double>(getWidth() + _font->getSpacing() - _lineWidth[line]) / 2.));
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
	///
	static inline void func(
			Uint8& dest,
			Uint8& src,
			int offset,
			int mult,
			int mid)
	{
		if (src != 0)
		{
			int inverseOffset;
			if (mid != 0)
				inverseOffset = (2 * (mid - static_cast<int>(src)));
			else
				inverseOffset = 0;

			dest = static_cast<Uint8>(offset + (static_cast<int>(src) * mult) + inverseOffset);
		}
	}
};

}


/**
 * Draws all the characters in the text with a really nasty complex gritty text
 * rendering algorithm logic stuff.
 */
void Text::draw()
{
	Surface::draw();

	if (_text.empty() == true
		|| _font == NULL)
	{
		return;
	}

	if (Options::debugUi == true) // show text borders for debugUI
	{
		SDL_Rect rect;
		rect.x =
		rect.y = 0;
		rect.w = static_cast<Uint16>(getWidth());
		rect.h = static_cast<Uint16>(getHeight());
		this->drawRect(&rect, 5);

		++rect.x;
		++rect.y;
		rect.w -= 2;
		rect.h -= 2;
		this->drawRect(&rect, 0);
	}

	int
		line = 0,
		x = getLineX(line),
		y = 0,
		height = 0,
		color = _color,
		dir = 1,
		mid = 0;

	std::wstring* wst = &_text;
	if (_wrap == true)
		wst = &_wrappedText;

	for (std::vector<int>::const_iterator
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
			y = static_cast<int>(std::ceil(
				static_cast<double>(getHeight() - height) / 2.));
		break;
		case ALIGN_BOTTOM:
			y = getHeight() - height;
	}

	if (_lang->getTextDirection() == DIRECTION_RTL) // set up text direction
		dir = -1;

	if (_invert == true) // invert text by inverting the font palette on index 3 (font palettes use indices 1..5)
		mid = 3;

	Font* font = _font;


	for (std::wstring::const_iterator // draw each letter one by one
			i = wst->begin();
			i != wst->end();
			++i)
	{
		if (Font::isSpace(*i) == true)
			x += dir * font->getCharSize(*i).w;
		else if (Font::isLinebreak(*i) == true)
		{
			++line;

			y += font->getCharSize(*i).h;
			x = getLineX(line);

			if (*i == L'\x02')
				font = _small;
		}
		else if (*i == L'\x01')
		{
			if (color == _color)
				color = _color2;
			else
				color = _color;
		}
		else
		{
/*			if (_contrast == true)
			{
				if		(color %16 < 2)	mult = 3;
				else if (color %16 < 6) mult = 2;
				else					mult = 1;
			} */
/*			int multer = 1;
			if (color == 176 // why does this go borky-borky <- PURPLE GeoscapePalette, seethrough pixels on highContrast=3
				&& _contrast != 1)
			{
				mult = 2;
			} */

			if (dir < 0)
				x += dir * font->getCharSize(*i).w;

			Surface* const srfChar = font->getChar(*i);
			srfChar->setX(x);
			srfChar->setY(y);
			ShaderDraw<PaletteShift>(
								ShaderSurface(this, 0,0),
								ShaderCrop(srfChar),
								ShaderScalar(color),
								ShaderScalar(_contrast), //multer),
								ShaderScalar(mid));

			if (dir > 0)
				x += dir * font->getCharSize(*i).w;
		}
	}
}

}
