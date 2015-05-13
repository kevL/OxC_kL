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

#ifndef OPENXCOM_CRAFTSSTATE_H
#define OPENXCOM_CRAFTSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Equip Craft screen that lets the player manage all the crafts in a base.
 */
class CraftsState
	:
		public State
{

private:
	static const Uint8
		GREEN		=  48,
		LAVENDER	=  64,
		YELLOW		=  96,
		BROWN		= 112,
		BLUE		= 128;

	Uint8 _cellColor;

	Base* _base;
	Text
		* _txtBase,
		* _txtName,
		* _txtStatus,
		* _txtTitle,
		* _txtWeapons;
	TextButton* _btnOk;
	TextList* _lstCrafts;
	Window* _window;

	/// A more descriptive status of a Craft.
	std::wstring getAltStatus(Craft* const craft);

	/// Formats a duration in hours into a day & hour string.
	std::wstring formatTime(
			const int total,
			const bool delayed);


	public:
		/// Creates the Crafts state.
		explicit CraftsState(Base* base);
		/// Cleans up the Crafts state.
		~CraftsState();

		/// Updates the craft info.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Crafts list.
		void lstCraftsPress(Action* action);

		/// Handler for clicking the Crafts reordering button.
		void lstLeftArrowClick(Action* action);
		/// Handler for clicking the Crafts reordering button.
		void lstRightArrowClick(Action* action);
};

}

#endif
