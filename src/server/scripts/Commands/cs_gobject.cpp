/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
Name: gobject_commandscript
%Complete: 100
Comment: All gobject related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "GameEventMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "Language.h"
#include "Log.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "PoolMgr.h"
#include "RBAC.h"
#include "WorldSession.h"
#include <sstream>
#include "Utilities/ArgumentTokenizer.h"
#include "FreedomMgr.h"
#include "World.h"
#include "BattlenetAccountMgr.h"
#include "Config.h"

using namespace Trinity::ChatCommands;

using GameObjectSpawnId = Variant<Hyperlink<gameobject>, ObjectGuid::LowType>;
using GameObjectEntry = Variant<Hyperlink<gameobject_entry>, uint32>;

// definitions are over in cs_npc.cpp
bool HandleNpcSpawnGroup(ChatHandler* handler, std::vector<Variant<uint32, EXACT_SEQUENCE("force"), EXACT_SEQUENCE("ignorerespawn")>> const& opts);
bool HandleNpcDespawnGroup(ChatHandler* handler, std::vector<Variant<uint32, EXACT_SEQUENCE("removerespawntime")>> const& opts);

class gobject_commandscript : public CommandScript
{
public:
    gobject_commandscript() : CommandScript("gobject_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable addCommandTable =
        {
            { "",     HandleGameObjectAddCommand,     rbac::RBAC_PERM_COMMAND_GOBJECT_ADD,      Console::No },
            { "temp", HandleGameObjectAddTempCommand, rbac::RBAC_PERM_COMMAND_GOBJECT_ADD_TEMP, Console::No}
        };
        static ChatCommandTable gobjectCommandTable =
        {
            { "activate",       HandleGameObjectActivateCommand,  rbac::RBAC_PERM_COMMAND_GOBJECT_ACTIVATE,       Console::No },
            { "delete",         HandleGameObjectDeleteCommand,    rbac::RBAC_PERM_COMMAND_GOBJECT_DELETE,         Console::No },
            { "info",           HandleGameObjectInfoCommand,      rbac::RBAC_PERM_COMMAND_GOBJECT_INFO,           Console::No },
            { "move",           HandleGameObjectMoveCommand,      rbac::RBAC_PERM_COMMAND_GOBJECT_MOVE,           Console::No },
            { "near",           HandleGameObjectNearCommand,      rbac::RBAC_PERM_COMMAND_GOBJECT_NEAR,           Console::No },
            { "target",         HandleGameObjectTargetCommand,    rbac::RBAC_PERM_COMMAND_GOBJECT_TARGET,         Console::No },
            { "turn",           HandleGameObjectTurnCommand,      rbac::RBAC_PERM_COMMAND_GOBJECT_TURN,           Console::No },
            { "spawngroup",     HandleNpcSpawnGroup,              rbac::RBAC_PERM_COMMAND_GOBJECT_SPAWNGROUP,     Console::No },
            { "despawngroup",   HandleNpcDespawnGroup,            rbac::RBAC_PERM_COMMAND_GOBJECT_DESPAWNGROUP,   Console::No },
            { "add",            addCommandTable                                                                               },
            { "spawn",          addCommandTable                                                                               },
            { "set phase",      HandleGameObjectSetPhaseCommand,  rbac::RBAC_PERM_COMMAND_GOBJECT_SET_PHASE,      Console::No },
            { "set state",      HandleGameObjectSetStateCommand,  rbac::RBAC_PERM_COMMAND_GOBJECT_SET_STATE,      Console::No },

            { "phase",          HandleGameObjectPhaseCommand,     rbac::RBAC_FPERM_COMMAND_GOBJECT_PHASE,         Console::No},
            { "select",         HandleGameObjectSelectCommand,    rbac::RBAC_FPERM_COMMAND_GOBJECT_SELECT,        Console::No },
            { "scale",          HandleGameObjectScaleCommand,     rbac::RBAC_FPERM_COMMAND_GOBJECT_SCALE,         Console::No},
            { "axial",          HandleGameObjectAxialCommand,     rbac::RBAC_FPERM_COMMAND_GOBJECT_AXIAL,         Console::No},
            { "roll",           HandleGameObjectRollCommand,      rbac::RBAC_FPERM_COMMAND_GOBJECT_ROLL,          Console::No},
            { "pitch",          HandleGameObjectPitchCommand,     rbac::RBAC_FPERM_COMMAND_GOBJECT_PITCH,         Console::No},
            { "yaw",            HandleGameObjectYawCommand,       rbac::RBAC_FPERM_COMMAND_GOBJECT_YAW,           Console::No}
        };
        static ChatCommandTable commandTable =
        {
            { "gobject", gobjectCommandTable },
        };
        return commandTable;
    }

    static bool HandleGameObjectActivateCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        char* id = handler->extractKeyFromLink((char*)args, "Hgameobject");
        if (id)
            guidLow = atoul(id);

        if (!guidLow) {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        GameObjectTemplate const* goTemplate = sObjectMgr->GetGameObjectTemplate(goData->id);

        if (!goTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_TEMPLATE_DOES_NOT_EXIST, guidLow, goData->id);
            return true;
        }
        uint32_t const autoCloseTime = object->GetGOInfo()->GetAutoCloseTime() ? 10000u : 0u;

        // Activate
        object->SetLootState(GO_READY);
        object->UseDoorOrButton(autoCloseTime, false, handler->GetSession()->GetPlayer());

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_ACTIVATED, sFreedomMgr->ToChatLink("Hgameobject", guidLow, goTemplate->name));

        return true;
    }

    //spawn go
    static bool HandleGameObjectAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GAMEOBJECT_SPAWN);
            return true;
        }

        // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
        std::string id = sFreedomMgr->GetChatLinkKey(args, "Hgameobject_entry");
        uint32 objectId = atoul(id.c_str());

        if (!objectId)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SPAWN_INVALID_ID);
            return true;
        }

        char* spawnTimeSecsToken = strtok(nullptr, " ");
        uint32 spawnTimeSecs = 0;
        if (spawnTimeSecsToken)
            spawnTimeSecs = atoul(spawnTimeSecsToken);

        const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(objectId);

        if (!objectInfo)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_ENTRY_NOT_EXISTS, objectId);
            return true;
        }

        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SPAWN_INVALID_DISPLAY_C, objectId, objectInfo->displayId);
            //return true; -- CUSTOM, testing force-spawning with allegedly invalid display IDs
        }

        Player* source = handler->GetSession()->GetPlayer();
        float x, y, z, o;
        source->GetPosition(x, y, z, o);
        Map* map = source->GetMap();

        if (auto extraData = sFreedomMgr->GetGameObjectTemplateExtraData(objectId))
        {
            if (extraData->disabled)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_IS_BLACKLISTED);
                return true;
            }
        }

        GameObject* object = sFreedomMgr->GameObjectCreate(source, objectInfo, spawnTimeSecs);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SPAWN_FAIL, objectId);
            return true;
        }

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_SPAWN,
            sFreedomMgr->ToChatLink("Hgameobject", object->GetSpawnId(), objectInfo->name),
            object->GetSpawnId(),
            objectInfo->entry, x, y, z);
        sFreedomMgr->SetGameobjectSelectionForPlayer(source->GetGUID().GetCounter(), object->GetSpawnId());
        return true;
    }

    // add go, temp only
    static bool HandleGameObjectAddTempCommand(ChatHandler* handler, GameObjectEntry objectId, Optional<uint64> spawntime)
    {
        Player* player = handler->GetSession()->GetPlayer();
        Seconds spawntm(spawntime.value_or(300));

        if (!sObjectMgr->GetGameObjectTemplate(objectId))
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

        player->SummonGameObject(objectId, *player, QuaternionData::fromEulerAnglesZYX(player->GetOrientation(), 0.0f, 0.0f), spawntm);

        return true;
    }

    static bool HandleGameObjectTargetCommand(ChatHandler* handler, Optional<Variant<GameObjectEntry, std::string_view>> objectId)
    {
        Player* player = handler->GetSession()->GetPlayer();
        QueryResult result;
        GameEventMgr::ActiveEvents const& activeEventsList = sGameEventMgr->GetActiveEventList();

        if (objectId)
        {
            if (objectId->holds_alternative<GameObjectEntry>())
            {
                result = WorldDatabase.PQuery("SELECT guid, id, position_x, position_y, position_z, orientation, map, PhaseId, PhaseGroup, (POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) AS order_ FROM gameobject WHERE map = '%i' AND id = '%u' ORDER BY order_ ASC LIMIT 1",
                    player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), static_cast<uint32>(objectId->get<GameObjectEntry>()));
            }
            else
            {
                std::string name = std::string(objectId->get<std::string_view>());
                WorldDatabase.EscapeString(name);
                result = WorldDatabase.PQuery(
                    "SELECT guid, id, position_x, position_y, position_z, orientation, map, PhaseId, PhaseGroup, (POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ "
                    "FROM gameobject LEFT JOIN gameobject_template ON gameobject_template.entry = gameobject.id WHERE map = %i AND name LIKE '%%%s%%' ORDER BY order_ ASC LIMIT 1",
                    player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), name.c_str());
            }
        }
        else
        {
            std::ostringstream eventFilter;
            eventFilter << " AND (eventEntry IS NULL ";
            bool initString = true;

            for (GameEventMgr::ActiveEvents::const_iterator itr = activeEventsList.begin(); itr != activeEventsList.end(); ++itr)
            {
                if (initString)
                {
                    eventFilter  <<  "OR eventEntry IN (" << *itr;
                    initString = false;
                }
                else
                    eventFilter << ',' << *itr;
            }

            if (!initString)
                eventFilter << "))";
            else
                eventFilter << ')';

            result = WorldDatabase.PQuery("SELECT gameobject.guid, id, position_x, position_y, position_z, orientation, map, PhaseId, PhaseGroup, "
                "(POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ FROM gameobject "
                "LEFT OUTER JOIN game_event_gameobject on gameobject.guid = game_event_gameobject.guid WHERE map = '%i' %s ORDER BY order_ ASC LIMIT 10",
                handler->GetSession()->GetPlayer()->GetPositionX(), handler->GetSession()->GetPlayer()->GetPositionY(), handler->GetSession()->GetPlayer()->GetPositionZ(),
                handler->GetSession()->GetPlayer()->GetMapId(), eventFilter.str().c_str());
        }

        if (!result)
        {
            handler->SendSysMessage(LANG_COMMAND_TARGETOBJNOTFOUND);
            return true;
        }

        bool found = false;
        float x, y, z, o;
        ObjectGuid::LowType guidLow;
        uint32 id, phaseId, phaseGroup;
        uint16 mapId;
        uint32 poolId;

        do
        {
            Field* fields = result->Fetch();
            guidLow =       fields[0].GetUInt64();
            id =            fields[1].GetUInt32();
            x =             fields[2].GetFloat();
            y =             fields[3].GetFloat();
            z =             fields[4].GetFloat();
            o =             fields[5].GetFloat();
            mapId =         fields[6].GetUInt16();
            phaseId =       fields[7].GetUInt32();
            phaseGroup =    fields[8].GetUInt32();
            poolId =  sPoolMgr->IsPartOfAPool<GameObject>(guidLow);
            if (!poolId || sPoolMgr->IsSpawnedObject<GameObject>(player->GetMap()->GetPoolData(), guidLow))
                found = true;
        } while (result->NextRow() && !found);

        if (!found)
        {
            handler->PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
            return false;
        }

        GameObjectTemplate const* objectInfo = sObjectMgr->GetGameObjectTemplate(id);

        if (!objectInfo)
        {
            handler->PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
            return false;
        }

        GameObject* target = handler->GetObjectFromPlayerMapByDbGuid(guidLow);

        handler->PSendSysMessage(LANG_GAMEOBJECT_DETAIL, std::to_string(guidLow).c_str(), objectInfo->name.c_str(), std::to_string(guidLow).c_str(), id, x, y, z, mapId, o, phaseId, phaseGroup);

        if (target)
        {
            int32 curRespawnDelay = int32(target->GetRespawnTimeEx() - GameTime::GetGameTime());
            if (curRespawnDelay < 0)
                curRespawnDelay = 0;

            std::string curRespawnDelayStr = secsToTimeString(curRespawnDelay, TimeFormat::ShortText);
            std::string defRespawnDelayStr = secsToTimeString(target->GetRespawnDelay(), TimeFormat::ShortText);

            handler->PSendSysMessage(LANG_COMMAND_RAWPAWNTIMES, defRespawnDelayStr.c_str(), curRespawnDelayStr.c_str());
        }
        return true;
    }

    //delete object by selection or guid
    static bool HandleGameObjectDeleteCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
        char* id = handler->extractKeyFromLink((char*)args, "Hgameobject");
        if (id)
            guidLow = strtoull(id, nullptr, 10);

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;

        // by DB guid
        if (GameObjectData const* gameObjectData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, gameObjectData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        ObjectGuid ownerGuid = object->GetOwnerGUID();
        if (!ownerGuid.IsEmpty())
        {
            Unit* owner = ObjectAccessor::GetUnit(*handler->GetSession()->GetPlayer(), ownerGuid);
            if (!owner || !ownerGuid.IsPlayer())
            {
                handler->PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, ownerGuid.ToString().c_str(), object->GetGUID().ToString().c_str());
                return true;
            }

            owner->RemoveGameObject(object, false);
        }

        sFreedomMgr->GameObjectDelete(object);

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_DELETE);

        return true;
    }

    //turn selected object
    static bool HandleGameObjectTurnCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());
        float o = source->GetOrientation();

        // Prepare tokenizer with command modifiers
        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-guid", 1);
        tokenizer.LoadModifier("-adeg", 1);
        tokenizer.LoadModifier("-sdeg", 1);

        if (tokenizer.ModifierExists("-guid"))
        {
            std::string guidValue = tokenizer.GetModifierValue("-guid", 0);
            std::string guidKey = sFreedomMgr->GetChatLinkKey(guidValue, "Hgameobject");
            guidLow = atoul(guidKey.c_str());
        }

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;

        // by DB guid
        if (GameObjectData const* gameObjectData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, gameObjectData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        if (tokenizer.ModifierExists("-adeg"))
        {
            std::string addDeg = tokenizer.GetModifierValue("-adeg", 0);
            o = ((float)atof(addDeg.c_str())) * M_PI / 180.0f + object->GetOrientation();
        }

        if (tokenizer.ModifierExists("-sdeg"))
        {
            std::string setDeg = tokenizer.GetModifierValue("-sdeg", 0);
            o = ((float)atof(setDeg.c_str())) * M_PI / 180.0f;
        }

        sFreedomMgr->GameObjectTurn(object, o);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        object = sFreedomMgr->GameObjectRefresh(object);

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_TURNED,
            sFreedomMgr->ToChatLink("Hgameobject", guidLow, object->GetGOInfo()->name),
            guidLow);

        return true;
    }

    //move selected object
    static bool HandleGameObjectMoveCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        // Prepare tokenizer with command modifiers
        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-guid", 1);
        tokenizer.LoadModifier("-adeg", 1);
        tokenizer.LoadModifier("-sdeg", 1);
        tokenizer.LoadModifier("-po", 0);

        if (tokenizer.ModifierExists("-guid"))
        {
            std::string guidValue = tokenizer.GetModifierValue("-guid", 0);
            std::string guidKey = sFreedomMgr->GetChatLinkKey(guidValue, "Hgameobject");
            guidLow = atoul(guidKey.c_str());
        }

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;

        // by DB guid
        if (GameObjectData const* gameObjectData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, gameObjectData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        float o = object->GetOrientation();

        if (tokenizer.ModifierExists("-po"))
        {
            o = source->GetOrientation();
        }

        if (tokenizer.ModifierExists("-adeg"))
        {
            std::string addDeg = tokenizer.GetModifierValue("-adeg", 0);
            o = ((float)atof(addDeg.c_str())) * M_PI / 180.0f + o;
        }

        if (tokenizer.ModifierExists("-sdeg"))
        {
            std::string setDeg = tokenizer.GetModifierValue("-sdeg", 0);
            o = ((float)atof(setDeg.c_str())) * M_PI / 180.0f;
        }

        float x, y, z;
        if (tokenizer.empty())
        {
            Player* player = handler->GetSession()->GetPlayer();
            player->GetPosition(x, y, z);
        }
        else
        {
            float add_x = tokenizer.TryGetParam<float>(0);
            float add_y = tokenizer.TryGetParam<float>(1);
            float add_z = tokenizer.TryGetParam<float>(2);
            // rotation matrix
            x = add_x*cos(o) - add_y*sin(o) + object->GetPositionX();
            y = add_x*sin(o) + add_y*cos(o) + object->GetPositionY();
            z = add_z + object->GetPositionZ();

            if (!MapManager::IsValidMapCoord(object->GetMapId(), x, y, z))
            {
                handler->PSendSysMessage(FREEDOM_CMDE_INVALID_TARGET_COORDS, x, y, z, object->GetMapId());
                return true;
            }
        }

        sFreedomMgr->GameObjectMove(object, x, y, z, o);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        object = sFreedomMgr->GameObjectRefresh(object);

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_MOVE,
            sFreedomMgr->ToChatLink("Hgameobject", guidLow, object->GetGOInfo()->name),
            guidLow);

        return true;
    }

    //set phasemask for selected object
    static bool HandleGameObjectSetPhaseCommand(ChatHandler* handler, GameObjectSpawnId guidLow, uint32 phaseId)
    {
        if (!guidLow)
            return false;

        GameObject* object = handler->GetObjectFromPlayerMapByDbGuid(guidLow);
        if (!object)
        {
            handler->PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, std::to_string(*guidLow).c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!phaseId)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PhasingHandler::AddPhase(object, phaseId, true);
        object->SaveToDB();
        return true;
    }

    static bool HandleGameObjectNearCommand(ChatHandler* handler, Optional<float> dist)
    {
        float distance = dist.value_or(10.0f);
        uint32 count = 0;

        Player* player = handler->GetSession()->GetPlayer();
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NEAREST_GAMEOBJECTS);
        stmt->setFloat(0, player->GetPositionX());
        stmt->setFloat(1, player->GetPositionY());
        stmt->setFloat(2, player->GetPositionZ());
        stmt->setUInt32(3, player->GetMapId());
        stmt->setFloat(4, player->GetPositionX());
        stmt->setFloat(5, player->GetPositionY());
        stmt->setFloat(6, player->GetPositionZ());
        stmt->setFloat(7, distance * distance);
        stmt->setUInt32(8, maxResults);
        PreparedQueryResult result = WorldDatabase.Query(stmt);

      if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                ObjectGuid::LowType guid = fields[0].GetUInt64();
                uint32 entry = fields[1].GetUInt32();
                double objDist = fields[2].GetDouble();

                GameObjectTemplate const* gameObjectInfo = sObjectMgr->GetGameObjectTemplate(entry);

                if (!gameObjectInfo)
                    continue;

                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_NEAR_LIST_ITEM,
                    objDist,
                    entry,
                    sFreedomMgr->ToChatLink("Hgameobject", guid, gameObjectInfo->name));

                ++count;
            } while (result->NextRow());
        }

        if (count == maxResults)
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_NEAR_MAX_RESULT_COUNT, count, distance);
        else
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_NEAR, count, distance);
        return true;
    }

    //show info of gameobject
    static bool HandleGameObjectInfoCommand(ChatHandler* handler,char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());
        bool historyInfo = false;
        bool positionInfo = false;
        bool advancedInfo = false;

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-full", 0);
        tokenizer.LoadModifier("-pos", 0);
        tokenizer.LoadModifier("-history", 0);
        tokenizer.LoadModifier("-advanced", 0);

        std::string id = tokenizer.TryGetParam(0);
        if (!id.empty())
            guidLow = atoul(id.c_str());

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        if (tokenizer.ModifierExists("-full"))
        {
            historyInfo = true;
            positionInfo = true;
            advancedInfo = true;
        }

        if (tokenizer.ModifierExists("-history"))
            historyInfo = true;

        if (tokenizer.ModifierExists("-pos"))
            positionInfo = true;

        if (tokenizer.ModifierExists("-advanced"))
            advancedInfo = true;

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        GameObjectTemplate const* goTemplate = sObjectMgr->GetGameObjectTemplate(goData->id);

        if (!goTemplate)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_TEMPLATE_DOES_NOT_EXIST, guidLow, goData->id);
            return true;
        }

        // Gather data
        float px, py, pz, po;
        source->GetPosition(px, py, pz, po);
        std::string guidString = object->GetGUID().ToString();
        uint32 entryId = goTemplate->entry;
        uint32 displayId = goTemplate->displayId;
        uint32 scriptId = object->GetScriptId();
        std::string name = goTemplate->name;
        std::string aiName = object->GetAIName();
        //uint32 phaseMask = object->GetPhaseMask();
        //auto phaseList = object->GetPhases();  TO UPDATE
        float ox, oy, oz, oo;
        object->GetPosition(ox, oy, oz, oo);
        uint32 mapId = object->GetMapId();
        float distance = std::sqrt(std::pow(ox - px, 2) + std::pow(oy - py, 2) + std::pow(oz - pz, 2));
        float scale = object->GetObjectScale();
        GOState state = object->GetGoState();
        GameObjectExtraData const* extraData = sFreedomMgr->GetGameObjectExtraData(object->GetSpawnId());

        // Display data
        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO, sFreedomMgr->ToChatLink("Hgameobject", guidLow, name));
        if (advancedInfo)
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_GUID_FULL, guidString);
        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_GUID, guidLow);
        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_ENTRY, entryId, sFreedomMgr->ToChatLink("Hgameobject_entry", entryId, "Spawn link"));
        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_DISPLAY_ID, displayId);

        if (advancedInfo)
        {
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_SCRIPT_ID, scriptId);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_NAME, name);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_AI_NAME, aiName);
            //handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_PHASEMASK, phaseMask);

            std::string stateEnumName;

            switch (state)
            {
            case GO_STATE_ACTIVE:
                stateEnumName = "GO_STATE_ACTIVE";
                break;
            case GO_STATE_READY:
                stateEnumName = "GO_STATE_READY";
                break;
            case GO_STATE_TRANSPORT_ACTIVE:
                stateEnumName = "GO_STATE_TRANSPORT_ACTIVE";
                break;
            case GO_STATE_TRANSPORT_STOPPED:
                stateEnumName = "GO_STATE_TRANSPORT_STOPPED";
                break;
            default:
                stateEnumName = "UNKNOWN";
                break;
            }

            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_STATE, (uint32)state, stateEnumName);
        }

        if (positionInfo)
        {
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_POS_X, ox);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_POS_Y, oy);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_POS_Z, oz);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_ORIENTATION, oo, round((oo * 180 / M_PI) * 100) / 100.0f);

            float rad_roll, rad_pitch, rad_yaw;
            float deg_roll, deg_pitch, deg_yaw;
            sFreedomMgr->GetGameObjectEulerAnglesRad(object, rad_roll, rad_pitch, rad_yaw);
            sFreedomMgr->GetGameObjectEulerAnglesDeg(object, deg_roll, deg_pitch, deg_yaw);

            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_ROLL, rad_roll, deg_roll);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_PITCH, rad_pitch, deg_pitch);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_YAW, rad_yaw, deg_yaw);
        }

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_DISTANCE, distance);
        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_SCALE, scale);
        /* TO UPDATE
        if (!phaseList.empty())
        {
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_PHASELIST);

            for (uint32 phaseId : phaseList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_PHASE_LI, phaseId, sFreedomMgr->GetPhaseMask(phaseId));
            }
        }
        */
        if (historyInfo)
        {
            if (!extraData)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_NO_HISTORY);
            }
            else
            {
                // Gather history data
                uint64 creatorPlayerId = extraData->creatorPlayerId;
                std::string creatorPlayerName = "(UNKNOWN)";
                uint32 creatorBnetId = extraData->creatorBnetAccId;
                std::string creatorAccountName = "(UNKNOWN)";
                uint64 modifierPlayerId = extraData->modifierPlayerId;
                std::string modifierPlayerName = "(UNKNOWN)";
                uint32 modifierBnetId = extraData->modifierBnetAccId;
                std::string modifierAccountName = "(UNKNOWN)";
                time_t created = extraData->created;
                std::string createdTimestamp = "(UNKNOWN)";
                time_t modified = extraData->modified;
                std::string modifiedTimestamp = "(UNKNOWN)";

                // Set names
                if (CharacterCacheEntry const* creatorPlayerInfo = sCharacterCache->GetCharacterCacheByGuid(ObjectGuid::Create<HighGuid::Player>(creatorPlayerId)))
                    creatorPlayerName = creatorPlayerInfo->Name;
                if (CharacterCacheEntry const* modifierPlayerInfo = sCharacterCache->GetCharacterCacheByGuid(ObjectGuid::Create<HighGuid::Player>(modifierPlayerId)))
                    modifierPlayerName = modifierPlayerInfo->Name;

                Battlenet::AccountMgr::GetName(creatorBnetId, creatorAccountName);
                Battlenet::AccountMgr::GetName(modifierBnetId, modifierAccountName);

                // Set timestamps
                if (created)
                    createdTimestamp = sFreedomMgr->ToDateTimeString(created);

                if (modified)
                    modifiedTimestamp = sFreedomMgr->ToDateTimeString(modified);

                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_CREATOR_PLAYER, creatorPlayerName, creatorPlayerId);
                if (handler->HasPermission(rbac::RBAC_FPERM_ADMINISTRATION))
                    handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_CREATOR_ACC, creatorAccountName, creatorBnetId);
                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_MODIFIER_PLAYER, modifierPlayerName, modifierPlayerId);
                if (handler->HasPermission(rbac::RBAC_FPERM_ADMINISTRATION))
                    handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_MODIFIER_ACC, modifierAccountName, modifierBnetId);
                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_CREATED, createdTimestamp);
                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_MODIFIED, modifiedTimestamp);
            }
        }

        return true;
    }

    static bool HandleGameObjectSetStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GAMEOBJECT_SET_STATE);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        std::string type = tokenizer.TryGetParam(0);
        std::string state = tokenizer.TryGetParam(1);
        std::string id = tokenizer.TryGetParam(2);

        if (!id.empty())
            guidLow = atoul(sFreedomMgr->GetChatLinkKey(id, "Hgameobject").c_str());

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;

        // by DB guid
        if (GameObjectData const* gameObjectData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, gameObjectData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        int32 objectType = atoi(type.c_str());
        if (objectType < 0)
        {
            if (objectType == -1)
                object->SendGameObjectDespawn();
            else if (objectType == -2)
                handler->PSendSysMessage(FREEDOM_CMDH_GAMEOBJECT_SET_STATE);
            return true;
        }

        if (state.empty())
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GAMEOBJECT_SET_STATE);
            return true;
        }

        int32 objectState = atoi(state.c_str());


        switch (objectType)
        {
            case 0:
                object->SetGoState(GOState(objectState));
                break;
            case 1:
                object->SetGoType(GameobjectTypes(objectState));
                break;
            case 2:
                object->SetGoArtKit(objectState);
                break;
            case 3:
                object->SetGoAnimProgress(objectState);
                break;
            case 4:
                object->SendCustomAnim(objectState);
                break;
            case 5:
                // removed from original legion code
                // if (objectState > GO_DESTRUCTIBLE_REBUILDING)
                //     return false;

                // object->SetDestructibleState(GameObjectDestructibleState(objectState));
                break;
            default:
                break;
        }

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_SET_STATE, objectType, objectState);
        return true;
    }

    // New
    static bool HandleGameObjectPhaseCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GAMEOBJECT_PHASE);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());
        bool parseAsPhaseMasks = true;

        ArgumentTokenizer tokenizer(args);
        tokenizer.LoadModifier("-ids", 0);
        tokenizer.LoadModifier("-guid", 1);

        if (tokenizer.ModifierExists("-guid"))
        {
            std::string guidValue = tokenizer.GetModifierValue("-guid", 0);
            std::string guidKey = sFreedomMgr->GetChatLinkKey(guidValue, "Hgameobject");
            guidLow = atoul(guidKey.c_str());
        }

        if (tokenizer.ModifierExists("-ids"))
        {
            parseAsPhaseMasks = false;
        }

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        std::string phases = "";
        uint32 phaseMask = 0;

        for (auto phaseElem : tokenizer)
        {
            uint32 phase = atoul(phaseElem.c_str());

            if (!phase)
                continue;

            if (parseAsPhaseMasks)
            {
                if (!sFreedomMgr->IsValidPhaseMask(phase))
                    continue;

                phaseMask |= phase;
                phases += " " + phaseElem;
            }
            else
            {
                if (!sFreedomMgr->IsValidPhaseId(phase))
                    continue;

                phaseMask |= sFreedomMgr->GetPhaseMask(phase);
                phases += " " + phaseElem;
            }
        }

        if (phaseMask)
        {
            sFreedomMgr->GameObjectPhase(object, phaseMask);
            sFreedomMgr->GameObjectSetModifyHistory(object, source);
            sFreedomMgr->SaveGameObject(object);
            object = sFreedomMgr->GameObjectRefresh(object);
        }

        if (phases.empty())
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_PHASE_NOT_SET);
        else if (parseAsPhaseMasks)
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_PHASE_BITFIELDS, phases);
        else
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_PHASE_PHASEIDS, phases);

        return true;
    }

    static bool HandleGameObjectSelectCommand(ChatHandler* handler, char const* args)
    {
        uint32 entryId = 0;
        Player* source = handler->GetSession()->GetPlayer();
        float x, y, z;
        source->GetPosition(x, y, z);
        uint32 mapId = source->GetMapId();
        int index = 0;
        PreparedQueryResult result;

        if (*args)
        {
            char* token = strtok((char*)args, " ");
            entryId = atoul(token);
        }

        if (entryId)
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NEAREST_GAMEOBJECT_BY_EID);
            stmt->setFloat(index++, x);
            stmt->setFloat(index++, y);
            stmt->setFloat(index++, z);
            stmt->setUInt16(index++, mapId);
            stmt->setUInt32(index++, entryId);

            result = WorldDatabase.Query(stmt);
        }
        else
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NEAREST_GAMEOBJECT);
            stmt->setFloat(index++, x);
            stmt->setFloat(index++, y);
            stmt->setFloat(index++, z);
            stmt->setUInt16(index++, mapId);

            result = WorldDatabase.Query(stmt);
        }

        if (result)
        {
            Field * fields = result->Fetch();

            uint64 guidLow = fields[0].GetUInt64();
            uint32 entryId = fields[1].GetUInt32();
            double objDist = fields[2].GetDouble();

            GameObjectTemplate const* objTemplate = sObjectMgr->GetGameObjectTemplate(entryId);

            if (!objTemplate)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SELECT_ENTRY_ID, entryId, guidLow);
                return true;
            }

            GameObject* obj = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, entryId);

            if (!obj)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_SELECT_NOT_IN_WORLD, guidLow, entryId);
                return true;
            }

            sFreedomMgr->SetGameobjectSelectionForPlayer(source->GetGUID().GetCounter(), guidLow);
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_SELECT,
                sFreedomMgr->ToChatLink("Hgameobject", guidLow, objTemplate->name),
                guidLow,
                entryId,
                objDist);

            return true;
        }
        else
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }
    }

    static bool HandleGameObjectScaleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GAMEOBJECT_SCALE);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        std::string scaleToken = tokenizer.TryGetParam(0);
        std::string id = tokenizer.TryGetParam(1);
        float scale = atof(scaleToken.c_str());

        if (!id.empty())
        {
            guidLow = atoul(sFreedomMgr->GetChatLinkKey(id, "Hgameobject").c_str());
        }

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;

        // by DB guid
        if (GameObjectData const* gameObjectData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, gameObjectData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        float maxScale = sConfigMgr->GetFloatDefault("Freedom.Gameobject.MaxScale", 15.0f);
        float minScale = sConfigMgr->GetFloatDefault("Freedom.Gameobject.MinScale", 0.0f);

        if (scale < minScale)
            scale = minScale;
        if (scale > maxScale)
            scale = maxScale;

        sFreedomMgr->GameObjectScale(object, scale);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        sFreedomMgr->GameObjectRefresh(object);

        handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_SCALE, scale);

        return true;
    }

    static bool HandleGameObjectAxialCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_AXIAL);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-adeg", 0);

        if (tokenizer.size() < 3)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_AXIAL);
            return true;
        }

        bool addDeg = tokenizer.ModifierExists("-adeg");
        float deg_x = tokenizer.TryGetParam<float>(0);
        float deg_y = tokenizer.TryGetParam<float>(1);
        float deg_z = tokenizer.TryGetParam<float>(2);

        std::string id = tokenizer.TryGetParam(3);
        if (!id.empty())
            guidLow = atoul(id.c_str());

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        // Set axial angle, save and refresh
        sFreedomMgr->GameObjectRotate(object, deg_x, deg_y, deg_z, addDeg);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        object = sFreedomMgr->GameObjectRefresh(object);

        float deg_roll, deg_pitch, deg_yaw;
        sFreedomMgr->GetGameObjectEulerAnglesDeg(object, deg_roll, deg_pitch, deg_yaw);
        handler->PSendSysMessage(FREEDOM_CMDI_GOBJECT_AXIAL, deg_roll, deg_pitch, deg_yaw);
        return true;
    }
    static bool HandleGameObjectRollCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_ROLL);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-adeg", 0);

        if (tokenizer.size() < 1)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_ROLL);
            return true;
        }

        bool addDeg = tokenizer.ModifierExists("-adeg");
        float deg_x = tokenizer.TryGetParam<float>(0);

        std::string id = tokenizer.TryGetParam(1);
        if (!id.empty())
            guidLow = atoul(id.c_str());

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        // Set axial angle, save and refresh
        sFreedomMgr->GameObjectRotateSingleAxis(object, deg_x, AXIS_ROLL, addDeg);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        object = sFreedomMgr->GameObjectRefresh(object);

        float deg_roll, deg_pitch, deg_yaw;
        sFreedomMgr->GetGameObjectEulerAnglesDeg(object, deg_roll, deg_pitch, deg_yaw);
        handler->PSendSysMessage(FREEDOM_CMDI_GOBJECT_ROLL, deg_roll);
        return true;
    }

    static bool HandleGameObjectPitchCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_PITCH);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-adeg", 0);

        if (tokenizer.size() < 1)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_PITCH);
            return true;
        }

        bool addDeg = tokenizer.ModifierExists("-adeg");
        float deg_y = tokenizer.TryGetParam<float>(0);

        std::string id = tokenizer.TryGetParam(1);
        if (!id.empty())
            guidLow = atoul(id.c_str());

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        // Set axial angle, save and refresh
        sFreedomMgr->GameObjectRotateSingleAxis(object, deg_y, AXIS_PITCH, addDeg);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        object = sFreedomMgr->GameObjectRefresh(object);

        float deg_roll, deg_pitch, deg_yaw;
        sFreedomMgr->GetGameObjectEulerAnglesDeg(object, deg_roll, deg_pitch, deg_yaw);
        handler->PSendSysMessage(FREEDOM_CMDI_GOBJECT_PITCH, deg_pitch);
        return true;
    }

    static bool HandleGameObjectYawCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_YAW);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid::LowType guidLow = sFreedomMgr->GetSelectedGameobjectGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-adeg", 0);

        if (tokenizer.size() < 1)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_GOBJECT_YAW);
            return true;
        }

        bool addDeg = tokenizer.ModifierExists("-adeg");
        float deg_z = tokenizer.TryGetParam<float>(0);

        std::string id = tokenizer.TryGetParam(1);
        if (!id.empty())
            guidLow = atoul(id.c_str());

        if (!guidLow)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_NOT_FOUND);
            return true;
        }

        GameObject* object = NULL;
        GameObjectData const* goData = NULL;

        // by DB guid
        if (goData = sObjectMgr->GetGameObjectData(guidLow))
            object = sFreedomMgr->GetAnyGameObject(source->GetMap(), guidLow, goData->id);

        if (!object)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_GUID_NOT_EXISTS, guidLow);
            return true;
        }

        // Set axial angle, save and refresh
        sFreedomMgr->GameObjectRotateSingleAxis(object, deg_z, AXIS_YAW, addDeg);
        sFreedomMgr->GameObjectSetModifyHistory(object, source);
        sFreedomMgr->SaveGameObject(object);
        object = sFreedomMgr->GameObjectRefresh(object);

        float deg_roll, deg_pitch, deg_yaw;
        sFreedomMgr->GetGameObjectEulerAnglesDeg(object, deg_roll, deg_pitch, deg_yaw);
        handler->PSendSysMessage(FREEDOM_CMDI_GOBJECT_YAW, deg_yaw);
        return true;
    }
};

void AddSC_gobject_commandscript()
{
    new gobject_commandscript();
}
