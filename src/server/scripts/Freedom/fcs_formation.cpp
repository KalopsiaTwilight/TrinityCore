#include "Chat.h"
#include "ChatCommand.h"
#include "ScriptMgr.h"
#include "RBAC.h"
#include "FreedomMgr.h"
#include "ObjectMgr.h"
#include <CreatureGroups.h>
#include <boost/algorithm/string/predicate.hpp>

using namespace Trinity::ChatCommands;

class F_formation_commandscript : public CommandScript
{
public:
    F_formation_commandscript() : CommandScript("F_formation_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable formationCommandTable =
        {
            { "new",     HandleNewFormationCommand,          rbac::RBAC_FPERM_COMMAND_FORMATION_NEW,          Console::No},
            { "add",     HandleAddToFormationCommand,        rbac::RBAC_FPERM_COMMAND_FORMATION_ADD,          Console::No},
            { "remove",  HandleRemoveFromFormationCommand,   rbac::RBAC_FPERM_COMMAND_FORMATION_REMOVE,       Console::No},
            { "delete",  HandleDeleteFormationCommand,       rbac::RBAC_FPERM_COMMAND_FORMATION_DELETE,       Console::No},
            { "savepos", HandleFormationSavePositionCommand, rbac::RBAC_FPERM_COMMAND_FORMATION_SAVEPOSITION, Console::No},
            { "list",    HandleListFormationCommand,         rbac::RBAC_FPERM_COMMAND_FORMATION_LIST,         Console::No},
        };
        static ChatCommandTable commandTable =
        {
            { "formation", formationCommandTable },
        };
        return commandTable;
    }

    static bool HandleRemoveFromFormationCommand(ChatHandler* handler, Optional<uint64> targetSpawnId)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (targetSpawnId.has_value()) {
            guidLow = targetSpawnId.value();
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        if (!target->GetFormation())
        {
            handler->PSendSysMessage("Selected creature is not a member of a formation.");
            return true;
        }

        ObjectGuid::LowType lowguid = target->GetSpawnId();
        if (!lowguid)
            return true;

        sFormationMgr->RemoveCreatureFromGroup(target->GetMap()->CreatureGroupHolder.find(target->GetFormation()->GetLeaderSpawnId())->second, target);
        sFormationMgr->ClearCreatureFormation(target);
        target->GetMotionMaster()->MoveIdle();

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_FORMATION);
        stmt->setUInt64(0, lowguid);
        WorldDatabase.Execute(stmt);

        handler->PSendSysMessage("Creature removed from formation.");
        return true;
    }

    static bool HandleDeleteFormationCommand(ChatHandler* handler, std::string const& formationKey)
    {
        if (!sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is no formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        sFreedomMgr->DeleteFormation(formationKey);

        handler->PSendSysMessage("Formation %s was deleted.", formationKey.c_str());
        return true;
    }

    static bool HandleNewFormationCommand(ChatHandler* handler, std::string const& formationKey, Optional<uint64> targetSpawnId)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (targetSpawnId.has_value()) {
            guidLow = targetSpawnId.value();
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        if (sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is already a formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return true;
        }

        ObjectGuid::LowType lowguid = target->GetSpawnId();
        if (target->GetFormation())
        {
            handler->PSendSysMessage("Selected creature is already member of group lead by " UI64FMTD, target->GetFormation()->GetLeaderSpawnId());
            return true;
        }

        if (!lowguid)
            return true;

        sFreedomMgr->AddFormation(formationKey, lowguid);

        handler->PSendSysMessage("Formation %s added with leader " UI64FMTD, formationKey.c_str(), lowguid);
        return true;
    }

    static bool HandleAddToFormationCommand(ChatHandler* handler, std::string const& formationKey, Optional<uint64> targetSpawnId)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (targetSpawnId.has_value()) {
            guidLow = targetSpawnId.value();
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        if (!sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is no formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return true;
        }

        ObjectGuid::LowType lowguid = target->GetSpawnId();
        if (target->GetFormation())
        {
            handler->PSendSysMessage("Selected creature is already member of group lead by " UI64FMTD, target->GetFormation()->GetLeaderSpawnId());
            return true;
        }

        if (!lowguid)
            return true;

        ObjectGuid::LowType leaderGUID = sFreedomMgr->GetFormationLeaderGuid(formationKey);
        Creature* leader = sFreedomMgr->GetAnyCreature(leaderGUID);

        if (!leader || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage("Could not find the leader of formation %s on this map.", formationKey.c_str());
            return true;
        }

        float  followAngle = (target->GetAbsoluteAngle(leader) -leader->GetOrientation()) * 180.0f / float(M_PI);
        float  followDist = std::sqrt(std::pow(leader->GetPositionX() - target->GetPositionX(), 2.f) + std::pow(leader->GetPositionY() - target->GetPositionY(), 2.f));
        uint32 groupAI = FLAG_IDLE_IN_FORMATION;
        sFormationMgr->AddFormationMember(lowguid, followAngle * float(M_PI) / 180.0f, followDist, leaderGUID, groupAI);
        target->SearchFormation();

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_CREATURE_FORMATION);
        stmt->setUInt64(0, leaderGUID);
        stmt->setUInt64(1, lowguid);
        stmt->setFloat(2, followDist);
        stmt->setFloat(3, followAngle);
        stmt->setUInt32(4, groupAI);

        WorldDatabase.Execute(stmt);

        handler->PSendSysMessage("Creature " UI64FMTD " added to formation %s with leader " UI64FMTD, lowguid, formationKey.c_str(), leaderGUID);

        return true;
    }

    static bool HandleFormationSavePositionCommand(ChatHandler* handler, std::string const& formationKey)
    {
        if (!sFreedomMgr->FormationExists(formationKey)) {
            handler->PSendSysMessage("There is no formation with the name: %s", formationKey.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        ObjectGuid::LowType leaderGUID = sFreedomMgr->GetFormationLeaderGuid(formationKey);
        Creature* leader = sFreedomMgr->GetAnyCreature(leaderGUID);

        if (!leader || leader->GetMapId() != leader->GetMapId())
        {
            handler->PSendSysMessage("Could not find the leader of formation %s on this map.", formationKey.c_str());
            return true;
        }

        sFreedomMgr->SaveFormationPosition(formationKey, handler->GetPlayer());

        handler->PSendSysMessage("Formation %s positions saved.", formationKey.c_str());
        return true;
    }

    static bool HandleListFormationCommand(ChatHandler* handler, Optional<std::string> search)
    {
        const FormationDataContainer formationList = sFreedomMgr->GetFormationContainer();
        uint64 count = 0;

        if (!search)
        {
            for (auto formationData : formationList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_FORMATION_LIST_ITEM, formationData.second.leader, formationData.first);
                count++;
            }
        }
        else
        {
            for (auto formationData : formationList)
            {
                if (boost::istarts_with(formationData.first, search.value()))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_FORMATION_LIST_ITEM, formationData.second.leader, formationData.first);
                    count++;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Formations");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }
};
void AddSC_F_formation_commandscript()
{
    new F_formation_commandscript();
}
