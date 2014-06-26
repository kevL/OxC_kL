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

#include "SoldierInfoState.h"

#include <sstream>

#include "SackSoldierState.h"
#include "SellState.h"
#include "SoldierArmorState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Info screen.
 * @param game, Pointer to the core game.
 * @param base, Pointer to the base to get info from. NULL to use the dead soldiers list.
 * @param soldierId, ID of the selected soldier.
 */
SoldierInfoState::SoldierInfoState(
		Base* base,
		size_t soldierId)
	:
		_base(base),
		_soldierId(soldierId),
		_soldier(NULL) // kL
{
	_list = _base->getSoldiers();

	_bg				= new Surface(320, 200, 0, 0);

	_rank			= new Surface(26, 23, 4, 4);

	_btnPrev		= new TextButton(29, 16, 0, 32);
	_btnOk			= new TextButton(49, 16, 30, 32);
	_btnNext		= new TextButton(29, 16, 80, 32);
	_btnAutoStat	= new TextButton(49, 16, 112, 32);

	_txtArmor		= new Text(30, 9, 208, 35);
	_btnArmor		= new TextButton(73, 16, 240, 32);

	_edtSoldier		= new TextEdit(this, 179, 16, 40, 9);
	_btnSack		= new TextButton(46, 17, 267, 7);

	_txtRank		= new Text(130, 9, 0, 49);
	_txtMissions	= new Text(100, 9, 130, 49);
	_txtKills		= new Text(100, 9, 230, 49);
	_txtCraft		= new Text(130, 9, 0, 57);
	_txtRecovery	= new Text(180, 9, 130, 57);
	_txtPsionic		= new Text(75, 9, 5, 67);


	int
		step = 11,
		yPos = 80;

	_txtTimeUnits	= new Text(120, 9, 6, yPos);
	_numTimeUnits	= new Text(18, 9, 131, yPos);
	_barTimeUnits	= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtStamina		= new Text(120, 9, 5, yPos);
	_numStamina		= new Text(18, 9, 131, yPos);
	_barStamina		= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtHealth		= new Text(120, 9, 6, yPos);
	_numHealth		= new Text(18, 9, 131, yPos);
	_barHealth		= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtBravery		= new Text(120, 9, 6, yPos);
	_numBravery		= new Text(18, 9, 131, yPos);
	_barBravery		= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtReactions	= new Text(120, 9, 6, yPos);
	_numReactions	= new Text(18, 9, 131, yPos);
	_barReactions	= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtFiring		= new Text(120, 9, 6, yPos);
	_numFiring		= new Text(18, 9, 131, yPos);
	_barFiring		= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtThrowing	= new Text(120, 9, 6, yPos);
	_numThrowing	= new Text(18, 9, 131, yPos);
	_barThrowing	= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtMelee		= new Text(120, 9, 6, yPos);
	_numMelee		= new Text(18, 9, 131, yPos);
	_barMelee		= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtStrength	= new Text(120, 9, 6, yPos);
	_numStrength	= new Text(18, 9, 131, yPos);
	_barStrength	= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtPsiStrength	= new Text(120, 9, 6, yPos);
	_numPsiStrength	= new Text(18, 9, 131, yPos);
	_barPsiStrength	= new Bar(170, 7, 150, yPos + 1);

	yPos += step;
	_txtPsiSkill	= new Text(120, 9, 6, yPos);
	_numPsiSkill	= new Text(18, 9, 131, yPos);
	_barPsiSkill	= new Bar(170, 7, 150, yPos + 1);

	setPalette("PAL_BASESCAPE");

	add(_bg);
	add(_rank);
	add(_btnOk);
	add(_btnPrev);
	add(_btnNext);
	add(_btnArmor);
	add(_edtSoldier);
	add(_btnAutoStat); // kL
	add(_btnSack);
	add(_txtArmor); // kL
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

	add(_txtMelee);
	add(_numMelee);
	add(_barMelee);

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

	_game->getResourcePack()->getSurface("BACK06.SCR")->blit(_bg);

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnOkClick,
					Options::keyCancel);

	_btnPrev->setColor(Palette::blockOffset(15)+6);
	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierInfoState::btnPrevClick);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnPrevClick,
					Options::keyBattlePrevUnit);

	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& SoldierInfoState::btnNextClick);
	_btnNext->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnNextClick,
					Options::keyBattleNextUnit);


	_btnArmor->setColor(Palette::blockOffset(15)+6);
	_btnArmor->onMouseClick((ActionHandler)& SoldierInfoState::btnArmorClick);

	_edtSoldier->setColor(Palette::blockOffset(13)+10);
	_edtSoldier->setBig();
	_edtSoldier->onChange((ActionHandler)& SoldierInfoState::edtSoldierChange);

	_btnSack->setColor(Palette::blockOffset(15)+6);
	_btnSack->setText(tr("STR_SACK"));
	_btnSack->onMouseClick((ActionHandler)& SoldierInfoState::btnSackClick);

	// kL_begin:
	_btnAutoStat->setColor(Palette::blockOffset(15)+6);
	_btnAutoStat->setText(tr("STR_AUTOSTAT"));
//	_btnAutoStat->onMouseClick((ActionHandler)& SoldierInfoState::btnAutoStat);
	_btnAutoStat->onMouseClick(
					(ActionHandler)& SoldierInfoState::btnAutoStat,
					SDL_BUTTON_LEFT);
	_btnAutoStat->onMouseClick(
					(ActionHandler)& SoldierInfoState::btnAutoStatAll,
					SDL_BUTTON_RIGHT);
	// kL_end.

	_txtArmor->setColor(Palette::blockOffset(13));
	_txtArmor->setText(tr("STR_ARMOR"));

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


	_txtMelee->setColor(Palette::blockOffset(15)+1);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));

	_numMelee->setColor(Palette::blockOffset(13));

	_barMelee->setColor(Palette::blockOffset(4));
	_barMelee->setColor2(Palette::blockOffset(4)+4);
	_barMelee->setScale(1.0);
	_barMelee->setInvert(true);


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
	State::init();

	if (_list->empty())
	{
		_game->popState();

		return;
	}

	if (_soldierId >= _list->size())
		_soldierId = 0;

	_soldier = _list->at(_soldierId);
//kL	_edtSoldier->setBig();
	_edtSoldier->setText(_soldier->getName());

	SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	texture->getFrame(_soldier->getRankSprite())->setX(0);
	texture->getFrame(_soldier->getRankSprite())->setY(0);
	texture->getFrame(_soldier->getRankSprite())->blit(_rank);


	UnitStats* initial = _soldier->getInitStats();
	UnitStats* current = _soldier->getCurrentStats();

	UnitStats armored (*current); // init.
	armored += *(_soldier->getArmor()->getStats());

	// kL: special handling for stats that could go below initial setting.
	std::wostringstream ss;

	ss << armored.tu;
	_numTimeUnits->setText(ss.str());
	_barTimeUnits->setValue(armored.tu);
	if (initial->tu > current->tu)
		_barTimeUnits->setMax(initial->tu);
	else
		_barTimeUnits->setMax(current->tu);

	if (armored.tu > initial->tu)
		_barTimeUnits->setValue2(initial->tu);
	else
	{
		if (armored.tu > current->tu)
			_barTimeUnits->setValue2(current->tu);
		else
			_barTimeUnits->setValue2(armored.tu);
	}

	ss.str(L"");
	ss << armored.stamina;
	_numStamina->setText(ss.str());
	_barStamina->setValue(armored.stamina);
	if (initial->stamina > current->stamina)
		_barStamina->setMax(initial->stamina);
	else
		_barStamina->setMax(current->stamina);

	if (armored.stamina > initial->stamina)
		_barStamina->setValue2(initial->stamina);
	else
	{
		if (armored.stamina > current->stamina)
			_barStamina->setValue2(current->stamina);
		else
			_barStamina->setValue2(armored.stamina);
	}

	ss.str(L"");
	ss << armored.health;
	_numHealth->setText(ss.str());
	_barHealth->setValue(armored.health);
	if (initial->health > current->health)
		_barHealth->setMax(initial->health);
	else
		_barHealth->setMax(current->health);

	if (armored.health > initial->health)
		_barHealth->setValue2(initial->health);
	else
	{
		if (armored.health > current->health)
			_barHealth->setValue2(current->health);
		else
			_barHealth->setValue2(armored.health);
	}

	ss.str(L"");
	ss << armored.bravery;
	_numBravery->setText(ss.str());
	_barBravery->setValue(armored.bravery);
	if (initial->bravery > current->bravery)
		_barBravery->setMax(initial->bravery);
	else
		_barBravery->setMax(current->bravery);

	if (armored.bravery > initial->bravery)
		_barBravery->setValue2(initial->bravery);
	else
	{
		if (armored.bravery > current->bravery)
			_barBravery->setValue2(current->bravery);
		else
			_barBravery->setValue2(armored.bravery);
	}

	ss.str(L"");
	ss << armored.reactions;
	_numReactions->setText(ss.str());
	_barReactions->setValue(armored.reactions);
	if (initial->reactions > current->reactions)
		_barReactions->setMax(initial->reactions);
	else
		_barReactions->setMax(current->reactions);

	if (armored.reactions > initial->reactions)
		_barReactions->setValue2(initial->reactions);
	else
	{
		if (armored.reactions > current->reactions)
			_barReactions->setValue2(current->reactions);
		else
			_barReactions->setValue2(armored.reactions);
	}

	ss.str(L"");
	ss << armored.firing;
	_numFiring->setText(ss.str());
	_barFiring->setValue(armored.firing);
	if (initial->firing > current->firing)
		_barFiring->setMax(initial->firing);
	else
		_barFiring->setMax(current->firing);

	if (armored.firing > initial->firing)
		_barFiring->setValue2(initial->firing);
	else
	{
		if (armored.firing > current->firing)
			_barFiring->setValue2(current->firing);
		else
			_barFiring->setValue2(armored.firing);
	}

	ss.str(L"");
	ss << armored.throwing;
	_numThrowing->setText(ss.str());
	_barThrowing->setValue(armored.throwing);
	if (initial->throwing > current->throwing)
		_barThrowing->setMax(initial->throwing);
	else
		_barThrowing->setMax(current->throwing);

	if (armored.throwing > initial->throwing)
		_barThrowing->setValue2(initial->throwing);
	else
	{
		if (armored.throwing > current->throwing)
			_barThrowing->setValue2(current->throwing);
		else
			_barThrowing->setValue2(armored.throwing);
	}

	ss.str(L"");
	ss << armored.melee;
	_numMelee->setText(ss.str());
	_barMelee->setValue(armored.melee);
	if (initial->melee > current->melee)
		_barMelee->setMax(initial->melee);
	else
		_barMelee->setMax(current->melee);

	if (armored.melee > initial->melee)
		_barMelee->setValue2(initial->melee);
	else
	{
		if (armored.melee > current->melee)
			_barMelee->setValue2(current->melee);
		else
			_barMelee->setValue2(armored.melee);
	}

	ss.str(L"");
	ss << armored.strength;
	_numStrength->setText(ss.str());
	_barStrength->setValue(armored.strength);
	if (initial->strength > current->strength)
		_barStrength->setMax(initial->strength);
	else
		_barStrength->setMax(current->strength);

	if (armored.strength > initial->strength)
		_barStrength->setValue2(initial->strength);
	else
	{
		if (armored.strength > current->strength)
			_barStrength->setValue2(current->strength);
		else
			_barStrength->setValue2(armored.strength);
	}


//	if (_base != NULL) // kL
//	{
	std::wstring
		armor,
		craft;

	std::string armorType = _soldier->getArmor()->getType();
	armor = tr(armorType);
	_btnArmor->setText(armor);
	if (_soldier->getCraft()
		&& _soldier->getCraft()->getStatus() == "STR_OUT")
	{
		_btnArmor->setColor(Palette::blockOffset(4)+8);
	}
	else
		_btnArmor->setColor(Palette::blockOffset(15)+6);

	if (_soldier->getCraft() == NULL)
		craft = tr("STR_NONE_UC");
	else
		craft = _soldier->getCraft()->getName(_game->getLanguage());
	_txtCraft->setText(tr("STR_CRAFT_").arg(craft));
//	}


	if (_soldier->getWoundRecovery() > 0)
		_txtRecovery->setText(tr("STR_WOUND_RECOVERY")
								.arg(tr("STR_DAY", _soldier->getWoundRecovery())));
	else
		_txtRecovery->setText(L"");

	_txtRank->setText(tr("STR_RANK_").arg(tr(_soldier->getRankString())));
	_txtMissions->setText(tr("STR_MISSIONS").arg(_soldier->getMissions()));
	_txtKills->setText(tr("STR_KILLS").arg(_soldier->getKills()));

	_txtPsionic->setVisible(_soldier->isInPsiTraining());

	int minPsi = _soldier->getRules()->getMinStats().psiSkill;

	if (current->psiSkill >= minPsi
		|| (Options::psiStrengthEval
			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
	{
		ss.str(L"");
		ss << armored.psiStrength;
		_numPsiStrength->setText(ss.str());
		_barPsiStrength->setValue(armored.psiStrength);
		if (initial->psiStrength > current->psiStrength)
			_barPsiStrength->setMax(initial->psiStrength);
		else
			_barPsiStrength->setMax(current->psiStrength);

		if (armored.psiStrength > initial->psiStrength)
			_barPsiStrength->setValue2(initial->psiStrength);
		else
		{
			if (armored.psiStrength > current->psiStrength)
				_barPsiStrength->setValue2(current->psiStrength);
			else
				_barPsiStrength->setValue2(armored.psiStrength);
		}

		_txtPsiStrength->setVisible(true);
		_numPsiStrength->setVisible(true);
		_barPsiStrength->setVisible(true);
	}
	else
	{
		_txtPsiStrength->setVisible(false);
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	if (current->psiSkill >= minPsi)
	{
		ss.str(L"");
		ss << armored.psiSkill;
		_numPsiSkill->setText(ss.str());
		_barPsiSkill->setValue(armored.psiSkill);
//		_barPsiSkill->setValue2(0.0); // kL
		if (initial->psiSkill > current->psiSkill)
			_barPsiSkill->setMax(initial->psiSkill);
		else
			_barPsiSkill->setMax(current->psiSkill);

		if (armored.psiSkill > initial->psiSkill)
			_barPsiSkill->setValue2(initial->psiSkill);
		else
		{
			if (armored.psiSkill > current->psiSkill)
				_barPsiSkill->setValue2(current->psiSkill);
			else
				_barPsiSkill->setValue2(armored.psiSkill);
		}

		_txtPsiSkill->setVisible(true);
		_numPsiSkill->setVisible(true);
		_barPsiSkill->setVisible(true);
	}
	else
	{
		_txtPsiSkill->setVisible(false);
		_numPsiSkill->setVisible(false);
		_barPsiSkill->setVisible(false);
	}

	_btnSack->setVisible(!
						(_soldier->getCraft()
							&& _soldier->getCraft()->getStatus() == "STR_OUT")
						&& _game->getSavedGame()->getMonthsPassed() > -1);

/*kL
	if (_base == 0) // dead don't talk
	{
		_btnArmor->setVisible(false);
		_btnSack->setVisible(false);
		_txtCraft->setVisible(false);
	}
	else
		_btnSack->setVisible(_game->getSavedGame()->getMonthsPassed() > -1); */
}

/**
 * kL. Automatically renames a soldier according to its statistics.
 * Left-click on the Auto-stat button.
 */
void SoldierInfoState::btnAutoStat(Action*)
{
	//Log(LOG_INFO) << "SoldierInfoState::btnAutoStat()";
//	Soldier* _soldier = _base->getSoldiers()->at(_soldierId);

//	_edtSoldier->deFocus();
//	_edtSoldier->setFocus(false);
	_soldier->setName(_edtSoldier->getText());

	std::wostringstream stat;

	int rank = _soldier->getRank();
	switch (rank)
	{
		case 0: stat << "r";
		break;
		case 1: stat << "q";
		break;
		case 2: stat << "t";
		break;
		case 3: stat << "p";
		break;
		case 4: stat << "n";
		break;
		case 5: stat << "x";
		break;

		default: stat << "";
		break;
	}

	UnitStats* current = _soldier->getCurrentStats();

	stat << current->firing << ".";
	stat << current->reactions << ".";
	stat << current->strength;

	int bravery = current->bravery;
	switch (bravery)
	{
		case 10: stat << "a";
		break;
		case 20: stat << "b";
		break;
		case 30: stat << "c";
		break;
		case 40: stat << "d";
		break;
		case 50: stat << "e";
		break;
		case 60: stat << "f";
		break;
		case 70: stat << "g";
		break;
		case 80: stat << "h";
		break;
		case 90: stat << "i";
		break;
		case 100: stat << "j";
		break;

		default: stat << "";
		break;
	}

	int minPsi = _soldier->getRules()->getMinStats().psiSkill;
	if (current->psiSkill >= minPsi)
	{
		int psiStr = current->psiStrength;
		int psiSkl = current->psiSkill;

		int psiDefense = psiStr + psiSkl / 5;
		int psiAttack = psiStr * psiSkl / 100;

		stat << psiDefense;

		if (psiSkl >= _soldier->getRules()->getStatCaps().psiSkill)
			stat << ":";
		else
			stat << ".";

		stat << psiAttack;
	}

	//Log(LOG_INFO) << ". set Soldier name : " << stat;
//	_base->getSoldiers()->at(_soldierId)->setName(stat.str());
	_soldier->setName(stat.str());

	//Log(LOG_INFO) << ". re-init";
	init();
	//Log(LOG_INFO) << "SoldierInfoState::btnAutoStat() EXIT";
}

/**
 * kL. Automatically renames all soldiers according to their statistics.
 * Right-click on the Auto-stat button.
 */
void SoldierInfoState::btnAutoStatAll(Action*)
{
	Soldier* soldier = _soldier;

	for (size_t
			i = 0;
			i < _list->size();
			++i)
	{
		_soldier = _list->at(i);
		btnAutoStat(NULL);
	}

	_soldier = soldier;
}

/**
 * Disables the soldier input.
 * @param action Pointer to an action.
 */
/* void SoldierInfoState::edtSoldierPress(Action* action)
{
	if (_base == 0) // kL_note: This should never happen, because (base=0) is handled by SoldierDeadInfoState.
		_edtSoldier->setFocus(false);
} */

/**
 * Changes the soldier's name.
 * @param action Pointer to an action.
 */
void SoldierInfoState::edtSoldierChange(Action* action)
{
	_soldier->setName(_edtSoldier->getText());
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnOkClick(Action*)
{
//	_edtSoldier->deFocus(); // kL
//	_base->getSoldiers()->at(_soldierId)->setName(_edtSoldier->getText());

//	_edtSoldier->setFocus(false); // kL

	_game->popState();

	if (_game->getSavedGame()->getMonthsPassed() > -1
		&& Options::storageLimitsEnforced
		&& _base != 0
		&& _base->storesOverfull())
	{
		_game->pushState(new SellState(_base));

		_game->pushState(new ErrorMessageState(
											tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
											_palette,
											Palette::blockOffset(15)+1,
											"BACK01.SCR",
											0));
	}
}

/**
 * Goes to the previous soldier.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnPrevClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierId)->setName(_edtSoldier->getText());

//	_edtSoldier->setFocus(false); // kL

	if (_soldierId == 0)
		_soldierId = _list->size() - 1;
	else
		_soldierId--;

	init();
}

/**
 * Goes to the next soldier.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnNextClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierId)->setName(_edtSoldier->getText());

//	_edtSoldier->setFocus(false); // kL

	_soldierId++;

	if (_soldierId >= _list->size())
		_soldierId = 0;

	init();
}

/**
 * Shows the Select Armor window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnArmorClick(Action*)
{
	if (!_soldier->getCraft()
		|| (_soldier->getCraft()
			&& _soldier->getCraft()->getStatus() != "STR_OUT"))
	{
		_game->pushState(new SoldierArmorState(
											_base,
											_soldierId));
	}
}

/**
 * Shows the Sack Soldier window.
 * @param action Pointer to an action.
 */
void SoldierInfoState::btnSackClick(Action*)
{
	_game->pushState(new SackSoldierState(
										_base,
										_soldierId));
}

}
