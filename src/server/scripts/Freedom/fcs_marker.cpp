#include "Chat.h"
#include "ChatCommand.h"
#include "ScriptMgr.h"
#include "RBAC.h"
#include "FreedomMgr.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"

using namespace Trinity::ChatCommands;
using CreatureEntry = Variant<Hyperlink<creature_entry>, uint32>;
using GobEntry = Variant<Hyperlink<gameobject_entry>, uint32>;

class F_marker_commandscript : public CommandScript
{
public:
    F_marker_commandscript() : CommandScript("F_marker_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable gotoMarkerCommandTable =
        {
            { "formation", HandleFormationGoToMarkerCommand,    rbac::RBAC_FPERM_COMMAND_GOTOMARKER_FORMATION,      Console::No},
            { "",          HandleGoToMarkerCommand,             rbac::RBAC_FPERM_COMMAND_GOTOMARKER,                Console::No},
        };

        static ChatCommandTable turntoMarkerCommandTable =
        {
            { "formation", HandleFormationTurnToMarkerCommand,  rbac::RBAC_FPERM_COMMAND_TURNTOMARKER_FORMATION,  Console::No},
            { "",          HandleTurnToMarkerCommand,           rbac::RBAC_FPERM_COMMAND_TURNTOMARKER,            Console::No},
        };

        static ChatCommandTable walktoMarkerCommandTable =
        {
            { "formation", HandleFormationWalkToMarkerCommand,  rbac::RBAC_FPERM_COMMAND_WALKTOMARKER_FORMATION,  Console::No},
            { "",          HandleWalkToMarkerCommand,           rbac::RBAC_FPERM_COMMAND_WALKTOMARKER,            Console::No},
        };

        static ChatCommandTable addToMarkerCommandTable =
        {
            { "gob", HandleMarkerAddGobCommand,                 rbac::RBAC_FPERM_COMMAND_MARKER_ADD_GOB,           Console::No},
            { "npc", HandleMarkerAddNpcCommand,                 rbac::RBAC_FPERM_COMMAND_MARKER_ADD_NPC,           Console::No},
        };

        static ChatCommandTable markerTable =
        {
            { "goto",   gotoMarkerCommandTable   },
            { "turnto", turntoMarkerCommandTable },
            { "walkto", walktoMarkerCommandTable },
            { "add", addToMarkerCommandTable }
        };

        static ChatCommandTable commandTable =
        {
            { "gotomarker",   gotoMarkerCommandTable   },
            { "turntomarker", turntoMarkerCommandTable },
            { "walktomarker", walktoMarkerCommandTable },
            { "marker", markerTable }
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

    static bool HandleMarkerAddGobCommand(ChatHandler* handler, GobEntry objectId)
    {
        const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);

        if (!objectInfo)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_ENTRY_NOT_EXISTS, objectId);
            return true;
        }

        if (auto extraData = sFreedomMgr->GetGameObjectTemplateExtraData(objectId))
        {
            if (extraData->disabled)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_IS_BLACKLISTED);
                return true;
            }
        }

        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SPAWN_INVALID_DISPLAY_C, objectId, objectInfo->displayId);
        }

        Player* source = handler->GetSession()->GetPlayer();
        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(source);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }


        GameObject* object = sFreedomMgr->GameObjectCreate(source, objectInfo, 0, -1, location);
        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SPAWN_FAIL, objectId);
            return true;
        }

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_SPAWN,
            sFreedomMgr->ToChatLink("Hgameobject", object->GetSpawnId(), objectInfo->name),
            object->GetSpawnId(),
            objectInfo->entry, location->GetPositionX(), location->GetPositionY(), location->GetPositionZ());
        sFreedomMgr->SetGameobjectSelectionForPlayer(source->GetGUID().GetCounter(), object->GetSpawnId());
        return true;
    }

    static bool HandleMarkerAddNpcCommand(ChatHandler* handler, CreatureEntry id)
    {
        if (!sObjectMgr->GetCreatureTemplate(id))
        {
            handler->PSendSysMessage("Could not find a creature template with id: %u. Perhaps you entered the wrong id or there is something wrong with the template?", id);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* chr = handler->GetSession()->GetPlayer();
        WorldLocation* location = sFreedomMgr->GetMarketLocationForPlayer(chr);
        if (!location)
        {
            handler->PSendSysMessage("You don't have a marker location.");
            return true;
        }

        Map* map = chr->GetMap();

        Creature* creature = Creature::CreateCreature(id, map, location->GetPosition());
        if (!creature)
        {
            handler->PSendSysMessage("Could not create creature with id: %u. This indicates there is something wrong with the creature template.", id);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PhasingHandler::InheritPhaseShift(creature, chr);
        creature->SetDBPhase(sFreedomMgr->GetPlayerPhase(chr));
        creature->SaveToDB(map->GetId(), { map->GetDifficultyID() });

        ObjectGuid::LowType db_guid = creature->GetSpawnId();

        // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells()
        // current "creature" variable is deleted and created fresh new, otherwise old values might trigger asserts or cause undefined behavior
        creature->CleanupsBeforeDelete();
        delete creature;

        creature = Creature::CreateCreatureFromDB(db_guid, map, true, true);
        if (!creature)
        {
            handler->PSendSysMessage("Could not create creature with id: %u. This indicates there is something wrong with the creature template.", id);
            handler->SetSentErrorMessage(true);
            return false;
        }

        sObjectMgr->AddCreatureToGrid(sObjectMgr->GetCreatureData(db_guid));
        sFreedomMgr->CreatureSetModifyHistory(creature, chr, true);
        sFreedomMgr->SaveCreature(creature);
        sFreedomMgr->SetCreatureSelectionForPlayer(chr->GetGUID().GetCounter(), creature->GetSpawnId());
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
