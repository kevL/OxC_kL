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

#include "UnitSprite.h"

//#define _USE_MATH_DEFINES
//#include <cmath>

#include "../Battlescape/Position.h"

//#include "../Engine/Options.h"
//#include "../Engine/ShaderDraw.h"
//#include "../Engine/ShaderMove.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels
 * @param y			- Y position in pixels
 * @param helmet	- true if unit is wearing a helmet (TFTD)
 * @param moveType	- the MovementType ()
 */
UnitSprite::UnitSprite(
		int width,
		int height,
		int x,
		int y,
		bool helmet,
		MovementType moveType)
	:
		Surface(
			width,
			height,
			x,y),
		_helmet(helmet),
		_moveType(moveType),
		_unit(NULL),
		_itemA(NULL),
		_itemB(NULL),
		_unitSurface(NULL),
		_itemSurfaceA(NULL),
		_itemSurfaceB(NULL),
		_part(0),
		_animationFrame(0),
		_drawingRoutine(0),
		_color(NULL),
		_colorSize(0)
{}

/**
 * Deletes the UnitSprite.
 */
UnitSprite::~UnitSprite()
{}

/**
 * Changes the surface sets for the UnitSprite to get resources for rendering.
 * @param unitSurface	- pointer to a unit's SurfaceSet
 * @param itemSurfaceA	- pointer to an item's SurfaceSet
 * @param itemSurfaceB	- pointer to an item's SurfaceSet
 */
void UnitSprite::setSurfaces(
		SurfaceSet* const unitSurface,
		SurfaceSet* const itemSurfaceA,
		SurfaceSet* const itemSurfaceB)
{
	_unitSurface = unitSurface;
	_itemSurfaceA = itemSurfaceA;
	_itemSurfaceB = itemSurfaceB;

	_redraw = true;
}

/**
 * Links this sprite to a BattleUnit to get the data for rendering.
 * @param unit - pointer to a BattleUnit
 * @param part - part number for large units (default 0)
 */
void UnitSprite::setBattleUnit(
		BattleUnit* const unit,
		int part)
{
	_drawingRoutine = unit->getArmor()->getDrawingRoutine();

	_unit = unit;
	_part = part;

	_redraw = true;

	if (Options::battleHairBleach == true)
	{
		_colorSize = static_cast<int>(_unit->getRecolor().size());
		if (_colorSize != 0)
			_color = &(_unit->getRecolor()[0]);
		else
			_color = NULL;
	}
}

/**
 * Links this sprite to a BattleItem to get the data for rendering.
 * @param item - pointer to a BattleItem
 */
void UnitSprite::setBattleItem(BattleItem* const item)
{
	if (item != NULL)
	{
		if (item->getSlot()->getId() == "STR_RIGHT_HAND")
			_itemA = item;

		if (item->getSlot()->getId() == "STR_LEFT_HAND")
			_itemB = item;
	}

	_redraw = true;
}


namespace
{

struct ColorReplace
{
	static const Uint8
		ColorGroup = 15 << 4,
		ColorShade = 15;

	///
	static inline bool loop(
			Uint8& dest,
			const Uint8& src,
			const std::pair<Uint8, Uint8>& face_color)
	{
		if ((src & ColorGroup) == face_color.first)
		{
			dest = face_color.second + (src & ColorShade);
			return true;
		}

		return false;
	}

	///
	static inline void func(
			Uint8& dest,
			const Uint8& src,
			const std::pair<Uint8, Uint8>* color,
			int colors,
			int)
	{
		if (src != 0)
		{
			for (size_t
					i = 0;
					i != static_cast<size_t>(colors);
					++i)
			{
				if (loop(
						dest,
						src,
						color[i]) == true)
				{
					return;
				}
			}
			dest = src;
		}
	}
};

}


void UnitSprite::drawRecolored(Surface* src)
{
	if (_colorSize != 0)
	{
		lock();
		ShaderDraw<ColorReplace>(
							ShaderSurface(this),
							ShaderSurface(src),
							ShaderScalar(_color),
							ShaderScalar(_colorSize));
		unlock();
	}
	else
		src->blit(this);
}

/**
 * Sets the animation frame for animated units.
 * @param frame - frame number
 */
void UnitSprite::setAnimationFrame(int frame)
{
	_animationFrame = frame;
}

/**
 * Draws a unit, using the drawing rules of the unit.
 * This function is called by Map for each unit on the screen.
 */
void UnitSprite::draw()
{
	//Log(LOG_INFO) << "UnitSprite::draw() Routine " << _drawingRoutine;
	Surface::draw();

	void (UnitSprite::*routines[])() = // Array of drawing routines.
	{
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine1,
		&UnitSprite::drawRoutine2,
		&UnitSprite::drawRoutine3,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine5,
		&UnitSprite::drawRoutine6,
		&UnitSprite::drawRoutine7,
		&UnitSprite::drawRoutine8,
		&UnitSprite::drawRoutine9,
		&UnitSprite::drawRoutine0
	};
/*		&UnitSprite::drawRoutine11,
		&UnitSprite::drawRoutine12,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine12,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine19,
		&UnitSprite::drawRoutine20,
		&UnitSprite::drawRoutine21}; */

	(this->*(routines[_drawingRoutine]))(); // Call the matching routine.
}

/**
 * Drawing routine for XCom soldiers in overalls, sectoids (routine 0),
 * mutons (routine 10),
 * aquanauts (routine 13),
 * calcinites, deep ones, gill men, lobster men, tasoths (routine 14),
 * aquatoids (routine 15) (this one is no different, it just precludes breathing animations).
 */
void UnitSprite::drawRoutine0()
{
	//Log(LOG_INFO) << "** UnitSprite::drawRoutine0()";
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface
			* torso		= NULL,
			* legs		= NULL,
			* leftArm	= NULL,
			* rightArm	= NULL,
			* itemA		= NULL,
			* itemB		= NULL;

	// magic numbers
	int torsoHandsWeaponY = 0; // kL
/*	const int legsStand = 16, legsKneel = 24;
	int maleTorso, femaleTorso, die, rarm1H, larm2H, rarm2H, rarmShoot, legsFloat, torsoHandsWeaponY = 0;
	if (_drawingRoutine <= 10)
	{
		die = 264; // ufo:eu death frame
		maleTorso = 32;
		femaleTorso = 267;
		rarm1H = 232;
		larm2H = 240;
		rarm2H = 248;
		rarmShoot = 256;
		legsFloat = 275;
	}
	else if (_drawingRoutine == 13)
	{ */
/*		if (_helmet)
		{
			die = 259; // aquanaut underwater death frame
			maleTorso = 32; // aquanaut underwater ion armour torso
			if (_unit->getArmor()->getForcedTorso() == TORSO_USE_GENDER)
			{
				femaleTorso = 32; // aquanaut underwater plastic aqua armour torso
			}
			else
			{
				femaleTorso = 286; // aquanaut underwater magnetic ion armour torso
			}
			rarm1H = 248;
			larm2H = 232;
			rarm2H = rarmShoot = 240;
			legsFloat = 294;
		}*/
//		else
/*		{
			die = 256; // aquanaut land death frame
			// aquanaut land torso
			maleTorso = 270;
			femaleTorso = 262;
			rarm1H = 248;
			larm2H = 232;
			rarm2H = rarmShoot = 240;
			legsFloat = 294;
		}
	}
	else
	{
		die = 256; // tftd unit death frame
		// tftd unit torso
		maleTorso = 32;
		femaleTorso = 262;
		rarm1H = 248;
		larm2H = 232;
		rarm2H = rarmShoot = 240;
		legsFloat = 294;
	}
	const int larmStand = 0, rarmStand = 8;
*/
// pre-TFTD:
	const int
		maleTorso	=  32,
		femaleTorso	= 267,
		legsStand	=  16,
		legsKneel	=  24,
		die			= 264,
		legsFloat	= 275;
	const int
		larmStand	=   0,
		rarmStand	=   8,
		rarm1H		= 232,
		larm2H		= 240,
		rarm2H		= 248,
		rarmShoot	= 256;
	const int
// direction:           0      1      2        3        4        5        6        7
// #firstFrame:        56     80    104      128      152      176      200      224
		legsWalk[8] = {56, 56+24, 56+24*2, 56+24*3, 56+24*4, 56+24*5, 56+24*6, 56+24*7},
// #firstFrame:        40     64     88      112      136      160      184      208
		larmWalk[8] = {40, 40+24, 40+24*2, 40+24*3, 40+24*4, 40+24*5, 40+24*6, 40+24*7},
// #firstFrame:        48     72     96      120      144      168      192      216
		rarmWalk[8] = {48, 48+24, 48+24*2, 48+24*3, 48+24*4, 48+24*5, 48+24*6, 48+24*7};

	const int aquatoidYoffWalk[8] = {1, 0, 0, 1, 2, 1, 0, 0}; // bobbing up and down (aquatoid)

	const int yoffWalk[8]		= { 1,  0, -1,  0,  1,  0, -1,  0}; // bobbing up and down
	const int yoffWalk_mut[8]	= { 1,  1,  0,  0,  1,  1,  0,  0}; // bobbing up and down (muton)
//	const int yoffWalk_mut[8]	= { 0,  0,  0,  0,  0,  0,  0,  0}; // kL_TEST.
	const int offX[8]			= { 8, 10,  7,  4, -9,-11, -7, -3}; // for the weapons
	const int offY[8]			= {-6, -3,  0,  2,  0, -4, -7, -9}; // for the weapons
	const int offX2[8]			= {-8,  3,  5, 12,  6, -1, -5,-13}; // for the left handed weapons
	const int offY2[8]			= { 1, -4, -2,  0,  3,  3,  5,  0}; // for the left handed weapons
	const int offX3[8]			= { 0,  0,  2,  2,  0,  0,  0,  0}; // for the weapons (muton)
	const int offY3[8]			= {-3, -3, -1, -1, -1, -3, -3, -2}; // for the weapons (muton)
	const int offX4[8]			= {-8,  2,  7, 14,  7, -2, -4, -8}; // for the left handed weapons
	const int offY4[8]			= {-3, -3, -1,  0,  3,  3,  0,  1}; // for the left handed weapons
	const int offX5[8]			= {-1,  1,  1,  2,  0, -1,  0,  0}; // for the weapons (muton)
//	const int offY5[8]			= { 1, -1, -1, -1, -1, -2, -3,  0}; // for the weapons (muton) // kL
	const int offY5[8]			= { 1, -1, -1, -1, -1, -1, -3,  0}; // for the weapons (muton)
	const int offX6[8]			= { 0,  6,  6,  12,-4, -5, -5,-13}; // for the left handed rifles
	const int offY6[8]			= {-4, -4, -1,  0,  5,  0,  1,  0}; // for the left handed rifles
	const int offX7[8]			= { 0,  6,  8, 12,  2, -5, -5,-13}; // for the left handed rifles (muton)
	const int offY7[8]			= {-4, -6, -1,  0,  3,  0,  1,  0}; // for the left handed rifles (muton)

	const int offYKneel = 4;
	const int offXAiming = 16;


	const int unitDir	= _unit->getDirection();
	const int walkPhase	= _unit->getWalkingPhase();

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(torso);

		return;
	}

	if (_drawingRoutine == 0
		|| _helmet == true)
	{
		if (_unit->getArmor()->getForcedTorso() == TORSO_ALWAYS_FEMALE // STR_FLYING_SUIT_UC, the mod Colored Armors might muck w/ these.
			|| (_unit->getGender() == GENDER_FEMALE
				&& _unit->getArmor()->getForcedTorso() != TORSO_ALWAYS_MALE))
		{
			torso = _unitSurface->getFrame(femaleTorso + unitDir);
		}
		else // STR_POWER_SUIT_UC
			torso = _unitSurface->getFrame(maleTorso + unitDir);
	}
	else
	{
		if (_unit->getGender() == GENDER_FEMALE)
			torso = _unitSurface->getFrame(femaleTorso + unitDir);
		else
			torso = _unitSurface->getFrame(maleTorso + unitDir);
	}

	const bool
		isWalking = _unit->getStatus() == STATUS_WALKING,
		isKneeled = _unit->isKneeled();

	if (isWalking == true) // when walking, torso(fixed sprite) has to be animated up/down
	{
//		//Log(LOG_INFO) << "UnitSprite::drawRoutine0() : " << _unit->getId() << " STATUS_WALKING";
		if (_drawingRoutine == 10)								// muton
			torsoHandsWeaponY = yoffWalk_mut[walkPhase];
		else if (_drawingRoutine == 13
			|| _drawingRoutine == 14)
		{
			torsoHandsWeaponY = yoffWalk[walkPhase] + 1;
		}
		else if (_drawingRoutine == 15)							// aquatoid
			torsoHandsWeaponY = aquatoidYoffWalk[walkPhase];
		else													// xCom agents etc
			torsoHandsWeaponY = yoffWalk[walkPhase];

		torso->setY(torsoHandsWeaponY);

		legs = _unitSurface->getFrame(legsWalk[unitDir] + walkPhase);
		leftArm = _unitSurface->getFrame(larmWalk[unitDir] + walkPhase);
		rightArm = _unitSurface->getFrame(rarmWalk[unitDir] + walkPhase);

		// kL_note: This needs to be removed because I already changed/ fixed the sprites:
/*		if (_drawingRoutine == 10
			&& unitDir == 3)
		{
			leftArm->setY(-1);
		} */ // kL_note_end.
	}
	else
	{
		if (isKneeled == true)
		{
//			//Log(LOG_INFO) << "UnitSprite::drawRoutine0() : " << _unit->getId() << " isKneeled";
			legs = _unitSurface->getFrame(legsKneel + unitDir);
		}
		else if (_unit->isFloating() == true
//			&& _unit->getMoveTypeUnit() == MT_FLY)
			&& _moveType == MT_FLY) // kL, try it.
		{
//			//Log(LOG_INFO) << "UnitSprite::drawRoutine0() : " << _unit->getId() << " isFloating in FlyingSuit";
			legs = _unitSurface->getFrame(legsFloat + unitDir);
		}
		else
		{
//			//Log(LOG_INFO) << "UnitSprite::drawRoutine0() : " << _unit->getId() << " etc.";
			legs = _unitSurface->getFrame(legsStand + unitDir);
		}

		leftArm = _unitSurface->getFrame(larmStand + unitDir);
		rightArm = _unitSurface->getFrame(rarmStand + unitDir);
	}

	sortRifles();

	if (_itemA) // holding an item
	{
		if (_unit->getStatus() == STATUS_AIMING // draw handob item
			&& _itemA->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2) % 8;
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + dir);
			itemA->setX(offX[unitDir]);
			itemA->setY(offY[unitDir]);
		}
		else
		{
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + unitDir);
			if (_drawingRoutine == 10)
			{
				if (_itemA->getRules()->isTwoHanded())
				{
					itemA->setX(offX3[unitDir]);
					itemA->setY(offY3[unitDir]);
				}
				else
				{
					itemA->setX(offX5[unitDir]);
					itemA->setY(offY5[unitDir]);
				}
			}
			else
			{
				itemA->setX(0);
				itemA->setY(0);
			}
		}

		if (_itemA->getRules()->isTwoHanded()) // draw arms holding the item
		{
			leftArm = _unitSurface->getFrame(larm2H + unitDir);

			if (_unit->getStatus() == STATUS_AIMING)
			{
				rightArm = _unitSurface->getFrame(rarmShoot + unitDir);
			}
			else
			{
				rightArm = _unitSurface->getFrame(rarm2H + unitDir);
			}
		}
		else
		{
			if (_drawingRoutine == 10)
			{
				rightArm = _unitSurface->getFrame(rarm2H + unitDir);
			}
			else
			{
				rightArm = _unitSurface->getFrame(rarm1H + unitDir);
			}
		}

		if (isWalking) // the fixed arm(s) have to be animated up/down when walking
		{
			rightArm->setY(torsoHandsWeaponY);

			itemA->setY(itemA->getY() + torsoHandsWeaponY);
			if (_itemA->getRules()->isTwoHanded())
			{
				leftArm->setY(torsoHandsWeaponY);
			}
		}
	}

	if (_itemB) // if we are left handed or dual wielding...
	{
		leftArm = _unitSurface->getFrame(larm2H + unitDir);

		itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + unitDir);
		if (!_itemB->getRules()->isTwoHanded())
		{
			if (_drawingRoutine == 10)
			{
				itemB->setX(offX4[unitDir]);
				itemB->setY(offY4[unitDir]);
			}
			else
			{
				itemB->setX(offX2[unitDir]);
				itemB->setY(offY2[unitDir]);
			}
		}
		else
		{
			itemB->setX(0);
			itemB->setY(0);

			rightArm = _unitSurface->getFrame(rarm2H + unitDir);
		}

		if (_unit->getStatus() == STATUS_AIMING
			&& _itemB->getRules()->isTwoHanded())
		{
			int dir = (unitDir + 2) %8;
			itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + dir);

			if (_drawingRoutine == 10)
			{
				itemB->setX(offX7[unitDir]);
				itemB->setY(offY7[unitDir]);
			}
			else
			{
				itemB->setX(offX6[unitDir]);
				itemB->setY(offY6[unitDir]);
			}

			rightArm = _unitSurface->getFrame(rarmShoot + unitDir);
		}

		if (isWalking)
		{
			leftArm->setY(torsoHandsWeaponY);

			itemB->setY(itemB->getY() + torsoHandsWeaponY);
			if (_itemB->getRules()->isTwoHanded())
				rightArm->setY(torsoHandsWeaponY);
		}
	}

	if (isKneeled) // offset everything but legs when kneeled
	{
		leftArm->setY(offYKneel);
		rightArm->setY(offYKneel);
		torso->setY(offYKneel);
		itemA? itemA->setY(itemA->getY() + offYKneel): void();
		itemB? itemB->setY(itemB->getY() + offYKneel): void();
	}
	else if (isWalking == false)
	{
		leftArm->setY(0);
		rightArm->setY(0);
		torso->setY(0);
	}

	if (itemA) // items are calculated for soldier height (22) - some aliens are smaller, so item is drawn lower.
	{
		itemA->setY(itemA->getY() + (22 - _unit->getStandHeight()));
	}

	if (itemB)
	{
		itemB->setY(itemB->getY() + (22 - _unit->getStandHeight()));
	}

	if (_unit->getStatus() == STATUS_AIMING)
	{
		if (_itemA == NULL // kL: using Universal Fist. so PUNCH!! ( this is so funny )
			&& _itemB == NULL)
		{
			rightArm = _unitSurface->getFrame(rarmShoot + unitDir);
		}

		torso->setX(offXAiming);
		legs->setX(offXAiming);
		leftArm->setX(offXAiming);
		rightArm->setX(offXAiming);
		if (itemA)
			itemA->setX(itemA->getX() + offXAiming);
		if (itemB)
			itemB->setX(itemB->getX() + offXAiming);
	}
	// kL_note: This needs to be removed because I already changed the sprites:
/*	else if (!itemA
		&& _drawingRoutine == 10
		&& _unit->getStatus() == STATUS_WALKING
		&& unitDir == 2)
	{
		rightArm->setX(-6);
	} */ // kL_note_end.


	// blit order depends on unit direction
	// and whether unit is holding a 2 handed weapon.
	switch (unitDir)
	{
		case 0:
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 1:
			drawRecolored(leftArm);
			drawRecolored(legs);
			itemB ? itemB->blit(this) : void();
			drawRecolored(torso);
			itemA ? itemA->blit(this) : void();
			drawRecolored(rightArm);
		break;
		case 2:
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			itemB ? itemB->blit(this) : void();
			itemA ? itemA->blit(this) : void();
			drawRecolored(rightArm);
		break;
		case 3:
			if (_unit->getStatus() != STATUS_AIMING
				&& ((_itemA
						&& _itemA->getRules()->isTwoHanded())
					|| (_itemB
						&& _itemB->getRules()->isTwoHanded())))
			{
				drawRecolored(legs);
				drawRecolored(torso);
				drawRecolored(leftArm);
				itemA ? itemA->blit(this) : void();
				itemB ? itemB->blit(this) : void();
				drawRecolored(rightArm);
			}
			else
			{
				drawRecolored(legs);
				drawRecolored(torso);
				drawRecolored(leftArm);
				drawRecolored(rightArm);
				itemA ? itemA->blit(this) : void();
				itemB ? itemB->blit(this) : void();
			}
		break;
		case 4:
			drawRecolored(legs);
			drawRecolored(rightArm);
			drawRecolored(torso);
			drawRecolored(leftArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 5:
			if (_unit->getStatus() != STATUS_AIMING
				&& ((_itemA
						&& _itemA->getRules()->isTwoHanded())
					|| (_itemB
						&& _itemB->getRules()->isTwoHanded())))
			{
				drawRecolored(rightArm);
				drawRecolored(legs);
				drawRecolored(torso);
				drawRecolored(leftArm);
				itemA ? itemA->blit(this) : void();
				itemB ? itemB->blit(this) : void();
			}
			else
			{
				drawRecolored(rightArm);
				drawRecolored(legs);
				itemA ? itemA->blit(this) : void();
				itemB ? itemB->blit(this) : void();
				drawRecolored(torso);
				drawRecolored(leftArm);
			}
		break;
		case 6:
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			if (_unit->getStatus() != STATUS_AIMING
				&& ((_itemA
						&& _itemA->getRules()->isTwoHanded())
					|| (_itemB
						&& _itemB->getRules()->isTwoHanded())))
			{
				drawRecolored(rightArm);
				itemA ? itemA->blit(this) : void();
				itemB ? itemB->blit(this) : void();
				drawRecolored(leftArm);
				drawRecolored(legs);
				drawRecolored(torso);
			}
			else
			{
				itemA ? itemA->blit(this) : void();
				itemB ? itemB->blit(this) : void();
				drawRecolored(leftArm);
				drawRecolored(rightArm);
				drawRecolored(legs);
				drawRecolored(torso);
			}
		break;
	}

	torso->setX(0);
	legs->setX(0);
	leftArm->setX(0);
	rightArm->setX(0);
	if (itemA)
		itemA->setX(0);
	if (itemB)
		itemB->setX(0);
}

/**
 * Drawing routine for floaters.
 */
void UnitSprite::drawRoutine1()
{
	if (_unit->isOut())
		return; // unit is drawn as an item

	Surface
			* torso		= NULL,
			* leftArm	= NULL,
			* rightArm	= NULL,
			* itemA		= NULL,
			* itemB		= NULL;

	// magic numbers
	const int
		stand		= 16,
		walk		= 24,
		die			= 64,

		larm		= 8,
		rarm		= 0,
		larm2H		= 67,
		rarm2H		= 75,
		rarmShoot	= 83,
		rarm1H		= 91, // note that arms are switched vs "normal" sheets

		yoffWalk[8]	= { 0,  0,  0,  0,  0,  0,  0,  0 }, // bobbing up and down
		offX[8]		= { 8, 10,  7,  4, -9,-11, -7, -3 }, // for the weapons
		offY[8]		= {-6, -3,  0,  2,  0, -4, -7, -9 }, // for the weapons
		offX2[8]	= {-8,  3,  7, 13,  6, -3, -5,-13 }, // for the weapons
		offY2[8]	= { 1, -4, -1,  0,  3,  3,  5,  0 }, // for the weapons
		offX3[8]	= { 0,  6,  6, 12, -4, -5, -5,-13 }, // for the left handed rifles
		offY3[8]	= {-4, -4, -1,  0,  5,  0,  1,  0 }, // for the left handed rifles

		offXAiming = 16;


	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(torso);

		return;
	}

	const int
		unitDir = _unit->getDirection(),
		walkPhase = _unit->getWalkingPhase();

	leftArm = _unitSurface->getFrame(larm + unitDir);
	rightArm = _unitSurface->getFrame(rarm + unitDir);

	// when walking, torso(fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		// floater only has 5 walk animations instead of 8
		torso = _unitSurface->getFrame(
									walk
									+ (5 * unitDir)
									+ static_cast<int>(
											static_cast<float>(walkPhase) / 1.6f));
		torso->setY(yoffWalk[walkPhase]);
	}
	else
		torso = _unitSurface->getFrame(stand + unitDir);

	sortRifles();

	// holding an item
	if (_itemA)
	{
		// draw handob item
		if (_unit->getStatus() == STATUS_AIMING && _itemA->getRules()->isTwoHanded())
		{
			const int dir = (unitDir + 2) %8;
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + dir);
			itemA->setX(offX[unitDir]);
			itemA->setY(offY[unitDir]);
		}
		else
		{
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + unitDir);
			itemA->setX(0);
			itemA->setY(0);
		}

		// draw arms holding the item
		if (_itemA->getRules()->isTwoHanded())
		{
			leftArm = _unitSurface->getFrame(larm2H + unitDir);

			if (_unit->getStatus() == STATUS_AIMING)
			{
				rightArm = _unitSurface->getFrame(rarmShoot + unitDir);
			}
			else
			{
				rightArm = _unitSurface->getFrame(rarm2H + unitDir);
			}
		}
		else
		{
			rightArm = _unitSurface->getFrame(rarm1H + unitDir);
		}
	}

	// if we are left handed or dual wielding...
	if (_itemB)
	{
		leftArm = _unitSurface->getFrame(larm2H + unitDir);
		itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + unitDir);
		if (_itemB->getRules()->isTwoHanded() == false)
		{
			itemB->setX(offX2[unitDir]);
			itemB->setY(offY2[unitDir]);
		}
		else
		{
			itemB->setX(0);
			itemB->setY(0);

			rightArm = _unitSurface->getFrame(rarm2H + unitDir);
		}

		if (_unit->getStatus() == STATUS_AIMING && _itemB->getRules()->isTwoHanded())
		{
			const int dir = (unitDir + 2) %8;
			itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + dir);
			itemB->setX(offX3[unitDir]);
			itemB->setY(offY3[unitDir]);
			rightArm = _unitSurface->getFrame(rarmShoot + unitDir);
		}

		if (_unit->getStatus() == STATUS_WALKING)
		{
			leftArm->setY(yoffWalk[walkPhase]);
			itemB->setY(itemB->getY() + yoffWalk[walkPhase]);

			if (_itemB->getRules()->isTwoHanded())
				rightArm->setY(yoffWalk[walkPhase]);
		}
	}

	if (_unit->getStatus() != STATUS_WALKING)
	{
		leftArm->setY(0);
		rightArm->setY(0);
		torso->setY(0);
	}

	if (_unit->getStatus() == STATUS_AIMING)
	{
		torso->setX(offXAiming);
		leftArm->setX(offXAiming);
		rightArm->setX(offXAiming);
		if (itemA)
			itemA->setX(itemA->getX() + offXAiming);
		if (itemB)
			itemB->setX(itemB->getX() + offXAiming);
	}

	// blit order depends on unit direction.
	switch (unitDir)
	{
		case 0:
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(leftArm);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 1:
		case 2:
			drawRecolored(leftArm);
			drawRecolored(torso);
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 3:
		case 4:
			drawRecolored(torso);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 5:
			drawRecolored(rightArm);
			drawRecolored(torso);
			drawRecolored(leftArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 6:
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(leftArm);
			drawRecolored(torso);
		break;
	}

	torso->setX(0);
	leftArm->setX(0);
	rightArm->setX(0);
	if (itemA)
		itemA->setX(0);
	if (itemB)
		itemB->setX(0);
}

/**
 * Drawing routine for XCom tanks.
 */
void UnitSprite::drawRoutine2()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	const int
		offX[8] = {-2,-7,-5, 0, 5, 7, 2, 0}, // hovertank offsets
		offY[8] = {-1,-3,-4,-5,-4,-3,-1,-1},
		hoverTank	= _unit->getMoveTypeUnit() == MT_FLY ? 32 : 0,
		turret		= _unit->getTurretType();

	Surface* srf = NULL;

	// draw the animated propulsion below the hwp
	if (_part > 0
		&& hoverTank != 0)
	{
		srf = _unitSurface->getFrame(104 + ((_part - 1) * 8) + _animationFrame);
		drawRecolored(srf);
	}

	// kL_note: This is a fix, more of a workaround for tank's reverse strafing move.
	// There's a problem somewhere that keeps switching BattleUnit::_direction back
	// and forth ... in reverse gears. That is, _direction should remain constant
	// throughout a single-tile strafe move with tanks. At least _faceDirection
	// seems constant during these sprite-frames.
	int dirFace = _unit->getDirection();
	if (_unit->getFaceDirection() > -1)
		dirFace = _unit->getFaceDirection();

	// draw the tank itself
	srf = _unitSurface->getFrame(hoverTank + (_part * 8) + dirFace);
	drawRecolored(srf);

	// draw the turret, together with the last part
//	if (_part == 3 &&
	if (turret != -1)
	{
		srf = _unitSurface->getFrame(64 + (turret * 8) + _unit->getTurretDirection());
		int
			turretOffsetX,
			turretOffsetY;

		if (_part == 0) // kL->
		{
			turretOffsetX = 0,
			turretOffsetY = 12;
		}
		else if (_part == 1)
		{
			turretOffsetX = -16,
			turretOffsetY = 4;
		}
		else if (_part == 2)
		{
			turretOffsetX = 16,
			turretOffsetY = 4;
		}
		else //if (_part == 3)
		{
			turretOffsetX = 0,
			turretOffsetY = -4;
		}

		if (hoverTank != 0)
		{
			turretOffsetX += offX[dirFace];
			turretOffsetY += offY[dirFace];
		}

		srf->setX(turretOffsetX);
		srf->setY(turretOffsetY);
		drawRecolored(srf);
	}
}

/**
 * Drawing routine for cyberdiscs.
 */
void UnitSprite::drawRoutine3()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface* srf;

	// draw the animated propulsion below the hwp
	if (_part > 0)
	{
		srf = _unitSurface->getFrame(32 + ((_part-1) * 8) + _animationFrame);
		drawRecolored(srf);
	}

	srf = _unitSurface->getFrame((_part * 8) + _unit->getDirection());
	drawRecolored(srf);
}

/**
 * Drawing routine for civilians, ethereals, zombies (routine 4),
 * tftd civilians, tftd zombies (routine 17), more tftd civilians (routine 18). // <- wtf. says routines 16 & 17 below.
 * Very easy: first 8 is standing positions, then 8 walking sequences of 8, finally death sequence of 3
 */
void UnitSprite::drawRoutine4()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface
		* srf	= NULL,
		* itemA	= NULL,
		* itemB	= NULL;

//	int stand = 0, walk = 8, die = 72; // TFTD
	const int
		stand	= 0,
		walk	= 8,
		die		= 72,

		offX[8]			= { 8, 10,  7,  4, -9,-11, -7, -3 }, // for the weapons
		offY[8]			= {-6, -3,  0,  2,  0, -4, -7, -9 }, // for the weapons
		offX2[8]		= {-8,  3,  5, 12,  6, -1, -5,-13 }, // for the weapons
		offY2[8]		= { 1, -4, -2,  0,  3,  3,  5,  0 }, // for the weapons
		offX3[8]		= { 0,  6,  6, 12, -4, -5, -5,-13 }, // for the left handed rifles
		offY3[8]		= {-4, -4, -1,  0,  5,  0,  1,  0 }, // for the left handed rifles
//		standConvert[8]	= { 3,  2,  1,  0,  7,  6,  5,  4 }, // array for converting stand frames for some TFTD civilians

		offXAiming = 16;

/*TFTD
		unitDir = _unit->getDirection();

	if (_drawingRoutine == 17) // tftd civilian - first set
	{
		stand = 64;
		walk = 0;
	}
	else if (_drawingRoutine == 18) // tftd civilian - second set
	{
		stand = 140;
		walk = 76;
		die = 148;
	} */

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		srf = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(srf);

		return;
	}
	else if (_unit->getStatus() == STATUS_WALKING
		&& _unit->getRaceString() != "STR_ETHEREAL") // kL
	{
		srf = _unitSurface->getFrame(walk + (8 * _unit->getDirection()) + _unit->getWalkingPhase());
	}
	else // if (_drawingRoutine != 17) // TFTD
	{
		srf = _unitSurface->getFrame(stand + _unit->getDirection());
	}
/*TFTD
	else
	{
		srf = _unitSurface->getFrame(stand + standConvert[unitDir]);
	} */


	sortRifles();

	if (_itemA
		&& !_itemA->getRules()->isFixed())
	{
		if (_unit->getStatus() == STATUS_AIMING // draw handob item
			&& _itemA->getRules()->isTwoHanded())
		{
			int dir = (_unit->getDirection() + 2) %8;
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + dir);
			itemA->setX(offX[_unit->getDirection()]);
			itemA->setY(offY[_unit->getDirection()]);
		}
		else
		{
			if (_itemA->getSlot()->getId() == "STR_RIGHT_HAND")
			{
				itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + _unit->getDirection());
				itemA->setX(0);
				itemA->setY(0);
			}
			else
			{
				itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + _unit->getDirection());
				itemA->setX(offX2[_unit->getDirection()]);
				itemA->setY(offY2[_unit->getDirection()]);
			}
		}
	}

	if (_itemB // if dual wielding...
		&& !_itemB->getRules()->isFixed())
	{
		itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + _unit->getDirection());
		if (!_itemB->getRules()->isTwoHanded())
		{
			itemB->setX(offX2[_unit->getDirection()]);
			itemB->setY(offY2[_unit->getDirection()]);
		}
		else
		{
			itemB->setX(0);
			itemB->setY(0);
		}

		if (_unit->getStatus() == STATUS_AIMING
			&& _itemB->getRules()->isTwoHanded())
		{
			int dir = (_unit->getDirection() + 2) %8;
			itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + dir);
			itemB->setX(offX3[_unit->getDirection()]);
			itemB->setY(offY3[_unit->getDirection()]);
		}
	}

	if (_unit->getStatus() == STATUS_AIMING)
	{
		srf->setX(offXAiming);
		if (itemA)
			itemA->setX(itemA->getX() + offXAiming);
		if (itemB)
			itemB->setX(itemB->getX() + offXAiming);
	}

	switch (_unit->getDirection())
	{
		case 0:
			itemB ? itemB->blit(this) : void();
			itemA ? itemA->blit(this) : void();
			drawRecolored(srf);
		break;
		case 1:
			itemB ? itemB->blit(this) : void();
			drawRecolored(srf);
			itemA ? itemA->blit(this) : void();
		break;
		case 2:
			drawRecolored(srf);
			itemB ? itemB->blit(this) : void();
			itemA ? itemA->blit(this) : void();
		break;
		case 3:
		case 4:
			drawRecolored(srf);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 5:
//kL	case 6:
			itemA ? itemA->blit(this) : void();
			drawRecolored(srf);
			itemB ? itemB->blit(this) : void();
		break;
		case 6: // kL
		case 7:
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(srf);
		break;
	}

	srf->setX(0);
	if (itemA)
		itemA->setX(0);
	if (itemB)
		itemB->setX(0);
}

/**
 * Drawing routine for sectopods and reapers.
 */
void UnitSprite::drawRoutine5()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface* srf;

	if (_unit->getStatus() == STATUS_WALKING)
		srf = _unitSurface->getFrame(32 + (_unit->getDirection() * 16) + (_part * 4) + ((_unit->getWalkingPhase() / 2) %4));
	else
		srf = _unitSurface->getFrame((_part * 8) + _unit->getDirection());

	drawRecolored(srf);
}

/**
 * Drawing routine for snakemen.
 */
void UnitSprite::drawRoutine6()
{
	if (_unit->isOut())
		return; // unit is drawn as an item

	Surface
		* torso		= NULL,
		* legs		= NULL,
		* leftArm	= NULL,
		* rightArm	= NULL,
		* itemA		= NULL,
		* itemB		= NULL;

	// magic numbers
	const int
		Torso		= 24,
		legsStand	= 16,
		die			= 96,

		larmStand	= 0,
		rarmStand	= 8,
		rarm1H		= 99,
		larm2H		= 107,
		rarm2H		= 115,
		rarmShoot	= 123,

		legsWalk[8]		= {32, 40, 48, 56, 64, 72, 80, 88},
		yoffWalk[8]		= { 3,  3,  2,  1,  0,  0,  1,  2}, // bobbing up and down
		xoffWalka[8]	= { 0,  0,  1,  2,  3,  3,  2,  1},
		xoffWalkb[8]	= { 0,  0, -1, -2, -3, -3, -2, -1},
		yoffStand[8]	= { 2,  1,  1,  0,  0,  0,  0,  0},
		offX[8]			= { 8, 10,  5,  2, -8,-10, -5, -2}, // for the weapons
//		offX[8]			= { 8, 10,  8,  5, -8,-10, -5, -2}, // kL
		offY[8]			= {-6, -3,  0,  0,  2, -3, -7, -9}, // for the weapons
//		offY[8]			= {-6, -3,  0, -3,  0, -4, -7, -9}, // kL
		offX2[8]		= {-8,  2,  7, 13,  7,  0, -3,-15}, // for the weapons
		offY2[8]		= { 1, -4, -2,  0,  3,  3,  5,  0}, // for the weapons
		offX3[8]		= { 0,  6,  6, 12, -4, -5, -5,-13}, // for the left handed rifles
		offY3[8]		= {-4, -4, -1,  0,  5,  0,  1,  0}; // for the left handed rifles

	const int offXAiming = 16;


	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(torso);

		return;
	}

	const int
		unitDir		= _unit->getDirection(),
		walkPhase	= _unit->getWalkingPhase();

	torso = _unitSurface->getFrame(Torso + _unit->getDirection());
	leftArm = _unitSurface->getFrame(larmStand + _unit->getDirection());
	rightArm = _unitSurface->getFrame(rarmStand + _unit->getDirection());

	// when walking, torso (fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		int xoffWalk = 0;
		if (_unit->getDirection() < 3)
			xoffWalk = xoffWalka[walkPhase];
		if (_unit->getDirection() < 7 && _unit->getDirection() > 3)
			xoffWalk = xoffWalkb[walkPhase];

		torso->setY(yoffWalk[walkPhase]);
		torso->setX(xoffWalk);
		legs = _unitSurface->getFrame(legsWalk[_unit->getDirection()] + walkPhase);
		rightArm->setY(yoffWalk[walkPhase]);
		leftArm->setY(yoffWalk[walkPhase]);
		rightArm->setX(xoffWalk);
		leftArm->setX(xoffWalk);
	}
	else
		legs = _unitSurface->getFrame(legsStand + _unit->getDirection());

	sortRifles();

	// holding an item
	if (_itemA)
	{
		// draw handob item
		if (_unit->getStatus() == STATUS_AIMING && _itemA->getRules()->isTwoHanded())
		{
			int dir = (_unit->getDirection() + 2) %8;
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + dir);
			itemA->setX(offX[_unit->getDirection()]);
			itemA->setY(offY[_unit->getDirection()]);
		}
		else
		{
			itemA = _itemSurfaceA->getFrame(_itemA->getRules()->getHandSprite() + _unit->getDirection());
			itemA->setX(0);
			itemA->setY(0);

			if (_itemA->getRules()->isTwoHanded() == false)
				itemA->setY(yoffStand[unitDir]);
		}

		// draw arms holding the item
		if (_itemA->getRules()->isTwoHanded())
		{
			leftArm = _unitSurface->getFrame(larm2H + _unit->getDirection());
			if (_unit->getStatus() == STATUS_AIMING)
				rightArm = _unitSurface->getFrame(rarmShoot + _unit->getDirection());
			else
				rightArm = _unitSurface->getFrame(rarm2H + _unit->getDirection());
		}
		else
			rightArm = _unitSurface->getFrame(rarm1H + _unit->getDirection());

		// the fixed arm(s) have to be animated up/down when walking
		if (_unit->getStatus() == STATUS_WALKING)
		{
			itemA->setY(yoffWalk[walkPhase]);
			rightArm->setY(yoffWalk[walkPhase]);
			if (_itemA->getRules()->isTwoHanded())
				leftArm->setY(yoffWalk[walkPhase]);
		}
	}

	if (_itemB) // if we are left handed or dual wielding...
	{
		leftArm = _unitSurface->getFrame(larm2H + _unit->getDirection());
		itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + _unit->getDirection());
		if (_itemB->getRules()->isTwoHanded() == false)
		{
			itemB->setX(offX2[_unit->getDirection()]);
			itemB->setY(offY2[_unit->getDirection()]);
		}
		else
		{
			itemB->setX(0);
			itemB->setY(0);

			if (_itemB->getRules()->isTwoHanded() == false)
				itemB->setY(yoffStand[unitDir]);

			rightArm = _unitSurface->getFrame(rarm2H + unitDir);
		}

		if (_unit->getStatus() == STATUS_AIMING && _itemB->getRules()->isTwoHanded())
		{
			int dir = (_unit->getDirection() + 2) %8;
			itemB = _itemSurfaceB->getFrame(_itemB->getRules()->getHandSprite() + dir);
			itemB->setX(offX3[_unit->getDirection()]);
			itemB->setY(offY3[_unit->getDirection()]);
			rightArm = _unitSurface->getFrame(rarmShoot + _unit->getDirection());
		}

		if (_unit->getStatus() == STATUS_WALKING)
		{
			leftArm->setY(yoffWalk[walkPhase]);
			itemB->setY(offY2[_unit->getDirection()] + yoffWalk[walkPhase]);
			if (_itemB->getRules()->isTwoHanded())
				rightArm->setY(yoffWalk[walkPhase]);
		}
	}

	// offset everything but legs when kneeled
	if (_unit->getStatus() != STATUS_WALKING)
	{
		leftArm->setY(0);
		rightArm->setY(0);
		torso->setY(0);
	}

	if (_unit->getStatus() == STATUS_AIMING)
	{
		if (_itemA == NULL // kL: using Universal Fist. so PUNCH!! ( this is so funny )
			&& _itemB == NULL)
		{
			rightArm = _unitSurface->getFrame(rarmShoot + _unit->getDirection());
		}

		torso->setX(offXAiming);
		legs->setX(offXAiming);
		leftArm->setX(offXAiming);
		rightArm->setX(offXAiming);
		if (itemA)
			itemA->setX(itemA->getX() + offXAiming);
		if (itemB)
			itemB->setX(itemB->getX() + offXAiming);
	}

	// blit order depends on unit direction.
	switch (_unit->getDirection())
	{
		case 0:
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 1:
			drawRecolored(leftArm);
			drawRecolored(legs);
			itemB ? itemB->blit(this) : void();
			drawRecolored(torso);
			itemA ? itemA->blit(this) : void();
			drawRecolored(rightArm);
		break;
		case 2:
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 3:
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 4:
		case 5:
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
		break;
		case 6:
			drawRecolored(rightArm);
			drawRecolored(legs);
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			itemA ? itemA->blit(this) : void();
			itemB ? itemB->blit(this) : void();
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
		break;
	}

	torso->setX(0);
	legs->setX(0);
	leftArm->setX(0);
	rightArm->setX(0);
	if (itemA)
		itemA->setX(itemA->getX() + 0);
	if (itemB)
		itemB->setX(itemB->getX() + 0);
}

/**
 * Drawing routine for chryssalid.
 */
void UnitSprite::drawRoutine7()
{
	if (_unit->isOut())
		return; // unit is drawn as an item

	Surface
			* torso		= NULL,
			* legs		= NULL,
			* leftArm	= NULL,
			* rightArm	= NULL;

	// magic numbers
	const int
		Torso		= 24,
		legsStand	= 16,
		die			= 224,

		larmStand	= 0,
		rarmStand	= 8,

		legsWalk[8] = { 48, 48+24, 48+24*2, 48+24*3, 48+24*4, 48+24*5, 48+24*6, 48+24*7 },
		larmWalk[8] = { 32, 32+24, 32+24*2, 32+24*3, 32+24*4, 32+24*5, 32+24*6, 32+24*7 },
		rarmWalk[8] = { 40, 40+24, 40+24*2, 40+24*3, 40+24*4, 40+24*5, 40+24*6, 40+24*7 },
		yoffWalk[8] = {  1,     0,      -1,       0,       1,       0,      -1,       0 }; // bobbing up and down


	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(torso);

		return;
	}

	const int
		unitDir		= _unit->getDirection(),
		walkPhase	= _unit->getWalkingPhase();

	torso = _unitSurface->getFrame(Torso + unitDir);

	// when walking, torso(fixed sprite) has to be animated up/down
	if (_unit->getStatus() == STATUS_WALKING)
	{
		torso->setY(yoffWalk[walkPhase]);
		legs = _unitSurface->getFrame(legsWalk[unitDir] + walkPhase);
		leftArm = _unitSurface->getFrame(larmWalk[unitDir] + walkPhase);
		rightArm = _unitSurface->getFrame(rarmWalk[unitDir] + walkPhase);
	}
	else
	{

		legs = _unitSurface->getFrame(legsStand + unitDir);
		leftArm = _unitSurface->getFrame(larmStand + unitDir);
		rightArm = _unitSurface->getFrame(rarmStand + unitDir);

		leftArm->setY(0);
		rightArm->setY(0);
		torso->setY(0);
	}

	// blit order depends on unit direction
	switch (unitDir)
	{
		case 0:
		case 1:
		case 2:
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 3:
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
		break;
		case 4:
		case 5:
		case 6:
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
		break;
	}
}

/**
 * Drawing routine for silacoids.
 */
void UnitSprite::drawRoutine8()
{
	if (_unit->isOut())
		return; // unit is drawn as an item

	Surface* legs = NULL;

	// magic numbers
	const int
		body	= 0,
		aim		= 5,
		die		= 6,
		pulsate[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };

	legs = _unitSurface->getFrame(body + pulsate[_animationFrame]);
	_redraw = true;

	if (_unit->getStatus() == STATUS_COLLAPSING)
		legs = _unitSurface->getFrame(die + _unit->getFallingPhase());

	if (_unit->getStatus() == STATUS_AIMING)
		legs = _unitSurface->getFrame(aim);

	drawRecolored(legs);
}

/**
 * Drawing routine for celatids.
 */
void UnitSprite::drawRoutine9()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	_redraw = true;

	const int // magic numbers
		body = 0,
		die = 25,
		shoot = 8;  // frames 8..23 or ..24 (24 is merely a green ball sprite)

	Surface* torso = NULL;
	if (_unit->getStatus() == STATUS_COLLAPSING)
		torso = _unitSurface->getFrame(die + _unit->getFallingPhase());
	else if (_unit->getStatus() == STATUS_AIMING)
	{
		const int framesTotal = _unit->getArmor()->getShootFrames();
		int
			phase = _unit->getAimingPhase(),
			extra;
		//if (_unit->getId() == 1000007) Log(LOG_INFO) << "DRAW phase = " << phase;

		if (phase == framesTotal)
		{
			//if (_unit->getId() == 1000007) Log(LOG_INFO) << ". add Extra";
			extra = 2; // bounce back one frame at the end.
		}
		else
			extra = 0;

		//if (_unit->getId() == 1000007) Log(LOG_INFO) << ". draw Frame " << (std::min(
		//									shoot + phase - extra,
		//									shoot + framesTotal - 1));
		torso = _unitSurface->getFrame(std::min(
											shoot + phase - extra,
											shoot + framesTotal - 1));
		// Clamp that, because slow (read, long) think()-draw intervals cause it
		// to exceed the upper bound of total shootFrames.
		_unit->setAimingPhase(++phase);
		// - let BattleUnit::keepAiming() iterate the final aimPhase. nix that;
		// super-slow animation speed doesn't even let keepAiming() to get called.
		// ... not sure how the animation is ended in that case, but something does
		//if (_unit->getId() == 1000007) Log(LOG_INFO) << ". set aimPhase to " << phase;
	}
	else
		torso = _unitSurface->getFrame(body + _animationFrame);

	drawRecolored(torso);
}

// kL_note: TFTD down to sortRifles()
/**
 * Drawing routine for tftd tanks.
 */
/* void UnitSprite::drawRoutine11()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item (ie. floorob)

	const int
		offTurretX[8] = { -2,  -6,  -5,   0,   5,   6,   2,   0},
		offTurretY[8] = {-12, -13, -16, -16, -16, -13, -12, -12};

	int
		body,
		animFrame;
	if (_unit->getMoveTypeUnit() == MT_FLY)
	{
		body = 128;
		animFrame = _animationFrame % 4;
	}
	else
	{
		body = 0;
		animFrame = _unit->getWalkingPhase() % 4;
	}

	Surface* srf = _unitSurface->getFrame(body + (_part * 4) + 16 * _unit->getDirection() + animFrame);
	srf->setY(4);
	drawRecolored(srf);

	if (_part == 3 // draw the turret, overlapping all 4 parts
		&& _unit->getFloorAbove() == false)
	{
		const int turret = _unit->getTurretType();
		if (turret != -1)
		{
			srf = _unitSurface->getFrame(256 + (turret * 8) + _unit->getTurretDirection());
			srf->setX(offTurretX[static_cast<size_t>(_unit->getDirection())]);
			srf->setY(offTurretY[static_cast<size_t>(_unit->getDirection())]);
			drawRecolored(srf);
		}
	}
} */

/**
 * Drawing routine for hallucinoids (routine 12) and biodrones (routine 16).
 */
/* void UnitSprite::drawRoutine12()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	const int die = 8;

	Surface* srf = _unitSurface->getFrame((_part * 8) + _animationFrame);
	_redraw = true;

	if (_unit->getStatus() == STATUS_COLLAPSING && _drawingRoutine == 16)
	{
		// biodrone death frames
		srf = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(srf);

		return;
	}

	drawRecolored(srf);
} */

/**
 * Drawing routine for tentaculats.
 */
/* void UnitSprite::drawRoutine19()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface* srf;
	// magic numbers
	const int
		stand = 0,
		move = 8,
		die = 16;

	if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		srf = _unitSurface->getFrame(die + _unit->getFallingPhase());
		drawRecolored(srf);

		return;
	}

	if (_unit->getStatus() == STATUS_WALKING)
		srf = _unitSurface->getFrame(move + _unit->getDirection());
	else
		srf = _unitSurface->getFrame(stand + _unit->getDirection());

	drawRecolored(srf);
} */

/**
 * Drawing routine for triscenes.
 */
/* void UnitSprite::drawRoutine20()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface* srf;

	if (_unit->getStatus() == STATUS_WALKING)
		srf = _unitSurface->getFrame((_unit->getWalkingPhase() / 2 %4) + 5 * (_part + 4 * _unit->getDirection()));
	else
		srf = _unitSurface->getFrame(5 * (_part + 4 * _unit->getDirection()));

	drawRecolored(srf);
} */

/**
 * Drawing routine for xarquids.
 */
/* void UnitSprite::drawRoutine21()
{
	if (_unit->isOut() == true)
		return; // unit is drawn as an item

	Surface* srf;

	srf = _unitSurface->getFrame((_part * 4) + (_unit->getDirection() * 16) + (_animationFrame %4));
	_redraw = true;

	drawRecolored(srf);
} */

/**
 * Determines which weapons to display in the case of two-handed weapons.
 */
void UnitSprite::sortRifles()
{
//	if (_itemA && _itemA->getRules()->isTwoHanded())
	if (_itemA) // kL
	{
//		if (_itemB && _itemB->getRules()->isTwoHanded())
		if (_itemB) // kL
		{
			if (_unit->getActiveHand() == "STR_LEFT_HAND")
				_itemA = _itemB;

			_itemB = NULL;
		}
		else if (_unit->getStatus() != STATUS_AIMING)
			_itemB = NULL;
	}
//	else if (_itemB && _itemB->getRules()->isTwoHanded())
	else if (_itemB) // kL
	{
		if (_unit->getStatus() != STATUS_AIMING)
			_itemA = NULL;
	}
}

}
