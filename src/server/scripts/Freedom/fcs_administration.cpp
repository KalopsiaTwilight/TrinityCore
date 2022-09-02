#include "Chat.h"
#include "ChatCommand.h"
#include "ScriptMgr.h"
#include "RBAC.h"
#include "FreedomMgr.h"
#include "ObjectMgr.h"

using namespace Trinity::ChatCommands;

class administration_commandscript : public CommandScript
{
public:
    administration_commandscript() : CommandScript("administration_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable blacklistCommandTable =
        {
            { "item",      HandleBlacklistItemCommand,       rbac::RBAC_FPERM_ADMINISTRATION,  Console::No},
            { "gobject",   HandleBlacklistGameobjectCommand, rbac::RBAC_FPERM_ADMINISTRATION,  Console::No},
            { "creature",  HandleBlacklistCreatureCommand,  rbac::RBAC_FPERM_ADMINISTRATION,  Console::No},
        };
        static ChatCommandTable commandTable =
        {
            { "blacklist", blacklistCommandTable },
        };
        return commandTable;
    }
#pragma region BLACKLIST
    static bool HandleBlacklistItemCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_BLACKLIST_ITEM);
            return true;
        }

        char* id = handler->extractKeyFromLink((char*)args, "Hitem");
        char* flagStr = strtok(NULL, " ");

        uint32 itemId = atoul(id);
        uint32 flag = flagStr ? atoul(flagStr) : 1;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        if (!itemTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Item", itemId);
            return true;
        }

        if (flag)
        {
            sFreedomMgr->SetItemTemplateExtraHiddenFlag(itemId, true);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_ITEM, "added to the blacklist");
        }
        else
        {
            sFreedomMgr->SetItemTemplateExtraHiddenFlag(itemId, false);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_ITEM, "removed from the blacklist");
        }

        return true;
    }

    static bool HandleBlacklistGameobjectCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_BLACKLIST_GAMEOBJECT);
            return true;
        }

        char* id = handler->extractKeyFromLink((char*)args, "Hgameobject_entry");
        char* flagStr = strtok(NULL, " ");

        uint32 entryId = atoul(id);
        uint32 flag = flagStr ? atoul(flagStr) : 1;

        GameObjectTemplate const* objectTemplate = sObjectMgr->GetGameObjectTemplate(entryId);
        if (!objectTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Gameobject", entryId);
            return true;
        }

        if (flag)
        {
            sFreedomMgr->SetGameobjectTemplateExtraDisabledFlag(entryId, true);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_GAMEOBJECT, "added to the blacklist");
        }
        else
        {
            sFreedomMgr->SetGameobjectTemplateExtraDisabledFlag(entryId, false);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_GAMEOBJECT, "removed from the blacklist");
        }

        return true;
    }

    static bool HandleBlacklistCreatureCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_BLACKLIST_CREATURE);
            return true;
        }

        char* id = handler->extractKeyFromLink((char*)args, "Hgameobject_entry");
        char* flagStr = strtok(NULL, " ");

        uint32 entryId = atoul(id);
        uint32 flag = flagStr ? atoul(flagStr) : 1;

        CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(entryId);
        if (!creatureTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Creature", entryId);
            return true;
        }

        if (flag)
        {
            sFreedomMgr->SetCreatureTemplateExtraDisabledFlag(entryId, true);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_CREATURE, "added to the blacklist");
        }
        else
        {
            sFreedomMgr->SetCreatureTemplateExtraDisabledFlag(entryId, false);
            handler->PSendSysMessage(FREEDOM_CMDI_BLACKLIST_CREATURE, "removed from the blacklist");
        }

        return true;
    }
#pragma endregion
};
void AddSC_administration_commandscript()
{
    new administration_commandscript();
}
