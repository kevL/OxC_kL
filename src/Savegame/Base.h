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

#ifndef OPENXCOM_BASE_H
#define OPENXCOM_BASE_H

//#include <cstdint>
//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

class BaseFacility;
class Craft;
class ItemContainer;
class Language;
class Production;
class ResearchProject;
class Ruleset;
class SavedGame;
class Soldier;
class Transfer;
class Vehicle;


/**
 * Represents a player base on the globe.
 * Bases can contain facilities, personnel, crafts and equipment.
 */
class Base
	:
		public Target
{

private:
	static const int BASE_SIZE = 6;

	bool
		_inBattlescape,
		_retaliationTarget;
	int
		_cashSpent, // kL
		_cashIncome, // kL
		_engineers,
		_scientists,
		_defenseResult;
	size_t _curSoldier; // kL

	ItemContainer* _items;
	const Ruleset* const _rules;

	std::wstring _name;

	std::vector<BaseFacility*>
		_defenses,
		_facilities;
	std::vector<Craft*> _crafts;
	std::vector<Production*> _productions;
	std::vector<ResearchProject*> _research;
	std::vector<Soldier*> _soldiers;
	std::vector<Transfer*> _transfers;
	std::vector<Vehicle*> _vehicles;

	/// Determines space taken up by ammo clips about to rearm craft.
//	double getIgnoredStores();


	public:
		/// Creates a new base.
		Base(const Ruleset* rules);
		/// Cleans up the base.
		~Base();

		/// Loads the base from YAML.
		void load(
				const YAML::Node& node,
				SavedGame* save,
				bool newGame,
				bool newBattleGame = false);
		/// Saves the base to YAML.
		YAML::Node save() const;
		/// Saves the base's ID to YAML.
		YAML::Node saveId() const;

		/// Gets the base's name.
		std::wstring getName(Language* lang = NULL) const;
		/// Sets the base's name.
		void setName(const std::wstring& name);
		/// Gets the base's marker.
		int getMarker() const;

		/// Gets the base's facilities.
		std::vector<BaseFacility*>* getFacilities();
		/// Gets the base's soldiers.
		std::vector<Soldier*>* getSoldiers();
		/// Gets the base's crafts.
		std::vector<Craft*>* getCrafts();
		/// Gets the base's transfers.
		std::vector<Transfer*>* getTransfers();

		/// Gets the base's items.
		ItemContainer* getItems();

		/// Gets the base's scientists.
		int getScientists() const;
		/// Sets the base's scientists.
		void setScientists(int scientists);
		/// Gets the base's engineers.
		int getEngineers() const;
		/// Sets the base's engineers.
		void setEngineers(int engineers);

		/// Checks if a target is detected by the base's radar.
		int detect(Target* target) const;
		/// Checks if a target is inside the base's radar range.
		double insideRadarRange(Target* target) const;

		/// Gets the base's available soldiers.
		int getAvailableSoldiers(const bool combatReady = false) const;
		/// Gets the base's total soldiers.
		int getTotalSoldiers() const;
		/// Gets the base's available scientists.
//kL	int getAvailableScientists() const;
		/// Gets the base's total scientists.
		int getTotalScientists() const;
		/// Gets the base's available engineers.
//kL	int getAvailableEngineers() const;
		/// Gets the base's total engineers.
		int getTotalEngineers() const;

		/// Gets the base's used living quarters.
		int getUsedQuarters() const;
		/// Gets the base's available living quarters.
		int getAvailableQuarters() const;

		/// Gets the base's used storage space.
		double getUsedStores();
		/// Checks if the base's stores are overfull.
		bool storesOverfull(double offset = 0.);

		/// Gets the base's available storage space.
		int getAvailableStores() const;

		/// Gets the base's used laboratory space.
		int getUsedLaboratories() const;
		/// Gets the base's available laboratory space.
		int getAvailableLaboratories() const;

		/// Gets the base's used workshop space.
		int getUsedWorkshops() const;
		/// Gets the base's available workshop space.
		int getAvailableWorkshops() const;

		/// Gets the base's used hangars.
		int getUsedHangars() const;
		/// Gets the base's available hangars.
		int getAvailableHangars() const;

		/// Gets the number of available space lab (not used by a ResearchProject).
		int getFreeLaboratories() const;
		/// Gets the number of available space lab (not used by a Production).
		int getFreeWorkshops() const;

		///
		int getAllocatedScientists() const;
		///
		int getAllocatedEngineers() const;

		/// Gets the base's defense value.
		int getDefenseValue() const;

		/// Gets the base's short range detection.
//kL	int getShortRangeDetection() const;
		/// kL. Gets the base's short range detection value.
		int getShortRangeValue() const; // kL
		/// Gets the base's long range detection.
//kL	int getLongRangeDetection() const;
		/// kL. Gets the base's long range detection.
		int getLongRangeValue() const; // kL

		/// Gets the base's crafts of a certain type.
		int getCraftCount(const std::string& craft) const;
		/// Gets the base's craft maintenance.
		int getCraftMaintenance() const;
		/// Gets the base's personnel maintenance.
		int getPersonnelMaintenance() const;
		/// Gets the base's facility maintenance.
		int getFacilityMaintenance() const;
		/// Gets the base's total monthly maintenance.
		int getMonthlyMaintenace() const;

		/// Gets the list of base's ResearchProject.
		const std::vector<ResearchProject*>& getResearch() const;
		/// Adds a new ResearchProject to the Base.
		void addResearch(ResearchProject* project);
		/// Removes a ResearchProject from the Base.
		void removeResearch(
				ResearchProject* project,
				bool grantHelp = true,		// kL
				bool goOffline = false);	// kL
		/// kL. Research Help ala XcomUtil.
		void researchHelp(const std::string& aLien); // kL

		/// Adds a new Production to Base.
		void addProduction(Production* prod);
		/// Removes a Base's Production.
		void removeProduction(Production* prod);
		/// Gets the list of Base's Production.
		const std::vector<Production*>& getProductions() const;

		/// Checks if this base is hyper-wave equipped.
		bool getHyperDetection() const;
		/// Checks if this base has research facilities.
		bool hasResearch() const;
		/// Checks if this base has production facilities.
		bool hasProduction() const;

		/// Gets the base's used psi lab space.
		int getUsedPsiLabs() const;
		/// Gets the base's total available psi lab space.
		int getAvailablePsiLabs() const;
		/// Gets the base's total free psi lab space.
		int getFreePsiLabs() const;

		/// Gets the total amount of Containment Space.
		int getAvailableContainment() const;
		/// Gets the total amount of used Containment Space.
		int getUsedContainment() const;

		/// Sets the craft's battlescape status.
		void setInBattlescape(bool inBattle);
		/// Gets if the craft is in battlescape.
		bool isInBattlescape() const;

		/// Marks this base for alien retaliation.
		void setIsRetaliationTarget(bool mark = true);
		/// Gets the retaliation status of this base.
		bool getIsRetaliationTarget() const;

		/// Gets the detection chance for this base.
		int getDetectionChance(int diff) const;

		/// Gets how many Grav Shields the base has.
		int getGravShields() const;
		/// Sets up base defenses.
		void setupDefenses();
		/// Gets a list of Defensive Facilities
		std::vector<BaseFacility*>* getDefenses();

		/// Gets the base's vehicles.
		std::vector<Vehicle*>* getVehicles();

		/// Destroys all disconnected facilities in the base.
		void destroyDisconnectedFacilities();
		/// Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
		std::list<std::vector<BaseFacility*>::const_iterator> getDisconnectedFacilities(BaseFacility* remFac);
		/// Destroys a facility and deals with the side effects.
		void destroyFacility(std::vector<BaseFacility*>::const_iterator fac);

		/// Cleans up base defenses vector after a Ufo attack and optionally reclaims the tanks and their ammo.
		void cleanupDefenses(bool reclaimItems);

		/// Increases (or decreases) the base's total income amount.
		void setCashIncome(int cash);
		/// Gets the base's total income amount.
		int getCashIncome() const;
		/// Increases (or decreases) the base's total spent amount.
		void setCashSpent(int cash);
		/// Gets the base's total spent amount.
		int getCashSpent() const;

		/// Sets the result of a base's defense against aLien attacks.
		void setDefenseResult(int result);
		/// Gets the result of a base's defense against aLien attacks.
		int getDefenseResult() const;

		/// Sets the current row of Soldiers list.
		void setCurrentSoldier(const size_t row);
		/// Gets the current row of Soldiers list.
		size_t getCurrentSoldier() const;
};

}

#endif
