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
#include "SoldierArmorState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Info screen.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected Soldier
 */
SoldierInfoState::SoldierInfoState(
		Base* base,
		size_t soldierId)
	:
		_base(base),
		_soldierId(soldierId),
		_soldier(NULL),
		_allowExit(true)
{
	_list = _base->getSoldiers();

	_bg				= new InteractiveSurface(320, 200);

	_rank			= new Surface(26, 23,  4, 4);
	_gender			= new Surface( 7, 7, 240, 8);

	_battleOrder	= new NumberText(15, 5, 4, -6);

	_btnOk			= new TextButton(49, 16,   0, 32);
	_btnPrev		= new TextButton(29, 16,  50, 32);
	_btnNext		= new TextButton(29, 16,  80, 32);
	_btnAutoStat	= new TextButton(69, 16, 112, 32);

	_txtArmor		= new Text(30, 9, 190, 36);
	_btnArmor		= new TextButton(100, 16, 220, 32);

	_btnSack		= new TextButton(36, 16, 284, 49);

	_edtSoldier		= new TextEdit(this, 179, 16, 40, 9);
	_btnDiary		= new TextButton(60, 16, 248, 8);

	_txtRank		= new Text(112, 9, 0, 49);
	_txtCraft		= new Text(112, 9, 0, 57);
	_txtPsionic		= new Text( 75, 9, 6, 67);

	_txtMissions	= new Text(80, 9, 112, 49);
	_txtKills		= new Text(80, 9, 112, 57);

	_txtRecovery	= new Text(45, 9, 192, 57);
	_txtDay			= new Text(30, 9, 237, 57);


	int yPos = 80;
	const int step = 11;

	_txtTimeUnits	= new Text(120, 9,   6, yPos);
	_numTimeUnits	= new Text( 18, 9, 131, yPos);
	_barTimeUnits	= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtStamina		= new Text(120, 9,   6, yPos);
	_numStamina		= new Text( 18, 9, 131, yPos);
	_barStamina		= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtHealth		= new Text(120, 9,   6, yPos);
	_numHealth		= new Text( 18, 9, 131, yPos);
	_barHealth		= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtBravery		= new Text(120, 9,   6, yPos);
	_numBravery		= new Text( 18, 9, 131, yPos);
	_barBravery		= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtReactions	= new Text(120, 9,   6, yPos);
	_numReactions	= new Text( 18, 9, 131, yPos);
	_barReactions	= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtFiring		= new Text(120, 9,   6, yPos);
	_numFiring		= new Text( 18, 9, 131, yPos);
	_barFiring		= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtThrowing	= new Text(120, 9,   6, yPos);
	_numThrowing	= new Text( 18, 9, 131, yPos);
	_barThrowing	= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtMelee		= new Text(120, 9,   6, yPos);
	_numMelee		= new Text( 18, 9, 131, yPos);
	_barMelee		= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtStrength	= new Text(120, 9,   6, yPos);
	_numStrength	= new Text( 18, 9, 131, yPos);
	_barStrength	= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtPsiStrength	= new Text(120, 9,   6, yPos);
	_numPsiStrength	= new Text( 18, 9, 131, yPos);
	_barPsiStrength	= new Bar( 234, 7, 150, yPos + 1);

	yPos += step;
	_txtPsiSkill	= new Text(120, 9,   6, yPos);
	_numPsiSkill	= new Text( 18, 9, 131, yPos);
	_barPsiSkill	= new Bar( 234, 7, 150, yPos + 1);

	setInterface("soldierInfo");

	add(_bg);
	add(_rank);
	add(_gender);
	add(_battleOrder);

	add(_btnOk,				"button",			"soldierInfo");
	add(_btnPrev,			"button",			"soldierInfo");
	add(_btnNext,			"button",			"soldierInfo");
	add(_btnAutoStat,		"button",			"soldierInfo");

	add(_edtSoldier,		"text1",			"soldierInfo");
	add(_txtRank,			"text1",			"soldierInfo");
	add(_txtMissions,		"text1",			"soldierInfo");
	add(_txtKills,			"text1",			"soldierInfo");
	add(_txtCraft,			"text1",			"soldierInfo");
	add(_txtRecovery,		"text1",			"soldierInfo");
	add(_txtDay);
	add(_txtPsionic);

	add(_txtArmor,			"text1",			"soldierInfo");
	add(_btnArmor,			"button",			"soldierInfo");
	add(_btnSack,			"button",			"soldierInfo");
	add(_btnDiary,			"button",			"soldierInfo");

	add(_txtTimeUnits,		"text2",			"soldierInfo");
	add(_numTimeUnits,		"numbers",			"soldierInfo");
	add(_barTimeUnits,		"barTUs",			"soldierInfo");

	add(_txtStamina,		"text2",			"soldierInfo");
	add(_numStamina,		"numbers",			"soldierInfo");
	add(_barStamina,		"barEnergy",		"soldierInfo");

	add(_txtHealth,			"text2",			"soldierInfo");
	add(_numHealth,			"numbers",			"soldierInfo");
	add(_barHealth,			"barHealth",		"soldierInfo");

	add(_txtBravery,		"text2",			"soldierInfo");
	add(_numBravery,		"numbers",			"soldierInfo");
	add(_barBravery,		"barBravery",		"soldierInfo");

	add(_txtReactions,		"text2",			"soldierInfo");
	add(_numReactions,		"numbers",			"soldierInfo");
	add(_barReactions,		"barReactions",		"soldierInfo");

	add(_txtFiring,			"text2",			"soldierInfo");
	add(_numFiring,			"numbers",			"soldierInfo");
	add(_barFiring,			"barFiring",		"soldierInfo");

	add(_txtThrowing,		"text2",			"soldierInfo");
	add(_numThrowing,		"numbers",			"soldierInfo");
	add(_barThrowing,		"barThrowing",		"soldierInfo");

	add(_txtMelee,			"text2",			"soldierInfo");
	add(_numMelee,			"numbers",			"soldierInfo");
	add(_barMelee,			"barMelee",			"soldierInfo");

	add(_txtStrength,		"text2",			"soldierInfo");
	add(_numStrength,		"numbers",			"soldierInfo");
	add(_barStrength,		"barStrength",		"soldierInfo");

	add(_txtPsiStrength,	"text2",			"soldierInfo");
	add(_numPsiStrength,	"numbers",			"soldierInfo");
	add(_barPsiStrength,	"barPsiStrength",	"soldierInfo");

	add(_txtPsiSkill,		"text2",			"soldierInfo");
	add(_numPsiSkill,		"numbers",			"soldierInfo");
	add(_barPsiSkill,		"barPsiSkill",		"soldierInfo");

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("BACK06.SCR")->blit(_bg);

	_bg->onMouseClick(
					(ActionHandler)& SoldierInfoState::exitClick,
					SDL_BUTTON_RIGHT);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnOkClick,
					Options::keyCancel);

	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierInfoState::btnPrevClick);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnPrevClick,
					Options::keyBattlePrevUnit);

	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& SoldierInfoState::btnNextClick);
	_btnNext->onKeyboardPress(
					(ActionHandler)& SoldierInfoState::btnNextClick,
					Options::keyBattleNextUnit);


	_btnArmor->onMouseClick((ActionHandler)& SoldierInfoState::btnArmorClick);

	_edtSoldier->setBig();
	_edtSoldier->onChange((ActionHandler)& SoldierInfoState::edtSoldierChange);

	_btnSack->setText(tr("STR_SACK"));
	_btnSack->onMouseClick((ActionHandler)& SoldierInfoState::btnSackClick);

	_btnDiary->setText(tr("STR_DIARY"));
	_btnDiary->onMouseClick((ActionHandler)& SoldierInfoState::btnDiaryClick);

	_btnAutoStat->setText(tr("STR_AUTOSTAT"));
	_btnAutoStat->onMouseClick(
					(ActionHandler)& SoldierInfoState::btnAutoStat,
					SDL_BUTTON_LEFT);
	_btnAutoStat->onMouseClick(
					(ActionHandler)& SoldierInfoState::btnAutoStatAll,
					SDL_BUTTON_RIGHT);

	_txtArmor->setText(tr("STR_ARMOR"));

	_txtDay->setHighContrast();

	_txtPsionic->setColor(Palette::blockOffset(10));
	_txtPsionic->setHighContrast();
	_txtPsionic->setText(tr("STR_IN_PSIONIC_TRAINING"));


	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));
//	_barTimeUnits->setScale();

	_txtStamina->setText(tr("STR_STAMINA"));
//	_barStamina->setScale();

	_txtHealth->setText(tr("STR_HEALTH"));
//	_barHealth->setScale();

	_txtBravery->setText(tr("STR_BRAVERY"));
//	_barBravery->setScale();

	_txtReactions->setText(tr("STR_REACTIONS"));
//	_barReactions->setScale();

	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));
//	_barFiring->setScale();

	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));
//	_barThrowing->setScale();

	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));
//	_barMelee->setScale();

	_txtStrength->setText(tr("STR_STRENGTH"));
//	_barStrength->setScale();

	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));
//	_barPsiStrength->setScale();

	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));
//	_barPsiSkill->setScale();
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

	if (_soldierId >= _list->size())
		_soldierId = 0;

	_soldier = _list->at(_soldierId);
	_edtSoldier->setText(_soldier->getName());

	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	srt->getFrame(_soldier->getRankSprite())->setX(0);
	srt->getFrame(_soldier->getRankSprite())->setY(0);
	srt->getFrame(_soldier->getRankSprite())->blit(_rank);

	_gender->clear();
	Surface* gender;
	if (_soldier->getGender() == GENDER_MALE)
		gender = _game->getResourcePack()->getSurface("GENDER_M");
	else
		gender = _game->getResourcePack()->getSurface("GENDER_F");
	if (gender != NULL)
		gender->blit(_gender);

	_battleOrder->setValue(_soldierId + 1);


	const UnitStats
		* const initial = _soldier->getInitStats(),
		* const current = _soldier->getCurrentStats();

	UnitStats armored = *current;
	armored += *(_soldier->getArmor()->getStats());

	// Special handling for stats that could go below initial setting.
	std::wostringstream woststr;

	woststr << armored.tu;
	_numTimeUnits->setText(woststr.str());
	_barTimeUnits->setValue(armored.tu);
	if (initial->tu > current->tu)
		_barTimeUnits->setMaxValue(initial->tu);
	else
		_barTimeUnits->setMaxValue(current->tu);

	if (armored.tu > initial->tu)
		_barTimeUnits->setValue2(initial->tu);
	else
	{
		if (armored.tu > current->tu)
			_barTimeUnits->setValue2(current->tu);
		else
			_barTimeUnits->setValue2(armored.tu);
	}

	woststr.str(L"");
	woststr << armored.stamina;
	_numStamina->setText(woststr.str());
	_barStamina->setValue(armored.stamina);
	if (initial->stamina > current->stamina)
		_barStamina->setMaxValue(initial->stamina);
	else
		_barStamina->setMaxValue(current->stamina);

	if (armored.stamina > initial->stamina)
		_barStamina->setValue2(initial->stamina);
	else
	{
		if (armored.stamina > current->stamina)
			_barStamina->setValue2(current->stamina);
		else
			_barStamina->setValue2(armored.stamina);
	}

	woststr.str(L"");
	woststr << armored.health;
	_numHealth->setText(woststr.str());
	_barHealth->setValue(armored.health);
	if (initial->health > current->health)
		_barHealth->setMaxValue(initial->health);
	else
		_barHealth->setMaxValue(current->health);

	if (armored.health > initial->health)
		_barHealth->setValue2(initial->health);
	else
	{
		if (armored.health > current->health)
			_barHealth->setValue2(current->health);
		else
			_barHealth->setValue2(armored.health);
	}

	woststr.str(L"");
	woststr << armored.bravery;
	_numBravery->setText(woststr.str());
	_barBravery->setValue(armored.bravery);
	if (initial->bravery > current->bravery)
		_barBravery->setMaxValue(initial->bravery);
	else
		_barBravery->setMaxValue(current->bravery);

	if (armored.bravery > initial->bravery)
		_barBravery->setValue2(initial->bravery);
	else
	{
		if (armored.bravery > current->bravery)
			_barBravery->setValue2(current->bravery);
		else
			_barBravery->setValue2(armored.bravery);
	}

	woststr.str(L"");
	woststr << armored.reactions;
	_numReactions->setText(woststr.str());
	_barReactions->setValue(armored.reactions);
	if (initial->reactions > current->reactions)
		_barReactions->setMaxValue(initial->reactions);
	else
		_barReactions->setMaxValue(current->reactions);

	if (armored.reactions > initial->reactions)
		_barReactions->setValue2(initial->reactions);
	else
	{
		if (armored.reactions > current->reactions)
			_barReactions->setValue2(current->reactions);
		else
			_barReactions->setValue2(armored.reactions);
	}

	woststr.str(L"");
	woststr << armored.firing;
	_numFiring->setText(woststr.str());
	_barFiring->setValue(armored.firing);
	if (initial->firing > current->firing)
		_barFiring->setMaxValue(initial->firing);
	else
		_barFiring->setMaxValue(current->firing);

	if (armored.firing > initial->firing)
		_barFiring->setValue2(initial->firing);
	else
	{
		if (armored.firing > current->firing)
			_barFiring->setValue2(current->firing);
		else
			_barFiring->setValue2(armored.firing);
	}

	woststr.str(L"");
	woststr << armored.throwing;
	_numThrowing->setText(woststr.str());
	_barThrowing->setValue(armored.throwing);
	if (initial->throwing > current->throwing)
		_barThrowing->setMaxValue(initial->throwing);
	else
		_barThrowing->setMaxValue(current->throwing);

	if (armored.throwing > initial->throwing)
		_barThrowing->setValue2(initial->throwing);
	else
	{
		if (armored.throwing > current->throwing)
			_barThrowing->setValue2(current->throwing);
		else
			_barThrowing->setValue2(armored.throwing);
	}

	woststr.str(L"");
	woststr << armored.melee;
	_numMelee->setText(woststr.str());
	_barMelee->setValue(armored.melee);
	if (initial->melee > current->melee)
		_barMelee->setMaxValue(initial->melee);
	else
		_barMelee->setMaxValue(current->melee);

	if (armored.melee > initial->melee)
		_barMelee->setValue2(initial->melee);
	else
	{
		if (armored.melee > current->melee)
			_barMelee->setValue2(current->melee);
		else
			_barMelee->setValue2(armored.melee);
	}

	woststr.str(L"");
	woststr << armored.strength;
	_numStrength->setText(woststr.str());
	_barStrength->setValue(armored.strength);
	if (initial->strength > current->strength)
		_barStrength->setMaxValue(initial->strength);
	else
		_barStrength->setMaxValue(current->strength);

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
	if (_soldier->getCraft() == NULL
		|| _soldier->getCraft()->getCraftStatus() != "STR_OUT")
	{
		_btnArmor->setColor(PURPLE);
	}
	else
		_btnArmor->setColor(PURPLE_GHOST);

	std::wstring craft;
	if (_soldier->getCraft() == NULL)
		craft = tr("STR_NONE_UC");
	else
		craft = _soldier->getCraft()->getName(_game->getLanguage());
	_txtCraft->setText(tr("STR_CRAFT_").arg(craft));


	const int recovery = _soldier->getRecovery();
	if (recovery != 0)
	{
		Uint8 color;
		const int pct = _soldier->getRecoveryPct();
		if (pct > 50)
			color = ORANGE;
		else if (pct > 10)
			color = YELLOW;
		else
			color = GREEN;

		_txtRecovery->setSecondaryColor(color);
		_txtRecovery->setText(tr("STR_WOUND_RECOVERY").arg(tr("STR_DAY", recovery)));
		_txtRecovery->setVisible();

		_txtDay->setColor(color);
		_txtDay->setText(tr("STR_DAY", recovery));
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

	_txtPsionic->setVisible(_soldier->inPsiTraining());

//	const int minPsi = _soldier->getRules()->getMinStats().psiSkill;
//		|| (Options::psiStrengthEval == true // for determination to show psiStrength
//			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements()) == true))
	if (current->psiSkill != 0)
	{
		woststr.str(L"");
		woststr << armored.psiStrength;
		_numPsiStrength->setText(woststr.str());
		_barPsiStrength->setValue(armored.psiStrength);
		if (initial->psiStrength > current->psiStrength)
			_barPsiStrength->setMaxValue(initial->psiStrength);
		else
			_barPsiStrength->setMaxValue(current->psiStrength);

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

		woststr.str(L"");
		woststr << armored.psiSkill;
		_numPsiSkill->setText(woststr.str());
		_barPsiSkill->setValue(armored.psiSkill);
		if (initial->psiSkill > current->psiSkill)
			_barPsiSkill->setMaxValue(initial->psiSkill);
		else
			_barPsiSkill->setMaxValue(current->psiSkill);

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
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);

		_numPsiSkill->setVisible(false);
		_barPsiSkill->setVisible(false);
	}

	_btnSack->setVisible(!
						(_soldier->getCraft() != NULL
							&& _soldier->getCraft()->getCraftStatus() == "STR_OUT")
					&& _game->getSavedGame()->getMonthsPassed() != -1);
}

/**
 * Handles autoStat click.
 * @note Left-click on the Auto-stat button.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnAutoStat(Action*)
{
	_soldier->setName(_edtSoldier->getText());
	_soldier->autoStat();

	init();
}

/**
 * Handles autoStatAll click.
 * @note Right-click on the Auto-stat button.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnAutoStatAll(Action*)
{
	_allowExit = false;

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

	// TEST: for updating Soldier Awards ***
	// note that decorationLevels need to also be zero'd in the savedgame file.
/*	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			(*j)->getDiary()->manageAwards(_game->getRuleset());
		}
	} */
}

/**
 * Sets the soldier ID.
 */
void SoldierInfoState::setSoldierId(size_t soldierId)
{
	_soldierId = soldierId;
}

/**
 * Changes the soldier's name.
 * @param action - pointer to an Action
 */
void SoldierInfoState::edtSoldierChange(Action*)
{
	_soldier->setName(_edtSoldier->getText());
}

/**
 * Disables the soldier input.
 * @param action - pointer to an Action
 */
/* void SoldierInfoState::edtSoldierPress(Action*)
{
	// kL_note: This should never happen, because (base=NULL) is handled by SoldierInfoDeadState.
	if (_base == NULL)
		_edtSoldier->setFocus(false);
} */

/**
 * Handles right-click exit.
 * @param action - pointer to an Action
 */
void SoldierInfoState::exitClick(Action*)
{
	if (_allowExit == true) // prevents RMB on btnAutoStatAll() from exiting state.
	{
		kL_soundPop->play(Mix_GroupAvailable(0));
		_game->popState();
	}
	else
		_allowExit = true;
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnOkClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierId)->setName(_edtSoldier->getText());
//	_edtSoldier->setFocus(false);

	_game->popState();
}

/**
 * Goes to the previous soldier.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnPrevClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierId)->setName(_edtSoldier->getText());
//	_edtSoldier->setFocus(false);

	if (_soldierId == 0)
		_soldierId = _list->size() - 1;
	else
		--_soldierId;

	init();
}

/**
 * Goes to the next soldier.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnNextClick(Action*)
{
//	_edtSoldier->deFocus();
//	_base->getSoldiers()->at(_soldierId)->setName(_edtSoldier->getText());
//	_edtSoldier->setFocus(false);

	++_soldierId;

	if (_soldierId >= _list->size())
		_soldierId = 0;

	init();
}

/**
 * Shows the Select Armor window.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnArmorClick(Action*)
{
	if (_soldier->getCraft() == NULL
		|| _soldier->getCraft()->getCraftStatus() != "STR_OUT")
	{
		_game->pushState(new SoldierArmorState(
											_base,
											_soldierId));
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
										_soldierId));
}

/**
 * Shows the Diary Soldier window.
 * @param action - pointer to an Action
 */
void SoldierInfoState::btnDiaryClick(Action*)
{
	_game->pushState(new SoldierDiaryOverviewState(
												_base,
												_soldierId,
												this,
												NULL));
}

}
