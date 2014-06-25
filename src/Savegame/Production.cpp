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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Production.h"

#include <limits>

#include "Base.h"
#include "Craft.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "SavedGame.h"

#include "../Engine/Options.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 *
 */
Production::Production(
		const RuleManufacture* rules,
		int amount)
	:
		_rules(rules),
		_amount(amount),
		_infinite(false),
		_timeSpent(0),
		_engineers(0),
		_sell(false)
{
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
void Production::setTimeSpent(int done)
{
	_timeSpent = done;
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
bool Production::enoughMoney(SavedGame* g)
{
	return (g->getFunds() >= _rules->getManufactureCost());
}

/**
 *
 */
bool Production::enoughMaterials(Base* b)
{
	for (std::map<std::string,int>::const_iterator
			i = _rules->getRequiredItems().begin();
			i != _rules->getRequiredItems().end();
			++i)
	{
		if (b->getItems()->getItem(i->first) < i->second)
			return false;
	}

	return true;
}

/**
 *
 */
ProductProgress Production::step(
		Base* b,
		SavedGame* g,
		const Ruleset* r)
{
	int done = getAmountProduced();
	_timeSpent += _engineers;

	if (done < getAmountProduced())
	{
		// std::min is required because we don't want to overproduce
		int produced = std::min(
							getAmountProduced(),
							_amount)
						- done;
		int count = 0;

		do
		{
			for (std::map<std::string,int>::const_iterator
					i = _rules->getProducedItems().begin();
					i != _rules->getProducedItems().end();
					++i)
			{
				if (_rules->getCategory() == "STR_CRAFT")
				{
					Craft* craft = new Craft(
											r->getCraft(i->first),
											b,
											g->getId(i->first));
					craft->setStatus("STR_REFUELLING");
					b->getCrafts()->push_back(craft);

					break;
				}
				else
				{
					// Check if it's ammo to reload a craft
					if (r->getItem(i->first)->getBattleType() == BT_NONE)
					{
						for (std::vector<Craft*>::iterator
								c = b->getCrafts()->begin();
								c != b->getCrafts()->end();
								++c)
						{
							if ((*c)->getStatus() != "STR_READY")
								continue;

							for (std::vector<CraftWeapon*>::iterator
									w = (*c)->getWeapons()->begin();
									w != (*c)->getWeapons()->end();
									++w)
							{
								if (*w != 0
									&& (*w)->getRules()->getClipItem() == i->first
									&& (*w)->getAmmo() < (*w)->getRules()->getAmmoMax())
								{
									(*w)->setRearming(true);
									(*c)->setStatus("STR_REARMING");
								}
							}
						}
					}

					// Check if it's fuel to refuel a craft
					if (r->getItem(i->first)->getBattleType() == BT_NONE)
					{
						for (std::vector<Craft*>::iterator
								c = b->getCrafts()->begin();
								c != b->getCrafts()->end();
								++c)
						{
							if ((*c)->getStatus() != "STR_READY")
								continue;

							if ((*c)->getRules()->getRefuelItem() == i->first
								&& (*c)->getFuelPercentage() < 100)
							{
								(*c)->setStatus("STR_REFUELLING");
							}
						}
					}

					if (getSellItems()) // <- this may be Fucked. kL_note
					{
						g->setFunds(g->getFunds() + (r->getItem(i->first)->getSellCost() * i->second));
						b->setCashIncome(r->getItem(i->first)->getSellCost() * i->second); // kL
					}
					else
						b->getItems()->addItem(i->first, i->second);
				}
			}

			if (!Options::canManufactureMoreItemsPerHour)
				break;

			count++;
			if (count < produced)
			{
				// We need to ensure that player has enough cash/item to produce a new unit
				if (!enoughMoney(g))
					return PROGRESS_NOT_ENOUGH_MONEY;

				if (!enoughMaterials(b))
					return PROGRESS_NOT_ENOUGH_MATERIALS;

				// kL_note: NOT ENOUGH HANGAR SPACE!!!

				startItem(b, g);
			}
		}
		while (count < produced);
	}

	if (getAmountProduced() >= _amount
		&& !getInfiniteAmount())
	{
		return PROGRESS_COMPLETE;
	}

	if (done < getAmountProduced())
	{
		// We need to ensure that player has enough cash/item to produce a new unit
		if (!enoughMoney(g))
			return PROGRESS_NOT_ENOUGH_MONEY;

		if (!enoughMaterials(b))
			return PROGRESS_NOT_ENOUGH_MATERIALS;

		// kL_note: NOT ENOUGH HANGAR SPACE!!!

		startItem(b, g);
	}

	return PROGRESS_NOT_COMPLETE;
}

/**
 *
 */
int Production::getAmountProduced() const
{
	return _timeSpent / _rules->getManufactureTime();
}

/**
 *
 */
const RuleManufacture* Production::getRules() const
{
	return _rules;
}

/**
 *
 */
void Production::startItem(
		Base* b,
		SavedGame* g)
{
	int cost = _rules->getManufactureCost();
	g->setFunds(g->getFunds() - cost);
	b->setCashSpent(cost); // kL

	for (std::map<std::string,int>::const_iterator
			i = _rules->getRequiredItems().begin();
			i != _rules->getRequiredItems().end();
			++i)
	{
		b->getItems()->removeItem(i->first, i->second);
	}
}

/**
 *
 */
void Production::load(const YAML::Node& node)
{
	setAssignedEngineers(node["assigned"].as<int>(getAssignedEngineers()));
	setTimeSpent(node["spent"].as<int>(getTimeSpent()));

	setAmountTotal(node["amount"].as<int>(getAmountTotal()));
	setInfiniteAmount(node["infinite"].as<bool>(getInfiniteAmount()));
	setSellItems(node["sell"].as<bool>(getSellItems()));

	// backwards compatiblity
/*kL
	if (getAmountTotal() == std::numeric_limits<int>::max())
	{
		setAmountTotal(999);
		setInfiniteAmount(true);
		setSellItems(true);
	} */
}

/**
 *
 */
YAML::Node Production::save() const
{
	YAML::Node node;

	node["item"]		= getRules()->getName();
	node["assigned"]	= getAssignedEngineers();
	node["spent"]		= getTimeSpent();
	node["amount"]		= getAmountTotal();
	node["infinite"]	= getInfiniteAmount();

	if (getSellItems())
		node["sell"]	= getSellItems();

	return node;
}

}
