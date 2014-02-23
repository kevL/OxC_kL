/**
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

#define _USE_MATH_DEFINES

#include "Base.h"

#include <algorithm>
#include <cmath>
#include <stack>

#include "../aresame.h"

#include "BaseFacility.h"
#include "Craft.h"
#include "ItemContainer.h"
#include "Production.h"
#include "ResearchProject.h"
#include "Soldier.h"
#include "Target.h"
#include "Transfer.h"
#include "Ufo.h"
#include "Vehicle.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes an empty base.
 * @param rule Pointer to ruleset.
 */
Base::Base(const Ruleset* rule)
	:
		Target(),
		_rule(rule),
		_name(L""),
		_scientists(0),
		_engineers(0),
		_inBattlescape(false),
		_retaliationTarget(false),
		_cashIncome(0),	// kL
		_cashSpent(0)	// kL
{
	_items = new ItemContainer();
}

/**
 * Deletes the contents of the base from memory.
 */
Base::~Base()
{
	for (std::vector<BaseFacility*>::iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Soldier*>::iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Craft*>::iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		for (std::vector<Vehicle*>::iterator
				j = (*i)->getVehicles()->begin();
				j != (*i)->getVehicles()->end();
				++j)
		{
			for (std::vector<Vehicle*>::iterator
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

	for (std::vector<Transfer*>::iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Production*>::iterator
			i = _productions.begin();
			i != _productions.end();
			++i)
	{
		delete *i;
	}

	delete _items;

	for (std::vector<ResearchProject*>::iterator
			i = _research.begin();
			i != _research.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Vehicle*>::iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads the base from a YAML file.
 * @param node YAML node.
 * @param save Pointer to saved game.
 */
void Base::load(
		const YAML::Node& node,
		SavedGame* save,
		bool newGame,
		bool newBattleGame)
{
	Target::load(node);

	_name = Language::utf8ToWstr(node["name"].as<std::string>(""));

	if (!newGame
		|| !Options::getBool("customInitialBase")
		|| newBattleGame)
	{
		for (YAML::const_iterator
				i = node["facilities"].begin();
				i != node["facilities"].end();
				++i)
		{
			std::string type = (*i)["type"].as<std::string>();
			BaseFacility* f = new BaseFacility(
											_rule->getBaseFacility(type),
											this);
			f->load(*i);

			_facilities.push_back(f);
		}
	}

	for (YAML::const_iterator
			i = node["crafts"].begin();
			i != node["crafts"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		Craft* c = new Craft(
						_rule->getCraft(type),
						this);
		c->load(
				*i,
				_rule,
				save);

		_crafts.push_back(c);
	}

	for (YAML::const_iterator
			i = node["soldiers"].begin();
			i != node["soldiers"].end();
			++i)
	{
		Soldier* s = new Soldier(
							_rule->getSoldier("XCOM"),
							_rule->getArmor("STR_NONE_UC"));
		s->load(*i, _rule);

		if (const YAML::Node& craft = (*i)["craft"])
		{
			std::string type = craft["type"].as<std::string>();
			int id = craft["id"].as<int>();

			for (std::vector<Craft*>::iterator
					j = _crafts.begin();
					j != _crafts.end();
					++j)
			{
				if ((*j)->getRules()->getType() == type
					&& (*j)->getId() == id)
				{
					s->setCraft(*j);

					break;
				}
			}
		}
		else
			s->setCraft(0);

		_soldiers.push_back(s);
	}

	_items->load(node["items"]);
	// Some old saves have bad items, better get rid of them to avoid further bugs
	for (std::map<std::string, int>::iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			)
	{
		if (std::find(
					_rule->getItemsList().begin(),
					_rule->getItemsList().end(),
					i->first)
				== _rule->getItemsList().end())
		{
			_items->getContents()->erase(i++);
		}
		else
			++i;
	}

	_scientists		= node["scientists"].as<int>(_scientists);
	_engineers		= node["engineers"].as<int>(_engineers);
	_inBattlescape	= node["inBattlescape"].as<bool>(_inBattlescape);
	_cashIncome		= node["cashIncome"].as<int>(_cashIncome);	// kL
	_cashSpent		= node["cashSpent"].as<int>(_cashSpent);	// kL

	for (YAML::const_iterator
			i = node["transfers"].begin();
			i != node["transfers"].end();
			++i)
	{
		int hours = (*i)["hours"].as<int>();
		Transfer* t = new Transfer(hours);
		t->load(
				*i,
				this,
				_rule);

		_transfers.push_back(t);
	}

	for (YAML::const_iterator
			i = node["research"].begin();
			i != node["research"].end();
			++i)
	{
		std::string research = (*i)["project"].as<std::string>();
		ResearchProject* r = new ResearchProject(_rule->getResearch(research));
		r->load(*i);

		_research.push_back(r);
	}

	for (YAML::const_iterator
			i = node["productions"].begin();
			i != node["productions"].end();
			++i)
	{
		std::string item = (*i)["item"].as<std::string>();
		Production* p = new Production(
									_rule->getManufacture(item),
									0);
		p->load(*i);

		_productions.push_back(p);
	}

	_retaliationTarget = node["retaliationTarget"].as<bool>(_retaliationTarget);
}

/**
 * Saves the base to a YAML file.
 * @return YAML node.
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
	node["cashIncome"]		= _cashIncome;	// kL
	node["cashSpent"]		= _cashSpent;	// kL

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
 * Saves the base's unique identifiers to a YAML file.
 * @return YAML node.
 */
YAML::Node Base::saveId() const
{
	YAML::Node node = Target::saveId();

	node["type"]	= "STR_BASE";
	node["id"]		= 0;

	return node;
}

/**
 * Returns the custom name for the base.
 * @param lang Language to get strings from.
 * @return Name.
 */
std::wstring Base::getName(Language*) const
{
	return _name;
}

/**
 * Changes the custom name for the base.
 * @param name Name.
 */
void Base::setName(const std::wstring& name)
{
	_name = name;
}

/**
 * Returns the list of facilities in the base.
 * @return Pointer to the facility list.
 */
std::vector<BaseFacility*>* Base::getFacilities()
{
	return &_facilities;
}

/**
 * Returns the list of soldiers in the base.
 * @return Pointer to the soldier list.
 */
std::vector<Soldier*>* Base::getSoldiers()
{
	return &_soldiers;
}

/**
 * Returns the list of crafts in the base.
 * @return Pointer to the craft list.
 */
std::vector<Craft*>* Base::getCrafts()
{
	return &_crafts;
}

/**
 * Returns the list of transfers destined to this base.
 * @return Pointer to the transfer list.
 */
std::vector<Transfer*>* Base::getTransfers()
{
	return &_transfers;
}

/**
 * Returns the list of items in the base.
 * @return Pointer to the item list.
 */
ItemContainer *Base::getItems()
{
	return _items;
}

/**
 * Returns the amount of scientists currently in the base.
 * @return Number of scientists.
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
 * @return Number of engineers.
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
 * radar range, taking in account the range and chance.
 * @param target, Pointer to target to compare.
 * @return, 0 undetected; 1 detected; 2 hyperdetected
 */
uint8_t Base::detect(Target* target) const
{
	//Log(LOG_INFO) << "Base::detect() -> " << getName().c_str();
	uint8_t ret = 0;

	double targetDistance = insideRadarRange(target);
	if (AreSame(targetDistance, -2.0))
	{
		//Log(LOG_INFO) << ". not in range";
		return 0;
	}
	else if (AreSame(targetDistance, -1.0))
	{
		//Log(LOG_INFO) << ". hyperdetected";
		return 2;
	}
	else
	{
		float chance = 0;

		for (std::vector<BaseFacility*>::const_iterator
				f = _facilities.begin();
				f != _facilities.end();
				++f)
		{
			if ((*f)->getBuildTime() == 0)
			{
				double radarRange = static_cast<double>((*f)->getRules()->getRadarRange());
				//Log(LOG_INFO) << ". . radarRange = " << (int)radarRange;

				if (targetDistance < radarRange)
				{
					chance += static_cast<float>((*f)->getRules()->getRadarChance());
					//Log(LOG_INFO) << ". . radarRange = " << (int)radarRange;
					//Log(LOG_INFO) << ". . . chance(base) = " << (int)chance;
				}
				//else Log(LOG_INFO) << ". . target out of Range";
			}
		}

		if (AreSame(chance, 0.f))
			return 0;
		else
		{
			Ufo* u = dynamic_cast<Ufo*>(target);
			if (u != 0)
			{
				chance += static_cast<float>(u->getVisibility());
				//Log(LOG_INFO) << ". . chance(base + ufo) = " << (int)chance;
			}

			// need to divide by 3, since Geoscape-detection was moved from 30m to 10m time-slot.
			chance /= 3.f;
			// and convert to int:
			int iChance = static_cast<int>(chance);

			if (iChance < 1)
				return 0;
			else
			{
				ret = static_cast<uint8_t>(RNG::percent(iChance));
				//Log(LOG_INFO) << ". detected = " << (int)ret;
			}
		}
	}

	return ret;
}

/**
 * Returns if a certain target is inside the base's
 * radar range, taking in account the positions of both.
 * @param target, Pointer to target to compare.
 * @return, Distance to ufo; -1.0 if outside range; -2.0 if within range of hyperwave facility
 */
double Base::insideRadarRange(Target* target) const
{
	double ret = -2.0;

	double targetDistance = getDistance(target) * 3440.0;
	//Log(LOG_INFO) << ". targetDistance = " << (int)targetDistance;

	if (targetDistance > 2400.0) // early out.
		return ret;

	for (std::vector<BaseFacility*>::const_iterator
			f = _facilities.begin();
			f != _facilities.end();
			++f)
	{
		if ((*f)->getBuildTime() == 0)
		{
			double radarRange = static_cast<double>((*f)->getRules()->getRadarRange());
			//Log(LOG_INFO) << ". . radarRange = " << (int)radarRange;

			if (targetDistance < radarRange)
			{
				if ((*f)->getRules()->isHyperwave())
				{
					//Log(LOG_INFO) << ". . . . ret = hyperWave";
					return -1.0;
				}
				else
				{
					//Log(LOG_INFO) << ". . . ret = radar";
					ret = targetDistance;
				}
			}
		}
	}

	return ret;
}

/**
 * Returns the amount of soldiers contained in the base without any assignments.
 * @param combatReady does what it says on the tin. ( bull..)
 * @return Number of soldiers.
 */
int Base::getAvailableSoldiers(bool combatReady) const
{
	int total = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		if (!combatReady
			&& (*i)->getCraft() == 0)
		{
			total++;
		}
		else if (combatReady
			&& (((*i)->getCraft() != 0
					&& (*i)->getCraft()->getStatus() != "STR_OUT")
				|| ((*i)->getCraft() == 0
					&& (*i)->getWoundRecovery() == 0)))
		{
			total++;
		}
	}

	return total;
}

/**
 * Returns the total amount of soldiers contained in the base.
 * @return Number of soldiers.
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
 * @return Number of scientists.
 */
/*kL int Base::getAvailableScientists() const
{
	return getScientists();
} */

/**
 * Returns the total amount of scientists contained in the base.
 * @return Number of scientists.
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
 * @return Number of engineers.
 */
/*kL int Base::getAvailableEngineers() const
{
	return getEngineers();
} */

/**
 * Returns the total amount of engineers contained in the base.
 * @return Number of engineers.
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
 * @return Living space.
 */
int Base::getUsedQuarters() const
{
	return getTotalSoldiers() + getTotalScientists() + getTotalEngineers();
}

/**
 * Returns the total amount of living quarters available in the base.
 * @return Living space.
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
 * Returns the amount of stores used up by equipment in the base.
 * @return Storage space.
 */
int Base::getUsedStores() const
{
	float total = static_cast<float>(_items->getTotalSize(_rule));

	for (std::vector<Craft*>::const_iterator
			i = _crafts.begin();
			i != _crafts.end();
			++i)
	{
		total += static_cast<float>((*i)->getItems()->getTotalSize(_rule));

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
			total += static_cast<float>((*i)->getQuantity()) * _rule->getItem((*i)->getItems())->getSize();
	}

	return static_cast<int>(floor(total));
}

/**
 * Returns the total amount of stores available in the base.
 * @return Storage space.
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
 * Returns the amount of laboratories used up by research projects in the base.
 * @return Laboratory space.
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
 * @return Laboratory space.
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
 * @return Storage space.
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
 * @return Workshop space.
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
 * @return Storage space.
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
	}

	return total;
}

/**
 * Returns the total amount of hangars available in the base.
 * @return Number of hangars.
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
 * @return laboratories space not used by a ResearchProject
*/
int Base::getFreeLaboratories() const
{
	return getAvailableLaboratories() - getUsedLaboratories();
}

/**
 * Return workshop space not used by a Production
 * @return workshop space not used by a Production
*/
int Base::getFreeWorkshops() const
{
	return getAvailableWorkshops() - getUsedWorkshops();
}

/**
 * Return psilab space not in use
 * @return psilab space not in use
*/
int Base::getFreePsiLabs() const
{
	return getAvailablePsiLabs() - getUsedPsiLabs();
}

/**
 * Returns the amount of scientists currently in use.
 * @return Amount of scientists.
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
 * @return Amount of engineers.
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
 * @return Defense value.
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
int Base::getShortRangeDetection() const
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
				// kL_note: that should be based off a string or Ruleset value.
			if (range
				&& range < 1501)
			{
				total++;
			}
		}
	}

	return total;
}

/**
 * kL. Returns the total %value of short range detection facilities in the base.
 * @return, Short Range Detection %value.
 */
int Base::getShortRangeValue() const
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
				// kL_note: that should be based off a string or Ruleset value.
			if (range
				&& range < 1501)
			{
				total += (*i)->getRules()->getRadarChance();
			}
		}
	}

	return total;
}

/**
 * Returns the total amount of long range detection facilities in the base.
 * @return Long Range Detection value.
 */
int Base::getLongRangeDetection() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getRadarRange() > 1500)
				// kL_note: that should be based off a string or Ruleset value.
		{
			total++;
		}
	}

	return total;
}

/**
 * Returns the total %value of long range detection facilities in the base.
 * @return, Long Range Detection %value.
 */
int Base::getLongRangeValue() const
{
	int total = 0;

	for (std::vector<BaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getRadarRange() > 1500)
				// kL_note: that should be based off a string or Ruleset value.
		{
			total += (*i)->getRules()->getRadarChance();
		}
	}

	return total;
}

/**
 * Returns the total amount of craft of a certain type stored in the base.
 * @param craft Craft type.
 * @return Number of craft.
 */
int Base::getCraftCount(const std::string& craft) const
{
	int total = 0;

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
 * @return Maintenance costs.
 */
int Base::getCraftMaintenance() const
{
	int total = 0;

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
 * @return Maintenance costs.
 */
int Base::getPersonnelMaintenance() const
{
	int total = 0;

	total += static_cast<int>(_soldiers.size()) * _rule->getSoldierCost();
	total += getTotalEngineers() * _rule->getEngineerCost();
	total += getTotalScientists() * _rule->getScientistCost();

	return total;
}

/**
 * Returns the total amount of monthly costs for maintaining the facilities in the base.
 * @return Maintenance costs.
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
 * @return Maintenance costs.
 */
int Base::getMonthlyMaintenace() const
{
	return getCraftMaintenance() + getPersonnelMaintenance() + getFacilityMaintenance();
}

/**
 * Remove a Production from the Base.
 * @param p A pointer to a Production
 */
void Base::removeProduction(Production* prod)
{
	_engineers += prod->getAssignedEngineers();

	std::vector<Production*>::iterator i = std::find(
												_productions.begin(),
												_productions.end(),
												prod);
	if (i != _productions.end())
		_productions.erase(i);
}

/**
 * Get the list of Base Productions.
 * @return the list of Base Productions
 */
const std::vector<Production*>& Base::getProductions() const
{
	return _productions;
}

/**
 * Add a new Production to the Base
 * @param p A pointer to a Production
 */
void Base::addProduction(Production* prod)
{
	_productions.push_back(prod);
}

/**
 * Returns the list of all base's ResearchProject
 * @return list of base's ResearchProject
 */
const std::vector<ResearchProject*>& Base::getResearch() const
{
	return _research;
}

/**
 * Add a new ResearchProject to Base
 * @param project, The project to add
 */
void Base::addResearch(ResearchProject* project)
{
	_research.push_back(project);
}

/**
 * Remove a ResearchProject from base.
 * @param project, The project to remove
 * @param help, True to apply researchHelp()
 */
void Base::removeResearch(
		ResearchProject* project,
		bool help)
{
	//Log(LOG_INFO) << "Base::removeResearch()";
	_scientists += project->getAssigned();

	std::vector<ResearchProject*>::iterator
			i = std::find(
						_research.begin(),
						_research.end(),
						project);
	if (i != _research.end())
	{
		// kL_begin: Add Research Help here. aLien must be interrogated
		// at same Base as project-help applies to for now.
		if (help)
		{
			std::string aLien = project->getRules()->getName();
			//Log(LOG_INFO) << ". . aLien = " << aLien;
			// eg. Base::removeResearch() aLien = STR_REAPER_CORPSE

			researchHelp(aLien);
		} // kL_end.

		_research.erase(i);
	}
}

/**
 * kL: Research Help ala XcomUtil.
 * @param aLien, Name of the alien that got interrogated
 */
void Base::researchHelp(std::string aLien)
{
	bool found = false;
	float
		spent,
		cost;
	std::string help;

	if (aLien == "STR_FLOATER_SOLDIER"
		|| aLien == "STR_SNAKEMAN_SOLDIER"
		|| aLien == "STR_MUTON_SOLDIER"
		|| aLien == "STR_SECTOID_SOLDIER"
		|| aLien == "STR_ETHEREAL_SOLDIER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++i)
		{
			help	= (*i)->getRules()->getName();
			spent	= static_cast<float>((*i)->getSpent());
			cost	= static_cast<float>((*i)->getCost());

			if (help == "STR_HEAVY_PLASMA"
				|| help == "STR_PLASMA_RIFLE"
				|| help == "STR_PLASMA_PISTOL"
				|| help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				//Log(LOG_INFO) << ". . Soldier.1 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				//Log(LOG_INFO) << ". . Soldier.2 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_POWER_SUIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				//Log(LOG_INFO) << ". . Soldier.25 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_HEAVY_PLASMA_CLIP"
				|| help == "STR_PLASMA_RIFLE_CLIP"
				|| help == "STR_PLASMA_PISTOL_CLIP")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.4f)));
				found = true;

				//Log(LOG_INFO) << ". . Soldier.4 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_GRENADE"
				|| help == "STR_ALIEN_ENTERTAINMENT"
				|| help == "STR_PERSONAL_ARMOR")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				//Log(LOG_INFO) << ". . Soldier.5 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*i)->getSpent() > static_cast<int>(cost) - 1)
				(*i)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (aLien == "STR_FLOATER_NAVIGATOR"
		|| aLien == "STR_SNAKEMAN_NAVIGATOR"
		|| aLien == "STR_MUTON_NAVIGATOR"
		|| aLien == "STR_SECTOID_NAVIGATOR")
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++i)
		{
			help	= (*i)->getRules()->getName();
			spent	= static_cast<float>((*i)->getSpent());
			cost	= static_cast<float>((*i)->getCost());

			if (help == "STR_CYDONIA_OR_BUST"
				|| help == "STR_POWER_SUIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.15f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.15 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_HEAVY_PLASMA"
				|| help == "STR_HEAVY_PLASMA_CLIP"
				|| help == "STR_PLASMA_RIFLE"
				|| help == "STR_PLASMA_RIFLE_CLIP"
				|| help == "STR_PLASMA_PISTOL"
				|| help == "STR_PLASMA_PISTOL_CLIP"
				|| help == "STR_NEW_FIGHTER_CRAFT"
				|| help == "STR_NEW_FIGHTER_TRANSPORTER"
				|| help == "STR_ULTIMATE_CRAFT"
				|| help == "STR_PLASMA_CANNON"
				|| help == "STR_FUSION_MISSILE")
//					|| help == "hovertank-plasma" // <-
//					|| help == "hovertank-fusion" // <-
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.2 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_UFO_POWER_SOURCE"
				|| help == "STR_UFO_CONSTRUCTION"
				|| help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.25: help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_FLYING_SUIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.3f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.3 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.35f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.35 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_GRAV_SHIELD"
				|| help == "STR_ALIEN_ALLOYS")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.4f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.4 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_MOTION_SCANNER"
				|| help == "STR_ALIEN_ENTERTAINMENT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.5 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_HYPER_WAVE_DECODER"
				|| help == "STR_UFO_NAVIGATION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				//Log(LOG_INFO) << ". . Navigator.8 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*i)->getSpent() > static_cast<int>(cost) - 1)
				(*i)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (aLien == "STR_FLOATER_MEDIC"
		|| aLien == "STR_SECTOID_MEDIC")
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++i)
		{
			help	= (*i)->getRules()->getName();
			spent	= static_cast<float>((*i)->getSpent());
			cost	= static_cast<float>((*i)->getCost());

			if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				//Log(LOG_INFO) << ". . Medic.1 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS"
				|| help == "STR_ALIEN_ENTERTAINMENT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				//Log(LOG_INFO) << ". . Medic.2 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_MEDI_KIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.4f)));
				found = true;

				//Log(LOG_INFO) << ". . Medic.4 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ETHEREAL_CORPSE"
				|| help == "STR_SECTOPOD_CORPSE"
				|| help == "STR_SECTOID_CORPSE"
				|| help == "STR_CYBERDISC_CORPSE"
				|| help == "STR_SNAKEMAN_CORPSE"
				|| help == "STR_CHRYSSALID_CORPSE"
				|| help == "STR_MUTON_CORPSE"
				|| help == "STR_SILACOID_CORPSE"
				|| help == "STR_CELATID_CORPSE"
				|| help == "STR_FLOATER_CORPSE"
				|| help == "STR_REAPER_CORPSE")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				//Log(LOG_INFO) << ". . Medic.5 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_PSI_AMP"
				|| help == "STR_SMALL_LAUNCHER"
				|| help == "STR_STUN_BOMB"
				|| help == "STR_MIND_PROBE"
				|| help == "STR_MIND_SHIELD"
				|| help == "STR_PSI_LAB")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.6f)));
				found = true;

				//Log(LOG_INFO) << ". . Medic.6 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_FOOD"
				|| help == "STR_ALIEN_SURGERY"
				|| help == "STR_EXAMINATION_ROOM"
				|| help == "STR_ALIEN_REPRODUCTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				//Log(LOG_INFO) << ". . Medic.8 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*i)->getSpent() > static_cast<int>(cost) - 1)
				(*i)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (aLien == "STR_FLOATER_ENGINEER"
		|| aLien == "STR_SNAKEMAN_ENGINEER"
		|| aLien == "STR_MUTON_ENGINEER"
		|| aLien == "STR_SECTOID_ENGINEER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++i)
		{
			help	= (*i)->getRules()->getName();
			spent	= static_cast<float>((*i)->getSpent());
			cost	= static_cast<float>((*i)->getCost());

			if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				//Log(LOG_INFO) << ". . Engineer.1 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS"
				|| help == "STR_SMALL_LAUNCHER"
				|| help == "STR_STUN_BOMB")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				//Log(LOG_INFO) << ". . Engineer.2 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_NEW_FIGHTER_CRAFT"
				|| help == "STR_NEW_FIGHTER_TRANSPORTER"
				|| help == "STR_ULTIMATE_CRAFT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.3f)));
				found = true;

				//Log(LOG_INFO) << ". . Engineer.3 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_MOTION_SCANNER"
				|| help == "STR_HEAVY_PLASMA"
				|| help == "STR_HEAVY_PLASMA_CLIP"
				|| help == "STR_PLASMA_RIFLE"
				|| help == "STR_PLASMA_RIFLE_CLIP"
				|| help == "STR_PLASMA_PISTOL"
				|| help == "STR_PLASMA_PISTOL_CLIP"
				|| help == "STR_ALIEN_GRENADE"
				|| help == "STR_ELERIUM_115"
				|| help == "STR_UFO_POWER_SOURCE"
				|| help == "STR_UFO_CONSTRUCTION"
				|| help == "STR_ALIEN_ALLOYS"
				|| help == "STR_PLASMA_CANNON"
				|| help == "STR_FUSION_MISSILE"
				|| help == "STR_PLASMA_DEFENSE"
				|| help == "STR_FUSION_DEFENSE"
				|| help == "STR_GRAV_SHIELD"
				|| help == "STR_PERSONAL_ARMOR"
				|| help == "STR_POWER_SUIT"
				|| help == "STR_FLYING_SUIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				//Log(LOG_INFO) << ". . Engineer.5 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_BLASTER_LAUNCHER"
				|| help == "STR_BLASTER_BOMB")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.7f)));
				found = true;

				//Log(LOG_INFO) << ". . Engineer.7 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*i)->getSpent() > static_cast<int>(cost) - 1)
				(*i)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (aLien == "STR_FLOATER_LEADER"
		|| aLien == "STR_SNAKEMAN_LEADER"
		|| aLien == "STR_SECTOID_LEADER"
		|| aLien == "STR_ETHEREAL_LEADER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++i)
		{
			help	= (*i)->getRules()->getName();
			spent	= static_cast<float>((*i)->getSpent());
			cost	= static_cast<float>((*i)->getCost());

			if (help == "STR_NEW_FIGHTER_CRAFT"
				|| help == "STR_NEW_FIGHTER_TRANSPORTER"
				|| help == "STR_ULTIMATE_CRAFT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.1 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_HEAVY_PLASMA"
				|| help == "STR_HEAVY_PLASMA_CLIP"
				|| help == "STR_PLASMA_RIFLE"
				|| help == "STR_PLASMA_RIFLE_CLIP"
				|| help == "STR_PLASMA_PISTOL"
				|| help == "STR_PLASMA_PISTOL_CLIP"
				|| help == "STR_BLASTER_BOMB"
				|| help == "STR_SMALL_LAUNCHER"
				|| help == "STR_STUN_BOMB"
				|| help == "STR_ELERIUM_115"
				|| help == "STR_ALIEN_ALLOYS"
				|| help == "STR_PLASMA_CANNON"
				|| help == "STR_FUSION_MISSILE"
				|| help == "STR_CYDONIA_OR_BUST"
				|| help == "STR_PERSONAL_ARMOR"
				|| help == "STR_POWER_SUIT"
				|| help == "STR_FLYING_SUIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.2 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_PSI_AMP")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.25 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.3f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.3 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.5 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_BLASTER_LAUNCHER")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.6f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.6 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_EXAMINATION_ROOM")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				//Log(LOG_INFO) << ". . Leader.8 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*i)->getSpent() > static_cast<int>(cost) - 1)
				(*i)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (aLien == "STR_FLOATER_COMMANDER"
		|| aLien == "STR_SNAKEMAN_COMMANDER"
		|| aLien == "STR_SECTOID_COMMANDER"
		|| aLien == "STR_ETHEREAL_COMMANDER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++i)
		{
			help	= (*i)->getRules()->getName();
			spent	= static_cast<float>((*i)->getSpent());
			cost	= static_cast<float>((*i)->getCost());

			if (help == "STR_HEAVY_PLASMA"
				|| help == "STR_HEAVY_PLASMA_CLIP"
				|| help == "STR_PLASMA_RIFLE"
				|| help == "STR_PLASMA_RIFLE_CLIP"
				|| help == "STR_PLASMA_PISTOL"
				|| help == "STR_PLASMA_PISTOL_CLIP"
				|| help == "STR_SMALL_LAUNCHER"
				|| help == "STR_STUN_BOMB"
				|| help == "STR_NEW_FIGHTER_CRAFT"
				|| help == "STR_NEW_FIGHTER_TRANSPORTER"
				|| help == "STR_ULTIMATE_CRAFT"
				|| help == "STR_PLASMA_CANNON"
				|| help == "STR_FUSION_MISSILE")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				//Log(LOG_INFO) << ". . Commander.2 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_BLASTER_BOMB"
					|| help == "STR_ELERIUM_115"
					|| help == "STR_ALIEN_ALLOYS"
					|| help == "STR_PERSONAL_ARMOR"
					|| help == "STR_POWER_SUIT"
					|| help == "STR_FLYING_SUIT")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				//Log(LOG_INFO) << ". . Commander.25 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_PSI_AMP"
				|| help == "STR_CYDONIA_OR_BUST")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				//Log(LOG_INFO) << ". . Commander.5 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.6f)));
				found = true;

				//Log(LOG_INFO) << ". . Commander.6 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.7f)));
				found = true;

				//Log(LOG_INFO) << ". . Commander.7 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}
			else if (help == "STR_BLASTER_LAUNCHER"
				|| help == "STR_EXAMINATION_ROOM")
			{
				(*i)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				//Log(LOG_INFO) << ". . Commander.8 : help = " << help
				//		<< ", spent = " << (int)spent
				//		<< ", cost = " << (int)cost
				//		<< ", newSpent = " << (*i)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*i)->getSpent() > static_cast<int>(cost) - 1)
				(*i)->setSpent(static_cast<int>(cost) - 1);
		}
	}
}

/**
 * Returns whether or not this base is equipped with hyper-wave detection facilities.
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
 * Returns the total amount of Psi Lab Space available in the base.
 * @return Psi Lab space.
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
 * Returns the total amount of used Psi Lab Space in the base.
 * @return used Psi Lab space.
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
			total ++;
	}

	return total;
}

/**
 * Returns the total amount of used Containment Space in the base.
 * @return Containment Lab space.
 */
int Base::getUsedContainment() const
{
	int total = 0;

	for (std::map<std::string, int>::iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			++i)
	{
		if (_rule->getItem((i)->first)->getAlien())
			total += (i)->second;
	}

	for (std::vector<Transfer*>::const_iterator
			i = _transfers.begin();
			i != _transfers.end();
			++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM
			&& _rule->getItem((*i)->getItems())->getAlien())
		{
			total += (*i)->getQuantity();
		}
	}

	if (Options::getBool("alienContainmentLimitEnforced"))
	{
		for (std::vector<ResearchProject*>::const_iterator
				i = _research.begin();
				i != _research.end();
				++i)
		{
			const RuleResearch* projRules = (*i)->getRules();
			if (projRules->needItem()
				&& _rule->getUnit(projRules->getName()))
			{
				++total;
			}
		}
	}

	return total;
}

/**
 * Returns the total amount of Containment Space available in the base.
 * @return Containment Lab space.
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
 * Returns the base's battlescape status.
 * @return Is the craft on the battlescape?
 */
bool Base::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Changes the base's battlescape status.
 * @param inbattle True if it's in battle, False otherwise.
 */
void Base::setInBattlescape(bool inBattle)
{
	_inBattlescape = inBattle;
}

/**
 * Mark the base as a valid alien retaliation target.
 * @param mark Mark (if @c true) or unmark (if @c false) the base.
 */
void Base::setRetaliationStatus(bool mark)
{
	_retaliationTarget = mark;
}

/**
 * Get the base's retaliation status.
 * @return If the base is a valid target for alien retaliation.
 */
bool Base::getRetaliationStatus() const
{
	return _retaliationTarget;
}


/**
 * Functor to check for mind shield capability.
 */
struct isMindShield
	:
		public std::unary_function<BaseFacility*, bool>
{
	/// Check isMindShield() for @a facility.
	bool operator()(const BaseFacility* facility) const;
};

/**
 * Only fully operational facilities are checked.
 * @param facility Pointer to the facility to check.
 * @return If @a facility can act as a mind shield.
 */
bool isMindShield::operator()(const BaseFacility* facility) const
{
	if (facility->getBuildTime() != 0)
		return false; // Still building this

	return facility->getRules()->isMindShield();
}


/**
 * Functor to check for completed facilities.
 */
struct isCompleted
	:
		public std::unary_function<BaseFacility*, bool>
{
	/// Check isCompleted() for @a facility.
	bool operator()(const BaseFacility* facility) const;
};

/**
 * Facilities are checked for construction completion.
 * @param, facility Pointer to the facility to check.
 * @return, If @a facility has completed construction.
 */
bool isCompleted::operator()(const BaseFacility* facility) const
{
	return facility->getBuildTime() == 0;
}


/**
 * Calculate the detection chance of this base.
 * Big bases without mindshields are easier to detect.
 * @return, The detection chance.
 */
/*kL unsigned Base::getDetectionChance(int difficulty) const
{
	unsigned mindShields = std::count_if(_facilities.begin(), _facilities.end(), isMindShield());
	unsigned completedFacilities = std::count_if(_facilities.begin(), _facilities.end(), isCompleted());
	return ((completedFacilities / 6 + 15) / (mindShields + 1)) * (int)(1 + difficulty / 2);
} */
// kL_begin: rewrite getDetectionChance()
int Base::getDetectionChance(int difficulty) const
{
	//Log(LOG_INFO) << "Base::getDetectionChance()";

	int facilities = static_cast<int>(std::count_if(
												_facilities.begin(),
												_facilities.end(),
												isCompleted()));
	int shields = static_cast<int>(std::count_if(
											_facilities.begin(),
											_facilities.end(),
											isMindShield()));

	facilities = (facilities / 6) + 9;
	shields = (shields * 2) + 1;
	//Log(LOG_INFO) << ". facilities = " << facilities;
	//Log(LOG_INFO) << ". shields = " << shields;
	//Log(LOG_INFO) << ". difficulty = " << difficulty;

	int detchance = (facilities / shields) + difficulty;
	//Log(LOG_INFO) << ". detchance = " << detchance;

	return detchance;
}

/**
 *
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
			&& (*i)->getRules()->isGravShield())
		{
			++total;
		}
	}

	return total;
}

/**
 *
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
			&& (*i)->getRules()->getDefenseValue())
		{
			_defenses.push_back(*i);
		}
	}

	_vehicles.clear();

	// add vehicles that are in the crafts of the base, if it's not out
	for (std::vector<Craft*>::iterator
			c = getCrafts()->begin();
			c != getCrafts()->end();
			++c)
	{
		if ((*c)->getStatus() != "STR_OUT")
		{
			for (std::vector<Vehicle*>::iterator
					i = (*c)->getVehicles()->begin();
					i != (*c)->getVehicles()->end();
					++i)
			{
				_vehicles.push_back(*i);
			}
		}
	}

	// add vehicles left on the base
	for (std::map<std::string, int>::iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			)
	{
		std::string itemId = (i)->first;
		int iqty = (i)->second;

		RuleItem* rule = _rule->getItem(itemId);
		if (rule->isFixed())
		{
			int size = 4;
			if (_rule->getUnit(itemId))
				size = _rule->getArmor(_rule->getUnit(itemId)->getArmor())->getSize();

			if (rule->getCompatibleAmmo()->empty()) // so this vehicle does not need ammo
			{
				for (int
						j = 0;
						j < iqty;
						++j) _vehicles.push_back(new Vehicle(
															rule,
															rule->getClipSize(),
															size));

				_items->removeItem(
								itemId,
								iqty);
			}
			else // so this vehicle needs ammo
			{
				RuleItem* ammo = _rule->getItem(rule->getCompatibleAmmo()->front());

				int baqty = _items->getItem(ammo->getType()); // Ammo Quantity for this vehicle-type on the base
				if (baqty < 1
					|| iqty < 1)
				{
					++i;

					continue;
				}

				int canBeAdded = std::min(iqty, baqty);
				int newAmmoPerVehicle = std::min(baqty / canBeAdded, ammo->getClipSize());
				int remainder = 0;

				if (ammo->getClipSize() > newAmmoPerVehicle) remainder = baqty - (canBeAdded * newAmmoPerVehicle);
				int newAmmo;
				for (int
						j = 0;
						j < canBeAdded;
						++j)
				{
					newAmmo = newAmmoPerVehicle;
					if (j < remainder)
						++newAmmo;

					_vehicles.push_back(new Vehicle(
												rule,
												newAmmo,
												size));
					_items->removeItem(
									ammo->getType(),
									newAmmo);
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

std::vector<BaseFacility*>* Base::getDefenses()
{
	return &_defenses;
}

/**
 * Returns the list of vehicles currently equipped in the base.
 * @return Pointer to vehicle list.
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
	std::list<std::vector<BaseFacility*>::iterator> disFacs = getDisconnectedFacilities(0);
	for (std::list<std::vector<BaseFacility*>::iterator>::reverse_iterator
			i = disFacs.rbegin();
			i != disFacs.rend();
			++i)
		destroyFacility(*i);
}

/**
 * Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
 * @param remove Facility to ignore (in case of facility dismantling).
 * @return a sorted list of iterators pointing to elements in _facilities.
 */
std::list<std::vector<BaseFacility*>::iterator> Base::getDisconnectedFacilities(BaseFacility* remove)
{
	std::list<std::vector<BaseFacility*>::iterator> result;

	if (remove != 0
		&& remove->getRules()->isLift()) // Theoretically this is impossible, but sanity check is good :)
	{
		for (std::vector<BaseFacility*>::iterator
				i = _facilities.begin();
				i != _facilities.end();
				++i)
			if ((*i) != remove) result.push_back(i);

		return result;
	}

	std::vector<std::pair<std::vector<BaseFacility*>::iterator, bool>* > facilitiesConnStates;
	std::pair<std::vector<BaseFacility*>::iterator, bool>* grid[BASE_SIZE][BASE_SIZE];
	BaseFacility* lift = 0;

	for (int
			x = 0;
			x < BASE_SIZE;
			++x)
		for (int
				y = 0;
				y < BASE_SIZE;
				++y)
			grid[x][y] = 0;


	// Ok, fill up the grid(+facilitiesConnStates), and search the lift
	for (std::vector<BaseFacility*>::iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
		if ((*i) != remove)
		{
			if ((*i)->getRules()->isLift())
				lift = *i;
			for (int
					x = 0;
					x != (*i)->getRules()->getSize();
					++x)
				for (int
						y = 0;
						y != (*i)->getRules()->getSize();
						++y)
				{
					std::pair<std::vector<BaseFacility*>::iterator, bool>* p =
						new std::pair<std::vector<BaseFacility*>::iterator, bool>(i, false);
					facilitiesConnStates.push_back(p);
					grid[(*i)->getX() + x][(*i)->getY() + y] = p;
				}
		}

	// we're in real trouble if this happens...
	if (lift == 0)
		//TODO: something clever.
		return result;


	// Now make the recursion manually using a stack
	std::stack<std::pair<int, int> > stack;
	stack.push(std::make_pair(
							lift->getX(),
							lift->getY()));
	while (!stack.empty())
	{
		int
			x = stack.top().first,
			y = stack.top().second;
		stack.pop();

		if (x >= 0
			&& x < BASE_SIZE
			&& y >= 0
			&& y < BASE_SIZE
			&& grid[x][y] != 0
			&& !grid[x][y]->second)
		{
			grid[x][y]->second = true;

			BaseFacility* fac = *(grid[x][y]->first);

			BaseFacility* neighborLeft = (x - 1 >= 0 && grid[x - 1][y] != 0)? *(grid[x - 1][y]->first): 0;
			BaseFacility* neighborRight = (x + 1 < BASE_SIZE && grid[x + 1][y] != 0)? *(grid[x + 1][y]->first): 0;

			BaseFacility* neighborTop = (y - 1 >= 0 && grid[x][y - 1] != 0)? *(grid[x][y - 1]->first): 0;
			BaseFacility* neighborBottom = (y + 1 < BASE_SIZE && grid[x][y + 1] != 0)? *(grid[x][y + 1]->first): 0;

			if (fac->getBuildTime() == 0
				|| (neighborLeft != 0
					&& (neighborLeft == fac
						|| neighborLeft->getBuildTime() > neighborLeft->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x-1,y));
			}

			if (fac->getBuildTime() == 0
				|| (neighborRight != 0
					&& (neighborRight == fac
						|| neighborRight->getBuildTime() > neighborRight->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x+1,y));
			}

			if (fac->getBuildTime() == 0
				|| (neighborTop != 0
					&& (neighborTop == fac
						|| neighborTop->getBuildTime() > neighborTop->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x,y-1));
			}

			if (fac->getBuildTime() == 0
				|| (neighborBottom != 0
					&& (neighborBottom == fac
						|| neighborBottom->getBuildTime() > neighborBottom->getRules()->getBuildTime())))
			{
				stack.push(std::make_pair(x,y+1));
			}
		}
	}

	BaseFacility* lastFacility = 0;
	for (std::vector<std::pair<std::vector<BaseFacility*>::iterator, bool>* >::iterator
			i = facilitiesConnStates.begin();
			i != facilitiesConnStates.end();
			++i)
	{
		// Not a connected fac? -> push its iterator into the list!
		// Oh, and we don't want duplicates (facilities with bigger sizes like hangar)
		if (*((*i)->first) != lastFacility
			&& !(*i)->second)
		{
			result.push_back((*i)->first);
		}

		lastFacility = *((*i)->first);

		delete *i; // We don't need the pair anymore.
	}

	return result;
}

/**
 * Removes a base module, and deals with the ramifications thereof.
 * @param facility, An iterator reference to the facility to destroy and remove.
 */
void Base::destroyFacility(std::vector<BaseFacility*>::iterator facility)
{
	if ((*facility)->getRules()->getCrafts() > 0)
	{
		// hangar destruction -
		// destroy crafts and any production of crafts
		// as this will mean there is no hangar to contain it
		if ((*facility)->getCraft())
		{
			// remove all soldiers
			if ((*facility)->getCraft()->getNumSoldiers())
			{
				for (std::vector<Soldier*>::iterator
						i = _soldiers.begin();
						i != _soldiers.end();
						++i)
				{
					if ((*i)->getCraft() == (*facility)->getCraft())
						(*i)->setCraft(0);
				}
			}

			// remove all items
			while (!(*facility)->getCraft()->getItems()->getContents()->empty())
			{
				std::map<std::string, int>::iterator i = (*facility)->getCraft()->getItems()->getContents()->begin();
				_items->addItem(
							(*i).first,
							(*i).second);
				(*facility)->getCraft()->getItems()->removeItem(
															(*i).first,
															(*i).second);
			}

			for (std::vector<Craft*>::iterator
					i = _crafts.begin();
					i != _crafts.end();
					++i)
			{
				if (*i == (*facility)->getCraft())
				{
					delete (*i);
					_crafts.erase(i);

					break;
				}
			}
		}
		else
		{
			bool remove = true;

			// no craft - check productions.
			for (std::vector<Production*>::iterator
					i = _productions.begin();
					i != _productions.end();
					)
			{
				if (getAvailableHangars() - getUsedHangars() - (*facility)->getRules()->getCrafts() < 0
					&& (*i)->getRules()->getCategory() == "STR_CRAFT")
				{
					remove = false;
					_engineers += (*i)->getAssignedEngineers();

					delete *i;
					_productions.erase(i);

					break;
				}
				else
					++i;
			}

			if (remove
				&& !_transfers.empty())
			{
				for (std::vector<Transfer*>::iterator
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
	else if ((*facility)->getRules()->getPsiLaboratories() > 0)
	{
		// psi lab destruction: remove any soldiers over the maximum allowable from psi training.
		int toRemove = (*facility)->getRules()->getPsiLaboratories() - getFreePsiLabs();
		for (std::vector<Soldier*>::iterator
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
	else if ((*facility)->getRules()->getLaboratories())
	{
		// lab destruction: enforce lab space limits.
		// take scientists off projects until it all evens out.
		// research is not cancelled.
		int toRemove = (*facility)->getRules()->getLaboratories() - getFreeLaboratories();
		for (std::vector<ResearchProject*>::iterator
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
	else if ((*facility)->getRules()->getWorkshops())
	{
		// workshop destruction: similar to lab destruction, but we'll lay off engineers instead. kL_note: huh!!!!
		// in this case, however, production IS cancelled, as it takes up space in the workshop.
		int toRemove = (*facility)->getRules()->getWorkshops() - getFreeWorkshops();
//		int toRemove = getUsedWorkshops() - (getAvailableWorkshops() - (*facility)->getRules()->getWorkshops());
		for (std::vector<Production*>::iterator
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
	else if ((*facility)->getRules()->getStorage())
	{
		// we won't destroy the items physically AT the base,
		// but any items in transit will end up at the dead letter office.
		if (!_transfers.empty()
			&& getAvailableStores() - getUsedStores() - (*facility)->getRules()->getStorage() < 0)
		{
			for (std::vector<Transfer*>::iterator
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
	else if ((*facility)->getRules()->getPersonnel())
	{
		// as above, we won't actually fire people, but we'll block any new ones coming in.
		if (!_transfers.empty()
			&& getAvailableQuarters() - getUsedQuarters() - (*facility)->getRules()->getPersonnel() < 0)
		{
			for (std::vector<Transfer*>::iterator
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

	delete *facility;
	_facilities.erase(facility);
}

// kL_begin: Base, for GraphsState monthly expenditures etc.
/**
 * Increase the Base's cash income amount by 'cash'.
 * @param (int)cash, The amount to increase _cashIncome by
 */
void Base::setCashIncome(int cash)
{
	_cashIncome += cash;
}

/**
 * Get the Base's current cash income value.
 * @return int, The Base's current _cashIncome amount
 */
int Base::getCashIncome() const
{
	return _cashIncome;
}

/**
 * Increase the Base's cash spent amount by 'cash'.
 * @param (int)cash, The amount to increase _cashSpent by
 */
void Base::setCashSpent(int cash)
{
	_cashSpent += cash;
}

/**
 * Get the Base's current cash spent value.
 * @return int, The Base's current _cashSpent amount
 */
int Base::getCashSpent() const
{
	return _cashSpent;
} // kL_end.

}
