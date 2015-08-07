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

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h" // Globe::GLM_BASE

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


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
		_inTactical(false),
		_exposed(false),
		_cashIncome(0),
		_cashSpent(0),
		_defenseResult(0),
		_curSoldier(0)
{
	_items = new ItemContainer();
}

/**
 * Deletes the contents of this Base from memory.
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
		|| Options::customInitialBase == false
		|| newBattleGame == true)
	{
		for (YAML::const_iterator
				i = node["facilities"].begin();
				i != node["facilities"].end();
				++i)
		{
			const std::string type = (*i)["type"].as<std::string>();
			if (_rules->getBaseFacility(type))
			{
				BaseFacility* const f = new BaseFacility(
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
			Craft* const c = new Craft(
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
		Soldier* const sol = new Soldier(
									_rules->getSoldier("XCOM"),
									_rules->getArmor("STR_ARMOR_NONE_UC"));
		sol->load(
			*i,
			_rules);
//			save);
		sol->setCraft(NULL);

		if (const YAML::Node& craft = (*i)["craft"])
		{
			const CraftId craftId = Craft::loadId(craft);
			for (std::vector<Craft*>::const_iterator
					j = _crafts.begin();
					j != _crafts.end();
					++j)
			{
				if ((*j)->getUniqueId() == craftId)
				{
					sol->setCraft(*j);
					break;
				}
			}
		}

		_soldiers.push_back(sol);
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

	_scientists	= node["scientists"].as<int>(_scientists);
	_engineers	= node["engineers"]	.as<int>(_engineers);
	_inTactical	= node["inTactical"].as<bool>(_inTactical);
	_cashIncome	= node["cashIncome"].as<int>(_cashIncome);
	_cashSpent	= node["cashSpent"]	.as<int>(_cashSpent);

	for (YAML::const_iterator
			i = node["transfers"].begin();
			i != node["transfers"].end();
			++i)
	{
		int hours = (*i)["hours"].as<int>();
		Transfer* const t = new Transfer(hours);
		if (t->load(
				*i,
				this,
				_rules) == true)
		{
			_transfers.push_back(t);
		}
	}

	for (YAML::const_iterator
			i = node["research"].begin();
			i != node["research"].end();
			++i)
	{
		const std::string research = (*i)["project"].as<std::string>();
		if (_rules->getResearch(research))
		{
			ResearchProject* const r = new ResearchProject(_rules->getResearch(research));
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
		const std::string item = (*i)["item"].as<std::string>();
		if (_rules->getManufacture(item))
		{
			Production* const p = new Production(
											_rules->getManufacture(item),
											0);
			p->load(*i);
			_productions.push_back(p);
		}
		else
			_engineers += (*i)["assigned"].as<int>(0);
	}

	_exposed = node["exposed"].as<bool>(_exposed);
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

	node["items"]		= _items->save();
	node["scientists"]	= _scientists;
	node["engineers"]	= _engineers;
	node["inTactical"]	= _inTactical;
	node["cashIncome"]	= _cashIncome;
	node["cashSpent"]	= _cashSpent;

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

	node["exposed"] = _exposed;

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
 * @param lang - pointer to Language to get strings from (default NULL)
 * @return, name string
 */
std::wstring Base::getName(const Language* const) const
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
 * @return, marker sprite #0 (-1 if none)
 */
int Base::getMarker() const
{
	// cheap hack to hide Bases when they haven't been placed yet
	if (AreSame(_lon, 0.) && AreSame(_lat, 0.))
		return -1;

	return Globe::GLM_BASE;
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
 * Returns the amount of scientists currently in this Base.
 * @return, number of scientists
 */
int Base::getScientists() const
{
	return _scientists;
}

/**
 * Changes the amount of scientists currently in this Base.
 * @param scientists - number of scientists
 */
void Base::setScientists(int scientists)
{
	 _scientists = scientists;
}

/**
 * Returns the amount of engineers currently in this Base.
 * @return, number of engineers
 */
int Base::getEngineers() const
{
	return _engineers;
}

/**
 * Changes the amount of engineers currently in this Base.
 * @param engineers - number of engineers
 */
void Base::setEngineers(int engineers)
{
	 _engineers = engineers;
}

/**
 * Returns if a certain Target is covered by this Base's radar range taking into
 * account the range and chance.
 * @param target - pointer to a UFO to attempt detection against
 * @return,	0 undetected
 *			1 hyperdetected only
 *			2 detected
 *			3 detected & hyperdetected
 */
int Base::detect(Target* const target) const
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

	int pct = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			const double radarRange = static_cast<double>((*i)->getRules()->getRadarRange()) * greatCircleConversionFactor;
			if (radarRange > targetDist)
				pct += (*i)->getRules()->getRadarChance();
		}
	}

	const Ufo* const ufo = dynamic_cast<Ufo*>(target);
	if (ufo != NULL)
	{
		pct += ufo->getVisibility();
		pct = static_cast<int>(Round(static_cast<double>(pct) / 3.)); // per 10-min.

		if (RNG::percent(pct) == true)
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
double Base::insideRadarRange(const Target* const target) const
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
					&& (*i)->getRecovery() == 0)))
		{
			++total;
		}
	}

	return total;
}

/**
 * Returns the total amount of soldiers contained in this Base.
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
 * Returns the amount of scientists contained in this Base without any assignments.
 * @return, number of scientists
 */
/* int Base::getAvailableScientists() const
{
	return getScientists();
} */

/**
 * Returns the total amount of scientists contained in this Base.
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
 * Returns the amount of engineers contained in this Base without any assignments.
 * @return, number of engineers
 */
/* int Base::getAvailableEngineers() const
{
	return getEngineers();
} */

/**
 * Returns the total amount of engineers contained in this Base.
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
 * Returns the amount of living quarters used up by personnel in this Base.
 * @return, personnel space
 */
int Base::getUsedQuarters() const
{
	return getTotalSoldiers() + getTotalScientists() + getTotalEngineers();
}

/**
 * Returns the total amount of living quarters available in this Base.
 * @return, personnel space
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
 * Returns the amount of storage space used by equipment in this Base.
 * @note This includes equipment on Craft and about to arrive in Transfers as
 * well as any armor that this Base's Soldiers are currently wearing.
 * @return, storage space
 */
double Base::getUsedStores() const
{
	double total = _items->getTotalSize(_rules); // items

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		total += (*i)->getItems()->getTotalSize(_rules); // craft items

		for (std::vector<Vehicle*>::const_iterator // craft vehicles (vehicles are items if not on a craft)
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
		{
			total += (*j)->getRules()->getSize();

			if ((*j)->getRules()->getCompatibleAmmo()->empty() == false) // craft vehicle ammo
			{
				const RuleItem
					* const vhclRule = _rules->getItem((*j)->getRules()->getType()),
					* const ammoRule = _rules->getItem(vhclRule->getCompatibleAmmo()->front());

				total += ammoRule->getSize() * (*j)->getAmmo();
			}
		}
	}

	for (std::vector<Transfer*>::const_iterator // transfers
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM)
		{
			total += _rules->getItem((*i)->getItems())->getSize()
				   * static_cast<double>((*i)->getQuantity());
		}
	}
/*		else if ((*i)->getType() == TRANSFER_CRAFT)
		{
			Craft* const craft = (*i)->getCraft();
			total += craft->getItems()->getTotalSize(_rules);

			for (std::vector<Vehicle*>::const_iterator
					j = craft->getVehicles()->begin();
					j != craft->getVehicles()->end();
					++j)
			{
				total += (*j)->getRules()->getSize();

				if ((*j)->getRules()->getCompatibleAmmo()->empty() == false)
				{
					const RuleItem
						* const vhclRule = _rules->getItem((*j)->getRules()->getType()),
						* const ammoRule = _rules->getItem(vhclRule->getCompatibleAmmo()->front());

					total += ammoRule->getSize() * (*j)->getAmmo();
				}
			}
		} */

	for (std::vector<Soldier*>::const_iterator // soldier armor
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if ((*i)->getArmor()->isBasic() == false)
			total += _rules->getItem((*i)->getArmor()->getStoreItem())->getSize();
	}

//	total -= getIgnoredStores();
	return total;
}

/**
 * Checks if a base's stores are overfilled.
 * @note Supplying an offset will add/subtract to the used capacity before
 * performing the check. A positive offset simulates adding items to the stores
 * whereas a negative offset can be used to check whether sufficient items have
 * been removed to stop stores from overflowing.
 * @param offset - adjusts used capacity (default 0.)
 * @return, true if this Base's stores are over their limit
 */
bool Base::storesOverfull(double offset) const
{
	const double
		total = static_cast<double>(getAvailableStores()),
		used = getUsedStores();

	return (used + offset > total + 0.05);
}

/**
 * Returns the total amount of stores available in this Base.
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
 * Returns the amount of laboratories used up by research projects in this Base.
 * @return, laboratory space
 */
int Base::getUsedLaboratories() const
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
 * Returns the total amount of laboratories available in this Base.
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
 * Returns the amount of workshops used up by manufacturing projects in this Base.
 * @return, workshop space
 */
int Base::getUsedWorkshops() const
{
	int total = 0;

	for (std::vector<Production*>::const_iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		total += (*i)->getAssignedEngineers() + (*i)->getRules()->getRequiredSpace();
	}

	return total;
}

/**
 * Returns the total amount of workshops available in this Base.
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
 * Returns the amount of hangars used up by crafts in this Base.
 * @return, hangars
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
			total += ((*i)->getAmountTotal() - (*i)->getAmountProduced());
			// TODO: This should be fixed on the case when (*i)->getInfiniteAmount() == TRUE
	}

	return total;
}

/**
 * Returns the total amount of hangars available in this Base.
 * @return, hangars
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
 * Returns laboratories space not used by a ResearchProject.
 * @return, lab space not used by a ResearchProject
*/
int Base::getFreeLaboratories() const
{
	return getAvailableLaboratories() - getUsedLaboratories();
}

/**
 * Returns workshop space not used by a Production.
 * @return, workshop space not used by a Production
*/
int Base::getFreeWorkshops() const
{
	return getAvailableWorkshops() - getUsedWorkshops();
}

/**
 * Returns psilab space not in use.
 * @return, psilab space not in use
*/
int Base::getFreePsiLabs() const
{
	return getAvailablePsiLabs() - getUsedPsiLabs();
}

/**
 * Returns the amount of scientists currently in use.
 * @return, amount of scientists
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
 * @return, amount of engineers
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
 * Returns the total defense value of all the facilities in this Base.
 * @note Used for BaseInfoState bar.
 * @return, defense value
 */
int Base::getDefenseTotal() const
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
 * Returns the total amount of short range detection facilities in this Base.
 * @return, quantity of shortrange detection facilities
 */
/* int Base::getShortRangeDetection() const
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
 * Returns the total value of short range detection facilities at this base.
 * @note Used for BaseInfoState bar.
 * @return, shortrange detection value as percent
 */
int Base::getShortRangeTotal() const
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
 * Returns the total amount of long range detection facilities in this Base.
 * @return, quantity of longrange detection facilities
 */
/* int Base::getLongRangeDetection() const
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
 * Returns the total value of long range detection facilities at this base.
 * @note Used for BaseInfoState bar.
 * @return, longrange detection value as percent
 */
int Base::getLongRangeTotal() const
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
 * Returns the total amount of Craft of a certain type stored at or being
 * transfered to this Base.
 * @note Used by MonthlyCostsState.
 * @param craft - reference the craft type
 * @return, quantity of craft-type
 */
int Base::getCraftCount(const std::string& craft) const
{
	int total = 0;

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT
			&& (*i)->getCraft()->getRules()->getType() == craft)
		{
			total += (*i)->getQuantity();
		}
	}

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		if ((*i)->getRules()->getType() == craft)
			++total;
	}

	return total;
}

/**
 * Returns the total amount of monthly costs for maintaining the craft in this Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getCraftMaintenance() const
{
	int total = 0;

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT)
			total += (*i)->getQuantity() * (*i)->getCraft()->getRules()->getRentCost();
	}

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
 * Returns the total amount of monthly costs for maintaining the personnel in this Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getPersonnelMaintenance() const
{
	int total = 0;

	total += getTotalSoldiers() * _rules->getSoldierCost();
	total += getTotalEngineers() * _rules->getEngineerCost();
	total += getTotalScientists() * _rules->getScientistCost();
	total += calcSoldierBonuses();

	return total;
}

/**
 * Returns the total amount of monthly costs for maintaining the facilities in this Base.
 * @note Used for monthly maintenance expenditure.
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
 * Returns the total amount of all the maintenance monthly costs in this Base.
 * @note Used for monthly maintenance expenditure.
 * @return, maintenance costs
 */
int Base::getMonthlyMaintenace() const
{
	return getCraftMaintenance() + getPersonnelMaintenance() + getFacilityMaintenance();
}

/**
 * Adds a new Production to the Base
 * @param prod - pointer to a Production
 */
void Base::addProduction(Production* prod)
{
	_productions.push_back(prod);
}

/**
 * Removes a Production from the Base.
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
 * Gets the list of Base Productions.
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
 * Adds a new ResearchProject to Base
 * @param project - project to add
 */
void Base::addResearch(ResearchProject* const project)
{
	_research.push_back(project);
}

/**
 * Removes a ResearchProject from base.
 * @param project	- pointer to a ResearchProject for removal
 * @param grantHelp	- true to apply researchHelp() (default true)
 * @param goOffline	- true to hide project but not remove it from base's active ResearchProjects (default false)
 */
void Base::removeResearch(
		ResearchProject* const project,
		bool grantHelp,
		bool goOffline)
{
	_scientists += project->getAssigned();

	std::vector<ResearchProject*>::const_iterator
			i = std::find(
						_research.begin(),
						_research.end(),
						project);
	if (i != _research.end())
	{
		if (goOffline == true)
		{
			project->setAssigned(0);
			project->setOffline();
		}
		else
		{
			if (grantHelp == true)
				researchHelp(project->getRules()->getName());

			_research.erase(i);
		}
	}
}

/**
 * Research Help ala XcomUtil.
 * @param aLien - reference the name of the alien that got prodded
 */
void Base::researchHelp(const std::string& aLien)
{
	std::string rp;
	double factor;

	for (std::vector<ResearchProject*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		if ((*i)->getOffline() == false)
		{
			rp = (*i)->getRules()->getName();
			factor = 0.;

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
//					|| rp == "hovertank-plasma" // <-
//					|| rp == "hovertank-fusion" // <-
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

				return;
			}
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
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->isHyperwave() == true)
		{
			return true;
		}
	}

	return false;
}

/**
 * Returns whether or not a base is equipped with research facilities.
 * @return, true if this base is capable of research
 */
bool Base::hasResearch() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getLaboratories() > 0)
		{
			return true;
		}
	}

	return false;
}

/**
 * Returns whether or not a base is equipped with production facilities.
 * @return, true if this base is capable of production
 */
bool Base::hasProduction() const
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getWorkshops() > 0)
		{
			return true;
		}
	}

	return false;
}

/**
 * Returns the total amount of PsiLab Space available in this Base.
 * @return, psilab space
 */
int Base::getAvailablePsiLabs() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
			total += (*i)->getRules()->getPsiLaboratories();
	}

	return total;
}

/**
 * Returns the total amount of used PsiLab Space in this Base.
 * @return, used psilab space
 */
int Base::getUsedPsiLabs() const
{
	int total = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if ((*i)->isInPsiTraining() == true)
			++total;
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
		total += getInterrogatedAliens();

	return total;
}

/**
 * Gets the quantity of aLiens currently under interrogation.
 * @return, quantity of aliens
 */
int Base::getInterrogatedAliens() const
{
	int total = 0;
	const RuleResearch* resRule;

	for (std::vector<ResearchProject*>::const_iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		resRule = (*i)->getRules();
		if (resRule->needItem() == true
			&& _rules->getUnit(resRule->getName()) != NULL)
		{
			++total;
		}
	}

	return total;
}

/**
 * Returns this Base's battlescape status.
 * @return, true if Base is the battlescape
 */
bool Base::isInBattlescape() const
{
	return _inTactical;
}

/**
 * Changes this Base's battlescape status.
 * @param inbattle - true if in the battlescape
 */
void Base::setInBattlescape(bool inTactical)
{
	_inTactical = inTactical;
}

/**
 * Marks this Base as a valid alien retaliation target.
 * @param exposed - exposed (if @c true) or unexposed (if @c false) the base (default true)
 */
void Base::setBaseExposed(bool exposed)
{
	_exposed = exposed;
}

/**
 * Gets this Base's retaliation status.
 * @return, true if valid target for alien retaliation
 */
bool Base::getBaseExposed() const
{
	return _exposed;
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
 * @note Big bases without mindshields are easier to detect.
 * @param diff		- the game's difficulty setting
 * @param facQty	- pointer to the quantity of facilities (default NULL)
 * @param shields	- pointer to the quantity of shield facilities (default NULL)
 * @return, detection chance
 */
int Base::getDetectionChance(
		int diff,
		int* facQty,
		int* shields) const
{
	if (facQty != NULL)
	{
		*facQty =
		*shields = 0;

		for (std::vector<BaseFacility*>::const_iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			if ((*i)->getBuildTime() == 0)
			{
				++(*facQty);

				if ((*i)->getRules()->isMindShield() == true)
					++(*shields);
			}
		}

		return calcDetChance(
						diff,
						*facQty,
						*shields);
	}

	int
		facQty0 = 0,
		shields0 = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			++facQty0;

			if ((*i)->getRules()->isMindShield() == true)
				++shields0;
		}
	}

	return calcDetChance(
					diff,
					facQty0,
					shields0);
}

/**
 * Calculates the chance that aLiens have to detect this Base.
 * @note Helper for getDetectionChance() to ensure consistency.
 * @param diff		- the game's difficulty setting
 * @param facQty	- the quantity of facilities
 * @param shields	- the quantity of shield facilities
 */
int Base::calcDetChance( // private.
		int diff,
		int facQty,
		int shields) const
{
	return (facQty / 6 + 9) / (shields * 2 + 1) + diff;
}

/**
 * Gets the number of gravShields at this base.
 * @return, total gravShields
 */
size_t Base::getGravShields() const
{
	size_t total = 0;

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

				const int baseQty = _items->getItemQty(ammoRule->getType()) / ammoPerVehicle;
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
 * Returns the list of vehicles currently equipped in this Base.
 * @return, pointer to a vector of pointers to Vehicle
 */
std::vector<Vehicle*>* Base::getVehicles()
{
	return &_vehicles;
}

/**
 * Destroys all disconnected facilities in this Base.
 */
void Base::destroyDisconnectedFacilities()
{
	std::list<std::vector<BaseFacility*>::const_iterator> discoFacs = getDisconnectedFacilities(NULL);
	for (std::list<std::vector<BaseFacility*>::const_iterator>::const_reverse_iterator
			i = discoFacs.rbegin();
			i != discoFacs.rend();
			++i)
	{
		destroyFacility(*i);
	}
}

/**
 * Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
 * @param ignoreFac - BaseFacility to ignore (in case of purposeful dismantling)
 * @return, a sorted list of iterators pointing to elements in '_facilities'
 */
std::list<std::vector<BaseFacility*>::const_iterator> Base::getDisconnectedFacilities(BaseFacility* ignoreFac)
{
	std::list<std::vector<BaseFacility*>::const_iterator> ret;

	if (ignoreFac != NULL
		&& ignoreFac->getRules()->isLift() == true) // Theoretically this is impossible, but sanity check is good :)
	{
		for (std::vector<BaseFacility*>::const_iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
		{
			if (*i != ignoreFac)
				ret.push_back(i);
		}

		return ret;
	}


	std::pair<std::vector<BaseFacility*>::const_iterator, bool>* facBool_coord[BASE_SIZE][BASE_SIZE];
	for (size_t
			x = 0;
			x != BASE_SIZE;
			++x)
	{
		for (size_t
				y = 0;
				y != BASE_SIZE;
				++y)
		{
			facBool_coord[x][y] = NULL;
		}
	}

	const BaseFacility* lift = NULL;

	std::vector<std::pair<std::vector<BaseFacility*>::const_iterator, bool>* > facConnections;
	for (std::vector<BaseFacility*>::const_iterator // fill up the facBool_coord (+facConnections), and search for the Lift
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if (*i != ignoreFac)
		{
			if ((*i)->getRules()->isLift() == true)
				lift = *i;

			for (size_t
					x = 0;
					x != (*i)->getRules()->getSize();
					++x)
			{
				for (size_t
						y = 0;
						y != (*i)->getRules()->getSize();
						++y)
				{
					std::pair<std::vector<BaseFacility*>::const_iterator, bool>* pairsOfFacBool =
									new std::pair<std::vector<BaseFacility*>::const_iterator, bool>(
																								i,
																								false);
					facConnections.push_back(pairsOfFacBool);
					facBool_coord[static_cast<size_t>((*i)->getX()) + x]
								 [static_cast<size_t>((*i)->getY()) + y] = pairsOfFacBool; // loli
				}
			}
		}
	}

	if (lift == NULL)
		return ret; // TODO: something clever.


	// make the recursion manually using a stack
	const BaseFacility
		* fac,
		* borLeft,
		* borRight,
		* borTop,
		* borBottom;
	size_t
		x,y;
	std::stack<std::pair<size_t, size_t> > stuff;

	stuff.push(std::make_pair(
						static_cast<size_t>(lift->getX()),
						static_cast<size_t>(lift->getY())));
	while (stuff.empty() == false)
	{
		x = stuff.top().first,
		y = stuff.top().second;

		stuff.pop();

		if (//   x > -1			// -> hopefully x&y will never point outside the baseGrid ... looks atm like it does!! It does. FIX inc!!!!
			//&& x < BASE_SIZE
			//&& y > -1
			//&& y < BASE_SIZE &&
			   facBool_coord[x][y] != NULL
			&& facBool_coord[x][y]->second == false)
		{
			facBool_coord[x][y]->second = true;

			fac = *(facBool_coord[x][y]->first);

			if (x > 0
				&& facBool_coord[x - 1][y] != NULL)
			{
				borLeft = *(facBool_coord[x - 1][y]->first);
			}
			else
				borLeft = NULL;

			if (x + 1 < BASE_SIZE
				&& facBool_coord[x + 1][y] != NULL)
			{
				borRight = *(facBool_coord[x + 1][y]->first);
			}
			else
				borRight = NULL;

			if (y > 0
				&& facBool_coord[x][y - 1] != NULL)
			{
				borTop = *(facBool_coord[x][y - 1]->first);
			}
			else
				borTop = NULL;

			if (y + 1 < BASE_SIZE
				&& facBool_coord[x][y + 1] != NULL)
			{
				borBottom = *(facBool_coord[x][y + 1]->first);
			}
			else
				borBottom = NULL;


			if (x > 0
				&& (fac->getBuildTime() == 0
					|| (borLeft != NULL
						&& (borLeft == fac
							|| borLeft->getBuildTime() > borLeft->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(
										x - 1,
										y));
			}

			if (x < BASE_SIZE - 1
				&& (fac->getBuildTime() == 0
					|| (borRight != NULL
						&& (borRight == fac
							|| borRight->getBuildTime() > borRight->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(
										x + 1,
										y));
			}

			if (y > 0
				&& (fac->getBuildTime() == 0
					|| (borTop != NULL
						&& (borTop == fac
							|| borTop->getBuildTime() > borTop->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(
										x,
										y - 1));
			}

			if (y < BASE_SIZE - 1
				&& (fac->getBuildTime() == 0
					|| (borBottom != NULL
						&& (borBottom == fac
							|| borBottom->getBuildTime() > borBottom->getRules()->getBuildTime()))))
			{
				stuff.push(std::make_pair(
										x,
										y + 1));
			}
		}
	}

	const BaseFacility* preEntry = NULL;
	for (std::vector<std::pair<std::vector<BaseFacility*>::const_iterator, bool>* >::const_iterator
			i = facConnections.begin();
			i != facConnections.end();
			++i)
	{
		// not a connected fac -> push its iterator onto the list!
		// and don't take duplicates of large-sized facilities.
		if (*((*i)->first) != preEntry
			&& (*i)->second == false)
		{
			ret.push_back((*i)->first);
		}

		preEntry = *((*i)->first);
		delete *i;
	}

	return ret;
}

/**
 * Removes a base module and deals with the ramifications thereof.
 * @param fac - an iterator reference to the facility to destroy
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
			bool destroyCraft = true;

			for (std::vector<Production*>::const_iterator // check productions
					i = _productions.begin();
					i != _productions.end();
					)
			{
				if ((*i)->getRules()->getCategory() == "STR_CRAFT"
					&& getAvailableHangars() - getUsedHangars() - (*fac)->getRules()->getCrafts() < 0)
				{
					destroyCraft = false;
					_engineers += (*i)->getAssignedEngineers();

					delete *i;
					_productions.erase(i);

					break;
				}
				else
					++i;
			}

			if (destroyCraft == true
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
		int qty = (*fac)->getRules()->getPsiLaboratories() - getFreePsiLabs();

		for (std::vector<Soldier*>::const_iterator
				i = _soldiers.begin();
				i != _soldiers.end()
					&& qty > 0;
				++i)
		{
			if ((*i)->isInPsiTraining() == true)
			{
				(*i)->togglePsiTraining();
				--qty;
			}
		}
	}
	else if ((*fac)->getRules()->getLaboratories() > 0)
	{
		// lab destruction: enforce lab space limits.
		// take scientists off projects until it all evens out.
		// research is not cancelled.
		int qty = (*fac)->getRules()->getLaboratories() - getFreeLaboratories();

		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& qty > 0;
				)
		{
			if ((*i)->getAssigned() >= qty)
			{
				(*i)->setAssigned((*i)->getAssigned() - qty);
				_scientists += qty;

				break;
			}
			else
			{
				qty -= (*i)->getAssigned();
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
		int qty = (*fac)->getRules()->getWorkshops() - getFreeWorkshops();
//		int qty = getUsedWorkshops() - (getAvailableWorkshops() - (*fac)->getRules()->getWorkshops());
		for (std::vector<Production*>::const_iterator
				i = _productions.begin();
				i != _productions.end()
					&& qty > 0;
				)
		{
			if ((*i)->getAssignedEngineers() > qty)
			{
				(*i)->setAssignedEngineers((*i)->getAssignedEngineers() - qty);
				_engineers += qty;

				break;
			}
			else
			{
				qty -= (*i)->getAssignedEngineers();
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
 * Cleans up base defenses vector after a Ufo attack and optionally reclaims
 * the tanks and their ammo.
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
			const RuleItem* const itRule = (*i)->getRules();
			_items->addItem(itRule->getType());

			if (itRule->getCompatibleAmmo()->empty() == false)
			{
				const RuleItem* const ammoRule = _rules->getItem(itRule->getCompatibleAmmo()->front());
				const int ammoPerVehicle = ammoRule->getClipSize();
				_items->addItem(
							ammoRule->getType(),
							ammoPerVehicle);
			}
		}

		delete *i;
		i = _vehicles.erase(i);
	}
}

/**
 * Increases this Base's cash income amount by @a cash.
 * @param cash - amount of income
 */
void Base::setCashIncome(int cash)
{
	_cashIncome += cash;
}

/**
 * Gets this Base's current cash income value.
 * @return, current income value
 */
int Base::getCashIncome() const
{
	return _cashIncome;
}

/**
 * Increases this Base's cash spent amount by @a cash.
 * @param cash - amount of expenditure
 */
void Base::setCashSpent(int cash)
{
	_cashSpent += cash;
}

/**
 * Gets this Base's current cash spent value.
 * @return, current cash spent value
 */
int Base::getCashSpent() const
{
	return _cashSpent;
}

/**
 * Sets the effect of this Base's defense facilities before BaseDefense starts.
 * @param result - the defense result
 */
void Base::setDefenseResult(int result)
{
	_defenseResult = result;
}

/**
 * Gets the effect of this Base's defense facilities before BaseDefense starts.
 * @return, the defense result
 */
int Base::getDefenseResult() const
{
	return _defenseResult;
}

/**
 * Sets the current row for all of this Base's soldier lists.
 * @param row - current row
 */
void Base::setCurrentSoldierSlot(const size_t row)
{
	_curSoldier = row;
}

/**
 * Gets the current row for any of this Base's soldier lists.
 * @return, current row
 */
size_t Base::getCurrentSoldierSlot() const
{
	return _curSoldier;
}

/**
 * Calculates the bonus cost for soldiers by rank. Also adds cost to maintain Vehicles.
 * If @a craft is specified this returns the cost for a tactical mission;
 * if @a craft is NULL it returns this Base's monthly cost for Soldiers' bonus salaries.
 * @param craft - pointer to the Craft for the sortie (default NULL)
 * @return, cost
 */
int Base::calcSoldierBonuses(const Craft* const craft) const
{
	int total = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if (craft == NULL)
			total += (*i)->getRank() * 5000;
		else if ((*i)->getCraft() == craft)
			total += (*i)->getRank() * 1500;
	}

	if (craft != NULL)
		total += craft->getNumVehicles(true) * 750;

	return total;
}

/**
 * Returns a Soldier's bonus pay for going on a tactical mission; subtracts
 * the value from current funds.
 * @param sol	- pointer to a Soldier
 * @param dead	- true if soldier dies while on tactical (default false)
 * @return, the expense
 */
int Base::soldierExpense(
		const Soldier* const sol,
		const bool dead)
{
	int cost = sol->getRank() * 1500;
	if (dead == true)
		cost /= 2;

	_cashSpent += cost;
	_rules->getGame()->getSavedGame()->setFunds(_rules->getGame()->getSavedGame()->getFunds()
												- static_cast<int64_t>(cost));

	return cost;
}
/*	switch (sol->getRank())
	{
		case RANK_ROOKIE:
		break;
		case RANK_SQUADDIE:
		break;
		case RANK_SERGEANT:
		break;
		case RANK_CAPTAIN:
		break;
		case RANK_COLONEL:
		break;
		case RANK_COMMANDER:
		break;
		default:
	} */

/**
 * Returns the expense of sending HWPs/doggies on a tactical mission;
 * subtracts the value from current funds.
 * @param hwpSize	- size of the HWP/doggie in tiles
 * @param dead		- true if HWP got destroyed while on tactical (default false)
 * @return, the expense
 */
int Base::hwpExpense(
		const int hwpSize,
		const bool dead)
{
	int cost = hwpSize * 750;
	if (dead == true)
		cost /= 2;

	_cashSpent += cost;
	_rules->getGame()->getSavedGame()->setFunds(_rules->getGame()->getSavedGame()->getFunds()
												- static_cast<int64_t>(cost));

	return cost;
}

/**
 * Returns the expense of sending a transport craft on a tactical mission;
 * subtracts the value from current funds.
 * @param craft - pointer to a Craft
 * @return, the expense
 */
int Base::craftExpense(const Craft* const craft)
{
	int cost = craft->getRules()->getSoldiers() * 1000;
	_cashSpent += cost;

	return cost;
}

/**
 * Sorts the soldiers according to a pre-determined algorithm.
 */
void Base::sortSoldiers()
{
	std::multimap<int, Soldier*> soldiersOrdered;
	const UnitStats* stats;
	int weight;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		stats = (*i)->getCurrentStats();
		weight = stats->tu			*  8 // old values from CWXCED
			   + stats->stamina		*  5
			   + stats->health		*  7
			   + stats->bravery		*  3
			   + stats->reactions	* 21
			   + stats->firing		* 19
			   + stats->throwing	*  1
			   + stats->strength	* 24
			   + stats->melee		*  2;
		// also: rank, missions, kills

		if (stats->psiSkill != 0) // don't include Psi unless revealed.
			weight += stats->psiStrength * 22
					+ stats->psiSkill	 * 11;

		soldiersOrdered.insert(std::pair<int, Soldier*>(weight, *i));
		// NOTE: unsure if multimap loses a player-preferred
		// order of two soldiers with the same weight (to preserve that
		// would have to use vector of key-weights, stable_sort'd,
		// referenced back to a vector of <weight,Soldier> pairs,
		// possibly using a comparoperator functor. /cheers)
	}

	size_t j = 0;
	for (std::multimap<int, Soldier*>::const_iterator
			i = soldiersOrdered.begin();
			i != soldiersOrdered.end();
			++i)
	{
		_soldiers.at(j++) = (*i).second;
	}
}

}
