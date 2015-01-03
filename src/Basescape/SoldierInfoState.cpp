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

#include "SoldierInfoState.h"

//#include <sstream>

#include "SoldierDiaryOverviewState.h"
#include "SackSoldierState.h"
#include "SellState.h"
#include "SoldierArmorState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
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
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Info screen.
 * @param base		- pointer to the base to get info from
 * @param soldierID	- ID of the selected soldier
 */
SoldierInfoState::SoldierInfoState(
		Base* base,
		size_t soldierID)
	:
		_base(base),
		_soldierID(soldierID),
		_soldier(NULL)
{
	_list = _base->getSoldiers();

	_bg				= new Surface(320, 200, 0, 0);

	_rank			= new Surface(26, 23, 4, 4);
	_gender			= new Surface(7, 7, 240, 8);

	_btnPrev		= new TextButton(29, 16, 0, 32);
	_btnOk			= new TextButton(49, 16, 30, 32);
	_btnNext		= new TextButton(29, 16, 80, 32);
	_btnAutoStat	= new TextButton(49, 16, 112, 32);

	_txtArmor		= new Text(30, 9, 190, 36);
	_btnArmor		= new TextButton(100, 16, 220, 32);

	_btnSack		= new TextButton(36, 16, 284, 49);

	_edtSoldier		= new TextEdit(this, 179, 16, 40, 9);
	_btnDiary		= new TextButton(60, 16, 248, 8);

	_txtRank		= new Text(112, 9, 0, 49);
	_txtCraft		= new Text(112, 9, 0, 57);
	_txtPsionic		= new Text(75, 9, 6, 67);

	_txtMissions	= new Text(80, 9, 112, 49);
	_txtKills		= new Text(80, 9, 112, 57);

	_txtRecovery	= new Text(45, 9, 192, 57);
	_txtDay			= new Text(30, 9, 237, 57);


	const int step = 11;
	int yPos = 80;

	_txtTimeUnits	= new Text(120, 9, 6, yPos);
	_numTimeUnits	= new Text(18, 9, 131, yPos);
	_barTimeUnits	= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtStamina		= new Text(120, 9, 6, yPos);
	_numStamina		= new Text(18, 9, 131, yPos);
	_barStamina		= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtHealth		= new Text(120, 9, 6, yPos);
	_numHealth		= new Text(18, 9, 131, yPos);
	_barHealth		= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtBravery		= new Text(120, 9, 6, yPos);
	_numBravery		= new Text(18, 9, 131, yPos);
	_barBravery		= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtReactions	= new Text(120, 9, 6, yPos);
	_numReactions	= new Text(18, 9, 131, yPos);
	_barReactions	= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtFiring		= new Text(120, 9, 6, yPos);
	_numFiring		= new Text(18, 9, 131, yPos);
	_barFiring		= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtThrowing	= new Text(120, 9, 6, yPos);
	_numThrowing	= new Text(18, 9, 131, yPos);
	_barThrowing	= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtMelee		= new Text(120, 9, 6, yPos);
	_numMelee		= new Text(18, 9, 131, yPos);
	_barMelee		= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtStrength	= new Text(120, 9, 6, yPos);
	_numStrength	= new Text(18, 9, 131, yPos);
	_barStrength	= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtPsiStrength	= new Text(120, 9, 6, yPos);
	_numPsiStrength	= new Text(18, 9, 131, yPos);
	_barPsiStrength	= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtPsiSkill	= new Text(120, 9, 6, yPos);
	_numPsiSkill	= new Text(18, 9, 131, yPos);
	_barPsiSkill	= new Bar(234, 7, 150, yPos + 1);

	setPalette("PAL_BASESCAPE");

	add(_bg);
	add(_rank);
	add(_gender);
	add(_btnOk, "button", "soldierInfo");
	add(_btnPrev, "button", "soldierInfo");
	add(_btnNext, "button", "soldierInfo");
	add(_btnAutoStat, "button", "soldierInfo");

	add(_edtSoldier, "text1", "soldierInfo");
	add(_txtRank, "text1", "soldierInfo");
	add(_txtMissions, "text1", "soldierInfo");
	add(_txtKills, "text1", "soldierInfo");
	add(_txtCraft, "text1", "soldierInfo");
	add(_txtRecovery);
	add(_txtDay);
	add(_txtPsionic);

	add(_txtArmor, "text1", "soldierInfo");
	add(_btnArmor, "button", "soldierInfo");
	add(_btnSack, "button", "soldierInfo");
	add(_btnDiary, "button", "soldierInfo");

	add(_txtTimeUnits, "text2", "soldierInfo");
	add(_numTimeUnits, "numbers", "soldierInfo");
	add(_barTimeUnits, "barTUs", "soldierInfo");

	add(_txtStamina, "text2", "soldierInfo");
	add(_numStamina, "numbers", "soldierInfo");
	add(_barStamina, "barEnergy", "soldierInfo");

	add(_txtHealth, "text2", "soldierInfo");
	add(_numHealth, "numbers", "soldierInfo");
	add(_barHealth, "barHealth", "soldierInfo");

	add(_txtBravery, "text2", "soldierInfo");
	add(_numBravery, "numbers", "soldierInfo");
	add(_barBravery, "barBravery", "soldierInfo");

	add(_txtReactions, "text2", "soldierInfo");
	add(_numReactions, "numbers", "soldierInfo");
	add(_barReactions, "barReactions", "soldierInfo");

	add(_txtFiring, "text2", "soldierInfo");
	add(_numFiring, "numbers", "soldierInfo");
	add(_barFiring, "barFiring", "soldierInfo");

	add(_txtThrowing, "text2", "soldierInfo");
	add(_numThrowing, "numbers", "soldierInfo");
	add(_barThrowing, "barThrowing", "soldierInfo");

	add(_txtMelee, "text2", "soldierInfo");
	add(_numMelee, "numbers", "soldierInfo");
	add(_barMelee, "barMelee", "soldierInfo");

	add(_txtStrength, "text2", "soldierInfo");
	add(_numStrength, "numbers", "soldierInfo");
	add(_barStrength, "barStrength", "soldierInfo");

	add(_txtPsiStrength, "text2", "soldierInfo");
	add(_numPsiStrength, "numbers", "soldierInfo");
	add(_barPsiStrength, "barPsiStrength", "soldierInfo");

	add(_txtPsiSkill, "text2", "soldierInfo");
	add(_numPsiSkill, "numbers", "soldierInfo");
	add(_barPsiSkill, "barPsiSkill", "soldierInfo");

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("BACK06.SCR")->blit(_bg);

//	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnOkClick,
					Options::keyCancel);

//	_btnPrev->setColor(Palette::blockOffset(15)+6);
	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierInfoState::btnPrevClick);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnPrevClick,
					Options::keyBattlePrevUnit);

//	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& SoldierInfoState::btnNextClick);
	_btnNext->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnNextClick,
					Options::keyBattleNextUnit);


//	_btnArmor->setColor(Palette::blockOffset(15)+6);
	_btnArmor->onMouseClick((ActionHandler)& SoldierInfoState::btnArmorClick);

//	_edtSoldier->setColor(Palette::blockOffset(13)+10);
	_edtSoldier->setBig();
	_edtSoldier->onChange((ActionHandler)& SoldierInfoState::edtSoldierChange);

//	_btnSack->setColor(Palette::blockOffset(15)+6);
	_btnSack->setText(tr("STR_SACK"));
	_btnSack->onMouseClick((ActionHandler)& SoldierInfoState::btnSackClick);

//	_btnDiary->setColor(Palette::blockOffset(15)+6);
	_btnDiary->setText(tr("STR_DIARY"));
	_btnDiary->onMouseClick((ActionHandler)& SoldierInfoState::btnDiaryClick);

//	_btnAutoStat->setColor(Palette::blockOffset(15)+6);
	_btnAutoStat->setText(tr("STR_AUTOSTAT"));
	_btnAutoStat->onMouseClick(
					(ActionHandler)& SoldierInfoState::btnAutoStat,
					SDL_BUTTON_LEFT);
	_btnAutoStat->onMouseClick(
					(ActionHandler)& SoldierInfoState::btnAutoStatAll,
					SDL_BUTTON_RIGHT);

//	_txtArmor->setColor(Palette::blockOffset(13));
	_txtArmor->setText(tr("STR_ARMOR"));

//	_txtRank->setColor(Palette::blockOffset(13)+10);
//	_txtRank->setSecondaryColor(Palette::blockOffset(13));

//	_txtMissions->setColor(Palette::blockOffset(13)+10);
//	_txtMissions->setSecondaryColor(Palette::blockOffset(13));

//	_txtKills->setColor(Palette::blockOffset(13)+10);
//	_txtKills->setSecondaryColor(Palette::blockOffset(13));

//	_txtCraft->setColor(Palette::blockOffset(13)+10);
//	_txtCraft->setSecondaryColor(Palette::blockOffset(13));

	_txtRecovery->setColor(Palette::blockOffset(13)+10);
	_txtRecovery->setSecondaryColor(Palette::blockOffset(13));
//	_txtRecovery->setText(tr("STR_WOUND_RECOVERY"));

	_txtDay->setHighContrast();

	_txtPsionic->setColor(Palette::blockOffset(10));
	_txtPsionic->setHighContrast();
	_txtPsionic->setText(tr("STR_IN_PSIONIC_TRAINING"));


//	_txtTimeUnits->setColor(Palette::blockOffset(15)+1);
	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));

//	_numTimeUnits->setColor(Palette::blockOffset(13));

//	_barTimeUnits->setColor(Palette::blockOffset(3));
//	_barTimeUnits->setSecondaryColor(Palette::blockOffset(3)+4);
	_barTimeUnits->setScale();
	_barTimeUnits->setInvert();


//	_txtStamina->setColor(Palette::blockOffset(15)+1);
	_txtStamina->setText(tr("STR_STAMINA"));

//	_numStamina->setColor(Palette::blockOffset(13));

//	_barStamina->setColor(Palette::blockOffset(9));
//	_barStamina->setSecondaryColor(Palette::blockOffset(9)+4);
	_barStamina->setScale();
	_barStamina->setInvert();


//	_txtHealth->setColor(Palette::blockOffset(15)+1);
	_txtHealth->setText(tr("STR_HEALTH"));

//	_numHealth->setColor(Palette::blockOffset(13));

//	_barHealth->setColor(Palette::blockOffset(2));
//	_barHealth->setSecondaryColor(Palette::blockOffset(2)+4);
	_barHealth->setScale();
	_barHealth->setInvert();


//	_txtBravery->setColor(Palette::blockOffset(15)+1);
	_txtBravery->setText(tr("STR_BRAVERY"));

//	_numBravery->setColor(Palette::blockOffset(13));

//	_barBravery->setColor(Palette::blockOffset(4));
//	_barBravery->setSecondaryColor(Palette::blockOffset(4)+4);
	_barBravery->setScale();
	_barBravery->setInvert();


//	_txtReactions->setColor(Palette::blockOffset(15)+1);
	_txtReactions->setText(tr("STR_REACTIONS"));

//	_numReactions->setColor(Palette::blockOffset(13));

//	_barReactions->setColor(Palette::blockOffset(1));
//	_barReactions->setSecondaryColor(Palette::blockOffset(1)+4);
	_barReactions->setScale();
	_barReactions->setInvert();


//	_txtFiring->setColor(Palette::blockOffset(15)+1);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));

//	_numFiring->setColor(Palette::blockOffset(13));

//	_barFiring->setColor(Palette::blockOffset(8));
//	_barFiring->setSecondaryColor(Palette::blockOffset(8)+4);
	_barFiring->setScale();
	_barFiring->setInvert();


//	_txtThrowing->setColor(Palette::blockOffset(15)+1);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));

//	_numThrowing->setColor(Palette::blockOffset(13));

//	_barThrowing->setColor(Palette::blockOffset(10));
//	_barThrowing->setSecondaryColor(Palette::blockOffset(10)+4);
	_barThrowing->setScale();
	_barThrowing->setInvert();


//	_txtMelee->setColor(Palette::blockOffset(15)+1);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));

//	_numMelee->setColor(Palette::blockOffset(13));

//	_barMelee->setColor(Palette::blockOffset(4));
//	_barMelee->setSecondaryColor(Palette::blockOffset(4)+4);
	_barMelee->setScale();
	_barMelee->setInvert();


//	_txtStrength->setColor(Palette::blockOffset(15)+1);
	_txtStrength->setText(tr("STR_STRENGTH"));

//	_numStrength->setColor(Palette::blockOffset(13));

//	_barStrength->setColor(Palette::blockOffset(5));
//	_barStrength->setSecondaryColor(Palette::blockOffset(5)+4);
	_barStrength->setScale();
	_barStrength->setInvert();


//	_txtPsiStrength->setColor(Palette::blockOffset(15)+1);
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));

//	_numPsiStrength->setColor(Palette::blockOffset(13));

//	_barPsiStrength->setColor(Palette::blockOffset(11));
//	_barPsiStrength->setSecondaryColor(Palette::blockOffset(11)+4);
	_barPsiStrength->setScale();
	_barPsiStrength->setInvert();


//	_txtPsiSkill->setColor(Palette::blockOffset(15)+1);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));

//	_numPsiSkill->setColor(Palette::blockOffset(13));

//	_barPsiSkill->setColor(Palette::blockOffset(11));
//	_barPsiSkill->setSecondaryColor(Palette::blockOffset(11)+4);
	_barPsiSkill->setScale();
	_barPsiSkill->setInvert();
}

/**
 * dTor.
 */
SoldierInfoState::~SoldierInfoState()
{}

/**
 * Updates soldier stats when the soldier changes.
 */
void SoldierInfoState::init()
{
	State::init();

	if (_list->empty() == true)
	{
		_game->popState();
		return;
	}

	if (_soldierID >= _list->size())
		_soldierID = 0;

	_soldier = _list->at(_soldierID);
	_edtSoldier->setText(_soldier->getName());

	SurfaceSet* const texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	texture->getFrame(_soldier->getRankSprite())->setX(0);
	texture->getFrame(_soldier->getRankSprite())->setY(0);
	texture->getFrame(_soldier->getRankSprite())->blit(_rank);

	_gender->clear();
	Surface* gender;
	if (_soldier->getGender() == GENDER_MALE)
		gender = _game->getResourcePack()->getSurface("GENDER_M");
	else
		gender = _game->getResourcePack()->getSurface("GENDER_F");
	if (gender != NULL)
		gender->blit(_gender);


	const UnitStats
		* const initial = _soldier->getInitStats(),
		* const current = _soldier->getCurrentStats();

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


	_btnArmor->setText(tr(_soldier->getArmor()->getType()));
	if (_soldier->getCraft() != NULL
		&& _soldier->getCraft()->getStatus() == "STR_OUT")
	{
		_btnArmor->setColor(Palette::blockOffset(4)+9); // dark pale purple
	}
	else
		_btnArmor->setColor(Palette::blockOffset(15)+6);

	std::wstring craft;
	if (_soldier->getCraft() == NULL)
		craft = tr("STR_NONE_UC");
	else
		craft = _soldier->getCraft()->getName(_game->getLanguage());
	_txtCraft->setText(tr("STR_CRAFT_").arg(craft));


	const int healTime = _soldier->getWoundRecovery();
	if (healTime > 0)
	{
		Uint8 color;
		const int woundPct = _soldier->getWoundPercent();
		if (woundPct > 50)
			color = Palette::blockOffset(6); // orange
		else if (woundPct > 10)
			color = Palette::blockOffset(9); // yellow
		else
			color = Palette::blockOffset(3); // green

		_txtRecovery->setSecondaryColor(color);
		_txtRecovery->setText(tr("STR_WOUND_RECOVERY").arg(tr("STR_DAY", healTime)));
		_txtRecovery->setVisible();

		_txtDay->setColor(color);
		_txtDay->setText(tr("STR_DAY", healTime));
		_txtDay->setVisible();
	}
	else
	{
		_txtRecovery->setText(L"");
		_txtRecovery->setVisible(false);

		_txtDay->setText(L"");
		_txtDay->setVisible(false);
	}

	_txtRank->setText(tr("STR_RANK_").arg(tr(_soldier->getRankString())));
	_txtMissions->setText(tr("STR_MISSIONS").arg(_soldier->getMissions()));
	_txtKills->setText(tr("STR_KILLS").arg(_soldier->getKills()));

	_txtPsionic->setVisible(_soldier->isInPsiTraining());

	const int minPsi = _soldier->getRules()->getMinStats().psiSkill;

	if (current->psiSkill >= minPsi
		|| (Options::psiStrengthEval == true
			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements()) == true))
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

		_numPsiStrength->setVisible();
		_barPsiStrength->setVisible();
	}
	else
	{
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	if (current->psiSkill >= minPsi)
	{
		ss.str(L"");
		ss << armored.psiSkill;
		_numPsiSkill->setText(ss.str());
		_barPsiSkill->setValue(armored.psiSkill);
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

		_numPsiSkill->setVisible();
		_barPsiSkill->setVisible();
	}
	else
	{
		_numPsiSkill->setVisible(false);
		_barPsiSkill->setVisible(false);
	}

	_btnSack->setVisible(!
							(_soldier->getCraft() != NULL
								&& _soldier->getCraft()->getStatus() == "STR_OUT")
						&& _game->getSavedGame()->getMonthsPassed() != -1);
}

/**
 * kL. Handles autoStat click.
 * Left-click on the Auto-stat button.
 */
void SoldierInfoState::btnAutoStat(Action*)
{
	_soldier->setName(_edtSoldier->getText());
	_soldier->autoStat();

	init();
}

/**
 * kL. Handles autoStatAll click.
 * Right-click on the Auto-stat button.
 */
void SoldierInfoState::btnAutoStatAll(Action*)
{
	for (size_t
			i = 0;
			i != _list->size();
			++i)
	{
		Soldier* const soldier = _list->at(i);

		if (soldier == _soldier)
			soldier->setName(_edtSoldier->getText());

		soldier->autoStat();

		if (soldier == _soldier)
			init();
	}
}

/**
 * Disables the soldier input.
 * @param action - pointer to an Action
 */
/* void SoldierInfoState::edtSoldierPress(Action* action)
{
	if (_base == NULL) // kL_note: This should never happen, because (base=NULL) is handled by SoldierInfoDeadState.
		_edtSoldier->setFocus(false);
} */

/**
 * Set the soldier Id.
 */
void SoldierInfoState::setSoldierID(size_t soldierID)
{
	_soldierID = soldierID;
}

/**
 * Changes the soldier's name.
 * @param action - pointer to an Action
 */
void SoldierInfoState::edtSoldierChange(Action* action)
{
	_soldier->setName(_edtSoldier->getText());
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnOkClick(Action*)
{
//	_edtSoldier->deFocus(); // kL
//	_base->getSoldiers()->at(_soldierID)->setName(_edtSoldier->getText());

//	_edtSoldier->setFocus(false); // kL
	_game->popState();

	if (_game->getSavedGame()->getMonthsPassed() != -1
		&& Options::storageLimitsEnforced == true
//		&& _base != NULL
		&& _base->storesOverfull() == true)
	{
		_game->pushState(new SellState(_base));
		_game->pushState(new ErrorMessageState(
											tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
											_palette,
											_game->getRuleset()->getInterface("soldierInfo")->getElement("errorMessage")->color, //Palette::blockOffset(15)+1,
											"BACK01.SCR",
											_game->getRuleset()->getInterface("soldierInfo")->getElement("errorPalette")->color)); //0
	}
}

/**
 * Goes to the previous soldier.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnPrevClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierID)->setName(_edtSoldier->getText());

//	_edtSoldier->setFocus(false); // kL

	if (_soldierID == 0)
		_soldierID = _list->size() - 1;
	else
		_soldierID--;

	init();
}

/**
 * Goes to the next soldier.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnNextClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierID)->setName(_edtSoldier->getText());

//	_edtSoldier->setFocus(false); // kL
	++_soldierID;

	if (_soldierID >= _list->size())
		_soldierID = 0;

	init();
}

/**
 * Shows the Select Armor window.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnArmorClick(Action*)
{
	if (_soldier->getCraft() == NULL
		|| (_soldier->getCraft() != NULL
			&& _soldier->getCraft()->getStatus() != "STR_OUT"))
	{
		_game->pushState(new SoldierArmorState(
											_base,
											_soldierID));
	}
}

/**
 * Shows the Sack Soldier window.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnSackClick(Action*)
{
	_game->pushState(new SackSoldierState(
										_base,
										_soldierID));
}

/**
 * Shows the Diary Soldier window.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnDiaryClick(Action*)
{
	_game->pushState(new SoldierDiaryOverviewState(
												_base,
												_soldierID,
												this,
												NULL));
}

}
