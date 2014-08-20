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

#include "BasescapeState.h"

#include "BaseInfoState.h"
#include "BaseView.h"
#include "BuildFacilitiesState.h"
#include "CraftInfoState.h"
#include "CraftsState.h"
#include "DismantleFacilityState.h"
#include "ManageAlienContainmentState.h"
#include "ManufactureState.h"
#include "MiniBaseView.h"
#include "MonthlyCostsState.h"
#include "PurchaseState.h"
#include "ResearchState.h"
#include "SellState.h"
#include "StoresState.h"
#include "SoldiersState.h"
#include "TransferBaseState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Sound.h" // kL

#include "../Geoscape/AllocatePsiTrainingState.h"
#include "../Geoscape/BuildNewBaseState.h"
#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h" // kL, kL_reCenter

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

Sound* BasescapeState::soundPop = 0; // kL


/**
 * Initializes all the elements in the Basescape screen.
 * @param base Pointer to the base to get info from.
 * @param globe Pointer to the Geoscape globe.
 */
BasescapeState::BasescapeState(
		Base* base,
		Globe* globe)
	:
		_base(base),
		_globe(globe)
{
	_txtFacility	= new Text(192, 9, 0, 0);
	_view			= new BaseView(192, 192, 0, 8);

	_edtBase		= new TextEdit(this, 127, 17, 193, 0);
	_txtLocation	= new Text(126, 9, 194, 16);
	_txtFunds		= new Text(126, 9, 194, 24);

	_mini			= new MiniBaseView(128, 20, 192, 35);

	_btnBaseInfo	= new TextButton(128, 12, 192, 56);
	_btnSoldiers	= new TextButton(128, 12, 192, 68);
	_btnCrafts		= new TextButton(128, 12, 192, 80);
	_btnAliens		= new TextButton(128, 12, 192, 92);
	_btnResearch	= new TextButton(128, 12, 192, 104);
	_btnManufacture	= new TextButton(128, 12, 192, 116);
	_btnPurchase	= new TextButton(128, 12, 192, 128);
	_btnSell		= new TextButton(128, 12, 192, 140);
	_btnTransfer	= new TextButton(128, 12, 192, 152);
	_btnFacilities	= new TextButton(128, 12, 192, 164);
	_btnNewBase		= new TextButton(128, 12, 192, 176);
	_btnGeoscape	= new TextButton(128, 12, 192, 188);

	setPalette("PAL_BASESCAPE");

	add(_view);
	add(_mini);
	add(_txtFacility);
	add(_edtBase);
	add(_txtLocation);
	add(_txtFunds);
	add(_btnAliens);
	add(_btnNewBase);
	add(_btnBaseInfo);
	add(_btnSoldiers);
	add(_btnCrafts);
	add(_btnFacilities);
	add(_btnResearch);
	add(_btnManufacture);
	add(_btnTransfer);
	add(_btnPurchase);
	add(_btnSell);
	add(_btnGeoscape);

	centerAllSurfaces();

	_view->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_view->onMouseClick(
					(ActionHandler)& BasescapeState::viewLeftClick,
					SDL_BUTTON_LEFT);
	_view->onMouseClick(
					(ActionHandler)& BasescapeState::viewRightClick,
					SDL_BUTTON_RIGHT);
	_view->onMouseOver((ActionHandler)& BasescapeState::viewMouseOver);
	_view->onMouseOut((ActionHandler)& BasescapeState::viewMouseOut);
//	_view->onKeyboardPress((ActionHandler)& BasescapeState::handleKeyPress);

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_game->getSavedGame()->getBases());
	_mini->onMouseClick((ActionHandler)& BasescapeState::miniClick);
	_mini->onKeyboardPress((ActionHandler)& BasescapeState::handleKeyPress);

	_txtFacility->setColor(Palette::blockOffset(13)+10);

	_edtBase->setColor(Palette::blockOffset(15)+1);
	_edtBase->setBig();
	_edtBase->onChange((ActionHandler)& BasescapeState::edtBaseChange);

	_txtLocation->setColor(Palette::blockOffset(15)+6);

	_txtFunds->setColor(Palette::blockOffset(13)+10);

	_btnAliens->setColor(Palette::blockOffset(13)+5);
	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick((ActionHandler)& BasescapeState::btnAliens);

	_btnNewBase->setColor(Palette::blockOffset(13)+5);
	_btnNewBase->setText(tr("STR_BUILD_NEW_BASE_UC"));
	_btnNewBase->onMouseClick((ActionHandler)& BasescapeState::btnNewBaseClick);

	_btnBaseInfo->setColor(Palette::blockOffset(13)+5);
	_btnBaseInfo->setText(tr("STR_BASE_INFORMATION"));
	_btnBaseInfo->onMouseClick((ActionHandler)& BasescapeState::btnBaseInfoClick);

	_btnSoldiers->setColor(Palette::blockOffset(13)+5);
	_btnSoldiers->setText(tr("STR_SOLDIERS_UC"));
	_btnSoldiers->onMouseClick((ActionHandler)& BasescapeState::btnSoldiersClick);

	_btnCrafts->setColor(Palette::blockOffset(13)+5);
	_btnCrafts->setText(tr("STR_EQUIP_CRAFT"));
	_btnCrafts->onMouseClick((ActionHandler)& BasescapeState::btnCraftsClick);

	_btnFacilities->setColor(Palette::blockOffset(13)+5);
	_btnFacilities->setText(tr("STR_BUILD_FACILITIES"));
	_btnFacilities->onMouseClick((ActionHandler)& BasescapeState::btnFacilitiesClick);

	_btnResearch->setColor(Palette::blockOffset(13)+5);
	_btnResearch->setText(tr("STR_RESEARCH"));
	_btnResearch->onMouseClick((ActionHandler)& BasescapeState::btnResearchClick);

	_btnManufacture->setColor(Palette::blockOffset(13)+5);
	_btnManufacture->setText(tr("STR_MANUFACTURE"));
	_btnManufacture->onMouseClick((ActionHandler)& BasescapeState::btnManufactureClick);

	_btnTransfer->setColor(Palette::blockOffset(13)+5);
	_btnTransfer->setText(tr("STR_TRANSFER_UC"));
	_btnTransfer->onMouseClick((ActionHandler)& BasescapeState::btnTransferClick);

	_btnPurchase->setColor(Palette::blockOffset(13)+5);
	_btnPurchase->setText(tr("STR_PURCHASE_RECRUIT"));
	_btnPurchase->onMouseClick((ActionHandler)& BasescapeState::btnPurchaseClick);

	_btnSell->setColor(Palette::blockOffset(13)+5);
	_btnSell->setText(tr("STR_SELL_SACK_UC"));
	_btnSell->onMouseClick((ActionHandler)& BasescapeState::btnSellClick);

	_btnGeoscape->setColor(Palette::blockOffset(13)+5);
	_btnGeoscape->setText(tr("STR_GEOSCAPE_UC"));
	_btnGeoscape->onMouseClick((ActionHandler)& BasescapeState::btnGeoscapeClick);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& BasescapeState::btnGeoscapeClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
BasescapeState::~BasescapeState()
{
	bool exists = false; // Clean up any temporary bases
	for (std::vector<Base*>::iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end()
				&& !exists;
			++i)
	{
		if (*i == _base)
		{
			exists = true;

			break;
		}
	}

	if (!exists)
		delete _base;
}

/**
 * The player can change the selected base or change info on other screens.
 */
void BasescapeState::init()
{
	State::init();

	setBase(_base);

	_view->setBase(_base);
	_mini->draw();
	_edtBase->setText(_base->getName());

	for (std::vector<Region*>::iterator // Get region
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										_base->getLongitude(),
										_base->getLatitude()))
		{
			_txtLocation->setText(tr((*i)->getRules()->getType()));

			break;
		}
	}

	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_btnNewBase->setVisible(_game->getSavedGame()->getBases()->size() < MiniBaseView::MAX_BASES);

	// kL_begin:
	bool hasFunds		= (_game->getSavedGame()->getFunds() > 0);
	bool hasQuarters	= false;
	bool hasHangar		= false;
	bool hasAlienCont	= false;
	bool hasLabs		= false;
	bool hasProd		= false;
	bool hasStores		= false;

	for (std::vector<BaseFacility*>::iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getRules()->getPersonnel() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasQuarters = true;
		}

		if ((*i)->getRules()->getCrafts() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasHangar = true;
		}

		if ((*i)->getRules()->getAliens() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasAlienCont = true;
		}

		if ((*i)->getRules()->getLaboratories() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasLabs = true;
		}

		if ((*i)->getRules()->getWorkshops() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasProd = true;
		}

		if ((*i)->getRules()->getStorage() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasStores = true;
		}
	}

	_btnSoldiers->setVisible(hasQuarters
								&& _base->getSoldiers()->empty() == false);
	_btnCrafts->setVisible(hasHangar);
	_btnAliens->setVisible(hasAlienCont);
	_btnResearch->setVisible(hasLabs);
	_btnManufacture->setVisible(hasProd);
	_btnSell->setVisible(hasStores || hasQuarters || hasHangar || hasAlienCont);
	_btnPurchase->setVisible(hasFunds && (hasStores || hasQuarters || hasHangar));
	_btnTransfer->setVisible(hasFunds && (hasStores || hasQuarters || hasHangar));
	_btnFacilities->setVisible(hasFunds); // kL_end.
}

/**
 * Changes the base currently displayed on screen.
 * @param base - pointer to new base to display
 */
void BasescapeState::setBase(Base* base)
{
	if (!_game->getSavedGame()->getBases()->empty())
	{
		bool exists = false; // Check if base still exists
		for (size_t
				i = 0;
				i < _game->getSavedGame()->getBases()->size();
				++i)
		{
			if (_game->getSavedGame()->getBases()->at(i) == base)
			{
				_base = base;
				_mini->setSelectedBase(i);
//kL			_game->getSavedGame()->setSelectedBase(i);
				exists = true;

				break;
			}
		}

		if (!exists) // If base was removed, select first one
		{
			_base = _game->getSavedGame()->getBases()->front();
			_mini->setSelectedBase(0);
//kL		_game->getSavedGame()->setSelectedBase(0);
		}
	}
	else // Use a blank base for special case when player has no bases
	{
		_base = new Base(_game->getRuleset());
		_mini->setSelectedBase(0);
//kL	_game->getSavedGame()->setSelectedBase(0);
	}
}

/**
 * kL. Goes to the Manage Alien Containment screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnAliens(Action*) // kL
{
	_game->pushState(new ManageAlienContainmentState(
													_base,
													OPT_GEOSCAPE));
}

/**
 * Goes to the Build New Base screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnNewBaseClick(Action*)
{
	Base* base = new Base(_game->getRuleset());

	_game->popState();
	_game->pushState(new BuildNewBaseState(
										base,
										_globe,
										false));
}

/**
 * Goes to the Base Info screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnBaseInfoClick(Action*)
{
	_game->pushState(new BaseInfoState(
									_base,
									this));
}

/**
 * Goes to the Soldiers screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnSoldiersClick(Action*)
{
	_game->pushState(new SoldiersState(_base));
}

/**
 * Goes to the Crafts screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnCraftsClick(Action*)
{
	_game->pushState(new CraftsState(_base));
}

/**
 * Opens the Build Facilities window.
 * @param action - pointer to an action
 */
void BasescapeState::btnFacilitiesClick(Action*)
{
	_game->pushState(new BuildFacilitiesState(
											_base,
											this));
}

/**
 * Goes to the Research screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnResearchClick(Action*)
{
	_game->pushState(new ResearchState(_base));
}

/**
 * Goes to the Manufacture screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnManufactureClick(Action*)
{
	_game->pushState(new ManufactureState(_base));
}

/**
 * Goes to the Purchase screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnPurchaseClick(Action*)
{
	_game->pushState(new PurchaseState(_base));
}

/**
 * Goes to the Sell screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnSellClick(Action*)
{
	_game->pushState(new SellState(_base));
}

/**
 * Goes to the Select Destination Base window.
 * @param action - pointer to an action
 */
void BasescapeState::btnTransferClick(Action*)
{
	_game->pushState(new TransferBaseState(_base));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an action
 */
void BasescapeState::btnGeoscapeClick(Action*)
{
	_game->popState();
}

/**
 * Processes left clicking on facilities.
 * @param action - pointer to an action
 */
void BasescapeState::viewLeftClick(Action*)
{
	bool bPop = false; // play "wha-wha" sound

	BaseFacility* fac = _view->getSelectedFacility();
	if (fac == NULL)
	{
		bPop = true;
//		_game->pushState(new BaseInfoState(
//										_base,
//										this));
		_game->pushState(new MonthlyCostsState(
											_base));
	}
	else if (fac->getBuildTime() > 0)
		return;
	else if (fac->getRules()->getCrafts() > 0)
	{
/*		if (_base->getCrafts()->size() == 0) // no Craft at base
		{
			bPop = true;
			_game->pushState(new CraftsState(_base));
		}
		else */
		if (_base->getCrafts()->size() > 0) // Craft at base
		{
			for (size_t // kL_begin:
					i = 0;
					i < _base->getCrafts()->size();
					++i)
			{
				Craft* craft = _base->getCrafts()->at(i);

				if (fac->getCraft() == NULL) // Empty hangar, craft currently out
				{
					bPop = true;

/*					if (craft->getStatus() == "STR_OUT")
					{
						_game->getSavedGame()->setGlobeLongitude(craft->getLongitude());
						_game->getSavedGame()->setGlobeLatitude(craft->getLatitude());

						kL_reCenter = true;

						_game->popState(); // pop to Geoscape.

						break; // might not be needed here
					}
					else // Empty hangar, no craft for it; however, base has at least one craft (docked or out)
					{ */
					_game->pushState(new CraftsState(_base));

					break;
//					}
				}
				else if (fac->getCraft() == craft) // craft is docked here
				{
//					bPop = false; // plays window-'swish' instead.
					_game->pushState(new CraftInfoState(
													_base,
													i));

					break;
				}
			}
		} // kL_end.
	}
	else if (fac->getRules()->getStorage() > 0)
	{
//kL		_game->pushState(new SellState(_base));
		bPop = true;
		_game->pushState(new StoresState(_base));
	}
	else if (fac->getRules()->getPersonnel() > 0
		&& _base->getSoldiers()->empty() == false)
	{
		bPop = true;
		_game->pushState(new SoldiersState(_base));
	}
	else if (fac->getRules()->getPsiLaboratories() > 0
			&& Options::anytimePsiTraining
			&& _base->getAvailablePsiLabs() > 0)
	{
		bPop = true;
		_game->pushState(new AllocatePsiTrainingState(_base));
	}
	else if (fac->getRules()->getLaboratories() > 0)
	{
		bPop = true;
		_game->pushState(new ResearchState(_base));
	}
	else if (fac->getRules()->getWorkshops() > 0)
	{
		bPop = true;
		_game->pushState(new ManufactureState(_base));
	}
	else if (fac->getRules()->getAliens() > 0)
	{
		bPop = true;
		_game->pushState(new ManageAlienContainmentState(
														_base,
														OPT_GEOSCAPE));
	}
/*	else if (fac->getRules()->isLift()) // my Lift has a radar range ... (see next)
	{
		bPop = true;
		_game->pushState(new MonthlyCostsState(_base));
	} */
	else if (fac->getRules()->getRadarRange() > 0)
	{
		bPop = true;

		_game->getSavedGame()->setGlobeLongitude(_base->getLongitude());
		_game->getSavedGame()->setGlobeLatitude(_base->getLatitude());

		kL_reCenter = true;

		_game->popState();
	}


	if (bPop)
		soundPop->play(Mix_GroupAvailable(0)); // kL: UI Fx channels #0 & #1 & #2, see Game.cpp
}

/**
 * Processes right clicking on facilities.
 * @param action - pointer to an action
 */
void BasescapeState::viewRightClick(Action*)
{
	BaseFacility* fac = _view->getSelectedFacility();
	if (fac != 0)
	{
		if (fac->inUse())
		{
			_game->pushState(new ErrorMessageState(
												tr("STR_FACILITY_IN_USE"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												6));
		}
		else if (!_base->getDisconnectedFacilities(fac).empty()) // would base become disconnected...
		{
			_game->pushState(new ErrorMessageState(
												tr("STR_CANNOT_DISMANTLE_FACILITY"),
												_palette,
												Palette::blockOffset(15)+1,
												"BACK13.SCR",
												6));
		}
		else
		{
			_game->pushState(new DismantleFacilityState(
													_base,
													_view,
													fac));
		}
	}
}

/**
 * Displays the name of the facility the mouse is over.
 * @param action - pointer to an action
 */
void BasescapeState::viewMouseOver(Action*)
{
	std::wostringstream ss;

	BaseFacility* fac = _view->getSelectedFacility();
	if (fac != NULL)
	{
		if (fac->getRules()->getCrafts() == 0
			|| fac->getBuildTime() > 0)
		{
			ss << tr(fac->getRules()->getType());
		}
		else
		{
			ss << tr(fac->getRules()->getType());

			if (fac->getCraft() != NULL)
				ss << L" " << tr("STR_CRAFT_")
								.arg(fac->getCraft()->getName(_game->getLanguage()));
		}
	}

	_txtFacility->setText(ss.str());
}

/**
 * Clears the facility name.
 * @param action - pointer to an action
 */
void BasescapeState::viewMouseOut(Action*)
{
	_txtFacility->setText(L"");
}

/**
 * Selects a new base to display.
 * @param action - pointer to an action
 */
void BasescapeState::miniClick(Action*)
{
	size_t base = _mini->getHoveredBase();
	if (base < _game->getSavedGame()->getBases()->size())
	{
		_base = _game->getSavedGame()->getBases()->at(base);

		init();
	}
}

/**
 * Selects a new base to display.
 * @param action - pointer to an action
 */
void BasescapeState::handleKeyPress(Action* action)
{
	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		SDLKey baseKeys[] =
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

		int key = action->getDetails()->key.keysym.sym;

		for (size_t
				i = 0;
				i < _game->getSavedGame()->getBases()->size();
				++i)
		{
			if (key == baseKeys[i])
			{
				_base = _game->getSavedGame()->getBases()->at(i);
				init();

				break;
			}
		}
	}
}

/**
 * Changes the Base name.
 * @param action - pointer to an action
 */
void BasescapeState::edtBaseChange(Action* action)
{
	_base->setName(_edtBase->getText());
}

}
