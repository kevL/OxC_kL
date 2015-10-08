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

#ifndef OPENXCOM_NEWPOSSIBLERESEARCHSTATE
#define OPENXCOM_NEWPOSSIBLERESEARCHSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleResearch;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window that informs the player of new possible research projects.
 * @note Also allows the player to go to ResearchState to dispatch scientists.
 */
class NewPossibleResearchState
	:
		public State
{

private:
	Base* _base;
	Text* _txtTitle;
	TextList* _lstPossibilities;
	TextButton
		* _btnResearch,
		* _btnOk;
	Window* _window;


	public:
		/// Creates the NewPossibleResearch state.
		NewPossibleResearchState(
				Base* const base,
				const std::vector<const RuleResearch*>& resRules,
				bool showBtn);

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Allocate Research button.
		void btnResearchClick(Action* action);
};

}

#endif
