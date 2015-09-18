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

#ifndef OPENXCOM_NEWMANUFACTURELISTSTATE_H
#define OPENXCOM_NEWMANUFACTURELISTSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleManufacture;

class ComboBox;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen that lists possible productions.
 */
class NewManufactureListState
	:
		public State
{

private:
	size_t _scroll;

	Base* _base;
	ComboBox* _cbxCategory;
	Text
		* _txtTitle,
		* _txtItem,
		* _txtCategory;
	TextButton
		* _btnCancel,
		* _btnCostTable;
	TextList* _lstManufacture;
	Window* _window;

	std::vector<RuleManufacture*> _possibleProductions;
	std::vector<std::string>
		_catStrings,
		_displayedStrings;


	public:
		/// Creates the state.
		explicit NewManufactureListState(Base* const base);
		/// dTor.
		~NewManufactureListState();

		/// Initializes state.
		void init();

		/// Fills the list of possible productions.
		void fillProductionList();

		/// Handler for the Costs button.
		void btnCostsClick(Action* action);
		/// Handler for clicking the OK button.
		void btnCancelClick(Action* action);
		/// Handler for clicking on the list.
		void lstProdClick(Action* action);
		/// Handler for changing the category filter.
		void cbxCategoryChange(Action* action);
};

}

#endif
