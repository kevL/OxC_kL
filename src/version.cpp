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

#include "version.h"


namespace OpenXcom
{

namespace Version
{

/**
 * Gets a date/time in a human-readable string using the ISO 8601 standard.
 * @note This uses current runtime and is therefore apropos only as
 * a timestamp for a saved file.
 * @return, string of local time useful for saved files
 */
std::string timeStamp()
{
	time_t timeOut = std::time(NULL);
	struct tm* timeInfo = std::localtime(&timeOut);

	char verDate[7];
	std::strftime(
				verDate,
				7,
				"%y%m%d",
				timeInfo);

	char verTime[7];
	std::strftime(
				verTime,
				7,
				"%H%M%S",
				timeInfo);

	std::ostringstream stamp;
	stamp << verDate << "." << verTime;

	return stamp.str();
}

/**
 * Gets version as a time string.
 * @note This is the (local) compile-time date & time.
 * @param built - true to add "built" preface (default true)
 * @return, current version of executable
 */
std::string getBuildDate(bool built)
{
	std::ostringstream build;

	if (built == true)
		build << "built ";

	build << __DATE__ << " " << __TIME__;

	return build.str();
}

}

}
