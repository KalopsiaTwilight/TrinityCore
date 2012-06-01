/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "World.h"
#include "Player.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "AccountMgr.h"
#include "SystemConfig.h"
#include "revision.h"
#include "Util.h"

bool ChatHandler::HandleHelpCommand(const char* args)
{
    char* cmd = strtok((char*)args, " ");
    if (!cmd)
    {
        ShowHelpForCommand(getCommandTable(), "help");
        ShowHelpForCommand(getCommandTable(), "");
    }
    else
    {
        if (!ShowHelpForCommand(getCommandTable(), cmd))
            SendSysMessage(LANG_NO_HELP_CMD);
    }

    return true;
}

bool ChatHandler::HandleCommandsCommand(const char* /*args*/)
{
    ShowHelpForCommand(getCommandTable(), "");
    return true;
}

bool ChatHandler::HandleStartCommand(const char* /*args*/)
{
    Player* player = m_session->GetPlayer();

    if (player->isInFlight())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    if (player->isInCombat())
    {
        SendSysMessage(LANG_YOU_IN_COMBAT);
        SetSentErrorMessage(true);
        return false;
    }

    if (player->isDead() || player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
    {
        // if player is dead and stuck, send ghost to graveyard
        player->RepopAtGraveyard();
        return true;
    }

    // cast spell Stuck
    player->CastSpell(player, 7355, false);
    return true;
}

bool ChatHandler::HandleServerInfoCommand(const char* /*args*/)
{
    uint32 playersNum = sWorld->GetPlayerCount();
    uint32 maxPlayersNum = sWorld->GetMaxPlayerCount();
    uint32 activeClientsNum = sWorld->GetActiveSessionCount();
    uint32 queuedClientsNum = sWorld->GetQueuedSessionCount();
    uint32 maxActiveClientsNum = sWorld->GetMaxActiveSessionCount();
    uint32 maxQueuedClientsNum = sWorld->GetMaxQueuedSessionCount();
    std::string uptime = secsToTimeString(sWorld->GetUptime());
    uint32 updateTime = sWorld->GetUpdateTime();

    SendSysMessage(_FULLVERSION);
    PSendSysMessage(LANG_CONNECTED_PLAYERS, playersNum, maxPlayersNum);
    PSendSysMessage(LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
    PSendSysMessage(LANG_UPTIME, uptime.c_str());
    PSendSysMessage(LANG_UPDATE_DIFF, updateTime);
    //! Can't use sWorld->ShutdownMsg here in case of console command
    if (sWorld->IsShuttingDown())
        PSendSysMessage(LANG_SHUTDOWN_TIMELEFT, secsToTimeString(sWorld->GetShutDownTimeLeft()).c_str());

    return true;
}

bool ChatHandler::HandleDismountCommand(const char* /*args*/)
{
    Player* player = m_session->GetPlayer();

    //If player is not mounted, so go out :)
    if (!player->IsMounted())
    {
        SendSysMessage(LANG_CHAR_NON_MOUNTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (player->isInFlight())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    player->Dismount();
    player->RemoveAurasByType(SPELL_AURA_MOUNTED);
    return true;
}

bool ChatHandler::HandleSaveCommand(const char* /*args*/)
{
    Player* player = m_session->GetPlayer();

    // save GM account without delay and output message
    if (!AccountMgr::IsPlayerAccount(m_session->GetSecurity()))
    {
        if (Player* target = getSelectedPlayer())
            target->SaveToDB();
        else
            player->SaveToDB();
        SendSysMessage(LANG_PLAYER_SAVED);
        return true;
    }

    // save if the player has last been saved over 20 seconds ago
    uint32 save_interval = sWorld->getIntConfig(CONFIG_INTERVAL_SAVE);
    if (save_interval == 0 || (save_interval > 20 * IN_MILLISECONDS && player->GetSaveTimer() <= save_interval - 20 * IN_MILLISECONDS))
        player->SaveToDB();

    return true;
}

/// Display the 'Message of the day' for the realm
bool ChatHandler::HandleServerMotdCommand(const char* /*args*/)
{
    PSendSysMessage(LANG_MOTD_CURRENT, sWorld->GetMotd());
    return true;
}

bool ChatHandler::HandleAddRPItemCommand(const char *args)
{
    if (!*args)
        return false;

    uint32 itemId = 0;

    if (args[0] == '[')                                        // [name] manual form
    {
        char* citemName = strtok((char*)args, "]");

        if (citemName && citemName[0])
        {
            std::string itemName = citemName+1;
            WorldDatabase.EscapeString(itemName);
            QueryResult result = WorldDatabase.PQuery("SELECT entry FROM item_template WHERE entry>200000 AND name = '%s'", itemName.c_str());
            if (!result)
            {
                PSendSysMessage(LANG_COMMAND_COULDNOTFIND, citemName+1);
                SetSentErrorMessage(true);
                return false;
            }
            itemId = result->Fetch()->GetUInt16();
        }
        else
            return false;
    }
    else                                                    // item_id or [name] Shift-click form |color|Hitem:item_id:0:0:0|h[name]|h|r
    {
        char* cId = extractKeyFromLink((char*)args,"Hitem");
        if (!cId)
            return false;
        itemId = atol(cId);
    }

    if (itemId <200000)
    {
        PSendSysMessage(LANG_COMMAND_RPITEMTOOLOW, itemId);
        SetSentErrorMessage(true);
        return false;
    }

    char* ccount = strtok(NULL, " ");

    int32 count = 1;

    if (ccount)
        count = strtol(ccount, NULL, 10);

    if (count == 0)
        count = 1;

    Player* pl = m_session->GetPlayer();
    //Player* plTarget = getSelectedPlayer();
    //if (!plTarget)
    //    plTarget = pl;

    //sLog.outDetail(GetTrinityString(LANG_ADDRPITEM), itemId, count);

    ItemTemplate const *pProto = sObjectMgr->GetItemTemplate(itemId);
    if (!pProto)
    {
        PSendSysMessage(LANG_COMMAND_RPITEMIDINVALID, itemId);
        SetSentErrorMessage(true);
        return false;
    }

    //Subtract
    if (count < 0)
    {
        pl->DestroyItemCount(itemId, -count, true, false);
        PSendSysMessage(LANG_REMOVERPITEM, itemId, -count, GetNameLink(pl).c_str());
        return true;
    }

    //Adding items
    uint32 noSpaceForCount = 0;

    // check space and find places
    ItemPosCountVec dest;
    uint8 msg = pl->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
    if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
        count -= noSpaceForCount;

    if (count == 0 || dest.empty())                         // can't add any
    {
        PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);
        SetSentErrorMessage(true);
        return false;
    }

    Item* item = pl->StoreNewItem(dest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));

    if (count > 0 && item)
    {
        pl->SendNewItem(item,count,false,true);
    }

    if (noSpaceForCount > 0)
        PSendSysMessage(LANG_ITEM_CANNOT_CREATE, itemId, noSpaceForCount);

    return true;
}

bool ChatHandler::HandleLookupRPItemCommand(const char *args)
{
    if (!*args)
        return false;

    std::string namepart = args;
    std::wstring wnamepart;

    // converting string that we try to find to lower case
    if (!Utf8toWStr(namepart,wnamepart))
        return false;

    wstrToLower(wnamepart);

    bool found = false;
	uint32 count = 0;
    uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

    // Search in `item_template`
    ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
    for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
    {
        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            uint8 uloc_idx = uint8(loc_idx);
            if (ItemLocale const *il = sObjectMgr->GetItemLocale(itr->second.ItemId))
            {
                if (il->Name.size() > uloc_idx && !il->Name[uloc_idx].empty())
                {
                    std::string name = il->Name[uloc_idx];

                    if (Utf8FitTo(name, wnamepart))
                    {
                        if (maxResults && count++ == maxResults)
                        {
                            PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                            return true;
                        }

                        if (m_session)
                            PSendSysMessage(LANG_ITEM_LIST_CHAT, itr->second.ItemId, itr->second.ItemId, name.c_str());
                        else
                            PSendSysMessage(LANG_ITEM_LIST_CONSOLE, itr->second.ItemId, name.c_str());

                        if (!found)
                            found = true;

                        continue;
                    }
                }
            }
        }

		if (itr->second.ItemId < 200000)
			continue;

        std::string name = itr->second.Name1;
        if (name.empty())
            continue;

        if (Utf8FitTo(name, wnamepart))
        {
            if (maxResults && count++ == maxResults)
            {
                PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                return true;
            }

            if (m_session)
                PSendSysMessage(LANG_ITEM_LIST_CHAT, itr->second.ItemId, itr->second.ItemId, name.c_str());
            else
                PSendSysMessage(LANG_ITEM_LIST_CONSOLE, itr->second.ItemId, name.c_str());

            if (!found)
                found = true;
        }
    }

    if (!found)
        SendSysMessage(LANG_COMMAND_NOITEMFOUND);

    return true;
}

bool ChatHandler::HandleSelfScaleCommand(const char* args)
{
    if (!*args)
        return false;

    float Scale = (float)atof((char*)args);
    if (Scale > 1.1f || Scale < 0.9f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    QueryResult result = CharacterDatabase.PQuery("SELECT scale, scale_times_changed FROM characters_addon WHERE guid='%u'", m_session->GetPlayer()->GetGUIDLow());
    if(result)
    {
        Field* fields = result->Fetch();

        float customScale = fields[0].GetFloat();
        uint8 scaleTimesChanged = fields[1].GetUInt8();

        if (scaleTimesChanged < 10)
        {
            Player *chr = m_session->GetPlayer();
		    uint8 scaleChangesRemaining = (10 - (scaleTimesChanged + 1));
            PSendSysMessage(LANG_CUSTOM_SCALE_CHANGE, Scale, scaleChangesRemaining);
            chr->SetFloatValue(OBJECT_FIELD_SCALE_X, Scale);

            QueryResult result = CharacterDatabase.PQuery("UPDATE characters_addon SET scale='%f', scale_times_changed=(`scale_times_changed`+1) WHERE guid='%u'", Scale, m_session->GetPlayer()->GetGUIDLow());

            return true;
	    }

        else
        {
            SendSysMessage(LANG_SCALE_NO_MORE_CHANGES);
            SetSentErrorMessage(true);
            return false;
        }
	}

    else
	{
        Player *chr = m_session->GetPlayer();

        uint8 scaleChangesRemaining = 9;
        PSendSysMessage(LANG_CUSTOM_SCALE_CHANGE, Scale, scaleChangesRemaining);
        chr->SetFloatValue(OBJECT_FIELD_SCALE_X, Scale);
        CharacterDatabase.PExecute("INSERT INTO characters_addon(guid,scale,scale_times_changed) VALUES ('%u','%f','1')", m_session->GetPlayer()->GetGUIDLow(), Scale);

        return true;
	}
}

bool ChatHandler::HandleSelfTaxiCheatCommand(const char* args)
{
    Player* chr = m_session->GetPlayer();

    if (!m_session->GetPlayer()->isTaxiCheater())
    {
        chr->SetTaxiCheater(true);
        PSendSysMessage(LANG_SELFTAXIS_UNL);

        return true;
    }
/*
    if (!m_session->GetPlayer()->isTaxiCheater())
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT taxi_times_enabled FROM characters_addon WHERE guid='%u'", m_session->GetPlayer()->GetGUIDLow());
        if(result)
        {
            Field* fields = result->Fetch();
            uint8 taxiTimesEnabled = fields[0].GetUInt8();

            if (taxiTimesEnabled < 3)
			{
                uint8 taxiTimesRemaining = (3 - (taxiTimesEnabled + 1));
                chr->SetTaxiCheater(true);
                PSendSysMessage(LANG_SELFTAXIS, taxiTimesRemaining);

                QueryResult result = CharacterDatabase.PQuery("UPDATE characters_addon SET taxi_times_enabled=(`taxi_times_enabled`+1) WHERE guid='%u'", m_session->GetPlayer()->GetGUIDLow());

                return true;
            }
			else
            {
                SendSysMessage(LANG_SELFTAXIS_NO_MORE_CHANGES);
                SetSentErrorMessage(true);
                return false;
            }
	    }

        else
	    {
            uint8 taxiTimesRemaining = 2;
            PSendSysMessage(LANG_SELFTAXIS, taxiTimesRemaining);
            chr->SetTaxiCheater(true);
            CharacterDatabase.PExecute("INSERT INTO characters_addon(guid,taxi_times_enabled) VALUES ('%u','1')", m_session->GetPlayer()->GetGUIDLow());

            return true;
	    }
    }
*/
    else
    {
        SendSysMessage(LANG_SELFTAXIS_ALREADYON);
        SetSentErrorMessage(true);
        return false;
    }
}
