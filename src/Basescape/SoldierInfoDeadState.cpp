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

#include "SoldierInfoDeadState.h"

//#include <sstream>

#include "SoldierDiaryOverviewState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldierInfoDeadState screen.
 * @param soldierId - ID of the selected soldier
 */
SoldierInfoDeadState::SoldierInfoDeadState(size_t soldierId)
	:
		_soldierId(soldierId),
		_soldier(NULL)
{
	_list = _game->getSavedGame()->getDeadSoldiers();

	_bg				= new Surface(320, 200);

	_rank			= new Surface(26, 23, 4, 4);
	_gender			= new Surface(7, 7, 240, 8);

	_txtSoldier		= new Text(179, 16, 40, 9);
	_btnDiary		= new TextButton(60, 16, 248, 8);

	_btnPrev		= new TextButton(29, 16, 0, 32);
	_btnOk			= new TextButton(49, 16, 30, 32);
	_btnNext		= new TextButton(29, 16, 80, 32);

	_txtDeath		= new Text(60, 9, 130, 36);
	_txtDate		= new Text(80, 9, 196, 36);

	_txtRank		= new Text(130, 9, 0, 49);
	_txtMissions	= new Text(100, 9, 130, 49);
	_txtKills		= new Text(100, 9, 230, 49);

	const int step = 11;
	int yPos = 80;

	_txtTimeUnits	= new Text(120, 9, 6, yPos);
	_numTimeUnits	= new Text(18, 9, 131, yPos);
	_barTimeUnits	= new Bar(234, 7, 150, yPos + 1);

	yPos += step;
	_txtStamina		= new Text(120, 9, 5, yPos);
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
	add(_btnOk,				"button",			"soldierInfo");
	add(_btnPrev,			"button",			"soldierInfo");
	add(_btnNext,			"button",			"soldierInfo");

	add(_txtSoldier,		"text1",			"soldierInfo");
	add(_txtRank,			"text1",			"soldierInfo");
	add(_txtMissions,		"text1",			"soldierInfo");
	add(_txtKills,			"text1",			"soldierInfo");
	add(_txtDeath,			"text1",			"soldierInfo");
	add(_txtDate); // text1->color2

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

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierInfoDeadState::btnOkClick,
					Options::keyCancel);

	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnPrevClick);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& SoldierInfoDeadState::btnPrevClick,
					Options::keyBattlePrevUnit);

	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnNextClick);
	_btnNext->onKeyboardPress(
					(ActionHandler)& SoldierInfoDeadState::btnNextClick,
					Options::keyBattleNextUnit);


	_txtSoldier->setBig();

	_txtDeath->setText(tr("STR_DATE_DEATH"));
	_txtDate->setColor(Palette::blockOffset(13)); // <- text1->color2

	_btnDiary->setText(tr("STR_DIARY"));
	_btnDiary->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnDiaryClick);


	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));
	_barTimeUnits->setScale();

	_txtStamina->setText(tr("STR_STAMINA"));
	_barStamina->setScale();

	_txtHealth->setText(tr("STR_HEALTH"));
	_barHealth->setScale();

	_txtBravery->setText(tr("STR_BRAVERY"));
	_barBravery->setScale();

	_txtReactions->setText(tr("STR_REACTIONS"));
	_barReactions->setScale();

	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));
	_barFiring->setScale();

	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));
	_barThrowing->setScale();

	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));
	_barMelee->setScale();

	_txtStrength->setText(tr("STR_STRENGTH"));
	_barStrength->setScale();

	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));
	_barPsiStrength->setScale();

	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));
	_barPsiSkill->setScale();
}

/**
 * dTor.
 */
SoldierInfoDeadState::~SoldierInfoDeadState()
{}

/**
 * Updates soldier stats when the dead soldier changes.
 */
void SoldierInfoDeadState::init()
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
	_txtSoldier->setText(_soldier->getName());

	SurfaceSet* const baseBits = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	baseBits->getFrame(_soldier->getRankSprite())->setX(0);
	baseBits->getFrame(_soldier->getRankSprite())->setY(0);
	baseBits->getFrame(_soldier->getRankSprite())->blit(_rank);

	_gender->clear();
	Surface* gender;
	if (_soldier->getGender() == GENDER_MALE)
		gender = _game->getResourcePack()->getSurface("GENDER_M");
	else
		gender = _game->getResourcePack()->getSurface("GENDER_F");

	if (gender != NULL)
		gender->blit(_gender);

	const SoldierDeath* const death = _soldier->getDeath();
	std::wostringstream date;
	date << death->getTime()->getDayString(_game->getLanguage());
	date << L" ";
	date << tr(death->getTime()->getMonthString());
	date << L" ";
	date << death->getTime()->getYear();
	_txtDate->setText(date.str());


	const UnitStats
		* const initial = _soldier->getInitStats(),
		* const current = _soldier->getCurrentStats();

	std::wostringstream woststr;

	woststr << current->tu;
	_numTimeUnits->setText(woststr.str());
	if (current->tu > initial->tu)
	{
		_barTimeUnits->setMax(current->tu);
		_barTimeUnits->setValue2(initial->tu);
	}
	else
	{
		_barTimeUnits->setMax(initial->tu);
		_barTimeUnits->setValue2(current->tu);
	}
	_barTimeUnits->setValue(current->tu);

	woststr.str(L"");
	woststr << current->stamina;
	_numStamina->setText(woststr.str());
	if (current->stamina > initial->stamina)
	{
		_barStamina->setMax(current->stamina);
		_barStamina->setValue2(initial->stamina);
	}
	else
	{
		_barStamina->setMax(initial->stamina);
		_barStamina->setValue2(current->stamina);
	}
	_barStamina->setValue(current->stamina);

	woststr.str(L"");
	woststr << current->health;
	_numHealth->setText(woststr.str());
	if (current->health > initial->health)
	{
		_barHealth->setMax(current->health);
		_barHealth->setValue2(initial->health);
	}
	else
	{
		_barHealth->setMax(initial->health);
		_barHealth->setValue2(current->health);
	}
	_barHealth->setValue(current->health);

	woststr.str(L"");
	woststr << current->bravery;
	_numBravery->setText(woststr.str());
	_barBravery->setMax(current->bravery);
	_barBravery->setValue(current->bravery);
	_barBravery->setValue2(initial->bravery);

	woststr.str(L"");
	woststr << current->reactions;
	_numReactions->setText(woststr.str());
	_barReactions->setMax(current->reactions);
	_barReactions->setValue(current->reactions);
	_barReactions->setValue2(initial->reactions);

	woststr.str(L"");
	woststr << current->firing;
	_numFiring->setText(woststr.str());
	_barFiring->setMax(current->firing);
	_barFiring->setValue(current->firing);
	_barFiring->setValue2(initial->firing);

	woststr.str(L"");
	woststr << current->throwing;
	_numThrowing->setText(woststr.str());
	_barThrowing->setMax(current->throwing);
	_barThrowing->setValue(current->throwing);
	_barThrowing->setValue2(initial->throwing);

	woststr.str(L"");
	woststr << current->melee;
	_numMelee->setText(woststr.str());
	_barMelee->setMax(current->melee);
	_barMelee->setValue(current->melee);
	_barMelee->setValue2(initial->melee);

	woststr.str(L"");
	woststr << current->strength;
	_numStrength->setText(woststr.str());
	if (current->strength > initial->strength)
	{
		_barStrength->setMax(current->strength);
		_barStrength->setValue2(initial->strength);
	}
	else
	{
		_barStrength->setMax(initial->strength);
		_barStrength->setValue2(current->strength);
	}
	_barStrength->setValue(current->strength);


	_txtRank->setText(tr("STR_RANK_").arg(tr(_soldier->getRankString())));
	_txtMissions->setText(tr("STR_MISSIONS").arg(_soldier->getMissions()));
	_txtKills->setText(tr("STR_KILLS").arg(_soldier->getKills()));

//	const int minPsi = _game->getRuleset()->getSoldier("XCOM")->getMinStats().psiSkill;
//		|| (Options::psiStrengthEval == true // for determination to show psiStrength
//			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements()) == true))

	if (current->psiSkill > 0) //>= minPsi)
	{
		woststr.str(L"");
		woststr << current->psiStrength;
		_numPsiStrength->setText(woststr.str());
		_barPsiStrength->setMax(current->psiStrength);
		_barPsiStrength->setValue(current->psiStrength);
		_barPsiStrength->setValue2(initial->psiStrength);

		_numPsiStrength->setVisible();
		_barPsiStrength->setVisible();

		woststr.str(L"");
		woststr << current->psiSkill;
		_numPsiSkill->setText(woststr.str());
		_barPsiSkill->setMax(current->psiSkill);
		_barPsiSkill->setValue(current->psiSkill);
		_barPsiSkill->setValue2(initial->psiSkill);

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
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the previous soldier.
 * kL: reversed these because SoldierMemorialState uses a reversed vector.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnNextClick(Action*)
{
	if (_soldierId == 0)
		_soldierId = _list->size() - 1;
	else
		--_soldierId;

	init();
}

/**
 * Goes to the next soldier.
 * kL: reversed these because SoldierMemorialState uses a reversed vector.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnPrevClick(Action*)
{
	++_soldierId;

	if (_soldierId >= _list->size())
		_soldierId = 0;

	init();
}

/**
 * Set the soldier Id.
 * @param soldierId - the ID for the current soldier
 */
void SoldierInfoDeadState::setSoldierID(size_t soldierId)
{
	_soldierId = soldierId;
}

/**
 * Shows the Diary Soldier window.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnDiaryClick(Action*)
{
	_game->pushState(new SoldierDiaryOverviewState(
												NULL,
												_soldierId,
												NULL,
												this));
}

}
