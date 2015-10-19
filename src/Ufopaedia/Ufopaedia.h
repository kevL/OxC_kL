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

#ifndef OPENXCOM_UFOPAEDIA_H
#define OPENXCOM_UFOPAEDIA_H

//#include <string>
//#include <vector>


namespace OpenXcom
{


class ArticleDefinition;
class ArticleState;
class Game;
class Ruleset;
class SavedGame;


/// definition of an article list
typedef std::vector<ArticleDefinition*> ArticleDefinitionList;


/// define Ufopaedia sections, which must be consistent
static const std::string
	UFOPAEDIA_XCOM_CRAFT_ARMAMENT		= "STR_XCOM_CRAFT_ARMAMENT",
	UFOPAEDIA_HEAVY_WEAPONS_PLATFORMS	= "STR_HEAVY_WEAPONS_PLATFORMS",
	UFOPAEDIA_WEAPONS_AND_EQUIPMENT		= "STR_WEAPONS_AND_EQUIPMENT",
	UFOPAEDIA_ALIEN_ARTIFACTS			= "STR_ALIEN_ARTIFACTS",
	UFOPAEDIA_BASE_FACILITIES			= "STR_BASE_FACILITIES",
	UFOPAEDIA_ALIEN_LIFE_FORMS			= "STR_ALIEN_LIFE_FORMS",
	UFOPAEDIA_ALIEN_RESEARCH			= "STR_ALIEN_RESEARCH_UC",
	UFOPAEDIA_UFO_COMPONENTS			= "STR_UFO_COMPONENTS",
	UFOPAEDIA_UFOS						= "STR_UFOS",
	UFOPAEDIA_AWARDS					= "STR_AWARDS",
	UFOPAEDIA_NOT_AVAILABLE				= "STR_NOT_AVAILABLE";

// The last section is meant for articles that have to be activated but have no
// entry in a list. E.g. Ammunition items. Maybe others as well that should just
// not be selectable.


/**
 * This pure static class encapsulates all functions related to Ufopaedia
 * for the game.
 * @note It manages the relationship between the UfopaediaSaved instance in
 * SavedGame and the UfopaediaFactory in Ruleset. Main purpose is to open
 * Ufopaedia from Geoscape, navigate between articles, and release new articles
 * after successful research. The Battlescape and Inventory have also been
 * granted access.
 */
class Ufopaedia
{

protected:
	/// Current selected article index (for prev/next navigation).
	static int _current_index;

	/// Gets index of the given article id in the visible list.
	static int getArticleIndex(
			const SavedGame* const gameSave,
			const Ruleset* const rules,
			std::string& article_id);
	/// Gets list of researched articles
	static ArticleDefinitionList getAvailableArticles(
			const SavedGame* const gameSave,
			const Ruleset* const rules);
	/// Creates a new state object from article definition.
	static ArticleState* createArticleState(ArticleDefinition* const article);


	public:
		/// Checks if a specific article is currently available.
		static bool isArticleAvailable(
				const SavedGame* const gameSave,
				const ArticleDefinition* const article);
		/// Opens Ufopaedia on a certain entry.
		static void openArticle(
				Game* const game,
				std::string& article_id);
		/// Opens Ufopaedia article from a given article definition.
		static void openArticle(
				Game* const game,
				ArticleDefinition* article);

		/// Opens Ufopaedia with selection dialog.
		static void open(
				Game* const game,
				bool tactical = false);

		/// Article navigation to next article.
		static void next(Game* const game);
		/// Article navigation to previous article.
		static void prev(Game* const game);

		/// Loads a vector with article ids that are currently visible of a given section.
		static void list(
				const SavedGame* const gameSave,
				const Ruleset* const rules,
				const std::string& section,
				ArticleDefinitionList& data);
};

}

#endif
