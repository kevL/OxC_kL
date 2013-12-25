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

#ifndef OPENXCOM_TRANSFERITEMSSTATE_H
#define OPENXCOM_TRANSFERITEMSSTATE_H

#include <string>
#include <vector>

#include "../Engine/State.h"
#include "../Savegame/Transfer.h"


namespace OpenXcom
{

class Base;
class Craft;
class Soldier;
class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Transfer screen that lets the player pick
 * what items to transfer between bases.
 */
class TransferItemsState
	:
		public State
{

private:

	bool
		_allowChangeListValuesByMouseWheel,
		_canTransferCraftsWhileAirborne,
		_containmentLimit;
	int
		_aQty,
		_cQty,
		_pQty,
		_total,
		_changeValueByMouseWheel,
		_hasEng,
		_hasSci;
	unsigned int _sel;
	float _iQty;
	double _distance;

	Base
		* _baseFrom,
		* _baseTo;
	Text
		* _txtTitle,
		* _txtItem,
		* _txtQuantity,
		* _txtAmountTransfer,
		* _txtAmountDestination;
	TextButton
		* _btnCancel,
		* _btnOk;
	TextList* _lstItems;
	Timer
		* _timerInc,
		* _timerDec;
	Window* _window;

	std::vector<int>
		_baseQty,
		_destQty,
		_transferQty;
	std::vector<std::string> _items;
	std::vector<Craft*> _crafts;
	std::vector<Soldier*> _soldiers;

	/// Gets selected cost.
	int getCost() const;
	/// Gets selected quantity.
	int getQuantity() const;
	/// Gets distance between bases.
	double getDistance() const;
	/// Gets type of selected item.
	enum TransferType getType(unsigned selected) const;
	/// Gets item Index.
	int getItemIndex(unsigned selected) const;


	public:
		/// Creates the Transfer Items state.
		TransferItemsState(
				Game* game,
				Base* baseFrom,
				Base* baseTo);
		/// Cleans up the Transfer Items state.
		~TransferItemsState();

		/// Updates the palette.
		void init();
		/// Re-initialize the Transfer menu, when cancelling TransferConfirmState.
		void reinit();

		/// Runs the timers.
		void think();

		/// Completes the transfer between bases.
		void completeTransfer();

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
		void lstItemsRightArrowPress(Action* action);
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

		/// Gets the total of the transfer.
		int getTotal() const;
};

}

#endif
