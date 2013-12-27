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
		statsDy = 0,
		recoveryDy = 0;

	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->qty == 0)
			continue;


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
			recoveryDy += 8;
		}
		else
		{
			_lstStats->addRow(
							3,
							tr((*i)->item).c_str(),
							ss.str().c_str(),
							ss2.str().c_str());
			statsDy += 8;
		}
	}

	std::wstringstream ss3;
	ss3 << total;
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					ss3.str().c_str());

	if (recoveryDy > 0)
	{
		_txtRecovery->setY(_lstStats->getY() + statsDy + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recoveryDy + 5);
	}
	else
	{
		_txtRecovery->setText(L"");
		_lstTotal->setY(_lstStats->getY() + statsDy + 5);
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
	if (total <= -100)
	{
		rating = tr("STR_RATING_TERRIBLE");
	}
	else if (total <= 50)
	{
		rating = tr("STR_RATING_POOR");
	}
	else if (total <= 250)
	{
		rating = tr("STR_RATING_OK");
	}
	else if (total <= 500)
	{
		rating = tr("STR_RATING_GOOD");
	}
	else if (total <= 900)
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
			_game->pushState(new CannotReequipState(_game, _missingItems));
		}

		if (_noContainment)
		{
			_game->pushState(new NoContainmentState(_game));
		}
		else if (_manageContainment)
		{
			_game->pushState(new ManageAlienContainmentState(_game, _base, OPT_BATTLESCAPE));
			_game->pushState(new ErrorMessageState(
					_game, tr("STR_CONTAINMENT_EXCEEDED")
					.arg(_base->getName()).c_str(), Palette::blockOffset(8)+5, "BACK01.SCR", 0));
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
void DebriefingState::addStat(const std::string& name, int quantity, int score)
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

	_stats.push_back(new DebriefingStat("STR_ALIENS_KILLED", false));
	_stats.push_back(new DebriefingStat("STR_ALIEN_CORPSES_RECOVERED", false));
	_stats.push_back(new DebriefingStat("STR_LIVE_ALIENS_RECOVERED", false));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ARTIFACTS_RECOVERED", false));
	_stats.push_back(new DebriefingStat("STR_ALIEN_BASE_CONTROL_DESTROYED", false));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_ALIENS", false));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES", false));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_SAVED", false));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_KILLED", false));
//	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_RETIRED_THROUGH_INJURY", false));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION", false));
	_stats.push_back(new DebriefingStat("STR_TANKS_DESTROYED", false));
	_stats.push_back(new DebriefingStat("STR_XCOM_CRAFT_LOST", false));
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

	int playerInExitArea = 0;	// if this stays 0 the craft is lost...
	int playersSurvived = 0;	// if this stays 0 the craft is lost...
	int playersUnconscious = 0;

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
				for (std::vector<Region*>::iterator
						k = _game->getSavedGame()->getRegions()->begin();
						k != _game->getSavedGame()->getRegions()->end();
						++k)
				{
					if ((*k)->getRules()->insideRegion((*j)->getLongitude(), (*j)->getLatitude()))
					{
						_region = *k;

						break;
					}
				}

				for (std::vector<Country*>::iterator
						k = _game->getSavedGame()->getCountries()->begin();
						k != _game->getSavedGame()->getCountries()->end();
						++k)
				{
					if ((*k)->getRules()->insideCountry((*j)->getLongitude(), (*j)->getLatitude()))
					{
						_country = *k;

						break;
					}
				}

				craft = *j;
				base = *i;
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

			for (std::vector<Region*>::iterator
					k = _game->getSavedGame()->getRegions()->begin();
					k != _game->getSavedGame()->getRegions()->end();
					++k)
			{
				if ((*k)->getRules()->insideRegion((*i)->getLongitude(), (*i)->getLatitude()))
				{
					_region = *k;

					break;
				}
			}

			for (std::vector<Country*>::iterator
					k = _game->getSavedGame()->getCountries()->begin();
					k != _game->getSavedGame()->getCountries()->end();
					++k)
			{
				if ((*k)->getRules()->insideCountry((*i)->getLongitude(), (*i)->getLatitude()))
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

	// lets see what happens with units

	// first, we evaluate how many surviving XCom units there are, and how many are conscious
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
				playersUnconscious++;
			}

			playersSurvived++;
		}
	}
	// if all our men are unconscious, the aliens get to have their way with them.
	if (playersUnconscious == playersSurvived)
	{
		playersSurvived = 0;
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

	// alien base disappears (if you didn't abort)
	if (battle->getMissionType() == "STR_ALIEN_BASE_ASSAULT")
	{
		_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));
		bool destroyAlienBase = true;

		if (aborted
			|| playersSurvived == 0)
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
//kL					addStat("STR_ALIEN_BASE_CONTROL_DESTROYED", 1, 500);
					addStat("STR_ALIEN_BASE_CONTROL_DESTROYED", 1, 350);		// kL

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

	// lets see what happens with units

	// first, we evaluate how many surviving XCom units there are, and how many are conscious
/*	for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
	{
		if ((*j)->getOriginalFaction() == FACTION_PLAYER
			&& (*j)->getStatus() != STATUS_DEAD)
		{
			if ((*j)->getStatus() == STATUS_UNCONSCIOUS
				|| (*j)->getFaction() == FACTION_HOSTILE)
			{
				playersUnconscious++;
			}

			playersSurvived++;
		}
	}

	// if all our men are unconscious, the aliens get to have their way with them.
	if (playersUnconscious == playersSurvived)
	{
		playersSurvived = 0;
		for (std::vector<BattleUnit*>::iterator j = battle->getUnits()->begin(); j != battle->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() == FACTION_PLAYER
				&& (*j)->getStatus() != STATUS_DEAD)
			{
				(*j)->instaKill();
			}
		}
	} */

	// time to care for units.
	for (std::vector<BattleUnit*>::iterator
			j = battle->getUnits()->begin();
			j != battle->getUnits()->end();
			++j)
	{
		UnitStatus status = (*j)->getStatus();
		UnitFaction faction = (*j)->getFaction();
		UnitFaction origFaction = (*j)->getOriginalFaction();

		int value = (*j)->getValue();
		Soldier* soldier = save->getSoldier((*j)->getId());
		std::string type = (*j)->getType();

		if ((*j)->getSpawnUnit() != "")
		{
			type = (*j)->getSpawnUnit();
		}

		if (!(*j)->getTile())
		{
			(*j)->setTile(battle->getTile((*j)->getPosition()));
		}

		if (status == STATUS_DEAD) // so this is a dead unit
		{
			if (origFaction == FACTION_HOSTILE
				&& (*j)->killedBy() == FACTION_PLAYER)
			{
				addStat("STR_ALIENS_KILLED", 1, value);
			}
			else if (origFaction == FACTION_PLAYER)
			{
				if (soldier != 0)
				{
					addStat("STR_XCOM_OPERATIVES_KILLED", 1, -value);

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
				else // non soldier player = tank
				{
					addStat("STR_TANKS_DESTROYED", 1, -value);
				}
			}
			else if (origFaction == FACTION_NEUTRAL)
			{
				if ((*j)->killedBy() == FACTION_PLAYER)
				{
//kL					addStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES", 1, -(*j)->getValue() - (*j)->getValue() * 2 / 3);
					addStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES", 1, -(value * 2));	// kL
				}
				else
				{
					// if civilians happen to kill themselves XCOM shouldn't get penalty for it
//kL					addStat("STR_CIVILIANS_KILLED_BY_ALIENS", 1, -(*j)->getValue());
					addStat("STR_CIVILIANS_KILLED_BY_ALIENS", 1, -value);	// kL
				}
			}
		}
		else // so this unit is not dead...
		{
			if (origFaction == FACTION_PLAYER)
			{
				if (((*j)->isInExitArea()
						&& (battle->getMissionType() != "STR_BASE_DEFENSE" || success))
					|| !aborted)
				{
					// so game is not aborted or aborted and unit is on exit area
					(*j)->postMissionProcedures(save);

					playerInExitArea++;

					if (soldier != 0)
						recoverItems((*j)->getInventory(), base);
					else // non soldier player = tank
					{
						base->getItems()->addItem(type);

						RuleItem* tankRule = _game->getRuleset()->getItem(type);
						BattleItem* ammoItem = (*j)->getItem("STR_RIGHT_HAND")->getAmmoItem();
						if (!tankRule->getCompatibleAmmo()->empty()
							&& 0 != ammoItem
							&& 0 < ammoItem->getAmmoQuantity())
						{
							base->getItems()->addItem(tankRule->getCompatibleAmmo()->front(), ammoItem->getAmmoQuantity());
						}
					}
				}
				else // so game is aborted and unit is not on exit area
				{
					addStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION", 1, -value);

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
				&& !(*j)->isOut(true, true))
			{
				for (std::vector<BattleItem*>::iterator
						k = (*j)->getInventory()->begin();
						k != (*j)->getInventory()->end();
						++k)
				{
					if (!(*k)->getRules()->isFixed())
					{
						(*j)->getTile()->addItem(*k, _game->getRuleset()->getInventory("STR_GROUND"));
					}
				}

				std::string corpseItem = (*j)->getArmor()->getCorpseGeoscape();
				if ((*j)->getSpawnUnit() != "")
				{
					corpseItem = _game->getRuleset()->getArmor(_game->getRuleset()->getUnit((*j)->getSpawnUnit())->getArmor())->getCorpseGeoscape();
				}

//kL				addStat("STR_LIVE_ALIENS_RECOVERED", 1, 10); // 10 points for recovery
//				addStat("STR_LIVE_ALIENS_RECOVERED", 1, (*j)->getValue());	// kL, duplicated in function below.
				addStat("STR_LIVE_ALIENS_RECOVERED", 1, value * 2);

				if (_game->getSavedGame()->isResearchAvailable(
						_game->getRuleset()->getResearch(type),
						_game->getSavedGame()->getDiscoveredResearch(),
						_game->getRuleset()))
				{
//kL					addStat("STR_LIVE_ALIENS_RECOVERED", 0, ((*j)->getValue() * 2) - 10); // more points if it's not researched
//					addStat("STR_LIVE_ALIENS_RECOVERED", 0, (*j)->getValue() * 3 / 2);	// kL, duplicated in function below.
					addStat("STR_LIVE_ALIENS_RECOVERED", 0, value * 3);
					if (base->getAvailableContainment() == 0)
					{
						_noContainment = true;
//kL						base->getItems()->addItem(corpseItem, 1);
						base->getItems()->addItem(corpseItem);
					}
					else
					{
//kL						base->getItems()->addItem(type, 1);
						base->getItems()->addItem(type);
						_manageContainment = base->getAvailableContainment() - (base->getUsedContainment() * _containmentLimit) < 0;
					}
				}
				else
				{
//kL					base->getItems()->addItem(corpseItem, 1);
					base->getItems()->addItem(corpseItem);
				}
			}
			else if (origFaction == FACTION_NEUTRAL)
			{
				if (aborted || playersSurvived == 0) // if mission fails, all civilians die
				{
//kL					addStat("STR_CIVILIANS_KILLED_BY_ALIENS", 1, -(*j)->getValue());
					addStat("STR_CIVILIANS_KILLED_BY_ALIENS", 1, -value);		// kL
				}
				else
				{
//kL					addStat("STR_CIVILIANS_SAVED", 1, (*j)->getValue());	// kL_note: duplicated below.
					addStat("STR_CIVILIANS_SAVED", 1, value);
				}
			}
		}
	}

	if (craft != 0
		&& ((playerInExitArea == 0 && aborted)
				|| playersSurvived == 0))
	{
		addStat("STR_XCOM_CRAFT_LOST", 1, -craft->getRules()->getScore());

		for (std::vector<Soldier*>::iterator
				i = base->getSoldiers()->begin();
				i != base->getSoldiers()->end();
				)
		{
			if ((*i)->getCraft() == craft)
			{
				delete (*i);
				i = base->getSoldiers()->erase(i);
			}
			else
			{
				 ++i;
			}
		}

		// Since this is not a base defense mission, we can safely erase the craft,
		// without worrying its vehicles' destructor calling double (on base defense missions
		// all vehicle object in the craft is also referenced by base->getVehicles() !!)
		delete craft;

		craft = 0; // To avoid a crash down there!!
		base->getCrafts()->erase(craftIterator);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return;
	}

	if (aborted
		&& battle->getMissionType() == "STR_BASE_DEFENSE"
		&& !base->getCrafts()->empty())
	{
		for (std::vector<Craft*>::iterator
				i = base->getCrafts()->begin();
				i != base->getCrafts()->end();
				++i)
		{
			addStat("STR_XCOM_CRAFT_LOST", 1, -(*i)->getRules()->getScore());
		}
	}

	if ((!aborted || success)
		&& playersSurvived > 0) // RECOVER UFO : run through all tiles to recover UFO components and items
	{
		if (battle->getMissionType() == "STR_BASE_DEFENSE")
		{
			_txtTitle->setText(tr("STR_BASE_IS_SAVED"));
		}
		else if (battle->getMissionType() == "STR_TERROR_MISSION")
		{
			_txtTitle->setText(tr("STR_ALIENS_DEFEATED"));
		}
		else if (battle->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
			|| battle->getMissionType() == "STR_MARS_CYDONIA_LANDING"
			|| battle->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
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
//kL								addStat("STR_UFO_POWER_SOURCE", 1, 20);
								addStat("STR_UFO_POWER_SOURCE", 1, 25);		// kL
							break;
							case UFO_NAVIGATION:
								addStat("STR_UFO_NAVIGATION", 1, 5);
							break;
							case UFO_CONSTRUCTION:
								addStat("STR_UFO_CONSTRUCTION", 1, 2);
							break;
							case ALIEN_FOOD:
//kL								addStat("STR_ALIEN_FOOD", 1, 2);
								addStat("STR_ALIEN_FOOD", 1, 1);		// kL
							break;
							case ALIEN_REPRODUCTION:
//kL								addStat("STR_ALIEN_REPRODUCTION", 1, 2);
								addStat("STR_ALIEN_REPRODUCTION", 1, 3);	// kL
							break;
							case ALIEN_ENTERTAINMENT:
//kL								addStat("STR_ALIEN_ENTERTAINMENT", 1, 2);
								addStat("STR_ALIEN_ENTERTAINMENT", 1, 1);	// kL
							break;
							case ALIEN_SURGERY:
								addStat("STR_ALIEN_SURGERY", 1, 2);
							break;
							case EXAM_ROOM:
								addStat("STR_EXAMINATION_ROOM", 1, 2);
							break;
							case ALIEN_ALLOYS:
								addStat("STR_ALIEN_ALLOYS", 1, 1);
							break;
							case ALIEN_HABITAT:
								addStat("STR_ALIEN_HABITAT", 1, 1);
							break;
							case MUST_DESTROY: // this is the brain
							break;

							default:
							break;
						}
					}
				}

				recoverItems(battle->getTiles()[i]->getInventory(), base); // recover items from the floor
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
					recoverItems(battle->getTiles()[i]->getInventory(), base);
				}
			}
		}
	}
	else
	{
		if (battle->getMissionType() == "STR_BASE_DEFENSE")
		{
			_txtTitle->setText(tr("STR_BASE_IS_LOST"));
			_destroyBase = true;
		}
		else if (battle->getMissionType() == "STR_TERROR_MISSION")
		{
			_txtTitle->setText(tr("STR_TERROR_CONTINUES"));
		}
		else if (battle->getMissionType() == "STR_ALIEN_BASE_ASSAULT"
			|| battle->getMissionType() == "STR_MARS_CYDONIA_LANDING"
			|| battle->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_STILL_INTACT"));
		}
		else
		{
			_txtTitle->setText(tr("STR_UFO_IS_NOT_RECOVERED"));
		}

		if (playersSurvived > 0
			&& !_destroyBase)
		{
			// recover items from the ground
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)
					&& (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT))
				{
					recoverItems(battle->getTiles()[i]->getInventory(), base);
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
	if (playersSurvived > 0)
	{
		int aadivider = battle->getMissionType() == "STR_ALIEN_BASE_ASSAULT"? 150: 10;
		for (std::vector<DebriefingStat*>::iterator
				i = _stats.begin();
				i != _stats.end();
				++i)
		{
			// alien alloys recovery values are divided by 10 or divided by 150 in case of an alien base
			if ((*i)->item == "STR_ALIEN_ALLOYS")
			{
				(*i)->qty = (*i)->qty / aadivider;
				(*i)->score = (*i)->score / aadivider;
			}

			// recoverable battlescape tiles are now converted to items and put in base inventory
			if ((*i)->recovery
				&& (*i)->qty > 0)
			{
				base->getItems()->addItem((*i)->item, (*i)->qty);
			}
		}
	}

	// reequip craft after a non-base-defense mission (of course only if it's not lost already (that case craft=0))
	if (craft)
	{
		reequipCraft(base, craft, true);
	}

	// reequip crafts (only which is on the base) after a base defense mission;
	// we MUST check the missionType here, to avoid non-base-defense missions case
	if (battle->getMissionType() == "STR_BASE_DEFENSE"
		&& !_destroyBase)
	{
		for (std::vector<Craft*>::iterator
				c = base->getCrafts()->begin();
				c != base->getCrafts()->end();
				++c)
		{
			if ((*c)->getStatus() != "STR_OUT")
				reequipCraft(base, *c, false);
		}

		// Clearing base->getVehicles() objects, they don't needed anymore.
		for (std::vector<Vehicle*>::iterator
				i = base->getVehicles()->begin();
				i != base->getVehicles()->end();
				++i)
			delete (*i);

		base->getVehicles()->clear();
	}

	if (_destroyBase
		&& _game->getSavedGame()->getMonthsPassed() != -1)
	{
		for (std::vector<Base*>::iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			if ((*i) == base)
			{
				delete (*i);

				_game->getSavedGame()->getBases()->erase(i);

				break;
			}
		}

		AlienMission* am = _game->getSavedGame()->getAlienMission(
															_region->getRules()->getType(),
															"STR_ALIEN_RETALIATION");
		for (std::vector<Ufo*>::iterator
				i = _game->getSavedGame()->getUfos()->begin();
				i != _game->getSavedGame()->getUfos()->end();
				)
		{
			if ((*i)->getMission() == am)
			{
				delete *i;

				i = _game->getSavedGame()->getUfos()->erase(i);
			}
			else
			{
				++i;
			}
		}

		for (std::vector<AlienMission*>::iterator
				i = _game->getSavedGame()->getAlienMissions().begin();
				i != _game->getSavedGame()->getAlienMissions().end();
				++i)
		{
//kL			if ((AlienMission*)(*i) == am)
			if (dynamic_cast<AlienMission*>(*i) == am)		// kL
			{
				delete (*i);
				_game->getSavedGame()->getAlienMissions().erase(i);

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
					if (j < remainder) ++newAmmo;

					craft->getVehicles()->push_back(new Vehicle(
															tankRule,
															newAmmo,
															size));
					base->getItems()->removeItem(ammoRule->getType(), newAmmo);
				}

				base->getItems()->removeItem(i->first, canBeAdded);
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
//kL			addStat("STR_ELERIUM_115", 50, 5); // special case of an item counted as a stat
			addStat("STR_ELERIUM_115", 50, 100);	// kL
		}
		else
		{
			if ((*it)->getRules()->getRecoveryPoints()
				&& !(*it)->getXCOMProperty())
			{
				if ((*it)->getRules()->getBattleType() == BT_CORPSE
					&& (*it)->getUnit()->getStatus() == STATUS_DEAD)
				{
//kL					addStat("STR_ALIEN_CORPSES_RECOVERED", 1, (*it)->getUnit()->getValue());
					addStat("STR_ALIEN_CORPSES_RECOVERED", 1, (*it)->getUnit()->getValue() / 5);	// kL
//kL					base->getItems()->addItem((*it)->getRules()->getName(), 1);
					base->getItems()->addItem((*it)->getRules()->getName());
				}
				else if ((*it)->getRules()->getBattleType() == BT_CORPSE
					&& (*it)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					if ((*it)->getUnit()->getOriginalFaction() == FACTION_HOSTILE)
					{
//kL						addStat("STR_LIVE_ALIENS_RECOVERED", 1, 10); // 10 points for recovery
						addStat("STR_LIVE_ALIENS_RECOVERED", 1, (*it)->getUnit()->getValue() * 2);	// kL, duplicated above.

						if (_game->getSavedGame()->isResearchAvailable(
								_game->getRuleset()->getResearch((*it)->getUnit()->getType()),
								_game->getSavedGame()->getDiscoveredResearch(),
								_game->getRuleset()))
						{
//kL							addStat("STR_LIVE_ALIENS_RECOVERED", 0, ((*it)->getUnit()->getValue() * 2) - 10); // more points if it's not researched
							addStat("STR_LIVE_ALIENS_RECOVERED", 0, (*it)->getUnit()->getValue() * 3);	// kL, duplicated above.
							if (base->getAvailableContainment() == 0)
							{
								_noContainment = true;
//kL								base->getItems()->addItem((*it)->getUnit()->getArmor()->getCorpseGeoscape(), 1);
								base->getItems()->addItem((*it)->getUnit()->getArmor()->getCorpseGeoscape());
							}
							else
							{
//kL								base->getItems()->addItem((*it)->getUnit()->getType(), 1);
								base->getItems()->addItem((*it)->getUnit()->getType());
								_manageContainment = (base->getAvailableContainment() - (base->getUsedContainment() * _containmentLimit) < 0);
							}
						}
						else
						{
//kL							base->getItems()->addItem((*it)->getUnit()->getArmor()->getCorpseGeoscape(), 1);
							base->getItems()->addItem((*it)->getUnit()->getArmor()->getCorpseGeoscape());
						}
					}
					else if ((*it)->getUnit()->getOriginalFaction() == FACTION_NEUTRAL)
					{
						addStat("STR_CIVILIANS_SAVED", 1, (*it)->getUnit()->getValue());	// kL_note: duplicated above.
					}
				}
				else if (!_game->getSavedGame()->isResearched((*it)->getRules()->getType())) // only "recover" unresearched items
				{
					addStat("STR_ALIEN_ARTIFACTS_RECOVERED", 1, (*it)->getRules()->getRecoveryPoints());
				}
			}

			if ((*it)->getRules()->isRecoverable()
				&& !(*it)->getRules()->isFixed()) // put items back in the base
			{
				switch ((*it)->getRules()->getBattleType())
				{
					case BT_CORPSE:
					break;
					case BT_AMMO: // It's a clip, count any rounds left.
						_rounds[(*it)->getRules()] += (*it)->getAmmoQuantity();
					break;
					case BT_FIREARM:
					case BT_MELEE: // It's a weapon, count any rounds left in the clip.
					{
						BattleItem* clip = (*it)->getAmmoItem();
						if (clip
							&& clip->getRules()->getClipSize() > 0)
						{
							_rounds[clip->getRules()] += clip->getAmmoQuantity();
						}
					}

					default: // Fall-through, to recover the weapon itself.
//kL						base->getItems()->addItem((*it)->getRules()->getType(), 1);
						base->getItems()->addItem((*it)->getRules()->getType());
					break;
				}
			}
		}
	}

	//Log(LOG_INFO) << "DebriefingState::recoverItems() EXIT";
}

}
