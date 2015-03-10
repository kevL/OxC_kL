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
 * a timestamp for a saved file eg.
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
	std::string tz;

#ifdef _WIN32
	TIME_ZONE_INFORMATION tziTest;
	DWORD dwRet = GetTimeZoneInformation(&tziTest);
	if (dwRet == TIME_ZONE_ID_DAYLIGHT)
	{
		tz = " MDT";
//		wprintf(L"%s\n", tziTest.DaylightName);
	}
	else if (dwRet == TIME_ZONE_ID_STANDARD
		|| dwRet == TIME_ZONE_ID_UNKNOWN)
	{
		tz = " MST";
//		wprintf(L"%s\n", tziTest.StandardName);
	}
//	else printf("GTZI failed (%d)\n", GetLastError());
#endif

	std::ostringstream oststr;

	if (built == true)
		oststr << "blt ";

	oststr << __DATE__ << " " << __TIME__ << tz;
	std::string st = oststr.str();

	size_t pos = st.find("  "); // remove possible double-space between month & single-digit days
	if (pos != std::string::npos)
		st.erase(pos, 1);

	return st;
}

}

}
