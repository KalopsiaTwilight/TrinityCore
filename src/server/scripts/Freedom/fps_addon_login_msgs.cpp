#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Player.h"
#include "AreaTrigger.h"
#include "Config.h"
#include "Log.h"
#include "GameTime.h"
#include <ChatPackets.h>

using namespace Trinity::ChatCommands;

class F_PS_Addon_Login_Msgs : public PlayerScript
{
public:
    F_PS_Addon_Login_Msgs() : PlayerScript("F_PS_Addon_Login_Msgs") { }

    void OnLogin(Player* player, bool /*firstLogin*/) {
        AddonLoginMessageContainer loginMessages = sFreedomMgr->GetAddonLoginMessageContainer();
        auto it = loginMessages.begin();
        while (it != loginMessages.end()) {
            AddonLoginMessageData msg = (*it);

            WorldPackets::Chat::Chat chat;
            chat.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, player, player, msg.message, 0, "", LOCALE_enUS, msg.prefix);

            chat.SenderGUID = msg.senderGuid;
            chat.SenderAccountGUID = msg.senderAccountGuid;
            chat.SenderGuildGUID = msg.senderGuildGuid;
            player->GetSession()->SendPacket(chat.Write());

            it++;
        }
    }
};

void AddSC_F_addon_msgs()
{
    new F_PS_Addon_Login_Msgs();
}
