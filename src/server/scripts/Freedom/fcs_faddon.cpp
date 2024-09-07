#include "Chat.h"
#include "ChatCommand.h"
#include "ScriptMgr.h"
#include "RBAC.h"
#include "FreedomMgr.h"
#include "ObjectMgr.h"

using namespace Trinity::ChatCommands;

class F_CS_FAddon : public CommandScript
{
public:
    F_CS_FAddon() : CommandScript("F_CS_FAddon") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable loginMsgTable =
        {
            { "add", HandleAddAddonLoginMsgCommand,     rbac::RBAC_FPERM_COMMAND_FADDON_LOGINMSG_ADD, Console::No},
            { "del", HandleDeleteAddonLoginMsgCommand,  rbac::RBAC_FPERM_COMMAND_FADDON_LOGINMSG_DEL, Console::No},
        };

        static ChatCommandTable getTable =
        {
            { "permlvl", HandleGetPermissionLevelCommand, rbac::RBAC_FPERM_COMMAND_FADDON_GET_PERMLVL, Console::No},
        };

        static ChatCommandTable fAddonTable = 
        {
            { "loginmsg",   loginMsgTable   },
            { "get",        getTable        }
        };

        static ChatCommandTable commandTable =
        {
            { "faddon",   fAddonTable   },
        };
        return commandTable;
    }

    static bool HandleAddAddonLoginMsgCommand(ChatHandler* handler, std::string const& prefix, Tail msg)
    {
        Player* source = handler->GetSession()->GetPlayer();

        if (prefix.size() > 25) {
            handler->PSendSysMessage("f1");
            return false;
        }

        if (msg.size() > 250) {
            handler->PSendSysMessage("f2");
            return false;
        }

        sFreedomMgr->AddAddonLoginMessage(prefix, msg.data(), source);
        handler->PSendSysMessage("%u", sFreedomMgr->GetAddonLoginMessageContainer().back().id);
        return true;
    }

    static bool HandleDeleteAddonLoginMsgCommand(ChatHandler* /*handler*/, uint32 msgId)
    {
        bool deleteSuccess = sFreedomMgr->DeleteAddonLoginMessage(msgId);
        return deleteSuccess;
    }

    static bool HandleGetPermissionLevelCommand(ChatHandler* handler)
    {
        handler->PSendSysMessage("%u", handler->GetSession()->GetSecurity());
        return true;
    }
};

void AddSC_F_CS_faddon()
{
    new F_CS_FAddon();
}
