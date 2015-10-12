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

#include "RuleArmor.h"

#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of armor.
 * @param type - reference string defining the type
 */
RuleArmor::RuleArmor(const std::string& type)
	:
		_type(type),
		_frontArmor(0),
		_sideArmor(0),
		_rearArmor(0),
		_underArmor(0),
		_drawingRoutine(0),
		_moveType(MT_WALK),
		_size(1),
		_weight(0),
		_deathFrames(3),
		_shootFrames(0),
		_firePhase(0),
		_constantAnimation(false),
		_canHoldWeapon(false),
		_hasInventory(true),
		_forcedTorso(TORSO_USE_GENDER),
		_faceColorGroup(0),
		_hairColorGroup(0),
		_utileColorGroup(0),
		_rankColorGroup(0),
		_isBasic(false),
		_isSpacesuit(false),
		_agility(0)
{
	_stats.tu			=
	_stats.stamina		=
	_stats.health		=
	_stats.bravery		=
	_stats.reactions	=
	_stats.firing		=
	_stats.throwing		=
	_stats.strength		=
	_stats.psiSkill		=
	_stats.psiStrength	=
	_stats.melee		= 0;

	for (size_t
			i = 0;
			i != DAMAGE_TYPES;
			++i)
	{
		_damageModifier[i] = 1.f;
	}

	_faceColor.resize((static_cast<size_t>(LOOK_AFRICAN) + 1) * 2);
	_hairColor.resize((static_cast<size_t>(LOOK_AFRICAN) + 1) * 2);
}

/**
 * dTor.
 */
RuleArmor::~RuleArmor()
{}

/**
 * Loads the armor from a YAML file.
 * @param node - reference a YAML node
 */
void RuleArmor::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_spriteSheet	= node["spriteSheet"]	.as<std::string>(_spriteSheet);
	_spriteInv		= node["spriteInv"]		.as<std::string>(_spriteInv);
	_hasInventory	= node["allowInv"]		.as<bool>(_hasInventory);

	if (node["corpseBattle"])
	{
		_corpseBattle	= node["corpseBattle"]	.as<std::vector<std::string> >();
		_corpseGeo		= _corpseBattle[0];
	}
	_corpseGeo		= node["corpseGeo"]			.as<std::string>(_corpseGeo);

	_storeItem		= node["storeItem"]		.as<std::string>(_storeItem);
//	_specWeapon		= node["specialWeapon"]	.as<std::string>(_specWeapon);
	_frontArmor		= node["frontArmor"]	.as<int>(_frontArmor);
	_sideArmor		= node["sideArmor"]		.as<int>(_sideArmor);
	_rearArmor		= node["rearArmor"]		.as<int>(_rearArmor);
	_underArmor		= node["underArmor"]	.as<int>(_underArmor);
	_drawingRoutine	= node["drawingRoutine"].as<int>(_drawingRoutine);
	_size			= node["size"]			.as<int>(_size);
	_weight			= node["weight"]		.as<int>(_weight);
	_agility		= node["agility"]		.as<int>(_agility);
	_isBasic		= node["isBasic"]		.as<bool>(_isBasic);
	_isSpacesuit	= node["isSpacesuit"]	.as<bool>(_isSpacesuit);
	_moveType		= static_cast<MovementType>(node["movementType"].as<int>(_moveType));

	_stats.mergeStats(node["stats"].as<UnitStats>(_stats));

	if (const YAML::Node& vuln = node["damageModifier"])
	{
		for (size_t
				i = 0;
				i < vuln.size()
					&& i < DAMAGE_TYPES;
				++i)
		{
			_damageModifier[i] = vuln[i].as<float>();
		}
	}

	_loftSet			= node["loftSet"]			.as<std::vector<size_t> >(_loftSet);
	_deathFrames		= node["deathFrames"]		.as<int>(_deathFrames);
	_shootFrames		= node["shootFrames"]		.as<int>(_shootFrames);
	_firePhase			= node["firePhase"]			.as<int>(_firePhase);
	_constantAnimation	= node["constantAnimation"]	.as<bool>(_constantAnimation);

	_forcedTorso = static_cast<ForcedTorso>(node["forcedTorso"].as<int>(_forcedTorso));

	if (   _drawingRoutine ==  0
		|| _drawingRoutine ==  1
		|| _drawingRoutine ==  4
		|| _drawingRoutine ==  6
		|| _drawingRoutine == 10)
	{
		_canHoldWeapon = true;
	}

	_faceColorGroup		= node["spriteFaceGroup"]	.as<int>(_faceColorGroup);
	_hairColorGroup		= node["spriteHairGroup"]	.as<int>(_hairColorGroup);
	_rankColorGroup		= node["spriteRankGroup"]	.as<int>(_rankColorGroup);
	_utileColorGroup	= node["spriteUtileGroup"]	.as<int>(_utileColorGroup);
	_faceColor			= node["spriteFaceColor"]	.as<std::vector<int> >(_faceColor);
	_hairColor			= node["spriteHairColor"]	.as<std::vector<int> >(_hairColor);
	_rankColor			= node["spriteRankColor"]	.as<std::vector<int> >(_rankColor);
	_utileColor			= node["spriteUtileColor"]	.as<std::vector<int> >(_utileColor);
}

/**
 * Returns the language string that names this armor.
 * Each armor has a unique name.
 * @return, armor name
 */
std::string RuleArmor::getType() const
{
	return _type;
}

/**
 * Gets the unit's sprite sheet.
 * @return, sprite sheet name
 */
std::string RuleArmor::getSpriteSheet() const
{
	return _spriteSheet;
}

/**
 * Gets the unit's inventory sprite.
 * @return, inventory sprite name
 */
std::string RuleArmor::getSpriteInventory() const
{
	return _spriteInv;
}

/**
 * Gets the front armor level.
 * @return, front armor level
 */
int RuleArmor::getFrontArmor() const
{
	return _frontArmor;
}

/**
 * Gets the side armor level.
 * @return, side armor level
 */
int RuleArmor::getSideArmor() const
{
	return _sideArmor;
}

/**
 * Gets the rear armor level.
 * @return, rear armor level
 */
int RuleArmor::getRearArmor() const
{
	return _rearArmor;
}

/**
 * Gets the under armor level.
 * @return, under armor level
 */
int RuleArmor::getUnderArmor() const
{
	return _underArmor;
}

/**
 * Gets the corpse item used in the Geoscape.
 * @return, name of the corpse item
 */
std::string RuleArmor::getCorpseGeoscape() const
{
	return _corpseGeo;
}

/**
 * Gets the list of corpse items dropped by the unit
 * in the Battlescape (one per unit tile).
 * @return, list of corpse items
 */
const std::vector<std::string>& RuleArmor::getCorpseBattlescape() const
{
	return _corpseBattle;
}

/**
 * Gets the storage item needed to equip this.
 * @return, name of the store item
 */
std::string RuleArmor::getStoreItem() const
{
	return _storeItem;
}

/**
 * Gets the type of special weapon.
 * @return, the name of the special weapon
 */
/* std::string RuleArmor::getSpecialWeapon() const
{
	return _specWeapon;
} */

/**
 * Gets the drawing routine ID.
 * @return, drawing routine ID
 */
int RuleArmor::getDrawingRoutine() const
{
	return _drawingRoutine;
}

/**
 * Gets the movement type of this armor.
 * @note Useful for determining whether the armor can fly.
 * @important: do not use this function outside the BattleUnit constructor
 * unless you are SURE you know what you are doing.
 * For more information see the BattleUnit constructor.
 * @return, MovementType enum
 */
MovementType RuleArmor::getMoveTypeArmor() const
{
	return _moveType;
}

/**
 * Gets the size of this armor. Normally 1 (small) or 2 (big).
 * @return, size
 */
int RuleArmor::getSize() const
{
	return _size;
}

/**
 * Gets the damage modifier for a certain damage type.
 * @param dt - the damageType
 * @return, damage modifier (0.f to 1.f+)
 */
float RuleArmor::getDamageModifier(DamageType dType) const
{
	return _damageModifier[static_cast<size_t>(dType)];
}

/**
 * Gets the Line Of Fire Template set.
 * @return, address of loftSet as a vector of templates
 */
const std::vector<size_t>& RuleArmor::getLoftSet() const
{
	return _loftSet;
}

/**
  * Gets pointer to the armor's stats.
  * @return, pointer to the armor's UnitStats
  */
const UnitStats* RuleArmor::getStats() const
{
	return &_stats;
}

/**
 * Gets the armor's weight.
 * @return, the weight of the armor
 */
int RuleArmor::getWeight() const
{
	return _weight;
}

/**
 * Gets number of death frames.
 * @return, number of death frames
 */
int RuleArmor::getDeathFrames() const
{
	return _deathFrames;
}

/**
 * Gets number of shoot frames.
 * @return, number of shoot frames
 */
int RuleArmor::getShootFrames() const
{
	return _shootFrames;
}

/**
 * Gets the frame of the armor's aiming-animation that first shows a projectile.
 * @return, shoot frame that reveals projectile
 */
int RuleArmor::getFirePhase() const
{
	return _firePhase;
}

/**
 * Gets if armor uses constant animation.
 * @return, true if it uses constant animation
 */
bool RuleArmor::getConstantAnimation() const
{
	return _constantAnimation;
}

/**
 * Gets if armor can hold weapon.
 * @return, true if it can hold weapon
 */
bool RuleArmor::getCanHoldWeapon() const
{
	return _canHoldWeapon;
}

/**
 * Checks if this armor ignores gender (power suit/flying suit).
 * @return, the torso to force on the sprite
 */
ForcedTorso RuleArmor::getForcedTorso() const
{
	return _forcedTorso;
}

/**
 * Gets hair base color group for replacement (0 dont replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getFaceColorGroup() const
{
	return _faceColorGroup;
}

/**
 * Gets hair base color group for replacement (0 dont replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getHairColorGroup() const
{
	return _hairColorGroup;
}

/**
 * Gets utile base color group for replacement (0 dont replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getUtileColorGroup() const
{
	return _utileColorGroup;
}

/**
 * Gets rank base color group for replacement (0 dont replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getRankColorGroup() const
{
	return _rankColorGroup;
}

/**
 * Gets new face colors for replacement (0 dont replace).
 * @return, colorindex or 0
 */
int RuleArmor::getFaceColor(int i) const
{
	const size_t foff = static_cast<size_t>(i);
	if (foff < _faceColor.size())
		return _faceColor[foff];

	return 0;
}

/**
 * Gets new hair colors for replacement (0 dont replace).
 * @return, colorindex or 0
 */
int RuleArmor::getHairColor(int i) const
{
	const size_t foff = static_cast<size_t>(i);
	if (foff < _hairColor.size())
		return _hairColor[foff];

	return 0;
}

/**
 * Gets new utile colors for replacement (0 dont replace).
 * @return, colorindex or 0
 */
int RuleArmor::getUtileColor(int i) const
{
	const size_t foff = static_cast<size_t>(i);
	if (foff < _utileColor.size())
		return _utileColor[foff];

	return 0;
}

/**
 * Gets new rank colors for replacement (0 dont replace).
 * @return, colorindex or 0
 */
int RuleArmor::getRankColor(int i) const
{
	const size_t foff = static_cast<size_t>(i);
	if (foff < _rankColor.size())
		return _rankColor[foff];

	return 0;
}

/**
 * Checks if this RuleArmor's inventory be accessed.
 * @return, true if inventory can be opened by player
 */
bool RuleArmor::hasInventory() const
{
	return _hasInventory;
}

/**
 * Gets if this RuleArmor is basic.
 * @note True denotes armor of the lowest basic standard issue wear. It does not
 * require space in Base Stores and is identical to Armors that have
 * 'storeItem' = "STR_NONE". Its cost is zero.
 * @return, true if basic
 */
bool RuleArmor::isBasic() const
{
	return _isBasic;
}

/**
 * Gets if this RuleArmor is powered and suitable for Mars.
 * @return, true if life-supporting
 */
bool RuleArmor::isSpacesuit() const
{
	return _isSpacesuit;
}

/**
 * Gets this Armor's agility used to determine stamina expenditure.
 * Higher values equate to less energy cost. Typically:
 *		0 personal armor
 *		1 no armor
 *		2 powered/flight suits
 * @note Armor cannot subtract more energy than a tile requires due to coding.
 * @return, agility
 */
int RuleArmor::getAgility() const
{
	return _agility;
}

}
