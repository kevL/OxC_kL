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

//#define _USE_MATH_DEFINES

#include "Base.h"

//#include <algorithm>
//#include <cmath>
//#include <stack>

//#include "../fmath.h"

#include "BaseFacility.h"
#include "Craft.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "Production.h"
#include "ResearchProject.h"
#include "Soldier.h"
#include "Target.h"
#include "Transfer.h"
#include "Ufo.h"
#include "Vehicle.h"

#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes an empty base.
 * @param rule - pointer to Ruleset
 */
Base::Base(const Ruleset* const rules)
	:
		Target(),
		_rules(rules),
		_scientists(0),
		_engineers(0),
		_inBattlescape(false),
		_retaliationTarget(false),
		_cashIncome(0),
		_cashSpent(0),
		_defenseResult(0),
		_curSoldier(0)
{
	_items = new ItemContainer();
}

/**
 * Deletes the contents of the base from memory.
 */
Base::~Base()
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		for (std::vector<Vehicle*>::const_iterator
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
		{
			for (std::vector<Vehicle*>::const_iterator
					k = _vehicles.begin();
					k != _vehicles.end();
					++k)
			{
				if (*k == *j) // to avoid calling a vehicle's destructor twice
				{
					_vehicles.erase(k);
					break;
				}
			}
		}

		delete *i;
	}

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		delete *i;
	}

	delete _items;

	for (std::vector<ResearchProject*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads a base from a YAML file.
 * @param node			- reference a YAML node
 * @param save			- pointer to SavedGame
 * @param newGame		- true if this is the first Base of a new game
 * @param newBattleGame	- true if this is the Base of a skirmish game
 */
void Base::load(
		const YAML::Node& node,
		SavedGame* save,
		bool newGame,
		bool newBattleGame)
{
	Target::load(node);

	_name = Language::utf8ToWstr(node["name"].as<std::string>(""));

	if (newGame == false
		|| !Options::customInitialBase
		|| newBattleGame)
	{
		for (YAML::const_iterator
				i = node["facilities"].begin();
				i != node["facilities"].end();
				++i)
		{
			std::string type = (*i)["type"].as<std::string>();
			if (_rules->getBaseFacility(type))
			{
				BaseFacility* f = new BaseFacility(
												_rules->getBaseFacility(type),
												this);
				f->load(*i);
				_facilities.push_back(f);
			}
		}
	}

	for (YAML::const_iterator
			i = node["crafts"].begin();
			i != node["crafts"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (_rules->getCraft(type))
		{
			Craft* c = new Craft(
							_rules->getCraft(type),
							this);
			c->load(
					*i,
					_rules,
					save);
			_crafts.push_back(c);
		}
	}

	for (YAML::const_iterator
			i = node["soldiers"].begin();
			i != node["soldiers"].end();
			++i)
	{
		Soldier* s = new Soldier(
							_rules->getSoldier("XCOM"),
							_rules->getArmor("STR_ARMOR_NONE_UC"));
		s->load(
				*i,
				_rules,
				save);
		s->setCraft(NULL);

		if (const YAML::Node& craft = (*i)["craft"])
		{
			CraftId craftId = Craft::loadId(craft);

			for (std::vector<Craft*>::const_iterator
					j = _crafts.begin();
					j != _crafts.end();
					++j)
			{
				if ((*j)->getUniqueId() == craftId)
				{
					s->setCraft(*j);
					break;
				}
			}
		}

		_soldiers.push_back(s);
	}

	_items->load(node["items"]);
	// Some old saves have bad items, better get rid of them to avoid further bugs
	for (std::map<std::string, int>::const_iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			)
	{
		if (std::find(
					_rules->getItemsList().begin(),
					_rules->getItemsList().end(),
					i->first) == _rules->getItemsList().end())
		{
			_items->getContents()->erase(i++);
		}
		else
			++i;
	}

	_scientists		= node["scientists"]	.as<int>(_scientists);
	_engineers		= node["engineers"]		.as<int>(_engineers);
	_inBattlescape	= node["inBattlescape"]	.as<bool>(_inBattlescape);
	_cashIncome		= node["cashIncome"]	.as<int>(_cashIncome);
	_cashSpent		= node["cashSpent"]		.as<int>(_cashSpent);

	for (YAML::const_iterator
			i = node["transfers"].begin();
			i != node["transfers"].end();
			++i)
	{
		int hours = (*i)["hours"].as<int>();
		Transfer* t = new Transfer(hours);
		if (t->load(
				*i,
				this,
				_rules,
				save))
		{
			_transfers.push_back(t);
		}
	}

	for (YAML::const_iterator
			i = node["research"].begin();
			i != node["research"].end();
			++i)
	{
		std::string research = (*i)["project"].as<std::string>();
		if (_rules->getResearch(research))
		{
			ResearchProject* r = new ResearchProject(_rules->getResearch(research));
			r->load(*i);
			_research.push_back(r);
		}
		else
			_scientists += (*i)["assigned"].as<int>(0);
	}

	for (YAML::const_iterator
			i = node["productions"].begin();
			i != node["productions"].end();
			++i)
	{
		std::string item = (*i)["item"].as<std::string>();
		if (_rules->getManufacture(item))
		{
			Production* p = new Production(
										_rules->getManufacture(item),
										0);
			p->load(*i);
			_productions.push_back(p);
		}
		else
			_engineers += (*i)["assigned"].as<int>(0);
	}

	_retaliationTarget = node["retaliationTarget"].as<bool>(_retaliationTarget);
}

/**
 * Saves this Base to a YAML file.
 * @return, YAML node
 */
YAML::Node Base::save() const
{
	YAML::Node node = Target::save();

	node["name"] = Language::wstrToUtf8(_name);

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		node["facilities"].push_back((*i)->save());
	}

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		node["soldiers"].push_back((*i)->save());
	}

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		node["crafts"].push_back((*i)->save());
	}

	node["items"]			= _items->save();
	node["scientists"]		= _scientists;
	node["engineers"]		= _engineers;
	node["inBattlescape"]	= _inBattlescape;
	node["cashIncome"]		= _cashIncome;
	node["cashSpent"]		= _cashSpent;

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		node["transfers"].push_back((*i)->save());
	}

	for (std::vector<ResearchProject*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		node["research"].push_back((*i)->save());
	}

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		node["productions"].push_back((*i)->save());
	}

	node["retaliationTarget"] = _retaliationTarget;

	return node;
}

/**
 * Saves this Base's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node Base::saveId() const
{
	YAML::Node node = Target::saveId();

	node["type"]	= "STR_BASE";
	node["id"]		= 0;

	return node;
}

/**
 * Returns the custom name for this Base.
 * @param lang - pointer to Language to get strings from
 * @return, name string
 */
std::wstring Base::getName(Language*) const
{
	return _name;
}

/**
 * Changes the custom name for this Base.
 * @param name - reference a name string
 */
void Base::setName(const std::wstring& name)
{
	_name = name;
}

/**
 * Returns the globe marker for this Base.
 * @return, the corresponding marker sprite (-1 if none)
 */
int Base::getMarker() const
{
	// cheap hack to hide Bases when they haven't been placed yet
	if (AreSame(_lon, 0.) && AreSame(_lat, 0.))
		return -1;

	return 0;
}

/**
 * Returns the list of facilities in this Base.
 * @return, pointer to a vector of pointers to BaseFacility
 */
std::vector<BaseFacility*>* Base::getFacilities()
{
	return &_facilities;
}

/**
 * Returns the list of soldiers in this Base.
 * @return, pointer to a vector of pointers to Soldier
 */
std::vector<Soldier*>* Base::getSoldiers()
{
	return &_soldiers;
}

/**
 * Returns the list of crafts in this Base.
 * @return, pointer to a vector of pointers to Craft
 */
std::vector<Craft*>* Base::getCrafts()
{
	return &_crafts;
}

/**
 * Returns the list of transfers destined to this Base.
 * @return, pointer to a vector of pointers to Transfer
 */
std::vector<Transfer*>* Base::getTransfers()
{
	return &_transfers;
}

/**
 * Returns the list of items in this Base.
 * @return, pointer to ItemContainer
 */
ItemContainer* Base::getItems()
{
	return _items;
}

/**
 * Returns the amount of scientists currently in the base.
 * @return, Number of scientists.
 */
int Base::getScientists() const
{
	return _scientists;
}

/**
 * Changes the amount of scientists currently in the base.
 * @param scientists Number of scientists.
 */
void Base::setScientists(int scientists)
{
	 _scientists = scientists;
}

/**
 * Returns the amount of engineers currently in the base.
 * @return, Number of engineers.
 */
int Base::getEngineers() const
{
	return _engineers;
}

/**
 * Changes the amount of engineers currently in the base.
 * @param engineers Number of engineers.
 */
void Base::setEngineers(int engineers)
{
	 _engineers = engineers;
}

/**
 * Returns if a certain target is covered by the base's
 * radar range taking in account the range and chance.
 * @param target - pointer to a UFO to attempt detection against
 * @return,	0 undetected
 *			1 hyperdetected only
 *			2 detected
 *			3 detected & hyperdetected
 */
int Base::detect(Target* target) const
{
	double targetDist = insideRadarRange(target);

	if (AreSame(targetDist, 0.))
		return 0;

	int ret = 0;

	if (targetDist < 0.)
	{
		++ret;
		targetDist = -targetDist;
	}

	int chance = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			const double radarRange = static_cast<double>((*i)->getRules()->getRadarRange()) * greatCircleConversionFactor;
			if (radarRange > targetDist)
				chance += (*i)->getRules()->getRadarChance();
		}
	}

	const Ufo* const ufo = dynamic_cast<Ufo*>(target);
	if (ufo != NULL)
	{
		chance += ufo->getVisibility();
		chance = static_cast<int>(Round(static_cast<double>(chance) / 3.)); // per 10-min.

		if (RNG::percent(chance) == true)
			ret += 2;
	}

	return ret;
}

/**
 * Returns if a certain target is inside this Base's radar range
 * taking in account the global positions of both.
 * @param target - pointer to UFO
 * @return, great circle distance to UFO (negative if hyperdetected)
 */
double Base::insideRadarRange(Target* target) const
{
	const double targetDist = getDistance(target) * earthRadius;
	if (targetDist > static_cast<double>(_rules->getMaxRadarRange()) * greatCircleConversionFactor)
		return 0.;


	double ret = 0.; // lets hope UFO is not *right on top of Base* Lol
	bool hyperDet = false;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end()
				&& hyperDet == false;
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			const double radarRange = static_cast<double>((*i)->getRules()->getRadarRange()) * greatCircleConversionFactor;
			if (targetDist < radarRange)
			{
				ret = targetDist; // identical value for every i; looking only for hyperDet after 1st successful iteration.

				if ((*i)->getRules()->isHyperwave() == true)
					hyperDet = true;
			}
		}
	}

	if (hyperDet == true)
		ret = -ret;

	return ret;
}

/**
 * Returns the amount of soldiers with neither a Craft assignment nor wounds at this Base.
 * @param combatReady - does what it says on the tin. [ bull..] (default false)
 * @return, number of soldiers
 */
int Base::getAvailableSoldiers(const bool combatReady) const
{
	int total = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if (combatReady == false
			&& (*i)->getCraft() == NULL)
		{
			++total;
		}
		else if (combatReady == true
			&& (((*i)->getCraft() != NULL
					&& (*i)->getCraft()->getStatus() != "STR_OUT")
				|| ((*i)->getCraft() == NULL
					&& (*i)->getWoundRecovery() == 0)))
		{
			++total;
		}
	}

	return total;
}

/**
 * Returns the total amount of soldiers contained in the base.
 * @return, number of soldiers
 */
int Base::getTotalSoldiers() const
{
	int total = static_cast<int>(_soldiers.size());

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_SOLDIER)
			total += (*i)->getQuantity();
	}

	return total;
}

/**
 * Returns the amount of scientists contained in the base without any assignments.
 * @return, number of scientists
 */
/*kL int Base::getAvailableScientists() const
{
	return getScientists();
} */

/**
 * Returns the total amount of scientists contained in the base.
 * @return, number of scientists
 */
int Base::getTotalScientists() const
{
	int total = _scientists;

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_SCIENTIST)
			total += (*i)->getQuantity();
	}

	const std::vector<ResearchProject*>& research(getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = research.begin();
			i != research.end();
			++i)
	{
		total += (*i)->getAssigned();
	}

	return total;
}

/**
 * Returns the amount of engineers contained in the base without any assignments.
 * @return, number of engineers
 */
/*kL int Base::getAvailableEngineers() const
{
	return getEngineers();
} */

/**
 * Returns the total amount of engineers contained in the base.
 * @return, number of engineers
 */
int Base::getTotalEngineers() const
{
	int total = _engineers;

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_ENGINEER)
			total += (*i)->getQuantity();
	}

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		total += (*i)->getAssignedEngineers();
	}

	return total;
}

/**
 * Returns the amount of living quarters used up by personnel in the base.
 * @return, living space
 */
int Base::getUsedQuarters() const
{
	return getTotalSoldiers() + getTotalScientists() + getTotalEngineers();
}

/**
 * Returns the total amount of living quarters available in the base.
 * @return, living space
 */
int Base::getAvailableQuarters() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getPersonnel();
	}

	return total;
}

/**
 * Returns the amount of stores used up by equipment in this Base
 * including equipment on Craft and about to arrive in Transfers.
 * @return, storage space
 */
double Base::getUsedStores()
{
	double total = _items->getTotalSize(_rules);

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		total += (*i)->getItems()->getTotalSize(_rules);

		for (std::vector<Vehicle*>::const_iterator
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
		{
			total += (*j)->getRules()->getSize();
		}
	}

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM)
		{
			total += _rules->getItem((*i)->getItems())->getSize()
				   * static_cast<double>((*i)->getQuantity());
		}
		else if ((*i)->getType() == TRANSFER_CRAFT)
		{
			Craft* craft = (*i)->getCraft();
			total += craft->getItems()->getTotalSize(_rules);

			for (std::vector<Vehicle*>::const_iterator
					j = craft->getVehicles()->begin();
					j != craft->getVehicles()->end();
					++j)
			{
				total += (*j)->getRules()->getSize();
			}
		}
	}

//	total -= getIgnoredStores();
	return total;
}

/**
 * Checks if a base's stores are overfilled.
 * Supplying an offset will add/subtract to the used capacity before performing the check.
 * A positive offset simulates adding items to the stores, whereas a negative offset can be
 * used to check whether sufficient items have been removed to stop stores from overflowing.
 * @param offset - adjusts used capacity (default 0.)
 * @return, true if this Base's stores are over their limit
 */
bool Base::storesOverfull(double offset)
{
	const double
		capacity = static_cast<double>(getAvailableStores()),
		used = getUsedStores();

	return (used + offset > capacity + 0.05);
}

/**
 * Returns the total amount of stores available in the base.
 * @return, storage space
 */
int Base::getAvailableStores() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getStorage();
	}

	return total;
}

/**
 * Determines space taken up by ammo clips about to rearm craft.
 * @return, ignored storage space
 */
/* double Base::getIgnoredStores()
{
	double space = 0.0;

	for (std::vector<Craft*>::const_iterator
			c = getCrafts()->begin();
			c != getCrafts()->end();
			++c)
	{
		if ((*c)->getStatus() == "STR_REARMING")
		{
			for (std::vector<CraftWeapon*>::const_iterator
					w = (*c)->getWeapons()->begin();
					w != (*c)->getWeapons()->end();
					++w)
			{
				if (*w != NULL
					&& (*w)->getRearming() == true)
				{
					const std::string clip = (*w)->getRules()->getClipItem();
					if (clip.empty() == false)
					{
						const int baseQty = getItems()->getItem(clip);
						if (baseQty > 0)
						{
							const int clipSize = _rules->getItem(clip)->getClipSize();
							if (clipSize > 0)
							{
								const int toLoad = static_cast<int>(ceil(
												   static_cast<double>((*w)->getRules()->getAmmoMax() - (*w)->getAmmo())
														/ static_cast<double>(clipSize)));

								space += static_cast<double>(std::min(
																	baseQty,
																	toLoad))
										* static_cast<double>(_rules->getItem(clip)->getSize());
							}
						}
					}
				}
			}
		}
	}

	return space;
} */

/**
 * Returns the amount of laboratories used up by research projects in the base.
 * @return, laboratory space
 */
int Base::getUsedLaboratories() const
{
	int usedLabSpace = 0;

	const std::vector<ResearchProject*>& research(getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = research.begin();
			i != research.end();
			++i)
	{
		usedLabSpace += (*i)->getAssigned();
	}

	return usedLabSpace;
}

/**
 * Returns the total amount of laboratories available in the base.
 * @return, laboratory space
 */
int Base::getAvailableLaboratories() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getLaboratories();
	}

	return total;
}

/**
 * Returns the amount of workshops used up by manufacturing projects in the base.
 * @return, storage space
 */
int Base::getUsedWorkshops() const
{
	int usedWorkShop = 0;

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		usedWorkShop += (*i)->getAssignedEngineers() + (*i)->getRules()->getRequiredSpace();
	}

	return usedWorkShop;
}

/**
 * Returns the total amount of workshops available in the base.
 * @return, workshop space
 */
int Base::getAvailableWorkshops() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getWorkshops();
	}

	return total;
}

/**
 * Returns the amount of hangars used up by crafts in the base.
 * @return, storage space
 */
int Base::getUsedHangars() const
{
	int total = static_cast<int>(_crafts.size());

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT)
			total += (*i)->getQuantity();
	}

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		if ((*i)->getRules()->getCategory() == "STR_CRAFT")
			// This should be fixed on the case when (*i)->getInfiniteAmount() == TRUE
			total += ((*i)->getAmountTotal() - (*i)->getAmountProduced());
	}

	return total;
}

/**
 * Returns the total amount of hangars available in the base.
 * @return, number of hangars
 */
int Base::getAvailableHangars() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getCrafts();
	}

	return total;
}

/**
 * Return laboratories space not used by a ResearchProject
 * @return, laboratories space not used by a ResearchProject
*/
int Base::getFreeLaboratories() const
{
	return getAvailableLaboratories() - getUsedLaboratories();
}

/**
 * Return workshop space not used by a Production
 * @return, workshop space not used by a Production
*/
int Base::getFreeWorkshops() const
{
	return getAvailableWorkshops() - getUsedWorkshops();
}

/**
 * Return psilab space not in use
 * @return, psilab space not in use
*/
int Base::getFreePsiLabs() const
{
	return getAvailablePsiLabs() - getUsedPsiLabs();
}

/**
 * Returns the amount of scientists currently in use.
 * @return, Amount of scientists.
*/
int Base::getAllocatedScientists() const
{
	int total = 0;

	const std::vector<ResearchProject*>& research(getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = research.begin();
			i != research.end();
			++i)
	{
		total += (*i)->getAssigned();
	}

	return total;
}

/**
 * Returns the amount of engineers currently in use.
 * @return, Amount of engineers.
*/
int Base::getAllocatedEngineers() const
{
	int total = 0;

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		total += (*i)->getAssignedEngineers();
	}

	return total;
}

/**
 * Returns the total defense value of all the facilities in the base.
 * @return, defense value
 */
int Base::getDefenseValue() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getDefenseValue();
	}

	return total;
}

/**
 * Returns the total amount of short range detection facilities in the base.
 * @return, Short Range Detection value.
 */
/*kL int Base::getShortRangeDetection() const
{
	int
		total = 0,
		range = 0;

	int minRadarRange = _rules->getMinRadarRange();
	if (minRadarRange == 0)
		return 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getRadarRange() > 0)
		{
			range = (*i)->getRules()->getRadarRange();
			// kL_note: that should be based off a string or Ruleset value.

			if ((*i)->getRules()->getRadarRange() <= minRadarRange)
//			if (range < 1501) // was changed to 1701
			{
				total++;
			}
		}
	}

	return total;
} */

/**
 * kL. Returns the total value of short range detection facilities at this base.
 * Used for BaseInfoState graphics.
 * @return, short range detection value (%)
 */
int Base::getShortRangeValue() const // kL
{
	int
		total = 0,
		range = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			range = (*i)->getRules()->getRadarRange();
			if (range > 0
				&& range < _rules->getRadarCutoffRange() + 1)
			{
				total += (*i)->getRules()->getRadarChance();

				if (total > 99)
					return 100;
			}
		}
	}

	return total;
}

/**
 * Returns the total amount of long range detection facilities in the base.
 * @return, Long Range Detection value.
 */
/*kL int Base::getLongRangeDetection() const
{
	int total = 0;
	int minRadarRange = _rules->getMinRadarRange();

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getRules()->getRadarRange() > minRadarRange
			&& (*i)->getBuildTime() == 0)
//			&& (*i)->getRules()->getRadarRange() > 1500) // was changed to 1700
		{
			total++;
		}
	}

	return total;
} */

/**
 * kL. Returns the total value of long range detection facilities at this base.
 * Used for BaseInfoState graphics.
 * @return, long range detection value (%)
 */
int Base::getLongRangeValue() const // kL
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getRadarRange() > _rules->getRadarCutoffRange())
		{
			total += (*i)->getRules()->getRadarChance();

			if (total > 99)
				return 100;
		}
	}

	return total;
}

/**
 * Returns the total amount of craft of a certain type stored at or being
 * tranferred to this base. Used for MonthlyCostsState.
 * @param craft - craft type
 * @return, quantity of craft type
 */
int Base::getCraftCount(const std::string& craft) const
{
	int total = 0;

	for (std::vector<Transfer*>::const_iterator // kL_begin:
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT
			&& (*i)->getCraft()->getRules()->getType() == craft)
		{
			total += (*i)->getQuantity();
		}
	} // kL_end.

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		if ((*i)->getRules()->getType() == craft)
			total++;
	}

	return total;
}

/**
 * Returns the total amount of monthly costs for maintaining the craft in the base.
 * Used for month-end maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getCraftMaintenance() const
{
	int total = 0;

	for (std::vector<Transfer*>::const_iterator // kL_begin: craft-transfer bugfix.
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT)
			total += (*i)->getQuantity() * (*i)->getCraft()->getRules()->getRentCost();
	} // kL_end.

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		total += (*i)->getRules()->getRentCost();
	}

	return total;
}

/**
 * Returns the total amount of monthly costs for maintaining the personnel in the base.
 * Used for month-end maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getPersonnelMaintenance() const
{
	int total = 0;

	total += getTotalSoldiers() * _rules->getSoldierCost();
	total += getTotalEngineers() * _rules->getEngineerCost();
	total += getTotalScientists() * _rules->getScientistCost();

	return total;
}

/**
 * Returns the total amount of monthly costs for maintaining the facilities in the base.
 * Used for month-end maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getFacilityMaintenance() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getMonthlyCost();
	}

	return total;
}

/**
 * Returns the total amount of all the maintenance monthly costs in the base.
 * Used for month-end maintenance expenditure.
 * @return, Maintenance costs.
 */
int Base::getMonthlyMaintenace() const
{
	return getCraftMaintenance() + getPersonnelMaintenance() + getFacilityMaintenance();
}

/**
 * Add a new Production to the Base
 * @param prod - pointer to a Production
 */
void Base::addProduction(Production* prod)
{
	_productions.push_back(prod);
}

/**
 * Remove a Production from the Base.
 * @param prod - pointer to a Production
 */
void Base::removeProduction(Production* prod)
{
	_engineers += prod->getAssignedEngineers();

	std::vector<Production*>::const_iterator i = std::find(
														_productions.begin(),
														_productions.end(),
														prod);
	if (i != _productions.end())
		_productions.erase(i);
}

/**
 * Get the list of Base Productions.
 * @return, the list of Base Productions
 */
const std::vector<Production*>& Base::getProductions() const
{
	return _productions;
}

/**
 * Returns the list of all base's ResearchProject
 * @return, list of base's ResearchProject
 */
const std::vector<ResearchProject*>& Base::getResearch() const
{
	return _research;
}

/**
 * Add a new ResearchProject to Base
 * @param project - project to add
 */
void Base::addResearch(ResearchProject* project)
{
	_research.push_back(project);
}

/**
 * Remove a ResearchProject from base.
 * @param project	- a project to remove
 * @param grantHelp	- true to apply researchHelp() (default true)
 * @param goOffline	- true to hide project but not remove it from base's active ResearchProjects (default false)
 */
void Base::removeResearch(
		ResearchProject* project,
		bool grantHelp, // kL
		bool goOffline) // kL
{
	//Log(LOG_INFO) << "Base::removeResearch()";
	_scientists += project->getAssigned();

	std::vector<ResearchProject*>::const_iterator
			i = std::find(
						_research.begin(),
						_research.end(),
						project);
	if (i != _research.end())
	{
		if (goOffline)
		{
			project->setAssigned(0);
			project->setOffline();
		}
		else
		{
			// kL_begin: Add Research Help here. aLien must be interrogated
			// at the same Base that project-help applies to, for now.
			if (grantHelp)
			{
				//Log(LOG_INFO) << ". . aLien = " << project->getRules()->getName();
				// eg. Base::removeResearch() aLien = STR_REAPER_CORPSE

				researchHelp(project->getRules()->getName());
			} // kL_end.

			_research.erase(i);
		}
	}
}

/**
 * kL. Research Help ala XcomUtil.
 * @param aLien - name of the alien that got prodded
 */
void Base::researchHelp(const std::string& aLien) // kL
{
	std::string rp;
	double factor = 0.;

	for (std::vector<ResearchProject*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getOffline())
			continue;

		rp = (*i)->getRules()->getName();

		if (aLien.find("_SOLDIER") != std::string::npos)
		{
			if (rp.compare("STR_ALIEN_GRENADE") == 0
				|| rp.compare("STR_ALIEN_ENTERTAINMENT") == 0
				|| rp.compare("STR_PERSONAL_ARMOR") == 0)
			{
				factor = 0.5;
			}
			else if (rp.compare("STR_HEAVY_PLASMA_CLIP") == 0
				|| rp.compare("STR_PLASMA_RIFLE_CLIP") == 0
				|| rp.compare("STR_PLASMA_PISTOL_CLIP") == 0)
			{
				factor = 0.4;
			}
			else if (rp.compare("STR_POWER_SUIT") == 0)
			{
				factor = 0.25;
			}
			else if (rp.compare("STR_ALIEN_ORIGINS") == 0)
			{
				factor = 0.2;
			}
			else if (rp.compare("STR_THE_MARTIAN_SOLUTION") == 0
				|| rp.compare("STR_HEAVY_PLASMA") == 0
				|| rp.compare("STR_PLASMA_RIFLE") == 0
				|| rp.compare("STR_PLASMA_PISTOL") == 0)
			{
				factor = 0.1;
			}
		}
		else if (aLien.find("_NAVIGATOR") != std::string::npos)
		{
			if (rp.compare("STR_HYPER_WAVE_DECODER") == 0
				|| rp.compare("STR_UFO_NAVIGATION") == 0)
			{
				factor = 0.8;
			}
			else if (rp.compare("STR_MOTION_SCANNER") == 0
				|| rp.compare("STR_ALIEN_ENTERTAINMENT") == 0)
			{
				factor = 0.5;
			}
			else if (rp.compare("STR_GRAV_SHIELD") == 0
				|| rp.compare("STR_ALIEN_ALLOYS") == 0)
			{
				factor = 0.4;
			}
			else if (rp.compare("STR_ALIEN_ORIGINS") == 0)
			{
				factor = 0.35;
			}
			else if (rp.compare("STR_FLYING_SUIT") == 0)
			{
				factor = 0.3;
			}
			else if (rp.compare("STR_UFO_POWER_SOURCE") == 0
				|| rp.compare("STR_UFO_CONSTRUCTION") == 0
				|| rp.compare("STR_THE_MARTIAN_SOLUTION") == 0)
			{
				factor = 0.25;
			}
			else if (rp == "STR_HEAVY_PLASMA"
				|| rp == "STR_HEAVY_PLASMA_CLIP"
				|| rp == "STR_PLASMA_RIFLE"
				|| rp == "STR_PLASMA_RIFLE_CLIP"
				|| rp == "STR_PLASMA_PISTOL"
				|| rp == "STR_PLASMA_PISTOL_CLIP"
				|| rp == "STR_NEW_FIGHTER_CRAFT"
				|| rp == "STR_NEW_FIGHTER_TRANSPORTER"
				|| rp == "STR_ULTIMATE_CRAFT"
				|| rp == "STR_PLASMA_CANNON"
				|| rp == "STR_FUSION_MISSILE")
//				|| rp == "hovertank-plasma" // <-
//				|| rp == "hovertank-fusion" // <-
			{
				factor = 0.2;
			}
			else if (rp.compare("STR_CYDONIA_OR_BUST") == 0
				|| rp.compare("STR_POWER_SUIT") == 0)
			{
				factor = 0.15;
			}
		}
		else if (aLien.find("_MEDIC") != std::string::npos)
		{
			if (rp.compare("STR_ALIEN_FOOD") == 0
				|| rp.compare("STR_ALIEN_SURGERY") == 0
				|| rp.compare("STR_EXAMINATION_ROOM") == 0
				|| rp.compare("STR_ALIEN_REPRODUCTION") == 0)
			{
				factor = 0.8;
			}
			else if (rp.compare("STR_PSI_AMP") == 0
				|| rp.compare("STR_SMALL_LAUNCHER") == 0
				|| rp.compare("STR_STUN_BOMB") == 0
				|| rp.compare("STR_MIND_PROBE") == 0
				|| rp.compare("STR_MIND_SHIELD") == 0
				|| rp.compare("STR_PSI_LAB") == 0)
			{
				factor = 0.6;
			}
			else if (rp.compare("STR_ETHEREAL_CORPSE") == 0
				|| rp.compare("STR_SECTOPOD_CORPSE") == 0
				|| rp.compare("STR_SECTOID_CORPSE") == 0
				|| rp.compare("STR_CYBERDISC_CORPSE") == 0
				|| rp.compare("STR_SNAKEMAN_CORPSE") == 0
				|| rp.compare("STR_CHRYSSALID_CORPSE") == 0
				|| rp.compare("STR_MUTON_CORPSE") == 0
				|| rp.compare("STR_SILACOID_CORPSE") == 0
				|| rp.compare("STR_CELATID_CORPSE") == 0
				|| rp.compare("STR_FLOATER_CORPSE") == 0
				|| rp.compare("STR_REAPER_CORPSE") == 0)
			{
				factor = 0.5;
			}
			else if (rp == "STR_MEDI_KIT")
			{
				factor = 0.4;
			}
			else if (rp.compare("STR_ALIEN_ORIGINS") == 0
				|| rp.compare("STR_ALIEN_ENTERTAINMENT") == 0)
			{
				factor = 0.2;
			}
			else if (rp.compare("STR_THE_MARTIAN_SOLUTION") == 0)
			{
				factor = 0.1;
			}
		}
		else if (aLien.find("_ENGINEER") != std::string::npos)
		{
			if (rp.compare("STR_BLASTER_LAUNCHER") == 0
				|| rp.compare("STR_BLASTER_BOMB") == 0)
			{
				factor = 0.7;
			}
			else if (rp.compare("STR_MOTION_SCANNER") == 0
				|| rp.compare("STR_HEAVY_PLASMA") == 0
				|| rp.compare("STR_HEAVY_PLASMA_CLIP") == 0
				|| rp.compare("STR_PLASMA_RIFLE") == 0
				|| rp.compare("STR_PLASMA_RIFLE_CLIP") == 0
				|| rp.compare("STR_PLASMA_PISTOL") == 0
				|| rp.compare("STR_PLASMA_PISTOL_CLIP") == 0
				|| rp.compare("STR_ALIEN_GRENADE") == 0
				|| rp.compare("STR_ELERIUM_115") == 0
				|| rp.compare("STR_UFO_POWER_SOURCE") == 0
				|| rp.compare("STR_UFO_CONSTRUCTION") == 0
				|| rp.compare("STR_ALIEN_ALLOYS") == 0
				|| rp.compare("STR_PLASMA_CANNON") == 0
				|| rp.compare("STR_FUSION_MISSILE") == 0
				|| rp.compare("STR_PLASMA_DEFENSE") == 0
				|| rp.compare("STR_FUSION_DEFENSE") == 0
				|| rp.compare("STR_GRAV_SHIELD") == 0
				|| rp.compare("STR_PERSONAL_ARMOR") == 0
				|| rp.compare("STR_POWER_SUIT") == 0
				|| rp.compare("STR_FLYING_SUIT") == 0)
			{
				factor = 0.5;
			}
			else if (rp.compare("STR_NEW_FIGHTER_CRAFT") == 0
				|| rp.compare("STR_NEW_FIGHTER_TRANSPORTER") == 0
				|| rp.compare("STR_ULTIMATE_CRAFT") == 0)
			{
				factor = 0.3;
			}
			else if (rp.compare("STR_ALIEN_ORIGINS") == 0
				|| rp.compare("STR_SMALL_LAUNCHER") == 0
				|| rp.compare("STR_STUN_BOMB") == 0)
			{
				factor = 0.2;
			}
			else if (rp.compare("STR_THE_MARTIAN_SOLUTION") == 0)
			{
				factor = 0.1;
			}
		}
		else if (aLien.find("_LEADER") != std::string::npos)
		{
			if (rp.compare("STR_EXAMINATION_ROOM") == 0)
			{
				factor = 0.8;
			}
			else if (rp.compare("STR_BLASTER_LAUNCHER") == 0)
			{
				factor = 0.6;
			}
			else if (rp.compare("STR_ALIEN_ORIGINS") == 0)
			{
				factor = 0.5;
			}
			else if (rp.compare("STR_THE_MARTIAN_SOLUTION") == 0)
			{
				factor = 0.3;
			}
			else if (rp.compare("STR_PSI_AMP") == 0)
			{
				factor = 0.25;
			}
			else if (rp.compare("STR_HEAVY_PLASMA") == 0
				|| rp.compare("STR_HEAVY_PLASMA_CLIP") == 0
				|| rp.compare("STR_PLASMA_RIFLE") == 0
				|| rp.compare("STR_PLASMA_RIFLE_CLIP") == 0
				|| rp.compare("STR_PLASMA_PISTOL") == 0
				|| rp.compare("STR_PLASMA_PISTOL_CLIP") == 0
				|| rp.compare("STR_BLASTER_BOMB") == 0
				|| rp.compare("STR_SMALL_LAUNCHER") == 0
				|| rp.compare("STR_STUN_BOMB") == 0
				|| rp.compare("STR_ELERIUM_115") == 0
				|| rp.compare("STR_ALIEN_ALLOYS") == 0
				|| rp.compare("STR_PLASMA_CANNON") == 0
				|| rp.compare("STR_FUSION_MISSILE") == 0
				|| rp.compare("STR_CYDONIA_OR_BUST") == 0
				|| rp.compare("STR_PERSONAL_ARMOR") == 0
				|| rp.compare("STR_POWER_SUIT") == 0
				|| rp.compare("STR_FLYING_SUIT") == 0)
			{
				factor = 0.2;
			}
			else if (rp.compare("STR_NEW_FIGHTER_CRAFT") == 0
				|| rp.compare("STR_NEW_FIGHTER_TRANSPORTER") == 0
				|| rp.compare("STR_ULTIMATE_CRAFT") == 0)
			{
				factor = 0.1;
			}
		}
		else if (aLien.find("_COMMANDER") != std::string::npos)
		{
			if (rp.compare("STR_BLASTER_LAUNCHER") == 0
				|| rp.compare("STR_EXAMINATION_ROOM") == 0)
			{
				factor = 0.8;
			}
			else if (rp.compare("STR_ALIEN_ORIGINS") == 0)
			{
				factor = 0.7;
			}
			else if (rp.compare("STR_THE_MARTIAN_SOLUTION") == 0)
			{
				factor = 0.6;
			}
			else if (rp.compare("STR_PSI_AMP") == 0
				|| rp.compare("STR_CYDONIA_OR_BUST") == 0)
			{
				factor = 0.5;
			}
			else if (rp.compare("STR_BLASTER_BOMB") == 0
					|| rp.compare("STR_ELERIUM_115") == 0
					|| rp.compare("STR_ALIEN_ALLOYS") == 0
					|| rp.compare("STR_PERSONAL_ARMOR") == 0
					|| rp.compare("STR_POWER_SUIT") == 0
					|| rp.compare("STR_FLYING_SUIT") == 0)
			{
				factor = 0.25;
			}
			else if (rp.compare("STR_HEAVY_PLASMA") == 0
				|| rp.compare("STR_HEAVY_PLASMA_CLIP") == 0
				|| rp.compare("STR_PLASMA_RIFLE") == 0
				|| rp.compare("STR_PLASMA_RIFLE_CLIP") == 0
				|| rp.compare("STR_PLASMA_PISTOL") == 0
				|| rp.compare("STR_PLASMA_PISTOL_CLIP") == 0
				|| rp.compare("STR_SMALL_LAUNCHER") == 0
				|| rp.compare("STR_STUN_BOMB") == 0
				|| rp.compare("STR_NEW_FIGHTER_CRAFT") == 0
				|| rp.compare("STR_NEW_FIGHTER_TRANSPORTER") == 0
				|| rp.compare("STR_ULTIMATE_CRAFT") == 0
				|| rp.compare("STR_PLASMA_CANNON") == 0
				|| rp.compare("STR_FUSION_MISSILE") == 0)
			{
				factor = 0.2;
			}
		}

		if (AreSame(factor, 0.) == false)
		{
			const int cost = (*i)->getCost();
			const double spent = static_cast<double>((*i)->getSpent());

			(*i)->setSpent(static_cast<int>(Round(
							spent + ((static_cast<double>(cost) - spent) * factor))));

			if ((*i)->getSpent() > cost - 1)
				(*i)->setSpent(cost - 1);

			break;
		}
	}
}

/**
 * Returns whether or not a base is equipped with hyper-wave detection facilities.
 * @return, true if this base has hyper-wave detection
 */
bool Base::getHyperDetection() const
{
	for (std::vector<BaseFacility*>::const_iterator
			f = _facilities.begin();
			f != _facilities.end();
			++f)
	{
		if ((*f)->getBuildTime() == 0
			&& (*f)->getRules()->isHyperwave())
		{
			return true;
		}
	}

	return false;
}

/**
 * Returns the total amount of PsiLab Space available in the base.
 * @return, PsiLab space
 */
int Base::getAvailablePsiLabs() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			f = _facilities.begin();
			f != _facilities.end();
			++f)
	{
		if ((*f)->getBuildTime() == 0)
			total += (*f)->getRules()->getPsiLaboratories();
	}

	return total;
}

/**
 * Returns the total amount of used PsiLab Space in the base.
 * @return, used PsiLab space
 */
int Base::getUsedPsiLabs() const
{
	int total = 0;

	for (std::vector<Soldier*>::const_iterator
			s = _soldiers.begin();
			s != _soldiers.end();
			++s)
	{
		if ((*s)->isInPsiTraining())
			++total;
	}

	return total;
}

/**
 * Returns the total amount of used containment space in this Base.
 * @return, containment space
 */
int Base::getUsedContainment() const
{
	int total = 0;

	for (std::map<std::string, int>::const_iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			++i)
	{
		if (_rules->getItem(i->first)->isAlien() == true)
			total += i->second;
	}

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM
			&& _rules->getItem((*i)->getItems())->isAlien() == true)
		{
			total += (*i)->getQuantity();
		}
	}

	if (Options::storageLimitsEnforced == true)
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end();
				++i)
		{
			const RuleResearch* const resRule = (*i)->getRules();
			if (resRule->needItem() == true
				&& _rules->getUnit(resRule->getName()) != NULL)
			{
				++total;
			}
		}
	}

	return total;
}

/**
 * Returns the total amount of containment space available in this Base.
 * @return, containment space
 */
int Base::getAvailableContainment() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getAliens();
	}

	return total;
}

/**
 * Returns this Base's battlescape status.
 * @return, true if Base is the battlescape
 */
bool Base::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Changes this Base's battlescape status.
 * @param inbattle - true if Base is the battlescape
 */
void Base::setInBattlescape(bool inBattle)
{
	_inBattlescape = inBattle;
}

/**
 * Marks this Base as a valid alien retaliation target.
 * @param mark - Mark (if @c true) or unmark (if @c false) the base (default true)
 */
void Base::setIsRetaliationTarget(bool mark)
{
	_retaliationTarget = mark;
}

/**
 * Gets this Base's retaliation status.
 * @return, true if Base is a valid target for alien retaliation
 */
bool Base::getIsRetaliationTarget() const
{
	return _retaliationTarget;
}


/**
 * Functor to check for mind shield capability.
 */
/* struct isMindShield
	:
		public std::unary_function<BaseFacility*, bool>
{
	/// Check isMindShield() for @a facility.
	bool operator()(const BaseFacility* facility) const;
}; */

/**
 * Only fully operational facilities are checked.
 * @param facility Pointer to the facility to check.
 * @return, If @a facility can act as a mind shield.
 */
/* bool isMindShield::operator()(const BaseFacility* facility) const
{
	if (facility->getBuildTime() != 0)
		return false; // Still building this

	return facility->getRules()->isMindShield();
} */


/**
 * Functor to check for completed facilities.
 */
/* struct isCompleted
	:
		public std::unary_function<BaseFacility*, bool>
{
	/// Check isCompleted() for @a facility.
	bool operator()(const BaseFacility* facility) const;
}; */

/**
 * Facilities are checked for construction completion.
 * @param, facility Pointer to the facility to check.
 * @return, If @a facility has completed construction.
 */
/* bool isCompleted::operator()(const BaseFacility* facility) const
{
	return facility->getBuildTime() == 0;
} */

/**
 * Calculates the chance for aLiens to detect this base.
 * Big bases without mindshields are easier to detect.
 * @param diff - the game's difficulty setting
 * @return, detection chance
 */
int Base::getDetectionChance(int diff) const
{
	//Log(LOG_INFO) << "Base::getDetectionChance()";
	int
		facQty = 0,
		shields = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			++facQty;

			if ((*i)->getRules()->isMindShield() == true)
				++shields;
		}
	}

	facQty = facQty / 6 + 9;
	shields = shields * 2 + 1;
	//Log(LOG_INFO) << ". facQty = " << facQty;
	//Log(LOG_INFO) << ". shields = " << shields;
	//Log(LOG_INFO) << ". diff = " << diff;

	//int detchance = facQty / shields + diff;
	//Log(LOG_INFO) << ". detchance = " << detchance;

	return facQty / shields + diff;
}

/**
 * Gets the number of gravShields at this base.
 * @return, total gravShields
 */
int Base::getGravShields() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->isGravShield() == true)
		{
			++total;
		}
	}

	return total;
}

/**
 * Sets up this Base's defenses against UFO attacks.
 */
void Base::setupDefenses()
{
	_defenses.clear();

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getDefenseValue() > 0)
		{
			_defenses.push_back(*i);
		}
	}

	for (std::vector<Craft*>::const_iterator
			i = getCrafts()->begin();
			i != getCrafts()->end();
			++i)
		for (std::vector<Vehicle*>::const_iterator
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
			for (std::vector<Vehicle*>::const_iterator
					k = _vehicles.begin();
					k != _vehicles.end();
					++k)
			{
				if (*k == *j) // to avoid calling a Vehicle's destructor for tanks on crafts
				{
					_vehicles.erase(k);
					break;
				}
			}

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			)
	{
		delete *i;
		i = _vehicles.erase(i);
	}

	for (std::vector<Craft*>::const_iterator // add vehicles that are in the crafts of the base, if it's not out
			i = getCrafts()->begin();
			i != getCrafts()->end();
			++i)
	{
		if ((*i)->getStatus() != "STR_OUT")
		{
			for (std::vector<Vehicle*>::const_iterator
					j = (*i)->getVehicles()->begin();
					j != (*i)->getVehicles()->end();
					++j)
			{
				_vehicles.push_back(*j);
			}
		}
	}

	for (std::map<std::string, int>::const_iterator // add vehicles left on the base
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			)
	{
		const std::string itemId = (i)->first;
		const int itemQty = (i)->second;

		RuleItem* const itemRule = _rules->getItem(itemId);
		if (itemRule->isFixed() == true)
		{
			int unitSize = 4;
			if (_rules->getUnit(itemId))
				unitSize = _rules->getArmor(_rules->getUnit(itemId)->getArmor())->getSize();

			if (itemRule->getCompatibleAmmo()->empty() == true) // so this vehicle does not need ammo
			{
				for (int
						j = 0;
						j < itemQty;
						++j)
				{
					_vehicles.push_back(new Vehicle(
												itemRule,
												itemRule->getClipSize(),
												unitSize));
				}

				_items->removeItem(
								itemId,
								itemQty);
			}
			else // this vehicle needs ammo
			{
				const RuleItem* const ammoRule = _rules->getItem(itemRule->getCompatibleAmmo()->front());
				const int ammoPerVehicle = ammoRule->getClipSize();

				const int baseQty = _items->getItem(ammoRule->getType()) / ammoPerVehicle;
				if (baseQty == 0)
				{
					++i;
					continue;
				}

				const int canBeAdded = std::min(
											itemQty,
											baseQty);
				for (int
						j = 0;
						j < canBeAdded;
						++j)
				{
					_vehicles.push_back(new Vehicle(
												itemRule,
												ammoPerVehicle,
												unitSize));
					_items->removeItem(
									ammoRule->getType(),
									ammoPerVehicle);
				}

				_items->removeItem(
								itemId,
								canBeAdded);
			}

			i = _items->getContents()->begin(); // we have to start over because iterator is broken because of the removeItem
		}
		else
			++i;
	}
}

/**
 * Gets the total value of this Base's defenses.
 * @return, pointer to a vector of pointers to BaseFacility
 */
std::vector<BaseFacility*>* Base::getDefenses()
{
	return &_defenses;
}

/**
 * Returns the list of vehicles currently equipped in the base.
 * @return, pointer to a vector of pointers to Vehicle
 */
std::vector<Vehicle*>* Base::getVehicles()
{
	return &_vehicles;
}

/**
 * Destroys all disconnected facilities in the base.
 */
void Base::destroyDisconnectedFacilities()
{
	std::list<std::vector<BaseFacility*>::const_iterator> discFacs = getDisconnectedFacilities(NULL);
	for (std::list<std::vector<BaseFacility*>::const_iterator>::const_reverse_iterator
			i = discFacs.rbegin();
			i != discFacs.rend();
			++i)
	{
		destroyFacility(*i);
	}
}

/**
 * Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
 * @param remFac - BaseFacility to ignore (in case of purposeful dismantling)
 * @return, a sorted list of iterators pointing to elements in '_facilities'
 */
std::list<std::vector<BaseFacility*>::const_iterator> Base::getDisconnectedFacilities(BaseFacility* remFac)
{
	std::list<std::vector<BaseFacility*>::const_iterator> ret;

	if (remFac != NULL
		&& remFac->getRules()->isLift() == true) // Theoretically this is impossible, but sanity check is good :)
	{
		for (std::vector<BaseFacility*>::const_iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			if (*i != remFac)
				ret.push_back(i);
		}

		return ret;
	}

	std::vector<std::pair<std::vector<BaseFacility*>::const_iterator, bool>* > facConnStates;
	std::pair<std::vector<BaseFacility*>::const_iterator, bool>* grid[BASE_SIZE][BASE_SIZE];
	const BaseFacility* lift = NULL;

	for (int
			x = 0;
			x < BASE_SIZE;
			++x)
	{
		for (int
				y = 0;
				y < BASE_SIZE;
				++y)
		{
			grid[x][y] = 0;
		}
	}

	for (std::vector<BaseFacility*>::const_iterator // fill up the grid(+facConnStates), and search the lift
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if (*i != remFac)
		{
			if ((*i)->getRules()->isLift())
				lift = *i;

			for (int
					x = 0;
					x != (*i)->getRules()->getSize();
					++x)
			{
				for (int
						y = 0;
						y != (*i)->getRules()->getSize();
						++y)
				{
					std::pair<std::vector<BaseFacility*>::const_iterator, bool>* p =
							new std::pair<std::vector<BaseFacility*>::const_iterator, bool>(
																						i,
																						false);

					facConnStates.push_back(p);
					grid[(*i)->getX() + x][(*i)->getY() + y] = p;
				}
			}
		}
	}

	// we're in real trouble if this happens...
	if (lift == NULL)
		return ret; // TODO: something clever.


	// Now make the recursion manually using a stack
	std::stack<std::pair<int, int> > stack;
	stack.push(std::make_pair(
							lift->getX(),
							lift->getY()));
	while (stack.empty() == false)
	{
		int
			x = stack.top().first,
			y = stack.top().second;

		stack.pop();

		if (x > -1
			&& x < BASE_SIZE
			&& y > -1
			&& y < BASE_SIZE
			&& grid[x][y] != 0
			&& !grid[x][y]->second)
		{
			grid[x][y]->second = true;

			BaseFacility
				* fac = *(grid[x][y]->first),

				* neighborLeft = (x - 1 > -1 && grid[x - 1][y] != 0)? *(grid[x - 1][y]->first): 0,
				* neighborRight = (x + 1 < BASE_SIZE && grid[x + 1][y] != 0)? *(grid[x + 1][y]->first): 0,

				* neighborTop = (y - 1 > -1 && grid[x][y - 1] != 0)? *(grid[x][y - 1]->first): 0,
				* neighborBottom = (y + 1 < BASE_SIZE && grid[x][y + 1] != 0)? *(grid[x][y + 1]->first): 0;

			if (fac->getBuildTime() == 0
				|| (neighborLeft != 0
					&& (neighborLeft == fac
						|| neighborLeft->getBuildTime() > neighborLeft->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x - 1, y));
			}

			if (fac->getBuildTime() == 0
				|| (neighborRight != 0
					&& (neighborRight == fac
						|| neighborRight->getBuildTime() > neighborRight->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x + 1, y));
			}

			if (fac->getBuildTime() == 0
				|| (neighborTop != 0
					&& (neighborTop == fac
						|| neighborTop->getBuildTime() > neighborTop->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x, y - 1));
			}

			if (fac->getBuildTime() == 0
				|| (neighborBottom != 0
					&& (neighborBottom == fac
						|| neighborBottom->getBuildTime() > neighborBottom->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x, y + 1));
			}
		}
	}

	BaseFacility* lastFac = NULL;
	for (std::vector<std::pair<std::vector<BaseFacility*>::const_iterator, bool>* >::const_iterator
			i = facConnStates.begin();
			i != facConnStates.end();
			++i)
	{
		// Not a connected fac? -> push its iterator into the list!
		// Oh, and we don't want duplicates (facilities with bigger sizes like hangar)
		if (*((*i)->first) != lastFac
			&& (*i)->second == false)
		{
			ret.push_back((*i)->first);
		}

		lastFac = *((*i)->first);
		delete *i; // don't need the pair anymore.
	}

	return ret;
}

/**
 * Removes a base module, and deals with the ramifications thereof.
 * @param fac - an iterator reference to the facility to destroy and remove
 */
void Base::destroyFacility(std::vector<BaseFacility*>::const_iterator fac)
{
	if ((*fac)->getRules()->getCrafts() > 0) // hangar destruction
	{
		// destroy crafts and any production of crafts
		// as this will mean there is no hangar to contain it
		if ((*fac)->getCraft() != NULL)
		{
			if ((*fac)->getCraft()->getNumSoldiers() > 0) // remove all soldiers
			{
				for (std::vector<Soldier*>::const_iterator
						i = _soldiers.begin();
						i != _soldiers.end();
						++i)
				{
					if ((*i)->getCraft() == (*fac)->getCraft())
						(*i)->setCraft(NULL);
				}
			}

			while ((*fac)->getCraft()->getItems()->getContents()->empty() == false) // remove all items
			{
				const std::map<std::string, int>::const_iterator i = (*fac)->getCraft()->getItems()->getContents()->begin();
				_items->addItem(
							i->first,
							i->second);
				(*fac)->getCraft()->getItems()->removeItem(
														i->first,
														i->second);
			}

			for (std::vector<Craft*>::const_iterator
					i = _crafts.begin();
					i != _crafts.end();
					++i)
			{
				if (*i == (*fac)->getCraft())
				{
					delete (*i);
					_crafts.erase(i);

					break;
				}
			}
		}
		else // no craft
		{
			bool rem = true;

			for (std::vector<Production*>::const_iterator // check productions
					i = _productions.begin();
					i != _productions.end();
					)
			{
				if ((*i)->getRules()->getCategory() == "STR_CRAFT"
					&& getAvailableHangars() - getUsedHangars() - (*fac)->getRules()->getCrafts() < 0)
				{
					rem = false;
					_engineers += (*i)->getAssignedEngineers();

					delete *i;
					_productions.erase(i);

					break;
				}
				else
					++i;
			}

			if (rem == true
				&& _transfers.empty() == false) // check transfers
			{
				for (std::vector<Transfer*>::const_iterator
						i = _transfers.begin();
						i != _transfers.end();
						)
				{
					if ((*i)->getType() == TRANSFER_CRAFT)
					{
						delete (*i)->getCraft();
						delete *i;
						_transfers.erase(i);

						break;
					}
				}
			}
		}
	}
	else if ((*fac)->getRules()->getPsiLaboratories() > 0)
	{
		// psi lab destruction: remove any soldiers over the maximum allowable from psi training.
		int toRemove = (*fac)->getRules()->getPsiLaboratories() - getFreePsiLabs();

		for (std::vector<Soldier*>::const_iterator
				i = _soldiers.begin();
				i != _soldiers.end()
					&& toRemove > 0;
				++i)
		{
			if ((*i)->isInPsiTraining())
			{
				(*i)->setPsiTraining();

				--toRemove;
			}
		}
	}
	else if ((*fac)->getRules()->getLaboratories() > 0)
	{
		// lab destruction: enforce lab space limits.
		// take scientists off projects until it all evens out.
		// research is not cancelled.
		int toRemove = (*fac)->getRules()->getLaboratories() - getFreeLaboratories();

		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& toRemove > 0;
				)
		{
			if ((*i)->getAssigned() >= toRemove)
			{
				(*i)->setAssigned((*i)->getAssigned() - toRemove);
				_scientists += toRemove;

				break;
			}
			else
			{
				toRemove -= (*i)->getAssigned();
				_scientists += (*i)->getAssigned();
				(*i)->setAssigned(0);

				++i;

//				delete *i;
//				i = _research.erase(i);
			}
		}
	}
	else if ((*fac)->getRules()->getWorkshops() > 0)
	{
		// workshop destruction: similar to lab destruction, but we'll lay off engineers instead. kL_note: huh!!!!
		// in this case, however, production IS cancelled, as it takes up space in the workshop.
		int toRemove = (*fac)->getRules()->getWorkshops() - getFreeWorkshops();
//		int toRemove = getUsedWorkshops() - (getAvailableWorkshops() - (*fac)->getRules()->getWorkshops());
		for (std::vector<Production*>::const_iterator
				i = _productions.begin();
				i != _productions.end()
					&& toRemove > 0;
				)
		{
			if ((*i)->getAssignedEngineers() > toRemove)
			{
				(*i)->setAssignedEngineers((*i)->getAssignedEngineers() - toRemove);
				_engineers += toRemove;

				break;
			}
			else
			{
				toRemove -= (*i)->getAssignedEngineers();
				_engineers += (*i)->getAssignedEngineers();

				delete *i;
				i = _productions.erase(i);
			}
		}
	}
	else if ((*fac)->getRules()->getStorage() > 0)
	{
		// we won't destroy the items physically AT the base,
		// but any items in transit will end up at the dead letter office.
		if (_transfers.empty() == false
			&& storesOverfull((*fac)->getRules()->getStorage()) == true)
		{
			for (std::vector<Transfer*>::const_iterator
					i = _transfers.begin();
					i != _transfers.end();
					)
			{
				if ((*i)->getType() == TRANSFER_ITEM)
				{
					delete *i;
					i = _transfers.erase(i);
				}
				else
					++i;
			}
		}
	}
	else if ((*fac)->getRules()->getPersonnel() > 0)
	{
		// as above, we won't actually fire people, but we'll block any new ones coming in.
		if (_transfers.empty() == false
			&& getAvailableQuarters() - getUsedQuarters() - (*fac)->getRules()->getPersonnel() < 0)
		{
			for (std::vector<Transfer*>::const_iterator
					i = _transfers.begin();
					i != _transfers.end();
					)
			{
				// let soldiers arrive, but block workers.
				if ((*i)->getType() == TRANSFER_ENGINEER
					|| (*i)->getType() == TRANSFER_SCIENTIST)
/*				bool del = false;

				if ((*i)->getType() == TRANSFER_ENGINEER)
				{
					del = true;
					_engineers -= (*i)->getQuantity();
				}
				else if ((*i)->getType() == TRANSFER_SCIENTIST)
				{
					del = true;
					_scientists -= (*i)->getQuantity();
				}
				else if ((*i)->getType() == TRANSFER_SOLDIER)
				{
					del = true;
				}

				if (del) */
				{
					delete *i;
					i = _transfers.erase(i);
				}
				else
					++i;
			}
		}
	}

	delete *fac;
	_facilities.erase(fac);
}

/**
 * Cleans up base defenses vector after a Ufo attack
 * and optionally reclaims the tanks and their ammo.
 * @param reclaimItems - true to return the HWPs to storage
 */
void Base::cleanupDefenses(bool reclaimItems)
{
	_defenses.clear();

	for (std::vector<Craft*>::const_iterator
			i = getCrafts()->begin();
			i != getCrafts()->end();
			++i)
		for (std::vector<Vehicle*>::const_iterator
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
			for (std::vector<Vehicle*>::const_iterator
					k = _vehicles.begin();
					k != _vehicles.end();
					++k)
			{
				if (*k == *j) // to avoid calling a vehicle's destructor for tanks on crafts
				{
					_vehicles.erase(k);
					break;
				}
			}

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			)
	{
		if (reclaimItems == true)
		{
			RuleItem* const rule = (*i)->getRules();
			std::string type = rule->getType();
			_items->addItem(type);
			if (rule->getCompatibleAmmo()->empty() == false)
			{
				const RuleItem* const ammo = _rules->getItem(rule->getCompatibleAmmo()->front());
				const int ammoPerVehicle = ammo->getClipSize();
				_items->addItem(
							ammo->getType(),
							ammoPerVehicle);
			}
		}

		delete *i;
		i = _vehicles.erase(i);
	}
}

/**
 * Increase the Base's cash income amount by 'cash'.
 * @param cash, The amount to increase _cashIncome by
 */
void Base::setCashIncome(int cash)
{
	_cashIncome += cash;
}

/**
 * Get the Base's current cash income value.
 * @return, The Base's current _cashIncome amount
 */
int Base::getCashIncome() const
{
	return _cashIncome;
}

/**
 * Increase the Base's cash spent amount by 'cash'.
 * @param cash, The amount to increase _cashSpent by
 */
void Base::setCashSpent(int cash)
{
	_cashSpent += cash;
}

/**
 * Get the Base's current cash spent value.
 * @return, The Base's current _cashSpent amount
 */
int Base::getCashSpent() const
{
	return _cashSpent;
}

/**
 * Sets the effect of a base's defense against aLien attacks.
 */
void Base::setDefenseResult(int result)
{
	_defenseResult = result;
}

/**
 * Gets the effect of a base's defense against aLien attacks.
 */
int Base::getDefenseResult() const
{
	return _defenseResult;
}

/**
 * Sets the current row of Soldiers list.
 */
void Base::setCurrentSoldier(const size_t row)
{
	_curSoldier = row;
}

/**
 * Gets the current row of Soldiers list.
 */
size_t Base::getCurrentSoldier() const
{
	return _curSoldier;
}

}
