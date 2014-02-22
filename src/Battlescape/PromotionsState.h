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

#ifndef OPENXCOM_PROMOTIONSSTATE_H
#define OPENXCOM_PROMOTIONSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Promotions screen that displays new soldier ranks.
 */
class PromotionsState
	:
		public State
{

private:
	Text
		* _txtTitle,
		* _txtName,
		* _txtRank,
		* _txtBase;
	TextButton* _btnOk;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates the Promotions state.
		PromotionsState(Game* game);
		/// Cleans up the Promotions state.
		~PromotionsState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
