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

#ifndef OPENXCOM_SAVECONVERTERXCOM1_H
#define OPENXCOM_SAVECONVERTERXCOM1_H


namespace OpenXcom
{

	// Mappings between X-COM 1 IDs and OpenXcom string IDs.

	const char* xcom1Markers[] =
	{
		"STR_UFO",
		"STR_TERROR_SITE",
		"STR_ALIEN_BASE",
		"STR_LANDING_SITE",
		"STR_CRASH_SITE",
		"STR_WAYPOINT",
		"STR_SKYRANGER",
		"STR_LIGHTNING",
		"STR_AVENGER",
		"STR_INTERCEPTOR",
		"STR_FIRESTORM"
	};

	const char* xcom1Countries[] =
	{
		"STR_USA",
		"STR_RUSSIA",
		"STR_UK",
		"STR_FRANCE",
		"STR_GERMANY",
		"STR_ITALY",
		"STR_SPAIN",
		"STR_CHINA",
		"STR_JAPAN",
		"STR_INDIA",
		"STR_BRAZIL",
		"STR_AUSTRALIA",
		"STR_NIGERIA",
		"STR_SOUTH_AFRICA",
		"STR_EGYPT",
		"STR_CANADA"
	};

	const char* xcom1Regions[] =
	{
		"STR_NORTH_AMERICA",
		"STR_ARCTIC",
		"STR_ANTARCTICA",
		"STR_SOUTH_AMERICA",
		"STR_EUROPE",
		"STR_NORTH_AFRICA",
		"STR_SOUTHERN_AFRICA",
		"STR_CENTRAL_ASIA",
		"STR_SOUTH_EAST_ASIA",
		"STR_SIBERIA",
		"STR_AUSTRALASIA",
		"STR_PACIFIC",
		"STR_NORTH_ATLANTIC",
		"STR_SOUTH_ATLANTIC",
		"STR_INDIAN_OCEAN"
	};

	const char* xcom1Facilities[] =
	{
		"STR_ACCESS_LIFT",
		"STR_LIVING_QUARTERS",
		"STR_LABORATORY",
		"STR_WORKSHOP",
		"STR_SMALL_RADAR_SYSTEM",
		"STR_LARGE_RADAR_SYSTEM",
		"STR_MISSILE_DEFENSES",
		"STR_GENERAL_STORES",
		"STR_ALIEN_CONTAINMENT",
		"STR_LASER_DEFENSES",
		"STR_PLASMA_DEFENSES",
		"STR_FUSION_BALL_DEFENSES",
		"STR_GRAV_SHIELD",
		"STR_MIND_SHIELD",
		"STR_PSIONIC_LABORATORY",
		"STR_HYPER_WAVE_DECODER",
		"STR_HANGAR"
	};

	const char* xcom1Items[] =
	{
		"STR_STINGRAY_LAUNCHER",
		"STR_AVALANCHE_LAUNCHER",
		"STR_CANNON",
		"STR_FUSION_BALL_LAUNCHER",
		"STR_LASER_CANNON",
		"STR_PLASMA_BEAM",
		"STR_STINGRAY_MISSILES",
		"STR_AVALANCHE_MISSILES",
		"STR_CANNON_ROUNDS_X50",
		"STR_FUSION_BALL",
		"STR_TANK_CANNON",
		"STR_TANK_ROCKET_LAUNCHER",
		"STR_TANK_LASER_CANNON",
		"STR_HOVERTANK_PLASMA",
		"STR_HOVERTANK_LAUNCHER",
		"STR_PISTOL",
		"STR_PISTOL_CLIP",
		"STR_RIFLE",
		"STR_RIFLE_CLIP",
		"STR_HEAVY_CANNON",
		"STR_HC_AP_AMMO",
		"STR_HC_HE_AMMO",
		"STR_HC_I_AMMO",
		"STR_AUTO_CANNON",
		"STR_AC_AP_AMMO",
		"STR_AC_HE_AMMO",
		"STR_AC_I_AMMO",
		"STR_ROCKET_LAUNCHER",
		"STR_SMALL_ROCKET",
		"STR_LARGE_ROCKET",
		"STR_INCENDIARY_ROCKET",
		"STR_LASER_PISTOL",
		"STR_LASER_RIFLE",
		"STR_HEAVY_LASER",
		"STR_GRENADE",
		"STR_SMOKE_GRENADE",
		"STR_PROXIMITY_GRENADE",
		"STR_HIGH_EXPLOSIVE",
		"STR_MOTION_SCANNER",
		"STR_MEDI_KIT",
		"STR_PSI_AMP",
		"STR_STUN_ROD",
		"STR_ELECTRO_FLARE",
		0,
		0,
		0,
		"STR_CORPSE",
		"STR_CORPSE_ARMOUR",
		"STR_CORPSE_SUIT",
		"STR_HEAVY_PLASMA",
		"STR_HEAVY_PLASMA_CLIP",
		"STR_PLASMA_RIFLE",
		"STR_PLASMA_RIFLE_CLIP",
		"STR_PLASMA_PISTOL",
		"STR_PLASMA_PISTOL_CLIP",
		"STR_BLASTER_LAUNCHER",
		"STR_BLASTER_BOMB",
		"STR_SMALL_LAUNCHER",
		"STR_STUN_BOMB",
		"STR_ALIEN_GRENADE",
		"STR_ELERIUM_115",
		"STR_MIND_PROBE",
		0,
		0,
		0,
		"STR_SECTOID_CORPSE",
		"STR_SNAKEMAN_CORPSE",
		"STR_ETHEREAL_CORPSE",
		"STR_MUTON_CORPSE",
		"STR_FLOATER_CORPSE",
		"STR_CELATID_CORPSE",
		"STR_SILACOID_CORPSE",
		"STR_CHRYSSALID_CORPSE",
		"STR_REAPER_CORPSE",
		"STR_SECTOPOD_CORPSE",
		"STR_CYBERDISC_CORPSE",
		"STR_HOVERTANK_CORPSE",
		"STR_TANK_CORPSE",
		"STR_CIVM_CORPSE",
		"STR_CIVF_CORPSE",
		"STR_UFO_POWER_SOURCE",
		"STR_UFO_NAVIGATION",
		"STR_UFO_CONSTRUCTION",
		"STR_ALIEN_FOOD",
		"STR_ALIEN_REPRODUCTION",
		"STR_ALIEN_ENTERTAINMENT",
		"STR_ALIEN_SURGERY",
		"STR_EXAMINATION_ROOM",
		"STR_ALIEN_ALLOYS",
		"STR_ALIEN_HABITAT",
		"STR_PERSONAL_ARMOR",
		"STR_POWER_SUIT",
		"STR_FLYING_SUIT",
		"STR_HWP_CANNON_SHELLS",
		"STR_HWP_ROCKETS",
		"STR_HWP_FUSION_BOMB"
	};

	const char* xcom1Races[] =
	{
		"STR_SECTOID",
		"STR_SNAKEMAN",
		"STR_ETHEREAL",
		"STR_MUTON",
		"STR_FLOATER",
		"STR_MIXED"
	};

	const char* xcom1Crafts[] =
	{
		"STR_SKYRANGER",
		"STR_LIGHTNING",
		"STR_AVENGER",
		"STR_INTERCEPTOR",
		"STR_FIRESTORM"
	};

	const char* xcom1Ufos[] =
	{
		"STR_SMALL_SCOUT",
		"STR_MEDIUM_SCOUT",
		"STR_LARGE_SCOUT",
		"STR_HARVESTER",
		"STR_ABDUCTOR",
		"STR_TERROR_SHIP",
		"STR_BATTLESHIP",
		"STR_SUPPLY_SHIP"
	};

	const char* xcom1CraftWeapons[] =
	{
		"STR_STINGRAY",
		"STR_AVALANCHE",
		"STR_CANNON_UC",
		"STR_FUSION_BALL_UC",
		"STR_LASER_CANNON_UC",
		"STR_PLASMA_BEAM_UC"
	};

	const char* xcom1Missions[] =
	{
		"STR_ALIEN_RESEARCH",
		"STR_ALIEN_HARVEST",
		"STR_ALIEN_ABDUCTION",
		"STR_ALIEN_INFILTRATION",
		"STR_ALIEN_BASE",
		"STR_ALIEN_TERROR",
		"STR_ALIEN_RETALIATION",
		"STR_ALIEN_SUPPLY"
	};

	const char* xcom1Armor[] =
	{
		"STR_NONE_UC",
		"STR_PERSONAL_ARMOR_UC",
		"STR_POWER_SUIT_UC",
		"STR_FLYING_SUIT_UC"
	};

	const char* xcom1LiveAliens[] =
	{
		0,
		"STR_SECTOID",
		"STR_SNAKEMAN",
		"STR_ETHEREAL",
		"STR_MUTON",
		"STR_FLOATER",
		"STR_CELATID",
		"STR_SILACOID",
		"STR_CHRYSSALID",
		"STR_REAPER",
		"STR_SECTOPOD",
		"STR_CYBERDISC",
	};

	const char* xcom1LiveRanks[] =
	{
		0,
		"_COMMANDER",
		"_LEADER",
		"_ENGINEER",
		"_MEDIC",
		"_NAVIGATOR",
		"_SOLDIER",
		"_TERRORIST"
	};

	const char* xcom1Research[] =
	{
		"STR_LASER_WEAPONS",
		"STR_MOTION_SCANNER",
		"STR_MEDI_KIT",
		"STR_PSI_AMP",
		"STR_HEAVY_PLASMA",
		"STR_HEAVY_PLASMA_CLIP",
		"STR_PLASMA_RIFLE",
		"STR_PLASMA_RIFLE_CLIP",
		"STR_PLASMA_PISTOL",
		"STR_PLASMA_PISTOL_CLIP",
		"STR_BLASTER_LAUNCHER",
		"STR_BLASTER_BOMB",
		"STR_SMALL_LAUNCHER",
		"STR_STUN_BOMB",
		"STR_ALIEN_GRENADE",
		"STR_ELERIUM_115",
		"STR_MIND_PROBE",
		"STR_UFO_POWER_SOURCE",
		"STR_UFO_NAVIGATION",
		"STR_UFO_CONSTRUCTION",
		"STR_ALIEN_FOOD",
		"STR_ALIEN_REPRODUCTION",
		"STR_ALIEN_ENTERTAINMENT",
		"STR_ALIEN_SURGERY",
		"STR_EXAMINATION_ROOM",
		"STR_ALIEN_ALLOYS",
		"STR_NEW_FIGHTER_CRAFT",
		"STR_NEW_FIGHTER_TRANSPORTER",
		"STR_ULTIMATE_CRAFT",
		"STR_LASER_PISTOL",
		"STR_LASER_RIFLE",
		"STR_HEAVY_LASER",
		"STR_LASER_CANNON",
		"STR_PLASMA_CANNON",
		"STR_FUSION_MISSILE",
		"STR_LASER_DEFENSE",
		"STR_PLASMA_DEFENSE",
		"STR_FUSION_DEFENSE",
		"STR_GRAV_SHIELD",
		"STR_MIND_SHIELD",
		"STR_PSI_LAB",
		"STR_HYPER_WAVE_DECODER",
		0,
		0,
		0,
		"STR_SECTOID_CORPSE",
		"STR_SNAKEMAN_CORPSE",
		"STR_ETHEREAL_CORPSE",
		"STR_MUTON_CORPSE",
		"STR_FLOATER_CORPSE",
		"STR_CELATID_CORPSE",
		"STR_SILACOID_CORPSE",
		"STR_CHRYSSALID_CORPSE",
		"STR_REAPER_CORPSE",
		"STR_SECTOPOD_CORPSE",
		"STR_CYBERDISC_CORPSE",
		"STR_ALIEN_ORIGINS",
		"STR_THE_MARTIAN_SOLUTION",
		"STR_CYDONIA_OR_BUST",
		"STR_PERSONAL_ARMOR",
		"STR_POWER_SUIT",
		"STR_FLYING_SUIT",
		"STR_SECTOID_COMMANDER",
		"STR_SECTOID_LEADER",
		"STR_SECTOID_ENGINEER",
		"STR_SECTOID_MEDIC",
		"STR_SECTOID_NAVIGATOR",
		"STR_SECTOID_SOLDIER",
		"STR_SNAKEMAN_COMMANDER",
		"STR_SNAKEMAN_LEADER",
		"STR_SNAKEMAN_ENGINEER",
		"STR_SNAKEMAN_MEDIC",
		"STR_SNAKEMAN_NAVIGATOR",
		"STR_SNAKEMAN_SOLDIER",
		"STR_ETHEREAL_COMMANDER",
		"STR_ETHEREAL_LEADER",
		"STR_ETHEREAL_ENGINEER",
		"STR_ETHEREAL_MEDIC",
		"STR_ETHEREAL_NAVIGATOR",
		"STR_ETHEREAL_SOLDIER",
		"STR_MUTON_COMMANDER",
		"STR_MUTON_LEADER",
		"STR_MUTON_ENGINEER",
		"STR_MUTON_MEDIC",
		"STR_MUTON_NAVIGATOR",
		"STR_MUTON_SOLDIER",
		"STR_FLOATER_COMMANDER",
		"STR_FLOATER_LEADER",
		"STR_FLOATER_ENGINEER",
		"STR_FLOATER_MEDIC",
		"STR_FLOATER_NAVIGATOR",
		"STR_FLOATER_SOLDIER",
		"STR_CELATID_TERRORIST",
		"STR_SILACOID_TERRORIST",
		"STR_CHRYSSALID_TERRORIST",
		"STR_REAPER_TERRORIST"
	};

	const char* xcom1Manufacture[] =
	{
		"STR_FUSION_BALL_LAUNCHER",
		"STR_LASER_CANNON",
		"STR_PLASMA_BEAM",
		"STR_FUSION_BALL",
		"STR_TANK_LASER_CANNON",
		"STR_HOVERTANK_PLASMA",
		"STR_HOVERTANK_LAUNCHER",
		"STR_HWP_FUSION_BOMB",
		"STR_LASER_PISTOL",
		"STR_LASER_RIFLE",
		"STR_HEAVY_LASER",
		"STR_MOTION_SCANNER",
		"STR_MEDI_KIT",
		"STR_PSI_AMP",
		"STR_HEAVY_PLASMA",
		"STR_HEAVY_PLASMA_CLIP",
		"STR_PLASMA_RIFLE",
		"STR_PLASMA_RIFLE_CLIP",
		"STR_PLASMA_PISTOL",
		"STR_PLASMA_PISTOL_CLIP",
		"STR_BLASTER_LAUNCHER",
		"STR_BLASTER_BOMB",
		"STR_SMALL_LAUNCHER",
		"STR_STUN_BOMB",
		"STR_ALIEN_GRENADE",
		"STR_MIND_PROBE",
		"STR_PERSONAL_ARMOR",
		"STR_POWER_SUIT",
		"STR_FLYING_SUIT",
		"STR_ALIEN_ALLOYS",
		"STR_UFO_POWER_SOURCE",
		"STR_UFO_NAVIGATION",
		"STR_FIRESTORM",
		"STR_LIGHTNING",
		"STR_AVENGER"
	};
}

#endif
