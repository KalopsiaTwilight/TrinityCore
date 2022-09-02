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
Name: npc_commandscript
%Complete: 100
Comment: All npc related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "CreatureAI.h"
#include "CreatureGroups.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "FollowMovementGenerator.h"
#include "GameTime.h"
#include "Language.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MovementDefines.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "RBAC.h"
#include "SmartEnum.h"
#include "Transport.h"
#include "World.h"
#include "WorldSession.h"
#include "FreedomMgr.h"
#include "Utilities/ArgumentTokenizer.h"
#include "BattlenetAccountMgr.h"
#include "CharacterCache.h"
#include "Config.h"

using namespace Trinity::ChatCommands;

using CreatureSpawnId = Variant<Hyperlink<creature>, ObjectGuid::LowType>;
using CreatureEntry = Variant<Hyperlink<creature_entry>, uint32>;

// shared with cs_gobject.cpp, definitions are at the bottom of this file
bool HandleNpcSpawnGroup(ChatHandler* handler, std::vector<Variant<uint32, EXACT_SEQUENCE("force"), EXACT_SEQUENCE("ignorerespawn")>> const& opts);
bool HandleNpcDespawnGroup(ChatHandler* handler, std::vector<Variant<uint32, EXACT_SEQUENCE("removerespawntime")>> const& opts);

enum AuraSpells
{
    SPELL_PERMANENT_FEIGN_DEATH = 114371,
    SPELL_PERMANENT_SLEEP_VISUAL = 107674,
    SPELL_PERMANENT_HOVER = 138092
};

class npc_commandscript : public CommandScript
{
public:
    npc_commandscript() : CommandScript("npc_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable npcAddCommandTable =
        {
            { "formation",      HandleNpcAddFormationCommand,      rbac::RBAC_PERM_COMMAND_NPC_ADD_FORMATION,                  Console::No },
            { "item",           HandleNpcAddVendorItemCommand,     rbac::RBAC_PERM_COMMAND_NPC_ADD_ITEM,                       Console::No },
            { "move",           HandleNpcAddMoveCommand,           rbac::RBAC_PERM_COMMAND_NPC_ADD_MOVE,                       Console::No },
            { "temp",           HandleNpcAddTempSpawnCommand,      rbac::RBAC_PERM_COMMAND_NPC_ADD_TEMP,                       Console::No },
//          { "weapon",         HandleNpcAddWeaponCommand,         rbac::RBAC_PERM_COMMAND_NPC_ADD_WEAPON,                     Console::No },
            { "",               HandleNpcAddCommand,               rbac::RBAC_PERM_COMMAND_NPC_ADD,                            Console::No },
        };
        static ChatCommandTable npcSetCommandTable =
        {
            { "emote",          HandleNpcSetEmoteCommand,          rbac::RBAC_FPERM_COMMAND_NPC_SET_EMOTE,                     Console::No },
            { "mount",          HandleNpcSetMountCommand,          rbac::RBAC_FPERM_COMMAND_NPC_SET_MOUNT,                     Console::No },
            { "aura",           HandleNpcSetAuraCommand,           rbac::RBAC_FPERM_COMMAND_NPC_SET_AURA,                      Console::No },
            { "deathstate",     HandleNpcSetDeathStateCommand,     rbac::RBAC_FPERM_COMMAND_NPC_SET_DEATHSTATE,                Console::No },
            { "sleepstate",     HandleNpcSetSleepStateCommand,     rbac::RBAC_FPERM_COMMAND_NPC_SET_SLEEPSTATE,                Console::No },
            { "hoverstate",     HandleNpcSetHoverStateCommand,     rbac::RBAC_FPERM_COMMAND_NPC_SET_HOVERSTATE,                Console::No },
            { "allowmove",      HandleNpcSetAllowMovementCommand,  rbac::RBAC_PERM_COMMAND_NPC_SET_ALLOWMOVE,                  Console::No },
            { "entry",          HandleNpcSetEntryCommand,          rbac::RBAC_PERM_COMMAND_NPC_SET_ENTRY,                      Console::No },
            { "factionid",      HandleNpcSetFactionIdCommand,      rbac::RBAC_PERM_COMMAND_NPC_SET_FACTIONID,                  Console::No },
            { "flag",           HandleNpcSetFlagCommand,           rbac::RBAC_PERM_COMMAND_NPC_SET_FLAG,                       Console::No },
            { "level",          HandleNpcSetLevelCommand,          rbac::RBAC_PERM_COMMAND_NPC_SET_LEVEL,                      Console::No },
            { "link",           HandleNpcSetLinkCommand,           rbac::RBAC_PERM_COMMAND_NPC_SET_LINK,                       Console::No },
            { "model",          HandleNpcSetModelCommand,          rbac::RBAC_PERM_COMMAND_NPC_SET_MODEL,                      Console::No },
            { "movetype",       HandleNpcSetMoveTypeCommand,       rbac::RBAC_PERM_COMMAND_NPC_SET_MOVETYPE,                   Console::No },
            { "phase",          HandleNpcSetPhaseCommand,          rbac::RBAC_PERM_COMMAND_NPC_SET_PHASE,                      Console::No },
            { "phasegroup",     HandleNpcSetPhaseGroup,            rbac::RBAC_PERM_COMMAND_NPC_SET_PHASE,                      Console::No },
            { "scale",          HandleNpcSetScaleCommand,          rbac::RBAC_FPERM_COMMAND_NPC_SET_SCALE,                     Console::No },
            { "wanderdistance", HandleNpcSetWanderDistanceCommand, rbac::RBAC_PERM_COMMAND_NPC_SET_SPAWNDIST,                  Console::No },
            { "spawntime",      HandleNpcSetSpawnTimeCommand,      rbac::RBAC_PERM_COMMAND_NPC_SET_SPAWNTIME,                  Console::No },
            { "data",           HandleNpcSetDataCommand,           rbac::RBAC_PERM_COMMAND_NPC_SET_DATA,                       Console::No },
            { "bytes1",         HandleNpcSetBytes1Command,         rbac::RBAC_FPERM_COMMAND_NPC_SET_BYTES1,                    Console::No },
            { "bytes2",         HandleNpcSetBytes2Command,         rbac::RBAC_FPERM_COMMAND_NPC_SET_BYTES2,                    Console::No },
            { "sitgroundstate", HandleNpcSetSitGroundStateCommand, rbac::RBAC_FPERM_COMMAND_NPC_SET_SITGROUNDSTATE,            Console::No },
            { "sitlowstate",    HandleNpcSetSitLowStateCommand,    rbac::RBAC_FPERM_COMMAND_NPC_SET_SITLOWSTATE,               Console::No },
            { "sitmediumstate", HandleNpcSetSitMediumStateCommand, rbac::RBAC_FPERM_COMMAND_NPC_SET_SITMEDSTATE,               Console::No },
            { "sithighstate",   HandleNpcSetSitHighStateCommand,   rbac::RBAC_FPERM_COMMAND_NPC_SET_SITHIGHSTATE,              Console::No },
            { "kneelstate",     HandleNpcSetKneelStateCommand,     rbac::RBAC_FPERM_COMMAND_NPC_SET_KNEELSTATE,                Console::No },
            { "sheathstate",    HandleNpcSetSheathStateCommand,    rbac::RBAC_FPERM_COMMAND_NPC_SET_SHEATHSTATE,               Console::No },
            { "gravity",        HandleNpcSetGravityCommand,        rbac::RBAC_FPERM_COMMAND_NPC_SET_GRAVITY,                   Console::No },
            { "swim",           HandleNpcSetSwimCommand,           rbac::RBAC_FPERM_COMMAND_NPC_SET_SWIM,                      Console::No },
            { "flystate",       HandleNpcSetFlyStateCommand,       rbac::RBAC_FPERM_COMMAND_NPC_SET_FLYSTATE,                  Console::No },

        };
        static ChatCommandTable npcCommandTable =
        {
            { "add", npcAddCommandTable },
            { "set", npcSetCommandTable },
            { "info",           HandleNpcInfoCommand2,              rbac::RBAC_PERM_COMMAND_NPC_INFO,                           Console::No },
            { "near",           HandleNpcNearCommand,              rbac::RBAC_PERM_COMMAND_NPC_NEAR,                           Console::No },
            { "move",           HandleNpcMoveCommand,              rbac::RBAC_PERM_COMMAND_NPC_MOVE,                           Console::No },
            { "turn",           HandleNpcTurnCommand,              rbac::RBAC_FPERM_COMMAND_NPC_TURN,                          Console::No },
            { "playemote",      HandleNpcPlayEmoteCommand,         rbac::RBAC_PERM_COMMAND_NPC_PLAYEMOTE,                      Console::No },
            { "say",            HandleNpcSayCommand,               rbac::RBAC_PERM_COMMAND_NPC_SAY,                            Console::No },
            { "textemote",      HandleNpcTextEmoteCommand,         rbac::RBAC_PERM_COMMAND_NPC_TEXTEMOTE,                      Console::No },
            { "whisper",        HandleNpcWhisperCommand,           rbac::RBAC_PERM_COMMAND_NPC_WHISPER,                        Console::No },
            { "yell",           HandleNpcYellCommand,              rbac::RBAC_PERM_COMMAND_NPC_YELL,                           Console::No },
            { "tame",           HandleNpcTameCommand,              rbac::RBAC_PERM_COMMAND_NPC_TAME,                           Console::No },
            { "select",         HandleNpcSelectCommand,            rbac::RBAC_FPERM_COMMAND_NPC_SELECT,                        Console::No },
            { "spawn",          HandleNpcAddCommand,               rbac::RBAC_PERM_COMMAND_NPC_ADD,                            Console::No },
            { "return",         HandleNpcReturnCommand,            rbac::RBAC_FPERM_COMMAND_NPC_RETURN,                        Console::No },
            { "spawngroup",     HandleNpcSpawnGroup,               rbac::RBAC_PERM_COMMAND_NPC_SPAWNGROUP,                     Console::No },
            { "despawngroup",   HandleNpcDespawnGroup,             rbac::RBAC_PERM_COMMAND_NPC_DESPAWNGROUP,                   Console::No },
            { "delete",         HandleNpcDeleteCommand,            rbac::RBAC_PERM_COMMAND_NPC_DELETE,                         Console::No },
            { "delete item",    HandleNpcDeleteVendorItemCommand,  rbac::RBAC_PERM_COMMAND_NPC_DELETE_ITEM,                    Console::No },
            { "follow",         HandleNpcFollowCommand,            rbac::RBAC_PERM_COMMAND_NPC_FOLLOW,                         Console::No },
            { "follow stop",    HandleNpcUnFollowCommand,          rbac::RBAC_PERM_COMMAND_NPC_FOLLOW,                         Console::No },
            { "evade",          HandleNpcEvadeCommand,             rbac::RBAC_PERM_COMMAND_NPC_EVADE,                          Console::No },
            { "showloot",       HandleNpcShowLootCommand,          rbac::RBAC_PERM_COMMAND_NPC_SHOWLOOT,                       Console::No },
        };
        static ChatCommandTable commandTable =
        {
            { "npc", npcCommandTable },
        };
        return commandTable;
    }

    //add spawn of creature
    static bool HandleNpcAddCommand(ChatHandler* handler, CreatureEntry id)
    {
        if (!sObjectMgr->GetCreatureTemplate(id))
            return false;

        Player* chr = handler->GetSession()->GetPlayer();
        Map* map = chr->GetMap();

        if (Transport* trans = dynamic_cast<Transport*>(chr->GetTransport()))
        {
            ObjectGuid::LowType guid = sObjectMgr->GenerateCreatureSpawnId();
            CreatureData& data = sObjectMgr->NewOrExistCreatureData(guid);
            data.spawnId = guid;
            data.spawnGroupData = sObjectMgr->GetDefaultSpawnGroup();
            data.id = id;
            data.spawnPoint.Relocate(chr->GetTransOffsetX(), chr->GetTransOffsetY(), chr->GetTransOffsetZ(), chr->GetTransOffsetO());
            if (Creature* creature = trans->CreateNPCPassenger(guid, &data))
            {
                creature->SaveToDB(trans->GetGOInfo()->moTransport.SpawnMap, { map->GetDifficultyID() });
                sObjectMgr->AddCreatureToGrid(&data);
            }
            return true;
        }

        Creature* creature = Creature::CreateCreature(id, map, chr->GetPosition());
        if (!creature)
            return false;

        PhasingHandler::InheritPhaseShift(creature, chr);
        creature->SaveToDB(map->GetId(), { map->GetDifficultyID() });

        ObjectGuid::LowType db_guid = creature->GetSpawnId();

        // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells()
        // current "creature" variable is deleted and created fresh new, otherwise old values might trigger asserts or cause undefined behavior
        creature->CleanupsBeforeDelete();
        delete creature;

        creature = Creature::CreateCreatureFromDB(db_guid, map, true, true);
        if (!creature)
            return false;

        sObjectMgr->AddCreatureToGrid(sObjectMgr->GetCreatureData(db_guid));
        return true;
    }

    //add item in vendorlist
    static bool HandleNpcAddVendorItemCommand(ChatHandler* handler, ItemTemplate const* item, Optional<uint32> mc, Optional<uint32> it, Optional<uint32> ec, Optional<std::string_view> bonusListIDs)
    {
        if (!item)
        {
            handler->SendSysMessage(LANG_COMMAND_NEEDITEMSEND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* vendor = handler->getSelectedCreature();
        if (!vendor)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 itemId = item->GetId();
        uint32 maxcount = mc.value_or(0);
        uint32 incrtime = it.value_or(0);
        uint32 extendedcost = ec.value_or(0);
        uint32 vendor_entry = vendor->GetEntry();

        VendorItem vItem;
        vItem.item = itemId;
        vItem.maxcount = maxcount;
        vItem.incrtime = incrtime;
        vItem.ExtendedCost = extendedcost;
        vItem.Type = ITEM_VENDOR_TYPE_ITEM;

        if (bonusListIDs)
            for (std::string_view token : Trinity::Tokenize(*bonusListIDs, ';', false))
                if (Optional<int32> bonusListID = Trinity::StringTo<int32>(token))
                    vItem.BonusListIDs.push_back(*bonusListID);

        if (!sObjectMgr->IsVendorItemValid(vendor_entry, vItem, handler->GetSession()->GetPlayer()))
        {
            handler->SetSentErrorMessage(true);
            return false;
        }

        sObjectMgr->AddVendorItem(vendor_entry, vItem);

        handler->PSendSysMessage(LANG_ITEM_ADDED_TO_LIST, itemId, item->GetDefaultLocaleName(), maxcount, incrtime, extendedcost);
        return true;
    }

    //add move for creature
    static bool HandleNpcAddMoveCommand(ChatHandler* handler, CreatureSpawnId lowGuid)
    {
        // attempt check creature existence by DB data
        CreatureData const* data = sObjectMgr->GetCreatureData(lowGuid);
        if (!data)
        {
            handler->PSendSysMessage(LANG_COMMAND_CREATGUIDNOTFOUND, std::to_string(*lowGuid).c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Update movement type
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_MOVEMENT_TYPE);

        stmt->setUInt8(0, uint8(WAYPOINT_MOTION_TYPE));
        stmt->setUInt64(1, lowGuid);

        WorldDatabase.Execute(stmt);

        handler->SendSysMessage(LANG_WAYPOINT_ADDED);

        return true;
    }

    static bool HandleNpcSetAllowMovementCommand(ChatHandler* handler)
    {
        if (sWorld->getAllowMovement())
        {
            sWorld->SetAllowMovement(false);
            handler->SendSysMessage(LANG_CREATURE_MOVE_DISABLED);
        }
        else
        {
            sWorld->SetAllowMovement(true);
            handler->SendSysMessage(LANG_CREATURE_MOVE_ENABLED);
        }
        return true;
    }

    static bool HandleNpcSetEntryCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_ENTRY);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        uint32 newEntryNum = tokenizer.TryGetParam<uint32>(0, "Hcreature_entry");

        if (!newEntryNum)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_ENTRY);
            return true;
        }

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target || target->GetTypeId() != TYPEID_UNIT)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        if (target->UpdateEntry(newEntryNum))
            handler->SendSysMessage(FREEDOM_CMDI_NPC_SET_ENTRY);
        else
            handler->SendSysMessage(FREEDOM_CMDE_NPC_SET_ENTRY_FAIL);

        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);
        return true;
    }

    //change level of creature or pet
    static bool HandleNpcSetLevelCommand(ChatHandler* handler, uint8 lvl)
    {
        if (lvl < 1 || lvl > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) + 3)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* creature = handler->getSelectedCreature();
        if (!creature || creature->IsPet())
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        creature->SetMaxHealth(100 + 30*lvl);
        creature->SetHealth(100 + 30*lvl);
        creature->SetLevel(lvl);
        creature->SaveToDB();

        return true;
    }

    static bool HandleNpcDeleteCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        if (*args)
        {
            ArgumentTokenizer tokenizer(args);
            guidLow = tokenizer.TryGetParam<uint64>(0, "Hcreature");
            target = nullptr; // remove selected target
        }

        if (!target && guidLow)
            target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        if (target->IsPet() || target->IsTotem())
        {
            handler->SendSysMessage(FREEDOM_CMDE_CREATURE_CANNOT_BE_PET_OR_TOTEM);
            return true;
        }

        sFreedomMgr->CreatureDelete(target);

        handler->SendSysMessage(FREEDOM_CMDI_NPC_DELETE);
        return true;
    }

    //del item from vendor list
    static bool HandleNpcDeleteVendorItemCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(FREEDOM_CMDH_NPC_DELETE_ITEM);
            return true;
        }

        Creature* vendor = handler->getSelectedCreature();
        if (!vendor || !vendor->IsVendor())
        {
            handler->SendSysMessage(FREEDOM_CMDE_CREATURE_SELECT_VENDOR);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        uint32 itemId = tokenizer.TryGetParam<uint32>(0, "Hitem");

        if (!itemId)
        {
            handler->SendSysMessage(FREEDOM_CMDE_INVALID_ITEM_ID);
            return true;
        }

        if (!sObjectMgr->RemoveVendorItem(vendor->GetEntry(), ITEM_VENDOR_TYPE_ITEM, itemId))
        {
            handler->PSendSysMessage(FREEDOM_CMDE_NPC_DELETE_ITEM_NOT_IN_LIST, itemId);
            return true;
        }

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_DELETE_ITEM,
            sFreedomMgr->ToChatLink("Hitem", itemId, itemTemplate->GetDefaultLocaleName()),
            itemId);
        return true;
    }

    //set faction of creature
    static bool HandleNpcSetFactionIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_FACTION_ID);
            return true;
        }

        uint32 factionId = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        factionId = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
            target = nullptr; // remove selected target
        }

        // Only fetch new target if selected target does not exist but we still managed to get guid
        if (!target && guidLow)
            target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        if (!sFactionTemplateStore.LookupEntry(factionId))
        {
            handler->PSendSysMessage(FREEDOM_CMDE_NPC_SET_FACTION_INVALID_ID, factionId);
            return true;
        }

        target->SetFaction(factionId);

        // Faction is set in creature_template - not inside creature

        // Update in memory..
        if (CreatureTemplate const* cinfo = target->GetCreatureTemplate())
            const_cast<CreatureTemplate*>(cinfo)->faction = factionId;

        // ..and DB
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_FACTION);

        stmt->setUInt16(0, uint16(factionId));
        stmt->setUInt32(1, target->GetEntry());

        WorldDatabase.Execute(stmt);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_FACTION_ID);
        return true;
    }

    //set npcflag of creature
  static bool HandleNpcSetFlagCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_FLAG);
            return true;
        }

        uint64 npcFlags = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        npcFlags = tokenizer.TryGetParam<uint64>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        // TODO: Check if bit shifting the 64 bits to get the 2 32 bits numbers is correct/
        // Original trinity command used 2 arguments and bitshifted according to Npcflags = uint64(npcFlags1) | (uint64(npcFlags2) << 32)
        // Could also change command to accept 2 32 bits number instead of 1 64 bits number.
        target->ReplaceAllNpcFlags(NPCFlags(uint32(npcFlags)));
        target->ReplaceAllNpcFlags2(NPCFlags2(uint32(npcFlags >> 32)));

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_NPCFLAG);

        stmt->setUInt64(0, npcFlags);
        stmt->setUInt32(1, target->GetEntry());

        WorldDatabase.Execute(stmt);

        handler->SendSysMessage(FREEDOM_CMDI_NPC_SET_FLAG);

        return true;
    }

    //set data of creature for testing scripting
    static bool HandleNpcSetDataCommand(ChatHandler* handler,char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_DATA);
            return true;
        }

        uint32 field = 0;
        uint32 data = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        field = tokenizer.TryGetParam<uint32>(0);
        data = tokenizer.TryGetParam<uint32>(1);

        if (!field || !data)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_DATA);
            return true;
        }

        if (tokenizer.size() > 2)
        {
            guidLow = tokenizer.TryGetParam<uint64>(2, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        target->AI()->SetData(field, data);
        std::string AIorScript = !target->GetAIName().empty() ? "AI type: " + target->GetAIName() :
            (!target->GetScriptName().empty() ? "Script Name: " + target->GetScriptName() : "No AI or Script Name Set");
        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_DATA,
            sFreedomMgr->ToChatLink("Hcreature", guidLow, target->GetName()),
            field,
            data,
            AIorScript.c_str());
        return true;
    }

    //npc follow handling
    static bool HandleNpcFollowCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        Creature* creature = handler->getSelectedCreature();

        if (!creature)
        {
            handler->PSendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Follow player - Using pet's default dist and angle
        creature->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, creature->GetFollowAngle());

        handler->PSendSysMessage(LANG_CREATURE_FOLLOW_YOU_NOW, creature->GetName().c_str());
        return true;
    }

    static bool HandleNpcInfoCommand(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        CreatureTemplate const* cInfo = target->GetCreatureTemplate();

        uint32 faction = target->GetFaction();
        uint64 npcflags;
        memcpy(&npcflags, target->m_unitData->NpcFlags.begin(), sizeof(npcflags));
        uint32 mechanicImmuneMask = cInfo->MechanicImmuneMask;
        uint32 displayid = target->GetDisplayId();
        uint32 nativeid = target->GetNativeDisplayId();
        uint32 entry = target->GetEntry();

        int64 curRespawnDelay = target->GetRespawnCompatibilityMode() ? target->GetRespawnTimeEx() - GameTime::GetGameTime() : target->GetMap()->GetCreatureRespawnTime(target->GetSpawnId()) - GameTime::GetGameTime();

        if (curRespawnDelay < 0)
            curRespawnDelay = 0;
        std::string curRespawnDelayStr = secsToTimeString(uint64(curRespawnDelay), TimeFormat::ShortText);
        std::string defRespawnDelayStr = secsToTimeString(target->GetRespawnDelay(), TimeFormat::ShortText);

        handler->PSendSysMessage(LANG_NPCINFO_CHAR, target->GetName().c_str(), std::to_string(target->GetSpawnId()).c_str(), target->GetGUID().ToString().c_str(), entry, faction, std::to_string(npcflags).c_str(), displayid, nativeid);
        if (target->GetCreatureData() && target->GetCreatureData()->spawnGroupData->groupId)
        {
            SpawnGroupTemplateData const* const groupData = target->GetCreatureData()->spawnGroupData;
            handler->PSendSysMessage(LANG_SPAWNINFO_GROUP_ID, groupData->name.c_str(), groupData->groupId, groupData->flags, target->GetMap()->IsSpawnGroupActive(groupData->groupId));
        }
        handler->PSendSysMessage(LANG_SPAWNINFO_COMPATIBILITY_MODE, target->GetRespawnCompatibilityMode());
        handler->PSendSysMessage(LANG_NPCINFO_LEVEL, target->GetLevel());
        handler->PSendSysMessage(LANG_NPCINFO_EQUIPMENT, target->GetCurrentEquipmentId(), target->GetOriginalEquipmentId());
        handler->PSendSysMessage(LANG_NPCINFO_HEALTH, target->GetCreateHealth(), std::to_string(target->GetMaxHealth()).c_str(), std::to_string(target->GetHealth()).c_str());
        handler->PSendSysMessage(LANG_NPCINFO_MOVEMENT_DATA, target->GetMovementTemplate().ToString().c_str());

        handler->PSendSysMessage(LANG_NPCINFO_UNIT_FIELD_FLAGS, *target->m_unitData->Flags);
        for (UnitFlags flag : EnumUtils::Iterate<UnitFlags>())
            if (target->HasUnitFlag(flag))
                handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(flag), flag);

        handler->PSendSysMessage(LANG_NPCINFO_UNIT_FIELD_FLAGS_2, *target->m_unitData->Flags2);
        for (UnitFlags2 flag : EnumUtils::Iterate<UnitFlags2>())
            if (target->HasUnitFlag2(flag))
                handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(flag), flag);

        handler->PSendSysMessage(LANG_NPCINFO_UNIT_FIELD_FLAGS_3, *target->m_unitData->Flags3);
        for (UnitFlags3 flag : EnumUtils::Iterate<UnitFlags3>())
            if (target->HasUnitFlag3(flag))
                handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(flag), flag);

        handler->PSendSysMessage(LANG_NPCINFO_DYNAMIC_FLAGS, target->GetDynamicFlags());
        handler->PSendSysMessage(LANG_COMMAND_RAWPAWNTIMES, defRespawnDelayStr.c_str(), curRespawnDelayStr.c_str());
        handler->PSendSysMessage(LANG_NPCINFO_LOOT,  cInfo->lootid, cInfo->pickpocketLootId, cInfo->SkinLootId);
        handler->PSendSysMessage(LANG_NPCINFO_DUNGEON_ID, target->GetInstanceId());

        if (CreatureData const* data = sObjectMgr->GetCreatureData(target->GetSpawnId()))
        {
            handler->PSendSysMessage(LANG_NPCINFO_PHASES, data->phaseId, data->phaseGroup);
            PhasingHandler::PrintToChat(handler, target);
        }

        handler->PSendSysMessage(LANG_NPCINFO_ARMOR, target->GetArmor());
        handler->PSendSysMessage(LANG_NPCINFO_POSITION, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
        handler->PSendSysMessage(LANG_OBJECTINFO_AIINFO, target->GetAIName().c_str(), target->GetScriptName().c_str());
        handler->PSendSysMessage(LANG_NPCINFO_REACTSTATE, DescribeReactState(target->GetReactState()));
        if (CreatureAI const* ai = target->AI())
            handler->PSendSysMessage(LANG_OBJECTINFO_AITYPE, GetTypeName(*ai).c_str());
        handler->PSendSysMessage(LANG_NPCINFO_FLAGS_EXTRA, cInfo->flags_extra);
        for (CreatureFlagsExtra flag : EnumUtils::Iterate<CreatureFlagsExtra>())
            if (cInfo->flags_extra & flag)
                handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(flag), flag);

        handler->PSendSysMessage(LANG_NPCINFO_NPC_FLAGS, target->m_unitData->NpcFlags[0]);
        for (NPCFlags flag : EnumUtils::Iterate<NPCFlags>())
            if (target->HasNpcFlag(flag))
                handler->PSendSysMessage("* %s (0x%X)", EnumUtils::ToTitle(flag), flag);

        for (NPCFlags2 flag : EnumUtils::Iterate<NPCFlags2>())
            if (target->HasNpcFlag2(flag))
                handler->PSendSysMessage("* %s (0x%X)", EnumUtils::ToTitle(flag), flag);

        handler->PSendSysMessage(LANG_NPCINFO_MECHANIC_IMMUNE, mechanicImmuneMask);
        for (Mechanics m : EnumUtils::Iterate<Mechanics>())
            if (m && (mechanicImmuneMask & (1 << (m - 1))))
                handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(m), m);

        return true;
    }

     static bool HandleNpcInfoCommand2(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());
        bool historyInfo = false;
        bool positionInfo = false;
        bool advancedInfo = false;

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-full", 0);
        tokenizer.LoadModifier("-pos", 0);
        tokenizer.LoadModifier("-history", 0);
        tokenizer.LoadModifier("-advanced", 0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
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

        CreatureTemplate const* cInfo = target->GetCreatureTemplate();
        CreatureExtraData const* extraData = sFreedomMgr->GetCreatureExtraData(guidLow);

        float px, py, pz, po;
        source->GetPosition(px, py, pz, po);
        float cx, cy, cz, co;
        target->GetPosition(cx, cy, cz, co);
        float distance = std::sqrt(std::pow(cx - px, 2) + std::pow(cy - py, 2) + std::pow(cz - pz, 2));

        std::string name = target->GetName();
        uint32 faction = target->GetFaction();
        uint64 npcflags;
        memcpy(&npcflags, target->m_unitData->NpcFlags.begin(), sizeof(npcflags));
        uint32 mechanicImmuneMask = cInfo->MechanicImmuneMask;
        uint32 displayId = target->GetDisplayId();
        uint32 nativeId = target->GetNativeDisplayId();
        uint32 entryId = cInfo->Entry;
        //auto phaseList = target->GetPhases(); -- To Update

        int64 curRespawnDelay = target->GetRespawnCompatibilityMode() ? target->GetRespawnTimeEx() - GameTime::GetGameTime() : target->GetMap()->GetCreatureRespawnTime(target->GetSpawnId()) - GameTime::GetGameTime();

        if (curRespawnDelay < 0)
            curRespawnDelay = 0;
        std::string curRespawnDelayStr = secsToTimeString(uint64(curRespawnDelay), TimeFormat::ShortText);
        std::string defRespawnDelayStr = secsToTimeString(target->GetRespawnDelay(), TimeFormat::ShortText);

        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO, sFreedomMgr->ToChatLink("Hcreature", guidLow, name));
        if (advancedInfo)
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_GUID_FULL, target->GetGUID().ToString());
        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_GUID, guidLow);
        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_ENTRY, entryId, sFreedomMgr->ToChatLink("Hcreature_entry", entryId, "Spawn link"));
        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_DISPLAY_ID, displayId, nativeId);
        /*  TO UPDATE
        if (!phaseList.empty())
        {
            handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_PHASELIST);

            for (uint32 phaseId : phaseList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_GAMEOBJECT_INFO_LI_PHASE_LI, phaseId, sFreedomMgr->GetPhaseMask(phaseId));
            }
        }
        */
        if (advancedInfo)
        {
            //handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_PHASEMASK, extraData ? extraData->phaseMask : target->GetPhaseMask());
            if (CreatureData const* data = sObjectMgr->GetCreatureData(target->GetSpawnId()))
            {
                handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_PHASESHIFT, data->phaseId, data->phaseGroup);
                PhasingHandler::PrintToChat(handler, target);
            }
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_FACTION_ID, faction);
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_NAME, name);
            //Removed from the Trinity Command version
            //handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_AI_INFO, target->GetAIName());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_SCRIPT_INFO, target->GetScriptName());

            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_FLAGSE_LIST, cInfo->flags_extra);
            for (CreatureFlagsExtra flag : EnumUtils::Iterate<CreatureFlagsExtra>())
            	if (cInfo->flags_extra & flag)
					handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_FLAGSE_LI, EnumUtils::ToTitle(flag), flag);

            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_NPCFLAGS_LIST, cInfo->npcflag);
            for (NPCFlags flag : EnumUtils::Iterate<NPCFlags>())
                if (target->HasNpcFlag(flag))
                    handler->PSendSysMessage("* %s (0x%X)", EnumUtils::ToTitle(flag), flag);

            for (NPCFlags2 flag : EnumUtils::Iterate<NPCFlags2>())
                if (target->HasNpcFlag2(flag))
                    handler->PSendSysMessage("* %s (0x%X)", EnumUtils::ToTitle(flag), flag);

            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_MECHANICIMM_LIST, mechanicImmuneMask);
            for (Mechanics m : EnumUtils::Iterate<Mechanics>())
                if (m && (mechanicImmuneMask & (1 << (m - 1))))
                    handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_MECHANICIMM_LI, EnumUtils::ToTitle(m), m);

            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_LEVEL, target-> GetLevel());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_EQUIPMENT, target->GetCurrentEquipmentId(), target->GetOriginalEquipmentId());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_HEALTH, target->GetHealth(), target->GetMaxHealth(), target->GetCreateHealth());
            //TODO: Inhabit type was removed. Possibly replaceable by movement_type or something. Not sure if actually useful info.
            //handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_INHABIT_TYPE, cInfo->InhabitType);

            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_UNITFLAGS_LIST, *target->m_unitData->Flags);
            for (UnitFlags flag : EnumUtils::Iterate<UnitFlags>())
                if (target->HasUnitFlag(flag))
                    handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_UNITFLAGS_LI, EnumUtils::ToTitle(flag), flag);

            handler->PSendSysMessage(LANG_NPCINFO_UNIT_FIELD_FLAGS_2, *target->m_unitData->Flags2);
            for (UnitFlags2 flag : EnumUtils::Iterate<UnitFlags2>())
                if (target->HasUnitFlag2(flag))
                    handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(flag), flag);

            handler->PSendSysMessage(LANG_NPCINFO_UNIT_FIELD_FLAGS_3, *target->m_unitData->Flags3);
            for (UnitFlags3 flag : EnumUtils::Iterate<UnitFlags3>())
                if (target->HasUnitFlag3(flag))
                    handler->PSendSysMessage("%s (0x%X)", EnumUtils::ToTitle(flag), flag);

            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_DYNFLAGS, target->GetDynamicFlags());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_FACTION_TEMPLATE, target-> GetFaction());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_RESPAWN_TIMES, defRespawnDelayStr.c_str(), curRespawnDelayStr.c_str());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_LOOT, cInfo->lootid, cInfo->pickpocketLootId, cInfo->SkinLootId);
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_DUNGEON_ID, target->GetInstanceId());
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_ARMOR, target->GetArmor());
        }

        if (positionInfo)
        {
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_POS_X, cx);
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_POS_Y, cy);
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_POS_Z, cz);
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_ORIENTATION, co);
        }

        if (historyInfo)
        {
            if (!extraData)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_NO_HISTORY);
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
                CharacterCacheEntry const* creatorPlayerInfo = sCharacterCache->GetCharacterCacheByGuid(ObjectGuid::Create<HighGuid::Player>(creatorPlayerId));
                if (creatorPlayerInfo)
                    creatorPlayerName = creatorPlayerInfo->Name;
                CharacterCacheEntry const* modifierPlayerInfo = sCharacterCache->GetCharacterCacheByGuid(ObjectGuid::Create<HighGuid::Player>(modifierPlayerId));
                 if (modifierPlayerInfo)
                    modifierPlayerName = modifierPlayerInfo->Name;

                Battlenet::AccountMgr::GetName(creatorBnetId, creatorAccountName);
                Battlenet::AccountMgr::GetName(modifierBnetId, modifierAccountName);

                // Set timestamps
                if (created)
                    createdTimestamp = sFreedomMgr->ToDateTimeString(created);

                if (modified)
                    modifiedTimestamp = sFreedomMgr->ToDateTimeString(modified);

                handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_CREATOR_PLAYER, creatorPlayerName, creatorPlayerId);
                if (handler->HasPermission(rbac::RBAC_FPERM_ADMINISTRATION))
                    handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_CREATOR_ACC, creatorAccountName, creatorBnetId);
                handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_MODIFIER_PLAYER, modifierPlayerName, modifierPlayerId);
                if (handler->HasPermission(rbac::RBAC_FPERM_ADMINISTRATION))
                    handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_MODIFIER_ACC, modifierAccountName, modifierBnetId);
                handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_CREATED, createdTimestamp);
                handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_INFO_LI_MODIFIED, modifiedTimestamp);
            }
        }

        return true;
    }

    static bool HandleNpcNearCommand(ChatHandler* handler, char const* args)
    {
        float distance = (!*args) ? 10.0f : (float)(atof(args));
        uint32 count = 0;

        Player* player = handler->GetSession()->GetPlayer();
        uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NEAREST_CREATURES);
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
                double creatureDist = fields[2].GetDouble();

                CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(entry);

                if (!creatureInfo)
                    continue;

                handler->PSendSysMessage(FREEDOM_CMDI_NPC_NEAR_LIST_ITEM,
                    creatureDist,
                    entry,
                    sFreedomMgr->ToChatLink("Hcreature", guid, creatureInfo->Name));

                ++count;
            } while (result->NextRow());
        }

        if (count == maxResults)
            handler->PSendSysMessage(FREEDOM_CMDI_NPC_NEAR_MAX_RESULT_COUNT, count, distance);
        else
            handler->PSendSysMessage(FREEDOM_CMDI_NPC_NEAR, count, distance);
        return true;
    }


    //move selected creature
    static bool HandleNpcMoveCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 spawnId = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-guid", 1);
        tokenizer.LoadModifier("-adeg", 1);
        tokenizer.LoadModifier("-sdeg", 1);
        tokenizer.LoadModifier("-po", 0);

        if (tokenizer.ModifierExists("-guid"))
        {
            std::string guidValue = tokenizer.GetModifierValue("-guid", 0);
            std::string guidKey = sFreedomMgr->GetChatLinkKey(guidValue, "Hcreature");
            spawnId = atoul(guidKey.c_str());
        }

        target = sFreedomMgr->GetAnyCreature(spawnId);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        float o = target->GetOrientation();

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
            x = add_x*cos(o) - add_y*sin(o) + target->GetPositionX();
            y = add_x*sin(o) + add_y*cos(o) + target->GetPositionY();
            z = add_z + target->GetPositionZ();
        }

        sFreedomMgr->CreatureMove(target, x, y, z, o);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_MOVE,
            sFreedomMgr->ToChatLink("Hcreature", spawnId, target->GetName()),
            spawnId);
        return true;
    }
    //play npc emote
    static bool HandleNpcPlayEmoteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_PLAYEMOTE);
            return true;
        }

        uint32 emoteId = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        emoteId = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        target->SetEmoteState(Emote(emoteId));

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_PLAYEMOTE);
        return true;
    }

    //set model of creature
    static bool HandleNpcSetModelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_MODEL);
            return true;
        }

        uint32 displayId = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        displayId = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        target->SetDisplayId(displayId);
        target->SetNativeDisplayId(displayId);

        target->SaveToDB();

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_MODEL);
        return true;
    }

    /**HandleNpcSetMoveTypeCommand
    * Set the movement type for an NPC.<br/>
    * <br/>
    * Valid movement types are:
    * <ul>
    * <li> stay - NPC wont move </li>
    * <li> random - NPC will move randomly according to the wander_distance </li>
    * <li> way - NPC will move with given waypoints set </li>
    * </ul>
    * additional parameter: NODEL - so no waypoints are deleted, if you
    *                       change the movement type
    */
    static bool HandleNpcSetMoveTypeCommand(ChatHandler* handler, Optional<CreatureSpawnId> lowGuid, Variant<EXACT_SEQUENCE("stay"), EXACT_SEQUENCE("random"), EXACT_SEQUENCE("way")> type, Optional<EXACT_SEQUENCE("nodel")> nodel)
    {
        // 3 arguments:
        // GUID (optional - you can also select the creature)
        // stay|random|way (determines the kind of movement)
        // NODEL (optional - tells the system NOT to delete any waypoints)
        //        this is very handy if you want to do waypoints, that are
        //        later switched on/off according to special events (like escort
        //        quests, etc)

        bool doNotDelete = nodel.has_value();

        ObjectGuid::LowType lowguid = UI64LIT(0);
        Creature* creature = nullptr;

        if (!lowGuid)                                           // case .setmovetype $move_type (with selected creature)
        {
            creature = handler->getSelectedCreature();
            if (!creature || creature->IsPet())
                return false;
            lowguid = creature->GetSpawnId();
        }
        else                                                    // case .setmovetype #creature_guid $move_type (with selected creature)
        {
            lowguid = *lowGuid;

            if (lowguid)
                creature = handler->GetCreatureFromPlayerMapByDbGuid(lowguid);

            // attempt check creature existence by DB data
            if (!creature)
            {
                CreatureData const* data = sObjectMgr->GetCreatureData(lowguid);
                if (!data)
                {
                    handler->PSendSysMessage(LANG_COMMAND_CREATGUIDNOTFOUND, std::to_string(lowguid).c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }
            }
            else
            {
                lowguid = creature->GetSpawnId();
            }
        }

        // now lowguid is low guid really existed creature
        // and creature point (maybe) to this creature or nullptr

        MovementGeneratorType move_type;
        switch (type.index())
        {
            case 0:
                move_type = IDLE_MOTION_TYPE;
                break;
            case 1:
                move_type = RANDOM_MOTION_TYPE;
                break;
            case 2:
                move_type = WAYPOINT_MOTION_TYPE;
                break;
            default:
                return false;
        }

        // update movement type
        //if (doNotDelete == false)
        //    WaypointMgr.DeletePath(lowguid);

        if (creature)
        {
            // update movement type
            if (doNotDelete == false)
                creature->LoadPath(0);

            creature->SetDefaultMovementType(move_type);
            creature->GetMotionMaster()->Initialize();
            if (creature->IsAlive())                            // dead creature will reset movement generator at respawn
            {
                creature->setDeathState(JUST_DIED);
                creature->Respawn();
            }
            creature->SaveToDB();
        }
        if (doNotDelete == false)
        {
            handler->PSendSysMessage(LANG_MOVE_TYPE_SET, EnumUtils::ToTitle(move_type));
        }
        else
        {
            handler->PSendSysMessage(LANG_MOVE_TYPE_SET_NODEL, EnumUtils::ToTitle(move_type));
        }

        return true;
    }

    //npc phase handling
    //change phase of creature
    static bool HandleNpcSetPhaseGroup(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        int32 phaseGroupId = atoi(args);

        Creature* creature = handler->getSelectedCreature();
        if (!creature || creature->IsPet())
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PhasingHandler::ResetPhaseShift(creature);
        PhasingHandler::AddPhaseGroup(creature, phaseGroupId, true);
        creature->SetDBPhase(-phaseGroupId);

        creature->SaveToDB();

        return true;
    }

    //npc phase handling
    //change phase of creature
    static bool HandleNpcSetPhaseCommand(ChatHandler* handler, uint32 phaseID)
    {
        if (phaseID == 0)
        {
            handler->SendSysMessage(LANG_PHASE_NOTFOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* creature = handler->getSelectedCreature();
        if (!creature || creature->IsPet())
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PhasingHandler::ResetPhaseShift(creature);
        PhasingHandler::AddPhase(creature, phaseID, true);
        creature->SetDBPhase(phaseID);

        creature->SaveToDB();

        return true;
    }

    //set spawn dist of creature
    static bool HandleNpcSetWanderDistanceCommand(ChatHandler* handler, float option)
    {
        if (option < 0.0f)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            return false;
        }

        MovementGeneratorType mtype = IDLE_MOTION_TYPE;
        if (option > 0.0f)
            mtype = RANDOM_MOTION_TYPE;

        Creature* creature = handler->getSelectedCreature();
        ObjectGuid::LowType guidLow = UI64LIT(0);

        if (creature)
            guidLow = creature->GetSpawnId();
        else
            return false;

        creature->SetWanderDistance((float)option);
        creature->SetDefaultMovementType(mtype);
        creature->GetMotionMaster()->Initialize();
        if (creature->IsAlive())                                // dead creature will reset movement generator at respawn
        {
            creature->setDeathState(JUST_DIED);
            creature->Respawn();
        }

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_WANDER_DISTANCE);

        stmt->setFloat(0, option);
        stmt->setUInt8(1, uint8(mtype));
        stmt->setUInt64(2, guidLow);

        WorldDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_COMMAND_WANDER_DISTANCE, option);
        return true;
    }

    //spawn time handling
    static bool HandleNpcSetSpawnTimeCommand(ChatHandler* handler, uint32 spawnTime)
    {
        Creature* creature = handler->getSelectedCreature();
        if (!creature)
            return false;

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_SPAWN_TIME_SECS);
        stmt->setUInt32(0, spawnTime);
        stmt->setUInt64(1, creature->GetSpawnId());
        WorldDatabase.Execute(stmt);

        creature->SetRespawnDelay(spawnTime);
        handler->PSendSysMessage(LANG_COMMAND_SPAWNTIME, spawnTime);

        return true;
    }

    static bool HandleNpcSayCommand(ChatHandler* handler, Tail text)
    {
        if (text.empty())
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SAY);
            return true;
        }

        Creature* creature = handler->getSelectedCreature();
        if (!creature)
        {
            handler->SendSysMessage(FREEDOM_CMDE_MANUAL_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        creature->Say(text, LANG_UNIVERSAL);

        // make some emotes
        switch (text.back())
        {
            case '?':   creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);      break;
            case '!':   creature->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);   break;
            default:    creature->HandleEmoteCommand(EMOTE_ONESHOT_TALK);          break;
        }

        return true;
    }

    //show text emote by creature in chat
    static bool HandleNpcTextEmoteCommand(ChatHandler* handler, Tail text)
    {
        if (text.empty())
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_TEXTEMOTE);
            return true;
        }

        Creature* creature = handler->getSelectedCreature();

        if (!creature)
        {
            handler->SendSysMessage(FREEDOM_CMDE_MANUAL_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        creature->TextEmote(text);

        return true;
    }

    // npc unfollow handling
    static bool HandleNpcUnFollowCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        Creature* creature = handler->getSelectedCreature();

        if (!creature)
        {
            handler->PSendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        MovementGenerator* movement = creature->GetMotionMaster()->GetMovementGenerator([player](MovementGenerator const* a) -> bool
        {
            if (a->GetMovementGeneratorType() == FOLLOW_MOTION_TYPE)
            {
                FollowMovementGenerator const* followMovement = dynamic_cast<FollowMovementGenerator const*>(a);
                return followMovement && followMovement->GetTarget() == player;
            }
            return false;
        });

        if (!movement)
        {
            handler->PSendSysMessage(LANG_CREATURE_NOT_FOLLOW_YOU, creature->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        creature->GetMotionMaster()->Remove(movement);
        handler->PSendSysMessage(LANG_CREATURE_NOT_FOLLOW_YOU_NOW, creature->GetName().c_str());
        return true;
    }

    // make npc whisper to player
    static bool HandleNpcWhisperCommand(ChatHandler* handler, Variant<Hyperlink<player>, std::string_view> recv, Tail text)
    {
        if (text.empty())
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_WHISPER);
            return true;
        }

        Creature* creature = handler->getSelectedCreature();
        if (!creature)
        {
            handler->SendSysMessage(FREEDOM_CMDE_MANUAL_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* receiver = ObjectAccessor::FindPlayerByName(recv);
        creature->Whisper(text, LANG_UNIVERSAL, receiver);
        return true;
    }

    static bool HandleNpcYellCommand(ChatHandler* handler, Tail text)
    {
        if (text.empty())
        {
            handler->SendSysMessage(FREEDOM_CMDH_NPC_YELL);
            return true;
        }

        Creature* creature = handler->getSelectedCreature();
        if (!creature)
        {
            handler->SendSysMessage(FREEDOM_CMDE_MANUAL_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        creature->Yell(text, LANG_UNIVERSAL);

        // make an emote
        creature->HandleEmoteCommand(EMOTE_ONESHOT_SHOUT);

        return true;
    }

    // add creature, temp only
    static bool HandleNpcAddTempSpawnCommand(ChatHandler* handler, Optional<std::string_view> lootStr, CreatureEntry id)
    {
        bool loot = false;
        if (lootStr)
        {
            if (StringEqualI(*lootStr, "loot"))
                loot = true;
            else if (StringEqualI(*lootStr, "noloot"))
                loot = false;
            else
                return false;
        }

        Player* chr = handler->GetSession()->GetPlayer();
        if (!sObjectMgr->GetCreatureTemplate(id))
            return false;

        chr->SummonCreature(id, chr->GetPosition(), loot ? TEMPSUMMON_CORPSE_TIMED_DESPAWN : TEMPSUMMON_CORPSE_DESPAWN, 30s);

        return true;
    }

    //npc tame handling
    static bool HandleNpcTameCommand(ChatHandler* handler)
    {
        Creature* creatureTarget = handler->getSelectedCreature();
        if (!creatureTarget || creatureTarget->IsPet())
        {
            handler->PSendSysMessage (LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage (true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        if (!player->GetPetGUID().IsEmpty())
        {
            handler->SendSysMessage (LANG_YOU_ALREADY_HAVE_PET);
            handler->SetSentErrorMessage (true);
            return false;
        }

        CreatureTemplate const* cInfo = creatureTarget->GetCreatureTemplate();

        if (!cInfo->IsTameable (player->CanTameExoticPets()))
        {
            handler->PSendSysMessage (LANG_CREATURE_NON_TAMEABLE, cInfo->Entry);
            handler->SetSentErrorMessage (true);
            return false;
        }

        // Everything looks OK, create new pet
        Pet* pet = player->CreateTamedPetFrom(creatureTarget);
        if (!pet)
        {
            handler->PSendSysMessage (LANG_CREATURE_NON_TAMEABLE, cInfo->Entry);
            handler->SetSentErrorMessage (true);
            return false;
        }

        // place pet before player
        float x, y, z;
        player->GetClosePoint (x, y, z, creatureTarget->GetCombatReach(), CONTACT_DISTANCE);
        pet->Relocate(x, y, z, float(M_PI) - player->GetOrientation());

        // set pet to defensive mode by default (some classes can't control controlled pets in fact).
        pet->SetReactState(REACT_DEFENSIVE);

        // calculate proper level
        uint8 level = std::max<uint8>(player->GetLevel()-5, creatureTarget->GetLevel());

        // prepare visual effect for levelup
        pet->SetLevel(level - 1);

        // add to world
        pet->GetMap()->AddToMap(pet->ToCreature());

        // visual effect for levelup
        pet->SetLevel(level);

        // caster have pet now
        player->SetMinion(pet, true);

        pet->SavePetToDB(PET_SAVE_AS_CURRENT);
        player->PetSpellInitialize();

        return true;
    }

    static bool HandleNpcEvadeCommand(ChatHandler* handler, Optional<CreatureAI::EvadeReason> why, Optional<EXACT_SEQUENCE("force")> force)
    {
        Creature* creatureTarget = handler->getSelectedCreature();
        if (!creatureTarget || creatureTarget->IsPet())
        {
            handler->PSendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!creatureTarget->IsAIEnabled())
        {
            handler->PSendSysMessage(LANG_CREATURE_NOT_AI_ENABLED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (force)
            creatureTarget->ClearUnitState(UNIT_STATE_EVADE);
        creatureTarget->AI()->EnterEvadeMode(why.value_or(CreatureAI::EVADE_REASON_OTHER));

        return true;
    }

    static void _ShowLootEntry(ChatHandler* handler, uint32 itemId, uint8 itemCount, bool alternateString = false)
    {
        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
        char const* name = nullptr;
        if (itemTemplate)
            name = itemTemplate->GetName(handler->GetSessionDbcLocale());
        if (!name)
            name = "Unknown item";
        handler->PSendSysMessage(alternateString ? LANG_COMMAND_NPC_SHOWLOOT_ENTRY_2 : LANG_COMMAND_NPC_SHOWLOOT_ENTRY,
            itemCount, ItemQualityColors[itemTemplate ? static_cast<ItemQualities>(itemTemplate->GetQuality()) : ITEM_QUALITY_POOR], itemId, name, itemId);
    }

    static void _IterateNotNormalLootMap(ChatHandler* handler, NotNormalLootItemMap const& map, std::vector<LootItem> const& items)
    {
        for (NotNormalLootItemMap::value_type const& pair : map)
        {
            if (!pair.second)
                continue;
            Player const* player = ObjectAccessor::FindConnectedPlayer(pair.first);
            handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_SUBLABEL, player ? player->GetName().c_str() : Trinity::StringFormat("Offline player (GUID %s)", pair.first.ToString().c_str()).c_str(), pair.second->size());

            for (auto it = pair.second->cbegin(); it != pair.second->cend(); ++it)
            {
                LootItem const& item = items[it->index];
                if (!(it->is_looted) && !item.is_looted)
                    _ShowLootEntry(handler, item.itemid, item.count, true);
            }
        }
    }
    static bool HandleNpcShowLootCommand(ChatHandler* handler, Optional<EXACT_SEQUENCE("all")> all)
    {
        Creature* creatureTarget = handler->getSelectedCreature();
        if (!creatureTarget || creatureTarget->IsPet())
        {
            handler->PSendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Loot const& loot = creatureTarget->loot;
        if (!creatureTarget->isDead() || loot.empty())
        {
            handler->PSendSysMessage(LANG_COMMAND_NOT_DEAD_OR_NO_LOOT, creatureTarget->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_HEADER, creatureTarget->GetName().c_str(), creatureTarget->GetEntry());
        handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_MONEY, loot.gold / GOLD, (loot.gold%GOLD) / SILVER, loot.gold%SILVER);

        if (!all)
        {
            handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_LABEL, "Standard items", loot.items.size());
            for (LootItem const& item : loot.items)
                if (!item.is_looted)
                    _ShowLootEntry(handler, item.itemid, item.count);

            handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_LABEL, "Quest items", loot.quest_items.size());
            for (LootItem const& item : loot.quest_items)
                if (!item.is_looted)
                    _ShowLootEntry(handler, item.itemid, item.count);
        }
        else
        {
            handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_LABEL, "Standard items", loot.items.size());
            for (LootItem const& item : loot.items)
                if (!item.is_looted && !item.freeforall && item.conditions.empty())
                    _ShowLootEntry(handler, item.itemid, item.count);

            if (!loot.GetPlayerQuestItems().empty())
            {
                handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_LABEL_2, "Per-player quest items");
                _IterateNotNormalLootMap(handler, loot.GetPlayerQuestItems(), loot.quest_items);
            }

            if (!loot.GetPlayerFFAItems().empty())
            {
                handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_LABEL_2, "FFA items per allowed player");
                _IterateNotNormalLootMap(handler, loot.GetPlayerFFAItems(), loot.items);
            }

            if (!loot.GetPlayerNonQuestNonFFAConditionalItems().empty())
            {
                handler->PSendSysMessage(LANG_COMMAND_NPC_SHOWLOOT_LABEL_2, "Per-player conditional items");
                _IterateNotNormalLootMap(handler, loot.GetPlayerNonQuestNonFFAConditionalItems(), loot.items);
            }
        }

        return true;
    }

    static bool HandleNpcAddFormationCommand(ChatHandler* handler, ObjectGuid::LowType leaderGUID)
    {
        Creature* creature = handler->getSelectedCreature();

        if (!creature || !creature->GetSpawnId())
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ObjectGuid::LowType lowguid = creature->GetSpawnId();
        if (creature->GetFormation())
        {
            handler->PSendSysMessage("Selected creature is already member of group " UI64FMTD, creature->GetFormation()->GetLeaderSpawnId());
            return false;
        }

        if (!lowguid)
            return false;

        Player* chr = handler->GetSession()->GetPlayer();

        float  followAngle = (creature->GetAbsoluteAngle(chr) - chr->GetOrientation()) * 180.0f / float(M_PI);
        float  followDist  = std::sqrt(std::pow(chr->GetPositionX() - creature->GetPositionX(), 2.f) + std::pow(chr->GetPositionY() - creature->GetPositionY(), 2.f));
        uint32 groupAI     = 0;
        sFormationMgr->AddFormationMember(lowguid, followAngle, followDist, leaderGUID, groupAI);
        creature->SearchFormation();

        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_CREATURE_FORMATION);
        stmt->setUInt64(0, leaderGUID);
        stmt->setUInt64(1, lowguid);
        stmt->setFloat (2, followAngle);
        stmt->setFloat (3, followDist);
        stmt->setUInt32(4, groupAI);

        WorldDatabase.Execute(stmt);

        handler->PSendSysMessage("Creature " UI64FMTD " added to formation with leader " UI64FMTD, lowguid, leaderGUID);

        return true;
    }

    static bool HandleNpcSetLinkCommand(ChatHandler* handler, ObjectGuid::LowType linkguid)
    {
        Creature* creature = handler->getSelectedCreature();

        if (!creature)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!creature->GetSpawnId())
        {
            handler->PSendSysMessage("Selected %s isn't in creature table", creature->GetGUID().ToString().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!sObjectMgr->SetCreatureLinkedRespawn(creature->GetSpawnId(), linkguid))
        {
            handler->PSendSysMessage("Selected creature can't link with guid '" UI64FMTD "'", linkguid);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("LinkGUID '" UI64FMTD "' added to creature with DBTableGUID: '" UI64FMTD "'", linkguid, creature->GetSpawnId());
        return true;
    }

    /// @todo NpcCommands that need to be fixed :
    static bool HandleNpcAddWeaponCommand([[maybe_unused]] ChatHandler* handler, [[maybe_unused]] uint32 SlotID, [[maybe_unused]] ItemTemplate const* tmpItem)
    {
        /*
        if (!tmpItem)
            return;

        uint64 guid = handler->GetSession()->GetPlayer()->GetSelection();
        if (guid == 0)
        {
            handler->SendSysMessage(LANG_NO_SELECTION);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* creature = ObjectAccessor::GetCreature(*handler->GetSession()->GetPlayer(), guid);

        if (!creature)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        switch (SlotID)
        {
            case 1:
                creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY, tmpItem->ItemId);
                break;
            case 2:
                creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY_01, tmpItem->ItemId);
                break;
            case 3:
                creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY_02, tmpItem->ItemId);
                break;
            default:
                handler->PSendSysMessage(LANG_ITEM_SLOT_NOT_EXIST, SlotID);
                handler->SetSentErrorMessage(true);
                return false;
        }

        handler->PSendSysMessage(LANG_ITEM_ADDED_TO_SLOT, tmpItem->ItemID, tmpItem->Name1, SlotID);
        */
        return true;
    }

    //
    static bool HandleNpcSetScaleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SCALE);
            return true;
        }

        float scale = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        scale = tokenizer.TryGetParam<float>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        float maxScale = sConfigMgr->GetFloatDefault("Freedom.Creature.MaxScale", 15.0f);
        float minScale = sConfigMgr->GetFloatDefault("Freedom.Creature.MinScale", 0.0f);

        if (scale < minScale)
            scale = minScale;
        if (scale > maxScale)
            scale = maxScale;

        sFreedomMgr->CreatureScale(target, scale);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SCALE, scale);
        return true;
    }

    static bool HandleNpcSetEmoteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_EMOTE);
            return true;
        }

        uint32 emoteId = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        emoteId = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetEmote(target, emoteId);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_EMOTE);
        return true;
    }

    static bool HandleNpcSetMountCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_MOUNT);
            return true;
        }

        uint32 mountId = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        mountId = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetMount(target, mountId);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_MOUNT);
        return true;
    }

    static bool HandleNpcSetAuraCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_AURA);
            return true;
        }

        uint32 auraId = 0;
        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        auraId = tokenizer.TryGetParam<uint32>(0);
        toggleToken = tokenizer.TryGetParam(1);

        if (tokenizer.size() > 2)
        {
            guidLow = tokenizer.TryGetParam<uint64>(2, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_AURA);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetAuraToggle(target, auraId, toggleToken == "on");
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_AURA);
        return true;
    }

    static bool HandleNpcSetDeathStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_DEATHSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_DEATHSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_DEAD : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_DEATHSTATE);
        return true;
    }

    static bool HandleNpcSetSleepStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SLEEPSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SLEEPSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_SLEEP : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SLEEPTSTATE);
        return true;
    }

    static bool HandleNpcSetHoverStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_HOVERSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_HOVERSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetBytes1(target, 0);
        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), toggle);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_HOVERSTATE);
        return true;
    }

    static bool HandleNpcSetBytes1Command(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_BYTES1);
            return true;
        }

        uint32 bytes1 = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        bytes1 = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetBytes1(target, bytes1);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_BYTES1);
        return true;
    }

    static bool HandleNpcSetBytes2Command(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_BYTES2);
            return true;
        }

        uint32 bytes2 = 0;
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        bytes2 = tokenizer.TryGetParam<uint32>(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetBytes2(target, bytes2);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_BYTES2);
        return true;
    }

    static bool HandleNpcSetSitGroundStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITGROUNDSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITGROUNDSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_SIT : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SITGROUNDSTATE);
        return true;
    }

    static bool HandleNpcSetSitLowStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITLOWSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITLOWSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_SIT_LOW_CHAIR : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SITLOWSTATE);
        return true;
    }

        static bool HandleNpcSetSitMediumStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITMEDIUMSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITMEDIUMSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_SIT_MEDIUM_CHAIR : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SITMEDIUMSTATE);
        return true;
    }

        static bool HandleNpcSetSitHighStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITHIGHSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SITHIGHSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_SIT_HIGH_CHAIR : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SITHIGHSTATE);
        return true;
    }

        static bool HandleNpcSetKneelStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_KNEELSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_KNEELSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        bool toggle = (toggleToken == "on");

        sFreedomMgr->CreatureSetAuraToggle(target, uint32(SPELL_PERMANENT_HOVER), false);
        sFreedomMgr->CreatureSetBytes1(target, toggle ? UNIT_STAND_STATE_KNEEL : 0);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_KNEELSTATE);
        return true;
    }

      static bool HandleNpcSetSheathStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SHEATHSTATE);
            return true;
        }

        std::string sheathToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        sheathToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (sheathToken != "0" && sheathToken != "1" && sheathToken != "2")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SHEATHSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        SheathState state = SHEATH_STATE_UNARMED;

        if (sheathToken == "0")
            state = SHEATH_STATE_UNARMED;
        if (sheathToken == "1")
            state = SHEATH_STATE_MELEE;
        if (sheathToken == "2")
            state = SHEATH_STATE_RANGED;

        sFreedomMgr->CreatureSetBytes2(target, state);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SHEATHSTATE);
        return true;
    }

     static bool HandleNpcSetGravityCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_GRAVITY);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_GRAVITY);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetGravity(target, toggleToken == "on");
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_GRAVITY);
        return true;
    }

    static bool HandleNpcSetSwimCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SWIM);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_SWIM);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetSwim(target, toggleToken == "on");
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_SWIM);
        return true;
    }


    static bool HandleNpcSetFlyStateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_FLYSTATE);
            return true;
        }

        std::string toggleToken = "";
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(args);
        toggleToken = tokenizer.TryGetParam(0);

        if (tokenizer.size() > 1)
        {
            guidLow = tokenizer.TryGetParam<uint64>(1, "Hcreature");
        }

        if (toggleToken != "on" && toggleToken != "off")
        {
            handler->PSendSysMessage(FREEDOM_CMDH_NPC_SET_FLYSTATE);
            return true;
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        sFreedomMgr->CreatureSetFly(target, toggleToken == "on");
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_NPC_SET_FLYSTATE);
        return true;
    }

    static bool HandleNpcTurnCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        Creature* target = handler->getSelectedCreature();
        uint64 guidLow = target ? target->GetSpawnId() : sFreedomMgr->GetSelectedCreatureGuidFromPlayer(source->GetGUID().GetCounter());

        ArgumentTokenizer tokenizer(*args ? args : "");
        tokenizer.LoadModifier("-guid", 1);
        tokenizer.LoadModifier("-adeg", 1);
        tokenizer.LoadModifier("-sdeg", 1);

        if (tokenizer.ModifierExists("-guid"))
        {
            std::string guidValue = tokenizer.GetModifierValue("-guid", 0);
            std::string guidKey = sFreedomMgr->GetChatLinkKey(guidValue, "Hcreature");
            guidLow = atoul(guidKey.c_str());
        }

        target = sFreedomMgr->GetAnyCreature(guidLow);

        if (!target || target->GetMapId() != source->GetMapId())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }

        float o = source->GetOrientation();

        if (tokenizer.ModifierExists("-adeg"))
        {
            std::string addDeg = tokenizer.GetModifierValue("-adeg", 0);
            o = ((float)atof(addDeg.c_str())) * M_PI / 180.0f + target->GetOrientation();
        }

        if (tokenizer.ModifierExists("-sdeg"))
        {
            std::string setDeg = tokenizer.GetModifierValue("-sdeg", 0);
            o = ((float)atof(setDeg.c_str())) * M_PI / 180.0f;
        }

        float x, y, z;
        target->GetPosition(x, y, z);

        sFreedomMgr->CreatureTurn(target, o);
        sFreedomMgr->CreatureSetModifyHistory(target, source);
        sFreedomMgr->SaveCreature(target);

        handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_TURNED,
            sFreedomMgr->ToChatLink("Hcreature", guidLow, target->GetName()),
            guidLow);

        return true;
    }

    static bool HandleNpcSelectCommand(ChatHandler* handler, char const* args)
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
            ArgumentTokenizer tokenizer(args);
            entryId = tokenizer.TryGetParam<uint32>(0, "Hcreature_entry");
        }

        if (entryId)
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NEAREST_CREATURE_BY_EID);
            stmt->setFloat(index++, x);
            stmt->setFloat(index++, y);
            stmt->setFloat(index++, z);
            stmt->setUInt16(index++, mapId);
            stmt->setUInt32(index++, entryId);

            result = WorldDatabase.Query(stmt);
        }
        else
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NEAREST_CREATURE);
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
            entryId = fields[1].GetUInt32();
            double creatureDist = fields[2].GetDouble();

            CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(entryId);

            if (!creatureTemplate)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_SELECT_ENTRY_ID, entryId, guidLow);
                return true;
            }

            Creature* creature = sFreedomMgr->GetAnyCreature(source->GetMap(), guidLow, entryId);

            if (!creature)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_SELECT_NOT_IN_WORLD, guidLow, entryId);
                return true;
            }

            sFreedomMgr->SetCreatureSelectionForPlayer(source->GetGUID().GetCounter(), guidLow);
            handler->PSendSysMessage(FREEDOM_CMDI_CREATURE_SELECT,
                sFreedomMgr->ToChatLink("Hcreature", guidLow, creatureTemplate->Name),
                guidLow,
                entryId,
                creatureDist);

            return true;
        }
        else
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CREATURE_NOT_FOUND);
            return true;
        }
    }

    static bool HandleNpcReturnCommand(ChatHandler* handler, const char* /*args*/)
    {
        Creature* pCreature = handler->getSelectedCreature();
        if (!pCreature)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        pCreature->AI()->EnterEvadeMode();
        return true;
    }
};

void AddSC_npc_commandscript()
{
    new npc_commandscript();
}

bool HandleNpcSpawnGroup(ChatHandler* handler, std::vector<Variant<uint32, EXACT_SEQUENCE("force"), EXACT_SEQUENCE("ignorerespawn")>> const& opts)
{
    if (opts.empty())
        return false;

    bool ignoreRespawn = false;
    bool force = false;
    uint32 groupId = 0;

    // Decode arguments
    for (auto const& variant : opts)
    {
        switch (variant.index())
        {
            case 0:
                groupId = variant.get<uint32>();
                break;
            case 1:
                force = true;
                break;
            case 2:
                ignoreRespawn = true;
                break;
        }
    }

    Player* player = handler->GetSession()->GetPlayer();

    std::vector <WorldObject*> creatureList;
    if (!player->GetMap()->SpawnGroupSpawn(groupId, ignoreRespawn, force, &creatureList))
    {
        handler->PSendSysMessage(LANG_SPAWNGROUP_BADGROUP, groupId);
        handler->SetSentErrorMessage(true);
        return false;
    }

    handler->PSendSysMessage(LANG_SPAWNGROUP_SPAWNCOUNT, creatureList.size());

    return true;
}

bool HandleNpcDespawnGroup(ChatHandler* handler, std::vector<Variant<uint32, EXACT_SEQUENCE("removerespawntime")>> const& opts)
{
    if (opts.empty())
        return false;

    bool deleteRespawnTimes = false;
    uint32 groupId = 0;

    // Decode arguments
    for (auto const& variant : opts)
    {
        if (variant.holds_alternative<uint32>())
            groupId = variant.get<uint32>();
        else
            deleteRespawnTimes = true;
    }

    Player* player = handler->GetSession()->GetPlayer();

    size_t n = 0;
    if (!player->GetMap()->SpawnGroupDespawn(groupId, deleteRespawnTimes, &n))
    {
        handler->PSendSysMessage(LANG_SPAWNGROUP_BADGROUP, groupId);
        handler->SetSentErrorMessage(true);
        return false;
    }
    handler->PSendSysMessage("Despawned a total of %zu objects.", n);

    return true;
}
