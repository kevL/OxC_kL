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

#include "DebriefingState.h"

//#include <sstream>

#include "CannotReequipState.h"
#include "CommendationState.h"
#include "CommendationDeadState.h"
#include "NoContainmentState.h"
#include "PromotionsState.h"

#include "../Basescape/AlienContainmentState.h"
#include "../Basescape/SellState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Debriefing screen.
 */
DebriefingState::DebriefingState()
	:
		_rules(_game->getRuleset()),
		_savedGame(_game->getSavedGame()),
		_region(NULL),
		_country(NULL),
		_noContainment(false),
		_manageContainment(false),
		_destroyXCOMBase(false),
		_limitsEnforced(0),
		_aliensControlled(0),
		_aliensKilled(0),
		_aliensStunned(0)
{
	Options::baseXResolution = Options::baseXGeoscape;
	Options::baseYResolution = Options::baseYGeoscape;
	_game->getScreen()->resetDisplay(false);

	// Restore the cursor in case something weird happened
	_game->getCursor()->setVisible();

	// kL: Clean up the leftover states from BattlescapeGame
	// ( was done in ~BattlescapeGame, but that causes CTD under reLoad situation )
	// Now done here and in NextTurnState. (not ideal: should find a safe place
	// when BattlescapeGame is really dTor'd, and not reLoaded ...... ) uh, i guess.
//	if (_savedGame->getSavedBattle()->getBattleGame())
	_savedGame->getSavedBattle()->getBattleGame()->cleanupDeleted();


	_missionStatistics = new MissionStatistics();

	if (Options::storageLimitsEnforced == true)
		_limitsEnforced = 1;

	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(280, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 216, 8);

	_txtItem		= new Text(184, 9, 16, 24);
	_txtQuantity	= new Text(60, 9, 200, 24);
	_txtScore		= new Text(36, 9, 260, 24);

	_lstStats		= new TextList(288, 81, 16, 32);

	_lstRecovery	= new TextList(288, 81, 16, 32);
	_txtRecovery	= new Text(180, 9, 16, 60);

	_lstTotal		= new TextList(288, 9, 16, 12);

	_btnOk			= new TextButton(176, 16, 16, 177);
	_txtRating		= new Text(100, 9, 212, 180);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("debriefing")->getElement("palette")->color);

	add(_window,		"window",	"debriefing");
	add(_txtTitle,		"heading",	"debriefing");
	add(_txtBaseLabel,	"text",		"debriefing");
	add(_txtItem,		"text",		"debriefing");
	add(_txtQuantity,	"text",		"debriefing");
	add(_txtScore,		"text",		"debriefing");
	add(_txtRecovery,	"text",		"debriefing");
	add(_lstStats,		"list",		"debriefing");
	add(_lstRecovery,	"list",		"debriefing");
	add(_lstTotal,		"list",		"debriefing");
	add(_btnOk,			"button",	"debriefing");
	add(_txtRating,		"text",		"debriefing");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& DebriefingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();

	_txtItem->setText(tr("STR_LIST_ITEM"));
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
	_txtScore->setText(tr("STR_SCORE"));
	_txtRecovery->setText(tr("STR_UFO_RECOVERY"));

	_lstStats->setColumns(3, 176, 60, 36);
	_lstStats->setDot();
	_lstStats->setMargin();

	_lstRecovery->setColumns(3, 176, 60, 36);
	_lstRecovery->setDot();
	_lstRecovery->setMargin();

	_lstTotal->setColumns(2, 244, 36);
	_lstTotal->setDot();


	prepareDebriefing(); /*/ <- -- GATHER ALL DATA HERE <- < */


	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_baseLabel);


	int
		total = 0,
		stats_offY = 0,
		recov_offY = 0,
		civiliansSaved = 0,
		civiliansDead = 0;

	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->qty != 0)
		{
			std::wostringstream
				wosts1,
				wosts2;

			wosts1 << L'\x01' << (*i)->qty << L'\x01';	// quantity of recovered item Type
			wosts2 << L'\x01' << (*i)->score;			// score for items of Type
			total += (*i)->score;						// total score

			if ((*i)->recover == true)
			{
				_lstRecovery->addRow(
								3,
								tr((*i)->item).c_str(),
								wosts1.str().c_str(),
								wosts2.str().c_str());
				recov_offY += 8;
			}
			else
			{
				_lstStats->addRow(
								3,
								tr((*i)->item).c_str(),
								wosts1.str().c_str(),
								wosts2.str().c_str());
				stats_offY += 8;
			}

			if ((*i)->item == "STR_CIVILIANS_SAVED")
				civiliansSaved = (*i)->qty;

			if ((*i)->item == "STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"
				|| (*i)->item == "STR_CIVILIANS_KILLED_BY_ALIENS")
			{
				civiliansDead += (*i)->qty;
			}

			if ((*i)->item == "STR_ALIENS_KILLED")
				_aliensKilled += (*i)->qty;
		}
	}

	if (civiliansSaved > 0
		&& civiliansDead == 0
		&& _missionStatistics->success == true)
	{
		_missionStatistics->valiantCrux = true;
	}

	std::wostringstream wosts;
	wosts << total;
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					wosts.str().c_str());

	if (recov_offY > 0)
	{
		_txtRecovery->setY(_lstStats->getY() + stats_offY + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recov_offY + 5);
	}
	else
	{
		_txtRecovery->setText(L"");
		_lstTotal->setY(_lstStats->getY() + stats_offY + 5);
	}


	if (_region != NULL)
	{
		if (_destroyXCOMBase == true)
		{
			const int diff = static_cast<int>(_savedGame->getDifficulty()) + 1;
			_region->addActivityAlien(diff * 235);
			_region->recentActivity();
		}
		else
		{
			_region->addActivityXcom(total);
			_region->recentActivityXCOM();
		}
	}

	if (_country != NULL)
	{
		if (_destroyXCOMBase == true)
		{
			const int diff = static_cast<int>(_savedGame->getDifficulty()) + 1;
			_country->addActivityAlien(diff * 235);
			_country->recentActivity();
		}
		else
		{
			_country->addActivityXcom(total);
			_country->recentActivityXCOM();
		}
	}

	std::string music = OpenXcom::res_MUSIC_TAC_DEBRIEFING;

	std::wstring rating; // Calculate rating
	if (total < -99)
	{
		rating = tr("STR_RATING_TERRIBLE");
		_missionStatistics->rating = "STR_RATING_TERRIBLE";

		music = OpenXcom::res_MUSIC_TAC_DEBRIEFING_BAD;
	}
	else if (total < 101)
	{
		rating = tr("STR_RATING_POOR");
		_missionStatistics->rating = "STR_RATING_POOR";
	}
	else if (total < 351)
	{
		rating = tr("STR_RATING_OK");
		_missionStatistics->rating = "STR_RATING_OK";
	}
	else if (total < 751)
	{
		rating = tr("STR_RATING_GOOD");
		_missionStatistics->rating = "STR_RATING_GOOD";
	}
	else if (total < 1251)
	{
		rating = tr("STR_RATING_EXCELLENT");
		_missionStatistics->rating = "STR_RATING_EXCELLENT";
	}
	else
	{
		rating = tr("STR_RATING_STUPENDOUS");
		_missionStatistics->rating = "STR_RATING_EXCELLENT";
	}

	_txtRating->setText(tr("STR_RATING").arg(rating));
	_missionStatistics->score = total;


	// Soldier Diary ->
	SavedBattleGame* const battle = _savedGame->getSavedBattle();

	_missionStatistics->id = _savedGame->getMissionStatistics()->size();
	_missionStatistics->shade = battle->getGlobalShade();

	for (std::vector<BattleUnit*>::const_iterator
			i = battle->getUnits()->begin();
			i != battle->getUnits()->end();
			++i)
	{
		Soldier* soldier = (*i)->getGeoscapeSoldier();
		// NOTE: In the case of a dead soldier, this pointer is Valid but points to garbage.
		// Use that.
		if (soldier != NULL)
		{
			BattleUnitStatistics* const statistics = (*i)->getStatistics();

			int soldierAlienKills = 0;

			for (std::vector<BattleUnitKills*>::const_iterator
					j = statistics->kills.begin();
					j != statistics->kills.end();
					++j)
			{
				if ((*j)->_faction == FACTION_HOSTILE)
					++soldierAlienKills;
			}

			// NOTE re. Nike Cross:
			// This can be exploited by MC'ing a bunch of aLiens
			// while having Option "psi-control ends battle" TRUE.
			// ... Patched
			// NOTE: This can still be exploited by MC'ing and
			// executing a bunch of aLiens with a single Soldier.
			if (_aliensControlled == 0
				&& _aliensKilled + _aliensStunned > 3 + static_cast<int>(_savedGame->getDifficulty())
				&& _aliensKilled + _aliensStunned == soldierAlienKills
				&& _missionStatistics->success == true)
			{
				statistics->nikeCross = true;
			}


			if ((*i)->getStatus() == STATUS_DEAD)
			{
				soldier = NULL;	// Zero out the BattleUnit from the geoscape Soldiers list
								// in this State; it's already gone from his/her former Base.
								// This makes them ineligible for promotion.
								// PS, there is no 'geoscape Soldiers list' really; it's
								// just a variable stored in each xCom-agent/BattleUnit ....
				SoldierDead* deadSoldier;

				for (std::vector<SoldierDead*>::const_iterator
						j = _savedGame->getDeadSoldiers()->begin();
						j != _savedGame->getDeadSoldiers()->end();
						++j)
				{
					if ((*j)->getId() == (*i)->getId())
					{
						deadSoldier = *j;
						break;
					}
				}

				// note: Safety on *deadSoldier should not be needed.
				statistics->KIA = true;
				statistics->daysWounded = 0;
				_missionStatistics->injuryList[deadSoldier->getId()] = -1;

				deadSoldier->getDiary()->updateDiary(
												statistics,
												_missionStatistics,
												_rules);
				deadSoldier->getDiary()->manageAwards(_rules);

				_soldiersKIA.push_back(deadSoldier);
			}
			else
			{
				const int wounds = soldier->getWoundRecovery();
				statistics->daysWounded =
				_missionStatistics->injuryList[soldier->getId()] = wounds;

				soldier->getDiary()->updateDiary(
											statistics,
											_missionStatistics,
											_rules);

				if (soldier->getDiary()->manageAwards(_rules) == true)
					_soldiersCommended.push_back(soldier);
			}
		}
	}

	_savedGame->getMissionStatistics()->push_back(_missionStatistics); // Soldier Diary_end.


	_game->getResourcePack()->playMusic(music);
}

/**
 * dTor.
 */
DebriefingState::~DebriefingState()
{
	if (_game->isQuitting() == true)
		_savedGame->setBattleGame(NULL);

	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		delete *i;
	}

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		delete i->second;
	}
	_specialTypes.clear();

	_rounds.clear();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void DebriefingState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 863);

	std::vector<Soldier*> participants;
	for (std::vector<BattleUnit*>::const_iterator
			i = _savedGame->getSavedBattle()->getUnits()->begin();
			i != _savedGame->getSavedBattle()->getUnits()->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier() != NULL)
			participants.push_back((*i)->getGeoscapeSoldier());
	}

	_savedGame->setBattleGame(NULL);
	_game->popState();

	if (_savedGame->getMonthsPassed() == -1)
		_game->setState(new MainMenuState());
	else
	{
		if (_destroyXCOMBase == false)
		{
			bool playAwardMusic = false;

			if (_soldiersKIA.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CommendationDeadState(_soldiersKIA));
			}

			if (_soldiersCommended.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CommendationState(_soldiersCommended));
			}

			if (_savedGame->handlePromotions(participants) == true)
			{
				playAwardMusic = true;
				_game->pushState(new PromotionsState());
			}

			if (_missingItems.empty() == false)
			{
//				playAwardMusic = true;
				_game->pushState(new CannotReequipState(_missingItems));
			}

			if (_noContainment == true)
			{
//				playAwardMusic = true;
				_game->pushState(new NoContainmentState());
			}
			else if (_manageContainment == true)
			{
//				playAwardMusic = true;
				_game->pushState(new AlienContainmentState(
														_base,
														OPT_BATTLESCAPE)); // Do not allow researchHelp!
				_game->pushState(new ErrorMessageState(
													tr("STR_CONTAINMENT_EXCEEDED")
														.arg(_base->getName()).c_str(),
													_palette,
													_game->getRuleset()->getInterface("debriefing")->getElement("errorMessage")->color,
													"BACK01.SCR",
													_game->getRuleset()->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			if (_manageContainment == false
				&& Options::storageLimitsEnforced == true
				&& _base->storesOverfull() == true)
			{
//				playAwardMusic = true;
				_game->pushState(new SellState(
											_base,
											OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
													tr("STR_STORAGE_EXCEEDED")
														.arg(_base->getName()).c_str(),
													_palette,
													_game->getRuleset()->getInterface("debriefing")->getElement("errorMessage")->color,
													"BACK01.SCR",
													_game->getRuleset()->getInterface("debriefing")->getElement("errorPalette")->color));
			}

			if (playAwardMusic == true)
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_AWARDS);
			else
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
		}

		if (_savedGame->isIronman() == true) // Autosave after mission
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_AUTO_GEOSCAPE,
											_palette));
	}
}

/**
 * Adds to the debriefing stats.
 * @param name		- reference the untranslated name of the stat
 * @param score		- the score to add
 * @param quantity	- the quantity to add (default 1)
 */
void DebriefingState::addStat(
		const std::string& name,
		int score,
		int quantity)
{
	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->item == name)
		{
			(*i)->score += score;
			(*i)->qty += quantity;

			break;
		}
	}
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
		{}

		/// Clears the base if required.
		void operator()(AlienMission* am) const;
};

/**
 * Removes the association between the alien mission and the alien base if one existed.
 * @param am - pointer to the AlienMission
 */
void ClearAlienBase::operator()(AlienMission* am) const
{
	if (am->getAlienBase() == _base)
		am->setAlienBase(NULL);
}


/**
 * Prepares debriefing: gathers Aliens, Corpses, Artefacts, UFO Components.
 * Adds the items to the craft.
 * Also calculates the soldiers' experience, and possible promotions.
 * If aborted, only the things on the exit area are recovered.
 */
void DebriefingState::prepareDebriefing()
{
	for (std::vector<std::string>::const_iterator
			i = _rules->getItemsList().begin();
			i != _rules->getItemsList().end();
			++i)
	{
		if (_rules->getItem(*i)->getSpecialType() > 1)
		{
			SpecialType* const item = new SpecialType();
			item->name = *i;
			item->value = _rules->getItem(*i)->getRecoveryPoints();

			_specialTypes[_rules->getItem(*i)->getSpecialType()] = item;
		}
	}

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

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		_stats.push_back(new DebriefingStat(
										(*i).second->name,
										true));
	}
/*	_stats.push_back(new DebriefingStat("STR_UFO_POWER_SOURCE", true));
	_stats.push_back(new DebriefingStat("STR_UFO_NAVIGATION", true));
	_stats.push_back(new DebriefingStat("STR_UFO_CONSTRUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_FOOD", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_REPRODUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ENTERTAINMENT", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_SURGERY", true));
	_stats.push_back(new DebriefingStat("STR_EXAMINATION_ROOM", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ALLOYS", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_HABITAT", true)); */

	_stats.push_back(new DebriefingStat(
									_rules->getAlienFuel(),
									true));

	SavedBattleGame* const battleSave = _savedGame->getSavedBattle();

	const bool aborted = battleSave->isAborted();
	bool missionAccomplished = !aborted;

	Base* base = NULL;
	Craft* craft = NULL;

	std::vector<Craft*>::const_iterator craftIterator;


	_missionStatistics->time = *_savedGame->getTime();
	_missionStatistics->type = battleSave->getMissionType();

	int
		soldierExit = 0, // if this stays 0 the craft is lost...
		soldierLive = 0, // if this stays 0 the craft is lost...
		soldierDead = 0, // Soldier Diary.
		soldierOut = 0;

	for (std::vector<Base*>::const_iterator
			i = _savedGame->getBases()->begin();
			i != _savedGame->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator // in case there is a craft - check which craft it is about
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->isInBattlescape() == true)
			{
				const double
					craftLon = (*j)->getLongitude(),
					craftLat = (*j)->getLatitude();

				for (std::vector<Region*>::const_iterator
						k = _savedGame->getRegions()->begin();
						k != _savedGame->getRegions()->end();
						++k)
				{
					if ((*k)->getRules()->insideRegion(
													craftLon,
													craftLat) == true)
					{
						_region = *k;
						_missionStatistics->region = _region->getRules()->getType();
						break;
					}
				}

				for (std::vector<Country*>::const_iterator
						k = _savedGame->getCountries()->begin();
						k != _savedGame->getCountries()->end();
						++k)
				{
					if ((*k)->getRules()->insideCountry(
													craftLon,
													craftLat) == true)
					{
						_country = *k;
						_missionStatistics->country = _country->getRules()->getType();
						break;
					}
				}

				base = *i;
				craft = *j;
				craftIterator = j;
				craft->returnToBase();
				craft->setMissionComplete(true);
				craft->setInBattlescape(false);
			}
			else if ((*j)->getDestination() != NULL)
			{
				const Ufo* const ufo = dynamic_cast<Ufo*>((*j)->getDestination());
				if (ufo != NULL
					&& ufo->isInBattlescape() == true)
				{
					(*j)->returnToBase();
				}
			}
		}

		if ((*i)->isInBattlescape() == true) // in case this DON'T have a craft (ie. baseDefense)
		{
			base = *i;

			const double
				baseLon = base->getLongitude(),
				baseLat = base->getLatitude();

			for (std::vector<Region*>::const_iterator
					k = _savedGame->getRegions()->begin();
					k != _savedGame->getRegions()->end();
					++k)
			{
				if ((*k)->getRules()->insideRegion(
												baseLon,
												baseLat) == true)
				{
					_region = *k;
					_missionStatistics->region = _region->getRules()->getType();
					break;
				}
			}

			for (std::vector<Country*>::const_iterator
					k = _savedGame->getCountries()->begin();
					k != _savedGame->getCountries()->end();
					++k)
			{
				if ((*k)->getRules()->insideCountry(
												baseLon,
												baseLat) == true)
				{
					_country = *k;
					_missionStatistics->country = _country->getRules()->getType();
					break;
				}
			}

			if (aborted == true)
				_destroyXCOMBase = true;
			else
			{
				base->setInBattlescape(false);
				base->cleanupDefenses(false);

				bool facDestroyed = false; // not in stockcode
				for (std::vector<BaseFacility*>::const_iterator
						k = base->getFacilities()->begin();
						k != base->getFacilities()->end();
						)
				{
					if (battleSave->getModuleMap()[(*k)->getX()]
												  [(*k)->getY()].second == 0) // this facility was demolished
					{
						facDestroyed = true; // not in stockcode
						base->destroyFacility(k);
					}
					else
						++k;
				}

				if (facDestroyed == true) // kL_add <-
					base->destroyDisconnectedFacilities(); // this may cause the base to become disjointed; destroy the disconnected parts.
			}
		}
	}

	_base = base;
	_baseLabel = _base->getName(_game->getLanguage());


	// Do all aLienRace types here for SoldierDiary stat.
	if (_savedGame->getMonthsPassed() != -1)
	{
		if (battleSave->getAlienRace().empty() == false) // safety.
			_missionStatistics->alienRace = battleSave->getAlienRace();
		else
			_missionStatistics->alienRace = "STR_UNKNOWN";
	}


	for (std::vector<Ufo*>::const_iterator // UFO crash/landing site disappears
			i = _savedGame->getUfos()->begin();
			i != _savedGame->getUfos()->end();
			++i)
	{
		if ((*i)->isInBattlescape() == true)
		{
			(*i)->setInBattlescape(false);

			_missionStatistics->ufo = (*i)->getRules()->getType();

			if (aborted == true
				&& (*i)->getStatus() == Ufo::LANDED)
			{
				(*i)->setSecondsLeft(15); // UFO lifts off ...
			}
			else if (aborted == false) // successful mission ( kL: failed mission leaves UFO still crashed! )
//				|| (*i)->getStatus() == Ufo::CRASHED) // UFO can't fly
			{
				delete *i;
				_savedGame->getUfos()->erase(i); // UFO disappears.
			}

			break;
		}
	}

	for (std::vector<MissionSite*>::const_iterator // mission site disappears even when aborted
			i = _savedGame->getMissionSites()->begin();
			i != _savedGame->getMissionSites()->end();
			++i)
	{
		if ((*i)->isInBattlescape() == true)
		{
			delete *i;
			_savedGame->getMissionSites()->erase(i);

			break;
		}
	}

	// lets see what happens with units; first,
	// we evaluate how many surviving XCom units there are,
	// and if they are unconscious
	// and how many have died for commendations
	for (std::vector<BattleUnit*>::const_iterator
			i = battleSave->getUnits()->begin();
			i != battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->getStatus() != STATUS_DEAD)
		{
			if ((*i)->getStatus() == STATUS_UNCONSCIOUS
				|| (*i)->getFaction() == FACTION_HOSTILE)
			{
				++soldierOut;
			}

			++soldierLive;
		}
		else if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->getStatus() == STATUS_DEAD)
		{
			++soldierDead;
		}
	}

	// if all xCom units are unconscious the aliens get to have their way with them
	if (soldierOut == soldierLive)
	{
		soldierLive = 0;

		for (std::vector<BattleUnit*>::const_iterator
				i = battleSave->getUnits()->begin();
				i != battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_PLAYER
				&& (*i)->getStatus() != STATUS_DEAD)
			{
				(*i)->instaKill();
			}
		}
	}

	if (soldierLive == 1)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = battleSave->getUnits()->begin();
				i != battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getGeoscapeSoldier() != NULL
				&& (*i)->getStatus() != STATUS_DEAD)
			{
				// if only one soldier survived AND none have died, means only one soldier went on the mission...
				if (soldierDead == 0
					&& _aliensControlled == 0
					&& _aliensKilled + _aliensStunned > 1 + static_cast<int>(_savedGame->getDifficulty())
					&& aborted == false)
				{
					(*i)->getStatistics()->ironMan = true;
					break;
				}
				// if only one soldier survived, give him a medal! (unless he killed all the others...)
				else if ((*i)->getStatistics()->hasFriendlyFired() == false
					&& soldierDead > 0)
				{
					(*i)->getStatistics()->loneSurvivor = true;
					break;
				}
			}
		}
	}

	const std::string& mission = battleSave->getMissionType();

	// alien base disappears (if not aborted)
	if (mission == "STR_ALIEN_BASE_ASSAULT")
	{
		_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));
		bool destroyAlienBase = true;

		if (aborted == true
			|| soldierLive == 0)
		{
			for (int
					i = 0;
					i < battleSave->getMapSizeXYZ();
					++i)
			{
				if (battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT) != NULL
					&& battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getSpecialType() == UFO_NAVIGATION)
				{
					destroyAlienBase = false;
					break;
				}
			}
		}

		missionAccomplished = destroyAlienBase;

		for (std::vector<AlienBase*>::const_iterator
				i = _savedGame->getAlienBases()->begin();
				i != _savedGame->getAlienBases()->end();
				++i)
		{
			if ((*i)->isInBattlescape() == true)
			{
				if (destroyAlienBase == true)
				{
					addStat(
						"STR_ALIEN_BASE_CONTROL_DESTROYED",
						300);

					// Take care to remove supply missions for this base.
					std::for_each(
							_savedGame->getAlienMissions().begin(),
							_savedGame->getAlienMissions().end(),
							ClearAlienBase(*i));

					delete *i;
					_savedGame->getAlienBases()->erase(i);

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

	// time to care about units.
	for (std::vector<BattleUnit*>::const_iterator
			i = battleSave->getUnits()->begin();
			i != battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getTile() == NULL)								// This unit is not on a tile... give it one.
		{
			Position pos = (*i)->getPosition();
			if (pos == Position(-1,-1,-1))							// in fact, this Unit is in limbo... ie, is carried.
			{
				for (std::vector<BattleItem*>::const_iterator		// so look for its body or corpse...
						j = battleSave->getItems()->begin();
						j != battleSave->getItems()->end();
						++j)
				{
					if ((*j)->getUnit() != NULL
						&& (*j)->getUnit() == *i)					// found it: corpse is a dead or unconscious BattleUnit!!
					{
						if ((*j)->getOwner() != NULL)				// corpse of BattleUnit has an Owner (ie. is being carried by another BattleUnit)
							pos = (*j)->getOwner()->getPosition();	// Put the corpse down.. slowly.
						else if ((*j)->getTile() != NULL)			// corpse of BattleUnit is laying around somewhere
							pos = (*j)->getTile()->getPosition();	// you're not vaporized yet, Get up.
					}
				}
			}

			(*i)->setTile(battleSave->getTile(pos));
		}


		const int value = (*i)->getValue();
		const UnitFaction origFaction = (*i)->getOriginalFaction();
		const UnitStatus status = (*i)->getStatus();

		if (status == STATUS_DEAD) // so this is a dead unit
		{
			Log(LOG_INFO) << ". unitDead " << (*i)->getId() << " type = " << (*i)->getType();

			if (origFaction == FACTION_HOSTILE
				&& (*i)->killedBy() == FACTION_PLAYER)
			{
				Log(LOG_INFO) << ". . killed by xCom";

				addStat(
					"STR_ALIENS_KILLED",
					value);
			}
			else if (origFaction == FACTION_PLAYER)
			{
				const Soldier* const soldier = _savedGame->getSoldier((*i)->getId());
				if (soldier != NULL) // xCom soldier.
				{
					addStat(
						"STR_XCOM_OPERATIVES_KILLED",
						-value);

					for (std::vector<Soldier*>::const_iterator
							j = base->getSoldiers()->begin();
							j != base->getSoldiers()->end();
							++j)
					{
						if (*j == soldier) // the specific soldier at Base..
						{
							(*i)->updateGeoscapeStats(*j);

							(*j)->die(_savedGame);

							delete *j;
							base->getSoldiers()->erase(j);

							// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
							break;
						}
					}
				}
				else // not soldier -> tank
					addStat(
						"STR_TANKS_DESTROYED",
						-value);
			}
			else if (origFaction == FACTION_NEUTRAL)
			{
				if ((*i)->killedBy() == FACTION_PLAYER)
					addStat(
						"STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES",
						-(value * 2));
				else // if civilians happen to kill themselves XCOM shouldn't get penalty for it
					addStat(
						"STR_CIVILIANS_KILLED_BY_ALIENS",
						-value);
			}
		}
		else // so this unit is NOT dead...
		{
			Log(LOG_INFO) << ". unitLive " << (*i)->getId() << " type = " << (*i)->getType();

			const UnitFaction faction = (*i)->getFaction();

			std::string type;
			if ((*i)->getSpawnUnit().empty() == false)
				type = (*i)->getSpawnUnit();
			else
				type = (*i)->getType();

			if (origFaction == FACTION_PLAYER)
			{
				const Soldier* const soldier = _savedGame->getSoldier((*i)->getId());

				if (aborted == false					// so game is not aborted
					|| ((*i)->isInExitArea() == true	// or aborted and unit is on exit area
						&& (missionAccomplished == true
							|| mission != "STR_BASE_DEFENSE")))
				{
					(*i)->postMissionProcedures(_savedGame);

					++soldierExit;

					if (soldier != NULL)
					{
						recoverItems(
								(*i)->getInventory(),
								base);
//						soldier->calcStatString( // calculate new statString
//											_rules->getStatStrings(),
//											Options::psiStrengthEval
//												&& _savedGame->isResearched(_rules->getPsiRequirements()));
					}
					else // not soldier -> tank
					{
						base->getItems()->addItem(type);

						const RuleItem* itRule;
						const BattleItem* amItem;

						if ((*i)->getItem("STR_RIGHT_HAND") != NULL)
						{
							itRule = _rules->getItem(type); // (*i)->getItem("STR_RIGHT_HAND")->getRules(), as below_
							if (itRule->getCompatibleAmmo()->empty() == false)
							{
								amItem = (*i)->getItem("STR_RIGHT_HAND")->getAmmoItem();
								if (amItem != NULL
									&& amItem->getAmmoQuantity() > 0)
								{
									base->getItems()->addItem(
															itRule->getCompatibleAmmo()->front(),
															amItem->getAmmoQuantity());
								}
							}
						}

						if ((*i)->getItem("STR_LEFT_HAND") != NULL)
						{
							itRule = (*i)->getItem("STR_LEFT_HAND")->getRules();
							if (itRule->getCompatibleAmmo()->empty() == false)
							{
								amItem = (*i)->getItem("STR_LEFT_HAND")->getAmmoItem();
								if (amItem != NULL
									&& amItem->getAmmoQuantity() > 0)
								{
									base->getItems()->addItem(
															itRule->getCompatibleAmmo()->front(),
															amItem->getAmmoQuantity());
								}
							}
						}
					}
				}
				else // so game is aborted and unit is not on exit area
				{
					addStat(
						"STR_XCOM_OPERATIVES_MISSING_IN_ACTION",
						-value);

					if (soldier != NULL)
					{
						for (std::vector<Soldier*>::const_iterator
								j = base->getSoldiers()->begin();
								j != base->getSoldiers()->end();
								++j)
						{
							if (*j == soldier)
							{
								(*i)->updateGeoscapeStats(*j);
								(*i)->setStatus(STATUS_DEAD);

								(*j)->die(_savedGame);

								delete *j;
								base->getSoldiers()->erase(j);

								// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
								break;
							}
						}
					}
				}
			}
			else if (origFaction == FACTION_HOSTILE	// This section is for units still standing.
				&& faction == FACTION_PLAYER		// ie, MC'd aLiens
				&& (*i)->isOut() == false
				&& (aborted == false
					|| (*i)->isInExitArea() == true))
				// kL_note: so, this never actually runs unless early psi-exit
				// when all aliens are dead or mind-controlled is turned on ....
				// Except the two-stage Cydonia mission, in which case all this
				// does not matter.
			{
				// ADD ALIENS MC'D COUNTER HERE_kL
				++_aliensControlled;

				for (std::vector<BattleItem*>::const_iterator
						j = (*i)->getInventory()->begin();
						j != (*i)->getInventory()->end();
						++j)
				{
					if ((*j)->getRules()->isFixed() == false)
						(*i)->getTile()->addItem(
											*j,
											_rules->getInventory("STR_GROUND"));
				}

				recoverLiveAlien(
						*i,
						base);
			}
			else if (origFaction == FACTION_NEUTRAL)
			{
				if (soldierLive == 0
					|| (aborted == true						// if mission fails, all civilians die
						&& (*i)->isInExitArea() == false))	// unless they're on an Exit_Tile. kL
				{
					addStat(
						"STR_CIVILIANS_KILLED_BY_ALIENS",
						-value);
				}
				else
					addStat(
						"STR_CIVILIANS_SAVED",
						value); // duplicated below.
			}
		}
	}

	if (craft != NULL // Craft lost.
		&& (soldierLive == 0
			|| (soldierExit == 0
				&& aborted == true)))
	{
		addStat(
			"STR_XCOM_CRAFT_LOST",
			-craft->getRules()->getScore());

		for (std::vector<Soldier*>::const_iterator
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
				 ++i;
		}

		// Since this is not a base defense mission, we can safely erase the craft,
		// without worrying its vehicles' destructor calling double (on base defense missions
		// all vehicle objects in the craft are also referenced by base->getVehicles() !!)
		delete craft;

		craft = NULL; // To avoid a crash down there!! uh, not after return; it won't.
		base->getCrafts()->erase(craftIterator);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return;
	}

	if (aborted == true
		&& base->getCrafts()->empty() == false
		&& mission == "STR_BASE_DEFENSE")
	{
		for (std::vector<Craft*>::const_iterator
				i = base->getCrafts()->begin();
				i != base->getCrafts()->end();
				++i)
		{
			addStat(
				"STR_XCOM_CRAFT_LOST",
				-(*i)->getRules()->getScore());
		}
	}


	if (soldierLive > 0							// RECOVER UFO:
		&& (aborted == false
			|| missionAccomplished == true))	// Run through all tiles to recover UFO components and items.
	{
		if (mission == "STR_BASE_DEFENSE")
			_txtTitle->setText(tr("STR_BASE_IS_SAVED"));
		else if (mission == "STR_TERROR_MISSION")
			_txtTitle->setText(tr("STR_ALIENS_DEFEATED"));
		else if (mission == "STR_ALIEN_BASE_ASSAULT"
			|| mission == "STR_MARS_CYDONIA_LANDING"
			|| mission == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_DESTROYED"));
		}
		else
			_txtTitle->setText(tr("STR_UFO_IS_RECOVERED"));


		if (aborted == false)
		{
			// if this was a 2-stage mission, and not aborted (ie: there's time to clean up)
			// recover items from the earlier stages as well
			recoverItems(
					battleSave->getConditionalRecoveredItems(),
					base);

			for (size_t // get recoverable map data objects from the battlescape map
					i = 0;
					i != static_cast<size_t>(battleSave->getMapSizeXYZ());
					++i)
			{
				for (int
						part = 0;
						part != 4;
						++part)
				{
					if (battleSave->getTiles()[i]->getMapData(part))
					{
						const size_t specialType = battleSave->getTiles()[i]->getMapData(part)->getSpecialType();
						if (_specialTypes.find(specialType) != _specialTypes.end())
							addStat(
								_specialTypes[specialType]->name,
								_specialTypes[specialType]->value);
					}
				}

				recoverItems( // recover items from the floor
						battleSave->getTiles()[i]->getInventory(),
						base);
			}
		}
		else
		{
			for (size_t
					i = 0;
					i != static_cast<size_t>(battleSave->getMapSizeXYZ());
					++i)
			{
				if (battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR) != NULL
					&& (battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT))
				{
					recoverItems(
							battleSave->getTiles()[i]->getInventory(),
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
			_destroyXCOMBase = true;
		}
		else if (mission == "STR_TERROR_MISSION")
			_txtTitle->setText(tr("STR_TERROR_CONTINUES"));
		else if (mission == "STR_ALIEN_BASE_ASSAULT"
			|| mission == "STR_MARS_CYDONIA_LANDING"
			|| mission == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_STILL_INTACT"));
		}
		else
			_txtTitle->setText(tr("STR_UFO_IS_NOT_RECOVERED"));

		if (soldierLive > 0
			&& _destroyXCOMBase == false)
		{
			for (size_t // recover items from the ground
					i = 0;
					i != static_cast<size_t>(battleSave->getMapSizeXYZ());
					++i)
			{
				if (battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR) != NULL
					&& battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT)
				{
					recoverItems(
							battleSave->getTiles()[i]->getInventory(),
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
		if (i->first->getClipSize() > 0)
		{
			const int total_clips = i->second / i->first->getClipSize();
			if (total_clips > 0)
				base->getItems()->addItem(
										i->first->getType(),
										total_clips);
		}
	}

	if (soldierLive > 0) // recover all the goodies
	{
		for (std::vector<DebriefingStat*>::const_iterator
				i = _stats.begin();
				i != _stats.end();
				++i)
		{
			// alien alloys recovery values are divided by 10 or divided by 150 in case of an alien base
			if ((*i)->item == _specialTypes[ALIEN_ALLOYS]->name)
			{
				int alloys;
				if (mission == "STR_ALIEN_BASE_ASSAULT")
					alloys = 150;
				else
					alloys = 10;

				(*i)->qty = (*i)->qty / alloys;
				(*i)->score = (*i)->score / alloys;
			}

			// recoverable battlescape tiles are now converted to items and put in base inventory
			if ((*i)->recover == true
				&& (*i)->qty > 0)
			{
				base->getItems()->addItem(
										(*i)->item,
										(*i)->qty);
			}
		}

		// assuming this was a multi-stage mission
		// recover everything that was in the craft in the previous stage
		recoverItems(
				battleSave->getGuaranteedRecoveredItems(),
				base);
	}

	// reequip craft after a non-base-defense mission;
	// of course only if it's not lost already, that case craft=0
	if (craft != NULL)
		reequipCraft(
					base,
					craft,
					true);

	if (mission == "STR_BASE_DEFENSE")
	{
		if (_destroyXCOMBase == false)
		{
			// reequip crafts (only those on the base) after a base defense mission;
			for (std::vector<Craft*>::const_iterator
					i = base->getCrafts()->begin();
					i != base->getCrafts()->end();
					++i)
			{
				if ((*i)->getStatus() != "STR_OUT")
					reequipCraft(
								base,
								*i,
								false);
			}

			// clear base->getVehicles() objects, they not needed anymore.
			for (std::vector<Vehicle*>::const_iterator
					i = base->getVehicles()->begin();
					i != base->getVehicles()->end();
					++i)
			{
				delete *i;
			}

			base->getVehicles()->clear();
		}
		else if (_savedGame->getMonthsPassed() != -1)
		{
			for (std::vector<Base*>::const_iterator
					i = _savedGame->getBases()->begin();
					i != _savedGame->getBases()->end();
					++i)
			{
				if (*i == base)
				{
					delete *i;
					_savedGame->getBases()->erase(i);

					break;
				}
			}
		}

		if (_region != NULL)
		{
			const AlienMission* const am = _game->getSavedGame()->findAlienMission(
																			_region->getRules()->getType(),
																			OBJECTIVE_RETALIATION);
			for (std::vector<Ufo*>::const_iterator
					i = _savedGame->getUfos()->begin();
					i != _savedGame->getUfos()->end();
					)
			{
				if ((*i)->getMission() == am) // dynamic_cast ?
				{
					delete *i;
					i = _savedGame->getUfos()->erase(i);
				}
				else
					++i;
			}

			for (std::vector<AlienMission*>::const_iterator
					i = _savedGame->getAlienMissions().begin();
					i != _savedGame->getAlienMissions().end();
					++i)
			{
				if (dynamic_cast<AlienMission*>(*i) == am)
				{
					delete *i;
					_savedGame->getAlienMissions().erase(i);

					break;
				}
			}
		}
	}

	_missionStatistics->success = missionAccomplished;
}

/**
 * Reequips a craft after a mission.
 * @param base					- pointer to a Base to reequip from
 * @param craft					- pointer to a Craft to reequip
 * @param vehicleDestruction	- true if vehicles on the craft can be destroyed
 */
void DebriefingState::reequipCraft(
		Base* base,
		Craft* craft,
		bool vehicleDestruction)
{
	int
		qty,
		lost;
//	int used; // kL

	const std::map<std::string, int> craftItems = *craft->getItems()->getContents();
	for (std::map<std::string, int>::const_iterator
			i = craftItems.begin();
			i != craftItems.end();
			++i)
	{
		qty = base->getItems()->getItem(i->first);

		if (qty >= i->second)
		{
			base->getItems()->removeItem(
										i->first,
										i->second);
//			used = i->second; // kL
		}
		else
		{
			base->getItems()->removeItem(
										i->first,
										qty);

			lost = i->second - qty;
			craft->getItems()->removeItem(
										i->first,
										lost);
//										i->second - qty); // kL
//			used = lost; // kL

			const ReequipStat stat =
			{
				i->first,
				lost,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}

		// kL_begin:
		// NOTE: quantifying what was Used on the battlefield is difficult,
		// because *all items recovered* on the 'field, including the craft's
		// stores, have already been added to base->Stores before this function
		// runs. An independent vector of either base->Stores or craft->Stores
		// is NOT maintained.

//		used = i->second - qty;
//		used = qty - i->second;
//		if (used > 0)
/*		{
			ReequipStat stat =
			{
				i->first,
//				i->second,
				used,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		} */
		// kL_end.
	}

	// the vehicles
	ItemContainer craftVehicles;
	for (std::vector<Vehicle*>::const_iterator
			i = craft->getVehicles()->begin();
			i != craft->getVehicles()->end();
			++i)
	{
		craftVehicles.addItem((*i)->getRules()->getType());
	}

	// Now we know how many vehicles (separated by types) we have to read.
	// Erase the current vehicles, because we have to
	// reAdd them ('cause we want to redistribute their ammo)
	// kL_note: and generally weave our way through this spaghetti ....
	if (vehicleDestruction == true)
	{
		for (std::vector<Vehicle*>::const_iterator
				i = craft->getVehicles()->begin();
				i != craft->getVehicles()->end();
				++i)
		{
			delete *i;
		}
	}

	craft->getVehicles()->clear();

	int addTanks = 0;

	for (std::map<std::string, int>::const_iterator // Ok, now read those vehicles
			i = craftVehicles.getContents()->begin();
			i != craftVehicles.getContents()->end();
			++i)
	{
		qty = base->getItems()->getItem(i->first);
		addTanks = std::min(
						qty,
						i->second);

		if (qty < i->second)
		{
			lost = i->second - qty; // missing tanks
			const ReequipStat stat =
			{
				i->first,
				lost,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}


		RuleItem* const tankRule = _rules->getItem(i->first);
		const Unit* const tankUnit = _rules->getUnit(tankRule->getType());

		int tankSize;
		if (tankUnit != NULL)
		{
			tankSize = _rules->getArmor(tankUnit->getArmor())->getSize();
			tankSize *= tankSize;
		}
		else
			tankSize = 4; // safety.

		if (tankRule->getCompatibleAmmo()->empty() == true) // so this tank does NOT require ammo
		{
			for (int
					j = 0;
					j != addTanks;
					++j)
			{
				craft->getVehicles()->push_back(new Vehicle(
														tankRule,
														tankRule->getClipSize(),
														tankSize));
			}

			base->getItems()->removeItem(
									i->first,
									addTanks);
		}
		else // so this tank requires ammo
		{
			const RuleItem* const ammoRule = _rules->getItem(tankRule->getCompatibleAmmo()->front());
			const int ammoPerVehicle = ammoRule->getClipSize();

			const int baseQty = base->getItems()->getItem(ammoRule->getType()); // Ammo Quantity for this vehicle-type on the base

			if (baseQty < i->second * ammoPerVehicle)
			{
				lost = (i->second * ammoPerVehicle) - baseQty; // missing ammo
				const ReequipStat stat =
				{
					ammoRule->getType(),
					lost,
					craft->getName(_game->getLanguage())
				};

				_missingItems.push_back(stat);
			}

			addTanks = std::min(
							addTanks,
							baseQty / ammoPerVehicle);

			if (addTanks != 0)
			{
				for (int
						j = 0;
						j != addTanks;
						++j)
				{
					craft->getVehicles()->push_back(new Vehicle(
															tankRule,
															ammoPerVehicle,
															tankSize));

					base->getItems()->removeItem(
											ammoRule->getType(),
											ammoPerVehicle);
				}

				base->getItems()->removeItem(
										i->first,
										addTanks);
			}
		}

		// kL_begin:
/*		ReequipStat stat =
		{
			i->first,
			addTanks,
			craft->getName(_game->getLanguage())
		};

		_missingItems.push_back(stat); */
		// kL_end.
	}
}

/**
 * Recovers items from the battlescape.
 * Converts the battlescape inventory into a geoscape itemcontainer.
 * @param battleItems	- pointer to a vector of pointers to BattleItems on the battlescape
 * @param base			- base to add recovered items to
 */
void DebriefingState::recoverItems(
		std::vector<BattleItem*>* battleItems,
		Base* base)
{
	RuleItem* itRule;

	for (std::vector<BattleItem*>::const_iterator
			i = battleItems->begin();
			i != battleItems->end();
			++i)
	{
		itRule = (*i)->getRules();

		if (itRule->isFixed() == false)
		{
			if (itRule->getName() == _rules->getAlienFuel())
			{
				if (itRule->isRecoverable() == true)
					addStat( // special case of an item counted as a stat
						_rules->getAlienFuel(),
						100,
						50);
			}
			else //if (*i)->getXCOMProperty() == false
			{
				if (itRule->isRecoverable() == true
					&& itRule->getRecoveryPoints() != 0
					&& itRule->getBattleType() != BT_CORPSE
					&& _savedGame->isResearched(itRule->getType()) == false)
				{
					Log(LOG_INFO) << ". . artefact = " << itRule->getType();

					addStat( // add pts. for unresearched items only
						"STR_ALIEN_ARTIFACTS_RECOVERED",
						itRule->getRecoveryPoints());
				}

				switch (itRule->getBattleType()) // put items back in the base
				{
					case BT_CORPSE:
					{
						BattleUnit* unit = (*i)->getUnit();

						if (unit != NULL)
						{
							if (itRule->isRecoverable() == true
								&& unit->getStatus() == STATUS_DEAD)
							{
								Log(LOG_INFO) << ". . corpse = " << itRule->getType();

								addStat(
									"STR_ALIEN_CORPSES_RECOVERED",
									unit->getValue() / 3); // This should rather be the 'recoveryPoints' of the corpse item!

								if (unit->getArmor()->getCorpseGeoscape().empty() == false) // safety.
									base->getItems()->addItem(unit->getArmor()->getCorpseGeoscape());
							}
							else if (unit->getStatus() == STATUS_UNCONSCIOUS)
							{
								if (itRule->isRecoverable() == true
									&& unit->getOriginalFaction() == FACTION_HOSTILE)
								{
									// ADD STUNNED ALIEN COUNTING HERE_kL
									++_aliensStunned; // for Nike Cross determination.

									recoverLiveAlien(
											unit,
											base);
								}
								else if (unit->getOriginalFaction() == FACTION_NEUTRAL)
								{
									Log(LOG_INFO) << ". . unconsciousCivie = " << itRule->getType();

									addStat(
										"STR_CIVILIANS_SAVED",
										unit->getValue()); // duplicated above.
								}
							}
						}
					}
					break;

					case BT_AMMO: // item is a clip, count rounds left
						if (itRule->isRecoverable() == true)
							_rounds[itRule] += (*i)->getAmmoQuantity();
					break;

					case BT_FIREARM:
					case BT_MELEE: // item is a weapon, count rounds in clip
						if (itRule->isRecoverable() == true)
						{
							const BattleItem* const clip = (*i)->getAmmoItem();
							if (clip != NULL
								&& clip != *i
								&& clip->getRules()->getClipSize() > 0)
							{
								_rounds[clip->getRules()] += clip->getAmmoQuantity();
							}
						}

					default: // fall-through, recover the weapon/item itself
						if (itRule->isRecoverable() == true)
							base->getItems()->addItem(itRule->getType());
				}
			}
		}
	}
}

/**
 * Recovers a live alien from the battlescape.
 * @param unit - pointer to a BattleUnit to recover
 * @param base - pointer to the Base to add corpse item
 */
void DebriefingState::recoverLiveAlien(
		BattleUnit* unit,
		Base* base)
{
	if (base->getAvailableContainment() != 0)
	{
		Log(LOG_INFO) << ". . . alienLive = " << unit->getType();

		const std::string type = unit->getType();

		int value;
		if (_savedGame->isResearched(type) == false)
			value = unit->getValue() * 2;
		else
			value = unit->getValue();

		addStat(
			"STR_LIVE_ALIENS_RECOVERED",
			value);

		base->getItems()->addItem(type);

		_manageContainment = base->getAvailableContainment()
						   - (base->getUsedContainment() * _limitsEnforced) < 0;
	}
	else
	{
		Log(LOG_INFO) << ". . . alienDead = " << unit->getType();

		_noContainment = true;
		addStat(
			"STR_ALIEN_CORPSES_RECOVERED",
			unit->getValue() / 3);

		std::string corpseItem;
		if (unit->getSpawnUnit().empty() == false)
			corpseItem = _game->getRuleset()->getArmor(
												_game->getRuleset()->getUnit(
																		unit->getSpawnUnit())
												->getArmor())->getCorpseGeoscape();
		else
			corpseItem = unit->getArmor()->getCorpseGeoscape();

		if (corpseItem.empty() == false) // safety.
			base->getItems()->addItem(corpseItem);
	}
}

}
