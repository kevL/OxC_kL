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

#include "Production.h"

//#include <limits>

#include "Base.h"
#include "Craft.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "SavedGame.h"

//#include "../Engine/Options.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param manufRule	- pointer to RuleManufacture
 * @param amount	- quantity to produce
 */
Production::Production(
		const RuleManufacture* const manufRule,
		int amount)
	:
		_manufRule(manufRule),
		_amount(amount),
		_infinite(false),
		_timeSpent(0),
		_engineers(0),
		_sell(false)
{}

/**
 * dTor.
 */
Production::~Production()
{}

/**
 * Loads from YAML.
 */
void Production::load(const YAML::Node& node)
{
	_engineers	= node["assigned"]	.as<int>(_engineers);
	_timeSpent	= node["spent"]		.as<int>(_timeSpent);
	_amount		= node["amount"]	.as<int>(_amount);
	_infinite	= node["infinite"]	.as<bool>(_infinite);
	_sell		= node["sell"]		.as<bool>(_sell);
}

/**
 * Saves to YAML.
 */
YAML::Node Production::save() const
{
	YAML::Node node;

	node["item"]		= _manufRule->getName();
	node["assigned"]	= _engineers;
	node["spent"]		= _timeSpent;
	node["amount"]		= _amount;

	if (_infinite == true)	node["infinite"]	= _infinite;
	if (_sell == true)		node["sell"]		= _sell;

	return node;
}

/**
 *
 */
int Production::getAmountTotal() const
{
	return _amount;
}

/**
 *
 */
void Production::setAmountTotal(int amount)
{
	_amount = amount;
}

/**
 *
 */
bool Production::getInfiniteAmount() const
{
	return _infinite;
}

/**
 *
 */
void Production::setInfiniteAmount(bool infinite)
{
	_infinite = infinite;
}

/**
 *
 */
int Production::getTimeSpent() const
{
	return _timeSpent;
}

/**
 *
 */
void Production::setTimeSpent(int spent)
{
	_timeSpent = spent;
}

/**
 *
 */
int Production::getAssignedEngineers() const
{
	return _engineers;
}

/**
 *
 */
void Production::setAssignedEngineers(int engineers)
{
	_engineers = engineers;
}

/**
 *
 */
bool Production::getSellItems() const
{
	return _sell;
}

/**
 *
 */
void Production::setSellItems(bool sell)
{
	_sell = sell;
}

/**
 *
 */
bool Production::enoughMoney(const SavedGame* const gameSave) const // private.
{
	return (gameSave->getFunds() >= _manufRule->getManufactureCost());
}

/**
 *
 */
bool Production::enoughMaterials(Base* const base) const // private.
{
	for (std::map<std::string,int>::const_iterator
			i = _manufRule->getRequiredItems().begin();
			i != _manufRule->getRequiredItems().end();
			++i)
	{
		if (base->getStorageItems()->getItemQty(i->first) < i->second)
			return false;
	}

	return true;
}

/**
 *
 */
ProductionProgress Production::step(
		Base* const base,
		SavedGame* const gameSave,
		const Ruleset* const rules)
{
	const int qtyDone = getAmountProduced();
	_timeSpent += _engineers;

	if (Options::canManufactureMoreItemsPerHour == false
		&& qtyDone < getAmountProduced())
	{
		// enforce pre-TFTD manufacturing rules: extra hours are wasted
		_timeSpent = (qtyDone + 1) * _manufRule->getManufactureTime();
	}

	if (qtyDone < getAmountProduced())
	{

		int produced;
		if (_infinite == false)
			produced = std::min( // required to not overproduce
							getAmountProduced(),
							_amount) - qtyDone;
		else
			produced = getAmountProduced() - qtyDone;

		int qty = 0;
		do
		{
			for (std::map<std::string, int>::const_iterator
					i = _manufRule->getProducedItems().begin();
					i != _manufRule->getProducedItems().end();
					++i)
			{
				if (_manufRule->getCategory() == "STR_CRAFT")
				{
					Craft* const craft = new Craft(
												rules->getCraft(i->first),
												base,
												gameSave->getCanonicalId(i->first));
					craft->setCraftStatus("STR_REFUELLING");
					base->getCrafts()->push_back(craft);

					break;
				}
				else
				{
					// Check if it's fuel OR ammo for a Craft
					if (rules->getItem(i->first)->getBattleType() == BT_NONE)
					{
						for (std::vector<Craft*>::const_iterator
								j = base->getCrafts()->begin();
								j != base->getCrafts()->end();
								++j)
						{
							if ((*j)->getWarned() == true)
							{
								if ((*j)->getCraftStatus() == "STR_REFUELING")
								{
									if ((*j)->getRules()->getRefuelItem() == i->first)
										(*j)->setWarned(false);
								}
								else if ((*j)->getCraftStatus() == "STR_REARMING")
								{
									for (std::vector<CraftWeapon*>::const_iterator
											k = (*j)->getWeapons()->begin();
											k != (*j)->getWeapons()->end();
											++k)
									{
										if (*k != NULL
											&& (*k)->getRules()->getClipItem() == i->first)
										{
											(*k)->setCantLoad(false);
											(*j)->setWarned(false);
										}
									}
								}
							}
						}
					}

					// kL_note: MISSING - check if it's ammo to load a tank (onboard a Craft)!
					// from, ItemsArrivingState:
/*					RuleItem* item = _game->getRuleset()->getItem((*j)->getItems());

					if (rules->getItem(i->first)->getBattleType() == BT_AMMO)
					{
						for (std::vector<Craft*>::iterator
								c = base->getCrafts()->begin();
								c != base->getCrafts()->end();
								++c)
						{
							for (std::vector<Vehicle*>::iterator // check if it's ammo to reload a vehicle
									v = (*c)->getVehicles()->begin();
									v != (*c)->getVehicles()->end();
									++v)
							{
								std::vector<std::string>::iterator ammo = std::find(
																				(*v)->getRules()->getCompatibleAmmo()->begin(),
																				(*v)->getRules()->getCompatibleAmmo()->end(),
																				item->getType());
								if (ammo != (*v)->getRules()->getCompatibleAmmo()->end()
									&& (*v)->getAmmo() < item->getClipSize())
								{
									int used = std::min(
													(*j)->getQuantity(),
													item->getClipSize() - (*v)->getAmmo());
									(*v)->setAmmo((*v)->getAmmo() + used);

									// Note that the items have already been delivered --
									// so they are removed from the base, not the transfer.
									base->getItems()->removeItem(item->getType(), used);
								}
							}
						}
					} */

					if (_sell == true) // <- this may be Fucked.
					{
						gameSave->setFunds(gameSave->getFunds() + (rules->getItem(i->first)->getSellCost() * i->second));
						base->setCashIncome(rules->getItem(i->first)->getSellCost() * i->second);
					}
					else
						base->getStorageItems()->addItem(i->first, i->second);
				}
			}

			++qty;
			if (qty < produced)
			{
				if (enoughMoney(gameSave) == false)
					return PROGRESS_NOT_ENOUGH_MONEY;

				if (enoughMaterials(base) == false)
					return PROGRESS_NOT_ENOUGH_MATERIALS;

				startProduction(base, gameSave);
			}
		}
		while (qty < produced);
	}

	if (getAmountProduced() >= _amount
		&& _infinite == false)
	{
		return PROGRESS_COMPLETE;
	}

	if (qtyDone < getAmountProduced())
	{
		if (enoughMoney(gameSave) == false)
			return PROGRESS_NOT_ENOUGH_MONEY;

		if (enoughMaterials(base) == false)
			return PROGRESS_NOT_ENOUGH_MATERIALS;

		startProduction(base, gameSave);
	}

	return PROGRESS_NOT_COMPLETE;
}

/**
 *
 */
int Production::getAmountProduced() const
{
	return _timeSpent / _manufRule->getManufactureTime();
}

/**
 *
 */
const RuleManufacture* Production::getRules() const
{
	return _manufRule;
}

/**
 *
 */
void Production::startProduction(
		Base* const base,
		SavedGame* const gameSave) const
{
	const int cost = _manufRule->getManufactureCost();
	gameSave->setFunds(gameSave->getFunds() - cost);
	base->setCashSpent(cost);

	for (std::map<std::string,int>::const_iterator
			i = _manufRule->getRequiredItems().begin();
			i != _manufRule->getRequiredItems().end();
			++i)
	{
		base->getStorageItems()->removeItem(i->first, i->second);
	}
}

}
