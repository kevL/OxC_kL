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

#ifndef OPENXCOM_ARTICLESTATE_H
#define OPENXCOM_ARTICLESTATE_H

//#include <string>

#include "../Engine/State.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

class Action;
class Surface;
class TextButton;
class Timer;


/**
 * UfopaediaArticle is the base class for all articles of various types.
 * @note It encapsulates the common characteristics.
 */

class ArticleState
	:
		public State
{

protected:
	static const Uint8
		uPed_ORANGE			= 16,
		uPed_VIOLET			= 83,
		uPed_PINK			= 162, // -> 192+ lt.brown for ExtraAlienInfo
		uPed_BLUE_SLATE		= 239,
		uPed_GREEN_SLATE	= 244,

		GEOSCAPE_CYAN		= 133,

		BASESCAPE_VIOLET	= 64,
		BASESCAPE_BLUE		= 218,
		BASESCAPE_WHITE		= 208;


	std::string _id;

	InteractiveSurface* _bg;
	TextButton
		* _btnNext,
		* _btnOk,
		* _btnPrev;
	Timer* _timer;

	/// constructor (can be instantiated by derived classes only)
	explicit ArticleState(const std::string& article_id);
	/// destructor
	virtual ~ArticleState();

	/// Advances to the next/previous Article when right/left key is depressed.
	void repeat();

	/// screen layout helpers
	void initLayout(bool contrast = true);

	/// callback for OK button
	void btnOkClick(Action* action);
	/// callback for PREV button
	void btnPrevClick(Action* action);
	/// callback for NEXT button
	void btnNextClick(Action* action);


	public:
		/// Runs the timer.
		void think();

		/// converts damage type to string
		static std::string getDamageTypeText(ItemDamageType dType);

		/// returns the article id
		std::string getId() const
		{ return _id; }
};

}

#endif
