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

//#define _USE_MATH_DEFINES

#include "MonthlyReportState.h"

//#include <cmath>
//#include <sstream>

#include "DefeatState.h"
#include "Globe.h"
#include "PsiTrainingState.h"

#include "../Battlescape/CommendationState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Menu/SaveGameState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleCountry.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Monthly Report screen.
 * @param psi	- true to show psi training afterwards
 * @param globe	- pointer to Globe
 */
MonthlyReportState::MonthlyReportState(
		bool psi,
		Globe* globe)
	:
		_psi(psi),
		_gameOver(false),
		_ratingTotal(0),
		_fundingDiff(0),
		_ratingLastMonth(0)
//		_happyList(0),
//		_sadList(0),
//		_pactList(0)
{
	_globe = globe;
	_savedGame = _game->getSavedGame();

	_window		= new Window(this, 320, 200, 0, 0);
	_txtTitle	= new Text(300, 17, 10, 8);

	_txtMonth	= new Text(110, 9, 16, 24);
	_txtRating	= new Text(178, 9, 126, 24);

	_txtChange	= new Text(288, 9, 16, 32);
	_txtDesc	= new Text(288, 140, 16, 40);

	_btnOk		= new TextButton(288, 16, 16, 177);

	_txtFailure	= new Text(288, 160, 16, 10);
	_btnBigOk	= new TextButton(120, 18, 100, 175);

	setPalette(
			"PAL_GEOSCAPE",
			_game->getRuleset()->getInterface("monthlyReport")->getElement("palette")->color); //3

	add(_window, "window", "monthlyReport");
	add(_txtTitle, "text1", "monthlyReport");
	add(_txtMonth, "text1", "monthlyReport");
	add(_txtRating, "text1", "monthlyReport");
	add(_txtChange, "text1", "monthlyReport");
	add(_txtFailure, "text2", "monthlyReport");
	add(_txtDesc, "text2", "monthlyReport");
	add(_btnOk, "button", "monthlyReport");
	add(_btnBigOk, "button", "monthlyReport");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

//	_btnOk->setColor(Palette::blockOffset(8)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& MonthlyReportState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyCancel);

//	_btnBigOk->setColor(Palette::blockOffset(8)+10);
	_btnBigOk->setText(tr("STR_OK"));
	_btnBigOk->onMouseClick((ActionHandler)& MonthlyReportState::btnOkClick);
	_btnBigOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyOk);
	_btnBigOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyCancel);
	_btnBigOk->setVisible(false);

//	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_XCOM_PROJECT_MONTHLY_REPORT"));

//	_txtFailure->setColor(Palette::blockOffset(8)+10);
	_txtFailure->setBig();
	_txtFailure->setAlign(ALIGN_CENTER);
	_txtFailure->setVerticalAlign(ALIGN_MIDDLE);
	_txtFailure->setWordWrap();
	_txtFailure->setText(tr("STR_YOU_HAVE_FAILED"));
	_txtFailure->setVisible(false);


	calculateChanges(); // <- sets Rating.

	int
		month = _savedGame->getTime()->getMonth() - 1,
		year = _savedGame->getTime()->getYear();

	if (month == 0)
	{
		month = 12;
		--year;
	}

	std::string m;
	switch (month)
	{
		case  1: m = "STR_JAN"; break;
		case  2: m = "STR_FEB"; break;
		case  3: m = "STR_MAR"; break;
		case  4: m = "STR_APR"; break;
		case  5: m = "STR_MAY"; break;
		case  6: m = "STR_JUN"; break;
		case  7: m = "STR_JUL"; break;
		case  8: m = "STR_AUG"; break;
		case  9: m = "STR_SEP"; break;
		case 10: m = "STR_OCT"; break;
		case 11: m = "STR_NOV"; break;
		case 12: m = "STR_DEC";	break;

		default: m = "";
	}

//	_txtMonth->setColor(Palette::blockOffset(15)-1);
//	_txtMonth->setSecondaryColor(Palette::blockOffset(8)+10);
	_txtMonth->setText(tr("STR_MONTH").arg(tr(m)).arg(year));

	const int
		diff = static_cast<int>(_savedGame->getDifficulty()),
		difficulty_threshold = 250 * (diff - 4);
		// 0 -> -1000
		// 1 -> -750
		// 2 -> -500
		// 3 -> -250
		// 4 -> 0

	// do rating:
	std::wstring rating = tr("STR_RATING_TERRIBLE");
	if (_ratingTotal > 10000)
		rating = tr("STR_RATING_STUPENDOUS");
	else if (_ratingTotal > 5000)
		rating = tr("STR_RATING_EXCELLENT");
	else if (_ratingTotal > 2500)
		rating = tr("STR_RATING_GOOD");
	else if (_ratingTotal > 1000)
		rating = tr("STR_RATING_OK");
	else if (_ratingTotal > difficulty_threshold)
		rating = tr("STR_RATING_POOR");
	_txtRating->setColor(Palette::blockOffset(15)-1);
	_txtRating->setSecondaryColor(Palette::blockOffset(8)+10);
	_txtRating->setText(tr("STR_MONTHLY_RATING").arg(_ratingTotal).arg(rating));

	std::wostringstream ss3;
	if (_fundingDiff > 0) ss3 << '+';
	ss3 << Text::formatFunding(_fundingDiff);
//	_txtChange->setColor(Palette::blockOffset(15)-1);
//	_txtChange->setSecondaryColor(Palette::blockOffset(8)+10);
	_txtChange->setText(tr("STR_FUNDING_CHANGE").arg(ss3.str()));

//	_txtDesc->setColor(Palette::blockOffset(8)+10);
	_txtDesc->setWordWrap();

	// calculate satisfaction
	std::wostringstream ss4;
	std::wstring satisFactionString = tr("STR_COUNCIL_IS_DISSATISFIED");
	if (_ratingTotal > 1000 + 2000 * diff) // was 1500 flat.
		satisFactionString = tr("STR_COUNCIL_IS_VERY_PLEASED");
	else if (_ratingTotal > difficulty_threshold)
		satisFactionString = tr("STR_COUNCIL_IS_GENERALLY_SATISFIED");

	if (_ratingLastMonth <= difficulty_threshold
		&& _ratingTotal <= difficulty_threshold)
	{
		satisFactionString = tr("STR_YOU_HAVE_NOT_SUCCEEDED");

		_pactList.erase(
					_pactList.begin(),
					_pactList.end());
		_happyList.erase(
					_happyList.begin(),
					_happyList.end());
		_sadList.erase(
					_sadList.begin(),
					_sadList.end());

		_gameOver = true; // you lose.
	}
	ss4 << satisFactionString;

	bool resetWarning = true;
	if (_gameOver == false)
	{
		if (_savedGame->getFunds() <= -1000000)
		{
			if (_savedGame->getWarned() == true)
			{
				ss4.str(L"");
				ss4 << tr("STR_YOU_HAVE_NOT_SUCCEEDED");
//				ss4 << "\n\n" << tr("STR_YOU_HAVE_NOT_SUCCEEDED");

				_pactList.erase(
							_pactList.begin(),
							_pactList.end());
				_happyList.erase(
							_happyList.begin(),
							_happyList.end());
				_sadList.erase(
							_sadList.begin(),
							_sadList.end());

				_gameOver = true; // you lose.
			}
			else
			{
				ss4 << "\n\n" << tr("STR_COUNCIL_REDUCE_DEBTS");

				_savedGame->setWarned(true);
				resetWarning = false;
			}
		}
	}

	if (resetWarning == true
		&& _savedGame->getWarned() == true)
	{
		_savedGame->setWarned(false);
	}

	ss4 << countryList(
					_happyList,
					"STR_COUNTRY_IS_PARTICULARLY_PLEASED",
					"STR_COUNTRIES_ARE_PARTICULARLY_HAPPY");
	ss4 << countryList(
					_sadList,
					"STR_COUNTRY_IS_UNHAPPY_WITH_YOUR_ABILITY",
					"STR_COUNTRIES_ARE_UNHAPPY_WITH_YOUR_ABILITY");
	ss4 << countryList(
					_pactList,
					"STR_COUNTRY_HAS_SIGNED_A_SECRET_PACT",
					"STR_COUNTRIES_HAVE_SIGNED_A_SECRET_PACT");

	_txtDesc->setText(ss4.str());


	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_MONTHLYREPORT);
}

/**
 * dTor.
 */
MonthlyReportState::~MonthlyReportState()
{}

/**
 * Update all our activity counters, gather all our scores,
 * get our countries to sign pacts, adjust their fundings,
 * assess their satisfaction, and finally calculate our overall
 * total score, with thanks to Volutar for the formulae.
 */
void MonthlyReportState::calculateChanges()
{
	_ratingLastMonth = 0;

	int
		xComSubTotal	= 0,
		xComTotal		= 0,
		aLienTotal		= 0,

		monthOffset		= _savedGame->getFundsList().size() - 2,
		lastMonthOffset	= _savedGame->getFundsList().size() - 3;

	if (lastMonthOffset < 0)
		lastMonthOffset += 2;

	// update activity meters, calculate a total score based
	// on regional activity and gather last month's score
	for (std::vector<Region*>::const_iterator
			i = _savedGame->getRegions()->begin();
			i != _savedGame->getRegions()->end();
			++i)
	{
		(*i)->newMonth();

		if ((*i)->getActivityXcom().size() > 2)
			_ratingLastMonth += (*i)->getActivityXcom().at(lastMonthOffset)
							  - (*i)->getActivityAlien().at(lastMonthOffset);

		xComSubTotal += (*i)->getActivityXcom().at(monthOffset);
		aLienTotal += (*i)->getActivityAlien().at(monthOffset);
	}

	// apply research bonus AFTER calculating our total, because this bonus applies
	// to the council ONLY, and shouldn't influence each country's decision.
	// kL_note: And yet you _do_ add it in to country->newMonth() decisions...!
	// So, hey, I'll take it out for you.. just a sec.
	xComTotal = _savedGame->getResearchScores().at(monthOffset) + xComSubTotal;

	// the council is more lenient after the first month
//	if (_savedGame->getMonthsPassed() > 1)
//		_savedGame->getResearchScores().at(monthOffset) += 400;

	if (_savedGame->getResearchScores().size() > 2)
		_ratingLastMonth += _savedGame->getResearchScores().at(lastMonthOffset);


	// now that we have our totals we can send the relevant info to the countries
	// and have them make their decisions weighted on the council's perspective.
	for (std::vector<Country*>::const_iterator
			i = _savedGame->getCountries()->begin();
			i != _savedGame->getCountries()->end();
			++i)
	{
		// add them to the list of new pact members; this is done BEFORE initiating
		// a new month because the _newPact flag will be reset in the process <-
		if ((*i)->getNewPact())
			_pactList.push_back((*i)->getRules()->getType());

		// determine satisfaction level, sign pacts, adjust funding, and update activity meters
		const int diff = static_cast<int>(_savedGame->getDifficulty());
		(*i)->newMonth(
					xComSubTotal, // There. done
					aLienTotal,
					diff);

		// and after they've made their decisions, calculate the difference,
		// and add them to the appropriate lists.
		_fundingDiff += (*i)->getFunding().back()
					  - (*i)->getFunding().at((*i)->getFunding().size() - 2);

		switch ((*i)->getSatisfaction())
		{
			case 1:
				_sadList.push_back((*i)->getRules()->getType());
			break;
			case 3:
				_happyList.push_back((*i)->getRules()->getType());
			break;

			default:
			break;
		}
	}

	_ratingTotal = xComTotal - aLienTotal; // total RATING
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void MonthlyReportState::btnOkClick(Action*)
{
	if (_gameOver == false)
	{
		_game->popState();

		for (std::vector<Base*>::const_iterator // Award medals for service time
				i = _savedGame->getBases()->begin();
				i != _savedGame->getBases()->end();
				++i)
		{
			for (std::vector<Soldier*>::const_iterator
					j = (*i)->getSoldiers()->begin();
					j != (*i)->getSoldiers()->end();
					++j)
			{
				Soldier* const soldier = _savedGame->getSoldier((*j)->getId());
				soldier->getDiary()->addMonthlyService();

				if (soldier->getDiary()->manageAwards(_game->getRuleset()) == true)
					_soldiersMedalled.push_back(soldier);
			}
		}

		if (_soldiersMedalled.empty() == false)
			_game->pushState(new CommendationState(_soldiersMedalled));

		if (_psi == true)
			_game->pushState(new PsiTrainingState());

		if (_savedGame->isIronman() == true)
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
	else
	{
		if (_txtFailure->getVisible() == true)
		{
			_game->popState();
			_game->pushState(new DefeatState());
		}
		else
		{
			_window->setColor(_game->getRuleset()->getInterface("monthlyReport")->getElement("window")->color2); //Palette::blockOffset(8)+10);

			_txtTitle->setVisible(false);
			_txtMonth->setVisible(false);
			_txtRating->setVisible(false);
			_txtChange->setVisible(false);
			_txtDesc->setVisible(false);
			_btnOk->setVisible(false);

			_txtFailure->setVisible();
			_btnBigOk->setVisible();


			_game->getResourcePack()->fadeMusic(_game, 1200);
			_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_LOSE);
		}
	}
}

/**
 * Builds a sentence from a list of countries, adding the appropriate
 * separators and pluralization.
 * @param countries	- reference a vector of strings that is the list of countries
 * @param singular	- reference a string to append if the returned string is singular
 * @param plural	- reference a string to append if the returned string is plural
 */
std::wstring MonthlyReportState::countryList(
		const std::vector<std::string>& countries,
		const std::string& singular,
		const std::string& plural)
{
	std::wostringstream ss;

	if (countries.empty() == false)
	{
		ss << "\n\n";
		if (countries.size() == 1)
			ss << tr(singular).arg(tr(countries.front()));
		else
		{
			LocalizedText countryList = tr(countries.front());

			std::vector<std::string>::const_iterator i;
			for (
					i = countries.begin() + 1;
					i != countries.end() - 1;
					++i)
			{
				countryList = tr("STR_COUNTRIES_COMMA").arg(countryList).arg(tr(*i));
			}
			countryList = tr("STR_COUNTRIES_AND").arg(countryList).arg(tr(*i));

			ss << tr(plural).arg(countryList);
		}
	}

	return ss.str();
}

}
