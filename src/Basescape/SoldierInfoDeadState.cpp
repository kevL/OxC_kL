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

#include "SoldierInfoDeadState.h"

//#include <sstream>

#include "SoldierDiaryOverviewState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
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
 * @param soldierID - ID of the selected soldier
 */
SoldierInfoDeadState::SoldierInfoDeadState(size_t soldierID)
	:
		_soldierID(soldierID),
		_soldier(NULL)
{
	_list = _game->getSavedGame()->getDeadSoldiers();

	_bg				= new Surface(320, 200, 0, 0);

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
	add(_btnOk);
	add(_btnPrev);
	add(_btnNext);

	add(_txtSoldier);
	add(_txtRank);
	add(_gender);
	add(_txtMissions);
	add(_txtKills);
	add(_txtDate);
	add(_txtDeath);

	add(_btnDiary);

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
	_btnOk->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierInfoDeadState::btnOkClick,
					Options::keyCancel);

	_btnPrev->setColor(Palette::blockOffset(15)+6);
	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnPrevClick);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& SoldierInfoDeadState::btnPrevClick,
					Options::keyBattlePrevUnit);

	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnNextClick);
	_btnNext->onKeyboardPress(
					(ActionHandler)& SoldierInfoDeadState::btnNextClick,
					Options::keyBattleNextUnit);


	_txtSoldier->setColor(Palette::blockOffset(13)+10);
	_txtSoldier->setBig();

	_txtRank->setColor(Palette::blockOffset(13)+10);
	_txtRank->setSecondaryColor(Palette::blockOffset(13));

	_txtMissions->setColor(Palette::blockOffset(13)+10);
	_txtMissions->setSecondaryColor(Palette::blockOffset(13));

	_txtKills->setColor(Palette::blockOffset(13)+10);
	_txtKills->setSecondaryColor(Palette::blockOffset(13));

	_txtDeath->setColor(Palette::blockOffset(13)+10);
	_txtDeath->setText(tr("STR_DATE_DEATH"));

	_txtDate->setColor(Palette::blockOffset(13));

	_btnDiary->setColor(Palette::blockOffset(15)+6);
	_btnDiary->setText(tr("STR_DIARY"));
	_btnDiary->onMouseClick((ActionHandler)& SoldierInfoDeadState::btnDiaryClick);


	_txtTimeUnits->setColor(Palette::blockOffset(15)+1);
	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));

	_numTimeUnits->setColor(Palette::blockOffset(13));

	_barTimeUnits->setColor(Palette::blockOffset(3));
	_barTimeUnits->setSecondaryColor(Palette::blockOffset(3)+4);
	_barTimeUnits->setScale();
	_barTimeUnits->setInvert();


	_txtStamina->setColor(Palette::blockOffset(15)+1);
	_txtStamina->setText(tr("STR_STAMINA"));

	_numStamina->setColor(Palette::blockOffset(13));

	_barStamina->setColor(Palette::blockOffset(9));
	_barStamina->setSecondaryColor(Palette::blockOffset(9)+4);
	_barStamina->setScale();
	_barStamina->setInvert();


	_txtHealth->setColor(Palette::blockOffset(15)+1);
	_txtHealth->setText(tr("STR_HEALTH"));

	_numHealth->setColor(Palette::blockOffset(13));

	_barHealth->setColor(Palette::blockOffset(2));
	_barHealth->setSecondaryColor(Palette::blockOffset(2)+4);
	_barHealth->setScale();
	_barHealth->setInvert();


	_txtBravery->setColor(Palette::blockOffset(15)+1);
	_txtBravery->setText(tr("STR_BRAVERY"));

	_numBravery->setColor(Palette::blockOffset(13));

	_barBravery->setColor(Palette::blockOffset(4));
	_barBravery->setSecondaryColor(Palette::blockOffset(4)+4);
	_barBravery->setScale();
	_barBravery->setInvert();


	_txtReactions->setColor(Palette::blockOffset(15)+1);
	_txtReactions->setText(tr("STR_REACTIONS"));

	_numReactions->setColor(Palette::blockOffset(13));

	_barReactions->setColor(Palette::blockOffset(1));
	_barReactions->setSecondaryColor(Palette::blockOffset(1)+4);
	_barReactions->setScale();
	_barReactions->setInvert();


	_txtFiring->setColor(Palette::blockOffset(15)+1);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));

	_numFiring->setColor(Palette::blockOffset(13));

	_barFiring->setColor(Palette::blockOffset(8));
	_barFiring->setSecondaryColor(Palette::blockOffset(8)+4);
	_barFiring->setScale();
	_barFiring->setInvert();


	_txtThrowing->setColor(Palette::blockOffset(15)+1);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));

	_numThrowing->setColor(Palette::blockOffset(13));

	_barThrowing->setColor(Palette::blockOffset(10));
	_barThrowing->setSecondaryColor(Palette::blockOffset(10)+4);
	_barThrowing->setScale();
	_barThrowing->setInvert();


	_txtMelee->setColor(Palette::blockOffset(15)+1);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));

	_numMelee->setColor(Palette::blockOffset(13));

	_barMelee->setColor(Palette::blockOffset(4));
	_barMelee->setSecondaryColor(Palette::blockOffset(4)+4);
	_barMelee->setScale();
	_barMelee->setInvert();


	_txtStrength->setColor(Palette::blockOffset(15)+1);
	_txtStrength->setText(tr("STR_STRENGTH"));

	_numStrength->setColor(Palette::blockOffset(13));

	_barStrength->setColor(Palette::blockOffset(5));
	_barStrength->setSecondaryColor(Palette::blockOffset(5)+4);
	_barStrength->setScale();
	_barStrength->setInvert();


	_txtPsiStrength->setColor(Palette::blockOffset(15)+1);
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));

	_numPsiStrength->setColor(Palette::blockOffset(13));

	_barPsiStrength->setColor(Palette::blockOffset(11));
	_barPsiStrength->setSecondaryColor(Palette::blockOffset(11)+4);
	_barPsiStrength->setScale();
	_barPsiStrength->setInvert();


	_txtPsiSkill->setColor(Palette::blockOffset(15)+1);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));

	_numPsiSkill->setColor(Palette::blockOffset(13));

	_barPsiSkill->setColor(Palette::blockOffset(11));
	_barPsiSkill->setSecondaryColor(Palette::blockOffset(11)+4);
	_barPsiSkill->setScale();
	_barPsiSkill->setInvert();
}

/**
 * dTor.
 */
SoldierInfoDeadState::~SoldierInfoDeadState()
{
}

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

	if (_soldierID >= _list->size())
		_soldierID = 0;

	_soldier = _list->at(_soldierID);
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

	std::wostringstream ss;

	ss << current->tu;
	_numTimeUnits->setText(ss.str());
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

	ss.str(L"");
	ss << current->stamina;
	_numStamina->setText(ss.str());
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

	ss.str(L"");
	ss << current->health;
	_numHealth->setText(ss.str());
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

	ss.str(L"");
	ss << current->bravery;
	_numBravery->setText(ss.str());
	_barBravery->setMax(current->bravery);
	_barBravery->setValue(current->bravery);
	_barBravery->setValue2(initial->bravery);

	ss.str(L"");
	ss << current->reactions;
	_numReactions->setText(ss.str());
	_barReactions->setMax(current->reactions);
	_barReactions->setValue(current->reactions);
	_barReactions->setValue2(initial->reactions);

	ss.str(L"");
	ss << current->firing;
	_numFiring->setText(ss.str());
	_barFiring->setMax(current->firing);
	_barFiring->setValue(current->firing);
	_barFiring->setValue2(initial->firing);

	ss.str(L"");
	ss << current->throwing;
	_numThrowing->setText(ss.str());
	_barThrowing->setMax(current->throwing);
	_barThrowing->setValue(current->throwing);
	_barThrowing->setValue2(initial->throwing);

	ss.str(L"");
	ss << current->melee;
	_numMelee->setText(ss.str());
	_barMelee->setMax(current->melee);
	_barMelee->setValue(current->melee);
	_barMelee->setValue2(initial->melee);

	ss.str(L"");
	ss << current->strength;
	_numStrength->setText(ss.str());
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


//	int minPsi = _soldier->getRules()->getMinStats().psiSkill;
	int minPsi = _game->getRuleset()->getSoldier("XCOM")->getMinStats().psiSkill;

	if (current->psiSkill >= minPsi
		|| (Options::psiStrengthEval == true
			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements()) == true))
	{
		ss.str(L"");
		ss << current->psiStrength;
		_numPsiStrength->setText(ss.str());
		_barPsiStrength->setMax(current->psiStrength);
		_barPsiStrength->setValue(current->psiStrength);
		_barPsiStrength->setValue2(initial->psiStrength);

//		_txtPsiStrength->setVisible();
		_numPsiStrength->setVisible();
		_barPsiStrength->setVisible();
	}
	else
	{
//		_txtPsiStrength->setVisible(false);
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	if (current->psiSkill >= minPsi)
	{
		ss.str(L"");
		ss << current->psiSkill;
		_numPsiSkill->setText(ss.str());
		_barPsiSkill->setMax(current->psiSkill);
		_barPsiSkill->setValue(current->psiSkill);
		_barPsiSkill->setValue2(initial->psiSkill);

//		_txtPsiSkill->setVisible();
		_numPsiSkill->setVisible();
		_barPsiSkill->setVisible();
	}
	else
	{
//		_txtPsiSkill->setVisible(false);
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
	if (_soldierID == 0)
		_soldierID = _list->size() - 1;
	else
		--_soldierID;

	init();
}

/**
 * Goes to the next soldier.
 * kL: reversed these because SoldierMemorialState uses a reversed vector.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnPrevClick(Action*)
{
	++_soldierID;

	if (_soldierID >= _list->size())
		_soldierID = 0;

	init();
}

/**
 * Set the soldier Id.
 * @param soldierID - the ID for the current soldier
 */
void SoldierInfoDeadState::setSoldierID(size_t soldierID)
{
	_soldierID = soldierID;
}

/**
 * Shows the Diary Soldier window.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnDiaryClick(Action*)
{
	_game->pushState(new SoldierDiaryOverviewState(
												NULL, //_base
												_soldierID,
												NULL,
												this));
}

}
