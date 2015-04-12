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

#ifndef OPENXCOM_MANUFACTURESTARTSTATE_H
#define OPENXCOM_MANUFACTURESTARTSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen which displays needed elements to start productions;
 * items/ required workshop space/ cost to build a unit, etc.
 */
class ManufactureStartState
	:
		public State
{

private:
	Base* _base;
	const RuleManufacture* _manufRule;
	Text
		* _txtCost,
		* _txtItemNameColumn,
		* _txtManHour,
		* _txtRequiredItemsTitle,
		* _txtTitle,
		* _txtWorkSpace,
		* _txtUnitAvailableColumn,
		* _txtUnitRequiredColumn;
	TextButton
		* _btnCancel,
		* _btnStart;
	TextList* _lstRequiredItems;
	Window* _window;


	public:
		/// Creates the State.
		ManufactureStartState(
				Base* base,
				const RuleManufacture* const manufRule);
		/// Handler for the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for the start button.
		void btnStartClick(Action* action);
};

}

#endif
