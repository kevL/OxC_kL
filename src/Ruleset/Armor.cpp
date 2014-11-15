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

#include "Armor.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of armor.
 * @param type - reference string defining the type
 */
Armor::Armor(const std::string& type)
	:
		_type(type),
		_frontArmor(0),
		_sideArmor(0),
		_rearArmor(0),
		_underArmor(0),
		_drawingRoutine(0),
		_movementType(MT_WALK),
		_size(1),
		_weight(0),
		_deathFrames(3),
		_shootFrames(0),
		_constantAnimation(false),
		_canHoldWeapon(false),
		_forcedTorso(TORSO_USE_GENDER),
		_isBasic(false),
		_isPowered(false)
{
	_stats.tu			= 0;
	_stats.stamina		= 0;
	_stats.health		= 0;
	_stats.bravery		= 0;
	_stats.reactions	= 0;
	_stats.firing		= 0;
	_stats.throwing		= 0;
	_stats.strength		= 0;
	_stats.psiSkill		= 0;
	_stats.psiStrength	= 0;
	_stats.melee		= 0;

	for (int
			i = 0;
			i < DAMAGE_TYPES;
			i++)
	{
		_damageModifier[i] = 1.f;
	}
}

/**
 * dTor.
 */
Armor::~Armor()
{
}

/**
 * Loads the armor from a YAML file.
 * @param node - reference a YAML node
 */
void Armor::load(const YAML::Node& node)
{
	_type			= node["type"]						.as<std::string>(_type);
	_spriteSheet	= node["spriteSheet"]				.as<std::string>(_spriteSheet);
	_spriteInv		= node["spriteInv"]					.as<std::string>(_spriteInv);

	if (node["corpseItem"])
	{
		_corpseBattle.clear();
		_corpseBattle.push_back(node["corpseItem"]		.as<std::string>());
		_corpseGeo		= _corpseBattle[0];
	}
	else if (node["corpseBattle"])
	{
		_corpseBattle	= node["corpseBattle"]			.as<std::vector<std::string> >();
		_corpseGeo		= _corpseBattle[0];
	}
	_corpseGeo		= node["corpseGeo"]					.as<std::string>(_corpseGeo);

	_storeItem		= node["storeItem"]					.as<std::string>(_storeItem);
	_frontArmor		= node["frontArmor"]				.as<int>(_frontArmor);
	_sideArmor		= node["sideArmor"]					.as<int>(_sideArmor);
	_rearArmor		= node["rearArmor"]					.as<int>(_rearArmor);
	_underArmor		= node["underArmor"]				.as<int>(_underArmor);
	_drawingRoutine	= node["drawingRoutine"]			.as<int>(_drawingRoutine);
	_movementType	= (MovementType)node["movementType"].as<int>(_movementType);
	_size			= node["size"]						.as<int>(_size);
	_weight			= node["weight"]					.as<int>(_weight);
	_isBasic		= node["isBasic"]					.as<bool>(_isBasic);
	_isPowered		= node["isPowered"]					.as<bool>(_isPowered);

	_stats.merge(node["stats"].as<UnitStats>(_stats));

	if (const YAML::Node& dmg = node["damageModifier"])
	{
		for (size_t
				i = 0;
				i < dmg.size()
					&& i < DAMAGE_TYPES;
				++i)
		{
			_damageModifier[i] = dmg[i]					.as<float>();
		}
	}

	_loftempsSet = node["loftempsSet"]					.as<std::vector<int> >(_loftempsSet);
	if (node["loftemps"])
		_loftempsSet.push_back(node["loftemps"]			.as<int>());

	_deathFrames = node["deathFrames"]					.as<int>(_deathFrames);
	_shootFrames = node["shootFrames"]					.as<int>(_shootFrames);
	_constantAnimation = node["constantAnimation"]		.as<bool>(_constantAnimation);

	_forcedTorso = (ForcedTorso)node["forcedTorso"]		.as<int>(_forcedTorso);

	if (_drawingRoutine == 0
		|| _drawingRoutine == 1
		|| _drawingRoutine == 4
		|| _drawingRoutine == 6
		|| _drawingRoutine == 10
		|| _drawingRoutine == 13
		|| _drawingRoutine == 14
		|| _drawingRoutine == 15
		|| _drawingRoutine == 17
		|| _drawingRoutine == 18)
	{
		_canHoldWeapon = true;
	}
	else
		_canHoldWeapon = false;
}

/**
 * Returns the language string that names this armor.
 * Each armor has a unique name.
 * @return, armor name
 */
std::string Armor::getType() const
{
	return _type;
}

/**
 * Gets the unit's sprite sheet.
 * @return, sprite sheet name
 */
std::string Armor::getSpriteSheet() const
{
	return _spriteSheet;
}

/**
 * Gets the unit's inventory sprite.
 * @return, inventory sprite name
 */
std::string Armor::getSpriteInventory() const
{
	return _spriteInv;
}

/**
 * Gets the front armor level.
 * @return, front armor level
 */
int Armor::getFrontArmor() const
{
	return _frontArmor;
}

/**
 * Gets the side armor level.
 * @return, side armor level
 */
int Armor::getSideArmor() const
{
	return _sideArmor;
}

/**
 * Gets the rear armor level.
 * @return, rear armor level
 */
int Armor::getRearArmor() const
{
	return _rearArmor;
}

/**
 * Gets the under armor level.
 * @return, under armor level
 */
int Armor::getUnderArmor() const
{
	return _underArmor;
}

/**
 * Gets the corpse item used in the Geoscape.
 * @return, name of the corpse item
 */
std::string Armor::getCorpseGeoscape() const
{
	return _corpseGeo;
}

/**
 * Gets the list of corpse items dropped by the unit
 * in the Battlescape (one per unit tile).
 * @return, list of corpse items
 */
const std::vector<std::string>& Armor::getCorpseBattlescape() const
{
	return _corpseBattle;
}

/**
 * Gets the storage item needed to equip this.
 * @return, name of the store item
 */
std::string Armor::getStoreItem() const
{
	return _storeItem;
}

/**
 * Gets the drawing routine ID.
 * @return, drawing routine ID
 */
int Armor::getDrawingRoutine() const
{
	return _drawingRoutine;
}

/**
 * Gets the movement type of this armor.
 * Useful for determining whether the armor can fly.
 * @important: do not use this function outside the BattleUnit constructor,
 * unless you are SURE you know what you are doing.
 * For more information, see the BattleUnit constructor.
 * @return, MovementType enum
 */
MovementType Armor::getMovementType() const
{
	return _movementType;
}

/**
 * Gets the size of this armor. Normally 1 (small) or 2 (big).
 * @return, size
 */
int Armor::getSize() const
{
	return _size;
}

/**
 * Gets the damage modifier for a certain damage type.
 * @param dt - the damageType
 * @return, damage modifier ( 0.f to 1.f+ )
 */
float Armor::getDamageModifier(ItemDamageType dType)
{
	return _damageModifier[static_cast<int>(dType)];
}

/** Gets the Line Of Fire Template set.
 * @return, loftempsSet as a vector of ints
 */
std::vector<int> Armor::getLoftempsSet() const
{
	return _loftempsSet;
}

/**
  * Gets pointer to the armor's stats.
  * @return, pointer to the armor's UnitStats
  */
UnitStats* Armor::getStats()
{
	return &_stats;
}

/**
 * Gets the armor's weight.
 * @return, the weight of the armor
 */
int Armor::getWeight() const
{
	return _weight;
}

/**
 * Gets number of death frames.
 * @return, number of death frames
 */
int Armor::getDeathFrames() const
{
	return _deathFrames;
}

/**
 * Gets number of shoot frames.
 * @return, number of shoot frames
 */
int Armor::getShootFrames() const
{
	return _shootFrames;
}

/**
 * Gets if armor uses constant animation.
 * @return, true if it uses constant animation
 */
bool Armor::getConstantAnimation() const
{
	return _constantAnimation;
}

/**
 * Gets if armor can hold weapon.
 * @return, true if it can hold weapon
 */
bool Armor::getCanHoldWeapon() const
{
	return _canHoldWeapon;
}

/**
 * Checks if this armor ignores gender (power suit/flying suit).
 * @return, the torso to force on the sprite
 */
ForcedTorso Armor::getForcedTorso() const
{
	return _forcedTorso;
}

/**
 * Gets if this Armor is basic (lowest rank, standard issue wear).
 * @return, true if basic
 */
bool Armor::isBasic() const
{
	return _isBasic;
}

/**
 * Gets if this Armor is powered and suitable for Mars.
 * @return, true if life-supporting
 */
bool Armor::isPowered() const
{
	return _isPowered;
}

}
