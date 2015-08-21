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

#include "RuleInventory.h"

//#include <cmath>

#include "RuleItem.h"


namespace YAML
{

template<>
struct convert<OpenXcom::RuleSlot>
{
	///
	static Node encode(const OpenXcom::RuleSlot& rhs)
	{
		Node node;

		node.push_back(rhs.x);
		node.push_back(rhs.y);

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::RuleSlot& rhs)
	{
		if (node.IsSequence() == false
			|| node.size() != 2)
		{
			return false;
		}

		rhs.x = node[0].as<int>();
		rhs.y = node[1].as<int>();

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of inventory section.
 * @param id - string defining the id
 */
RuleInventory::RuleInventory(const std::string& id)
	:
		_id(id),
		_x(0),
		_y(0),
		_type(INV_SLOT),
		_listOrder(0)
{}

/**
 * dTor.
 */
RuleInventory::~RuleInventory()
{}

/**
 * Loads the inventory from a YAML file.
 * @param node - reference a YAML node
 * @param listOrder - the list weight for this inventory
 */
void RuleInventory::load(
		const YAML::Node &node,
		int listOrder)
{
	_id			= node["id"]		.as<std::string>(_id);
	_x			= node["x"]			.as<int>(_x);
	_y			= node["y"]			.as<int>(_y);
	_slots		= node["slots"]		.as<std::vector<RuleSlot> >(_slots);
	_costs		= node["costs"]		.as<std::map<std::string, int> >(_costs);
	_listOrder	= node["listOrder"]	.as<int>(listOrder);

	_type		= static_cast<InventoryType>(node["type"].as<int>(_type));
}

/**
 * Gets the language string that names this inventory section.
 * @note Each section has a unique name.
 * @return, section name
 */
std::string RuleInventory::getId() const
{
	return _id;
}

/**
 * Gets the X position of the inventory section on the screen.
 * @return, X position in pixels
 */
int RuleInventory::getX() const
{
	return _x;
}

/**
 * Gets the Y position of the inventory section on the screen.
 * @return, Y position in pixels
 */
int RuleInventory::getY() const
{
	return _y;
}

/**
 * Gets the type of the inventory section.
 * @note Slot-based contain a limited number of slots. Hands only contain one
 * slot but can hold any item. Ground can hold infinite items but it doesn't
 * attach to soldiers.
 * @return, InventoryType (RuleInventory.h)
 */
InventoryType RuleInventory::getType() const
{
	return _type;
}

/**
 * Gets all the slots in the inventory section.
 * @return, pointer to a vector of RuleSlot-structs
 */
std::vector<struct RuleSlot>* RuleInventory::getSlots()
{
	return &_slots;
}

/**
 * Gets the slot located in the specified mouse position.
 * @param x - mouse X position; returns the slot's X position
 * @param y - mouse Y position; returns the slot's Y position
 * @return, true if there's a slot here
 */
bool RuleInventory::checkSlotInPosition(
		int* x,
		int* y) const
{
	const int
		mouseX = *x,
		mouseY = *y;

	switch (_type)
	{
		case INV_HAND:
			for (int
					xx = 0;
					xx < HAND_W;
					++xx)
			{
				for (int
						yy = 0;
						yy < HAND_H;
						++yy)
				{
					if (   mouseX >= _x + xx * SLOT_W
						&& mouseX < _x + (xx + 1) * SLOT_W
						&& mouseY >= _y + yy * SLOT_H
						&& mouseY < _y + (yy + 1) * SLOT_H)
					{
						*x =
						*y = 0;

						return true;
					}
				}
			}
		break;

		case INV_GROUND:
			if (   mouseX >= _x
				&& mouseX < 320
				&& mouseY >= _y
				&& mouseY < 200)
			{
				*x = static_cast<int>(std::floor(
					 static_cast<double>(mouseX - _x) / static_cast<double>(SLOT_W)));
				*y = static_cast<int>(std::floor(
					 static_cast<double>(mouseY - _y) / static_cast<double>(SLOT_H)));

				return true;
			}
		break;

		default:
			for (std::vector<RuleSlot>::const_iterator
					i = _slots.begin();
					i != _slots.end();
					++i)
			{
				if (   mouseX >= _x + i->x * SLOT_W
					&& mouseX < _x + (i->x + 1) * SLOT_W
					&& mouseY >= _y + i->y * SLOT_H
					&& mouseY < _y + (i->y + 1) * SLOT_H)
				{
					*x = i->x;
					*y = i->y;

					return true;
				}
			}
	}

	return false;
}

/**
 * Checks if an item completely fits in a certain slot.
 * @param item	- pointer to item ruleset
 * @param x		- slot X position
 * @param y		- slot Y position
 * @return, true if there's room there
 */
bool RuleInventory::fitItemInSlot(
		RuleItem* item,
		int x,
		int y) const
{
	switch (_type)
	{
		case INV_GROUND:
		{
			const int
				width = (320 - _x) / SLOT_W,
				height = (200 - _y) / SLOT_H;
			int xOffset = 0;

			while (x >= xOffset + width)
				xOffset += width;

			for (int
					find_x = x;
					find_x != x + item->getInventoryWidth();
					++find_x)
			{
				for (int
						find_y = y;
						find_y != y + item->getInventoryHeight();
						++find_y)
				{
					if (!
							(  find_x >= xOffset
							&& find_x < xOffset + width
							&& find_y > -1
							&& find_y < height))
					{
						return false;
					}
				}
			}
		}
		case INV_HAND:
		return true;

		default:
		{
			const int slotsTotal = item->getInventoryWidth() * item->getInventoryHeight();
			int slotsFound = 0;

			for (std::vector<RuleSlot>::const_iterator
					i = _slots.begin();
					i != _slots.end()
						&& slotsFound < slotsTotal;
					++i)
			{
				if (   i->x >= x
					&& i->x < x + item->getInventoryWidth()
					&& i->y >= y
					&& i->y < y + item->getInventoryHeight())
				{
					++slotsFound;
				}
			}

			return (slotsFound == slotsTotal);
		}
	}
}

/**
 * Gets the time unit cost to place an item in this slot to another.
 * @param slot - the new slot id
 * @return, tu cost
 */
int RuleInventory::getCost(const RuleInventory* const slot) const
{
	if (slot == this)
		return 0;

	return _costs.find(slot->getId())->second;
}

/**
 * Gets the list order.
 */
int RuleInventory::getListOrder() const
{
	return _listOrder;
}

}
