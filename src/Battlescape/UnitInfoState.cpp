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

#include "UnitInfoState.h"

//#include <sstream>

#include "BattlescapeGame.h"
#include "BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
//kL #include "../Engine/Screen.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/Base.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Unit Info screen.
 * @param unit			- pointer to the selected unit
 * @param parent		- pointer to parent Battlescape
 * @param fromInventory	- true if player is here from the inventory
 * @param mindProbe		- true if player is using a Mind Probe
 */
UnitInfoState::UnitInfoState(
		BattleUnit* unit,
		BattlescapeState* parent,
		bool fromInventory,
		bool mindProbe)
	:
		_unit(unit),
		_parent(parent),
		_fromInventory(fromInventory),
		_mindProbe(mindProbe)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_battleGame = _game->getSavedGame()->getSavedBattle();

	_bg			= new Surface(320, 200, 0, 0);
	_exit		= new InteractiveSurface(320, 180, 0, 20);
	_txtName	= new Text(288, 17, 16, 4);

	int
		step = 9,
		yPos = 38;

	_txtTimeUnits	= new Text(140, 9, 8, yPos);
	_numTimeUnits	= new Text(18, 9, 151, yPos);
	_barTimeUnits	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtEnergy		= new Text(140, 9, 8, yPos);
	_numEnergy		= new Text(18, 9, 151, yPos);
	_barEnergy		= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtHealth		= new Text(140, 9, 8, yPos);
	_numHealth		= new Text(18, 9, 151, yPos);
	_barHealth		= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtFatalWounds	= new Text(140, 9, 8, yPos);
	_numFatalWounds	= new Text(18, 9, 151, yPos);
	_barFatalWounds	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtBravery		= new Text(140, 9, 8, yPos);
	_numBravery		= new Text(18, 9, 151, yPos);
	_barBravery		= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtMorale		= new Text(140, 9, 8, yPos);
	_numMorale		= new Text(18, 9, 151, yPos);
	_barMorale		= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtReactions	= new Text(140, 9, 8, yPos);
	_numReactions	= new Text(18, 9, 151, yPos);
	_barReactions	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtFiring		= new Text(140, 9, 8, yPos);
	_numFiring		= new Text(18, 9, 151, yPos);
	_barFiring		= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtThrowing	= new Text(140, 9, 8, yPos);
	_numThrowing	= new Text(18, 9, 151, yPos);
	_barThrowing	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtMelee		= new Text(140, 9, 8, yPos);
	_numMelee		= new Text(18, 9, 151, yPos);
	_barMelee		= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtStrength	= new Text(140, 9, 8, yPos);
	_numStrength	= new Text(18, 9, 151, yPos);
	_barStrength	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtPsiStrength	= new Text(140, 9, 8, yPos);
	_numPsiStrength	= new Text(18, 9, 151, yPos);
	_barPsiStrength	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtPsiSkill	= new Text(140, 9, 8, yPos);
	_numPsiSkill	= new Text(18, 9, 151, yPos);
	_barPsiSkill	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtFrontArmor	= new Text(140, 9, 8, yPos);
	_numFrontArmor	= new Text(18, 9, 151, yPos);
	_barFrontArmor	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtLeftArmor	= new Text(140, 9, 8, yPos);
	_numLeftArmor	= new Text(18, 9, 151, yPos);
	_barLeftArmor	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtRightArmor	= new Text(140, 9, 8, yPos);
	_numRightArmor	= new Text(18, 9, 151, yPos);
	_barRightArmor	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtRearArmor	= new Text(140, 9, 8, yPos);
	_numRearArmor	= new Text(18, 9, 151, yPos);
	_barRearArmor	= new Bar(180, 5, 170, yPos + 1);

	yPos += step;
	_txtUnderArmor	= new Text(140, 9, 8, yPos);
	_numUnderArmor	= new Text(18, 9, 151, yPos);
	_barUnderArmor	= new Bar(180, 5, 170, yPos + 1);

	if (_mindProbe == false)
	{
		_btnPrev	= new TextButton(18, 18, 2, 2);
		_btnNext	= new TextButton(18, 18, 300, 2);
	}

	setPalette("PAL_BATTLESCAPE");

	add(_bg);
	add(_exit);
	add(_txtName, "textName", "stats", 0);

	add(_txtTimeUnits);
	add(_numTimeUnits);
	add(_barTimeUnits, "barTUs", "stats", 0);

	add(_txtEnergy);
	add(_numEnergy);
	add(_barEnergy, "barEnergy", "stats", 0);

	add(_txtHealth);
	add(_numHealth);
	add(_barHealth, "barHealth", "stats", 0);

	add(_txtFatalWounds);
	add(_numFatalWounds);
	add(_barFatalWounds, "barWounds", "stats", 0);

	add(_txtBravery);
	add(_numBravery);
	add(_barBravery, "barBravery", "stats", 0);

	add(_txtMorale);
	add(_numMorale);
	add(_barMorale, "barMorale", "stats", 0);

	add(_txtReactions);
	add(_numReactions);
	add(_barReactions, "barReactions", "stats", 0);

	add(_txtFiring);
	add(_numFiring);
	add(_barFiring, "barFiring", "stats", 0);

	add(_txtThrowing);
	add(_numThrowing);
	add(_barThrowing, "barThrowing", "stats", 0);

	add(_txtMelee);
	add(_numMelee);
	add(_barMelee, "barMelee", "stats", 0);

	add(_txtStrength);
	add(_numStrength);
	add(_barStrength, "barStrength", "stats", 0);

	add(_txtPsiStrength);
	add(_numPsiStrength);
	add(_barPsiStrength, "barPsiStrength", "stats", 0);

	add(_txtPsiSkill);
	add(_numPsiSkill);
	add(_barPsiSkill, "barPsiSkill", "stats", 0);

	add(_txtFrontArmor);
	add(_numFrontArmor);
	add(_barFrontArmor, "barFrontArmor", "stats", 0);

	add(_txtLeftArmor);
	add(_numLeftArmor);
	add(_barLeftArmor, "barLeftArmor", "stats", 0);

	add(_txtRightArmor);
	add(_numRightArmor);
	add(_barRightArmor, "barRightArmor", "stats", 0);

	add(_txtRearArmor);
	add(_numRearArmor);
	add(_barRearArmor, "barRearArmor", "stats", 0);

	add(_txtUnderArmor);
	add(_numUnderArmor);
	add(_barUnderArmor, "barUnderArmor", "stats", 0);

	if (_mindProbe == false)
	{
		add(_btnPrev);
		add(_btnNext);
	}

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("UNIBORD.PCK")->blit(_bg);

	_exit->onMouseClick((ActionHandler)& UnitInfoState::exitClick,
					SDL_BUTTON_RIGHT);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyCancel);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyBattleStats);

	Uint8
		color = _game->getRuleset()->getInterface("stats")->getElement("text")->color,
		color2 = _game->getRuleset()->getInterface("stats")->getElement("text")->color2;

	_txtName->setAlign(ALIGN_CENTER);
	_txtName->setBig();
//	_txtName->setColor(Palette::blockOffset(4));
	_txtName->setHighContrast();


	_txtTimeUnits->setColor(color);
//	_txtTimeUnits->setColor(Palette::blockOffset(3));
	_txtTimeUnits->setHighContrast();
	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));

	_numTimeUnits->setColor(color2);
//	_numTimeUnits->setColor(Palette::blockOffset(9));
	_numTimeUnits->setHighContrast();

//	_barTimeUnits->setColor(Palette::blockOffset(4));
	_barTimeUnits->setScale();


	_txtEnergy->setColor(color);
//	_txtEnergy->setColor(Palette::blockOffset(3));
	_txtEnergy->setHighContrast();
	_txtEnergy->setText(tr("STR_ENERGY"));

	_numEnergy->setColor(color2);
//	_numEnergy->setColor(Palette::blockOffset(9));
	_numEnergy->setHighContrast();

//	_barEnergy->setColor(Palette::blockOffset(9));
	_barEnergy->setScale();


	_txtHealth->setColor(color);
//	_txtHealth->setColor(Palette::blockOffset(3));
	_txtHealth->setHighContrast();
	_txtHealth->setText(tr("STR_HEALTH"));

	_numHealth->setColor(color2);
//	_numHealth->setColor(Palette::blockOffset(9));
	_numHealth->setHighContrast();

//	_barHealth->setColor(Palette::blockOffset(2));
//	_barHealth->setColor2(Palette::blockOffset(5)+2);
	_barHealth->setScale();


	_txtFatalWounds->setColor(color);
//	_txtFatalWounds->setColor(Palette::blockOffset(3));
	_txtFatalWounds->setHighContrast();
	_txtFatalWounds->setText(tr("STR_FATAL_WOUNDS"));

	_numFatalWounds->setColor(color2);
//	_numFatalWounds->setColor(Palette::blockOffset(9));
	_numFatalWounds->setHighContrast();

//	_barFatalWounds->setColor(Palette::blockOffset(2));
	_barFatalWounds->setScale();


	_txtBravery->setColor(color);
//	_txtBravery->setColor(Palette::blockOffset(3));
	_txtBravery->setHighContrast();
	_txtBravery->setText(tr("STR_BRAVERY"));

	_numBravery->setColor(color2);
//	_numBravery->setColor(Palette::blockOffset(9));
	_numBravery->setHighContrast();

//	_barBravery->setColor(Palette::blockOffset(12));
	_barBravery->setScale();


	_txtMorale->setColor(color);
//	_txtMorale->setColor(Palette::blockOffset(3));
	_txtMorale->setHighContrast();
	_txtMorale->setText(tr("STR_MORALE"));

	_numMorale->setColor(color2);
//	_numMorale->setColor(Palette::blockOffset(9));
	_numMorale->setHighContrast();

//	_barMorale->setColor(Palette::blockOffset(12));
	_barMorale->setScale();


	_txtReactions->setColor(color);
//	_txtReactions->setColor(Palette::blockOffset(3));
	_txtReactions->setHighContrast();
	_txtReactions->setText(tr("STR_REACTIONS"));

	_numReactions->setColor(color2);
//	_numReactions->setColor(Palette::blockOffset(9));
	_numReactions->setHighContrast();

//	_barReactions->setColor(Palette::blockOffset(9));
	_barReactions->setScale();


	_txtFiring->setColor(color);
//	_txtFiring->setColor(Palette::blockOffset(3));
	_txtFiring->setHighContrast();
	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));

	_numFiring->setColor(color2);
//	_numFiring->setColor(Palette::blockOffset(9));
	_numFiring->setHighContrast();

//	_barFiring->setColor(Palette::blockOffset(8));
	_barFiring->setScale();


	_txtThrowing->setColor(color);
//	_txtThrowing->setColor(Palette::blockOffset(3));
	_txtThrowing->setHighContrast();
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));

	_numThrowing->setColor(color2);
//	_numThrowing->setColor(Palette::blockOffset(9));
	_numThrowing->setHighContrast();

//	_barThrowing->setColor(Palette::blockOffset(6));
	_barThrowing->setScale();


	_txtMelee->setColor(color);
//	_txtMelee->setColor(Palette::blockOffset(3));
	_txtMelee->setHighContrast();
	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));

	_numMelee->setColor(color2);
//	_numMelee->setColor(Palette::blockOffset(9));
	_numMelee->setHighContrast();

//	_barMelee->setColor(Palette::blockOffset(14));
	_barMelee->setScale();


	_txtStrength->setColor(color);
//	_txtStrength->setColor(Palette::blockOffset(3));
	_txtStrength->setHighContrast();
	_txtStrength->setText(tr("STR_STRENGTH"));

	_numStrength->setColor(color2);
//	_numStrength->setColor(Palette::blockOffset(9));
	_numStrength->setHighContrast();

//	_barStrength->setColor(Palette::blockOffset(3));
	_barStrength->setScale();


	_txtPsiStrength->setColor(color);
//	_txtPsiStrength->setColor(Palette::blockOffset(3));
	_txtPsiStrength->setHighContrast();
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));

	_numPsiStrength->setColor(color2);
//	_numPsiStrength->setColor(Palette::blockOffset(9));
	_numPsiStrength->setHighContrast();

//	_barPsiStrength->setColor(Palette::blockOffset(12));
	_barPsiStrength->setScale();


	_txtPsiSkill->setColor(color);
//	_txtPsiSkill->setColor(Palette::blockOffset(3));
	_txtPsiSkill->setHighContrast();
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));

	_numPsiSkill->setColor(color2);
//	_numPsiSkill->setColor(Palette::blockOffset(9));
	_numPsiSkill->setHighContrast();

//	_barPsiSkill->setColor(Palette::blockOffset(12));
	_barPsiSkill->setScale();


	_txtFrontArmor->setColor(color);
//	_txtFrontArmor->setColor(Palette::blockOffset(3));
	_txtFrontArmor->setHighContrast();
	_txtFrontArmor->setText(tr("STR_FRONT_ARMOR_UC"));

	_numFrontArmor->setColor(color2);
//	_numFrontArmor->setColor(Palette::blockOffset(9));
	_numFrontArmor->setHighContrast();

//	_barFrontArmor->setColor(Palette::blockOffset(5));
	_barFrontArmor->setScale();


	_txtLeftArmor->setColor(color);
//	_txtLeftArmor->setColor(Palette::blockOffset(3));
	_txtLeftArmor->setHighContrast();
	_txtLeftArmor->setText(tr("STR_LEFT_ARMOR_UC"));

	_numLeftArmor->setColor(color2);
//	_numLeftArmor->setColor(Palette::blockOffset(9));
	_numLeftArmor->setHighContrast();

//	_barLeftArmor->setColor(Palette::blockOffset(5));
	_barLeftArmor->setScale();


	_txtRightArmor->setColor(color);
//	_txtRightArmor->setColor(Palette::blockOffset(3));
	_txtRightArmor->setHighContrast();
	_txtRightArmor->setText(tr("STR_RIGHT_ARMOR_UC"));

	_numRightArmor->setColor(color2);
//	_numRightArmor->setColor(Palette::blockOffset(9));
	_numRightArmor->setHighContrast();

//	_barRightArmor->setColor(Palette::blockOffset(5));
	_barRightArmor->setScale();


	_txtRearArmor->setColor(color);
//	_txtRearArmor->setColor(Palette::blockOffset(3));
	_txtRearArmor->setHighContrast();
	_txtRearArmor->setText(tr("STR_REAR_ARMOR_UC"));

	_numRearArmor->setColor(color2);
//	_numRearArmor->setColor(Palette::blockOffset(9));
	_numRearArmor->setHighContrast();

//	_barRearArmor->setColor(Palette::blockOffset(5));
	_barRearArmor->setScale();


	_txtUnderArmor->setColor(color);
//	_txtUnderArmor->setColor(Palette::blockOffset(3));
	_txtUnderArmor->setHighContrast();
	_txtUnderArmor->setText(tr("STR_UNDER_ARMOR_UC"));

	_numUnderArmor->setColor(color2);
//	_numUnderArmor->setColor(Palette::blockOffset(9));
	_numUnderArmor->setHighContrast();

//	_barUnderArmor->setColor(Palette::blockOffset(5));
	_barUnderArmor->setScale();


	if (_mindProbe == false)
	{
		_btnPrev->setText(L"<");
		_btnPrev->setColor(Palette::blockOffset(4)+4);
		_btnPrev->onMouseClick((ActionHandler)& UnitInfoState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& UnitInfoState::btnPrevClick,
						Options::keyBattlePrevUnit);
		_btnNext->setText(L">");
		_btnNext->setColor(Palette::blockOffset(4)+4);
		_btnNext->onMouseClick((ActionHandler)& UnitInfoState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& UnitInfoState::btnNextClick,
						Options::keyBattleNextUnit);
	}
}

/**
 * dTor.
 */
UnitInfoState::~UnitInfoState()
{
}

/**
 * Updates unit info which can change after going into other screens.
 */
void UnitInfoState::init()
{
	State::init();

	std::wostringstream ss;

	int minPsi = 0;
	if (_unit->getGeoscapeSoldier() != NULL)
	{
		ss << tr(_unit->getRankString());
		ss << " ";

		minPsi = _unit->getGeoscapeSoldier()->getRules()->getMinStats().psiSkill - 1;
//		minPsi = _game->getSavedGame()->getSoldier(_unit->getId())->getRules()->getMinStats().psiSkill - 1; // kL, shit..
	}
	ss << _unit->getName(_game->getLanguage(), BattlescapeGame::_debugPlay);
	_txtName->setBig();
	_txtName->setText(ss.str());

	int stat = _unit->getTimeUnits();
	ss.str(L"");
	ss << stat;
	_numTimeUnits->setText(ss.str());
	_barTimeUnits->setMax(static_cast<double>(_unit->getBaseStats()->tu));
	_barTimeUnits->setValue(static_cast<double>(stat));

	stat = _unit->getEnergy();
	ss.str(L"");
	ss << stat;
	_numEnergy->setText(ss.str());
	_barEnergy->setMax(static_cast<double>(_unit->getBaseStats()->stamina));
	_barEnergy->setValue(static_cast<double>(stat));

	stat = _unit->getHealth();
	ss.str(L"");
	ss << stat;
	_numHealth->setText(ss.str());
	_barHealth->setMax(static_cast<double>(_unit->getBaseStats()->health));
	_barHealth->setValue(static_cast<double>(stat));
	_barHealth->setValue2(static_cast<double>(_unit->getStun()));

	ss.str(L"");
	if (_unit->isWoundable() == true)
	{
		stat = _unit->getFatalWounds();
		ss << stat;

		_barFatalWounds->setMax(static_cast<double>(stat));
		_barFatalWounds->setValue(static_cast<double>(stat));
		_barFatalWounds->setVisible();
	}
	else
	{
		ss << L"-";
		_barFatalWounds->setVisible(false);
	}
	_numFatalWounds->setText(ss.str());

	stat = _unit->getBaseStats()->bravery;
	ss.str(L"");
	ss << stat;
	_numBravery->setText(ss.str());
	_barBravery->setMax(static_cast<double>(stat));
	_barBravery->setValue(static_cast<double>(stat));

	stat = _unit->getMorale();
	ss.str(L"");
	ss << stat;
	_numMorale->setText(ss.str());
	_barMorale->setMax(100);
	_barMorale->setValue(static_cast<double>(stat));

	double acuModi = _unit->getAccuracyModifier();

	double arbitraryVariable = static_cast<double>(_unit->getBaseStats()->reactions);
	_barReactions->setMax(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barReactions->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	ss.str(L"");
	ss << stat;
	_numReactions->setText(ss.str());

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->firing);
	_barFiring->setMax(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barFiring->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	ss.str(L"");
	ss << stat;
	_numFiring->setText(ss.str());

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->throwing);
	_barThrowing->setMax(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barThrowing->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	ss.str(L"");
	ss << stat;
	_numThrowing->setText(ss.str());

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->melee);
	_barMelee->setMax(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barMelee->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	ss.str(L"");
	ss << stat;
	_numMelee->setText(ss.str());

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->strength);
	_barStrength->setMax(arbitraryVariable);
	arbitraryVariable *= acuModi / 2.0 + 0.5;
	_barStrength->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	ss.str(L"");
	ss << stat;
	_numStrength->setText(ss.str());


	if (_unit->getBaseStats()->psiSkill > minPsi
		|| _unit->getGeoscapeSoldier() == NULL
		|| (Options::psiStrengthEval
			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
	{
		ss.str(L"");

		if (_unit->isFearable() == true)
		{
			stat = _unit->getBaseStats()->psiStrength;
			ss << stat;

			_barPsiStrength->setMax(static_cast<double>(stat));
			_barPsiStrength->setValue(static_cast<double>(stat));
			_barPsiStrength->setVisible();
		}
		else
		{
			ss << L"oo";
			_barPsiStrength->setVisible(false);
		}

		_numPsiStrength->setText(ss.str());
		_numPsiStrength->setVisible();
	}
	else
	{
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	if (_unit->getBaseStats()->psiSkill > minPsi)
	{
		stat = _unit->getBaseStats()->psiSkill;
		ss.str(L"");
		ss << stat;
		_numPsiSkill->setText(ss.str());
		_barPsiSkill->setMax(static_cast<double>(stat));
		_barPsiSkill->setValue(static_cast<double>(stat));

		_numPsiSkill->setVisible();
		_barPsiSkill->setVisible();
	}
	else
	{
		_numPsiSkill->setVisible(false);
		_barPsiSkill->setVisible(false);
	}


	stat = _unit->getArmor(SIDE_FRONT);
	ss.str(L"");
	ss << stat;
	_numFrontArmor->setText(ss.str());
	_barFrontArmor->setMax(static_cast<double>(_unit->getArmor()->getFrontArmor()));
	_barFrontArmor->setValue(static_cast<double>(stat));

	arbitraryVariable = _unit->getArmor()->getSideArmor(); // haha!!
	stat = _unit->getArmor(SIDE_LEFT);
	ss.str(L"");
	ss << stat;
	_numLeftArmor->setText(ss.str());
	_barLeftArmor->setMax(arbitraryVariable);
	_barLeftArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_RIGHT);
	ss.str(L"");
	ss << stat;
	_numRightArmor->setText(ss.str());
	_barRightArmor->setMax(arbitraryVariable);
	_barRightArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_REAR);
	ss.str(L"");
	ss << stat;
	_numRearArmor->setText(ss.str());
	_barRearArmor->setMax(_unit->getArmor()->getRearArmor());
	_barRearArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_UNDER);
	ss.str(L"");
	ss << stat;
	_numUnderArmor->setText(ss.str());
	_barUnderArmor->setMax(_unit->getArmor()->getUnderArmor());
	_barUnderArmor->setValue(static_cast<double>(stat));
}

/**
 * Closes the window on right-click.
 * @param action - pointer to an Action
 */
void UnitInfoState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
//		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
//			exitClick(action); else
		if (!_mindProbe)
		{
			if (action->getDetails()->button.button == SDL_BUTTON_X1)
				btnNextClick(action);
			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
				btnPrevClick(action);
		}
	}
}

/**
* Selects the previous unit.
* @param action Pointer to an action.
*/
void UnitInfoState::btnPrevClick(Action* action)
{
	if (_parent) // so we are here from a Battlescape Game
		_parent->selectPreviousFactionUnit(
										false,
										false);
//kL									_fromInventory);
	else // so we are here from the Craft Equipment screen
		_battleGame->selectPreviousFactionUnit(
											false,
											false,
											true); // no tanks.

	_unit = _battleGame->getSelectedUnit();

	if (_unit != NULL)
		init();
	else
		exitClick(action);
}

/**
 * Selects the next unit.
 * @param action - pointer to an Action
 */
void UnitInfoState::btnNextClick(Action* action)
{
	if (_parent != NULL) // this is from a Battlescape Game
		_parent->selectNextFactionUnit(
									false,
									false);
//kL								_fromInventory);
	else // this is from the Craft Equipment screen
		_battleGame->selectNextFactionUnit(
										false,
										false,
										true); // no tanks.

	_unit = _battleGame->getSelectedUnit();

	if (_unit != NULL)
		init();
	else
		exitClick(action);
}

/**
 * Exits the screen.
 * @param action - pointer to an Action
 */
void UnitInfoState::exitClick(Action*)
{
/*kL
	if (_fromInventory == false)
	{
		if (Options::maximizeInfoScreens)
		{
			Screen::updateScale(
							Options::battlescapeScale,
							Options::battlescapeScale,
							Options::baseXBattlescape,
							Options::baseYBattlescape,
							true);
			_game->getScreen()->resetDisplay(false);
		}
	} else */
	if (_fromInventory == true
		&& _unit->hasInventory() == false)
	{
		_game->popState(); // tanks require double pop if from Inventory.
	}

	_game->popState();
}

}
