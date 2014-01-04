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

#include "DebriefingState.h"

#include <sstream>

#include "CannotReequipState.h"
#include "NoContainmentState.h"
#include "PromotionsState.h"

#include "../Basescape/ManageAlienContainmentState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/MainMenuState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Debriefing screen.
 * @param game Pointer to the core game.
 */
DebriefingState::DebriefingState(Game* game)
	:
		State(game),
		_region(0),
		_country(0),
		_noContainment(false),
		_manageContainment(false),
		_destroyBase(false)
{
	//Log(LOG_INFO) << "Create DebriefingState";

	// Restore the cursor in case something weird happened
	_game->getCursor()->setVisible(true);


	if (Options::getBool("alienContainmentLimitEnforced"))
		_containmentLimit = 1;
	else
		_containmentLimit = 0;
	_containmentLimit = Options::getBool("alienContainmentLimitEnforced")? 1: 0;


	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(280, 17, 16, 8);

	_txtItem		= new Text(180, 9, 16, 24);
	_txtQuantity	= new Text(60, 9, 200, 24);
	_txtScore		= new Text(55, 9, 260, 24);

	_lstStats		= new TextList(288, 80, 16, 32);

	_lstRecovery	= new TextList(288, 80, 16, 32);
	_txtRecovery	= new Text(180, 9, 16, 60);

	_lstTotal		= new TextList(288, 9, 16, 12);

	_btnOk			= new TextButton(176, 16, 16, 177);
	_txtRating		= new Text(100, 9, 212, 180);


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_0")->getColors());
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtItem);
	add(_txtQuantity);
	add(_txtScore);
	add(_txtRecovery);
	add(_lstStats);
	add(_lstRecovery);
	add(_lstTotal);
	add(_btnOk);
	add(_txtRating);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& DebriefingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					(SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setBig();

	_txtItem->setColor(Palette::blockOffset(8)+5);
	_txtItem->setText(tr("STR_LIST_ITEM"));

	_txtQuantity->setColor(Palette::blockOffset(8)+5);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtScore->setColor(Palette::blockOffset(8)+5);
	_txtScore->setText(tr("STR_SCORE"));

	_txtRecovery->setColor(Palette::blockOffset(8)+5);
	_txtRecovery->setText(tr("STR_UFO_RECOVERY"));

	_txtRating->setColor(Palette::blockOffset(8)+5);

	_lstStats->setColor(Palette::blockOffset(15)-1);
	_lstStats->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstStats->setColumns(3, 176, 60, 64);
	_lstStats->setDot(true);
	_lstStats->setMargin(8);

	_lstRecovery->setColor(Palette::blockOffset(15)-1);
	_lstRecovery->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstRecovery->setColumns(3, 176, 60, 64);
	_lstRecovery->setDot(true);
	_lstRecovery->setMargin(8);

	_lstTotal->setColor(Palette::blockOffset(8)+5);
	_lstTotal->setColumns(2, 244, 64);
	_lstTotal->setDot(true);


	prepareDebriefing();


	int total = 0,
		stats_dy = 0,
		recovery_dy = 0;

	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->qty == 0) continue;


		std::wstringstream
			ss,
			ss2;

		ss << L'\x01' << (*i)->qty << L'\x01';	// quantity of recovered item-type
		ss2 << L'\x01' << (*i)->score;			// score
		total += (*i)->score;					// total score

		if ((*i)->recovery)
		{
			_lstRecovery->addRow(
								3,
								tr((*i)->item).c_str(),
								ss.str().c_str(),
								ss2.str().c_str());
			recovery_dy += 8;
		}
		else
		{
			_lstStats->addRow(
							3,
							tr((*i)->item).c_str(),
							ss.str().c_str(),
							ss2.str().c_str());
			stats_dy += 8;
		}
	}

	std::wstringstream ss3;
	ss3 << total;
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					ss3.str().c_str());

	if (recovery_dy > 0)
	{
		_txtRecovery->setY(_lstStats->getY() + stats_dy + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recovery_dy + 5);
	}
	else
	{
		_txtRecovery->setText(L"");
		_lstTotal->setY(_lstStats->getY() + stats_dy + 5);
	}

	// add the points to our activity score
	if (_region)
	{
		_region->addActivityXcom(total);
	}

	if (_country)
	{
		_country->addActivityXcom(total);
	}

	// Calculate rating
	std::wstring rating;
	if (total < -99)
	{
		rating = tr("STR_RATING_TERRIBLE");
	}
	else if (total < 51)
	{
		rating = tr("STR_RATING_POOR");
	}
	else if (total < 276)
	{
		rating = tr("STR_RATING_OK");
	}
	else if (total < 601)
	{
		rating = tr("STR_RATING_GOOD");
	}
	else if (total < 1001)
	{
		rating = tr("STR_RATING_EXCELLENT");
	}
	else
	{
		rating = tr("STR_RATING_STUPENDOUS");
	}

	_txtRating->setText(tr("STR_RATING").arg(rating));


	_game->getResourcePack()->getMusic("GMMARS")->play();

	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);

	//Log(LOG_INFO) << "Create DebriefingState DONE";
}

/**
 *
 */
DebriefingState::~DebriefingState()
{
	//Log(LOG_INFO) << "Delete DebriefingState";

	if (_game->isQuitting())
	{
		_game->getSavedGame()->setBattleGame(0);
	}

	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		delete *i;
	}

	_rounds.clear();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void DebriefingState::btnOkClick(Action*)
{
	//Log(LOG_INFO) << "DebriefingState::btnOkClick()";

	_game->getSavedGame()->setBattleGame(0);
	_game->popState();

	if (_game->getSavedGame()->getMonthsPassed() == -1)
	{
		_game->setState(new MainMenuState(_game));
	}
	else if (!_destroyBase)
	{
		if (_game->getSavedGame()->handlePromotions())
		{
			_game->pushState(new PromotionsState(_game));
		}

		if (!_missingItems.empty())
		{
			_game->pushState(new CannotReequipState(
												_game,
												_missingItems));
		}

		if (_noContainment)
		{
			_game->pushState(new NoContainmentState(_game));
		}
		else if (_manageContainment)
		{
			_game->pushState(new ManageAlienContainmentState(
															_game,
															_base,
															OPT_BATTLESCAPE));
			_game->pushState(new ErrorMessageState(
												_game,
												tr("STR_CONTAINMENT_EXCEEDED")
													.arg(_base->getName()).c_str(),
												Palette::blockOffset(8)+5,
												"BACK01.SCR",
												0));
		}
	}

	//Log(LOG_INFO) << "DebriefingState::btnOkClick() EXIT";
}

/**
 * Adds to the debriefing stats.
 * @param name The untranslated name of the stat.
 * @param quantity The quantity to add.
 * @param score The score to add.
 */
void DebriefingState::addStat(
		const std::string& name,
		int quantity,
		int score)
{
	//Log(LOG_INFO) << "DebriefingState::addStat()";

	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->item == name)
		{
			(*i)->qty = (*i)->qty + quantity;
			(*i)->score = (*i)->score + score;

			break;
		}
	}

	//Log(LOG_INFO) << "DebriefingState::addStat() EXIT";
}


/**
 * Clears the alien base from supply missions that use it.
 */
class ClearAlienBase
	:
		public std::unary_function<AlienMission*, void>
{

private:
	const AlienBase* _base;

	public:
		/// Remembers the base.
		ClearAlienBase(const AlienBase* base)
			:
				_base(base)
		{
			//Log(LOG_INFO) << "DebriefingState, ClearAlienBase() cTor";
			/* Empty by design. */
		}

		/// Clears the base if required.
		void operator()(AlienMission* am) const;
};

/**
 * Removes the association between the alien mission and the alien base,
 * if one existed.
 * @param am Pointer to the alien mission.
 */
void ClearAlienBase::operator()(AlienMission* am) const
{
	//Log(LOG_INFO) << "DebriefingState, ClearAlienBase()";

	if (am->getAlienBase() == _base)
	{
		am->setAlienBase(0);
	}
}


/**
 * Prepares debriefing: gathers Aliens, Corpses, Artefacts, UFO Components.
 * Adds the items to the craft.
 * Also calculates the soldiers' experience, and possible promotions.
 * If aborted, only the things on the exit area are recovered.
 */
void DebriefingState::prepareDebriefing()
{
	//Log(LOG_INFO) << "DebriefingState::prepareDebriefing()";

	_stats.push_back(new DebriefingStat("STR_ALIENS_KILLED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_CORPSES_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_LIVE_ALIENS_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ARTIFACTS_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_BASE_CONTROL_DESTROYED"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_ALIENS"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_SAVED"));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_KILLED"));
//	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_RETIRED_THROUGH_INJURY"));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION"));
	_stats.push_back(new DebriefingStat("STR_TANKS_DESTROYED"));
	_stats.push_back(new DebriefingStat("STR_XCOM_CRAFT_LOST"));

	_stats.push_back(new DebriefingStat("STR_UFO_POWER_SOURCE", true));
	_stats.push_back(new DebriefingStat("STR_UFO_NAVIGATION", true));
	_stats.push_back(new DebriefingStat("STR_UFO_CONSTRUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_FOOD", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_REPRODUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ENTERTAINMENT", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_SURGERY", true));
	_stats.push_back(new DebriefingStat("STR_EXAMINATION_ROOM", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ALLOYS", true));
	_stats.push_back(new DebriefingStat("STR_ELERIUM_115", true));

	SavedGame* save = _game->getSavedGame();
	SavedBattleGame* battle = save->getSavedBattle();

	bool aborted = battle->isAborted();
	bool success = !aborted;

	Craft* craft = 0;
	std::vector<Craft*>::iterator craftIterator;

	Base* base = 0;

	int soldierExit = 0;	// if this stays 0 the craft is lost...
	int soldierLive = 0;	// if this stays 0 the craft is lost...
	int soldierOut = 0;

	for (std::vector<Base*>::iterator
			i = save->getBases()->begin();
			i != save->getBases()->end();
			++i)
	{
		// in case we have a craft - check which craft it is about
		for (std::vector<Craft*>::iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->isInBattlescape())
			{
				double craftLon = (*j)->getLongitude();
				double craftLat = (*j)->getLatitude();

				for (std::vector<Region*>::iterator
						k = save->getRegions()->begin();
						k != save->getRegions()->end();
						++k)
				{
					if ((*k)->getRules()->insideRegion(
													craftLon,
													craftLat))
					{
						_region = *k;

						break;
					}
				}

				for (std::vector<Country*>::iterator
						k = save->getCountries()->begin();
						k != save->getCountries()->end();
						++k)
				{
					if ((*k)->getRules()->insideCountry(
													craftLon,
													craftLat))
					{
						_country = *k;

						break;
					}
				}

				base = *i;
				craft = *j;
				craftIterator = j;
				craft->returnToBase();
				craft->setLowFuel(true);
				craft->setInBattlescape(false);
			}
			else if ((*j)->getDestination() != 0)
			{
				Ufo* u = dynamic_cast<Ufo*>((*j)->getDestination());
				if (u != 0
					&& u->isInBattlescape())
				{
					(*j)->returnToBase();
				}
			}
		}

		if ((*i)->isInBattlescape()) // in case we DON'T have a craft (base defense)
		{
			base = *i;
			base->setInBattlescape(false);

			double baseLon = (*i)->getLongitude();
			double baseLat = (*i)->getLatitude();

			for (std::vector<Region*>::iterator
					k = save->getRegions()->begin();
					k != save->getRegions()->end();
					++k)
			{
				if ((*k)->getRules()->insideRegion(
												baseLon,
												baseLat))
				{
					_region = *k;

					break;
				}
			}

			for (std::vector<Country*>::iterator
					k = save->getCountries()->begin();
					k != save->getCountries()->end();
					++k)
			{
				if ((*k)->getRules()->insideCountry(
												baseLon,
												baseLat))
				{
					_country = *k;

					break;
				}
			}

			if (aborted)
			{
				_destroyBase = true;
			}
		}
	}

	_base = base;

	// UFO crash/landing site disappears
	for (std::vector<Ufo*>::iterator
			i = save->getUfos()->begin();
			i != save->getUfos()->end();
			++i)
	{
		if ((*i)->isInBattlescape())
		{
			if (!aborted)
			{
				delete *i;
				save->getUfos()->erase(i);

				break;
			}
			else
			{
				(*i)->setInBattlescape(false);
			}
		}
	}

	// terror site disappears (even when you abort)
	for (std::vector<TerrorSite*>::iterator
			i = save->getTerrorSites()->begin();
			i != save->getTerrorSites()->end();
			++i)
	{
		if ((*i)->isInBattlescape())
		{
			delete *i;
			save->getTerrorSites()->erase(i);

			break;
		}
	}

	// lets see what happens with units; first,
	// we evaluate how many surviving XCom units there are,
	// and if they are unconscious
	for (std::vector<BattleUnit*>::iterator
			j = battle->getUnits()->begin();
			j != battle->getUnits()->end();
			++j)
	{
		if ((*j)->getOriginalFaction() == FACTION_PLAYER
			&& (*j)->getStatus() != STATUS_DEAD)
		{
			if ((*j)->getStatus() == STATUS_UNCONSCIOUS
				|| (*j)->getFaction() == FACTION_HOSTILE)
			{
				soldierOut++;
			}

			soldierLive++;
		}
	}

	// if all our men are unconscious, the aliens get to have their way with them.
	if (soldierOut == soldierLive)
	{
		soldierLive = 0;

		for (std::vector<BattleUnit*>::iterator
				j = battle->getUnits()->begin();
				j != battle->getUnits()->end();
				++j)
		{
			if ((*j)->getOriginalFaction() == FACTION_PLAYER
				&& (*j)->getStatus() != STATUS_DEAD)
			{
				(*j)->instaKill();
			}
		}
	}

	std::string mission = battle->getMissionType();

	// alien base disappears (if you didn't abort)
	if (mission == "STR_ALIEN_BASE_ASSAULT")
	{
		_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));
		bool destroyAlienBase = true;

		if (aborted
			|| soldierLive == 0)
		{
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				// get recoverable map data objects from the battlescape map
				if (battle->getTiles()[i]->getMapData(3)
					&& battle->getTiles()[i]->getMapData(3)->getSpecialType() == UFO_NAVIGATION)
				{
					destroyAlienBase = false;

					break;
				}
			}
		}

		success = destroyAlienBase;

		for (std::vector<AlienBase*>::iterator
				i = save->getAlienBases()->begin();
				i != save->getAlienBases()->end();
				++i)
		{
			if ((*i)->isInBattlescape())
			{
				if (destroyAlienBase)
				{
					addStat(
						"STR_ALIEN_BASE_CONTROL_DESTROYED",
						1,
						350);

					// Take care to remove supply missions for this base.
					std::for_each(
							save->getAlienMissions().begin(),
							save->getAlienMissions().end(),
							ClearAlienBase(*i));

					delete *i;
					save->getAlienBases()->erase(i);

					break;
				}
				else
				{
					(*i)->setInBattlescape(false);

					break;
				}
			}
		}
	}

	// time to care for units.
	for (std::vector<BattleUnit*>::iterator
			j = battle->getUnits()->begin();
			j != battle->getUnits()->end();
			++j)
	{
		if (!(*j)->getTile())
		{
			(*j)->setTile(battle->getTile((*j)->getPosition())); // this, why
		}

		UnitFaction origFaction = (*j)->getOriginalFaction();
		int value = (*j)->getValue();

		UnitStatus status = (*j)->getStatus();
		if (status == STATUS_DEAD) // so this is a dead unit
		{
			if (origFaction == FACTION_HOSTILE
				&& (*j)->killedBy() == FACTION_PLAYER)
			{
				addStat(
					"STR_ALIENS_KILLED",
					1,
					value);
			}
			else if (origFaction == FACTION_PLAYER)
			{
				Soldier* soldier = save->getSoldier((*j)->getId());
				if (soldier != 0)
				{
					addStat(
						"STR_XCOM_OPERATIVES_KILLED",
						1,
						-value);

					for (std::vector<Soldier*>::iterator
							i = base->getSoldiers()->begin();
							i != base->getSoldiers()->end();
							++i)
					{
						if ((*i) == soldier)
						{
							(*j)->updateGeoscapeStats(*i);
							SoldierDeath* death = new SoldierDeath();
							death->setTime(new GameTime(*save->getTime()));

							(*i)->die(death);
							save->getDeadSoldiers()->push_back(*i);
							base->getSoldiers()->erase(i);

							break;
						}
					}
				}
				else // not soldier -> tank
				{
					addStat(
						"STR_TANKS_DESTROYED",
						1,
						-value);
				}
			}
			else if (origFaction == FACTION_NEUTRAL)
			{
				if ((*j)->killedBy() == FACTION_PLAYER)
				{
					addStat(
						"STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES",
						1,
						-(value * 2)); // kL
				}
				else
				{
					// if civilians happen to kill themselves XCOM shouldn't get penalty for it
					addStat(
						"STR_CIVILIANS_KILLED_BY_ALIENS",
						1,
						-value); // kL
				}
			}
		}
		else // so this unit is not dead...
		{
			UnitFaction faction = (*j)->getFaction();

			std::string type = (*j)->getType();
			if ((*j)->getSpawnUnit() != "")
			{
				type = (*j)->getSpawnUnit();
			}

			if (origFaction == FACTION_PLAYER)
			{
				Soldier* soldier = save->getSoldier((*j)->getId());

				if (((*j)->isInExitArea()
						&& (mission != "STR_BASE_DEFENSE" || success))
					|| !aborted)
				{
					// so game is not aborted or aborted and unit is on exit area
					(*j)->postMissionProcedures(save);

					soldierExit++;

					if (soldier != 0)
						recoverItems(
								(*j)->getInventory(),
								base);
					else // not soldier -> tank
					{
						base->getItems()->addItem(type);

						RuleItem* tankRule = _game->getRuleset()->getItem(type);
						BattleItem* ammoItem = (*j)->getItem("STR_RIGHT_HAND")->getAmmoItem();

						if (!tankRule->getCompatibleAmmo()->empty()
							&& ammoItem != 0
							&& ammoItem->getAmmoQuantity() > 0)
						{
							base->getItems()->addItem(
													tankRule->getCompatibleAmmo()->front(),
													ammoItem->getAmmoQuantity());
						}
					}
				}
				else // so game is aborted and unit is not on exit area
				{
					addStat(
						"STR_XCOM_OPERATIVES_MISSING_IN_ACTION",
						1,
						-value);

					if (soldier != 0)
					{
						for (std::vector<Soldier*>::iterator
								i = base->getSoldiers()->begin();
								i != base->getSoldiers()->end();
								++i)
						{
							if ((*i) == soldier)
							{
								(*j)->updateGeoscapeStats(*i);
								SoldierDeath* death = new SoldierDeath();
								death->setTime(new GameTime(*save->getTime()));

								(*i)->die(death);
								save->getDeadSoldiers()->push_back(*i);
								base->getSoldiers()->erase(i);

								break;
							}
						}
					}
				}
			}
			else if (origFaction == FACTION_HOSTILE
				&& (!aborted || (*j)->isInExitArea())
				&& faction == FACTION_PLAYER // mind controlled units may as well count as unconscious
				&& !(*j)->isOut())
			{
				for (std::vector<BattleItem*>::iterator
						k = (*j)->getInventory()->begin();
						k != (*j)->getInventory()->end();
						++k)
				{
					if (!(*k)->getRules()->isFixed())
					{
						(*j)->getTile()->addItem(
												*k,
												_game->getRuleset()->getInventory("STR_GROUND"));
					}
				}

				std::string corpseItem = (*j)->getArmor()->getCorpseGeoscape();
				if ((*j)->getSpawnUnit() != "")
				{
					Ruleset* rule = _game->getRuleset();
					corpseItem = rule
									->getArmor(rule->getUnit((*j)->getSpawnUnit())->getArmor())
									->getCorpseGeoscape();
				}

				addStat(
					"STR_LIVE_ALIENS_RECOVERED",
					1,
					value * 2); // kL, duplicated in function below.

				if (save->isResearchAvailable(
						_game->getRuleset()->getResearch(type),
						save->getDiscoveredResearch(),
						_game->getRuleset()))
				{
					addStat( // more points if it's not researched
						"STR_LIVE_ALIENS_RECOVERED",
						0,
						value * 3); // kL, duplicated in function below.

					if (base->getAvailableContainment() == 0)
					{
						_noContainment = true;
						base->getItems()->addItem(corpseItem);
					}
					else
					{
						base->getItems()->addItem(type);
						_manageContainment = base->getAvailableContainment()
												- (base->getUsedContainment() * _containmentLimit) < 0;
					}
				}
				else
				{
					base->getItems()->addItem(corpseItem);
				}
			}
			else if (origFaction == FACTION_NEUTRAL)
			{
				if (aborted || soldierLive == 0) // if mission fails, all civilians die
				{
					addStat(
						"STR_CIVILIANS_KILLED_BY_ALIENS",
						1,
						-value); // kL
				}
				else
				{
					addStat(
						"STR_CIVILIANS_SAVED",
						1,
						value); // kL_note: duplicated below.
				}
			}
		}
	}

	if (craft != 0
		&& ((soldierExit == 0 && aborted)
				|| soldierLive == 0))
	{
		addStat(
			"STR_XCOM_CRAFT_LOST",
			1,
			-craft->getRules()->getScore());

		for (std::vector<Soldier*>::iterator
				i = base->getSoldiers()->begin();
				i != base->getSoldiers()->end();
				)
		{
			if ((*i)->getCraft() == craft)
			{
				delete *i;
				i = base->getSoldiers()->erase(i);
			}
			else
			{
				 ++i;
			}
		}

		// Since this is not a base defense mission, we can safely erase the craft,
		// without worrying its vehicles' destructor calling double (on base defense missions
		// all vehicle objects in the craft are also referenced by base->getVehicles() !!)
		delete craft;

		craft = 0; // To avoid a crash down there!!
		base->getCrafts()->erase(craftIterator);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return;
	}

	if (aborted
		&& mission == "STR_BASE_DEFENSE"
		&& !base->getCrafts()->empty())
	{
		for (std::vector<Craft*>::iterator
				i = base->getCrafts()->begin();
				i != base->getCrafts()->end();
				++i)
		{
			addStat(
				"STR_XCOM_CRAFT_LOST",
				1,
				-(*i)->getRules()->getScore());
		}
	}


	if ((!aborted || success)	// RECOVER UFO:
		&& soldierLive > 0)		// Run through all tiles to recover UFO components and items.
	{
		if (mission == "STR_BASE_DEFENSE")
		{
			_txtTitle->setText(tr("STR_BASE_IS_SAVED"));
		}
		else if (mission == "STR_TERROR_MISSION")
		{
			_txtTitle->setText(tr("STR_ALIENS_DEFEATED"));
		}
		else if (mission == "STR_ALIEN_BASE_ASSAULT"
			|| mission == "STR_MARS_CYDONIA_LANDING"
			|| mission == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_DESTROYED"));
		}
		else
		{
			_txtTitle->setText(tr("STR_UFO_IS_RECOVERED"));
		}

		if (!aborted)
		{
			// get recoverable map data objects from the battlescape map
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				for (int
						part = 0;
						part < 4;
						part++)
				{
					if (battle->getTiles()[i]->getMapData(part))
					{
						switch (battle->getTiles()[i]->getMapData(part)->getSpecialType())
						{
							case UFO_POWER_SOURCE:
								addStat(
									"STR_UFO_POWER_SOURCE",
									1,
									25); // kL
							break;
							case UFO_NAVIGATION:
								addStat(
									"STR_UFO_NAVIGATION",
									1,
									5);
							break;
							case UFO_CONSTRUCTION:
								addStat(
									"STR_UFO_CONSTRUCTION",
									1,
									2);
							break;
							case ALIEN_FOOD:
								addStat(
									"STR_ALIEN_FOOD",
									1,
									1); // kL
							break;
							case ALIEN_REPRODUCTION:
								addStat(
									"STR_ALIEN_REPRODUCTION",
									1,
									3); // kL
							break;
							case ALIEN_ENTERTAINMENT:
								addStat(
									"STR_ALIEN_ENTERTAINMENT",
									1,
									1); // kL
							break;
							case ALIEN_SURGERY:
								addStat(
									"STR_ALIEN_SURGERY",
									1,
									2);
							break;
							case EXAM_ROOM:
								addStat(
									"STR_EXAMINATION_ROOM",
									1,
									2);
							break;
							case ALIEN_ALLOYS:
								addStat(
									"STR_ALIEN_ALLOYS",
									1,
									1);
							break;
							case ALIEN_HABITAT:
								addStat(
									"STR_ALIEN_HABITAT",
									1,
									1);
							break;
							case MUST_DESTROY: // this is the brain!
							break;

							default:
							break;
						}
					}
				}

				recoverItems( // recover items from the floor
						battle->getTiles()[i]->getInventory(),
						base);
			}
		}
		else
		{
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)
					&& (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT))
				{
					recoverItems(
							battle->getTiles()[i]->getInventory(),
							base);
				}
			}
		}
	}
	else // mission FAIL.
	{
		if (mission == "STR_BASE_DEFENSE")
		{
			_txtTitle->setText(tr("STR_BASE_IS_LOST"));
			_destroyBase = true;
		}
		else if (mission == "STR_TERROR_MISSION")
		{
			_txtTitle->setText(tr("STR_TERROR_CONTINUES"));
		}
		else if (mission == "STR_ALIEN_BASE_ASSAULT"
			|| mission == "STR_MARS_CYDONIA_LANDING"
			|| mission == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_STILL_INTACT"));
		}
		else
		{
			_txtTitle->setText(tr("STR_UFO_IS_NOT_RECOVERED"));
		}

		if (soldierLive > 0
			&& !_destroyBase)
		{
			for (int // recover items from the ground
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)
					&& (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT))
				{
					recoverItems(
							battle->getTiles()[i]->getInventory(),
							base);
				}
			}
		}
	}

	// calculate the clips for each type based on the recovered rounds.
	for (std::map<RuleItem*, int>::const_iterator
			i = _rounds.begin();
			i != _rounds.end();
			++i)
	{
		int total_clips = i->second / i->first->getClipSize();
		if (total_clips > 0)
			base->getItems()->addItem(i->first->getType(), total_clips);
	}

	// recover all our goodies
	if (soldierLive > 0)
	{
		for (std::vector<DebriefingStat*>::iterator
				i = _stats.begin();
				i != _stats.end();
				++i)
		{
			// alien alloys recovery values are divided by 10 or divided by 150 in case of an alien base
			if ((*i)->item == "STR_ALIEN_ALLOYS")
			{
				int alloy = 10;
				if (mission == "STR_ALIEN_BASE_ASSAULT")
					alloy = 150;

				(*i)->qty = (*i)->qty / alloy;
				(*i)->score = (*i)->score / alloy;
			}

			// recoverable battlescape tiles are now converted to items and put in base inventory
			if ((*i)->recovery
				&& (*i)->qty > 0)
			{
				base->getItems()->addItem(
										(*i)->item,
										(*i)->qty);
			}
		}
	}

	// reequip craft after a non-base-defense mission;
	// of course only if it's not lost already, that case craft=0
	if (craft)
	{
		reequipCraft(
					base,
					craft,
					true);
	}
//	else // kL, maybe...
	// reequip crafts (only which is on the base) after a base defense mission;
	// we MUST check the missionType here, to avoid non-base-defense missions case
	if (mission == "STR_BASE_DEFENSE"
		&& !_destroyBase)
	{
		for (std::vector<Craft*>::iterator
				c = base->getCrafts()->begin();
				c != base->getCrafts()->end();
				++c)
		{
			if ((*c)->getStatus() != "STR_OUT")
			{
				reequipCraft(
							base,
							*c,
							false);
			}
		}

		// Clearing base->getVehicles() objects, they don't needed anymore.
		for (std::vector<Vehicle*>::iterator
				i = base->getVehicles()->begin();
				i != base->getVehicles()->end();
				++i)
		{
			delete *i;
		}

		base->getVehicles()->clear();
	}

	if (_destroyBase
		&& save->getMonthsPassed() != -1)
	{
		for (std::vector<Base*>::iterator
				i = save->getBases()->begin();
				i != save->getBases()->end();
				++i)
		{
			if (*i == base)
			{
				delete *i;
				save->getBases()->erase(i);

				break;
			}
		}

		AlienMission* am = save->getAlienMission(
											_region->getRules()->getType(),
											"STR_ALIEN_RETALIATION");
		for (std::vector<Ufo*>::iterator
				i = save->getUfos()->begin();
				i != save->getUfos()->end();
				)
		{
			if ((*i)->getMission() == am)
			{
				delete *i;

				i = save->getUfos()->erase(i);
			}
			else
				++i;
		}

		for (std::vector<AlienMission*>::iterator
				i = save->getAlienMissions().begin();
				i != save->getAlienMissions().end();
				++i)
		{
//kL			if ((AlienMission*)(*i) == am)
			if (dynamic_cast<AlienMission*>(*i) == am) // kL
			{
				delete *i;
				save->getAlienMissions().erase(i);

				break;
			}
		}
	}

	//Log(LOG_INFO) << "DebriefingState::prepareDebriefing() EXIT";
}

/**
 * Reequips a craft after a mission.
 * @param base, Base to reequip from.
 * @param craft, Craft to reequip.
 * @param vehicleItemsCanBeDestroyed, Whether we can destroy the vehicles on the craft.
 */
void DebriefingState::reequipCraft(
		Base* base,
		Craft* craft,
		bool vehicleItemsCanBeDestroyed)
{
	//Log(LOG_INFO) << "DebriefingState::reequipCraft()";

	std::map<std::string, int> craftItems = *craft->getItems()->getContents();
	for (std::map<std::string, int>::iterator
			i = craftItems.begin();
			i != craftItems.end();
			++i)
	{
		int qty = base->getItems()->getItem(i->first);
		if (qty >= i->second)
		{
			base->getItems()->removeItem(i->first, i->second);
		}
		else
		{
			int missing = i->second - qty;

			base->getItems()->removeItem(i->first, qty);
			craft->getItems()->removeItem(i->first, missing);

			ReequipStat stat =
			{
				i->first,
				missing,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}
	}


	// Now let's see the vehicles
	ItemContainer craftVehicles;
	for (std::vector<Vehicle*>::iterator
			i = craft->getVehicles()->begin();
			i != craft->getVehicles()->end();
			++i)
	{
		craftVehicles.addItem((*i)->getRules()->getType());
	}

	// Now we know how many vehicles (separated by types) we have to read.
	// Erase the current vehicles, because we have to
	// reAdd them ('cause we want to redistribute their ammo)
	if (vehicleItemsCanBeDestroyed)
	{
		for (std::vector<Vehicle*>::iterator
				i = craft->getVehicles()->begin();
				i != craft->getVehicles()->end();
				++i)
		{
			delete *i;
		}
	}

	craft->getVehicles()->clear();

	for (std::map<std::string, int>::iterator // Ok, now read those vehicles
			i = craftVehicles.getContents()->begin();
			i != craftVehicles.getContents()->end();
			++i)
	{
		int qty = base->getItems()->getItem(i->first);
		int canBeAdded = std::min(qty, i->second);

		if (qty < i->second)
		{
			int missing = i->second - qty; // missing tanks
			ReequipStat stat =
			{
				i->first,
				missing,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}


		RuleItem* tankRule = _game->getRuleset()->getItem(i->first);
		Unit* tankUnit = _game->getRuleset()->getUnit(tankRule->getType());

		int size = 4;
		if (tankUnit)
		{
			size = _game->getRuleset()->getArmor(tankUnit->getArmor())->getSize();
			size *= size;
		}

		if (tankRule->getCompatibleAmmo()->empty()) // so this tank does NOT require ammo
		{
			for (int
					j = 0;
					j < canBeAdded;
					++j)
			{
				craft->getVehicles()->push_back(new Vehicle(
														tankRule,
														tankRule->getClipSize(),
														size));
			}

			base->getItems()->removeItem(i->first, canBeAdded);
		}
		else
		{
			// so this tank requires ammo
			RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());

			int baQty = base->getItems()->getItem(ammoRule->getType()); // Ammo Quantity for this vehicle-type on the base
			if (baQty < i->second * ammoRule->getClipSize())
			{
				// missing ammo
				int missing = (i->second * ammoRule->getClipSize()) - baQty;
				ReequipStat stat =
				{
					ammoRule->getType(), missing, craft->getName(_game->getLanguage())
				};

				_missingItems.push_back(stat);
			}

			canBeAdded = std::min(canBeAdded, baQty);
			if (canBeAdded > 0)
			{
				int newAmmoPerVehicle = std::min(baQty / canBeAdded, ammoRule->getClipSize());
				int remainder = 0;
				if (ammoRule->getClipSize() > newAmmoPerVehicle)
					remainder = baQty - (canBeAdded * newAmmoPerVehicle);

				int newAmmo;
				for (int
						j = 0;
						j < canBeAdded;
						++j)
				{
					newAmmo = newAmmoPerVehicle;
					if (j < remainder)
					{
						++newAmmo;
					}

					craft->getVehicles()->push_back(new Vehicle(
															tankRule,
															newAmmo,
															size));
					base->getItems()->removeItem(
											ammoRule->getType(),
											newAmmo);
				}

				base->getItems()->removeItem(
											i->first,
											canBeAdded);
			}
		}
	}

	//Log(LOG_INFO) << "DebriefingState::reequipCraft() EXIT";
}

/**
 * Recovers items from the battlescape.
 *
 * Converts the battlescape inventory into a geoscape itemcontainer.
 * @param from, Items recovered from the battlescape.
 * @param base, Base to add items to.
 */
void DebriefingState::recoverItems(
		std::vector<BattleItem*>* from,
		Base* base)
{
	//Log(LOG_INFO) << "DebriefingState::recoverItems()";

	for (std::vector<BattleItem*>::iterator
			it = from->begin();
			it != from->end();
			++it)
	{
		if ((*it)->getRules()->getName() == "STR_ELERIUM_115")
		{
			// special case of an item counted as a stat
			addStat(
				"STR_ELERIUM_115",
				50,
				100); // kL
		}
		else
		{
			if ((*it)->getRules()->getRecoveryPoints()
				&& !(*it)->getXCOMProperty())
			{
				if ((*it)->getRules()->getBattleType() == BT_CORPSE
					&& (*it)->getUnit()->getStatus() == STATUS_DEAD)
				{
					addStat(
						"STR_ALIEN_CORPSES_RECOVERED",
						1,
						(*it)->getUnit()->getValue() / 5); // kL

					base->getItems()->addItem((*it)->getRules()->getName());
				}
				else if ((*it)->getRules()->getBattleType() == BT_CORPSE
					&& (*it)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					if ((*it)->getUnit()->getOriginalFaction() == FACTION_HOSTILE)
					{
						addStat(
							"STR_LIVE_ALIENS_RECOVERED",
							1,
							(*it)->getUnit()->getValue() * 2); // kL, duplicated above.

						if (_game->getSavedGame()->isResearchAvailable(
								_game->getRuleset()->getResearch((*it)->getUnit()->getType()),
								_game->getSavedGame()->getDiscoveredResearch(),
								_game->getRuleset()))
						{
							addStat( // more points if it's not researched
								"STR_LIVE_ALIENS_RECOVERED",
								0,
								(*it)->getUnit()->getValue() * 3); // kL, duplicated above.

							if (base->getAvailableContainment() == 0)
							{
								_noContainment = true;
								base->getItems()->addItem((*it)->getUnit()->getArmor()->getCorpseGeoscape());
							}
							else
							{
								base->getItems()->addItem((*it)->getUnit()->getType());
								_manageContainment = base->getAvailableContainment()
														- (base->getUsedContainment() * _containmentLimit) < 0;
							}
						}
						else
						{
							base->getItems()->addItem((*it)->getUnit()->getArmor()->getCorpseGeoscape());
						}
					}
					else if ((*it)->getUnit()->getOriginalFaction() == FACTION_NEUTRAL)
					{
						addStat(
							"STR_CIVILIANS_SAVED",
							1,
							(*it)->getUnit()->getValue()); // kL_note: duplicated above.
					}
				}
				else if (!_game->getSavedGame()->isResearched((*it)->getRules()->getType()))
				{
					addStat( // add pts. for unresearched items only
						"STR_ALIEN_ARTIFACTS_RECOVERED",
						1,
						(*it)->getRules()->getRecoveryPoints());
				}
			}

			if ((*it)->getRules()->isRecoverable()
				&& !(*it)->getRules()->isFixed())
			{
				// put items back in the base
				switch ((*it)->getRules()->getBattleType())
				{
					case BT_CORPSE:
					break;
					case BT_AMMO: // it's a clip, count rounds left
						_rounds[(*it)->getRules()] += (*it)->getAmmoQuantity();
					break;
					case BT_FIREARM:
					case BT_MELEE: // it's a weapon, count rounds in clip
					{
						BattleItem* clip = (*it)->getAmmoItem();
						if (clip
							&& clip->getRules()->getClipSize() > 0)
						{
							_rounds[clip->getRules()] += clip->getAmmoQuantity();
						}
					}

					default: // fall-through, recover the weapon itself
						base->getItems()->addItem((*it)->getRules()->getType());
					break;
				}
			}
		}
	}

	//Log(LOG_INFO) << "DebriefingState::recoverItems() EXIT";
}

}
