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

#include "BattleItem.h"

#include "BattleUnit.h"
#include "Tile.h"

#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Initializes an item of the specified type.
 * @param itRule	- pointer to RuleItem
 * @param pId		- pointer to an integer ID for this item
 * @param id		- value for ID when loading a saved game (default -1)
 */
BattleItem::BattleItem(
		RuleItem* const itRule,
		int* pId,
		int id)
	:
		_itRule(itRule),
		_owner(NULL),
		_previousOwner(NULL),
		_unit(NULL),
		_tile(NULL),
		_inventorySlot(NULL),
		_inventoryX(0),
		_inventoryY(0),
		_ammoItem(NULL),
		_ammoQty(itRule->getClipSize()),
		_fuse(-1),
		_painKiller(0),
		_heal(0),
		_stimulant(0),
		_XCOMProperty(false),
		_isLoad(false)
{
	if (pId != NULL)	// <- this is for SavedBattleGame only to keep
	{					// track of brand new item on the battlefield
		_id = *pId;
		++(*pId);
	}
	else				// load item from saved game
		_id = id;


	if (_itRule->getBattleType() == BT_MEDIKIT)
	{
		_heal = _itRule->getHealQuantity();
		_painKiller = _itRule->getPainKillerQuantity();
		_stimulant = _itRule->getStimulantQuantity();
	}
	else if (_itRule->getCompatibleAmmo()->empty() == true
		&& (_itRule->getBattleType() == BT_FIREARM // These weapons do not need ammo; '_ammoItem' points to the weapon itself.
			|| _itRule->getBattleType() == BT_MELEE))
	{
		// lasers, melee, etc have "clipsize -1"
//		setAmmoQuantity(-1);	// needed for melee-item reaction hits, etc. (can be set in Ruleset but do it here)
								// Except that it creates problems w/ TANKS returning to Base. So do it in Ruleset:
								// melee items need "clipSize -1" to do reactionFire.
		_ammoItem = this;
	}
}

/**
 * dTor.
 */
BattleItem::~BattleItem()
{}

/**
 * Loads the item from a YAML file.
 * @param node - YAML node
 */
void BattleItem::load(const YAML::Node& node)
{
	_inventoryX	= node["inventoryX"].as<int>(_inventoryX);
	_inventoryY	= node["inventoryY"].as<int>(_inventoryY);
	_ammoQty	= node["ammoQty"]	.as<int>(_ammoQty);
	_painKiller	= node["painKiller"].as<int>(_painKiller);
	_heal		= node["heal"]		.as<int>(_heal);
	_stimulant	= node["stimulant"]	.as<int>(_stimulant);
	_fuse		= node["fuse"]		.as<int>(_fuse);
}

/**
 * Saves the item to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleItem::save() const
{
	YAML::Node node;

	node["id"]			= _id;
	node["type"]		= _itRule->getType();
	node["inventoryX"]	= _inventoryX;
	node["inventoryY"]	= _inventoryY;

	if (_ammoQty != 0)
		node["ammoQty"]		= _ammoQty;
	if (_painKiller != 0)
		node["painKiller"]	= _painKiller;
	if (_heal != 0)
		node["heal"]		= _heal;
	if (_stimulant != 0)
		node["stimulant"]	= _stimulant;

	if (_owner != NULL)			node["owner"]			= _owner->getId();
	else						node["owner"]			= -1;
	if (_previousOwner != NULL)	node["previousOwner"]	= _previousOwner->getId();
	if (_unit != NULL)			node["unit"]			= _unit->getId();
	else						node["unit"]			= -1;
	if (_inventorySlot != NULL)	node["inventoryslot"]	= _inventorySlot->getId();
	else						node["inventoryslot"]	= "NULL";
	if (_tile != NULL)			node["position"]		= _tile->getPosition();
	else						node["position"]		= Position(-1,-1,-1);
	if (_ammoItem != NULL)		node["ammoItem"]		= _ammoItem->getId();
	else						node["ammoItem"]		= -1;
	if (_fuse != -1)			node["fuse"]			= _fuse;

	return node;
}

/**
 * Gets the ruleset for the item's type.
 * @return, pointer to ruleset
 */
RuleItem* BattleItem::getRules() const
{
	return _itRule;
}

/**
 * Gets the turns until detonation. -1 = unprimed grenade
 * @return, turns until detonation
 */
int BattleItem::getFuse() const
{
	return _fuse;
}

/**
 * Sets the turns until detonation.
 * @param turns - turns until detonation (player/alien turns not full turns)
 */
void BattleItem::setFuse(int turn)
{
	_fuse = turn;
}

/**
 * Gets the '_ammoQty' of this BattleItem.
 * @return, ammo quantity
 *			0		- item is not ammo
 *			INT_MAX	- item is its own ammo
 */
int BattleItem::getAmmoQuantity() const
{
	if (_ammoQty == -1)
		return std::numeric_limits<int>::max();

	return _ammoQty;
}

/**
 * Sets the '_ammoQty' of this BattleItem.
 * @param qty - ammo quantity
 */
void BattleItem::setAmmoQuantity(int qty)
{
	_ammoQty = qty;
}

/**
 * Gets if the item is loaded in a weapon.
 * @return, true if loaded
 */
bool BattleItem::getIsLoadedAmmo() const
{
	return _isLoad;
}

/**
 * Sets if the item is loaded in a weapon.
 * @param - true if loaded (default true)
 */
void BattleItem::setIsLoadedAmmo(bool loaded)
{
	_isLoad = loaded;
}

/**
 * Gets an item's currently loaded ammo item.
 * @return, pointer to BattleItem
 *			- NULL if this BattleItem is ammo or this BattleItem has no ammo loaded
 *			- the weapon itself if weapon is its own ammo
 */
BattleItem* BattleItem::getAmmoItem() const
{
	return _ammoItem;
}

/**
 * Sets an ammo item for this BattleItem.
 * @param item - the ammo item (default NULL)
 * @return,	 0 = successful load or unload
 *			-1 = weapon already contains ammo
 *			-2 = item doesn't fit / weapon is self-powered
 */
int BattleItem::setAmmoItem(BattleItem* const item)
{
	if (_ammoItem != this)
	{
		if (item == NULL) // unload weapon
		{
			if (_ammoItem != NULL)
				_ammoItem->setIsLoadedAmmo(false);

			_ammoItem = NULL;
			return 0;
		}

		if (_ammoItem != NULL)
			return -1;

		for (std::vector<std::string>::const_iterator
				i = _itRule->getCompatibleAmmo()->begin();
				i != _itRule->getCompatibleAmmo()->end();
				++i)
		{
			if (*i == item->getRules()->getType())
			{
				item->setIsLoadedAmmo();
				_ammoItem = item;

				return 0;
			}
		}
	}

	return -2;
}

/**
 * Determines if this BattleItem uses ammo OR is self-powered.
 * @note No ammo is needed if the item has itself assigned as its '_ammoItem'.
 * @return, true if self-powered
 */
bool BattleItem::selfPowered() const
{
	return (_ammoItem == this);
}

/**
 * Spends a bullet from this BattleItem.
 * @param battleSave	- reference to the SavedBattleGame
 * @param weapon		- reference to the weapon containing this ammo
 */
void BattleItem::spendBullet(
		SavedBattleGame& battleSave,
		BattleItem& weapon)
{
	if (_ammoQty != -1 // <- infinite ammo
		&& --_ammoQty == 0)
	{
		weapon.setAmmoItem();
		battleSave.removeItem(this); // <- could be dangerous.
	}
}
/*	if (_ammoQty == -1)		// the ammo should have gotten deleted if/when it reaches 0;
		return true;		// less than 0 denotes self-powered weapons. But ...
							// let ==0 be a fudge-factor.
	if (--_ammoQty == 0)	// TODO: See about removing the '_ammoItem' from the game here
		return false;		// so there is *never* a clip w/ 0 qty IG. */
/*	if (_ammoQty != -1
		&& --_ammoQty == 0)
	{
		return false;
	}
	return true; */

/**
 * Gets the item's owner.
 * @return, pointer to BattleUnit
 */
BattleUnit* BattleItem::getOwner() const
{
	return _owner;
}

/**
 * Gets the item's previous owner.
 * @return, pointer to BattleUnit
 */
BattleUnit* BattleItem::getPreviousOwner() const
{
	return _previousOwner;
}

/**
 * Sets the item's owner.
 * @param owner - pointer to BattleUnit (default NULL)
 */
void BattleItem::setOwner(BattleUnit* const owner)
{
	_previousOwner = _owner;
	_owner = owner;
}

/**
 * Sets the item's previous owner.
 * @param owner - pointer to a Battleunit
 */
void BattleItem::setPreviousOwner(BattleUnit* const owner)
{
	_previousOwner = owner;
}

/**
 * Removes the item from the previous owner and moves it to a new owner.
 * @param owner - pointer to BattleUnit (default NULL)
 */
void BattleItem::moveToOwner(BattleUnit* const owner)
{
	if (_owner != NULL)
		_previousOwner = _owner;
	else
		_previousOwner = owner;

	_owner = owner;

	if (_previousOwner != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _previousOwner->getInventory()->begin();
				i != _previousOwner->getInventory()->end();
				++i)
		{
			if (*i == this)
			{
				_previousOwner->getInventory()->erase(i);
				break;
			}
		}
	}

	if (_owner != NULL)
		_owner->getInventory()->push_back(this);
}

/**
 * Gets the item's inventory slot.
 * @return, the slot id
 */
RuleInventory* BattleItem::getSlot() const
{
	return _inventorySlot;
}

/**
 * Sets the item's inventory slot.
 * @param slot - the slot id
 */
void BattleItem::setSlot(RuleInventory* const slot)
{
	_inventorySlot = slot;
}

/**
 * Gets the item's inventory X position.
 * @return, X position
 */
int BattleItem::getSlotX() const
{
	return _inventoryX;
}

/**
 * Sets the item's inventory X position.
 * @param x - X position
 */
void BattleItem::setSlotX(int x)
{
	_inventoryX = x;
}

/**
 * Gets the item's inventory Y position.
 * @return, Y position
 */
int BattleItem::getSlotY() const
{
	return _inventoryY;
}

/**
 * Sets the item's inventory Y position.
 * @param y - Y position
 */
void BattleItem::setSlotY(int y)
{
	_inventoryY = y;
}

/**
 * Checks if the item is covering certain inventory slot(s).
 * @param x		- slot X position
 * @param y 	- slot Y position
 * @param item	- pointer to an item to check for overlap or default NULL if none
 * @return, true if it is covering
 */
bool BattleItem::occupiesSlot(
		int x,
		int y,
		const BattleItem* const item) const
{
	if (item == this)
		return false;

	if (_inventorySlot->getType() == INV_HAND)
		return true;

	if (item == NULL)
		return (   x >= _inventoryX
				&& x < _inventoryX + _itRule->getInventoryWidth()
				&& y >= _inventoryY
				&& y < _inventoryY + _itRule->getInventoryHeight());
	else
		return !(
				   x >= _inventoryX + _itRule->getInventoryWidth()
				|| x + item->getRules()->getInventoryWidth() <= _inventoryX
				|| y >= _inventoryY + _itRule->getInventoryHeight()
				|| y + item->getRules()->getInventoryHeight() <= _inventoryY);
}

/**
 * Gets the item's tile.
 * @return, Pointer to the Tile
 */
Tile* BattleItem::getTile() const
{
	return _tile;
}

/**
 * Sets the item's tile.
 * @param tile - pointer to a Tile
 */
void BattleItem::setTile(Tile* tile)
{
	_tile = tile;
}

/**
 * Gets the item's id.
 * @return, the item's id
 */
int BattleItem::getId() const
{
	return _id;
}

/**
 * Gets a corpse's unit.
 * @return, pointer to BattleUnit
 */
BattleUnit* BattleItem::getUnit() const
{
	return _unit;
}

/**
 * Sets the corpse's unit.
 * @param unit - pointer to BattleUnit
 */
void BattleItem::setUnit(BattleUnit* unit)
{
	_unit = unit;
}

/**
 * Sets the heal quantity of the item.
 * @param heal - the new heal quantity
 */
void BattleItem::setHealQuantity(int heal)
{
	_heal = heal;
}

/**
 * Gets the heal quantity of the item.
 * @return, the new heal quantity
 */
int BattleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Sets the pain killer quantity of the item.
 * @param pk - the new pain killer quantity
 */
void BattleItem::setPainKillerQuantity(int pk)
{
	_painKiller = pk;
}

/**
 * Gets the pain killer quantity of the item.
 * @return, the new pain killer quantity
 */
int BattleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Sets the stimulant quantity of the item.
 * @param stimulant - the new stimulant quantity
 */
void BattleItem::setStimulantQuantity(int stimulant)
{
	_stimulant = stimulant;
}

/**
 * Gets the stimulant quantity of the item.
 * @return, the new stimulant quantity
 */
int BattleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Sets the XCom property flag.
 * @note This is to determine at Debriefing what goes back into the base/craft.
 * kL_note: no, actually it's not: In its current implementation it is
 * fundamentally useless ... well it may bear only on artefacts for a Base
 * Defense mission ....
 * @param flag - true if xCom property
 */
void BattleItem::setXCOMProperty(bool flag)
{
	_XCOMProperty = flag;
}

/**
 * Gets the XCom property flag.
 * @note This is to determine at Debriefing what goes back into the base/craft.
 * kL_note: no, actually it's not: In its current implementation it is
 * fundamentally useless.
 * @return, true if xCom property
 */
bool BattleItem::getXCOMProperty() const
{
	return _XCOMProperty;
}

/**
 * Converts a carried unconscious body into a battlefield corpse-item.
 * @param itRule - pointer to rules of the corpse item to convert this item into
 */
void BattleItem::convertToCorpse(RuleItem* const itRule)
{
	if (_unit != NULL
		&& _itRule->getBattleType() == BT_CORPSE
		&& itRule->getBattleType() == BT_CORPSE)
	{
		_itRule = itRule;
	}
}

}
