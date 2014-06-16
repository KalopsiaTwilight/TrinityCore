/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

/* ScriptData
Name: lfrp_commandscript
%Complete: 100
Comment: All lfrp related commands
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

class lfrp_commandscript : public CommandScript
{
public:
	lfrp_commandscript() : CommandScript("lfrp_commandscript") { }

    ChatCommand* GetCommands() const override
    {
        static ChatCommand lfrpCommandTable[] =
        {
            { "list",   rbac::RBAC_PERM_COMMAND_LFRP,       false, &HandleLfrpListCommand,  "", NULL },
            { "clear",  rbac::RBAC_PERM_COMMAND_LFRP,       false, &HandleLfrpClearCommand, "", NULL },
            { "",       rbac::RBAC_PERM_COMMAND_LFRP,       false, &HandleLfrpCommand,      "", NULL },
            { NULL,     0,                                  false, NULL,                    "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "lfrp", rbac::RBAC_PERM_COMMAND_LFRP, false, NULL, "", lfrpCommandTable },
            { NULL, 0,                    false, NULL, "", NULL }
        };
        return commandTable;
    }

	static bool HandleLfrpCommand(ChatHandler* handler, const char* args)
	{
		if (!*args)
		{
			QueryResult result = CharacterDatabase.PQuery("SELECT content, timestamp FROM characters_lfrp WHERE guid='%u'", handler->GetSession()->GetPlayer()->GetGUIDLow());
			if (result)
			{
				Field* fields = result->Fetch();

				std::string content = fields[0].GetString();
				std::string timestamp = fields[1].GetString();

				handler->PSendSysMessage(LANG_LFRP_CURRENTENTRY, content.c_str(), timestamp.c_str());
			}
			else
				handler->PSendSysMessage(LANG_LFRP_NOCURRENTENTRY);

			return true;
		}
		/*
		char* tailStr = *args != '"' ? strtok(NULL, "") : (char*)args;
        if (!tailStr)
            return false;

        char* guildStr = handler->extractQuotedArg(tailStr);
        if (!guildStr)
            return false;

		char* content = (char*)args;
		*/
		if (*args != '\0')
        {
            char const* content = strtok(NULL, "\r");
			std::string contentStr = content;

			CharacterDatabase.PExecute("DELETE FROM characters_lfrp WHERE guid='%u'", handler->GetSession()->GetPlayer()->GetGUIDLow());
			CharacterDatabase.PExecute("INSERT INTO characters_lfrp (guid,content) VALUES ('%u','%s')", handler->GetSession()->GetPlayer()->GetGUIDLow(), contentStr);
			handler->PSendSysMessage(LANG_LFRP_NEWENTRY, contentStr.c_str());
			return true;
        }
	}

	static bool HandleLfrpClearCommand(ChatHandler* handler, const char* args)
	{
		if (!*args)
			{
				CharacterDatabase.PExecute("DELETE FROM characters_lfrp WHERE guid='%u'", handler->GetSession()->GetPlayer()->GetGUIDLow());
				handler->PSendSysMessage(LANG_LFRP_CLEAR);

				return true;
			}
		else
		{
			if (handler->GetSession()->HasPermission(rbac::RBAC_PERM_COMMAND_LFRP_CLEAR_OTHER))
			{
				std::string character = strtok((char*)args, " ");
				uint64 characterGuid;

				characterGuid = sObjectMgr->GetPlayerGUIDByName(character);

				if (!characterGuid)
				{
					handler->PSendSysMessage(LANG_CHAR_ACCOUNT_CHAR_NOT_FOUND, character.c_str());
					handler->SetSentErrorMessage(true);
					return false;
				}

				CharacterDatabase.PExecute("DELETE FROM characters_lfrp WHERE guid='%u'", characterGuid);
				handler->PSendSysMessage(LANG_LFRP_CLEAR_OTHER, character.c_str());

				return true;
			}
			else
			{
				CharacterDatabase.PExecute("DELETE FROM characters_lfrp WHERE guid='%u'", handler->GetSession()->GetPlayer()->GetGUIDLow());
				handler->PSendSysMessage(LANG_LFRP_CLEAR);
			}
			return true;
		}
    }

	static bool HandleLfrpListCommand(ChatHandler* handler, const char* args)
	{
		if (!*args)
		{
			return true;
		}
		else
		{
			return true;
		}
	}

};

void AddSC_lfrp_commandscript()
{
    new lfrp_commandscript();
}
