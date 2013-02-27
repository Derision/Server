/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2006  EQEMu Development Team (http://eqemulator.net)

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

#include "debug.h"
#include "guild_base.h"
#include "database.h"
#include "logsys.h"
#include "MiscFunctions.h"
#include <cstdlib>
#include <cstring>

//until we move MAX_NUMBER_GUILDS
#include "eq_packet_structs.h"

const char *const BaseGuildManager::GuildActionNames[GUILD_PERMISSION_MAX] =
{
"BANNER_CHANGE",
"BANNER_PLANT",
"BANNER_REMOVE",
"DISPLAY_GUILD_NAME",
"RANKS_CHANGE_PERMISSIONS",
"RANKS_CHANGE_NAMES",
"MEMBERS_INVITE",
"MEMBERS_PROMOTE",
"MEMBERS_DEMOTE",
"MEMBERS_REMOVE",
"EDIT_RECRUITING_SETTINGS",
"EDIT_PUBLIC_NOTES",
"BANK_DEPOSIT_ITEMS",
"BANK_WITHDRAW_ITEMS",
"BANK_VIEW_ITEMS",
"BANK_PROMOTE_ITEMS",
"BANK_CHANGE_ITEM_PERMISSIONS",
"CHANGE_MOTD",
"GUILD_CHAT_SEE",
"GUILD_CHAT_SPEAK",
"GUILD_EMAIL",
"TRIBUTE_CHANGE_OTHERS",
"TRIBUTE_CHANGE_ACTIVE_BENEFITS",
"TROPHY_TRIBUTE_CHANGE_OTHERS",
"TROPHY_TRIBUTE_CHANGE_ACTIVE_BENEFITS",
"CHANGE_ALT_FLAG",
"REAL_ESTATE_GUILD_PLOT_BUY",
"REAL_ESTATE_GUILD_PLOT_SELL",
"REAL_ESTATE_MODIFY_TROPHIES",
"MEMBERS_DEMOTE_SELF"
};

BaseGuildManager::BaseGuildManager()
: m_db(NULL)
{
}

BaseGuildManager::~BaseGuildManager() {
	ClearGuilds();
}

bool BaseGuildManager::LoadGuilds() {
	
	ClearGuilds();
	
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to load guilds when we have no database object.");
		return(false);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	map<uint32, GuildInfo *>::iterator res;
	
	// load up all the guilds
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT id, name, leader, minstatus, motd, motd_setter,channel,url FROM guilds"), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error loading guilds '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	while ((row = mysql_fetch_row(result))) {
		_CreateGuild(atoi(row[0]), row[1], atoi(row[2]), atoi(row[3]), row[4], row[5], row[6], row[7]);
	}
	mysql_free_result(result);
	
	//load up the rank info for each guild.
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT guild_id,rank,title,permissions FROM guild_ranks"), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error loading guild ranks '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	while ((row = mysql_fetch_row(result))) {
		uint32 guild_id = atoi(row[0]);
		uint8 rankn = atoi(row[1]);
		if(rankn > GUILD_MAX_RANK) {
			_log(GUILDS__ERROR, "Found invalid (too high) rank %d for guild %d, skipping.", rankn, guild_id);
			continue;
		}
		
		res = m_guilds.find(guild_id);
		if(res == m_guilds.end()) {
			_log(GUILDS__ERROR, "Found rank %d for non-existent guild %d, skipping.", rankn, guild_id);
			continue;
		}
		
		RankInfo &rank = res->second->ranks[rankn];
		
		rank.name = row[2];
		rank.permissions.Bits = atoi(row[3]);
		/*
		rank.permissions[GUILD_HEAR] = (row[3][0] == '1')?true:false;
		rank.permissions[GUILD_SPEAK] = (row[4][0] == '1')?true:false;
		rank.permissions[GUILD_INVITE] = (row[5][0] == '1')?true:false;
		rank.permissions[GUILD_REMOVE] = (row[6][0] == '1')?true:false;
		rank.permissions[GUILD_PROMOTE] = (row[7][0] == '1')?true:false;
		rank.permissions[GUILD_DEMOTE] = (row[8][0] == '1')?true:false;
		rank.permissions[GUILD_MOTD] = (row[9][0] == '1')?true:false;
		rank.permissions[GUILD_WARPEACE] = (row[10][0] == '1')?true:false
		*/
	}
	mysql_free_result(result);
	
	return(true);
}

bool BaseGuildManager::RefreshGuild(uint32 guild_id) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to refresh guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	map<uint32, GuildInfo *>::iterator res;
	GuildInfo *info;
	
	// load up all the guilds
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT name, leader, minstatus, motd, motd_setter, channel,url FROM guilds WHERE id=%lu", (unsigned long)guild_id), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error reloading guilds '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	if ((row = mysql_fetch_row(result))) {
		//delete the old entry and create the new one.
		info = _CreateGuild(guild_id, row[0], atoi(row[1]), atoi(row[2]), row[3], row[4], row[5], row[6]);
	} else {
		_log(GUILDS__ERROR, "Unable to find guild %d in the database.", guild_id);
		return(false);
	}
	mysql_free_result(result);
	
	//load up the rank info for each guild.
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT guild_id,rank,title,permissions "
		"FROM guild_ranks WHERE guild_id=%lu", (unsigned long)guild_id), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error reloading guild ranks '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	while((row = mysql_fetch_row(result))) {
		uint8 rankn = atoi(row[1]);
		if(rankn > GUILD_MAX_RANK) {
			_log(GUILDS__ERROR, "Found invalid (too high) rank %d for guild %d, skipping.", rankn, guild_id);
			continue;
		}
		RankInfo &rank = info->ranks[rankn];
		
		rank.name = row[2];
		rank.permissions.Bits = atoi(row[3]);
		/*
		rank.permissions[GUILD_HEAR] = (row[3][0] == '1')?true:false;
		rank.permissions[GUILD_SPEAK] = (row[4][0] == '1')?true:false;
		rank.permissions[GUILD_INVITE] = (row[5][0] == '1')?true:false;
		rank.permissions[GUILD_REMOVE] = (row[6][0] == '1')?true:false;
		rank.permissions[GUILD_PROMOTE] = (row[7][0] == '1')?true:false;
		rank.permissions[GUILD_DEMOTE] = (row[8][0] == '1')?true:false;
		rank.permissions[GUILD_MOTD] = (row[9][0] == '1')?true:false;
		rank.permissions[GUILD_WARPEACE] = (row[10][0] == '1')?true:false;
		*/
	}
	mysql_free_result(result);
	
	_log(GUILDS__DB, "Successfully refreshed guild %d from the database.", guild_id);
	
	return(true);
}

BaseGuildManager::GuildInfo *BaseGuildManager::_CreateGuild(uint32 guild_id, const char *guild_name, uint32 leader_char_id, uint8 minstatus, const char *guild_motd, const char *motd_setter, const char *Channel, const char *URL)
{
	map<uint32, GuildInfo *>::iterator res;
	
	//remove any old entry.
	res = m_guilds.find(guild_id);
	if(res != m_guilds.end()) {
		delete res->second;
		m_guilds.erase(res);
	}
	
	//make the new entry and store it into the map.
	GuildInfo *info = new GuildInfo;
	info->name = guild_name;
	info->leader_char_id = leader_char_id;
	info->minstatus = minstatus;
	info->motd = guild_motd;
	info->motd_setter = motd_setter;
	info->url = URL;
	info->channel = Channel;
	m_guilds[guild_id] = info;
	
	//now setup default ranks (everything defaults to false)
	info->ranks[GUILD_RANK_RECRUIT].name = "Recruit";
	info->ranks[GUILD_RANK_RECRUIT].permissions.Bits =	GUILD_PERMISSION_BIT_DISPLAY_GUILD_NAME |
								GUILD_PERMISSION_BIT_GUILD_CHAT_SEE |
								GUILD_PERMISSION_BIT_GUILD_CHAT_SPEAK;	

	info->ranks[GUILD_RANK_INITIATE].name = "Initiate";
	info->ranks[GUILD_RANK_INITIATE].permissions.Bits = info->ranks[GUILD_RANK_RECRUIT].permissions.Bits;

	info->ranks[GUILD_RANK_JUNIOR_MEMBER].name = "Junior Member";
	info->ranks[GUILD_RANK_JUNIOR_MEMBER].permissions.Bits = info->ranks[GUILD_RANK_INITIATE].permissions.Bits;

	info->ranks[GUILD_RANK_MEMBER].name = "Member";
	info->ranks[GUILD_RANK_MEMBER].permissions.Bits = info->ranks[GUILD_RANK_JUNIOR_MEMBER].permissions.Bits;
	info->ranks[GUILD_RANK_MEMBER].permissions.BankDepositItems = 1;
	info->ranks[GUILD_RANK_MEMBER].permissions.BankViewItems = 1;
	
	info->ranks[GUILD_RANK_SENIOR_MEMBER].name = "Senior Member";
	info->ranks[GUILD_RANK_SENIOR_MEMBER].permissions.Bits = info->ranks[GUILD_RANK_MEMBER].permissions.Bits;

	info->ranks[GUILD_RANK_OFFICER].name = "Officer";
	info->ranks[GUILD_RANK_OFFICER].permissions.Bits = info->ranks[GUILD_RANK_SENIOR_MEMBER].permissions.Bits;
	info->ranks[GUILD_RANK_OFFICER].permissions.BannerChange = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.BannerPlant = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.BannerRemove = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.MembersInvite = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.MembersPromote = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.EditRecruitingSettings = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.EditPublicNotes = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.BankWithdrawItems = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.ChangeMOTD = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.SendGuildEMail = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.TributeChangeForOthers = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.TributeChangeActiveBenefit = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.TrophyTributeChangeForOthers = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.TrophyTributeChangeActiveBenefit = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.ChangeAltFlagForOthers = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.RealEstateModifyTrophies = 1;
	info->ranks[GUILD_RANK_OFFICER].permissions.MembersDemoteSelf = 1;

	info->ranks[GUILD_RANK_SENIOR_OFFICER].name = "Senior Officer";
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.Bits = info->ranks[GUILD_RANK_OFFICER].permissions.Bits;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.RanksChangePermissions = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.RanksChangeNames = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.MembersDemote = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.MembersRemove = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.BankPromoteItems = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.BankChangeItemPermissions = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.RealEstateGuildPlotBuy = 1;
	info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.RealEstateGuildPlotSell = 1;

	info->ranks[GUILD_RANK_LEADER].name = "Leader";
	info->ranks[GUILD_RANK_LEADER].permissions.Bits = info->ranks[GUILD_RANK_SENIOR_OFFICER].permissions.Bits;

	return(info);
}

bool BaseGuildManager::_StoreGuildDB(uint32 guild_id) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to store guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end()) {
		_log(GUILDS__DB, "Requested to store non-existent guild %d", guild_id);
		return(false);
	}
	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//clear out old `guilds` entry
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM guilds WHERE id=%lu", (unsigned long)guild_id), errbuf))
	{
		_log(GUILDS__ERROR, "Error clearing old guild record when storing %d '%s': %s", guild_id, query, errbuf);
	}
	safe_delete_array(query);
	
	//clear out old `guild_ranks` entries
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM guild_ranks WHERE guild_id=%lu", (unsigned long)guild_id), errbuf))
	{
		_log(GUILDS__ERROR, "Error clearing old guild_ranks records when storing %d '%s': %s", guild_id, query, errbuf);
	}
	safe_delete_array(query);
	
	//escape our strings.
	char *name_esc = new char[info->name.length()*2+1];
	char *motd_esc = new char[info->motd.length()*2+1];
	char *motd_set_esc = new char[info->motd_setter.length()*2+1];
	m_db->DoEscapeString(name_esc, info->name.c_str(), info->name.length());
	m_db->DoEscapeString(motd_esc, info->motd.c_str(), info->motd.length());
	m_db->DoEscapeString(motd_set_esc, info->motd_setter.c_str(), info->motd_setter.length());
	
	//insert the new `guilds` entry
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"INSERT INTO guilds (id,name,leader,minstatus,motd,motd_setter) VALUES(%lu,'%s',%lu,%d,'%s', '%s')", 
		(unsigned long)guild_id, name_esc, (unsigned long)info->leader_char_id, info->minstatus, motd_esc, motd_set_esc), errbuf))
	{
		_log(GUILDS__ERROR, "Error inserting new guild record when storing %d. Giving up. '%s': %s", guild_id, query, errbuf);
		safe_delete_array(query);
		safe_delete_array(name_esc);
		safe_delete_array(motd_esc);
		safe_delete_array(motd_set_esc);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(name_esc);
	safe_delete_array(motd_esc);
	safe_delete_array(motd_set_esc);
	
	//now insert the new ranks
	uint8 rank;
	for(rank = 1; rank <= GUILD_MAX_RANK; rank++) {
		const RankInfo &r = info->ranks[rank];
		
		char *title_esc = new char[r.name.length()*2+1];
		m_db->DoEscapeString(title_esc, r.name.c_str(), r.name.length());
		
		if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"INSERT INTO guild_ranks (guild_id,rank,title,permissions)"
		" VALUES(%d,%d,'%s',%d)",
			guild_id, rank, title_esc, r.permissions.Bits), errbuf))
		{
			_log(GUILDS__ERROR, "Error inserting new guild rank record when storing %d for %d. Giving up. '%s': %s", rank, guild_id, query, errbuf);
			safe_delete_array(query);
			safe_delete_array(title_esc);
			return(false);
		}
		safe_delete_array(query);
		safe_delete_array(title_esc);
	}
	
	_log(GUILDS__DB, "Stored guild %d in the database", guild_id);
	
	return(true);
}

uint32 BaseGuildManager::_GetFreeGuildID() {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested find a free guild ID when we have no database object.");
		return(GUILD_NONE);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[100];
    MYSQL_RES *result;
	
	//this has got to be one of the more retarded things I have seen.
	//none the less, im too lazy to rewrite it right now.
	
	uint16 x;
	for (x = 1; x < MAX_NUMBER_GUILDS; x++) {
		snprintf(query, 100, "SELECT id FROM guilds where id=%i;", x);
		
		if (m_db->RunQuery(query, strlen(query), errbuf, &result)) {
			if (mysql_num_rows(result) == 0) {
				mysql_free_result(result);
				_log(GUILDS__DB, "Located free guild ID %d in the database", x);
				return x;
			}
			mysql_free_result(result);
		}
		else {
			LogFile->write(EQEMuLog::Error, "Error in _GetFreeGuildID query '%s': %s", query, errbuf);
		}
	}
	
	_log(GUILDS__ERROR, "Unable to find a free guild ID when requested.");
	return GUILD_NONE;
}


	
uint32 BaseGuildManager::CreateGuild(const char* name, uint32 leader_char_id) {
	uint32 gid = DBCreateGuild(name, leader_char_id);
	if(gid == GUILD_NONE)
		return(GUILD_NONE);
	
	SendGuildRefresh(gid, true, false, false, false);
	SendCharRefresh(GUILD_NONE, gid, leader_char_id);
	
	return(gid);
}

bool BaseGuildManager::DeleteGuild(uint32 guild_id) {
	if(!DBDeleteGuild(guild_id))
		return(false);
	
	SendGuildDelete(guild_id);
	
	return(true);
}

bool BaseGuildManager::RenameGuild(uint32 guild_id, const char* name) {
	if(!DBRenameGuild(guild_id, name))
		return(false);
	
	SendGuildRefresh(guild_id, true, false, false, false);
	
	return(true);
}

bool BaseGuildManager::SetGuildLeader(uint32 guild_id, uint32 leader_char_id) {
	//get old leader first.
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);
	GuildInfo *info = res->second;
	uint32 old_leader = info->leader_char_id;
	
	if(!DBSetGuildLeader(guild_id, leader_char_id))
		return(false);
	
	SendGuildRefresh(guild_id, false, false, false, false);
	SendCharRefresh(GUILD_NONE, guild_id, old_leader);
	SendCharRefresh(GUILD_NONE, guild_id, leader_char_id);
	
	return(true);
}

bool BaseGuildManager::SetGuildMOTD(uint32 guild_id, const char* motd, const char *setter) {
	if(!DBSetGuildMOTD(guild_id, motd, setter))
		return(false);
	
	SendGuildRefresh(guild_id, false, true, false, false);
	
	return(true);
}

bool BaseGuildManager::SetGuildURL(uint32 GuildID, const char* URL)
{
	if(!DBSetGuildURL(GuildID, URL))
		return(false);
	
	SendGuildRefresh(GuildID, false, true, false, false);
	
	return(true);
}

bool BaseGuildManager::SetGuildChannel(uint32 GuildID, const char* Channel)
{
	if(!DBSetGuildChannel(GuildID, Channel))
		return(false);
	
	SendGuildRefresh(GuildID, false, true, false, false);
	
	return(true);
}

bool BaseGuildManager::SetGuild(uint32 charid, uint32 guild_id, uint8 rank) {
	if(rank > GUILD_MAX_RANK && guild_id != GUILD_NONE)
		return(false);
	
	//lookup their old guild, if they had one.
	uint32 old_guild = GUILD_NONE;
	CharGuildInfo gci;
	if(GetCharInfo(charid, gci)) {
		old_guild = gci.guild_id;
	}
	
	if(!DBSetGuild(charid, guild_id, rank))
		return(false);
	
	SendCharRefresh(old_guild, guild_id, charid);
	
	return(true);
}

//changes rank, but not guild.
bool BaseGuildManager::SetGuildRank(uint32 charid, uint8 rank) {
	if(rank > GUILD_MAX_RANK)
		return(false);
	
	if(!DBSetGuildRank(charid, rank))
		return(false);
	
	SendCharRefresh(GUILD_NONE, 0, charid);
	
	return(true);
}


bool BaseGuildManager::SetBankerFlag(uint32 charid, bool is_banker) {
	if(!DBSetBankerFlag(charid, is_banker))
		return(false);
	
	SendRankUpdate(charid);
	
	return(true);
}

bool BaseGuildManager::SetAltFlag(uint32 charid, bool is_alt)
{
	if(!DBSetAltFlag(charid, is_alt))
		return(false);
	
	SendRankUpdate(charid);

	return(true);
}

bool BaseGuildManager::SetTributeFlag(uint32 charid, bool enabled) {
	if(!DBSetTributeFlag(charid, enabled))
		return(false);
	
	SendCharRefresh(GUILD_NONE, 0, charid);
	
	return(true);
}

bool BaseGuildManager::SetPublicNote(uint32 charid, const char *note) {
	if(!DBSetPublicNote(charid, note))
		return(false);
	
	SendCharRefresh(GUILD_NONE, 0, charid);
	
	return(true);
}

uint32 BaseGuildManager::DBCreateGuild(const char* name, uint32 leader) {
	//first try to find a free ID.
	uint32 new_id = _GetFreeGuildID();
	if(new_id == GUILD_NONE)
		return(GUILD_NONE);
	
	//now make the guild record in our local manager.
	//this also sets up the default ranks for us.
	_CreateGuild(new_id, name, leader, 0, "", "", "", "");
	
	//now store the resulting guild setup into the DB.
	if(!_StoreGuildDB(new_id)) {
		_log(GUILDS__ERROR, "Error storing new guild. It may have been partially created which may need manual removal.");
		return(GUILD_NONE);
	}
	
	_log(GUILDS__DB, "Created guild %d in the database.", new_id);
	
	return(new_id);
}

bool BaseGuildManager::DBDeleteGuild(uint32 guild_id) {
	
	//remove the local entry
	map<uint32, GuildInfo *>::iterator res;
	res = m_guilds.find(guild_id);
	if(res != m_guilds.end()) {
		delete res->second;
		m_guilds.erase(res);
	}
	
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to delete guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	char *query = 0;
	
	//clear out old `guilds` entry
	_RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM guilds WHERE id=%lu", (unsigned long)guild_id), "clearing old guild record");
	
	//clear out old `guild_ranks` entries
	_RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM guild_ranks WHERE guild_id=%lu", (unsigned long)guild_id), "clearing old guild_ranks records");
	
	//clear out people belonging to this guild.
	_RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM guild_members WHERE guild_id=%lu", (unsigned long)guild_id), "clearing chars in guild");
	
	// Delete the guild bank
	_RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM guild_bank WHERE guildid=%lu", (unsigned long)guild_id), "deleting guild bank");
	
	_log(GUILDS__DB, "Deleted guild %d from the database.", guild_id);
	
	return(true);
}

bool BaseGuildManager::DBRenameGuild(uint32 guild_id, const char* name) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to rename guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);
	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//escape our strings.
	uint32 len = strlen(name);
	char *esc = new char[len*2+1];
	m_db->DoEscapeString(esc, name, len);
	
	//insert the new `guilds` entry
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guilds SET name='%s' WHERE id=%d", 
		esc, guild_id), errbuf))
	{
		_log(GUILDS__ERROR, "Error renaming guild %d '%s': %s", guild_id, query, errbuf);
		safe_delete_array(query);
		safe_delete_array(esc);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(esc);
	
	_log(GUILDS__DB, "Renamed guild %s (%d) to %s in database.", info->name.c_str(), guild_id, name);
	
	info->name = name;	//update our local record.
	
	return(true);
}

bool BaseGuildManager::DBSetGuildLeader(uint32 guild_id, uint32 leader) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to set the leader for guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);
	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//insert the new `guilds` entry
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guilds SET leader='%d' WHERE id=%d", 
		leader, guild_id), errbuf))
	{
		_log(GUILDS__ERROR, "Error changing leader on guild %d '%s': %s", guild_id, query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	//set the old leader to officer
	if(!DBSetGuildRank(info->leader_char_id, GUILD_RANK_SENIOR_OFFICER))
		return(false);
	//set the new leader to leader
	if(!DBSetGuildRank(leader, GUILD_RANK_LEADER))
		return(false);
	
	_log(GUILDS__DB, "Set guild leader for guild %d to %d in the database", guild_id, leader);
	
	info->leader_char_id = leader;	//update our local record.
	
	return(true);
}

bool BaseGuildManager::DBSetGuildMOTD(uint32 guild_id, const char* motd, const char *setter) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to set the MOTD for guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);
	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//escape our strings.
	uint32 len = strlen(motd);
	uint32 len2 = strlen(setter);
	char *esc = new char[len*2+1];
	char *esc_set = new char[len2*2+1];
	m_db->DoEscapeString(esc, motd, len);
	m_db->DoEscapeString(esc_set, setter, len2);
	
	//insert the new `guilds` entry
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guilds SET motd='%s',motd_setter='%s' WHERE id=%d", 
		esc, esc_set, guild_id), errbuf))
	{
		_log(GUILDS__ERROR, "Error setting MOTD for guild %d '%s': %s", guild_id, query, errbuf);
		safe_delete_array(query);
		safe_delete_array(esc);
		safe_delete_array(esc_set);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(esc);
	safe_delete_array(esc_set);
	
	_log(GUILDS__DB, "Set MOTD for guild %d in the database", guild_id);
	
	info->motd = motd;	//update our local record.
	info->motd_setter = setter;	//update our local record.
	
	return(true);
}

bool BaseGuildManager::DBSetGuildURL(uint32 GuildID, const char* URL)
{
	if(m_db == NULL)
		return(false);
	
	map<uint32, GuildInfo *>::const_iterator res;

	res = m_guilds.find(GuildID);

	if(res == m_guilds.end())
		return(false);

	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//escape our strings.
	uint32 len = strlen(URL);

	char *esc = new char[len*2+1];

	m_db->DoEscapeString(esc, URL, len);
	
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, "UPDATE guilds SET url='%s' WHERE id=%d", esc, GuildID), errbuf))
	{
		_log(GUILDS__ERROR, "Error setting URL for guild %d '%s': %s", GuildID, query, errbuf);
		safe_delete_array(query);
		safe_delete_array(esc);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(esc);
	
	_log(GUILDS__DB, "Set URL for guild %d in the database", GuildID);
	
	info->url = URL;	//update our local record.
	
	return(true);
}

bool BaseGuildManager::DBSetGuildChannel(uint32 GuildID, const char* Channel)
{
	if(m_db == NULL)
		return(false);
	
	map<uint32, GuildInfo *>::const_iterator res;

	res = m_guilds.find(GuildID);

	if(res == m_guilds.end())
		return(false);

	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//escape our strings.
	uint32 len = strlen(Channel);

	char *esc = new char[len*2+1];

	m_db->DoEscapeString(esc, Channel, len);
	
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, "UPDATE guilds SET channel='%s' WHERE id=%d", esc, GuildID), errbuf))
	{
		_log(GUILDS__ERROR, "Error setting Channel for guild %d '%s': %s", GuildID, query, errbuf);
		safe_delete_array(query);
		safe_delete_array(esc);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(esc);
	
	_log(GUILDS__DB, "Set Channel for guild %d in the database", GuildID);
	
	info->channel = Channel;	//update our local record.
	
	return(true);
}

bool BaseGuildManager::DBSetGuild(uint32 charid, uint32 guild_id, uint8 rank) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested to set char to guild %d when we have no database object.", guild_id);
		return(false);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	if(guild_id != GUILD_NONE) {
		if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
			"REPLACE INTO guild_members (char_id,guild_id,rank) VALUES(%d,%d,%d)", 
			charid, guild_id, rank), errbuf))
		{
			_log(GUILDS__ERROR, "Error Changing char %d to guild %d '%s': %s", charid, guild_id, query, errbuf);
			safe_delete_array(query);
			return(false);
		}
	} else {
		if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
			"DELETE FROM guild_members WHERE char_id=%d", 
			charid), errbuf))
		{
			_log(GUILDS__ERROR, "Error removing char %d from guild '%s': %s", charid, guild_id, query, errbuf);
			safe_delete_array(query);
			return(false);
		}
	}
	safe_delete_array(query);
	
	_log(GUILDS__DB, "Set char %d to guild %d and rank %d in the database.", charid, guild_id, rank);
	
	return(true);
}

bool BaseGuildManager::DBSetGuildRank(uint32 charid, uint8 rank) {
	char *query = 0;
	return(_RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guild_members SET rank=%d WHERE char_id=%d", 
		rank, charid), "setting a guild member's rank"));
}

bool BaseGuildManager::DBSetBankerFlag(uint32 charid, bool is_banker) {
	char *query = 0;
	return(_RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guild_members SET banker=%d WHERE char_id=%d", 
		is_banker?1:0, charid), "setting a guild member's banker flag"));
}

bool BaseGuildManager::GetBankerFlag(uint32 CharID)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(!m_db)
		return false;

	if(!m_db->RunQuery(query, MakeAnyLenString(&query, "select `banker` from `guild_members` where char_id=%i LIMIT 1", CharID), errbuf, &result))
	{
		_log(GUILDS__ERROR, "Error retrieving banker flag '%s': %s", query, errbuf);

		safe_delete_array(query);

		return false;
	}

	safe_delete_array(query);

	if(mysql_num_rows(result) != 1)
		return false;	

	row = mysql_fetch_row(result);

	bool IsBanker = atoi(row[0]);

	mysql_free_result(result);

	return IsBanker;
}

bool BaseGuildManager::DBSetAltFlag(uint32 charid, bool is_alt)
{
	char *query = 0;

	return(_RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guild_members SET alt=%d WHERE char_id=%d", 
		is_alt?1:0, charid), "setting a guild member's alt flag"));
}

bool BaseGuildManager::GetAltFlag(uint32 CharID)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if(!m_db)
		return false;

	if(!m_db->RunQuery(query, MakeAnyLenString(&query, "select `alt` from `guild_members` where char_id=%i LIMIT 1", CharID), errbuf, &result))
	{
		_log(GUILDS__ERROR, "Error retrieving alt flag '%s': %s", query, errbuf);

		safe_delete_array(query);

		return false;
	}

	safe_delete_array(query);

	if(mysql_num_rows(result) != 1)
		return false;	

	row = mysql_fetch_row(result);

	bool IsAlt = atoi(row[0]);

	mysql_free_result(result);

	return IsAlt;
}

bool BaseGuildManager::DBSetTributeFlag(uint32 charid, bool enabled) {
	char *query = 0;
	return(_RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guild_members SET tribute_enable=%d WHERE char_id=%d", 
		enabled?1:0, charid), "setting a guild member's tribute flag"));
}

bool BaseGuildManager::DBSetPublicNote(uint32 charid, const char* note) {
	if(m_db == NULL)
		return(false);
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	//escape our strings.
	uint32 len = strlen(note);
	char *esc = new char[len*2+1];
	m_db->DoEscapeString(esc, note, len);
	
	//insert the new `guilds` entry
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE guild_members SET public_note='%s' WHERE char_id=%d", 
		esc, charid), errbuf))
	{
		_log(GUILDS__ERROR, "Error setting public note for char %d '%s': %s", charid, query, errbuf);
		safe_delete_array(query);
		safe_delete_array(esc);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(esc);
	
	_log(GUILDS__DB, "Set public not for char %d", charid);
	
	return(true);
}

bool BaseGuildManager::_RunQuery(char *&query, int len, const char *errmsg) {
	if(m_db == NULL)
		return(false);
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	
	if (!m_db->RunQuery(query, len, errbuf))
	{
		_log(GUILDS__ERROR, "Error %s: '%s': %s", errmsg, query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	return(true);
}

//factored out so I dont have to copy this crap.
#ifdef BOTS
#define GuildMemberBaseQuery \
"SELECT c.id,c.name,c.class,c.level,c.timelaston,c.zoneid," \
" g.guild_id,g.rank,g.tribute_enable,g.total_tribute,g.last_tribute," \
" g.banker,g.public_note,g.alt" \
" FROM vwBotCharacterMobs AS c LEFT JOIN vwGuildMembers AS g ON c.id=g.char_id AND c.mobtype = g.mobtype "
#else
#define GuildMemberBaseQuery \
"SELECT c.id,c.name,c.class,c.level,c.timelaston,c.zoneid," \
" g.guild_id,g.rank,g.tribute_enable,g.total_tribute,g.last_tribute," \
" g.banker,g.public_note,g.alt " \
" FROM character_ AS c LEFT JOIN guild_members AS g ON c.id=g.char_id "
#endif
static void ProcessGuildMember(MYSQL_ROW &row, CharGuildInfo &into) {
	//fields from `characer_`
	into.char_id 		= atoi(row[0]);
	into.char_name 		= row[1];
	into.class_ 		= atoi(row[2]);
	into.level 			= atoi(row[3]);
	into.time_last_on 	= atoul(row[4]);
	into.zone_id 		= atoi(row[5]);
	
	//fields from `guild_members`, leave at defaults if missing
	into.guild_id 		= row[6] ? atoi(row[6]) : GUILD_NONE;
	into.rank 			= row[7] ? atoi(row[7]) : (GUILD_MAX_RANK+1);
	into.tribute_enable = row[8] ? (row[8][0] == '0'?false:true) : false;
	into.total_tribute	= row[9] ? atoi(row[9]) : 0;
	into.last_tribute	= row[10]? atoul(row[10]) : 0;		//timestamp
	into.banker 		= row[11]? (row[11][0] == '0'?false:true) : false;
	into.public_note	= row[12]? row[12] : "";
	into.alt 		= row[13]? (row[13][0] == '0'?false:true) : false;
	
	//a little sanity checking/cleanup
	if(into.guild_id == 0)
		into.guild_id = GUILD_NONE;
	if(into.rank > GUILD_MAX_RANK)
		into.rank = GUILD_RANK_NONE;
}


bool BaseGuildManager::GetEntireGuild(uint32 guild_id, vector<CharGuildInfo *> &members) {
	members.clear();
	
	if(m_db == NULL)
		return(false);
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//load up the rank info for each guild.
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		GuildMemberBaseQuery " WHERE g.guild_id=%d", guild_id
		), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error loading guild member list '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	while ((row = mysql_fetch_row(result))) {
		CharGuildInfo *ci = new CharGuildInfo;
		ProcessGuildMember(row, *ci);
		members.push_back(ci);
	}
	mysql_free_result(result);
	
	_log(GUILDS__DB, "Retreived entire guild member list for guild %d from the database", guild_id);
	
	return(true);
}

std::map<uint32, std::string> BaseGuildManager::GetGuildBankers(uint32 GuildID)
{
	std::map<uint32, std::string> Bankers;

	if(m_db == NULL)
		return Bankers;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT c.id, c.name FROM character_ AS c LEFT JOIN guild_members AS g ON c.id=g.char_id WHERE g.guild_id = %u and g.banker = 1", GuildID),
		errbuf, &result))
	{
		_log(GUILDS__ERROR, "Error querying guild bankers '%s': %s", query, errbuf);
		safe_delete_array(query);
		return Bankers;
	}
	safe_delete_array(query);
	
	while ((row = mysql_fetch_row(result)))
	{
		Bankers[atoi(row[0])] = row[1];
	}
	mysql_free_result(result);
	
	return Bankers;
}

std::map<uint32, std::string> BaseGuildManager::GetEntireGuild(uint32 GuildID)
{
	std::map<uint32, std::string> Members;

	if(m_db == NULL)
		return Members;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT c.id, c.name FROM character_ AS c LEFT JOIN guild_members AS g ON c.id=g.char_id WHERE g.guild_id = %u", GuildID),
		errbuf, &result))
	{
		_log(GUILDS__ERROR, "Error querying guild members '%s': %s", query, errbuf);
		safe_delete_array(query);
		return Members;
	}
	safe_delete_array(query);
	
	while ((row = mysql_fetch_row(result)))
	{
		Members[atoi(row[0])] = row[1];
	}
	mysql_free_result(result);
	
	return Members;
}

bool BaseGuildManager::GetCharInfo(const char *char_name, CharGuildInfo &into) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested char info on %s when we have no database object.", char_name);
		return(false);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//escape our strings.
	uint32 nl = strlen(char_name);
	char *esc = new char[nl*2+1];
	m_db->DoEscapeString(esc, char_name, nl);
	
	//load up the rank info for each guild.
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
		GuildMemberBaseQuery " WHERE c.name='%s'", esc
		), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error loading guild member '%s': %s", query, errbuf);
		safe_delete_array(query);
		safe_delete_array(esc);
		return(false);
	}
	safe_delete_array(query);
	safe_delete_array(esc);
	
	bool ret = true;
	if ((row = mysql_fetch_row(result))) {
		ProcessGuildMember(row, into);
		_log(GUILDS__DB, "Retreived guild member info for char %s from the database", char_name);
	} else {
		ret = true;
	}
	mysql_free_result(result);
	
	return(ret);
	
	
}

bool BaseGuildManager::GetCharInfo(uint32 char_id, CharGuildInfo &into) {
	if(m_db == NULL) {
		_log(GUILDS__DB, "Requested char info on %d when we have no database object.", char_id);
		return(false);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//load up the rank info for each guild.
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, 
#ifdef BOTS
		GuildMemberBaseQuery " WHERE c.id=%d AND c.mobtype = 'C'", char_id
#else
		GuildMemberBaseQuery " WHERE c.id=%d", char_id
#endif
		), errbuf, &result)) {
		_log(GUILDS__ERROR, "Error loading guild member '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	bool ret = true;
	if ((row = mysql_fetch_row(result))) {
		ProcessGuildMember(row, into);
		_log(GUILDS__DB, "Retreived guild member info for char %d", char_id);
	} else {
		ret = true;
	}
	mysql_free_result(result);
	
	return(ret);
	
}

//returns ownership of the buffer.
uint8 *BaseGuildManager::MakeGuildList(const char *head_name, uint32 &length) const {
	//dynamic structs will make this a lot less painful.
	
	length = sizeof(GuildsList_Struct);
	uint8 *buffer = new uint8[length];
	
	//a bit little better than memsetting the whole thing...
	uint32 r,pos;
	for(r = 0, pos = 0; r <= MAX_NUMBER_GUILDS; r++, pos += 64) {
		//strcpy((char *) buffer+pos, "BAD GUILD");
		// These 'BAD GUILD' entries were showing in the drop-downs for selecting guilds in the LFP window,
		// so just fill unused entries with an empty string instead.
		buffer[pos] = '\0';
	}
	
	strn0cpy((char *) buffer, head_name, 64);
	
	map<uint32, GuildInfo *>::const_iterator cur, end;
	cur = m_guilds.begin();
	end = m_guilds.end();
	for(; cur != end; cur++) {
		pos = 64 + (64 * cur->first);
		strn0cpy((char *) buffer + pos, cur->second->name.c_str(), 64);
	}
	return(buffer);
}

const char *BaseGuildManager::GetRankName(uint32 guild_id, uint8 rank) const {
	if(rank > GUILD_MAX_RANK)
		return("Invalid Rank");
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return("Invalid Guild Rank");
	return(res->second->ranks[rank].name.c_str());
}

const char *BaseGuildManager::GetGuildName(uint32 guild_id) const {
	if(guild_id == GUILD_NONE)
		return("");
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return("Invalid Guild");
	return(res->second->name.c_str());
}

bool BaseGuildManager::GetGuildNameByID(uint32 guild_id, std::string &into) const {
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);
	into = res->second->name;
	return(true);
}

uint32 BaseGuildManager::GetGuildIDByName(const char *GuildName)
{
	map<uint32, GuildInfo *>::iterator Iterator;

	for(Iterator = m_guilds.begin(); Iterator != m_guilds.end(); ++Iterator)
	{
		if(!strcasecmp((*Iterator).second->name.c_str(), GuildName))
			return (*Iterator).first;
	}

	return GUILD_NONE;
}

bool BaseGuildManager::GetGuildMOTD(uint32 guild_id, char *motd_buffer, char *setter_buffer) const {
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);
	strn0cpy(motd_buffer, res->second->motd.c_str(), 512);
	strn0cpy(setter_buffer, res->second->motd_setter.c_str(), 64);
	return(true);
}
	
bool BaseGuildManager::GetGuildURL(uint32 GuildID, char *URLBuffer) const
{
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
		return(false);
	strn0cpy(URLBuffer, res->second->url.c_str(), 512);

	return(true);
}
	
bool BaseGuildManager::GetGuildChannel(uint32 GuildID, char *ChannelBuffer) const
{
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
		return(false);
	strn0cpy(ChannelBuffer, res->second->channel.c_str(), 128);
	return(true);
}
	
bool BaseGuildManager::GuildExists(uint32 guild_id) const {
	if(guild_id == GUILD_NONE)
		return(false);
	return(m_guilds.find(guild_id) != m_guilds.end());
}

bool BaseGuildManager::IsGuildLeader(uint32 guild_id, uint32 char_id) const {
	if(guild_id == GUILD_NONE) {
		_log(GUILDS__PERMISSIONS, "Check leader for char %d: not a guild.", char_id);
		return(false);
	}
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end()) {
		_log(GUILDS__PERMISSIONS, "Check leader for char %d: invalid guild.", char_id);
		return(false);	//invalid guild
	}
	_log(GUILDS__PERMISSIONS, "Check leader for guild %d, char %d: leader id=%d", guild_id, char_id, res->second->leader_char_id);
	return(char_id == res->second->leader_char_id);
}

uint32 BaseGuildManager::FindGuildByLeader(uint32 leader) const {
	map<uint32, GuildInfo *>::const_iterator cur, end;
	cur = m_guilds.begin();
	end = m_guilds.end();
	for(; cur != end; cur++) {
		if(cur->second->leader_char_id == leader)
			return(cur->first);
	}
	return(GUILD_NONE);
}

//returns the rank to be sent to the client for display purposes, given their eqemu rank.
uint8 BaseGuildManager::GetDisplayedRank(uint32 guild_id, uint8 rank, uint32 char_id) const
{
	/*
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(3);	//invalid guild rank
	if (res->second->ranks[rank].permissions[GUILD_WARPEACE] || res->second->leader_char_id == char_id)
		return(2);	//leader rank
	else if (res->second->ranks[rank].permissions[GUILD_INVITE] || res->second->ranks[rank].permissions[GUILD_REMOVE] || res->second->ranks[rank].permissions[GUILD_MOTD])
		return(1);	//officer rank
	return(0);	//member rank
	*/
	return rank;
}

bool BaseGuildManager::CheckGMStatus(uint32 guild_id, uint8 status) const {
	if(status >= 250) {
		_log(GUILDS__PERMISSIONS, "Check permission on guild %d with user status %d > 250, granted.", guild_id, status);
		return(true);	//250+ as allowed anything
	}
	
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end()) {
		_log(GUILDS__PERMISSIONS, "Check permission on guild %d with user status %d, no such guild, denied.", guild_id, status);
		return(false);	//invalid guild
	}
	
	bool granted = (res->second->minstatus <= status);
	
	_log(GUILDS__PERMISSIONS, "Check permission on guild %s (%d) with user status %d. Min status %d: %s", 
		res->second->name.c_str(), guild_id, status, res->second->minstatus, granted?"granted":"denied");
	
	return(granted);
}

bool BaseGuildManager::CheckPermission(uint32 guild_id, uint8 rank, uint32 act) const {
	if(rank > GUILD_MAX_RANK) {
		_log(GUILDS__PERMISSIONS, "Check permission on guild %d and rank %d for action %s (%d): Invalid rank, denied.",
			guild_id, rank, GuildActionNames[act], act);
		return(false);	//invalid rank
	}
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end()) {
		_log(GUILDS__PERMISSIONS, "Check permission on guild %d and rank %d for action %s (%d): Invalid guild, denied.",
			guild_id, rank, GuildActionNames[act - 1], act);
		return(false);	//invalid guild
	}
	
	//bool granted = res->second->ranks[rank].permissions[act];
	bool granted = res->second->ranks[rank].permissions.Bits & (1 << (act -1));
	
	_log(GUILDS__PERMISSIONS, "Check permission on guild %s (%d) and rank %s (%d) for action %s (%d): %s",
		res->second->name.c_str(), guild_id, 
		res->second->ranks[rank].name.c_str(), rank, 
		GuildActionNames[act - 1], act, 
		granted?"granted":"denied");
	
	return(granted);
}

bool BaseGuildManager::LocalDeleteGuild(uint32 guild_id) {
	map<uint32, GuildInfo *>::iterator res;
	res = m_guilds.find(guild_id);
	if(res == m_guilds.end())
		return(false);	//invalid guild
	m_guilds.erase(res);
	return(true);
}

void BaseGuildManager::ClearGuilds() {
	map<uint32, GuildInfo *>::iterator cur, end;
	cur = m_guilds.begin();
	end = m_guilds.end();
	for(; cur != end; cur++) {
		delete cur->second;
	}
	m_guilds.clear();
}

BaseGuildManager::RankInfo::RankInfo()
{
	permissions.Bits = 0;
	/*
	uint8 r;
	for(r = 0; r < _MaxGuildAction; r++)
		permissions[r] = false;
	*/
}

BaseGuildManager::GuildInfo::GuildInfo() {
	leader_char_id = 0;
	minstatus = 0;
}

uint32 BaseGuildManager::DoesAccountContainAGuildLeader(uint32 AccountID)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	
	if (!m_db->RunQuery(query,
			    MakeAnyLenString(&query,
			    		     "select guild_id from guild_members where char_id in (select id from character_ where account_id = %i) and rank = 2",
				     	     AccountID), errbuf, &result))
	{
		_log(GUILDS__ERROR, "Error executing query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return 0;
	}
	safe_delete_array(query);
	
	uint32 Rows = mysql_num_rows(result);
	mysql_free_result(result);
	
	return Rows;
}

bool BaseGuildManager::SetGuildPermission(uint32 GuildID, uint32 Rank, uint32 Permission, uint32 Allowed)
{
	if((Rank < 1) || (Rank > 8))
		return false;

	if((Permission < 1) || (Permission > 30))
		return false;

	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
	{
		return(false);
	}

	if(!Allowed)	
		res->second->ranks[Rank].permissions.Bits &= ~(1 << (Permission - 1));
	else
		res->second->ranks[Rank].permissions.Bits |= (1 << (Permission - 1));

	return true;
}

bool BaseGuildManager::SetGuildRankName(uint32 GuildID, uint32 Rank, const char* RankName)
{
	if((Rank < 1) || (Rank > 8))
		return false;

	for(uint32 i = 0; i < strlen(RankName); ++i)
	{
		if(!(isalpha(RankName[i]) || isspace(RankName[i])))
			return false;
	}

	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
	{
		return(false);
	}
	
	res->second->ranks[Rank].name = RankName;

	return true;
}

uint64 BaseGuildManager::GetRankPermissionBits(uint32 GuildID, uint32 Rank)
{
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
	{
		return 0;
	}
	
	return res->second->ranks[Rank].permissions.Bits;

}

bool BaseGuildManager::SetRankPermissionBits(uint32 GuildID, uint32 Rank, uint64 Permissions)
{
	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
	{
		return false;
	}
	
	res->second->ranks[Rank].permissions.Bits = Permissions;

	return true;

}

bool BaseGuildManager::DBSaveGuildRankPermissions(uint32 GuildID, uint32 Rank)
{
	if(m_db == NULL)
		return(false);
	
	if((Rank < 1) || (Rank > 8))
		return false;

	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
		return(false);
	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, "UPDATE guild_ranks SET permissions = %llu WHERE guild_id=%u AND rank=%u", 
		res->second->ranks[Rank].permissions.Bits, GuildID, Rank), errbuf))
	{
		_log(GUILDS__ERROR, "Error updating rank permissions, %s : %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	return(true);
}

bool BaseGuildManager::DBSaveGuildRankName(uint32 GuildID, uint32 Rank)
{
	if(m_db == NULL)
		return(false);
	
	if((Rank < 1) || (Rank > 8))
		return false;

	map<uint32, GuildInfo *>::const_iterator res;
	res = m_guilds.find(GuildID);
	if(res == m_guilds.end())
		return(false);
	GuildInfo *info = res->second;
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	
	if (!m_db->RunQuery(query, MakeAnyLenString(&query, "UPDATE guild_ranks SET title = '%s' WHERE guild_id=%u AND rank=%u", 
		res->second->ranks[Rank].name.c_str(), GuildID, Rank), errbuf))
	{
		_log(GUILDS__ERROR, "Error updating rank name, %s : %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	return(true);
}

