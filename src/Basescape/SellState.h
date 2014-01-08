/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_SELLSTATE_H
#define OPENXCOM_SELLSTATE_H

#include <string>
#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

enum SellType
{
	SELL_SOLDIER,	// 0
	SELL_CRAFT,		// 1
	SELL_ITEM,		// 2
	SELL_SCIENTIST,	// 3
	SELL_ENGINEER	// 4
};


class Base;
class Craft;
class Soldier;
class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Sell/Sack screen that lets the player sell
 * any items in a particular base.
 */
class SellState
	:
		public State
{

private:
	bool _allowChangeListValuesByMouseWheel;
	int
		_changeValueByMouseWheel,
		_hasSci,
		_hasEng,
		_total;
	unsigned int _sel;

	Base* _base;
	Text
		* _txtFunds,
		* _txtBaseLabel,
		* _txtItem,
		* _txtQuantity,
		* _txtSales,
		* _txtSell,
		* _txtTitle,
		* _txtValue;
	TextButton
		* _btnOk,
		* _btnCancel;
	TextList* _lstItems;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	std::vector<int> _qtys;
	std::vector<std::string> _items;
	std::vector<Craft*> _crafts;
	std::vector<Soldier*> _soldiers;

	/// Gets selected price.
	int getPrice();
	/// Gets selected quantity.
	int getQuantity();
	/// Gets the Type of the selected item.
	enum SellType getType(unsigned selected) const;
	/// Gets the index of selected item.
	int getItemIndex(unsigned selected) const;
	/// Gets the index of the selected craft.
	int getCraftIndex(unsigned selected) const;


	public:
		/// Creates the Sell state.
		SellState(
				Game* game,
				Base* base);
		/// Cleans up the Sell state.
		~SellState();

		/// Runs the timers.
		void think();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for pressing an Increase arrow in the list.
		void lstItemsLeftArrowPress(Action* action);
		/// Handler for releasing an Increase arrow in the list.
		void lstItemsLeftArrowRelease(Action* action);
		/// Handler for clicking an Increase arrow in the list.
		void lstItemsLeftArrowClick(Action* action);
		/// Handler for pressing a Decrease arrow in the list.
		void lstItemsRightArrowPress(Action *action);
		/// Handler for releasing a Decrease arrow in the list.
		void lstItemsRightArrowRelease(Action* action);
		/// Handler for clicking a Decrease arrow in the list.
		void lstItemsRightArrowClick(Action* action);
		/// Handler for pressing-down a mouse-button in the list.
		void lstItemsMousePress(Action* action);

		/// Increases the quantity of an item by one.
		void increase();
		/// Increases the quantity of an item by the given value.
		void increaseByValue(int change);
		/// Decreases the quantity of an item by one.
		void decrease();
		/// Decreases the quantity of an item by the given value.
		void decreaseByValue(int change);

		/// Updates the quantity-strings of the selected item.
		void updateItemStrings();
};

}

#endif
