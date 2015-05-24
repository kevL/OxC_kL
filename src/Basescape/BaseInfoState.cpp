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

#include "BaseInfoState.h"

//#include <sstream>

#include "BasescapeState.h"
#include "MiniBaseView.h"
#include "MonthlyCostsState.h"
#include "StoresState.h"
#include "TransfersState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleRegion.h"

#include "../Savegame/Base.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Base Info screen.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the BasescapeState
 */
BaseInfoState::BaseInfoState(
		Base* base,
		BasescapeState* state)
	:
		_base(base),
		_state(state),
		_baseList(_game->getSavedGame()->getBases())
{
	_bg					= new Surface(320,200);
	_mini				= new MiniBaseView(128, 16, 182, 8, MBV_INFO);

	_btnMonthlyCosts	= new TextButton(72, 14, 10, 179);
	_btnTransfers		= new TextButton(72, 14, 86, 179);
	_btnStores			= new TextButton(72, 14, 162, 179);
	_btnOk				= new TextButton(72, 14, 238, 179);

	_edtBase			= new TextEdit(this, 127, 16, 8, 9);

	_txtPersonnel		= new Text(171, 9, 8, 31);
	_txtHoverBase		= new Text(73, 9, 179, 31);
	_txtRegion			= new Text(60, 9, 252, 31);

	_txtSoldiers		= new Text(114, 9, 8, 41);
	_numSoldiers		= new Text(40, 9, 126, 42);
	_barSoldiers		= new Bar(280, 5, 166, 43);
	_txtScientists		= new Text(114, 9, 8, 51);
	_numScientists		= new Text(40, 9, 126, 52);
	_barScientists		= new Bar(280, 5, 166, 53);
	_txtEngineers		= new Text(114, 9, 8, 61);
	_numEngineers		= new Text(40, 9, 126, 62);
	_barEngineers		= new Bar(280, 5, 166, 63);

	_txtSpace			= new Text(300, 9, 8, 72);

	_txtQuarters		= new Text(114, 9, 8, 83);
	_numQuarters		= new Text(40, 9, 126, 84);
	_barQuarters		= new Bar(280, 5, 166, 85);
	_txtLaboratories	= new Text(114, 9, 8, 93);
	_numLaboratories	= new Text(40, 9, 126, 94);
	_barLaboratories	= new Bar(280, 5, 166, 95);
	_txtWorkshops		= new Text(114, 9, 8, 103);
	_numWorkshops		= new Text(40, 9, 126, 104);
	_barWorkshops		= new Bar(280, 5, 166, 105);
	_txtContainment		= new Text(114, 9, 8, 113);
	_numContainment		= new Text(40, 9, 126, 114);
	_barContainment		= new Bar(280, 5, 166, 115);
	_txtStores			= new Text(114, 9, 8, 123);
	_numStores			= new Text(40, 9, 126, 124);
	_barStores			= new Bar(280, 5, 166, 125);
	_txtHangars			= new Text(114, 9, 8, 133);
	_numHangars			= new Text(40, 9, 126, 134);
	_barHangars			= new Bar(280, 5, 166, 135);

	_txtLongRange		= new Text(114, 9, 8, 147);
	_numLongRange		= new Text(40, 9, 126, 148);
	_barLongRange		= new Bar(280, 5, 166, 149);
	_txtShortRange		= new Text(114, 9, 8, 157);
	_numShortRange		= new Text(40, 9, 126, 158);
	_barShortRange		= new Bar(280, 5, 166, 159);
	_txtDefense			= new Text(114, 9, 8, 167);
	_numDefense			= new Text(40, 9, 126, 168);
	_barDefense			= new Bar(280, 5, 166, 169);

	setInterface("baseInfo");

	add(_bg);
	add(_mini,				"miniBase",			"basescape");
	add(_btnOk,				"button",			"baseInfo");
	add(_btnTransfers,		"button",			"baseInfo");
	add(_btnStores,			"button",			"baseInfo");
	add(_btnMonthlyCosts,	"button",			"baseInfo");
	add(_edtBase,			"text1",			"baseInfo");

	add(_txtPersonnel,		"text1",			"baseInfo");
	add(_txtHoverBase,		"numbers",			"baseInfo");
	add(_txtRegion,			"text1",			"baseInfo");

	add(_txtSoldiers,		"text2",			"baseInfo");
	add(_numSoldiers,		"numbers",			"baseInfo");
	add(_barSoldiers,		"personnelBars",	"baseInfo");
	add(_txtEngineers,		"text2",			"baseInfo");
	add(_numEngineers,		"numbers",			"baseInfo");
	add(_barEngineers,		"personnelBars",	"baseInfo");
	add(_txtScientists,		"text2",			"baseInfo");
	add(_numScientists,		"numbers",			"baseInfo");
	add(_barScientists,		"personnelBars",	"baseInfo");

	add(_txtSpace,			"text1",			"baseInfo");
	add(_txtQuarters,		"text2",			"baseInfo");
	add(_numQuarters,		"numbers",			"baseInfo");
	add(_barQuarters,		"facilityBars",		"baseInfo");
	add(_txtStores,			"text2",			"baseInfo");
	add(_numStores,			"numbers",			"baseInfo");
	add(_barStores,			"facilityBars",		"baseInfo");
	add(_txtLaboratories,	"text2",			"baseInfo");
	add(_numLaboratories,	"numbers",			"baseInfo");
	add(_barLaboratories,	"facilityBars",		"baseInfo");
	add(_txtWorkshops,		"text2",			"baseInfo");
	add(_numWorkshops,		"numbers",			"baseInfo");
	add(_barWorkshops,		"facilityBars",		"baseInfo");

	if (Options::storageLimitsEnforced == true)
	{
		add(_txtContainment, "text2",			"baseInfo");
		add(_numContainment, "numbers",			"baseInfo");
		add(_barContainment, "facilityBars",	"baseInfo");
	}

	add(_txtHangars,	"text2",				"baseInfo");
	add(_numHangars,	"numbers",				"baseInfo");
	add(_barHangars,	"facilityBars",			"baseInfo");

	add(_txtDefense,	"text2",				"baseInfo");
	add(_numDefense,	"numbers",				"baseInfo");
	add(_barDefense,	"defenceBar",			"baseInfo");
	add(_txtShortRange,	"text2",				"baseInfo");
	add(_numShortRange,	"numbers",				"baseInfo");
	add(_barShortRange,	"detectionBars",		"baseInfo");
	add(_txtLongRange,	"text2",				"baseInfo");
	add(_numLongRange,	"numbers",				"baseInfo");
	add(_barLongRange,	"detectionBars",		"baseInfo");

	centerAllSurfaces();


	std::ostringstream oststr;
	if (Options::storageLimitsEnforced == true)
		oststr << "ALT";
	oststr << "BACK07.SCR";
	_game->getResourcePack()->getSurface(oststr.str())->blit(_bg);

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_baseList);
	for (size_t
			i = 0;
			i != _baseList->size();
			++i)
	{
		if (_baseList->at(i) == base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick((ActionHandler)& BaseInfoState::miniClick);
	_mini->onKeyboardPress((ActionHandler)& BaseInfoState::handleKeyPress);
	_mini->onMouseOver((ActionHandler)& BaseInfoState::viewMouseOver);
	_mini->onMouseOut((ActionHandler)& BaseInfoState::viewMouseOut);

	_edtBase->setBig();
	_edtBase->onChange((ActionHandler)& BaseInfoState::edtBaseChange);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseInfoState::btnOkClick,
					Options::keyCancel);

	_btnTransfers->setText(tr("STR_TRANSFERS"));
	_btnTransfers->onMouseClick((ActionHandler)& BaseInfoState::btnTransfersClick);

	_btnStores->setText(tr("STR_STORES_UC"));
	_btnStores->onMouseClick((ActionHandler)& BaseInfoState::btnStoresClick);

	_btnMonthlyCosts->setText(tr("STR_MONTHLY_COSTS"));
	_btnMonthlyCosts->onMouseClick((ActionHandler)& BaseInfoState::btnMonthlyCostsClick);

	_txtPersonnel->setText(tr("STR_PERSONNEL_AVAILABLE_PERSONNEL_TOTAL"));

	_txtRegion->setAlign(ALIGN_RIGHT);

	_txtSoldiers->setText(tr("STR_SOLDIERS"));
	_barSoldiers->setScale();

	_txtEngineers->setText(tr("STR_ENGINEERS"));
	_barEngineers->setScale();

	_txtScientists->setText(tr("STR_SCIENTISTS"));
	_barScientists->setScale();


	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_AVAILABLE"));

	_txtQuarters->setText(tr("STR_LIVING_QUARTERS_PLURAL"));
	_barQuarters->setScale(0.5);

	_txtStores->setText(tr("STR_STORES"));
	_barStores->setScale(0.25); //0.5

	_txtLaboratories->setText(tr("STR_LABORATORIES"));
	_barLaboratories->setScale(0.5);

	_txtWorkshops->setText(tr("STR_WORK_SHOPS"));
	_barWorkshops->setScale(0.5);

	if (Options::storageLimitsEnforced == true)
	{
		_txtContainment->setText(tr("STR_ALIEN_CONTAINMENT"));
		_barContainment->setScale(); //0.5
	}

	_txtHangars->setText(tr("STR_HANGARS"));
	_barHangars->setScale(18.);

	_txtDefense->setText(tr("STR_DEFENSE_STRENGTH"));
	_barDefense->setScale(0.018); // 0.125

	_txtShortRange->setText(tr("STR_SHORT_RANGE_DETECTION"));
	_barShortRange->setScale(1.25); // 25.0

	_txtLongRange->setText(tr("STR_LONG_RANGE_DETECTION"));
	_barLongRange->setScale(1.25); // 25.0
}

/**
 * dTor.
 */
BaseInfoState::~BaseInfoState()
{}

/**
 * The player can change the selected base.
 */
void BaseInfoState::init()
{
	State::init();

	_edtBase->setText(_base->getName());

	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										_base->getLongitude(),
										_base->getLatitude()))
		{
			_txtRegion->setText(tr((*i)->getRules()->getType()));
			break;
		}
	}

	std::wostringstream
		woststr1,
		woststr2,
		woststr3,
		woststr4,
		woststr5,
		woststr6,
		woststr7,
		woststr8,
		woststr9,
		woststr10,
		woststr11,
		woststr12;

	int var,
		var2;

	var = _base->getTotalSoldiers();
	var2 = _base->getAvailableSoldiers(true);
	woststr1 << var2 << ":" << var;
	_numSoldiers->setText(woststr1.str());
	_barSoldiers->setMax(var);
	_barSoldiers->setValue(var2);

	var = _base->getTotalEngineers();
	var2 = _base->getEngineers();
	woststr2 << var2 << ":" << var;
	_numEngineers->setText(woststr2.str());
	_barEngineers->setMax(var);
	_barEngineers->setValue(var2);

	var = _base->getTotalScientists();
	var2 = _base->getScientists();
	woststr3 << var2 << ":" << var;
	_numScientists->setText(woststr3.str());
	_barScientists->setMax(var);
	_barScientists->setValue(var2);

	var = _base->getAvailableQuarters();
	var2 = _base->getUsedQuarters();
	woststr4 << var2 << ":" << var;
	_numQuarters->setText(woststr4.str());
	_barQuarters->setMax(var);
	_barQuarters->setValue(var2);

	var = _base->getAvailableStores();
	var2 = static_cast<int>(std::floor(_base->getUsedStores() + 0.05));
	woststr5 << var2 << ":" << var;
	_numStores->setText(woststr5.str());
	_barStores->setMax(var);
	_barStores->setValue(var2);

	var = _base->getAvailableLaboratories();
	var2 = _base->getUsedLaboratories();
	woststr6 << var2 << ":" << var;
	_numLaboratories->setText(woststr6.str());
	_barLaboratories->setMax(var);
	_barLaboratories->setValue(var2);

	var = _base->getAvailableWorkshops();
	var2 = _base->getUsedWorkshops();
	woststr7 << var2 << ":" << var;
	_numWorkshops->setText(woststr7.str());
	_barWorkshops->setMax(var);
	_barWorkshops->setValue(var2);

	if (Options::storageLimitsEnforced)
	{
		var = _base->getAvailableContainment();
		var2 = _base->getUsedContainment();
		woststr8 << var2 << ":" << var;
		_numContainment->setText(woststr8.str());
		_barContainment->setMax(var);
		_barContainment->setValue(var2);
	}

	var = _base->getAvailableHangars();
	var2 = _base->getUsedHangars();
	woststr9 << var2 << ":" << var;
	_numHangars->setText(woststr9.str());
	_barHangars->setMax(var);
	_barHangars->setValue(var2);

	var = _base->getDefenseTotal();
	woststr10 << var;
	_numDefense->setText(woststr10.str());
	_barDefense->setMax(var);
	_barDefense->setValue(var);

	var = _base->getShortRangeTotal();
	woststr11 << var;
	_numShortRange->setText(woststr11.str());
	_barShortRange->setMax(var);
	_barShortRange->setValue(var);


	if (_base->getHyperDetection() == true)
		_barLongRange->setColor(Palette::blockOffset(4)+4);

	var = _base->getLongRangeTotal();
	woststr12 << var;
	_numLongRange->setText(woststr12.str());
	_barLongRange->setMax(var);
	_barLongRange->setValue(var);
}

/**
 * Changes the base name.
 * @param action - pointer to an Action
 */
void BaseInfoState::edtBaseChange(Action*)
{
	_base->setName(_edtBase->getText());
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void BaseInfoState::handleKeyPress(Action* action)
{
	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		const SDLKey baseKeys[] =
		{
			Options::keyBaseSelect1,
			Options::keyBaseSelect2,
			Options::keyBaseSelect3,
			Options::keyBaseSelect4,
			Options::keyBaseSelect5,
			Options::keyBaseSelect6,
			Options::keyBaseSelect7,
			Options::keyBaseSelect8
		};

		const SDLKey key = action->getDetails()->key.keysym.sym;

		for (size_t
				i = 0;
				i != _baseList->size();
				++i)
		{
			if (key == baseKeys[i])
			{
				_txtHoverBase->setText(L"");

				_mini->setSelectedBase(i);
				_base = _baseList->at(i);
				_state->setBase(_base);

				init();
				break;
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the Transfers window.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnTransfersClick(Action*)
{
	_game->pushState(new TransfersState(_base));
}

/**
 * Goes to the Stores screen.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnStoresClick(Action*)
{
	_game->pushState(new StoresState(_base));
}

/**
 * Goes to the Monthly Costs screen.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnMonthlyCostsClick(Action*)
{
	_game->pushState(new MonthlyCostsState(_base));
}

/**
 * Selects a new Base to display.
 * @param action - pointer to an Action
 */
void BaseInfoState::miniClick(Action*)
{
	const size_t baseId = _mini->getHoveredBase();

	if (baseId < _baseList->size()
		&& _base != _baseList->at(baseId))
	{
		_txtHoverBase->setText(L"");

		_mini->setSelectedBase(baseId);
		_base = _baseList->at(baseId);
		_state->setBase(_base);

		init();
	}
}

/**
 * Displays the name of the Base the mouse is over.
 * @param action - pointer to an Action
 */
void BaseInfoState::viewMouseOver(Action*)
{
	const size_t baseId = _mini->getHoveredBase();

	if (baseId < _baseList->size()
		&& _base != _baseList->at(baseId))
	{
		_txtHoverBase->setText(_baseList->at(baseId)->getName(_game->getLanguage()).c_str());
	}
	else
		_txtHoverBase->setText(L"");
}

/**
 * Clears the hovered Base name.
 * @param action - pointer to an Action
 */
void BaseInfoState::viewMouseOut(Action*)
{
	_txtHoverBase->setText(L"");
}

}
