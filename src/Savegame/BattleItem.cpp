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


namespace OpenXcom
{

/**
 * Initializes a item of the specified type.
 * @param rules	- pointer to RuleItem
 * @param id	- the IG id of this item
 */
BattleItem::BattleItem(
		RuleItem* const rules,
		int* id)
	:
		_rules(rules),
		_id(*id),
		_owner(NULL),
		_previousOwner(NULL),
		_unit(NULL),
		_tile(NULL),
		_inventorySlot(NULL),
		_inventoryX(0),
		_inventoryY(0),
		_ammoItem(NULL),
		_fuseTimer(-1),
		_ammoQty(0),
		_painKiller(0),
		_heal(0),
		_stimulant(0),
		_XCOMProperty(false)
//		_droppedOnAlienTurn(false)
{
	if (_rules != NULL)
	{
		if (_rules->getBattleType() == BT_AMMO)
			setAmmoQuantity(_rules->getClipSize());
		else if (_rules->getBattleType() == BT_MEDIKIT)
		{
			setHealQuantity(_rules->getHealQuantity());
			setPainKillerQuantity(_rules->getPainKillerQuantity());
			setStimulantQuantity(_rules->getStimulantQuantity());
		}
		else if (_rules->getCompatibleAmmo()->empty() == true
			&& (_rules->getBattleType() == BT_FIREARM		// These weapons do not need ammo;
				|| _rules->getBattleType() == BT_MELEE))	// (ammo)item points to weapon itself.
		{
			setAmmoQuantity(_rules->getClipSize()); // melee, lasers, etc have clipsize(-1).
//			setAmmoQuantity(-1);	// needed for melee-item reaction hits, etc. (can be set in Ruleset but do it here)
									// But it creates problems w/ TANKS returning to Base. So do it in Ruleset:
									// melee items need "clipSize: -1" to do reactionFire.
			_ammoItem = this;
		}
	}

	(*id)++;
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
	_fuseTimer	= node["fuseTimer"]	.as<int>(_fuseTimer);

//	_droppedOnAlienTurn	= node["droppedOnAlienTurn"].as<bool>(_droppedOnAlienTurn);
}

/**
 * Saves the item to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleItem::save() const
{
	YAML::Node node;

	node["id"]					= _id;
	node["type"]				= _rules->getType();

	if (_owner)
		node["owner"]			= _owner->getId();
	else
		node["owner"]			= -1;

	if (_previousOwner)
		node["previousOwner"]	= _previousOwner->getId();

	if (_unit)
		node["unit"]			= _unit->getId();
	else
		node["unit"]			= -1;

	if (_inventorySlot)
		node["inventoryslot"]	= _inventorySlot->getId();
	else
		node["inventoryslot"]	= "NULL";

	node["inventoryX"]			= _inventoryX;
	node["inventoryY"]			= _inventoryY;

	if (_tile)
		node["position"]		= _tile->getPosition();
	else
		node["position"]		= Position(-1,-1,-1);

	node["ammoQty"]				= _ammoQty;

	if (_ammoItem)
		node["ammoItem"]		= _ammoItem->getId();
	else
		node["ammoItem"]		= -1;

	node["painKiller"]			= _painKiller;
	node["heal"]				= _heal;
	node["stimulant"]			= _stimulant;
	node["fuseTimer"]			= _fuseTimer;

//	if (_droppedOnAlienTurn)
//		node["droppedOnAlienTurn"]	= _droppedOnAlienTurn;

	return node;
}

/**
 * Gets the ruleset for the item's type.
 * @return, pointer to ruleset
 */
RuleItem* BattleItem::getRules() const
{
	return _rules;
}

/**
 * Gets the turns until detonation. -1 = unprimed grenade
 * @return, turns until detonation
 */
int BattleItem::getFuseTimer() const
{
	return _fuseTimer;
}

/**
 * Sets the turns until detonation.
 * @param turns - turns until detonation (player/alien turns, not game turns)
 */
void BattleItem::setFuseTimer(int turn)
{
	_fuseTimer = turn;
}

/**
 * Gets the quantity of ammo in this ammo-item.
 * @return, ammo quantity
 *			0	- item is not ammo
 *			255	- item is its own ammo
 */
int BattleItem::getAmmoQuantity() const
{
	if (_rules->getClipSize() == -1) // is Laser, melee, etc. This could be taken out
//		|| _ammoQty == -1)	// kL, NOTE: specifying clipSize(-1) in Ruleset should no longer be necessary. BLEH !
							// This should iron out some ( rare ) reaction & AI problems ....
							// But it creates problems w/ TANKS returning to Base.
	{
		return 255;
	}

	return _ammoQty;
}

/**
 * Changes the quantity of ammo in this item.
 * @param qty - ammo quantity
 */
void BattleItem::setAmmoQuantity(int qty)
{
	_ammoQty = qty;
}

/**
 * Spends a bullet from the ammo in this item.
 * @return, true if there are bullets left
 */
bool BattleItem::spendBullet()
{
	if (_ammoQty < 0)	// the ammo should have gotten deleted if/when it reaches 0;
		return true;	// less than 0 denotes self-powered weapons. But ...
						// let ==0 be a fudge-factor.

	--_ammoQty;

	if (_ammoQty == 0)
		return false;

	return true;
}

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
 * @param owner - pointer to BattleUnit
 */
void BattleItem::setOwner(BattleUnit* owner)
{
	_previousOwner = _owner;
	_owner = owner;
}

/**
 * Sets the item's previous owner.
 * @param owner - pointer to a Battleunit
 */
void BattleItem::setPreviousOwner(BattleUnit* owner)
{
	_previousOwner = owner;
}

/**
 * Removes the item from the previous owner and moves it to the new owner.
 * @param owner - pointer to BattleUnit
 */
void BattleItem::moveToOwner(BattleUnit* owner)
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
 * @param item	- pointer to an item to check for overlap, or NULL if none
 * @return, true if it is covering
 */
bool BattleItem::occupiesSlot(
		int x,
		int y,
		BattleItem* item) const
{
	if (item == this)
		return false;

	if (_inventorySlot->getType() == INV_HAND)
		return true;

	if (item == NULL)
		return (x >= _inventoryX
				&& x < _inventoryX + _rules->getInventoryWidth()
				&& y >= _inventoryY
				&& y < _inventoryY + _rules->getInventoryHeight());
	else
		return !(
				x >= _inventoryX + _rules->getInventoryWidth()
				|| x + item->getRules()->getInventoryWidth() <= _inventoryX
				|| y >= _inventoryY + _rules->getInventoryHeight()
				|| y + item->getRules()->getInventoryHeight() <= _inventoryY);
}

/**
 * Gets an item's currently loaded ammo item.
 * @return, pointer to BattleItem
 *			- NULL if item is ammo or has no ammo loaded
 *			- the weapon itself if weapon is its own ammo
 */
BattleItem* BattleItem::getAmmoItem() const
{
	return _ammoItem;
}

/**
 * Determines if the item uses (needs?) ammo.
 * No ammo is needed if the item has itself assigned as its ammoItem.
 * See set/getAmmoItem()
 * @return, true if ammo is used
 */
bool BattleItem::needsAmmo() const
{
	return (_ammoItem != this);
}

/**
 * Sets an ammo item.
 * @param item - the ammo item
 * @return,	-2 = ammo doesn't fit or nothing happened
 *			-1 = weapon already contains ammo
 *			 0 = success or invalid item
 */
int BattleItem::setAmmoItem(BattleItem* item)
{
	if (needsAmmo() == true)
	{
		if (item == NULL)
		{
			_ammoItem = NULL;
			return 0;
		}

		if (_ammoItem != NULL)
			return -1;

		for (std::vector<std::string>::const_iterator
				i = _rules->getCompatibleAmmo()->begin();
				i != _rules->getCompatibleAmmo()->end();
				++i)
		{
			if (*i == item->getRules()->getType())
			{
				_ammoItem = item;
				return 0;
			}
		}
	}

	return -2;
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
 * Sets the XCom property flag. This is to determine at debriefing what goes into the base/craft.
 * kL_note: no, actually it's not: In its current implementation it is fundamentally useless.
 * kL_note: well it may bear only on artefacts for a Base Defense mission ....
 * @param flag - true if it's XCom property
 */
void BattleItem::setXCOMProperty(bool flag)
{
	_XCOMProperty = flag;
}

/**
 * Gets the XCom property flag. This is to determine at debriefing what goes into the base/craft.
 * kL_note: no, actually it's not: In its current implementation it is fundamentally useless.
 * @return, true if it's XCom property
 */
bool BattleItem::getXCOMProperty() const
{
	return _XCOMProperty;
}

/**
 * Gets the "dropped on non-player turn" flag. This is to determine whether or not aliens
 * should attempt to pick this item up, as items dropped by the player may be "honey traps".
 * kL_note: holy shit that's cynical. (or just fascits). Worse than 'honey traps' are
 * players, like me, who Mc aLiens and make them drop their weapons - on the xCom turn!
 * so, in a word or 25, TAKE THIS OUT.
 * @return. true if the aliens dropped the item
 */
/* bool BattleItem::getTurnFlag() const
{
	return _droppedOnAlienTurn;
} */

/**
 * Sets the "dropped on non-player turn" flag. This is set when the item
 * is dropped in the battlescape or picked up in the inventory screen.
 * @param flag - true if the aliens dropped the item
 */
/* void BattleItem::setTurnFlag(bool flag)
{
	_droppedOnAlienTurn = flag;
} */

/**
 * Converts an unconscious body into a dead one.
 * @param rules - pointer to rules of the corpse item to convert this item into
 */
void BattleItem::convertToCorpse(const RuleItem* rules)
{
	if (_unit != NULL
		&& _rules->getBattleType() == BT_CORPSE
		&& rules->getBattleType() == BT_CORPSE)
	{
		_rules = const_cast<RuleItem*>(rules);
	}
}

}
