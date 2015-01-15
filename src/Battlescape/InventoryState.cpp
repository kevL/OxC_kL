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

#include "InventoryState.h"

//#include <sstream>

#include "BattlescapeState.h"
#include "Camera.h"
#include "Inventory.h"
#include "Map.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "UnitInfoState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

//static const int _templateBtnX		= 288;
//static const int _createTemplateBtnY	= 67;	//90
//static const int _applyTemplateBtnY	= 90;	//113
//static const int _clearInventoryBtnY	= 113;


/**
 * Initializes all the elements in the Inventory screen.
 * @param tuMode - true if in battle when inventory usage costs time units
 * @param parent - pointer to parent BattlescapeState
 */
InventoryState::InventoryState(
		bool tuMode,
		BattlescapeState* parent)
	:
		_tuMode(tuMode),
		_parent(parent)
{
	_battleGame = _game->getSavedGame()->getSavedBattle();
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}
	else if (!_battleGame->getTileEngine())
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_bg			= new Surface(320, 200, 0, 0);
	_soldier	= new Surface(320, 200, 0, 0);

	_txtName	= new Text(200, 17, 36, 6);
	_gender		= new Surface(7, 7, 28, 1);

	_txtWeight	= new Text(70, 9, 237, 24); // 237 -> was, 245
	_txtTUs		= new Text(40, 9, 237, 24);
	_txtFAcc	= new Text(40, 9, 237, 32);
	_txtReact	= new Text(40, 9, 237, 40);
	_txtThrow	= new Text(40, 9, 237, 48);
	_txtMelee	= new Text(40, 9, 237, 56);
	_txtPStr	= new Text(40, 9, 237, 64);
	_txtPSkill	= new Text(40, 9, 237, 72);

	_txtUseTU	= new Text(45, 9, 245, 123);
	_txtThrowTU	= new Text(40, 9, 245, 132);
	_txtPsiTU	= new Text(40, 9, 245, 141);

	_numOrder	= new NumberText(7, 5, 228, 4);
	_tuCost		= new NumberText(7, 5, 310, 60);

	_txtItem	= new Text(160, 9, 128, 140);

	_btnOk		= new BattlescapeButton(35, 23, 237, 0);
	_btnPrev	= new BattlescapeButton(23, 23, 273, 0);
	_btnNext	= new BattlescapeButton(23, 23, 297, 0);

	_btnRank	= new BattlescapeButton(26, 23, 0, 0);
	_btnUnload	= new BattlescapeButton(32, 25, 288, 32);
	_btnGround	= new BattlescapeButton(32, 15, 288, 137);

/*	_btnCreateTemplate	= new BattlescapeButton(
											32,
											22,
											_templateBtnX,
											_createTemplateBtnY);
	_btnApplyTemplate	= new BattlescapeButton(
											32,
											22,
											_templateBtnX,
											_applyTemplateBtnY);
	_btnClearInventory	= new BattlescapeButton(
											32,
											22,
											_templateBtnX,
											_clearInventoryBtnY); */

//	_txtAmmo	= new Text(40, 24, 272, 64);
	_txtAmmo	= new Text(40, 24, 288, 64);
	_selAmmo	= new Surface(
						RuleInventory::HAND_W * RuleInventory::SLOT_W,
						RuleInventory::HAND_H * RuleInventory::SLOT_H,
						288, // 272,
						88);

	_inv		= new Inventory(
							_game,
							320,
							200,
							0,
							0,
							_parent == NULL);

	setPalette("PAL_BATTLESCAPE");

	add(_bg);
	_game->getResourcePack()->getSurface("TAC01.SCR")->blit(_bg);

	add(_gender);
	add(_soldier);
	add(_txtName, "textName", "inventory", _bg);

	add(_txtWeight, "textWeight", "inventory", _bg);
	add(_txtTUs, "textTUs", "inventory", _bg);
	add(_txtFAcc, "textFiring", "inventory", _bg);
	add(_txtReact, "textReaction", "inventory", _bg);
	add(_txtThrow, "textThrowing", "inventory", _bg);
	add(_txtMelee, "textMelee", "inventory", _bg);
	add(_txtPStr, "textPsiStrength", "inventory", _bg);
	add(_txtPSkill, "textPsiSkill", "inventory", _bg);

	add(_numOrder);
	add(_tuCost);
	add(_txtItem, "textItem", "inventory", _bg);
	add(_txtAmmo, "textAmmo", "inventory", _bg);
	add(_btnOk, "buttonOK", "inventory", _bg);
	add(_btnPrev, "buttonPrev", "inventory", _bg);
	add(_btnNext, "buttonNext", "inventory", _bg);
	add(_btnUnload, "buttonUnload", "inventory", _bg);
	add(_btnGround, "buttonGround", "inventory", _bg);
	add(_btnRank, "rank", "inventory", _bg);
//	add(_btnCreateTemplate, "buttonCreate", "inventory", _bg);
//	add(_btnApplyTemplate, "buttonApply", "inventory", _bg);
//	add(_btnClearInventory);
	add(_selAmmo);
	add(_inv);
	add(_txtUseTU, "textTUs", "inventory", _bg);
	add(_txtThrowTU, "textTUs", "inventory", _bg);
	add(_txtPsiTU, "textTUs", "inventory", _bg);

	// move the TU display down to make room for the weight display
	if (Options::showMoreStatsInInventoryView == true)
		_txtTUs->setY(_txtTUs->getY() + 8);

	centerAllSurfaces();


//	_txtName->setColor(Palette::blockOffset(4));
	_txtName->setBig();
	_txtName->setHighContrast();

//	_txtWeight->setColor(Palette::blockOffset(4));
//	_txtWeight->setSecondaryColor(Palette::blockOffset(1));
	_txtWeight->setHighContrast();

//	_txtTUs->setColor(Palette::blockOffset(4));
//	_txtTUs->setSecondaryColor(Palette::blockOffset(1));
	_txtTUs->setHighContrast();

//	_txtFAcc->setColor(Palette::blockOffset(4));
//	_txtFAcc->setSecondaryColor(Palette::blockOffset(1));
	_txtFAcc->setHighContrast();

//	_txtReact->setColor(Palette::blockOffset(4));
//	_txtReact->setSecondaryColor(Palette::blockOffset(1));
	_txtReact->setHighContrast();

//	_txtThrow->setColor(Palette::blockOffset(4));
//	_txtThrow->setSecondaryColor(Palette::blockOffset(1));
	_txtThrow->setHighContrast();

//	_txtMelee->setColor(Palette::blockOffset(4));
//	_txtMelee->setSecondaryColor(Palette::blockOffset(1));
	_txtMelee->setHighContrast();

//	_txtPStr->setColor(Palette::blockOffset(4));
//	_txtPStr->setSecondaryColor(Palette::blockOffset(1));
	_txtPStr->setHighContrast();

//	_txtPSkill->setColor(Palette::blockOffset(4));
//	_txtPSkill->setSecondaryColor(Palette::blockOffset(1));
	_txtPSkill->setHighContrast();

//	_txtUseTU->setColor(Palette::blockOffset(4));
//	_txtUseTU->setSecondaryColor(Palette::blockOffset(1));
	_txtUseTU->setHighContrast();

//	_txtThrowTU->setColor(Palette::blockOffset(4));
//	_txtThrowTU->setSecondaryColor(Palette::blockOffset(1));
	_txtThrowTU->setHighContrast();

//	_txtPsiTU->setColor(Palette::blockOffset(4));
//	_txtPsiTU->setSecondaryColor(Palette::blockOffset(1));
	_txtPsiTU->setHighContrast();

	_numOrder->setColor(1);
	_numOrder->setValue(0);

	_tuCost->setColor(1);
	_tuCost->setValue(0);
	_tuCost->setVisible(false);

//	_txtItem->setColor(Palette::blockOffset(3));
	_txtItem->setHighContrast();

//	_txtAmmo->setColor(Palette::blockOffset(4));
//	_txtAmmo->setSecondaryColor(Palette::blockOffset(1));
	_txtAmmo->setAlign(ALIGN_LEFT);
	_txtAmmo->setHighContrast();

	_btnOk->onMouseClick((ActionHandler)& InventoryState::btnOkClick);
	_btnOk->onKeyboardPress(
						(ActionHandler)& InventoryState::btnOkClick,
						Options::keyCancel);
	_btnOk->onKeyboardPress(
						(ActionHandler)& InventoryState::btnOkClick,
						Options::keyBattleInventory);
//	_btnOk->setTooltip("STR_OK");
//	_btnOk->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnOk->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnPrev->onMouseClick((ActionHandler)& InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress(
						(ActionHandler)& InventoryState::btnPrevClick,
						Options::keyBattlePrevUnit);
//	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
//	_btnPrev->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnPrev->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)& InventoryState::btnNextClick);
	_btnNext->onKeyboardPress(
						(ActionHandler)& InventoryState::btnNextClick,
						Options::keyBattleNextUnit);
//	_btnNext->setTooltip("STR_NEXT_UNIT");
//	_btnNext->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnNext->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick(
					(ActionHandler)& InventoryState::btnUnloadClick,
					SDL_BUTTON_LEFT);
	_btnUnload->onMouseClick(
					(ActionHandler)& InventoryState::btnClearInventoryClick,
					SDL_BUTTON_RIGHT);
//	_btnUnload->onMouseClick((ActionHandler)& InventoryState::btnUnloadClick);
//	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
//	_btnUnload->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnUnload->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnGround->onMouseClick((ActionHandler)& InventoryState::btnGroundClick);
//	_btnGround->setTooltip("STR_SCROLL_RIGHT");
//	_btnGround->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnGround->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnRank->onMouseClick((ActionHandler)& InventoryState::btnRankClick);
//	_btnRank->setTooltip("STR_UNIT_STATS");
//	_btnRank->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnRank->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);


/*	_btnCreateTemplate->onMouseClick((ActionHandler)& InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress(
						(ActionHandler)& InventoryState::btnCreateTemplateClick,
						Options::keyInvCreateTemplate); */
//	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
//	_btnCreateTemplate->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnCreateTemplate->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

/*	_btnApplyTemplate->onMouseClick((ActionHandler)& InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress(
						(ActionHandler)& InventoryState::btnApplyTemplateClick,
						Options::keyInvApplyTemplate); */
//	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
//	_btnApplyTemplate->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnApplyTemplate->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

/*	_btnClearInventory->onMouseClick((ActionHandler)& InventoryState::btnClearInventoryClick);
	_btnClearInventory->onKeyboardPress(
					(ActionHandler)& InventoryState::btnClearInventoryClick,
					Options::keyInvClear); */
//	_btnClearInventory->setTooltip("STR_CLEAR_INVENTORY");
//	_btnClearInventory->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnClearInventory->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);


	// only use copy/paste layout-template buttons in setup (i.e. non-tu) mode
/*	if (_tuMode)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
		_btnClearInventory->setVisible(false);
	}
	else
		_updateTemplateButtons(true); */

	_inv->draw();
	_inv->setTuMode(_tuMode);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit());
	_inv->onMouseClick((ActionHandler)& InventoryState::invClick, 0);
	_inv->onMouseOver((ActionHandler)& InventoryState::invMouseOver);
	_inv->onMouseOut((ActionHandler)& InventoryState::invMouseOut);

	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtTUs->setVisible(_tuMode);
	_txtUseTU->setVisible(_tuMode);

	const bool vis = _tuMode == false
				  && Options::showMoreStatsInInventoryView == true;
	_txtFAcc->setVisible(vis);
	_txtReact->setVisible(vis);
	_txtThrow->setVisible(vis);
	_txtMelee->setVisible(vis);
	_txtPStr->setVisible(vis);
	_txtPSkill->setVisible(vis);
}

/**
 * Helper for the dTor.
 */
/* static void _clearInventoryTemplate(std::vector<EquipmentLayoutItem*>& inventoryTemplate)
{
	for (std::vector<EquipmentLayoutItem*>::iterator
			i = inventoryTemplate.begin();
			i != inventoryTemplate.end();
			)
	{
		delete *i;
	}
} */

/**
 * dTor.
 */
InventoryState::~InventoryState()
{
//	_clearInventoryTemplate(_curInventoryTemplate);

	// kL_begin:
	if (_parent != NULL)
	{
		TileEngine* const tileEngine = _battleGame->getTileEngine();
		Tile* const tile = _battleGame->getSelectedUnit()->getTile();

		tileEngine->applyGravity(tile);
		tileEngine->calculateTerrainLighting();
		tileEngine->recalculateFOV();
	} // kL_end.
}
//	if (_battleGame->getTileEngine())
//	{
//		if (Options::maximizeInfoScreens)
//		{
//			Screen::updateScale(
//							Options::battlescapeScale,
//							Options::battlescapeScale,
//							Options::baseXBattlescape,
//							Options::baseYBattlescape,
//							true);
//			_game->getScreen()->resetDisplay(false);
//		}

//kL	Tile* tile = _battleGame->getSelectedUnit()->getTile();
//kL	_battleGame->getTileEngine()->applyGravity(tile);
//kL	_battleGame->getTileEngine()->calculateTerrainLighting(); // dropping/picking up flares
//kL	_battleGame->getTileEngine()->recalculateFOV();

//	}
//	else
//	{
//		Screen::updateScale(
//						Options::geoscapeScale,
//						Options::geoscapeScale,
//						Options::baseXGeoscape,
//						Options::baseYGeoscape,
//						true);
//		_game->getScreen()->resetDisplay(false);
//	}

/**
 * Updates all soldier stats when the soldier changes.
 */
void InventoryState::init()
{
	State::init();

	BattleUnit* unit = _battleGame->getSelectedUnit();
	if (unit == NULL) // no selected unit, close inventory
	{
		btnOkClick(NULL);
		return;
	}
	else if (unit->hasInventory() == false) // skip to the first unit with inventory
	{
		if (_parent != NULL)
			_parent->selectNextFactionUnit(
										false,
										false,
										true);
		else
			_battleGame->selectNextFactionUnit(
											false,
											false,
											true);

		if (_battleGame->getSelectedUnit() == NULL
			 || _battleGame->getSelectedUnit()->hasInventory() == false)
		{
			btnOkClick(NULL);
			return; // starting a mission with just vehicles. kL_note: DISALLOWED!!!
		}
		else
			unit = _battleGame->getSelectedUnit();
	}

//	if (_parent)
//		_parent->getMap()->getCamera()->centerOnPosition(unit->getPosition(), false);

	unit->setCache(NULL);

	_soldier->clear();
	_btnRank->clear();
	_gender->clear();

	_txtName->setText(unit->getName(_game->getLanguage()));

	_inv->setSelectedUnit(unit);

	const Soldier* const soldier = unit->getGeoscapeSoldier();
	if (soldier != NULL)
	{
		Surface* gender = NULL;
		if (soldier->getGender() == GENDER_MALE)
			gender = _game->getResourcePack()->getSurface("GENDER_M");
		else
			gender = _game->getResourcePack()->getSurface("GENDER_F");

		if (gender != NULL)
			gender->blit(_gender);

		SurfaceSet* const texture = _game->getResourcePack()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(20 + soldier->getRank())->setX(0);
		texture->getFrame(20 + soldier->getRank())->setY(0);
		texture->getFrame(20 + soldier->getRank())->blit(_btnRank);

		std::string look = soldier->getArmor()->getSpriteInventory();
		if (soldier->getGender() == GENDER_MALE)
			look += "M";
		else
			look += "F";

		if		(soldier->getLook() == LOOK_BLONDE)		look += "0";
		else if (soldier->getLook() == LOOK_BROWNHAIR)	look += "1";
		else if (soldier->getLook() == LOOK_ORIENTAL)	look += "2";
		else if (soldier->getLook() == LOOK_AFRICAN)	look += "3";

		look += ".SPK";

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look)) == false
			&& _game->getResourcePack()->getSurface(look) == NULL)
		{
			look = soldier->getArmor()->getSpriteInventory() + ".SPK";
		}

		_game->getResourcePack()->getSurface(look)->blit(_soldier);
	}
	else
	{
		SurfaceSet* const texture = _game->getResourcePack()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(26)->setX(0);
		texture->getFrame(26)->setY(0);
		texture->getFrame(26)->blit(_btnRank);

		Surface* const armorSurface = _game->getResourcePack()->getSurface(unit->getArmor()->getSpriteInventory());
		if (armorSurface != NULL)
			armorSurface->blit(_soldier);
	}

	updateStats();

	_refreshMouse();
}

/**
 * Updates the soldier stats & TU usages.
 */
void InventoryState::updateStats()
{
	BattleUnit* const unit = _battleGame->getSelectedUnit();

	_numOrder->setValue(unit->getBattleOrder());
	_numOrder->setVisible(unit->getOriginalFaction() == FACTION_PLAYER);

	if (_tuMode == true)
		_txtTUs->setText(tr("STR_TIME_UNITS_SHORT").arg(unit->getTimeUnits()));

	if (Options::showMoreStatsInInventoryView == true)
	{
		const int
			weight = unit->getCarriedWeight(_inv->getSelectedItem()),
			strength = static_cast<int>(Round(
					   static_cast<double>(unit->getBaseStats()->strength) * (unit->getAccuracyModifier() / 2. + 0.5)));

		_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(strength));
		if (weight > strength)
			_txtWeight->setSecondaryColor(_game->getRuleset()->getInterface("inventory")->getElement("weight")->color2); //Palette::blockOffset(2)); // +5 gives green border (high contrast)
		else
			_txtWeight->setSecondaryColor(_game->getRuleset()->getInterface("inventory")->getElement("weight")->color); //Palette::blockOffset(3));


		const int psiSkill = unit->getBaseStats()->psiSkill;

		if (_tuMode == true)
		{
			_txtThrowTU->setText(tr("STR_THROW_").arg(unit->getActionTUs(BA_THROW)));

			if (unit->getOriginalFaction() == FACTION_HOSTILE
				&& psiSkill > 0)
			{
				_txtPsiTU->setVisible();
				_txtPsiTU->setText(tr("STR_PSI_").arg(unit->getActionTUs(
																	BA_PANIC,
																	_parent->getBattleGame()->getAlienPsi())));
			}
			else
				_txtPsiTU->setVisible(false);
		}
		else
		{
			_txtFAcc->setText(tr("STR_ACCURACY_SHORT").arg(unit->getBaseStats()->firing));
			_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(unit->getBaseStats()->reactions));
			_txtThrow->setText(tr("STR_THROWACC_SHORT").arg(unit->getBaseStats()->throwing));
			_txtMelee->setText(tr("STR_MELEEACC_SHORT").arg(unit->getBaseStats()->melee));

			int minPsi = 0;
			if (unit->getGeoscapeSoldier() != NULL)
				minPsi = unit->getGeoscapeSoldier()->getRules()->getMinStats().psiSkill - 1;

			if (psiSkill > minPsi)
				_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(psiSkill));
			else
				_txtPSkill->setText(L"");

			if (psiSkill > minPsi
				|| (Options::psiStrengthEval == true
					&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
			{
				_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(unit->getBaseStats()->psiStrength));
			}
			else
				_txtPStr->setText(L"");
		}
	}
}

/**
 * Saves all Soldiers' equipment-layouts.
 */
void InventoryState::saveEquipmentLayout()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleGame->getUnits()->begin();
			i != _battleGame->getUnits()->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier() != NULL)
		{
			std::vector<EquipmentLayoutItem*>* const layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();
			if (layoutItems->empty() == false)
			{
				for (std::vector<EquipmentLayoutItem*>::const_iterator
						j = layoutItems->begin();
						j != layoutItems->end();
						++j)
				{
					delete *j;
				}

				layoutItems->clear();
			}

			// save Soldier's items
			// Note: when using getInventory() the loaded ammos are skipped because
			// they're not owned by the unit; ammo is handled separately by the weapon.
			for (std::vector<BattleItem*>::const_iterator
					j = (*i)->getInventory()->begin();
					j != (*i)->getInventory()->end();
					++j)
			{
				std::string ammo;
				if ((*j)->needsAmmo() == true
					&& (*j)->getAmmoItem() != NULL)
				{
					ammo = (*j)->getAmmoItem()->getRules()->getType();
				}
				else
					ammo = "NONE";

				layoutItems->push_back(new EquipmentLayoutItem(
															(*j)->getRules()->getType(),
															(*j)->getSlot()->getId(),
															(*j)->getSlotX(),
															(*j)->getSlotY(),
															ammo,
															(*j)->getFuseTimer()));
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnOkClick(Action*)
{
	if (_inv->getSelectedItem() != NULL)
		return;


	_game->popState();

	if (_tuMode == false) // pre-Battle inventory equip.
	{
		saveEquipmentLayout();

		if (_parent != NULL) // going into Battlescape!
		{
			_battleGame->resetUnitTiles();

			Tile* const invTile = _battleGame->getBattleInventory();
			_battleGame->randomizeItemLocations(invTile);	// This doesn't seem to happen on second stage of Multi-State MISSIONS.
															// In fact, none of this !_tuMode InventoryState appears to run for 2nd staged missions.
															// and BattlescapeGenerator::nextStage() has its own bu->prepUnit() call ....
/*			if (_battleGame->getTurn() == 1) // Leaving this out could be troublesome for Multi-Stage MISSIONS.
			{
				//Log(LOG_INFO) << ". turn = 1";
				_battleGame->randomizeItemLocations(invTile);
				if (invTile->getUnit())
				{
					// make sure we select the unit closest to the ramp.
					_battleGame->setSelectedUnit(invTile->getUnit());
				}
			} */

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleGame->getUnits()->begin();
					i != _battleGame->getUnits()->end();
					++i)
			{
				if ((*i)->getFaction() == FACTION_PLAYER)
					(*i)->initTU(true);
			}
		}
	}
}

/**
 * Selects the previous soldier.
 * @param action - pointer to an Action
 */
void InventoryState::btnPrevClick(Action*)
{
	if (_inv->getSelectedItem() != NULL)
		return;


	if (_parent != NULL)
		_parent->selectPreviousFactionUnit(
										false,
										false,
										true);
	else
		_battleGame->selectPreviousFactionUnit(
											false,
											false,
											true);
	init();
}

/**
 * Selects the next soldier.
 * @param action - pointer to an Action
 */
void InventoryState::btnNextClick(Action*)
{
	if (_inv->getSelectedItem() != NULL)
		return;


	if (_parent != NULL)
		_parent->selectNextFactionUnit(
									false,
									false,
									true);
	else
		_battleGame->selectNextFactionUnit(
										false,
										false,
										true);
	init();
}

/**
 * Unloads the selected weapon.
 * @param action - pointer to an Action
 */
void InventoryState::btnUnloadClick(Action*)
{
	if (_inv->unload() == true)
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_txtUseTU->setText(L"");

		_selAmmo->clear();

		updateStats();
		_game->getResourcePack()->getSoundByDepth(
											0,
											ResourcePack::ITEM_UNLOAD_HQ)
										->play();
	}
}

/**
 * Shows more ground items / rearranges them.
 * @param action - pointer to an Action
 */
void InventoryState::btnGroundClick(Action*)
{
	_inv->arrangeGround();
}

/**
 * Shows the unit info screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnRankClick(Action*)
{
	_game->pushState(new UnitInfoState(
									_battleGame->getSelectedUnit(),
									_parent,
									true,
									false));
}

/**
 * Clears unit's inventory - move everything to the ground.
 * @note Helper for btnClearInventoryClick().
 * @param game			- pointer to the core Game to get the Ruleset
 * @param unitInv		- pointer to a vector of pointers to BattleItems
 * @param groundTile	- pointer to the ground Tile
 */
static void _clearInventory(
		Game* game,
		std::vector<BattleItem*>* unitInv,
		Tile* groundTile)
{
	RuleInventory* const groundRuleInv = game->getRuleset()->getInventory("STR_GROUND");

	for (std::vector<BattleItem*>::const_iterator
			i = unitInv->begin();
			i != unitInv->end();
			)
	{
		(*i)->setOwner(NULL);
		groundTile->addItem(
						*i,
						groundRuleInv);

		i = unitInv->erase(i);
	}
}

/**
 * Jogs the mouse cursor to refresh appearances.
 */
void InventoryState::_refreshMouse()
{
	int
		x,
		y;
	SDL_GetMouseState(&x, &y);

	SDL_WarpMouse(x + 1, y);	// send a mouse motion event to refresh any hover actions
	SDL_WarpMouse(x, y);		// move the mouse back to avoid cursor creep
}

/**
 * Clears the current unit's inventory and moves all items to the ground.
 * @param action - pointer to an Action
 */
void InventoryState::btnClearInventoryClick(Action* action)
{
	if (_inv->getSelectedItem() != NULL) // don't accept clicks when moving items
		return;

	BattleUnit* const unit = _battleGame->getSelectedUnit();
	std::vector<BattleItem*>* const unitInv = unit->getInventory();
	Tile* const groundTile = unit->getTile();

	_clearInventory(
				_game,
				unitInv,
				groundTile);

	_inv->arrangeGround(false); // refresh ui
	updateStats();
	_refreshMouse();

	_game->getResourcePack()->getSoundByDepth(
										_battleGame->getDepth(),
										ResourcePack::ITEM_DROP)->play();
}

/**
 * Updates item info.
 */
void InventoryState::invClick(Action*)
{
	_tuCost->setVisible(false);

	// kL_note: This function has only updateStats() in the stock code,
	// since induction of Copy/Paste Inventory Layouts ... that is, the
	// vast majority of this function has been subsumed into invMouseOver().
	// But i'm leaving it in anyway ...

	const BattleItem* const item = _inv->getSelectedItem();
	if (item != NULL)
	{
		const RuleItem* const itemRule = item->getRules();
		const BattleItem* const ammo = item->getAmmoItem();

		setExtraInfo(
					item,
					itemRule,
					ammo);

		std::wstring wstr;

		if (ammo != NULL
			&& item->needsAmmo() == true)
		{
			wstr = tr("STR_AMMO_ROUNDS_LEFT").arg(ammo->getAmmoQuantity());

			SDL_Rect r;
			r.x = 0;
			r.y = 0;
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_selAmmo->drawRect(&r, _game->getRuleset()->getInterface("inventory")->getElement("grid")->color);
//			_selAmmo->drawRect(&r, Palette::blockOffset(0)+8);

			++r.x;
			++r.y;
			r.w -= 2;
			r.h -= 2;
			_selAmmo->drawRect(&r, Palette::blockOffset(0)+15);

			ammo->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_selAmmo);
		}
		else if (item->getAmmoQuantity() != 0
			&& item->needsAmmo() == true)
		{
			wstr = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (itemRule->getBattleType() == BT_MEDIKIT)
			wstr = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
						.arg(item->getPainKillerQuantity())
						.arg(item->getStimulantQuantity())
						.arg(item->getHealQuantity());

		_txtAmmo->setText(wstr);
	}
/*	else
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");

		_selAmmo->clear();
	} */

	updateStats();
}

/**
 * Shows item info.
 * @param action - pointer to an Action
 */
void InventoryState::invMouseOver(Action* action)
{
	if (_inv->getSelectedItem() != NULL)
	{
		_tuCost->setValue(_inv->getTUCost());
		_tuCost->setVisible(_tuMode
						 && _inv->getTUCost() > 0);

//		_updateTemplateButtons(false);
		return;
	}

	const BattleItem* const item = _inv->getMouseOverItem();
	if (item != NULL)
	{
//		_updateTemplateButtons(false);
//		_tuCost->setVisible(false);

		const RuleItem* const itemRule = item->getRules();
		const BattleItem* const ammo = item->getAmmoItem();

		setExtraInfo(
				item,
				itemRule,
				ammo);

		std::wstring wstr;

		if (ammo != NULL
			&& item->needsAmmo() == true)
		{
			wstr = tr("STR_AMMO_ROUNDS_LEFT").arg(ammo->getAmmoQuantity());

			SDL_Rect r;
			r.x = 0;
			r.y = 0;
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_selAmmo->drawRect(&r, _game->getRuleset()->getInterface("inventory")->getElement("grid")->color);
//			_selAmmo->drawRect(&r, Palette::blockOffset(0)+8);

			++r.x;
			++r.y;
			r.w -= 2;
			r.h -= 2;
			_selAmmo->drawRect(&r, Palette::blockOffset(0)+15);

			ammo->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_selAmmo);
//			_updateTemplateButtons(false);
		}
		else
		{
			_selAmmo->clear();
//			_updateTemplateButtons(!_tuMode);
		}

		if (item->getAmmoQuantity() != 0
			&& item->needsAmmo() == true)
		{
			wstr = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (itemRule->getBattleType() == BT_MEDIKIT)
		{
			wstr = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
					.arg(item->getPainKillerQuantity())
					.arg(item->getStimulantQuantity())
					.arg(item->getHealQuantity());
		}

		_txtAmmo->setText(wstr);
	}
	else
	{
//		if (_currentTooltip.empty())
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_txtUseTU->setText(L"");

		_selAmmo->clear();
//		_updateTemplateButtons(!_tuMode);
	}
}

/**
 * Hides item info.
 * @param action - pointer to an Action
 */
void InventoryState::invMouseOut(Action*)
{
	_txtItem->setText(L"");
	_txtAmmo->setText(L"");
	_txtUseTU->setText(L"");

	_selAmmo->clear();

//	_updateTemplateButtons(!_tuMode);
	_tuCost->setVisible(false);
}

/**
 * Takes care of any events from the core game engine.
 * @param action - pointer to an Action
 */
void InventoryState::handle(Action* action)
{
	State::handle(action);

#ifndef __MORPHOS__
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
			btnNextClick(action);
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
			btnPrevClick(action);
	}
#endif
}

/**
 * Sets the extra-info fields on mouseover and mouseclicks.
 * @param item		- pointer to an item
 * @param itemRule	- pointer to item's RuleItem
 * @param ammo		- pointer to item's ammo
 */
// private:
void InventoryState::setExtraInfo(
		const BattleItem* const item,
		const RuleItem* const itemRule,
		const BattleItem* const ammo)
{
		std::wostringstream label;
		bool isArt = false;

		if (item->getUnit() != NULL
			&& item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		{
			label << item->getUnit()->getName(_game->getLanguage());
		}
		else
		{
			if (_game->getSavedGame()->isResearched(itemRule->getRequirements()))
				label << tr(itemRule->getName());
			else
			{
				label << tr("STR_ALIEN_ARTIFACT");
				isArt = true;
			}
		}

		int weight = itemRule->getWeight();
		if (ammo != NULL
			&& ammo != item)
		{
			weight += ammo->getRules()->getWeight();
		}

		label << " (" << weight << ")";
		_txtItem->setText(label.str());


		const BattleUnit* const unit = _battleGame->getSelectedUnit();
		const BattleActionType bat = itemRule->getDefaultAction(item->getFuseTimer() > -1);

		if (unit != NULL
			&& isArt == false
			&& (bat != BA_NONE
				|| itemRule->getBattleType() == BT_AMMO))
		{
			std::string actionType;
			int tuUse;

			if (itemRule->getBattleType() == BT_AMMO)
			{
				actionType = "STR_RELOAD_";
				tuUse = 15;
			}
			else
			{
				switch (bat)
				{
					case BA_LAUNCH:		actionType = "STR_LAUNCH_";	break;
					case BA_SNAPSHOT:	actionType = "STR_SNAP_";	break;
					case BA_AUTOSHOT:	actionType = "STR_BURST_";	break;
					case BA_AIMEDSHOT:	actionType = "STR_SCOPE_";	break;
					case BA_PRIME:		actionType = "STR_PRIME_";	break;
					case BA_DEFUSE:		actionType = "STR_DEFUSE_";	break;
					case BA_USE:		actionType = "STR_USE_";	break;
					case BA_PANIC:		actionType = "STR_PSI_";	break;
					case BA_HIT:		actionType = "STR_ATTACK_";
				}

				tuUse = unit->getActionTUs(
										bat,
										item);
			}

			_txtUseTU->setText(tr(actionType).arg(tuUse));
		}
		else
			_txtUseTU->setText(L"");
}

}

/**
* Shows a tooltip for the appropriate button.
* @param action - pointer to an Action
*/
/*kL
void InventoryState::txtTooltipIn(Action* action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtItem->setText(tr(_currentTooltip));
	}
} */

/**
* Clears the tooltip text.
* @param action - pointer to an Action
*/
/*kL
void InventoryState::txtTooltipOut(Action* action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText(L"");
		}
	}
} */

/**
 *
 */
/* void InventoryState::_updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		_game->getResourcePack()->getSurface("InvClear")->blit(_btnClearInventory);

		if (_curInventoryTemplate.empty()) // use "empty template" icons
		{
			_game->getResourcePack()->getSurface("InvCopy")->blit(_btnCreateTemplate);
			_game->getResourcePack()->getSurface("InvPasteEmpty")->blit(_btnApplyTemplate);
		}
		else // use "active template" icons
		{
			_game->getResourcePack()->getSurface("InvCopyActive")->blit(_btnCreateTemplate);
			_game->getResourcePack()->getSurface("InvPaste")->blit(_btnApplyTemplate);
		}

		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
		_btnClearInventory->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
		_btnClearInventory->clear();
	}
} */

/**
 *
 */
/* void InventoryState::btnCreateTemplateClick(Action* action)
{
	if (_inv->getSelectedItem() != NULL) // don't accept clicks when moving items
		return;

	_clearInventoryTemplate(_curInventoryTemplate); // clear current template

	// Copy inventory instead of just keeping a pointer to it. This way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked.
	std::vector<BattleItem*>* unitInv = _battleGame->getSelectedUnit()->getInventory();
	for (std::vector<BattleItem*>::iterator
			j = unitInv->begin();
			j != unitInv->end();
			++j)
	{
		std::string ammo;
		if ((*j)->needsAmmo()
			&& (*j)->getAmmoItem())
		{
			ammo = (*j)->getAmmoItem()->getRules()->getType();
		}
		else
			ammo = "NONE";

		_curInventoryTemplate.push_back(new EquipmentLayoutItem(
															(*j)->getRules()->getType(),
															(*j)->getSlot()->getId(),
															(*j)->getSlotX(),
															(*j)->getSlotY(),
															ammo,
															(*j)->getFuseTimer()));
	}

	_game->getResourcePack()->getSoundByDepth(
											_battleGame->getDepth(),
											ResourcePack::ITEM_DROP)
										->play();
	_refreshMouse();
} */

/**
 *
 */
/* void InventoryState::btnApplyTemplateClick(Action* action)
{
	if (_inv->getSelectedItem() != NULL)	// don't accept clicks when moving items
//		|| _curInventoryTemplate.empty())	// or when the template is empty ->
											// if the template is empty -- it will just result in clearing the unit's inventory
	{
		return;
	}

	BattleUnit* unit = _battleGame->getSelectedUnit();
	std::vector<BattleItem*>* unitInv = unit->getInventory();

	Tile* groundTile = unit->getTile();
	std::vector<BattleItem*>* groundInv = groundTile->getInventory();

	RuleInventory* groundRuleInv = _game->getRuleset()->getInventory("STR_GROUND");

	_clearInventory(
				_game,
				unitInv,
				groundTile);

	// attempt to replicate inventory template by grabbing corresponding items
	// from the ground. if any item is not found on the ground, display warning
	// message, but continue attempting to fulfill the template as best we can
	bool itemMissing = false;

	std::vector<EquipmentLayoutItem*>::iterator templateIt;
	for (
			templateIt = _curInventoryTemplate.begin();
			templateIt != _curInventoryTemplate.end();
			++templateIt)
	{
		// search for template item in ground inventory
		std::vector<BattleItem*>::iterator groundItem;
		const bool needsAmmo = !_game->getRuleset()->getItem((*templateIt)->getItemType())->getCompatibleAmmo()->empty();
		bool
			found = false,
			rescan = true;

		while (rescan)
		{
			rescan = false;

			const std::string targetAmmo = (*templateIt)->getAmmoItem();
			BattleItem* matchedWeapon = NULL;
			BattleItem* matchedAmmo = NULL;

			for (
					groundItem = groundInv->begin();
					groundItem != groundInv->end();
					++groundItem)
			{
				// if we find the appropriate ammo, remember it for later for if we find
				// the right weapon but with the wrong ammo
				const std::string groundItemName = (*groundItem)->getRules()->getType();
				if (needsAmmo
					&& targetAmmo == groundItemName)
				{
					matchedAmmo = *groundItem;
				}

				if ((*templateIt)->getItemType() == groundItemName)
				{
					// if the loaded ammo doesn't match the template item's,
					// remember the weapon for later and continue scanning
					BattleItem* loadedAmmo = (*groundItem)->getAmmoItem();
					if ((needsAmmo
							&& loadedAmmo
							&& targetAmmo != loadedAmmo->getRules()->getType())
						|| (needsAmmo
							&& !loadedAmmo))
					{
						// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
						if (!matchedWeapon
							|| matchedWeapon->getAmmoItem())
						{
							matchedWeapon = *groundItem;
						}

						continue;
					}

					// move matched item from ground to the appropriate inv slot
					(*groundItem)->setOwner(unit);
					(*groundItem)->setSlot(_game->getRuleset()->getInventory((*templateIt)->getSlot()));
					(*groundItem)->setSlotX((*templateIt)->getSlotX());
					(*groundItem)->setSlotY((*templateIt)->getSlotY());
					(*groundItem)->setFuseTimer((*templateIt)->getFuseTimer());

					unitInv->push_back(*groundItem);
					groundInv->erase(groundItem);

					found = true;
					break;
				}
			}

			// if we failed to find an exact match, but found unloaded ammo and
			// the right weapon, unload the target weapon, load the right ammo, and use it
			if (!found
				&& matchedWeapon
				&& (!needsAmmo
					|| matchedAmmo))
			{
				// unload the existing ammo (if any) from the weapon
				BattleItem* loadedAmmo = matchedWeapon->getAmmoItem();
				if (loadedAmmo)
				{
					groundTile->addItem(loadedAmmo, groundRuleInv);
					matchedWeapon->setAmmoItem(NULL);
				}

				// load the correct ammo into the weapon
				if (matchedAmmo)
				{
					matchedWeapon->setAmmoItem(matchedAmmo);
					groundTile->removeItem(matchedAmmo);
				}

				// rescan and pick up the newly-loaded/unloaded weapon
				rescan = true;
			}
		}

		if (!found)
			itemMissing = true;
	}

	if (itemMissing)
		_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));


	_inv->arrangeGround(false); // refresh ui
	updateStats();
	_refreshMouse();

	_game->getResourcePack()->getSoundByDepth(
											_battleGame->getDepth(),
											ResourcePack::ITEM_DROP)
										->play();
} */
