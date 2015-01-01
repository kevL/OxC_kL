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

#ifndef OPENXCOM_RULEINVENTORY_H
#define OPENXCOM_RULEINVENTORY_H

//#include <map>
//#include <string>
//#include <vector>

//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

struct RuleSlot
{
	int
		x,
		y;
};


enum InventoryType
{
	INV_SLOT,	// 0
	INV_HAND,	// 1
	INV_GROUND	// 2
};


class RuleItem;


/**
 * Represents a specific section of the inventory, containing
 * information like available slots and screen position.
 */
class RuleInventory
{

private:
	int
		_listOrder,
		_x,
		_y;
	std::string _id;

	InventoryType _type;

	std::map<std::string, int> _costs;
	std::vector<RuleSlot> _slots;


	public:
		static const int
			SLOT_W = 16,
			SLOT_H = 16,
			HAND_W = 2,
			HAND_H = 3;

		/// Creates a blank inventory ruleset.
		RuleInventory(const std::string& id);
		/// Cleans up the inventory ruleset.
		~RuleInventory();

		/// Loads inventory data from YAML.
		void load(
				const YAML::Node& node,
				int listOrder);

		/// Gets the inventory's id.
		std::string getId() const;

		/// Gets the X position of the inventory.
		int getX() const;
		/// Gets the Y position of the inventory.
		int getY() const;

		/// Gets the inventory type.
		InventoryType getType() const;

		/// Gets all the slots in the inventory.
		std::vector<struct RuleSlot>* getSlots();
		/// Checks for a slot in a certain position.
		bool checkSlotInPosition(
				int* x,
				int* y) const;
		/// Checks if an item fits in a slot.
		bool fitItemInSlot(
				RuleItem* item,
				int x,
				int y) const;

		/// Gets a certain cost in the inventory.
		int getCost(const RuleInventory* const slot) const;

		/// Gets the list order.
		int getListOrder() const;
};

}

#endif
