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

#ifndef OPENXCOM_RULEARMOR_H
#define OPENXCOM_RULEARMOR_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "MapData.h"
#include "RuleUnit.h"


namespace OpenXcom
{

enum ForcedTorso
{
	TORSO_USE_GENDER,	// 0
	TORSO_ALWAYS_MALE,	// 1
	TORSO_ALWAYS_FEMALE	// 2
};


/**
 * Represents a specific type of armor.
 * Not only soldier armor, but also alien armor - some alien
 * races wear Soldier Armor, Leader Armor or Commander Armor
 * depending on their rank.
 */
class RuleArmor
{

	public:
		static const size_t DAMAGE_TYPES = 10;


private:
	std::string
		_corpseGeo,
//		_specWeapon,
		_spriteSheet,
		_spriteInv,
		_storeItem,
		_type;
	bool
		_canHoldWeapon,
		_constantAnimation,
		_hasInventory,
		_isBasic,
		_isSpacesuit;
	int
		_agility,

		_deathFrames,
		_shootFrames,
		_firePhase,

		_frontArmor,
		_sideArmor,
		_rearArmor,
		_underArmor,

		_drawingRoutine,

		_size,
		_weight,

		_faceColorGroup,
		_hairColorGroup,
		_utileColorGroup,
		_rankColorGroup;

	float _damageModifier[DAMAGE_TYPES];

	ForcedTorso _forcedTorso;
	MovementType _moveType;
	UnitStats _stats;

	std::vector<int>
		_faceColor,
		_hairColor,
		_utileColor,
		_rankColor,
		_loftempsSet;
	std::vector<std::string> _corpseBattle;


	public:
		/// Creates a blank armor ruleset.
		explicit RuleArmor(const std::string& type);
		/// Cleans up the armor ruleset.
		~RuleArmor();

		/// Loads the armor data from YAML.
		void load(const YAML::Node& node);

		/// Gets the armor's type.
		std::string getType() const;

		/// Gets the unit's sprite sheet.
		std::string getSpriteSheet() const;
		/// Gets the unit's inventory sprite.
		std::string getSpriteInventory() const;

		/// Gets the front armor level.
		int getFrontArmor() const;
		/// Gets the side armor level.
		int getSideArmor() const;
		/// Gets the rear armor level.
		int getRearArmor() const;
		/// Gets the under armor level.
		int getUnderArmor() const;

		/// Gets the Geoscape corpse item.
		std::string getCorpseGeoscape() const;
		/// Gets the Battlescape corpse item.
		const std::vector<std::string>& getCorpseBattlescape() const;

		/// Gets the stores item.
		std::string getStoreItem() const;

		/// Gets the special weapon type.
//		std::string getSpecialWeapon() const;

		/// Gets the battlescape drawing routine ID.
		int getDrawingRoutine() const;

		/// Gets whether the armor can fly.
		/// DO NOT USE THIS FUNCTION OUTSIDE THE BATTLEUNIT CONSTRUCTOR OR I WILL HUNT YOU DOWN and kiss you.
		MovementType getMoveTypeArmor() const;

		/// Gets whether this is a normal or big unit.
		int getSize() const;

		/// Gets damage modifier.
		float getDamageModifier(ItemDamageType dType) const;

		/// Gets loftempSet
		const std::vector<int>& getLoftempsSet() const;

		/// Gets the armor's stats.
		const UnitStats* getStats() const;

		/// Gets the armor's weight.
		int getWeight() const;

		/// Gets number of death frames.
		int getDeathFrames() const;
		/// Gets number of shoot frames.
		int getShootFrames() const;
		/// Gets the frame of the armor's aiming-animation that first shows a projectile.
		int getFirePhase() const;

		/// Gets if armor uses constant animation.
		bool getConstantAnimation() const;

		/// Gets if armor can hold weapon.
		bool getCanHoldWeapon() const;

		/// Checks if this armor ignores gender (power suit/flying suit).
		ForcedTorso getForcedTorso() const;

		/// Gets face base color.
		int getFaceColorGroup() const;
		/// Gets hair base color.
		int getHairColorGroup() const;
		/// Get utile base color.
		int getUtileColorGroup() const;
		/// Get rank base color.
		int getRankColorGroup() const;
		/// Get face color.
		int getFaceColor(int i) const;
		/// Get hair color.
		int getHairColor(int i) const;
		/// Get utile color.
		int getUtileColor(int i) const;
		/// Get rank color.
		int getRankColor(int i) const;

		/// Checks if this Armor's inventory be accessed.
		bool hasInventory() const;

		/// Gets if this Armor is basic (lowest rank, standard issue wear).
		bool isBasic() const;
		/// Gets if this Armor is powered and suitable for Mars.
		bool isSpacesuit() const;

		/// Gets this Armor's agility used to determine stamina expenditure.
		int getAgility() const;
};

}

#endif
