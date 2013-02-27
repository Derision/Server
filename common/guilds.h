/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef GUILD_H
#define GUILD_H

#include "types.h"

#define GUILD_NONE		0xFFFFFFFF // user has no guild

#define GUILD_MAX_RANK  8   // 0-2 - some places in the code assume a single digit, dont go above 9


//#define GUILD_MEMBER	0
//#define GUILD_OFFICER	1
//#define GUILD_LEADER	2
//#define GUILD_RANK_NONE (GUILD_MAX_RANK+1)

static const uint32 GUILD_RANK_LEADER		= 1;
static const uint32 GUILD_RANK_SENIOR_OFFICER	= 2;
static const uint32 GUILD_RANK_OFFICER		= 3;
static const uint32 GUILD_RANK_SENIOR_MEMBER	= 4;
static const uint32 GUILD_RANK_MEMBER		= 5;
static const uint32 GUILD_RANK_JUNIOR_MEMBER	= 6;
static const uint32 GUILD_RANK_INITIATE		= 7;
static const uint32 GUILD_RANK_RECRUIT		= 8;
static const uint32 GUILD_RANK_NONE		= 9;

static const uint32 GUILD_PERMISSION_BANNER_CHANGE				= 1;
static const uint32 GUILD_PERMISSION_BANNER_PLANT				= 2;
static const uint32 GUILD_PERMISSION_BANNER_REMOVE				= 3;
static const uint32 GUILD_PERMISSION_DISPLAY_GUILD_NAME				= 4;
static const uint32 GUILD_PERMISSION_RANKS_CHANGE_PERMISSIONS			= 5;
static const uint32 GUILD_PERMISSION_RANKS_CHANGE_NAMES				= 6;
static const uint32 GUILD_PERMISSION_MEMBERS_INVITE				= 7;
static const uint32 GUILD_PERMISSION_MEMBERS_PROMOTE				= 8;
static const uint32 GUILD_PERMISSION_MEMBERS_DEMOTE				= 9;
static const uint32 GUILD_PERMISSION_MEMBERS_REMOVE				= 10;
static const uint32 GUILD_PERMISSION_EDIT_RECRUITING_SETTINGS			= 11;
static const uint32 GUILD_PERMISSION_EDIT_PUBLIC_NOTES				= 12;
static const uint32 GUILD_PERMISSION_BANK_DEPOSIT_ITEMS				= 13;
static const uint32 GUILD_PERMISSION_BANK_WITHDRAW_ITEMS			= 14;
static const uint32 GUILD_PERMISSION_BANK_VIEW_ITEMS				= 15;
static const uint32 GUILD_PERMISSION_BANK_PROMOTE_ITEMS				= 16;
static const uint32 GUILD_PERMISSION_BANK_CHANGE_ITEM_PERMISSIONS		= 17;
static const uint32 GUILD_PERMISSION_CHANGE_MOTD				= 18;
static const uint32 GUILD_PERMISSION_GUILD_CHAT_SEE				= 19;
static const uint32 GUILD_PERMISSION_GUILD_CHAT_SPEAK				= 20;
static const uint32 GUILD_PERMISSION_GUILD_EMAIL				= 21;
static const uint32 GUILD_PERMISSION_TRIBUTE_CHANGE_OTHERS			= 22;
static const uint32 GUILD_PERMISSION_TRIBUTE_CHANGE_ACTIVE_BENEFITS		= 23;
static const uint32 GUILD_PERMISSION_TROPHY_TRIBUTE_CHANGE_OTHERS		= 24;
static const uint32 GUILD_PERMISSION_TROPHY_TRIBUTE_CHANGE_ACTIVE_BENEFITS	= 25;
static const uint32 GUILD_PERMISSION_CHANGE_ALT_FLAG				= 26;
static const uint32 GUILD_PERMISSION_REAL_ESTATE_GUILD_PLOT_BUY			= 27;
static const uint32 GUILD_PERMISSION_REAL_ESTATE_GUILD_PLOT_SELL			= 28;
static const uint32 GUILD_PERMISSION_REAL_ESTATE_MODIFY_TROPHIES			= 29;
static const uint32 GUILD_PERMISSION_MEMBERS_DEMOTE_SELF				= 30;
static const uint32 GUILD_PERMISSION_MAX = GUILD_PERMISSION_MEMBERS_DEMOTE_SELF;

static const uint64 GUILD_PERMISSION_BIT_BANNER_CHANGE				= 0x0000000000000001;
static const uint64 GUILD_PERMISSION_BIT_BANNER_PLANT				= 0x0000000000000002;
static const uint64 GUILD_PERMISSION_BIT_BANNER_REMOVE				= 0x0000000000000004;
static const uint64 GUILD_PERMISSION_BIT_DISPLAY_GUILD_NAME			= 0x0000000000000008;
static const uint64 GUILD_PERMISSION_BIT_RANKS_CHANGE_PERMISSIONS		= 0x0000000000000010;
static const uint64 GUILD_PERMISSION_BIT_RANKS_CHANGE_NAMES			= 0x0000000000000020;
static const uint64 GUILD_PERMISSION_BIT_MEMBERS_INVITE				= 0x0000000000000040;
static const uint64 GUILD_PERMISSION_BIT_MEMBERS_PROMOTE			= 0x0000000000000080;
static const uint64 GUILD_PERMISSION_BIT_MEMBERS_DEMOTE				= 0x0000000000000100;
static const uint64 GUILD_PERMISSION_BIT_MEMBERS_REMOVE				= 0x0000000000000200;
static const uint64 GUILD_PERMISSION_BIT_EDIT_RECRUITING_SETTINGS		= 0x0000000000000400;
static const uint64 GUILD_PERMISSION_BIT_EDIT_PUBLIC_NOTES			= 0x0000000000000800;
static const uint64 GUILD_PERMISSION_BIT_BANK_DEPOSIT_ITEMS			= 0x0000000000001000;
static const uint64 GUILD_PERMISSION_BIT_BANK_WITHDRAW_ITEMS			= 0x0000000000002000;
static const uint64 GUILD_PERMISSION_BIT_BANK_VIEW_ITEMS			= 0x0000000000004000;
static const uint64 GUILD_PERMISSION_BIT_BANK_PROMOTE_ITEMS			= 0x0000000000008000;
static const uint64 GUILD_PERMISSION_BIT_BANK_CHANGE_ITEM_PERMISSIONS		= 0x0000000000010000;
static const uint64 GUILD_PERMISSION_BIT_CHANGE_MOTD				= 0x0000000000020000;
static const uint64 GUILD_PERMISSION_BIT_GUILD_CHAT_SEE				= 0x0000000000040000;
static const uint64 GUILD_PERMISSION_BIT_GUILD_CHAT_SPEAK			= 0x0000000000080000;
static const uint64 GUILD_PERMISSION_BIT_GUILD_EMAIL				= 0x0000000000100000;
static const uint64 GUILD_PERMISSION_BIT_TRIBUTE_CHANGE_OTHERS			= 0x0000000000200000;
static const uint64 GUILD_PERMISSION_BIT_TRIBUTE_CHANGE_ACTIVE_BENEFITS		= 0x0000000000400000;
static const uint64 GUILD_PERMISSION_BIT_TROPHY_TRIBUTE_CHANGE_OTHERS		= 0x0000000000800000;
static const uint64 GUILD_PERMISSION_BIT_TROPHY_TRIBUTE_CHANGE_ACTIVE_BENEFITS	= 0x0000000001000000;
static const uint64 GUILD_PERMISSION_BIT_CHANGE_ALT_FLAG			= 0x0000000002000000;
static const uint64 GUILD_PERMISSION_BIT_REAL_ESTATE_GUILD_PLOT_BUY		= 0x0000000004000000;
static const uint64 GUILD_PERMISSION_BIT_REAL_ESTATE_GUILD_PLOT_SELL		= 0x0000000008000000;
static const uint64 GUILD_PERMISSION_BIT_REAL_ESTATE_MODIFY_TROPHIES		= 0x0000000010000000;
static const uint64 GUILD_PERMISSION_BIT_MEMBERS_DEMOTE_SELF			= 0x0000000020000000;

struct GuildPermissions
{
	union
	{
		struct
		{
			unsigned int BannerChange:1;
			unsigned int BannerPlant:1;
			unsigned int BannerRemove:1;
			unsigned int DisplayGuildName:1;
			unsigned int RanksChangePermissions:1;
			unsigned int RanksChangeNames:1;
			unsigned int MembersInvite:1;
			unsigned int MembersPromote:1;
			unsigned int MembersDemote:1;
			unsigned int MembersRemove:1;
			unsigned int EditRecruitingSettings:1;
			unsigned int EditPublicNotes:1;
			unsigned int BankDepositItems:1;
			unsigned int BankWithdrawItems:1;
			unsigned int BankViewItems:1;
			unsigned int BankPromoteItems:1;
			unsigned int BankChangeItemPermissions:1;
			unsigned int ChangeMOTD:1;
			unsigned int GuildChatSee:1;
			unsigned int GuildChatSpeak:1;
			unsigned int SendGuildEMail:1;
			unsigned int TributeChangeForOthers:1;
			unsigned int TributeChangeActiveBenefit:1;
			unsigned int TrophyTributeChangeForOthers:1;
			unsigned int TrophyTributeChangeActiveBenefit:1;
			unsigned int ChangeAltFlagForOthers:1;
			unsigned int RealEstateGuildPlotBuy:1;
			unsigned int RealEstateGuildPlotSell:1;
			unsigned int RealEstateModifyTrophies:1;
			unsigned int MembersDemoteSelf:1;
			uint64 Unused:34;
		};
		uint64 Bits;
	};
};
/*
typedef enum {
	GUILD_HEAR		= 0,
	GUILD_SPEAK		= 1,
	GUILD_INVITE	= 2,
	GUILD_REMOVE	= 3,
	GUILD_PROMOTE	= 4,
	GUILD_DEMOTE	= 5,
	GUILD_MOTD		= 6,
	GUILD_WARPEACE	= 7,
	_MaxGuildAction
} GuildAction;
*/

#endif
