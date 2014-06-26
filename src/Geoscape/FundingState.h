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

#ifndef OPENXCOM_FUNDINGSTATE_H
#define OPENXCOM_FUNDINGSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;

/**
 * Funding screen accessible from the Geoscape
 * that shows all the countries' funding.
 */
class FundingState
	:
		public State
{

private:
	Text
		* _txtChange,
		* _txtCountry,
		* _txtFunding,
		* _txtTitle;
	TextButton* _btnOk;
	TextList
		* _lstCountries,
		* _lstTotal;
	Window* _window;


	public:
		/// Creates the Funding state.
		FundingState();
		/// Cleans up the Funding state.
		~FundingState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
