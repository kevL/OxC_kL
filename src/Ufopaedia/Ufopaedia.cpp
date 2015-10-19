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

#include "Ufopaedia.h"

#include "ArticleState.h"
#include "ArticleStateArmor.h"
#include "ArticleStateAward.h"
#include "ArticleStateBaseFacility.h"
#include "ArticleStateCraft.h"
#include "ArticleStateCraftWeapon.h"
#include "ArticleStateItem.h"
#include "ArticleStateText.h"
#include "ArticleStateTextImage.h"
#include "ArticleStateUfo.h"
#include "ArticleStateVehicle.h"
#include "UfopaediaStartState.h"

#include "../Engine/Game.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

int Ufopaedia::_current_index = 0; // static.


/**
 * Gets the index of the selected article_id in the visible list.
 * @param gameSave		- pointer to SavedGame
 * @param rules			- pointer to Ruleset
 * @param article_id	- reference the article ID to find
 * @return, index of the given article ID in the internal list (-1 if not found)
 */
int Ufopaedia::getArticleIndex( // protected/static.
		const SavedGame* const gameSave,
		const Ruleset* const rules,
		std::string& article_id)
{
	const ArticleDefinitionList articles = getAvailableArticles(gameSave, rules);
	const std::string Id_UC = article_id + "_UC";

	for (size_t
			i = 0;
			i != articles.size();
			++i)
	{
		for (std::vector<std::string>::const_iterator
				j = articles[i]->requires.begin();
				j != articles[i]->requires.end();
				++j)
		{
			if (*j == article_id)
			{
				article_id = articles[i]->id;
				return i;
			}
		}

		if (articles[i]->id == article_id)
			return i;

		if (articles[i]->id == Id_UC)
		{
			article_id = Id_UC;
			return i;
		}
	}

	return -1;
}

/**
 * Returns an ArticleList with all the currently visible ArticleIds.
 * @param gameSave	- pointer to SavedGame
 * @param rules		- pointer to Ruleset
 * @return, ArticleDefinitionList of visible articles
 */
ArticleDefinitionList Ufopaedia::getAvailableArticles( // protected/static.
		const SavedGame* const gameSave,
		const Ruleset* const rules)
{
	const std::vector<std::string>& pedList = rules->getUfopaediaList();
	ArticleDefinitionList articles;

	for (std::vector<std::string>::const_iterator
			i = pedList.begin();
			i != pedList.end();
			++i)
	{
		ArticleDefinition* const article = rules->getUfopaediaArticle(*i);
		if (article->section != UFOPAEDIA_NOT_AVAILABLE
			&& isArticleAvailable(gameSave, article) == true)
		{
			articles.push_back(article);
		}
	}

	return articles;
}

/**
 * Creates a new article State dependent on the given ArticleDefinition.
 * @param article - pointer to ArticleDefinition to create from
 * @return, pointer to ArticleState object if created or NULL otherwise
 */
ArticleState* Ufopaedia::createArticleState(ArticleDefinition* const article) // protected/static.
{
	switch (article->getType())
	{
		case UFOPAEDIA_TYPE_CRAFT:
		return new ArticleStateCraft(dynamic_cast<ArticleDefinitionCraft*>(article));

		case UFOPAEDIA_TYPE_CRAFT_WEAPON:
		return new ArticleStateCraftWeapon(dynamic_cast<ArticleDefinitionCraftWeapon*>(article));

		case UFOPAEDIA_TYPE_VEHICLE:
		return new ArticleStateVehicle(dynamic_cast<ArticleDefinitionVehicle*>(article));

		case UFOPAEDIA_TYPE_ITEM:
		return new ArticleStateItem(dynamic_cast<ArticleDefinitionItem*>(article));

		case UFOPAEDIA_TYPE_ARMOR:
		return new ArticleStateArmor(dynamic_cast<ArticleDefinitionArmor*>(article));

		case UFOPAEDIA_TYPE_BASE_FACILITY:
		return new ArticleStateBaseFacility(dynamic_cast<ArticleDefinitionBaseFacility*>(article));

		case UFOPAEDIA_TYPE_TEXT:
		return new ArticleStateText(dynamic_cast<ArticleDefinitionText*>(article));

		case UFOPAEDIA_TYPE_TEXTIMAGE:
		return new ArticleStateTextImage(dynamic_cast<ArticleDefinitionTextImage*>(article));

		case UFOPAEDIA_TYPE_UFO:
		return new ArticleStateUfo(dynamic_cast<ArticleDefinitionUfo*>(article));

		case UFOPAEDIA_TYPE_AWARD:
		return new ArticleStateAward(dynamic_cast<ArticleDefinitionAward*>(article));
	}

	return NULL;
}

/**
 * Checks if an article has already been released.
 * @param gameSave	- pointer to SavedGame
 * @param article	- ArticleDefinition to release
 * @return, true if the article is available
 */
bool Ufopaedia::isArticleAvailable( // static.
		const SavedGame* const gameSave,
		const ArticleDefinition* const article)
{
	return gameSave->isResearched(article->requires);
}

/**
 * Sets UPSaved index and opens the new state.
 * @param game		- pointer to actual Game
 * @param article	- pointer to ArticleDefinition of the article to open
 */
void Ufopaedia::openArticle( // static.
		Game* const game,
		ArticleDefinition* const article)
{
	_current_index = getArticleIndex(
								game->getSavedGame(),
								game->getRuleset(),
								article->id);
	if (_current_index != -1)
		game->pushState(createArticleState(article));
}

/**
 * Checks if selected article_id is available -> if yes, open it.
 * @param game			- pointer to actual Game
 * @param article_id	- reference the article id to find
 */
void Ufopaedia::openArticle( // static.
		Game* const game,
		std::string& article_id)
{
	_current_index = getArticleIndex(
								game->getSavedGame(),
								game->getRuleset(),
								article_id);
	if (_current_index != -1)
	{
		ArticleDefinition* const article = game->getRuleset()->getUfopaediaArticle(article_id);
		game->pushState(createArticleState(article));
	}
}

/**
 * Opens Ufopaedia start state presenting the section selection buttons.
 * @param game		- pointer to the Game
 * @param tactical	- true if opening Ufopaedia from battlescape (default false)
 */
void Ufopaedia::open( // static.
		Game* const game,
		bool tactical)
{
	game->pushState(new UfopaediaStartState(tactical));
}

/**
 * Opens the next article in the list or loops to the first.
 * @param game - pointer to actual Game
 */
void Ufopaedia::next(Game* const game) // static.
{
	ArticleDefinitionList articles = getAvailableArticles(
														game->getSavedGame(),
														game->getRuleset());
	if (_current_index >= static_cast<int>(articles.size()) - 1)
		_current_index = 0; // goto first
	else
		++_current_index;

	game->popState();
	game->pushState(createArticleState(articles[_current_index]));
}

/**
 * Opens the previous article in the list or loops to the last.
 * @param game - pointer to actual Game
 */
void Ufopaedia::prev(Game* const game) // static.
{
	ArticleDefinitionList articles = getAvailableArticles(
														game->getSavedGame(),
														game->getRuleset());
	if (_current_index == 0)
		_current_index = static_cast<int>(articles.size()) - 1;
	else
		--_current_index;

	game->popState();
	game->pushState(createArticleState(articles[_current_index]));
}

/**
 * Fills an ArticleList with the currently visible ArticleIds of the given section.
 * @param gameSave	- pointer to SavedGame
 * @param rules		- pointer to Ruleset
 * @param section	- reference the article section to find, e.g. "XCOM Crafts & Armaments", "Alien Lifeforms", etc.
 * @param data		- reference the article definition list object to fill data in
 */
void Ufopaedia::list( // static.
		const SavedGame* const gameSave,
		const Ruleset* const rules,
		const std::string& section,
		ArticleDefinitionList& data)
{
	ArticleDefinitionList articles = getAvailableArticles(gameSave, rules);
	for (ArticleDefinitionList::const_iterator
			i = articles.begin();
			i != articles.end();
			++i)
	{
		if ((*i)->section == section)
			data.push_back(*i);
	}
}

}
