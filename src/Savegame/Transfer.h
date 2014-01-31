/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#ifndef OPENXCOM_TRANSFER_H
#define OPENXCOM_TRANSFER_H

#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum TransferType
{
	TRANSFER_SOLDIER,	// 0
	TRANSFER_CRAFT,		// 1
	TRANSFER_ITEM,		// 2
	TRANSFER_SCIENTIST,	// 3
	TRANSFER_ENGINEER	// 4
};


class Base;
class Craft;
class Language;
class Ruleset;
class Soldier;


/**
 * Represents an item transfer.
 * Items are placed "in transit" whenever they are
 * purchased or transferred between bases.
 */
class Transfer
{

private:
	bool _delivered;
	int
		_engineers,
		_hours,
		_itemQty,
		_scientists;

	std::string _itemId;

	Craft *_craft;
	Soldier *_soldier;


	public:
		/// Creates a new transfer.
		Transfer(int hours);
		/// Cleans up the transfer.
		~Transfer();

		/// Loads the transfer from YAML.
		void load(
				const YAML::Node& node,
				Base* base,
				const Ruleset* rule);
		/// Saves the transfer to YAML.
		YAML::Node save() const;

		/// Sets the scientists of the transfer.
		void setScientists(int scientists);
		/// Sets the engineers of the transfer.
		void setEngineers(int engineers);
		/// Sets the soldier of the transfer.
		void setSoldier(Soldier* soldier);

		/// Sets the craft of the transfer.
		void setCraft(Craft* craft);
		/// Gets the craft of the transfer.
		Craft* getCraft();

		/// Gets the items of the transfer.
		std::string getItems() const;
		/// Sets the items of the transfer.
		void setItems(
				const std::string& id,
				int qty = 1);

		/// Gets the name of the transfer.
		std::wstring getName(Language* lang) const;

		/// Gets the hours remaining of the transfer.
		int getHours() const;
		/// Gets the quantity of the transfer.
		int getQuantity() const;
		/// Gets the type of the transfer.
		TransferType getType() const;

		/// Advances the transfer.
		void advance(Base* base);
};

}

#endif
