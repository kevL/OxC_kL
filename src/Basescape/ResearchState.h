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

#ifndef OPENXCOM_RESEARCHSTATE_H
#define OPENXCOM_RESEARCHSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class BasescapeState;
class MiniBaseView;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Research screen that lets the player manage the research operations of a Base.
 */
class ResearchState
	:
		public State
{

private:
	Base* _base;
	BasescapeState* _state;
	MiniBaseView* _mini;
	Text
		* _txtAllocated,
		* _txtAvailable,
		* _txtBaseLabel,
		* _txtHoverBase,
		* _txtProgress,
		* _txtProject,
		* _txtScientists,
		* _txtSpace,
		* _txtTitle;
	TextButton
		* _btnAliens,
		* _btnNew,
		* _btnOk;
	TextList* _lstResearch;
	Window* _window;

	std::vector<bool> _online;

	std::vector<Base*>* _baseList;


	public:
		/// Creates the Research state.
		ResearchState(
				Base* base,
				BasescapeState* state = NULL);
		/// Cleans up the Research state.
		~ResearchState();

		/// Updates the research list.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the New Research button.
		void btnNewClick(Action* action);
		/// Handler for clicking the Alien Containment button.
		void btnAliens(Action* action);
		/// Handler for clicking the ResearchProject list.
		void onSelectProject(Action* action);

		/// Handler for clicking the mini base view.
		void miniClick(Action* action);
		/// Handler for hovering the MiniBase view.
		void viewMouseOver(Action* action);
		/// Handler for hovering out of the MiniBase view.
		void viewMouseOut(Action* action);
};

}

#endif
