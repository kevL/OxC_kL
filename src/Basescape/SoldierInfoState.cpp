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

#include "SoldierInfoState.h"
#include <sstream>
#include "../Engine/Game.h"
#include "../Engine/Action.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Options.h"
#include "../Interface/Bar.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Engine/Surface.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Ruleset/RuleCraft.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/ItemContainer.h"
#include "../Engine/SurfaceSet.h"
#include "../Ruleset/Armor.h"
#include "SoldierArmorState.h"
#include "SackSoldierState.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Info screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param soldier ID of the selected soldier.
 */
SoldierInfoState::SoldierInfoState(Game* game, Base* base, size_t soldier)
	:
		State(game),
		_base(base),
		_soldier(soldier)
{
	// Create objects
	_bg				= new Surface(320, 200, 0, 0);
	_rank			= new Surface(26, 23, 4, 4);
	_btnPrev		= new TextButton(28, 14, 0, 33);
	_btnOk			= new TextButton(48, 14, 30, 33);
	_btnNext		= new TextButton(28, 14, 80, 33);

	_txtArmor		= new Text(30, 9, 130, 35);				// kL. reinstating this!
//kL	_btnArmor		= new TextButton(110, 14, 130, 33);	// (60, 14, 130, 33);
	_btnArmor		= new TextButton(70, 14, 164, 33);		// kL

	_edtSoldier		= new TextEdit(200, 16, 40, 9);
//kL	_btnSack		= new TextButton(60, 14, 260, 33);	// (60, 14, 248, 10);
	_btnSack		= new TextButton(44, 16, 269, 8); 		// kL
//	_txtArmor		= new Text(120, 9, 194, 38);

/*kL	_txtRank		= new Text(130, 9, 0, 48);
	_txtMissions	= new Text(100, 9, 130, 48);
	_txtKills		= new Text(100, 9, 230, 48);
	_txtCraft		= new Text(130, 9, 0, 56);
	_txtRecovery	= new Text(180, 9, 130, 56); */
	_txtRank		= new Text(130, 9, 0, 49);		// kL
	_txtMissions	= new Text(100, 9, 130, 49);	// kL
	_txtKills		= new Text(100, 9, 230, 49);	// kL
	_txtCraft		= new Text(130, 9, 0, 57);		// kL
	_txtRecovery	= new Text(180, 9, 130, 57);	// kL
//kL	_txtPsionic		= new Text(150, 9, 0, 66);
	_txtPsionic		= new Text(75, 9, 5, 67);		// kL

//kL	_txtTimeUnits	= new Text(120, 9, 6, 82);
//kL	_numTimeUnits	= new Text(18, 9, 131, 82);
	_txtTimeUnits	= new Text(120, 9, 5, 82);		// kL
	_numTimeUnits	= new Text(18, 9, 131, 82);		// kL
	_barTimeUnits	= new Bar(170, 7, 150, 82);

//kL	_txtStamina		= new Text(120, 9, 6, 94);
//kL	_numStamina		= new Text(18, 9, 131, 94);
	_txtStamina		= new Text(120, 9, 5, 94);		// kL
	_numStamina		= new Text(18, 9, 131, 94);		// kL
	_barStamina		= new Bar(170, 7, 150, 94);

//kL	_txtHealth		= new Text(120, 9, 6, 106);
//kL	_numHealth		= new Text(18, 9, 131, 106);
	_txtHealth		= new Text(120, 9, 6, 106);		// kL
	_numHealth		= new Text(18, 9, 131, 106);	// kL
	_barHealth		= new Bar(170, 7, 150, 106);

//kL	_txtBravery		= new Text(120, 9, 6, 118);
//kL	_numBravery		= new Text(18, 9, 131, 118);
	_txtBravery		= new Text(120, 9, 6, 118);		// kL
	_numBravery		= new Text(18, 9, 131, 118);	// kL
	_barBravery		= new Bar(170, 7, 150, 118);

//kL	_txtReactions	= new Text(120, 9, 6, 130);
//kL	_numReactions	= new Text(18, 9, 131, 130);
	_txtReactions	= new Text(120, 9, 6, 130);		// kL
	_numReactions	= new Text(18, 9, 131, 130);	// kL
	_barReactions	= new Bar(170, 7, 150, 130);

//kL	_txtFiring		= new Text(120, 9, 6, 142);
//kL	_numFiring		= new Text(18, 9, 131, 142);
	_txtFiring		= new Text(120, 9, 6, 142);		// kL
	_numFiring		= new Text(18, 9, 131, 142);	// kL
	_barFiring		= new Bar(170, 7, 150, 142);

//kL	_txtThrowing	= new Text(120, 9, 6, 154);
//kL	_numThrowing	= new Text(18, 9, 131, 154);
	_txtThrowing	= new Text(120, 9, 6, 154);		// kL
	_numThrowing	= new Text(18, 9, 131, 154);	// kL
	_barThrowing	= new Bar(170, 7, 150, 154);

//kL	_txtStrength	= new Text(120, 9, 6, 166);
//kL	_numStrength	= new Text(18, 9, 131, 166);
	_txtStrength	= new Text(120, 9, 6, 166);		// kL
	_numStrength	= new Text(18, 9, 131, 166);	// kL
	_barStrength	= new Bar(170, 7, 150, 166);
	
//kL	_txtPsiStrength	= new Text(120, 9, 6, 178);
//kL	_numPsiStrength	= new Text(18, 9, 131, 178);
	_txtPsiStrength	= new Text(120, 9, 6, 178);		// kL
	_numPsiStrength	= new Text(18, 9, 131, 178);	// kL
	_barPsiStrength	= new Bar(170, 7, 150, 178);
	
//kL	_txtPsiSkill	= new Text(120, 9, 6, 190);
//kL	_numPsiSkill	= new Text(18, 9, 131, 190);
	_txtPsiSkill	= new Text(120, 9, 6, 190);		// kL
	_numPsiSkill	= new Text(18, 9, 131, 190);	// kL
	_barPsiSkill	= new Bar(170, 7, 150, 190);

	add(_bg);
	add(_rank);
	add(_btnOk);
	add(_btnPrev);
	add(_btnNext);
	add(_btnArmor);
	add(_edtSoldier);
	add(_btnSack);
	add(_txtArmor);		// kL_note: reinstating this!
	add(_txtRank);
	add(_txtMissions);
	add(_txtKills);
	add(_txtCraft);
	add(_txtRecovery);
	add(_txtPsionic);

	add(_txtTimeUnits);
	add(_numTimeUnits);
	add(_barTimeUnits);

	add(_txtStamina);
	add(_numStamina);
	add(_barStamina);

	add(_txtHealth);
	add(_numHealth);
	add(_barHealth);

	add(_txtBravery);
	add(_numBravery);
	add(_barBravery);

	add(_txtReactions);
	add(_numReactions);
	add(_barReactions);

	add(_txtFiring);
	add(_numFiring);
	add(_barFiring);

	add(_txtThrowing);
	add(_numThrowing);
	add(_barThrowing);

	add(_txtStrength);
	add(_numStrength);
	add(_barStrength);

	add(_txtPsiStrength);
	add(_numPsiStrength);
	add(_barPsiStrength);

	add(_txtPsiSkill);
	add(_numPsiSkill);
	add(_barPsiSkill);

	centerAllSurfaces();

	// Set up objects
	_game->getResourcePack()->getSurface("BACK06.SCR")->blit(_bg);

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierInfoState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)& SoldierInfoState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_btnPrev->setColor(Palette::blockOffset(15)+6);
	_btnPrev->setText(L"<<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierInfoState::btnPrevClick);

	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">>");
	_btnNext->onMouseClick((ActionHandler)& SoldierInfoState::btnNextClick);

	_btnArmor->setColor(Palette::blockOffset(15)+6);
//kL	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)& SoldierInfoState::btnArmorClick);

	_edtSoldier->setColor(Palette::blockOffset(13)+10);
	_edtSoldier->setBig();
	_edtSoldier->onKeyboardPress((ActionHandler)& SoldierInfoState::edtSoldierKeyPress);

	_btnSack->setColor(Palette::blockOffset(15)+6);
	_btnSack->setText(tr("STR_SACK"));
	_btnSack->onMouseClick((ActionHandler)& SoldierInfoState::btnSackClick);

	_txtArmor->setColor(Palette::blockOffset(13));		// kL_note: reinstating this!
	_txtArmor->setText(tr("STR_ARMOR"));				// kL

	_txtRank->setColor(Palette::blockOffset(13)+10);
	_txtRank->setSecondaryColor(Palette::blockOffset(13));

	_txtMissions->setColor(Palette::blockOffset(13)+10);
	_txtMissions->setSecondaryColor(Palette::blockOffset(13));

	_txtKills->setColor(Palette::blockOffset(13)+10);
	_txtKills->setSecondaryColor(Palette::blockOffset(13));

	_txtCraft->setColor(Palette::blockOffset(13)+10);
	_txtCraft->setSecondaryColor(Palette::blockOffset(13));

	_txtRecovery->setColor(Palette::blockOffset(13)+10);
	_txtRecovery->setSecondaryColor(Palette::blockOffset(13));

	_txtPsionic->setColor(Palette::blockOffset(15)+1);
	_txtPsionic->setText(tr("STR_IN_PSIONIC_TRAINING"));

	_txtTimeUnits->setColor(Palette::blockOffset(15)+1);
	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));

	_numTimeUnits->setColor(Palette::blockOffset(13));

	_barTimeUnits->setColor(Palette::blockOffset(3));
	_barTimeUnits->setColor2(Palette::blockOffset(3)+4);
	_barTimeUnits->setScale(1.0);
	_barTimeUnits->setInvert(true);

	_txtStamina->setColor(Palette::blockOffset(15)+1);
	_txtStamina->setText(tr("STR_STAMINA"));

	_numStamina->setColor(Palette::blockOffset(13));

	_barStamina->setColor(Palette::blockOffset(9));
	_barStamina->setColor2(Palette::blockOffset(9)+4);
	_barStamina->setScale(1.0);
	_barStamina->setInvert(true);

	_txtHealth->setColor(Palette::blockOffset(15)+1);
	_txtHealth->setText(tr("STR_HEALTH"));

	_numHealth->setColor(Palette::blockOffset(13));

	_barHealth->setColor(Palette::blockOffset(2));
	_barHealth->setColor2(Palette::blockOffset(2)+4);
	_barHealth->setScale(1.0);
	_barHealth->setInvert(true);

	_txtBravery->setColor(Palette::blockOffset(15)+1);
	_txtBravery->setText(tr("STR_BRAVERY"));

	_numBravery->setColor(Palette::blockOffset(13));

	_barBravery->setColor(Palette::blockOffset(4));
	_barBravery->setColor2(Palette::blockOffset(4)+4);
	_barBravery->setScale(1.0);
	_barBravery->setInvert(true);

	_txtReactions->setColor(Palette::blockOffset(15)+1);
	_txtReactions->setText(tr("STR_REACTIONS"));

	_numReactions->setColor(Palette::blockOffset(13));

	_barReactions->setColor(Palette::blockOffset(1));
	_barReactions->setColor2(Palette::blockOffset(1)+4);
	_barReactions->setScale(1.0);
	_barReactions->setInvert(true);

	_txtFiring->setColor(Palette::blockOffset(15)+1);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));

	_numFiring->setColor(Palette::blockOffset(13));

	_barFiring->setColor(Palette::blockOffset(8));
	_barFiring->setColor2(Palette::blockOffset(8)+4);
	_barFiring->setScale(1.0);
	_barFiring->setInvert(true);

	_txtThrowing->setColor(Palette::blockOffset(15)+1);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));

	_numThrowing->setColor(Palette::blockOffset(13));

	_barThrowing->setColor(Palette::blockOffset(10));
	_barThrowing->setColor2(Palette::blockOffset(10)+4);
	_barThrowing->setScale(1.0);
	_barThrowing->setInvert(true);

	_txtStrength->setColor(Palette::blockOffset(15)+1);
	_txtStrength->setText(tr("STR_STRENGTH"));

	_numStrength->setColor(Palette::blockOffset(13));

	_barStrength->setColor(Palette::blockOffset(5));
	_barStrength->setColor2(Palette::blockOffset(5)+4);
	_barStrength->setScale(1.0);
	_barStrength->setInvert(true);

	_txtPsiStrength->setColor(Palette::blockOffset(15)+1);
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));

	_numPsiStrength->setColor(Palette::blockOffset(13));

	_barPsiStrength->setColor(Palette::blockOffset(11));
	_barPsiStrength->setColor2(Palette::blockOffset(11)+4);
	_barPsiStrength->setScale(1.0);
	_barPsiStrength->setInvert(true);

	_txtPsiSkill->setColor(Palette::blockOffset(15)+1);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));

	_numPsiSkill->setColor(Palette::blockOffset(13));

	_barPsiSkill->setColor(Palette::blockOffset(11));
	_barPsiSkill->setColor2(Palette::blockOffset(11)+4);
	_barPsiSkill->setScale(1.0);
	_barPsiSkill->setInvert(true);
}

/**
 *
 */
SoldierInfoState::~SoldierInfoState()
{
}

/**
 * Updates soldier stats when the soldier changes.
 */
void SoldierInfoState::init()
{
	if (_base->getSoldiers()->empty())
	{
		_game->popState();

		return;
	}

	if (_soldier == _base->getSoldiers()->size())
	{
		_soldier = 0;
	}

	Soldier* s = _base->getSoldiers()->at(_soldier);
	_edtSoldier->setText(s->getName());
	UnitStats* initial = s->getInitStats();
	UnitStats* current = s->getCurrentStats();

	SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	texture->getFrame(s->getRankSprite())->setX(0);
	texture->getFrame(s->getRankSprite())->setY(0);
	texture->getFrame(s->getRankSprite())->blit(_rank);

	std::wstringstream ss;
	ss << current->tu;
	_numTimeUnits->setText(ss.str());
//kL	_barTimeUnits->setMax(current->tu);
	// kL_begin:
	if (current->tu > initial->tu)
	{
		_barTimeUnits->setMax(current->tu);
	}
	else
		_barTimeUnits->setMax(initial->tu);
	// kL_end.
	_barTimeUnits->setValue(current->tu);
//kL	_barTimeUnits->setValue2(initial->tu);
	// kL_begin:
	if (current->tu > initial->tu)
	{
		_barTimeUnits->setValue2(initial->tu);
	}
	else
		_barTimeUnits->setValue2(current->tu);
	// kL_end.

	std::wstringstream ss2;
	ss2 << current->stamina;
	_numStamina->setText(ss2.str());
//kL	_barStamina->setMax(current->stamina);
	// kL_begin:
	if (current->stamina > initial->stamina)
	{
		_barStamina->setMax(current->stamina);
	}
	else
		_barStamina->setMax(initial->stamina);
	// kL_end.
	_barStamina->setValue(current->stamina);
//kL	_barStamina->setValue2(initial->stamina);
	// kL_begin:
	if (current->stamina > initial->stamina)
	{
		_barStamina->setValue2(initial->stamina);
	}
	else
		_barStamina->setValue2(current->stamina);
	// kL_end.

	std::wstringstream ss3;
	ss3 << current->health;
	_numHealth->setText(ss3.str());
//kL	_barHealth->setMax(current->health);
	// kL_begin:
	if (current->health > initial->health)
	{
		_barHealth->setMax(current->health);
	}
	else
		_barHealth->setMax(initial->health);
	// kL_end.
	_barHealth->setValue(current->health);
//kL	_barHealth->setValue2(initial->health);
	// kL_begin:
	if (current->health > initial->health)
	{
		_barHealth->setValue2(initial->health);
	}
	else
		_barHealth->setValue2(current->health);
	// kL_end.

	std::wstringstream ss4;
	ss4 << current->bravery;
	_numBravery->setText(ss4.str());
	_barBravery->setMax(current->bravery);
	_barBravery->setValue(current->bravery);
	_barBravery->setValue2(initial->bravery);

	std::wstringstream ss5;
	ss5 << current->reactions;
	_numReactions->setText(ss5.str());
	_barReactions->setMax(current->reactions);
	_barReactions->setValue(current->reactions);
	_barReactions->setValue2(initial->reactions);

	std::wstringstream ss6;
	ss6 << current->firing;
	_numFiring->setText(ss6.str());
	_barFiring->setMax(current->firing);
	_barFiring->setValue(current->firing);
	_barFiring->setValue2(initial->firing);

	std::wstringstream ss7;
	ss7 << current->throwing;
	_numThrowing->setText(ss7.str());
	_barThrowing->setMax(current->throwing);
	_barThrowing->setValue(current->throwing);
	_barThrowing->setValue2(initial->throwing);

	std::wstringstream ss8;
	ss8 << current->strength;
	_numStrength->setText(ss8.str());
//kL	_barStrength->setMax(current->strength);
	// kL_begin:
	if (current->strength > initial->strength)
	{
		_barStrength->setMax(current->strength);
	}
	else
		_barStrength->setMax(initial->strength);
	// kL_end.
	_barStrength->setValue(current->strength);
//kL	_barStrength->setValue2(initial->strength);
	// kL_begin:
	if (current->strength > initial->strength)
	{
		_barStrength->setValue2(initial->strength);
	}
	else
		_barStrength->setValue2(current->strength);
	// kL_end.

	std::wstring wsArmor;
	std::string armorType = s->getArmor()->getType();
/*kL	if (armorType == "STR_NONE_UC")
	{
		wsArmor = tr("STR_ARMOR_").arg(tr(armorType));
	}
	else */
	wsArmor = tr(armorType);

	_btnArmor->setText(wsArmor);
//	_txtArmor->setText(tr(s->getArmor()->getType()));

	_btnSack->setVisible(!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"));

	_txtRank->setText(tr("STR_RANK_").arg(tr(s->getRankString())));
	_txtMissions->setText(tr("STR_MISSIONS").arg(s->getMissions()));
	_txtKills->setText(tr("STR_KILLS").arg(s->getKills()));

	std::wstring craft;
	if (s->getCraft() == 0)
	{
		craft = tr("STR_NONE_UC");
	}
	else
	{
		craft = s->getCraft()->getName(_game->getLanguage());
	}
	_txtCraft->setText(tr("STR_CRAFT_").arg(craft));

	if (s->getWoundRecovery() > 0)
	{
		_txtRecovery->setText(tr("STR_WOUND_RECOVERY").arg(tr("STR_DAY", s->getWoundRecovery())));
	}
	else
	{
		_txtRecovery->setText(L"");
	}

	_txtPsionic->setVisible(s->isInPsiTraining());

	if (current->psiSkill > 0)
	{
		std::wstringstream ss14;
		ss14 << current->psiStrength;
		_numPsiStrength->setText(ss14.str());
		_barPsiStrength->setMax(current->psiStrength);
		_barPsiStrength->setValue(current->psiStrength);
		_barPsiStrength->setValue2(initial->psiStrength);

		std::wstringstream ss15;
		ss15 << current->psiSkill;
		_numPsiSkill->setText(ss15.str());
		_barPsiSkill->setMax(current->psiSkill);
		_barPsiSkill->setValue(current->psiSkill);
		_barPsiSkill->setValue2(initial->psiSkill);

		_txtPsiStrength->setVisible(true);
		_numPsiStrength->setVisible(true);
		_barPsiStrength->setVisible(true);

		_txtPsiSkill->setVisible(true);
		_numPsiSkill->setVisible(true);
		_barPsiSkill->setVisible(true);
	}
	else
	{
		_txtPsiStrength->setVisible(false);
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);

		_txtPsiSkill->setVisible(false);
		_numPsiSkill->setVisible(false);
		_barPsiSkill->setVisible(false);
	}
}

/**
 * Changes the soldier's name.
 * @param action Pointer to an action.
 */
void SoldierInfoState::edtSoldierKeyPress(Action* action)
{
	if (action->getDetails()->key.keysym.sym == SDLK_RETURN
		|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
	{
		_base->getSoldiers()->at(_soldier)->setName(_edtSoldier->getText());
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnOkClick(Action* )
{
	_base->getSoldiers()->at(_soldier)->setName(_edtSoldier->getText());
	_game->popState();
}

/**
 * Goes to the previous soldier.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnPrevClick(Action* )
{
	_edtSoldier->deFocus();
	_base->getSoldiers()->at(_soldier)->setName(_edtSoldier->getText());

	if (_soldier == 0)
		_soldier = _base->getSoldiers()->size() - 1;
	else
		_soldier--;

	init();
}

/**
 * Goes to the next soldier.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnNextClick(Action* )
{
	_edtSoldier->deFocus();
	_base->getSoldiers()->at(_soldier)->setName(_edtSoldier->getText());

	_soldier++;
	if (_soldier >= _base->getSoldiers()->size())
		_soldier = 0;

	init();
}

/**
 * Shows the Select Armor window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnArmorClick(Action* )
{
	_edtSoldier->deFocus();
	_base->getSoldiers()->at(_soldier)->setName(_edtSoldier->getText());

	if (!_base->getSoldiers()->at(_soldier)->getCraft()
		|| (_base->getSoldiers()->at(_soldier)->getCraft()
			&& _base->getSoldiers()->at(_soldier)->getCraft()->getStatus() != "STR_OUT"))
	{
		_game->pushState(new SoldierArmorState(_game, _base, _soldier));
	}
}

/**
 * Shows the Sack Soldier window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnSackClick(Action* )
{
	Soldier *soldier = _base->getSoldiers()->at(_soldier);
	_game->pushState(new SackSoldierState(_game, _base, soldier));
}

}
