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

#ifndef OPENXCOM_INVENTORY_H
#define OPENXCOM_INVENTORY_H

//#include <map>
//#include <string>

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class Game;
class NumberText;
class RuleInventory;
class Timer;
class WarningMessage;


/**
 * Interactive view of an inventory.
 * @note Lets the player view and manage a soldier's equipment.
 */
class Inventory
	:
		public InteractiveSurface
{

private:
	bool
		_atBase,
		_tuMode;
	int
		_fuseFrame,
//		_depth,
		_groundOffset,
		_prime,
		_tuCost;

	BattleItem
		* _mouseOverItem,
		* _selItem;
	BattleUnit* _selUnit;
	Game* _game;
	NumberText* _stackNumber;
	Surface
		* _grid,
		* _items,
		* _selection;
	Timer* _animTimer;
	WarningMessage* _warning;

	std::map<int, std::map<int, int> > _stackLevel;
	std::vector<std::pair<int, int> > _grenadeFuses;


	/// Moves an item to a specified slot.
	void moveItem(
			BattleItem* const item,
			RuleInventory* const slot,
			int x = 0,
			int y = 0);

	/// Gets the slot in the specified position.
	RuleInventory* getSlotInPosition(
			int* x,
			int* y) const;


	public:
		/// Creates a new inventory view at the specified position and size.
		Inventory(
				Game* game,
				int width,
				int height,
				int x = 0,
				int y = 0,
				bool atBase = false);
		/// Cleans up the inventory.
		~Inventory();

		/// Sets the inventory's palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256);

		/// Sets the inventory's Time Unit mode.
		void setTuMode(bool tu);

		/// Sets the inventory's selected unit.
		void setSelectedUnit(BattleUnit* unit);

		/// Draws the inventory.
		void draw();
		/// Draws the inventory grid.
		void drawGrid();
		/// Draws the inventory items.
		void drawItems();

		/// Checks for item overlap.
		static bool overlapItems(
				BattleUnit* unit,
				BattleItem* item,
				RuleInventory* slot,
				int x = 0,
				int y = 0);

		/// Gets the currently selected item.
		BattleItem* getSelectedItem() const;
		/// Sets the currently selected item.
		void setSelectedItem(BattleItem* const item);

		/// Gets the mouse over item.
		BattleItem* getMouseOverItem() const;
		/// Sets the mouse over item.
		void setMouseOverItem(BattleItem* const item);

		/// Handles timers.
		void think();

		/// Blits the inventory onto another surface.
		void blit(Surface* surface);

		/// Special handling for mouse hovers.
		void mouseOver(Action* action, State* state);
		/// Special handling for mouse clicks.
		void mouseClick(Action* action, State* state);

		/// Unloads the selected weapon.
		bool unload();

		/// Arranges items on the ground.
		void arrangeGround(bool alterOffset = true);
		/// Attempts to place an item in an inventory slot.
		bool fitItem(
				RuleInventory* slot,
				BattleItem* item,
				bool test = false);
		/// Checks if two items can be stacked on one another.
		bool canBeStacked(
				const BattleItem* const itemA,
				const BattleItem* const itemB);

		/// Shows a warning message.
		void showWarning(const std::wstring& msg);
		/// Shows priming warnings on grenades.
		void drawPrimers();
		/// Sets grenade to show a warning in Inventory.
		void setPrimeGrenade(int turn);

		/// Gets the TU cost for moving items around.
		int getTUCostInventory() const;
};

}

#endif
