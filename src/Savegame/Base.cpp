/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#define _USE_MATH_DEFINES

#include "Base.h"

#include <algorithm>
#include <cmath>

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
		_retaliationTarget(false)
//		_currentBase(0)		// kL
{
	_items = new ItemContainer();
}

/**
 * Deletes the contents of the base from memory.
 */
Base::~Base()
{
	for (std::vector<BaseFacility*>::iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		delete *i;
	}

	for (std::vector<Soldier*>::iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		delete *i;
	}

	for (std::vector<Craft*>::iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		for (std::vector<Vehicle*>::iterator j = (*i)->getVehicles()->begin(); j != (*i)->getVehicles()->end(); ++j)
		{
			for (std::vector<Vehicle*>::iterator k = _vehicles.begin(); k != _vehicles.end(); ++k)
			{
				if ((*k) == (*j)) // to avoid calling a vehicle's destructor twice
				{
					_vehicles.erase(k);

					break;
				}
			}
		}

		delete *i;
	}

	for (std::vector<Transfer*>::iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		delete *i;
	}

	for (std::vector<Production*>::iterator i = _productions.begin (); i != _productions.end (); ++i)
	{
		delete *i;
	}

	delete _items;

	for (std::vector<ResearchProject*>::iterator i = _research.begin(); i != _research.end(); ++i)
	{
		delete *i;
	}

	for (std::vector<Vehicle*>::iterator i = _vehicles.begin(); i != _vehicles.end(); ++i)
	{
		delete *i;
	}
}

/**
 * Loads the base from a YAML file.
 * @param node YAML node.
 * @param save Pointer to saved game.
 */
void Base::load(const YAML::Node& node, SavedGame* save, bool newGame, bool newBattleGame)
{
	Target::load(node);
	_name = Language::utf8ToWstr(node["name"].as<std::string>(""));

	if (!newGame || !Options::getBool("customInitialBase") || newBattleGame)
	{
		for (YAML::const_iterator i = node["facilities"].begin(); i != node["facilities"].end(); ++i)
		{
			std::string type = (*i)["type"].as<std::string>();
			BaseFacility* f = new BaseFacility(_rule->getBaseFacility(type), this);
			f->load(*i);
			_facilities.push_back(f);
		}
	}

	for (YAML::const_iterator i = node["crafts"].begin(); i != node["crafts"].end(); ++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		Craft* c = new Craft(_rule->getCraft(type), this);
		c->load(*i, _rule, save);		
		_crafts.push_back(c);
	}

	for (YAML::const_iterator i = node["soldiers"].begin(); i != node["soldiers"].end(); ++i)
	{
		Soldier* s = new Soldier(_rule->getSoldier("XCOM"), _rule->getArmor("STR_NONE_UC"));
		s->load(*i, _rule);

		if (const YAML::Node& craft = (*i)["craft"])
		{
			std::string type = craft["type"].as<std::string>();
			int id = craft["id"].as<int>();
			for (std::vector<Craft*>::iterator j = _crafts.begin(); j != _crafts.end(); ++j)
			{
				if ((*j)->getRules()->getType() == type && (*j)->getId() == id)
				{
					s->setCraft(*j);
					break;
				}
			}
		}
		else
		{
			s->setCraft(0);
		}
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
					i->first) == _rule->getItemsList().end())
		{
			_items->getContents()->erase(i++);
		}
		else
		{
			++i;
		}
	}

	_scientists = node["scientists"].as<int>(_scientists);
	_engineers = node["engineers"].as<int>(_engineers);
	_inBattlescape = node["inBattlescape"].as<bool>(_inBattlescape);

	for (YAML::const_iterator i = node["transfers"].begin(); i != node["transfers"].end(); ++i)
	{
		int hours = (*i)["hours"].as<int>();
		Transfer* t = new Transfer(hours);
		t->load(*i, this, _rule);
		_transfers.push_back(t);
	}

	for (YAML::const_iterator i = node["research"].begin(); i != node["research"].end(); ++i)
	{
		std::string research = (*i)["project"].as<std::string>();
		ResearchProject* r = new ResearchProject(_rule->getResearch(research));
		r->load(*i);
		_research.push_back(r);
	}

	for (YAML::const_iterator i = node["productions"].begin(); i != node["productions"].end(); ++i)
	{
		std::string item = (*i)["item"].as<std::string>();
		Production* p = new Production(_rule->getManufacture(item), 0);
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

	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		node["facilities"].push_back((*i)->save());
	}

	for (std::vector<Soldier*>::const_iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		node["soldiers"].push_back((*i)->save());
	}

	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		node["crafts"].push_back((*i)->save());
	}

	node["items"] = _items->save();
	node["scientists"] = _scientists;
	node["engineers"] = _engineers;
	node["inBattlescape"] = _inBattlescape;

	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		node["transfers"].push_back((*i)->save());
	}

	for (std::vector<ResearchProject*>::const_iterator i = _research.begin(); i != _research.end(); ++i)
	{
		node["research"].push_back((*i)->save());
	}

	for (std::vector<Production*>::const_iterator i = _productions.begin(); i != _productions.end(); ++i)
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
	Log(LOG_INFO) << "Base::detect()";
	uint8_t ret = 0;

	double targetDistance = insideRadarRange(target);
	if (targetDistance == -2.0)
	{
		Log(LOG_INFO) << ". not in range";
		return 0;
	}
	else if (targetDistance == -1.0)
	{
		Log(LOG_INFO) << ". hyperdetected";
		return 2;
	}
	else
	{
		int percent = 0;

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
					percent += (*f)->getRules()->getRadarChance();
					Log(LOG_INFO) << ". . radarRange = " << (int)radarRange;
					Log(LOG_INFO) << ". . . percent(base) = " << percent;
				}
				else
				{
					Log(LOG_INFO) << ". . target out of Range";
				}
			}
		}

		if (percent == 0)
			return 0;
		else
		{
			Ufo* u = dynamic_cast<Ufo*>(target);
			if (u != 0)
			{
				percent += u->getVisibility();
				Log(LOG_INFO) << ". . percent(base + ufo) = " << percent;
			}

			if (percent < 1)
				return 0;
			else
			{
				ret = static_cast<uint8_t>(RNG::percent(percent));
				Log(LOG_INFO) << ". detected = " << static_cast<int>(ret);
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
	Log(LOG_INFO) << ". targetDistance = " << (int)targetDistance;

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
			Log(LOG_INFO) << ". . radarRange = " << (int)radarRange;

			if (targetDistance < radarRange)
			{
				if ((*f)->getRules()->isHyperwave())
				{
					Log(LOG_INFO) << ". . . . ret = hyperWave";
					return -1.0;
				}
				else
				{
					Log(LOG_INFO) << ". . . ret = radar";
					ret = targetDistance;
				}
			}
		}
	}
	
	return ret;
}

/**
 * Returns the amount of soldiers contained in the base without any assignments.
 * @param checkCombatReadiness does what it says on the tin.
 * @return Number of soldiers.
 */
int Base::getAvailableSoldiers(bool checkCombatReadiness) const
{
	int total = 0;

	for (std::vector<Soldier*>::const_iterator i = _soldiers.begin(); i != _soldiers.end(); ++i)
	{
		if (!checkCombatReadiness
			&& (*i)->getCraft() == 0)
		{
			total++;
		}
		else if (checkCombatReadiness
			&& (((*i)->getCraft() != 0 && (*i)->getCraft()->getStatus() != "STR_OUT")
				|| ((*i)->getCraft() == 0 && (*i)->getWoundRecovery() == 0)))
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
	size_t total = _soldiers.size();
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_SOLDIER)
		{
			total += (*i)->getQuantity();
		}
	}

	return total;
}

/**
 * Returns the amount of scientists contained in the base without any assignments.
 * @return Number of scientists.
 */
int Base::getAvailableScientists() const
{
	return getScientists();
}

/**
 * Returns the total amount of scientists contained in the base.
 * @return Number of scientists.
 */
int Base::getTotalScientists() const
{
	int total = _scientists;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_SCIENTIST)
		{
			total += (*i)->getQuantity();
		}
	}

	const std::vector<ResearchProject*>& research(getResearch());
	for (std::vector<ResearchProject*>::const_iterator itResearch = research.begin(); itResearch != research.end(); ++itResearch)
	{
		total += (*itResearch)->getAssigned();
	}

	return total;
}

/**
 * Returns the amount of engineers contained in the base without any assignments.
 * @return Number of engineers.
 */
int Base::getAvailableEngineers() const
{
	return getEngineers();
}

/**
 * Returns the total amount of engineers contained in the base.
 * @return Number of engineers.
 */
int Base::getTotalEngineers() const
{
	int total = _engineers;
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_ENGINEER)
		{
			total += (*i)->getQuantity();
		}
	}

	for (std::vector<Production*>::const_iterator iter = _productions.begin (); iter != _productions.end (); ++iter)
	{
		total += (*iter)->getAssignedEngineers();
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getPersonnel();
		}
	}

	return total;
}

/**
 * Returns the amount of stores used up by equipment in the base.
 * @return Storage space.
 */
int Base::getUsedStores() const
{
	double total = _items->getTotalSize(_rule);
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		total += (*i)->getItems()->getTotalSize(_rule);
		for (std::vector<Vehicle*>::const_iterator j = (*i)->getVehicles()->begin(); j != (*i)->getVehicles()->end(); ++j)
		{
			total += (*j)->getRules()->getSize();
		}
	}

	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM)
		{
			total += (*i)->getQuantity() * _rule->getItem((*i)->getItems())->getSize();
		}
	}

	return (int)floor(total);
}

/**
 * Returns the total amount of stores available in the base.
 * @return Storage space.
 */
int Base::getAvailableStores() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getStorage();
		}
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
	for (std::vector<ResearchProject*>::const_iterator itResearch = research.begin (); itResearch != research.end(); ++itResearch)
	{
		usedLabSpace += (*itResearch)->getAssigned();
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getLaboratories();
		}
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
	for (std::vector<Production*>::const_iterator iter = _productions.begin(); iter != _productions.end(); ++iter)
	{
		usedWorkShop += (*iter)->getAssignedEngineers() + (*iter)->getRules()->getRequiredSpace();
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getWorkshops();
		}
	}

	return total;
}

/**
 * Returns the amount of hangars used up by crafts in the base.
 * @return Storage space.
 */
int Base::getUsedHangars() const
{
	size_t total = _crafts.size();
	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_CRAFT)
		{
			total += (*i)->getQuantity();
		}
	}

	for (std::vector<Production*>::const_iterator i = _productions.begin(); i != _productions.end(); ++i)
	{
		if ((*i)->getRules()->getCategory() == "STR_CRAFT")
		{
			total += ((*i)->getAmountTotal() - (*i)->getAmountProduced());
		}
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getCrafts();
		}
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
 * Returns the amount of scientists currently in use.
 * @return Amount of scientists.
*/
int Base::getAllocatedScientists() const
{
	int total = 0;
	const std::vector<ResearchProject*>& research (getResearch());
	for (std::vector<ResearchProject*>::const_iterator itResearch = research.begin (); itResearch != research.end (); ++itResearch)
	{
		total += (*itResearch)->getAssigned ();
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
	for (std::vector<Production *>::const_iterator iter = _productions.begin (); iter != _productions.end (); ++iter)
	{
		total += (*iter)->getAssignedEngineers();
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getDefenseValue();
		}
	}

	return total;
}

/**
 * Returns the total amount of short range detection facilities in the base.
 * @return Defense value.
 */
int Base::getShortRangeDetection() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getRadarRange() == 1500)
		{
			total++;
		}
	}

	return total;
}

/**
 * Returns the total amount of long range detection facilities in the base.
 * @return Defense value.
 */
int Base::getLongRangeDetection() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getRadarRange() > 1500)
		{
			total++;
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
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
	{
		if ((*i)->getRules()->getType() == craft)
		{
			total++;
		}
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
	for (std::vector<Craft*>::const_iterator i = _crafts.begin(); i != _crafts.end(); ++i)
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
	size_t total = 0;
	total += _soldiers.size() * _rule->getSoldierCost();
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getMonthlyCost();
		}
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
 * Returns the list of all base's ResearchProject
 * @return list of base's ResearchProject
*/
const std::vector<ResearchProject*>& Base::getResearch() const
{
	return _research;
}

/**
 * Add a new Production to the Base
 * @param p A pointer to a Production
*/
void Base::addProduction(Production* p)
{
	_productions.push_back(p);
}

/**
 * Add a new ResearchProject to Base
 * @param project The project to add
*/
void Base::addResearch(ResearchProject* project)
{
	_research.push_back(project);
}

/**
 * Remove a ResearchProject from base
 * @param project, The project to remove
*/
void Base::removeResearch(ResearchProject* project)
{
	//Log(LOG_INFO) << "Base::removeResearch()";
	_scientists += project->getAssigned();

	std::vector<ResearchProject*>::iterator iter = std::find(_research.begin(), _research.end(), project);
	if (iter != _research.end())
	{
		// kL_begin: Add Research Help here. aLien must be interrogated at same Base as project-help goes to (for now).
		std::string sProject = project->getRules()->getName();
		//Log(LOG_INFO) << ". . sProject = " << sProject;
		// eg. Base::removeResearch() sProject = STR_REAPER_CORPSE

		researchHelp(sProject);		// kL
		// kL_end.

		_research.erase(iter);
	}
}

/**
 * kL: Research Help ala XcomUtil.
 * @param sProject, name of the alien getting interrogated
 */
void Base::researchHelp(std::string sProject)
{
	std::string help;
	float spent, cost;
	bool found = false;

	if (sProject == "STR_FLOATER_SOLDIER"
		|| sProject == "STR_SNAKEMAN_SOLDIER"
		|| sProject == "STR_MUTON_SOLDIER"
		|| sProject == "STR_SECTOID_SOLDIER"
		|| sProject == "STR_ETHEREAL_SOLDIER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				iter = _research.begin();
				iter != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++iter)
		{
			help	= (*iter)->getRules()->getName();
			spent	= static_cast<float>((*iter)->getSpent());
			cost	= static_cast<float>((*iter)->getCost());

			if (help == "STR_HEAVY_PLASMA"
				|| help == "STR_PLASMA_RIFLE"
				|| help == "STR_PLASMA_PISTOL"
				|| help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				Log(LOG_INFO) << ". . Soldier.1 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				Log(LOG_INFO) << ". . Soldier.2 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_POWER_SUIT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				Log(LOG_INFO) << ". . Soldier.25 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_HEAVY_PLASMA_CLIP"
				|| help == "STR_PLASMA_RIFLE_CLIP"
				|| help == "STR_PLASMA_PISTOL_CLIP")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.4f)));
				found = true;

				Log(LOG_INFO) << ". . Soldier.4 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_GRENADE"
				|| help == "STR_ALIEN_ENTERTAINMENT"
				|| help == "STR_PERSONAL_ARMOR")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				Log(LOG_INFO) << ". . Soldier.5 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*iter)->getSpent() > static_cast<int>(cost) - 1)
				(*iter)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (sProject == "STR_FLOATER_NAVIGATOR"
		|| sProject == "STR_SNAKEMAN_NAVIGATOR"
		|| sProject == "STR_MUTON_NAVIGATOR"
		|| sProject == "STR_SECTOID_NAVIGATOR")
	{
		for (std::vector<ResearchProject*>::const_iterator
				iter = _research.begin();
				iter != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++iter)
		{
			help	= (*iter)->getRules()->getName();
			spent	= static_cast<float>((*iter)->getSpent());
			cost	= static_cast<float>((*iter)->getCost());

			if (help == "STR_CYDONIA_OR_BUST"
				|| help == "STR_POWER_SUIT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.15f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.15 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
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
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.2 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_UFO_POWER_SOURCE"
				|| help == "STR_UFO_CONSTRUCTION"
				|| help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.25: help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_FLYING_SUIT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.3f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.3 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.35f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.35 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_GRAV_SHIELD"
				|| help == "STR_ALIEN_ALLOYS")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.4f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.4 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_MOTION_SCANNER"
				|| help == "STR_ALIEN_ENTERTAINMENT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.5 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_HYPER_WAVE_DECODER"
				|| help == "STR_UFO_NAVIGATION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				Log(LOG_INFO) << ". . Navigator.8 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*iter)->getSpent() > static_cast<int>(cost) - 1)
				(*iter)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (sProject == "STR_FLOATER_MEDIC"
		|| sProject == "STR_SNAKEMAN_MEDIC"
		|| sProject == "STR_SECTOID_MEDIC")
	{
		for (std::vector<ResearchProject*>::const_iterator
				iter = _research.begin();
				iter != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++iter)
		{
			help	= (*iter)->getRules()->getName();
			spent	= static_cast<float>((*iter)->getSpent());
			cost	= static_cast<float>((*iter)->getCost());

			if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				Log(LOG_INFO) << ". . Medic.1 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				Log(LOG_INFO) << ". . Medic.2 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_REPRODUCTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				Log(LOG_INFO) << ". . Medic.5 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_MEDI_KIT"
				|| help == "STR_PSI_AMP"
				|| help == "STR_SMALL_LAUNCHER"
				|| help == "STR_STUN_BOMB"
				|| help == "STR_MIND_PROBE"
				|| help == "STR_ALIEN_FOOD"
				|| help == "STR_ALIEN_SURGERY"
				|| help == "STR_EXAMINATION_ROOM"
				|| help == "STR_MIND_SHIELD"
				|| help == "STR_PSI_LAB")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				Log(LOG_INFO) << ". . Medic.8 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*iter)->getSpent() > static_cast<int>(cost) - 1)
				(*iter)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (sProject == "STR_FLOATER_ENGINEER"
		|| sProject == "STR_SNAKEMAN_ENGINEER"
		|| sProject == "STR_MUTON_ENGINEER"
		|| sProject == "STR_SECTOID_ENGINEER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				iter = _research.begin();
				iter != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++iter)
		{
			help	= (*iter)->getRules()->getName();
			spent	= static_cast<float>((*iter)->getSpent());
			cost	= static_cast<float>((*iter)->getCost());

			if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				Log(LOG_INFO) << ". . Engineer.1 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS"
				|| help == "STR_SMALL_LAUNCHER"
				|| help == "STR_STUN_BOMB")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				Log(LOG_INFO) << ". . Engineer.2 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_NEW_FIGHTER_CRAFT"
				|| help == "STR_NEW_FIGHTER_TRANSPORTER"
				|| help == "STR_ULTIMATE_CRAFT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.3f)));
				found = true;

				Log(LOG_INFO) << ". . Engineer.3 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
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
//				(*iter)->setSpent(spent + (int)(cost * 0.5f));
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				Log(LOG_INFO) << ". . Engineer.5 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_BLASTER_LAUNCHER"
				|| help == "STR_BLASTER_BOMB")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.7f)));
				found = true;

				Log(LOG_INFO) << ". . Engineer.7 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*iter)->getSpent() > static_cast<int>(cost) - 1)
				(*iter)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (sProject == "STR_FLOATER_LEADER"
		|| sProject == "STR_SNAKEMAN_LEADER"
		|| sProject == "STR_SECTOID_LEADER"
		|| sProject == "STR_ETHEREAL_LEADER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				iter = _research.begin();
				iter != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++iter)
		{
			help	= (*iter)->getRules()->getName();
			spent	= static_cast<float>((*iter)->getSpent());
			cost	= static_cast<float>((*iter)->getCost());

			if (help == "STR_NEW_FIGHTER_CRAFT"
				|| help == "STR_NEW_FIGHTER_TRANSPORTER"
				|| help == "STR_ULTIMATE_CRAFT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.1f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.1 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
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
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.2 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_PSI_AMP")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.25 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.3f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.3 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.5 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_BLASTER_LAUNCHER")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.6f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.6 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_EXAMINATION_ROOM")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				Log(LOG_INFO) << ". . Leader.8 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*iter)->getSpent() > static_cast<int>(cost) - 1)
				(*iter)->setSpent(static_cast<int>(cost) - 1);
		}
	}
	else if (sProject == "STR_FLOATER_COMMANDER"
		|| sProject == "STR_SNAKEMAN_COMMANDER"
		|| sProject == "STR_SECTOID_COMMANDER"
		|| sProject == "STR_ETHEREAL_COMMANDER")
	{
		for (std::vector<ResearchProject*>::const_iterator
				iter = _research.begin();
				iter != _research.end()
					&& !found; // only the first project found gets the benefit. Could do a menu allowing player to choose...
				++iter)
		{
			help	= (*iter)->getRules()->getName();
			spent	= static_cast<float>((*iter)->getSpent());
			cost	= static_cast<float>((*iter)->getCost());

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
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.2f)));
				found = true;

				Log(LOG_INFO) << ". . Commander.2 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_BLASTER_BOMB"
					|| help == "STR_ELERIUM_115"
					|| help == "STR_ALIEN_ALLOYS"
					|| help == "STR_PERSONAL_ARMOR"
					|| help == "STR_POWER_SUIT"
					|| help == "STR_FLYING_SUIT")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.25f)));
				found = true;

				Log(LOG_INFO) << ". . Commander.25 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_PSI_AMP"
				|| help == "STR_CYDONIA_OR_BUST")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.5f)));
				found = true;

				Log(LOG_INFO) << ". . Commander.5 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_THE_MARTIAN_SOLUTION")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.6f)));
				found = true;

				Log(LOG_INFO) << ". . Commander.6 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_ALIEN_ORIGINS")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.7f)));
				found = true;

				Log(LOG_INFO) << ". . Commander.7 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}
			else if (help == "STR_BLASTER_LAUNCHER"
				|| help == "STR_EXAMINATION_ROOM")
			{
				(*iter)->setSpent(static_cast<int>(spent + ((cost - spent) * 0.8f)));
				found = true;

				Log(LOG_INFO) << ". . Commander.8 : help = " << help
						<< ", spent = " << (int)spent
						<< ", cost = " << (int)cost
						<< ", newSpent = " << (*iter)->getSpent();
			}

			// always takes an extra day to complete (and guards vs. potential code-flow bork!)
			if ((*iter)->getSpent() > static_cast<int>(cost) - 1)
				(*iter)->setSpent(static_cast<int>(cost) - 1);
		}
	}
}

/**
 * Remove a Production from the Base.
 * @param p A pointer to a Production
*/
void Base::removeProduction(Production* p)
{
	_engineers += p->getAssignedEngineers();

	std::vector<Production*>::iterator iter = std::find(_productions.begin(), _productions.end(), p);
	if (iter != _productions.end())
	{
		_productions.erase(iter);
	}
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
 * Returns whether or not this base is equipped with hyper-wave detection facilities.
 */
bool Base::getHyperDetection() const
{
	for (std::vector<BaseFacility*>::const_iterator f = _facilities.begin(); f != _facilities.end(); ++f)
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
	for (std::vector<BaseFacility*>::const_iterator f = _facilities.begin(); f != _facilities.end(); ++f)
	{
		if ((*f)->getBuildTime() == 0)
		{
			total += (*f)->getRules()->getPsiLaboratories();
		}
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
	for (std::vector<Soldier*>::const_iterator s = _soldiers.begin(); s != _soldiers.end(); ++s)
	{
		if ((*s)->isInPsiTraining())
		{
			total ++;
		}
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
	for (std::map<std::string, int>::iterator i = _items->getContents()->begin(); i != _items->getContents()->end(); ++i)
	{
		if (_rule->getItem((i)->first)->getAlien())
		{
			total += (i)->second;
		}
	}

	for (std::vector<Transfer*>::const_iterator i = _transfers.begin(); i != _transfers.end(); ++i)
	{
		if ((*i)->getType() == TRANSFER_ITEM)
		{
			if (_rule->getItem((*i)->getItems())->getAlien())
			{
				total += (*i)->getQuantity();
			}
		}
	}

	if (Options::getBool("alienContainmentLimitEnforced"))
	{
		for (std::vector<ResearchProject*>::const_iterator i = _research.begin(); i != _research.end(); ++i)
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
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			total += (*i)->getRules()->getAliens();
		}
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
void Base::setInBattlescape(bool inbattle)
{
	_inBattlescape = inbattle;
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
struct isMindShield: public std::unary_function<BaseFacility*, bool>
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
	{
		return false; // Still building this
	}

	return facility->getRules()->isMindShield();
}

/**
 * Functor to check for completed facilities.
 */
struct isCompleted: public std::unary_function<BaseFacility*, bool>
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
/*kL unsigned Base::getDetectionChance() const
{
	unsigned mindShields = std::count_if(_facilities.begin(), _facilities.end(), isMindShield());
	unsigned completedFacilities = std::count_if(_facilities.begin(), _facilities.end(), isCompleted());
	return (completedFacilities / 6 + 15) / (mindShields + 1);
} */
// kL_begin: rewrite getDetectionChance() using floats.
int Base::getDetectionChance() const
{
	Log(LOG_INFO) << "Base::getDetectionChance()";
	int shields = static_cast<int>(std::count_if(_facilities.begin(), _facilities.end(), isMindShield()));
	int facilities = static_cast<int>(std::count_if(_facilities.begin(), _facilities.end(), isCompleted()));

	facilities = (facilities / 6) + 9;
	shields = (shields * 2) + 1;
	Log(LOG_INFO) << ". facilities = " << facilities;
	Log(LOG_INFO) << ". shields = " << shields;

	int detchance = facilities / shields;
	Log(LOG_INFO) << ". detchance = " << detchance;

	return detchance;
}

/**
 *
 */
int Base::getGravShields() const
{
	int total = 0;
	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
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

	for (std::vector<BaseFacility*>::const_iterator i = _facilities.begin(); i != _facilities.end(); ++i)
	{
		if ((*i)->getBuildTime() == 0
			&& (*i)->getRules()->getDefenseValue())
		{
			_defenses.push_back(*i);
		}
	}

	_vehicles.clear();

	// add vehicles that are in the crafts of the base, if it's not out
	for (std::vector<Craft*>::iterator c = getCrafts()->begin(); c != getCrafts()->end(); ++c)
	{
		if ((*c)->getStatus() != "STR_OUT")
		{
			for (std::vector<Vehicle*>::iterator i = (*c)->getVehicles()->begin(); i != (*c)->getVehicles()->end(); ++i)
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
			{
				size = _rule->getArmor(_rule->getUnit(itemId)->getArmor())->getSize();
			}

			if (rule->getCompatibleAmmo()->empty()) // so this vehicle does not need ammo
			{
				for (int
						j = 0;
						j < iqty;
						++j) _vehicles.push_back(new Vehicle(rule, rule->getClipSize(), size));

				_items->removeItem(itemId, iqty);
			}
			else // so this vehicle needs ammo
			{
				RuleItem* ammo = _rule->getItem(rule->getCompatibleAmmo()->front());

				int baqty = _items->getItem(ammo->getType()); // Ammo Quantity for this vehicle-type on the base
				if (0 >= baqty || 0 >= iqty)
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

					if (j < remainder) ++newAmmo;

					_vehicles.push_back(new Vehicle(rule, newAmmo, size));
					_items->removeItem(ammo->getType(), newAmmo);
				}

				_items->removeItem(itemId, canBeAdded);
			}

			i = _items->getContents()->begin(); // we have to start over because iterator is broken because of the removeItem
		}
		else ++i;
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

}
