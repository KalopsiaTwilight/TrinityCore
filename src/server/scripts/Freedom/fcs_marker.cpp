#include "Chat.h"
#include "ChatCommand.h"
#include "ScriptMgr.h"
#include "RBAC.h"
#include "FreedomMgr.h"
#include "ObjectMgr.h"

using namespace Trinity::ChatCommands;

class F_marker_commandscript : public CommandScript
{
public:
    F_marker_commandscript() : CommandScript("F_marker_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable gotoMarkerCommandTable =
        {
            { "formation", HandleFormationGoToMarkerCommand, rbac::RBAC_FPERM_COMMAND_GOTOMARKER_FORMATION,      Console::No},
            { "",          HandleGoToMarkerCommand,          rbac::RBAC_FPERM_COMMAND_GOTOMARKER,                Console::No},
        };

        static ChatCommandTable turntoMarkerCommandTable =
        {
            { "formation", HandleFormationTurnToMarkerCommand, rbac::RBAC_FPERM_COMMAND_TURNTOMARKER_FORMATION,  Console::No},
            { "",          HandleTurnToMarkerCommand,          rbac::RBAC_FPERM_COMMAND_TURNTOMARKER,            Console::No},
        };

        static ChatCommandTable walktoMarkerCommandTable =
        {
            { "formation", HandleFormationWalkToMarkerCommand,  rbac::RBAC_FPERM_COMMAND_WALKTOMARKER_FORMATION,  Console::No},
            { "",          HandleWalkToMarkerCommand,          rbac::RBAC_FPERM_COMMAND_WALKTOMARKER,             Console::No},
        };

        static ChatCommandTable commandTable =
        {
            { "gotomarker",   gotoMarkerCommandTable   },
            { "turntomarker", turntoMarkerCommandTable },
            { "walktomarker", walktoMarkerCommandTable },

        };
        return commandTable;
    }

    static bool HandleGoToMarkerCommand(ChatHandler* handler, Optional<uint64> targetSpawnId)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 spawnId = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (targetSpawnId.has_value()) {
            spawnId = targetSpawnId.value();
        }

        target = sFreedomMgr->GetAnyCreature(spawnId);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        target->GetMotionMaster()->MovePoint(0, location->GetPosition());

        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_MOVE,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);
        return true;
    }

    static bool HandleFormationGoToMarkerCommand(ChatHandler* handler, std::string const& formationKey)
    {
        if (!sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is no formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = sFreedomMgr->GetAnyCreature(sFreedomMgr->GetFormationLeaderGuid(formationKey));

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        target->GetMotionMaster()->MovePoint(0, location->GetPosition());

        uint64 spawnId = target->GetSpawnId();
        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_MOVE,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);
        return true;
    }

    static bool HandleTurnToMarkerCommand(ChatHandler* handler, Optional<uint64> targetSpawnId)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 spawnId = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (targetSpawnId.has_value()) {
            spawnId = targetSpawnId.value();
        }

        target = sFreedomMgr->GetAnyCreature(spawnId);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        float angle = target->GetAbsoluteAngle(location);
        target->GetMotionMaster()->MovePoint(0, target->GetPosition(), false, angle);

        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_TURNED,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);

        return true;
    }

    static bool HandleFormationTurnToMarkerCommand(ChatHandler* handler, std::string const& formationKey)
    {
        if (!sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is no formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = sFreedomMgr->GetAnyCreature(sFreedomMgr->GetFormationLeaderGuid(formationKey));

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        float angle = target->GetAbsoluteAngle(location);
        target->GetMotionMaster()->MovePoint(0, target->GetPosition(), false, angle);

        uint64 spawnId = target->GetSpawnId();
        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_TURNED,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);

        return true;
    }

    static bool HandleWalkToMarkerCommand(ChatHandler* handler, Optional<uint64> targetSpawnId)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 spawnId = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (targetSpawnId.has_value()) {
            spawnId = targetSpawnId.value();
        }

        target = sFreedomMgr->GetAnyCreature(spawnId);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        target->GetMotionMaster()->MovePoint(0, location->GetPositionX(), location->GetPositionY(), location->GetPositionZ(), target->GetSpeed(MOVE_WALK));

        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_MOVE,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);
        return true;
    }

    static bool HandleFormationWalkToMarkerCommand(ChatHandler* handler, std::string const& formationKey)
    {
        if (!sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is no formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = sFreedomMgr->GetAnyCreature(sFreedomMgr->GetFormationLeaderGuid(formationKey));

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        target->GetMotionMaster()->MovePoint(0, location->GetPositionX(), location->GetPositionY(), location->GetPositionZ(), target->GetSpeed(MOVE_WALK));

        uint64 spawnId = target->GetSpawnId();
        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_MOVE,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);
        return true;
    }
};

class F_marker_spellscript_loader : public SpellScriptLoader
{
public:
    F_marker_spellscript_loader() : SpellScriptLoader("F_marker_spellscript_loader") { }

    class F_marker_spellscript : public SpellScript
    {
        PrepareSpellScript(F_marker_spellscript);

        void HandleAfterCast()
        {
            if (!GetCaster())
                return;
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;
            const WorldLocation* summonPos = GetExplTargetDest();
            if (!summonPos)
                return;
            sFreedomMgr->StoreMarkerLocationForPlayer(player, summonPos);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(F_marker_spellscript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new F_marker_spellscript();
    }
};

void AddSC_F_marker_commandscript()
{
    new F_marker_commandscript();
    new F_marker_spellscript_loader();
}
