#include "AccountMgr.h"
#include "BattlenetAccountMgr.h"
#include "BigNumber.h"
#include "ChatCommand.h"
#include "CharacterPackets.h"
#include "CharacterCache.h"
#include "Config.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "FreedomMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Language.h"
#include "MovementPackets.h"
#include "MoveSpline.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <boost/algorithm/string/predicate.hpp>
#include "Utilities/ArgumentTokenizer.h"
#include "CollectionMgr.h"
#include "BattlePetMgr.h"
#include "SpellAuras.h"

enum FreedomCmdAuraSpells
{
    SPELL_PERMANENT_FEIGN_DEATH = 114371,
    SPELL_PERMANENT_SLEEP_VISUAL = 107674,
    SPELL_PERMANENT_HOVER = 138092
};

class F_freedom_commandscript : public CommandScript
{
public:
    F_freedom_commandscript() : CommandScript("F_freedom_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> freedomPhaseCommandTable =
        {
            { "lock",           rbac::RBAC_FPERM_COMMAND_PHASE_LOCK,                false, &HandleFreedomPhaseLockCommand,         "" },
            { "clear",          rbac::RBAC_FPERM_COMMAND_PHASE_CLEAR,               false, &HandleFreedomPhaseClearCommand,        "" },
            { "",               rbac::RBAC_FPERM_COMMAND_PHASE,                     false, &HandleFreedomPhaseCommand,             "" },
        };

        static std::vector<ChatCommand> freedomMorphCommandTable =
        {
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_MORPH,             false, &HandleFreedomMorphListCommand,          "" },
            { "add",            rbac::RBAC_FPERM_COMMAND_FREEDOM_MORPH_MODIFY,      false, &HandleFreedomMorphAddCommand,           "" },
            { "delete",         rbac::RBAC_FPERM_COMMAND_FREEDOM_MORPH_MODIFY,      false, &HandleFreedomMorphDelCommand,           "" },
            { "",               rbac::RBAC_FPERM_COMMAND_FREEDOM_MORPH,             false, &HandleFreedomMorphCommand,              "" },
        };

        static std::vector<ChatCommand> freedomMountCommandTable =
        {
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_MOUNT,             false, &HandleFreedomMountListCommand,          "" },
            { "add",            rbac::RBAC_FPERM_COMMAND_FREEDOM_MOUNT_MODIFY,      false, &HandleFreedomMountAddCommand,           "" },
            { "delete",         rbac::RBAC_FPERM_COMMAND_FREEDOM_MOUNT_MODIFY,      false, &HandleFreedomMountDelCommand,           "" },
            { "",               rbac::RBAC_FPERM_COMMAND_FREEDOM_MOUNT,             false, &HandleFreedomMountCommand,              "" },
        };

        static std::vector<ChatCommand> freedomTeleportCommandTable =
        {
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_TELE,              false, &HandleFreedomTeleListCommand,           "" },
            { "add",            rbac::RBAC_FPERM_COMMAND_FREEDOM_TELE_MODIFY,       false, &HandleFreedomTeleAddCommand,            "" },
            { "delete",         rbac::RBAC_FPERM_COMMAND_FREEDOM_TELE_MODIFY,       false, &HandleFreedomTeleDelCommand,            "" },
            { "",               rbac::RBAC_FPERM_COMMAND_FREEDOM_TELE,              false, &HandleFreedomTeleCommand,               "" },
        };

        static std::vector<ChatCommand> freedomPrivateTeleportCommandTable =
        {
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_PTELE,             false, &HandleFreedomPrivateTeleListCommand,    "" },
            { "add",            rbac::RBAC_FPERM_COMMAND_FREEDOM_PTELE,             false, &HandleFreedomPrivateTeleAddCommand,     "" },
            { "delete",         rbac::RBAC_FPERM_COMMAND_FREEDOM_PTELE,             false, &HandleFreedomPrivateTeleDelCommand,     "" },
            { "",               rbac::RBAC_FPERM_COMMAND_FREEDOM_PTELE,             false, &HandleFreedomPrivateTeleCommand,        "" },
        };

        static std::vector<ChatCommand> freedomSpellCommandTable =
        {
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL,             false, &HandleFreedomSpellListCommand,          "" },
            { "add",            rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL_MODIFY,      false, &HandleFreedomSpellAddCommand,           "" },
            { "delete",         rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL_MODIFY,      false, &HandleFreedomSpellDelCommand,           "" },
            { "",               rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL,             false, &HandleFreedomSpellCommand,              "" },
        };

        static std::vector<ChatCommand> freedomReloadCreatureCommandTable =
        {
            { "extra",          rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadCreatureExtraCommand,   "" },
            { "addon",          rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadCreatureAddonCommand,   "" },
            { "base",           rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadCreatureBaseCommand,    "" },
            { "equip",          rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadCreatureEquipCommand,   "" },
        };

        static std::vector<ChatCommand> freedomReloadGameobjectCommandTable =
        {
            { "extra",          rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadGameobjectExtraCommand,   "" },
            { "base",           rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadGameobjectBaseCommand,   "" },
        };

        static std::vector<ChatCommand> freedomReloadCommandTable =
        {
            { "creature",       rbac::RBAC_FPERM_ADMINISTRATION,                    false, NULL,   "", freedomReloadCreatureCommandTable },
            { "gameobject",     rbac::RBAC_FPERM_ADMINISTRATION,                    false, NULL,   "", freedomReloadGameobjectCommandTable },
            { "public_tele",    rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadPublicTeleCommand,   "" },
            { "private_tele",   rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadPrivateTeleCommand,  "" },
            { "public_spell",   rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadPublicSpellCommand,  "" },
            { "morphs",         rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadMorphsCommand,       "" },
            { "item",           rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadItemCommand,         "" },
            { "freedomtables",  rbac::RBAC_FPERM_ADMINISTRATION,                    false, &HandleFreedomReloadAllCommand,          "" },
        };

        static std::vector<ChatCommand> freedomPandaCommandTable =
        {
            { "horde",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomPandarenHordeCommand,      "" },
            { "alliance",       rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomPandarenAllianceCommand,   "" },
        };

        static std::vector<ChatCommand> freedomTitleCommandTable =
        {
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomTitleListCommand,      "" },
            { "set",            rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomTitleSetCommand,       "" },
        };

        static std::vector<ChatCommand> freedomGuildCommandTable =
        {
            { "create",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomGuildCreateCommand,      "" },
        };

        static std::vector<ChatCommand> freedomPetCommandTable =
        {
            { "scale",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomPetScaleCommand,           "" },
            { "delete",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomPetDeleteCommand,          "" },
            { "list",           rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomListPetCommand,            "" },
            { "add",            rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomAddBPetCommand,            "" },
        };

        static std::vector<ChatCommand> freedomCommandTable =
        {
            { "hover",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomHoverCommand,              "" },
            { "cast",           rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL,             false, &HandleFreedomSpellCommand,              "" },
            { "aura",           rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL,             false, &HandleFreedomAuraCommand,              "" },
            { "summon",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomSummonCommand,             "" },
            { "demorph",        rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomDemorphCommand,            "" },
            { "fly",            rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomFlyCommand,                "" },
            { "revive",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomReviveCommand,             "" },
            { "unaura",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomUnAuraCommand,             "" },
            { "walk",           rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomWalkCommand,               "" },
            { "run",            rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomRunCommand,                "" },
            { "swim",           rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomSwimCommand,               "" },
            { "scale",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomScaleCommand,              "" },
            { "drunk",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomDrunkCommand,              "" },
            { "waterwalk",      rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomWaterwalkCommand,          "" },
            { "fix",            rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomFixCommand,                "" },
            { "morph",          rbac::RBAC_FPERM_COMMAND_FREEDOM_MORPH,             false, NULL,                                    "", freedomMorphCommandTable },
            { "mailbox",        rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomMailboxCommand,            "" },
            { "money",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomMoneyCommand,              "" },
            { "bank",           rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomBankCommand,               "" },
            { "customize",      rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomCustomizeCommand,          "" },
            { "racechange",     rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomRaceChangeCommand,         "" },
            { "factionchange",  rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomFactionChangeCommand,      "" },
            { "t",              rbac::RBAC_FPERM_COMMAND_FREEDOM_TELE,              false, NULL,                                    "", freedomTeleportCommandTable },
            { "teleport",       rbac::RBAC_FPERM_COMMAND_FREEDOM_TELE,              false, NULL,                                    "", freedomTeleportCommandTable },
            { "pteleport",      rbac::RBAC_FPERM_COMMAND_FREEDOM_PTELE,             false, NULL,                                    "", freedomPrivateTeleportCommandTable },
            { "spell",          rbac::RBAC_FPERM_COMMAND_FREEDOM_SPELL,             false, NULL,                                    "", freedomSpellCommandTable },
            { "speed",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomSpeedCommand,              "" },
            { "reload",         rbac::RBAC_FPERM_ADMINISTRATION,                    false, NULL,                                    "", freedomReloadCommandTable },
            //{ "tabard",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomTabardCommand,               "" },
            { "panda",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, NULL,                                    "",  freedomPandaCommandTable },
            { "tame",           rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomTameCommand,               "" },
            { "title",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, NULL,                                    "", freedomTitleCommandTable },
            { "recall",         rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomRecallCommand,             "" },
            { "guild",          rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, NULL,                                    "", freedomGuildCommandTable },
            { "gameaccount",    rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomGameAccountCreateCommand,  "" },
            { "accountaccess",  rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomAccountAccessCommand,      "" },
            { "changeaccount",  rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, &HandleFreedomChangeAccountCommand,      "" },
            { "phase",          rbac::RBAC_FPERM_COMMAND_PHASE,                     false, NULL,                                    "", freedomPhaseCommandTable },
            { "enchant",        rbac::RBAC_FPERM_COMMAND_FREEDOM_ENCHANT,           false, &HandleFreedomEnchantCommand,            "" },
            { "pet",            rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,         false, NULL,                                    "", freedomPetCommandTable },
            { "mount",          rbac::RBAC_FPERM_COMMAND_FREEDOM_MOUNT,             false, NULL,                                    "", freedomMountCommandTable },
            { "artifactappearances", rbac::RBAC_FPERM_COMMAND_FREEDOM_UTILITIES,    false, &HandleFreedomArtifactAppearancesCommand,"", },

        };

        static std::vector<ChatCommand> commandTable =
        {
            { "freedom",            rbac::RBAC_FPERM_COMMAND_FREEDOM,               false, NULL,                                    "", freedomCommandTable },
            { "f",                  rbac::RBAC_FPERM_COMMAND_FREEDOM,               false, NULL,                                    "", freedomCommandTable },
        };
        return commandTable;
    }

    static void ModifyMovementSpeed(ChatHandler* handler, UnitMoveType type, float value)
    {
        float max = sConfigMgr->GetFloatDefault("Freedom.Modify.MaxSpeed", 10.0f);
        float min = sConfigMgr->GetFloatDefault("Freedom.Modify.MinSpeed", 0.01f);
        std::string speedName;

        if (value < min || value > max)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_VALUE_OUT_OF_RANGE, max, min);
            return;
        }

        switch (type)
        {
        case UnitMoveType::MOVE_FLIGHT:
        case UnitMoveType::MOVE_FLIGHT_BACK:
            speedName = "fly";
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_FLIGHT, value);
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_FLIGHT_BACK, value);
            break;
        case UnitMoveType::MOVE_RUN:
            speedName = "run";
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_RUN, value);
            break;
        case UnitMoveType::MOVE_SWIM:
        case UnitMoveType::MOVE_SWIM_BACK:
            speedName = "swim";
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_SWIM, value);
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_SWIM_BACK, value);
            break;
        case UnitMoveType::MOVE_WALK:
        case UnitMoveType::MOVE_RUN_BACK:
            speedName = "walk";
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_WALK, value);
            handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_RUN_BACK, value);
            break;
        default:
            return;
        }

        handler->PSendSysMessage(FREEDOM_CMDI_MOD_SPEED, speedName.c_str(), value);
    }

    static bool HandleFreedomMorphListCommand(ChatHandler* handler, Optional<std::string> morphName)
    {
        const MorphDataContainer morphList = sFreedomMgr->GetMorphContainer(handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
        uint64 count = 0;

        if (!morphName)
        {
            for (auto morphData : morphList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_MORPH_LIST_ITEM, morphData.displayId, morphData.name);
                count++;
            }
        }
        else
        {
            for (auto morphData : morphList)
            {
                if (boost::istarts_with(morphData.name, morphName.value()))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_MORPH_LIST_ITEM, morphData.displayId, morphData.name);
                    count++;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Morphs");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }

    static bool HandleFreedomMorphAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MORPH_ADD);
            return true;
        }

        ArgumentTokenizer tokenizer(args);

        if (tokenizer.size() < 2)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_NOT_ENOUGH_PARAMS);
            handler->PSendSysMessage(FREEDOM_CMDH_MORPH_ADD);
            return true;
        }

        std::string morphName = tokenizer.TryGetParam<std::string>(0);
        uint32 displayId = tokenizer.TryGetParam<uint32>(1);
        Player* source = handler->GetSession()->GetPlayer();
        std::string targetNameArg = tokenizer.size() > 2 ? tokenizer.TryGetParam<std::string>(2) : "";
        std::string targetName;
        ObjectGuid targetGuid;

        if (!displayId)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_INVALID_ARGUMENT_X, "$displayId");
            return true;
        }

        if (!handler->extractPlayerTarget(&targetNameArg[0], nullptr, &targetGuid, &targetName))
            return true;

        // Check if morph already exists
        const MorphData* morphDataByName = sFreedomMgr->GetMorphByName(targetGuid.GetCounter(), morphName);
        const MorphData* morphDataByDisplayId = sFreedomMgr->GetMorphByDisplayId(targetGuid.GetCounter(), displayId);

        if (morphDataByName)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_ALREADY_EXISTS, "Morph", morphName);
            return true;
        }

        if (morphDataByDisplayId)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_ALREADY_EXISTS, "Morph", displayId);
            return true;
        }

        // Create teleport
        MorphData newMorphData;
        newMorphData.name = morphName;
        newMorphData.displayId = displayId;
        newMorphData.gmBnetAccId = source->GetSession()->GetBattlenetAccountId();

        sFreedomMgr->AddMorph(targetGuid.GetCounter(), newMorphData);

        handler->PSendSysMessage(FREEDOM_CMDI_MORPH_ADD, morphName, displayId, targetName);
        return true;
    }

    static bool HandleFreedomMorphDelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MORPH_DEL);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string morphName = tokenizer.TryGetParam(0);
        uint32 displayId = tokenizer.TryGetParam<uint32>(0);
        std::string targetNameArg = tokenizer.size() > 1 ? tokenizer.TryGetParam(1) : "";
        std::string targetName;
        ObjectGuid targetGuid;

        if (!handler->extractPlayerTarget(&targetNameArg[0], nullptr, &targetGuid, &targetName))
            return true;

        // Check if morph actually exists
        const MorphData* morphData = sFreedomMgr->GetMorphByName(targetGuid.GetCounter(), morphName);

        if (displayId && !morphData) // get by displayId only if name search doesn't turn up anything
        {
            morphData = sFreedomMgr->GetMorphByDisplayId(targetGuid.GetCounter(), displayId);

            if (!morphData)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Morph", displayId);
                return true;
            }
        }
        else if (!morphData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_NOT_FOUND, "Morph", morphName);
            return true;
        }

        morphName = morphData->name;
        displayId = morphData->displayId;

        sFreedomMgr->DeleteMorphByName(targetGuid.GetCounter(), morphData->name);

        handler->PSendSysMessage(FREEDOM_CMDI_MORPH_DEL, morphName, displayId, targetName);
        return true;
    }

    static bool HandleFreedomMorphCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MORPH);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string morphName = tokenizer.TryGetParam(0);
        uint32 displayId = tokenizer.TryGetParam<uint32>(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Check if morph actually exists
        const MorphData* morphData = sFreedomMgr->GetMorphByName(source->GetGUID().GetCounter(), morphName);

        if (displayId && !morphData) // get by displayId only if name search doesn't turn up anything
        {
            morphData = sFreedomMgr->GetMorphByDisplayId(source->GetGUID().GetCounter(), displayId);

            if (!morphData)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Morph", displayId);
                return true;
            }
        }
        else if (!morphData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_NOT_FOUND, "Morph", morphName);
            return true;
        }

        QueryResult result = FreedomDatabase.PQuery("SELECT display FROM character_extra WHERE guid='%u'", source->GetGUID().GetCounter());

        if (result)
            FreedomDatabase.PExecute("UPDATE character_extra SET display='%u' WHERE guid='%u'", morphData->displayId, source->GetGUID().GetCounter());
        else
            FreedomDatabase.PExecute("INSERT INTO character_extra(guid,display) VALUES ('%u','%u')", source->GetGUID().GetCounter(), morphData->displayId);

        source->SetDisplayId(morphData->displayId);
        handler->PSendSysMessage(FREEDOM_CMDI_MORPH, morphData->name, morphData->displayId);
        return true;
    }

    static bool HandleFreedomTeleListCommand(ChatHandler* handler, char const* args)
    {
        const PublicTeleContainer teleList = sFreedomMgr->GetPublicTeleportContainer();
        uint64 count = 0;

        if (!*args)
        {
            for (auto teleData : teleList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_PUBLIC_TELE_LIST_ITEM, teleData.name, sFreedomMgr->GetMapName(teleData.map));
                count++;
            }
        }
        else
        {
            ArgumentTokenizer tokenizer(args);
            std::string name = tokenizer.TryGetParam(0);

            for (auto teleData : teleList)
            {
                if (boost::istarts_with(teleData.name, name))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_PUBLIC_TELE_LIST_ITEM, teleData.name, sFreedomMgr->GetMapName(teleData.map));
                    count++;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Public teleports");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }

    static bool HandleFreedomTeleAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_TELE_ADD);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string name = tokenizer.TryGetParam(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Check if teleport already exists
        const PublicTeleData* teleData = sFreedomMgr->GetPublicTeleport(name);

        if (teleData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_ALREADY_EXISTS, "Public teleport", name);
            return true;
        }

        // Create teleport
        PublicTeleData newTeleData;
        source->GetPosition(newTeleData.x, newTeleData.y, newTeleData.z, newTeleData.o);
        newTeleData.map = source->GetMapId();
        newTeleData.name = name;
        newTeleData.gmBnetAccId = source->GetSession()->GetBattlenetAccountId();
        newTeleData.phaseId = sFreedomMgr->GetPlayerPhase(source);

        sFreedomMgr->AddPublicTeleport(newTeleData);

        handler->PSendSysMessage(FREEDOM_CMDI_X_ADDED_WITH_NAME, "Public teleport", name);
        return true;
    }

    static bool HandleFreedomTeleDelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_TELE_DEL);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string name = tokenizer.TryGetParam(0);

        // Check if teleport actually exists
        const PublicTeleData* teleData = sFreedomMgr->GetPublicTeleport(name);

        if (!teleData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_NOT_FOUND, "Public teleport", name);
            return true;
        }

        sFreedomMgr->DeletePublicTeleport(name);

        handler->PSendSysMessage(FREEDOM_CMDI_X_WITH_NAME_REMOVED, "Public teleport", name);
        return true;
    }

    static bool HandleFreedomTeleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_TELE);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string name = tokenizer.TryGetParam(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Stop combat before teleporting
        if (source->IsInCombat())
        {
            source->CombatStop();
        }

        // Get first match of public teleport
        const PublicTeleData* teleData = sFreedomMgr->GetFirstClosestPublicTeleport(name);

        if (!teleData)
        {
            handler->PSendSysMessage(FREEDOM_CMDI_X_NONE_FOUND_WITH_NAME, "public teleports", name);
            return true;
        }

        // Stop flight if needed
        if (source->IsInFlight())
        {
            source->FinishTaxiFlight();
        }
        // Save only in non-flight case
        else
        {
            source->SaveRecallPosition();
        }

        source->TeleportTo(teleData->map, teleData->x, teleData->y, teleData->z, teleData->o);
        sFreedomMgr->PlayerPhase(source, teleData->phaseId);
        return true;
    }

    static bool HandleFreedomPrivateTeleListCommand(ChatHandler* handler, char const* args)
    {
        const PrivateTeleVector teleList = sFreedomMgr->GetPrivateTeleportContainer(handler->GetSession()->GetBattlenetAccountId());
        uint64 count = 0;

        if (!*args)
        {
            for (auto teleData : teleList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_PRIVATE_TELE_LIST_ITEM, teleData.name, sFreedomMgr->GetMapName(teleData.map));
                count++;
            }
        }
        else
        {
            ArgumentTokenizer tokenizer(args);
            std::string name = tokenizer.TryGetParam(0);

            for (auto teleData : teleList)
            {
                if (boost::istarts_with(teleData.name, name))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_PRIVATE_TELE_LIST_ITEM, teleData.name, sFreedomMgr->GetMapName(teleData.map));
                    count++;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Private teleports");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }

    static bool HandleFreedomPrivateTeleAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PRIVATE_TELE_ADD);
            return true;
        }


        ArgumentTokenizer tokenizer(args);
        std::string name = tokenizer.TryGetParam(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Check if teleport already exists
        const PrivateTeleData* teleData = sFreedomMgr->GetPrivateTeleport(source->GetSession()->GetBattlenetAccountId(), name);

        if (teleData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_ALREADY_EXISTS, "Private teleport", name);
            return true;
        }

        // Create teleport
        PrivateTeleData newTeleData;
        source->GetPosition(newTeleData.x, newTeleData.y, newTeleData.z, newTeleData.o);
        newTeleData.map = source->GetMapId();
        newTeleData.name = name;
        newTeleData.phaseId = sFreedomMgr->GetPlayerPhase(source);

        sFreedomMgr->AddPrivateTeleport(source->GetSession()->GetBattlenetAccountId(), newTeleData);

        handler->PSendSysMessage(FREEDOM_CMDI_X_ADDED_WITH_NAME, "Private teleport", name);
        return true;
    }

    static bool HandleFreedomPrivateTeleDelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PRIVATE_TELE_DEL);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string name = tokenizer.TryGetParam(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Check if teleport actually exists
        const PrivateTeleData* teleData = sFreedomMgr->GetPrivateTeleport(source->GetSession()->GetBattlenetAccountId(), name);

        if (!teleData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_NOT_FOUND, "Private teleport", name);
            return true;
        }

        sFreedomMgr->DeletePrivateTeleport(source->GetSession()->GetBattlenetAccountId(), name);

        handler->PSendSysMessage(FREEDOM_CMDI_X_WITH_NAME_REMOVED, "Private teleport", name);
        return true;
    }

    static bool HandleFreedomPrivateTeleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PRIVATE_TELE);
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string name = tokenizer.TryGetParam(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Stop combat before teleporting
        if (source->IsInCombat())
        {
            source->CombatStop();
        }

        // Get first match of public teleport
        const PrivateTeleData* teleData = sFreedomMgr->GetFirstClosestPrivateTeleport(source->GetSession()->GetBattlenetAccountId(), name);

        if (!teleData)
        {
            handler->PSendSysMessage(FREEDOM_CMDI_X_NONE_FOUND_WITH_NAME, "private teleports", name);
            return true;
        }

        // Stop flight if needed
        if (source->IsInFlight())
        {
            source->FinishTaxiFlight();
        }
        // Save only in non-flight case
        else
        {
            source->SaveRecallPosition();
        }

        source->TeleportTo(teleData->map, teleData->x, teleData->y, teleData->z, teleData->o);
        sFreedomMgr->PlayerPhase(source, teleData->phaseId);
        return true;
    }

    static bool HandleFreedomSpellListCommand(ChatHandler* handler, char const* args)
    {
        const PublicSpellContainer spellList = sFreedomMgr->GetPublicSpellContainer();
        uint64 count = 0;

        if (!*args)
        {
            for (auto spellData : spellList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_PUBLIC_SPELL_LIST_ITEM, spellData.first, sFreedomMgr->ToChatLink("Hspell", spellData.first, spellData.second.name));
                count++;
            }
        }
        else
        {
            ArgumentTokenizer tokenizer(args);
            std::string name = tokenizer.TryGetParam(0);

            for (auto spellData : spellList)
            {
                if (boost::istarts_with(spellData.second.name, name))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_PUBLIC_SPELL_LIST_ITEM, spellData.first, sFreedomMgr->ToChatLink("Hspell", spellData.first, spellData.second.name));
                    count++;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Public spells");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }

    static bool HandleFreedomSpellAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_SPELL_ADD);
            return true;
        }

        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        Player* source = handler->GetSession()->GetPlayer();
        uint8 targetOthers = 0;
        char* targetOthersArg = strtok(NULL, args);

        if (targetOthersArg && boost::iequals("true", targetOthersArg))
            targetOthers = 1;

        // Check if public spell already exists
        const PublicSpellData* spellData = sFreedomMgr->GetPublicSpell(spellId);

        if (spellData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_ALREADY_EXISTS, "Public spell", spellId);
            return true;
        }

        const SpellNameEntry* spellEntry = sSpellNameStore.LookupEntry(spellId);

        if (!spellEntry)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Spell entry", spellId);
            return true;
        }

        // Create teleport
        PublicSpellData newSpellData;
        LocaleConstant locale = handler->GetSessionDbcLocale();
        newSpellData.name = spellEntry->Name[locale];
        newSpellData.targetOthers = targetOthers;
        newSpellData.gmBnetAccId = source->GetSession()->GetBattlenetAccountId();

        sFreedomMgr->AddPublicSpell(spellId, newSpellData);

        handler->PSendSysMessage(FREEDOM_CMDI_PUBLIC_SPELL_ADD, sFreedomMgr->ToChatLink("Hspell", spellId, spellEntry->Name[locale]), spellId);
        return true;
    }

    static bool HandleFreedomSpellDelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_SPELL_DEL);
            return true;
        }

        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        std::string spellName;

        // Check if public spell already exists
        const PublicSpellData* spellData = sFreedomMgr->GetPublicSpell(spellId);

        if (!spellData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Public spell", spellId);
            return true;
        }

        spellName = spellData->name;

        sFreedomMgr->DeletePublicSpell(spellId);

        handler->PSendSysMessage(FREEDOM_CMDI_PUBLIC_SPELL_DEL, sFreedomMgr->ToChatLink("Hspell", spellId, spellName), spellId);
        return true;
    }

    static bool HandleFreedomSpellCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_SPELL);
            return true;
        }

        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        Player* source = handler->GetSession()->GetPlayer();
        Unit* target = handler->getSelectedUnit();

        // Check if public spell already exists
        const PublicSpellData* spellData = sFreedomMgr->GetPublicSpell(spellId);

        if (!spellData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Public spell", spellId);
            return true;
        }

        const SpellNameEntry* spellEntry = sSpellNameStore.LookupEntry(spellId);

        if (!spellEntry)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Spell entry", spellId);
            return true;
        }

        source->CastSpell(target, spellId);
        return true;
    }


    static bool HandleFreedomAuraCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_PUBLIC_SPELL);
            return true;
        }

        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        Player* source = handler->GetSession()->GetPlayer();

        // Check if public spell already exists
        const PublicSpellData* spellData = sFreedomMgr->GetPublicSpell(spellId);

        if (!spellData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Public spell", spellId);
            return true;
        }

        const SpellInfo* spellEntry = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NORMAL);

        if (!spellEntry)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Spell entry", spellId);
            return true;
        }

        ObjectGuid castId = ObjectGuid::Create<HighGuid::Cast>(SPELL_CAST_SOURCE_NORMAL, source->GetMapId(), spellId, source->GetMap()->GenerateLowGuid<HighGuid::Cast>());
        AuraCreateInfo createInfo(castId, spellEntry, source->GetMap()->GetDifficultyID(), MAX_EFFECT_MASK, source);
        createInfo.SetCaster(source);

        Aura::TryRefreshStackOrCreate(createInfo);
        return true;
    }

    static bool HandleFreedomReloadAllCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "freedomtables", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadAllTables();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_ALL).c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadCreatureExtraCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "creature extra", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadCreatureTemplateExtras();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "creature_template_extra").c_str());
        sFreedomMgr->LoadCreatureExtras();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "creature_extra").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadCreatureAddonCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "creature addon", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sObjectMgr->LoadCreatureTemplateAddons();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature_template_addon").c_str());
        sObjectMgr->LoadCreatureAddons();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature_addon").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadCreatureBaseCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "creature base", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sObjectMgr->LoadCreatureModelInfo();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature_model_info").c_str());
        sObjectMgr->LoadCreatureTemplates();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature_template").c_str());
        sObjectMgr->LoadEquipmentTemplates();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature_equip_template").c_str());
        sObjectMgr->LoadCreatures();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadCreatureEquipCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "creature equip", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sObjectMgr->LoadEquipmentTemplates();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "creature_equip_template").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadGameobjectExtraCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "gameobject extra", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadGameObjectTemplateExtras();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "gameobject_template_extra").c_str());
        sFreedomMgr->LoadGameObjectExtras();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "gameobject_extra").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadGameobjectBaseCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "gameobject base", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sObjectMgr->LoadGameObjectTemplate();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "gameobject_template").c_str());
        sObjectMgr->LoadGameObjects();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_WORLD_DB, "gameobject").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadPublicTeleCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "public_tele", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadPublicTeleports();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "public_tele").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadPrivateTeleCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "private_tele", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadPrivateTeleports();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "private_tele").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadPublicSpellCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "public_spell", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadPublicSpells();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "public_spell").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadMorphsCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "morphs", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadMorphs();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "morphs").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomReloadItemCommand(ChatHandler* handler)
    {
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_EXECUTOR, "item", handler->GetNameLink()).c_str());
        uint32 oldMSTime = getMSTime();
        sFreedomMgr->LoadItemTemplateExtras();
        handler->SendGlobalGMSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD, "item_template_extra").c_str());
        handler->SendGlobalSysMessage(handler->PGetParseString(FREEDOM_CMDI_RELOAD_FINISH, GetMSTimeDiffToNow(oldMSTime)).c_str());
        return true;
    }

    static bool HandleFreedomGuildCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_FREEDOM_GUILD_CREATE);
            return true;
        }

        // if not guild name only (in "") then player name
        Player* source = handler->GetSession()->GetPlayer();
        ArgumentTokenizer tokenizer(args);

        std::string guildName = tokenizer.GetUntokenizedString();

        if (source->GetGuildId())
        {
            handler->SendSysMessage(FREEDOM_CMDE_FREEDOM_GUILD_CREATE_ALREADY_IN);
            return true;
        }

        Guild* guild = new Guild;
        if (!guild->Create(source, guildName))
        {
            delete guild;
            handler->SendSysMessage(FREEDOM_CMDE_FREEDOM_GUILD_CREATE_FAILED);
            return true;
        }

        sGuildMgr->AddGuild(guild);
        handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_GUILD_CREATE, guildName);
        return true;
    }

    static bool HandleFreedomRecallCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* source = handler->GetSession()->GetPlayer();

        // stop flight if need
        if (source->IsInFlight())
        {
            source->FinishTaxiFlight();
        }

        source->Recall();
        return true;
    }

    static bool HandleFreedomPandarenHordeCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (player->GetRace() != RACE_PANDAREN_NEUTRAL)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_FREEDOM_PANDA_NOT_NEUTRAL);
            return true;
        }

        player->SetRace(RACE_PANDAREN_HORDE);
        player->SetFactionForRace(RACE_PANDAREN_HORDE);
        player->SaveToDB();
        player->LearnSpell(108131, false); // Language Pandaren Horde
        handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_PANDAHORDE);

        return true;
    }

    static bool HandleFreedomPandarenAllianceCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (player->GetRace() != RACE_PANDAREN_NEUTRAL)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_FREEDOM_PANDA_NOT_NEUTRAL);
            return true;
        }

        player->SetRace(RACE_PANDAREN_ALLIANCE);
        player->SetFactionForRace(RACE_PANDAREN_ALLIANCE);
        player->SaveToDB();
        player->LearnSpell(108130, false); // Language Pandaren Alliance
        handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_PANDAALLIANCE);

        return true;
    }

    static bool HandleFreedomTabardCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();
        handler->GetSession()->SendTabardVendorActivate(source->GetGUID());
        return true;
    }

    static bool HandleFreedomTitleListCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();

        ArgumentTokenizer tokenizer(*args ? args : "");
        std::string namePart = tokenizer.GetUntokenizedString();
        Gender gender = (Gender)source->GetGender();
        uint32 count = 0;

        for (auto titleEntry : sCharTitlesStore)
        {
            LocaleConstant locale = handler->GetSessionDbcLocale();
            std::string titleName = (gender == GENDER_MALE ? titleEntry->Name : titleEntry->Name1)[locale];

            if (boost::icontains(titleName, namePart))
            {
                count++;
                handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_TITLE_LIST_ITEM,
                    titleEntry->ID,
                    sFreedomMgr->ToChatLink("Htitle", titleEntry->ID, titleName));
            }
        }

        if (count)
            handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_TITLE_LIST, count);
        else
            handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_TITLE_LIST_NONE_FOUND);

        return true;
    }

    static bool HandleFreedomTitleSetCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_FREEDOM_TITLE_SET);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        ArgumentTokenizer tokenizer(args);
        uint32 titleId = tokenizer.TryGetParam<uint32>(0, "Htitle");
        uint32 prevMaskId = source->m_playerData->PlayerTitle;

        if (!titleId)
        {
            source->SetChosenTitle(0);
            source->SaveToDB();

            WorldPackets::Character::TitleEarned packetRemovePrevious(SMSG_TITLE_LOST);
            packetRemovePrevious.Index = prevMaskId;
            source->GetSession()->SendPacket(packetRemovePrevious.Write());
            handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_TITLE_SET_UNSET);
            return true;
        }

        CharTitlesEntry const* titleEntry = sCharTitlesStore.LookupEntry(titleId);

        if (!titleEntry)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_FREEDOM_TITLE_SET_INVALID_TITLEID, titleId);
            return true;
        }

        // remove previous one
        WorldPackets::Character::TitleEarned packetRemovePrevious(SMSG_TITLE_LOST);
        packetRemovePrevious.Index = prevMaskId;
        source->GetSession()->SendPacket(packetRemovePrevious.Write());

        // set chosen title
        WorldPackets::Character::TitleEarned packet(SMSG_TITLE_EARNED);
        packet.Index = titleEntry->MaskID;
        source->GetSession()->SendPacket(packet.Write());

        source->SetChosenTitle(titleEntry->MaskID);
        source->SaveToDB();

        Gender gender = (Gender)source->GetGender();
        LocaleConstant locale = handler->GetSessionDbcLocale();
        std::string titleName = (gender == GENDER_MALE ? titleEntry->Name : titleEntry->Name1)[locale];

        handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_TITLE_SET,
            titleEntry->ID,
            titleEntry->MaskID,
            sFreedomMgr->ToChatLink("Htitle", titleEntry->ID, titleName));
        return true;
    }

    static bool HandleFreedomTameCommand(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_MANUAL_SELECT_CREATURE);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();

        if (!source->GetPetGUID().IsEmpty())
        {
            handler->SendSysMessage(FREEDOM_CMDE_FREEDOM_TAME_ALREADY_HAVE_PET);
            return true;
        }

        CreatureTemplate const* cInfo = target->GetCreatureTemplate();

        PetStable& petStable = source->GetOrInitPetStable();
        auto freeActiveSlotItr = std::find_if(petStable.ActivePets.begin(), petStable.ActivePets.end(), [](Optional<PetStable::PetInfo> const& petInfo)
            {
                return !petInfo.has_value();
            });

        if (freeActiveSlotItr == petStable.ActivePets.end())
        {
            handler->PSendSysMessage("Your pet stable is currently full, please try using .freedom pet delete to clear up space.");
            return true;
        }

        // Everything looks OK, create new pet
        Pet* pet = source->CreateTamedPetFrom(target);
        if (!pet)
        {
            handler->PSendSysMessage("Unable to create a tamed pet for creature template %u. Please contact a dev.", cInfo->Entry);
            return true;
        }

        // place pet before player
        float x, y, z;
        source->GetClosePoint(x, y, z, target->GetCombatReach(), CONTACT_DISTANCE);
        pet->Relocate(x, y, z, float(M_PI) - source->GetOrientation());

        // set pet to defensive mode by default (some classes can't control controlled pets in fact).
        pet->SetReactState(REACT_DEFENSIVE);

        // calculate proper level
        uint8 level = (target->GetLevel() < (source->GetLevel() - 5)) ? (source->GetLevel() - 5) : target->GetLevel();

        // prepare visual effect for levelup
        pet->SetLevel(level - 1);

        // add to world
        pet->GetMap()->AddToMap(pet->ToCreature());

        // visual effect for levelup
        pet->SetLevel(level);

        // caster have pet now
        source->SetMinion(pet, true);

        pet->SavePetToDB(PET_SAVE_AS_CURRENT);
        source->PetSpellInitialize();

        handler->PSendSysMessage(FREEDOM_CMDI_FREEDOM_TAME,
            sFreedomMgr->ToChatLink("Hcreature_entry", target->GetEntry(), target->GetName()));
        return true;
    }

    static bool HandleFreedomPetDeleteCommand(ChatHandler* handler, char const* args)
    {
        Player* source = handler->GetSession()->GetPlayer();
        if (!*args) {
            handler->PSendSysMessage("Please provide a slot (1-%u) to clear the pet from.", MAX_ACTIVE_PETS);
            return true;
        }
        int slot = atoi(args);
        if (slot > MAX_ACTIVE_PETS || slot < 1) {
            handler->PSendSysMessage("Slot %i is invalid. Please provide a slot (1-%u) to clear the pet from.", slot, MAX_ACTIVE_PETS);
            return true;
        }
        PetStable& petStable = source->GetOrInitPetStable();
        std::optional<PetStable::PetInfo> pInfo = petStable.ActivePets[slot-1];
        if (!pInfo.has_value()) {
            handler->PSendSysMessage("Slot %i does not have an active pet in it.", slot);
            return true;
        }

        Pet* currentPet = source->GetPet();
        if (currentPet && currentPet->GetCharmInfo()->GetPetNumber() == pInfo.value().PetNumber)
        {
            source->SetMinion(currentPet, false);
            currentPet->AddObjectToRemoveList();
            currentPet->m_removed = true;
        }

        Pet::DeleteFromDB(pInfo.value().PetNumber);
        petStable.ActivePets[slot-1] = std::nullopt;
        handler->PSendSysMessage("Pet %s in slot %i was succesfully deleted!", pInfo.value().Name, slot);
        return true;
    }

    static bool HandleFreedomHoverCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            // did not specify ON or OFF value
            handler->PSendSysMessage(FREEDOM_CMDH_HOVER);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();

        // on & off
        std::string arg = strtok((char*)args, " ");
        if (boost::iequals(arg, "on"))
        {
            handler->PSendSysMessage(FREEDOM_CMDI_HOVER, "on");
            source->AddAura(SPELL_PERMANENT_HOVER, source);
            source->SetHover(true);
            return true;
        }
        else if (boost::iequals(arg, "off"))
        {
            sFreedomMgr->RemoveHoverFromPlayer(source);
            source->RemoveAura(SPELL_PERMANENT_HOVER);
            handler->PSendSysMessage(FREEDOM_CMDI_HOVER, "off");
            return true;
        }

        // did not specify ON or OFF value
        handler->PSendSysMessage(FREEDOM_CMDH_HOVER);

        return true;
    }

    static bool HandleFreedomSummonCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_SUMMON);
            return true;
        }
        std::string playerName = args;
        playerName[0] = toupper(playerName[0]);

        Player* source = handler->GetSession()->GetPlayer();
        ObjectGuid targetGuid = sCharacterCache->GetCharacterGuidByName(playerName);
        Player* target = ObjectAccessor::FindConnectedPlayer(targetGuid);

        if (!target || target->IsLoading() || target->IsBeingTeleported())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SUMMON_NOT_FOUND, args);
            return true;
        }

        if (target->GetSocial()->HasIgnore(source->GetGUID(), source->GetSession()->GetAccountGUID()))
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SUMMON_IGNORE, target->GetName().c_str());
            return true;
        }

        if (target->IsGameMaster())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SUMMON_GM_ON, target->GetName().c_str());
            return true;
        }

        if (target->HasSummonPending())
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SUMMON_PENDING, target->GetName().c_str());
            return true;
        }

        // Evil Twin (ignore player summon, but hide this for summoner)
        if (target->HasAura(23445))
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SUMMON_EVIL_TWIN, target->GetName().c_str());
            return true;
        }

        target->SendSummonRequestFrom(source);

        handler->PSendSysMessage(FREEDOM_CMDI_SUMMON, target->GetName().c_str());
        return true;
    }

    static bool HandleFreedomDemorphCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();

        QueryResult result = FreedomDatabase.PQuery("SELECT display FROM character_extra WHERE guid='%u'", source->GetGUID().GetCounter());
        if (result)
            FreedomDatabase.PExecute("UPDATE character_extra SET display='0' WHERE guid='%u'", source->GetGUID().GetCounter());

        source->DeMorph();
        handler->PSendSysMessage(FREEDOM_CMDI_DEMORPH);
        return true;
    }

    static bool HandleFreedomFlyCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_FLY);
            handler->PSendSysMessage(FREEDOM_CMDH_MOD_SPEED);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        std::string arg = strtok((char*)args, " ");

        if (boost::iequals(arg, "on"))
        {
            source->SetCanFly(true);
            handler->PSendSysMessage(FREEDOM_CMDI_FLY, "on");
        }
        else if (boost::iequals(arg, "off"))
        {
            sFreedomMgr->RemoveFlyFromPlayer(source);
            handler->PSendSysMessage(FREEDOM_CMDI_FLY, "off");
        }
        else
        {
            ModifyMovementSpeed(handler, MOVE_FLIGHT, (float)atof(arg.c_str()));
        }

        return true;
    }

    static bool HandleFreedomReviveCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();

        if (source->IsAlive()) {
            handler->PSendSysMessage(FREEDOM_CMDE_REVIVE);
            return true;
        }

        source->ResurrectPlayer(1.0);
        source->SaveToDB();
        handler->PSendSysMessage(FREEDOM_CMDI_REVIVE);
        return true;
    }

    static bool HandleFreedomUnAuraCommand(ChatHandler* handler, Optional<uint32> spellId)
    {
        Player* source = handler->GetSession()->GetPlayer();

        if (!spellId.has_value()) {

            sFreedomMgr->RemoveHoverFromPlayer(source); // unaura removes hover animation, so proceed to remove entire hover mechanic
            source->RemoveAllAuras();
            source->RemoveAllAreaTriggers();
            while (!source->m_dynObj.empty())
                source->m_dynObj.front()->Remove();
            // Remove summons from spells.
            for (auto i = source->m_Controlled.begin(); i != source->m_Controlled.end();)
            {
                Unit* unit = (*i);
                i++;
                if (unit && unit->IsSummon()) {
                    unit->ToTempSummon()->UnSummon();
                }
            }
            source->RemoveAllGameObjects();
            sFreedomMgr->RemoveAllAuraApplications(source);
            source->SetAIAnimKitId(0);

            handler->PSendSysMessage(FREEDOM_CMDI_UNAURA);
            return true;
        }

        sFreedomMgr->RemoveSpellEffects(source, spellId.value());
        handler->PSendSysMessage("Effects from spell %u have been removed from you.", spellId.value());
        return true;
    }

    static bool HandleFreedomSpeedCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MOD_SPEED);
            return true;
        }

        float speed = (float)atof((char*)args);
        float max = sConfigMgr->GetFloatDefault("Freedom.Modify.MaxSpeed", 10.0f);
        float min = sConfigMgr->GetFloatDefault("Freedom.Modify.MinSpeed", 0.01f);
        std::string speedName = "all";

        if (speed < min || speed > max)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_VALUE_OUT_OF_RANGE, max, min);
            return true;
        }

        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_FLIGHT, speed);
        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_FLIGHT_BACK, speed);
        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_RUN, speed);
        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_SWIM, speed);
        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_SWIM_BACK, speed);
        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_WALK, speed);
        handler->GetSession()->GetPlayer()->SetSpeedRate(MOVE_RUN_BACK, speed);
        handler->PSendSysMessage(FREEDOM_CMDI_MOD_SPEED, speedName.c_str(), speed);
        return true;
    }

    static bool HandleFreedomWalkCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MOD_SPEED);
            return true;
        }

        ModifyMovementSpeed(handler, MOVE_WALK, (float)atof((char*)args));
        return true;
    }

    static bool HandleFreedomRunCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MOD_SPEED);
            return true;
        }

        ModifyMovementSpeed(handler, MOVE_RUN, (float)atof((char*)args));
        return true;
    }

    static bool HandleFreedomSwimCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_MOD_SPEED);
            return true;
        }

        ModifyMovementSpeed(handler, MOVE_SWIM, (float)atof((char*)args));
        return true;
    }

    static bool HandleFreedomScaleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            float currentScale = handler->GetSession()->GetPlayer()->GetObjectScale();
            handler->PSendSysMessage(FREEDOM_CMDI_SCALE, currentScale);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        float scale = (float)atof((char*)args);
        float max = sConfigMgr->GetFloatDefault("Freedom.Modify.MaxScale", 10.0f);
        float min = sConfigMgr->GetFloatDefault("Freedom.Modify.MinScale", 0.01f);

        if (scale < min || scale > max)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_VALUE_OUT_OF_RANGE, max, min);
            return true;
        }


        QueryResult result = FreedomDatabase.PQuery("SELECT scale FROM character_extra WHERE guid='%u'", handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
        if (result)
        {
            //Field* fields = result->Fetch();
            //float currentScale = fields[0].GetFloat();
            //handler->PSendSysMessage(FREEDOM_CMDI_SCALE_CHANGE, currentScale, scale);

            Player* chr = handler->GetSession()->GetPlayer();
            handler->PSendSysMessage(FREEDOM_CMDI_SCALE, scale);
            chr->SetObjectScale(scale);

            FreedomDatabase.PExecute("UPDATE character_extra SET scale='%f' WHERE guid='%u'", scale, chr->GetGUID().GetCounter());
            return true;
        }

        else
        {
            Player* chr = handler->GetSession()->GetPlayer();

            handler->PSendSysMessage(FREEDOM_CMDI_SCALE, scale);
            chr->SetObjectScale(scale);
            FreedomDatabase.PExecute("INSERT INTO character_extra (guid,scale) VALUES ('%u','%f')", chr->GetGUID().GetCounter(), scale);

            return true;
        }

        source->SetObjectScale(scale);
        return true;
    }

    static bool HandleFreedomDrunkCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_DRUNK);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();
        uint8 drunklevel = (uint8)atoul(args);

        if (drunklevel > 100)
            drunklevel = 100;

        source->SetDrunkValue(drunklevel);
        handler->PSendSysMessage(FREEDOM_CMDI_DRUNK, drunklevel);
        return true;
    }

    static bool HandleFreedomWaterwalkCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            // did not specify ON or OFF value
            handler->PSendSysMessage(FREEDOM_CMDH_WATERWALK);
            return true;
        }

        Player* source = handler->GetSession()->GetPlayer();

        // on & off
        std::string arg = strtok((char*)args, " ");
        if (boost::iequals(arg, "on"))
        {
            handler->PSendSysMessage(FREEDOM_CMDI_WATERWALK, "on");
            source->SetWaterWalking(true);
            return true;
        }
        else if (boost::iequals(arg, "off"))
        {
            sFreedomMgr->RemoveWaterwalkFromPlayer(source);
            handler->PSendSysMessage(FREEDOM_CMDI_WATERWALK, "off");
            return true;
        }

        // did not specify ON or OFF value
        handler->PSendSysMessage(FREEDOM_CMDH_WATERWALK);

        return true;
    }

    static bool HandleFreedomFixCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();
        source->DurabilityRepairAll(false, 0, false);
        handler->PSendSysMessage(FREEDOM_CMDI_FIX_ITEMS);
        return true;
    }

    static bool HandleFreedomMailboxCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();
        handler->GetSession()->SendShowMailBox(source->GetGUID());
        return true;
    }

    static bool HandleFreedomMoneyCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();
        source->SetMoney(uint64(100000000000Ui64));
        handler->PSendSysMessage(FREEDOM_CMDI_MONEY_RESET);
        return true;
    }

    static bool HandleFreedomBankCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();
        handler->GetSession()->SendShowBank(source->GetGUID());
        return true;
    }

    static bool HandleFreedomCustomizeCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->setUInt16(0, uint16(AT_LOGIN_CUSTOMIZE));
        stmt->setUInt32(1, source->GetGUID().GetCounter());
        CharacterDatabase.Execute(stmt);

        source->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
        handler->PSendSysMessage(FREEDOM_CMDI_FLAG_FOR_CUSTOMIZATION);

        return true;
    }

    static bool HandleFreedomRaceChangeCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();

        if (source->GetRace() == RACE_PANDAREN_ALLIANCE || source->GetRace() == RACE_PANDAREN_HORDE || source->GetRace() == RACE_PANDAREN_NEUTRAL)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SERVICE_RESTRICTED_FOR_PANDAS);
            return true;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->setUInt16(0, uint16(AT_LOGIN_CHANGE_RACE));
        stmt->setUInt32(1, source->GetGUID().GetCounter());
        CharacterDatabase.Execute(stmt);

        source->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
        handler->PSendSysMessage(FREEDOM_CMDI_FLAG_FOR_RACECHANGE);
        return true;
    }

    static bool HandleFreedomFactionChangeCommand(ChatHandler* handler)
    {
        Player* source = handler->GetSession()->GetPlayer();

        if (source->GetRace() == RACE_PANDAREN_NEUTRAL)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_SERVICE_RESTRICTED_FOR_NPANDAS);
            return true;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->setUInt16(0, uint16(AT_LOGIN_CHANGE_FACTION));
        stmt->setUInt32(1, source->GetGUID().GetCounter());
        CharacterDatabase.Execute(stmt);

        source->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
        handler->PSendSysMessage(FREEDOM_CMDI_FLAG_FOR_FACTIONCHANGE);
        return true;
    }

    static bool HandleFreedomPetScaleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        Pet* pet;

        const float PET_SCALE_MAX = sConfigMgr->GetFloatDefault("Freedom.Modify.MaxScale", 10.0f);
        const float PET_SCALE_MIN = sConfigMgr->GetFloatDefault("Freedom.Modify.MinScale", 0.01f);

        Player* source = handler->GetSession()->GetPlayer();
        pet = source->GetPet();
        /*
        if (AccountMgr::IsModeratorAccount(handler->GetSession()->GetSecurity()) && handler->getSelectedPlayer())
            pet = handler->getSelectedPlayer()->GetPet();
        else
            pet = handler->GetSession()->GetPlayer()->GetPet();

        if (AccountMgr::IsModeratorAccount(handler->GetSession()->GetSecurity()) && !pet)
        {
            handler->PSendSysMessage("You/your target has no pet.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        else*/ if (!pet)
        {
            handler->PSendSysMessage("You have no pet.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (pet->isTemporarySummoned())
        {
            handler->PSendSysMessage("You cannot save the scale of temporary pets!");
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (param == "reset")
        {
            pet->SetPetAddon(source, 0);
            pet->SetObjectScale(1);
            return true;
        }
        else
        {
            float Scale = (float)atof((char*)args);
            if (Scale > PET_SCALE_MAX || Scale < PET_SCALE_MIN)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_VALUE_OUT_OF_RANGE, PET_SCALE_MAX, PET_SCALE_MIN);
                handler->SetSentErrorMessage(true);
                return false;
            }
            pet->SetPetAddon(source, Scale);
            pet->SetObjectScale(Scale);
            return true;
        }

        return true;
    }

    static bool HandleFreedomGameAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(FREEDOM_CMDH_GAMEACCOUNTCREATE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* indexVer = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");

        if (!indexVer || !password)
            return false;

        std::string bnetAccountName;
        uint32 accountId = handler->GetSession()->GetBattlenetAccountId();

        uint8 indexVerify = atoi(indexVer);
        if (indexVerify != Battlenet::AccountMgr::GetMaxIndex(accountId) || indexVerify > 9)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_ACCT_INDEX_NO_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_BNET_ACCOUNT_EMAIL_BY_ID);
        stmt->setUInt32(0, accountId);
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (!result)
            return false;

        bnetAccountName = (*result)[0].GetString();

        uint8 index = Battlenet::AccountMgr::GetMaxIndex(accountId) + 1;
        std::string accountName = std::to_string(accountId) + '#' + std::to_string(uint32(index));

        // Generate random hex string for password, these accounts must not be logged on with GRUNT
        /*BigNumber randPassword;
        randPassword.SetRand(8 * 16);

        switch (sAccountMgr->CreateAccount(accountName, ByteArrayToHexStr(randPassword.AsByteArray().get(), randPassword.GetNumBytes()), bnetAccountName, accountId, index))*/
        switch (sAccountMgr->CreateAccount(accountName, password, bnetAccountName, accountId, index))
        {
        case AccountOpResult::AOR_OK:
            handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName.c_str());
            if (handler->GetSession())
            {
                TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (%s) created Account %s (Email: '%s')",
                    handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                    handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().ToString().c_str(),
                    accountName.c_str(), bnetAccountName.c_str());
            }
            break;
        case AccountOpResult::AOR_NAME_TOO_LONG:
            handler->SendSysMessage(LANG_ACCOUNT_NAME_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_PASS_TOO_LONG:
            handler->SendSysMessage(LANG_ACCOUNT_PASS_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_NAME_ALREADY_EXIST:
            handler->SendSysMessage(LANG_ACCOUNT_ALREADY_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_DB_INTERNAL_ERROR:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountTypes gmlevel = handler->GetSession()->GetSecurity();
        rbac::RBACData* rbac = handler->getSelectedPlayer()->GetSession()->GetRBACData();
        sAccountMgr->UpdateAccountAccess(rbac, AccountMgr::GetId(accountName), uint8(gmlevel), -1);

        return true;
    }

    static bool HandleFreedomAccountAccessCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(FREEDOM_CMDH_ACCOUNTACCESS);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string confirm = (char*)args;

        if (confirm != "1")
        {
            handler->SendSysMessage(FREEDOM_CMDE_ACCOUNTACCESS_CONFIRM);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 BNetAccountId = handler->GetSession()->GetBattlenetAccountId();
        std::string AccountName = std::to_string(BNetAccountId) + "#1";
        uint32 maingmlevel = sAccountMgr->GetSecurity(AccountMgr::GetId(AccountName), -1);

        if (maingmlevel == AccountMgr::GetSecurity(handler->GetSession()->GetAccountId(), -1))
        {
            handler->SendSysMessage(FREEDOM_CMDE_ACCOUNTACCESS_SAMELEVEL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        rbac::RBACData* rbac = handler->GetSession()->GetRBACData();
        sAccountMgr->UpdateAccountAccess(rbac, handler->GetSession()->GetAccountId(), uint8(maingmlevel), -1);
        handler->SendSysMessage(FREEDOM_CMDI_ACCOUNTACCESS_DONE);

        return true;
    }

    // TODO: Can probably go with new command structure args.
    static void extractOptFirstArg(char* args, char** arg1, char** arg2)
    {
        char* p1 = strtok(args, " ");
        char* p2 = strtok(nullptr, " ");

        if (!p2)
        {
            p2 = p1;
            p1 = nullptr;
        }

        if (arg1)
            *arg1 = p1;

        if (arg2)
            *arg2 = p2;
    }
    static bool HandleFreedomChangeAccountCommand(ChatHandler* handler, char const* args)
    {
        char* playerNameStr;
        char* accountNameStr;
        extractOptFirstArg(const_cast<char*>(args), &playerNameStr, &accountNameStr);
        if (!accountNameStr)
        {
            handler->PSendSysMessage(FREEDOM_CMDH_CHANGEACCOUNT);
            return true;
        }

        ObjectGuid targetGuid;
        std::string targetName;
        if (!handler->extractPlayerTarget(playerNameStr, nullptr, &targetGuid, &targetName))
            return false;

        CharacterCacheEntry const* characterInfo = sCharacterCache->GetCharacterCacheByGuid(targetGuid);
        if (!characterInfo)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 userBNetAccountId = handler->GetSession()->GetBattlenetAccountId();

        uint32 oldAccountId = characterInfo->AccountId;
        uint32 newAccountId = oldAccountId;

        std::string accountName = std::to_string(userBNetAccountId) + "#" + accountNameStr;
        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME);
        stmt->setString(0, accountName);
        if (PreparedQueryResult result = LoginDatabase.Query(stmt))
            newAccountId = (*result)[0].GetUInt32();
        else
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountNameStr);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // nothing to do :)
        if (newAccountId == oldAccountId)
            return true;

        uint32 oldBNetAccountId = Battlenet::AccountMgr::GetIdByGameAccount(oldAccountId);
        uint32 newBNetAccountId = Battlenet::AccountMgr::GetIdByGameAccount(newAccountId);

        if (userBNetAccountId != oldBNetAccountId || userBNetAccountId != newBNetAccountId)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_CHANGEACCOUNT_DIFFBNETACCT);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (uint32 charCount = AccountMgr::GetCharactersCount(newAccountId))
        {
            if (charCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_CHARACTER_LIST_FULL, accountName.c_str(), newAccountId);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        CharacterDatabasePreparedStatement* stmt2 = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ACCOUNT_BY_GUID);
        stmt2->setUInt32(0, newAccountId);
        stmt2->setUInt32(1, targetGuid.GetCounter());
        CharacterDatabase.DirectExecute(stmt2);

        sWorld->UpdateRealmCharCount(oldAccountId);
        sWorld->UpdateRealmCharCount(newAccountId);

        sCharacterCache->UpdateCharacterAccountId(targetGuid, newAccountId);

        handler->PSendSysMessage(LANG_CHANGEACCOUNT_SUCCESS, targetName.c_str(), accountName.c_str());
        return true;
    }


    static bool HandleFreedomPhaseCommand(ChatHandler* handler, char const* args) {
        if (!*args) {
            // TODO: Trinity string this
            handler->SendSysMessage("Please specify a phase number");
            handler->SetSentErrorMessage(true);
            return false;
        }
        uint32 phaseNumber = (uint32)atoul((char*)args);
        Player* source = handler->GetSession()->GetPlayer();
        sFreedomMgr->PlayerPhase(source, phaseNumber);
        char msg[80];
        sprintf(msg, "Your have been set to phase %u", phaseNumber);
        handler->SendSysMessage(msg);
        return true;
    }

    static bool HandleFreedomPhaseLockCommand(ChatHandler* handler, char const* args) {
        if (!*args) {
            // TODO: Trinity string this
            handler->SendSysMessage("Please specify on/off for the phase lock");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* source = handler->GetSession()->GetPlayer();
        sFreedomMgr->ClearPlayerPhase(source);
        std::string arg = strtok((char*)args, " ");
        if (boost::iequals(arg, "on")) {
            sFreedomMgr->SetPhaseLock(source, true);
            handler->SendSysMessage("Phase lock is now: |cffFF0000ON|r");
            return true;
        }
        if (boost::iequals(arg, "off")) {
            sFreedomMgr->SetPhaseLock(source, false);
            handler->SendSysMessage("Phase lock is now: |cff00FF00OFF|r");
        }
        return true;
    }

    static bool HandleFreedomPhaseClearCommand(ChatHandler* handler) {
        Player* source = handler->GetSession()->GetPlayer();
        sFreedomMgr->ClearPlayerPhase(source);
        handler->SendSysMessage("Your phases have been reset to default.");
        return true;
    }

    static bool HandleFreedomEnchantCommand(ChatHandler* handler, uint32 enchantId) {
        Player* source = handler->GetSession()->GetPlayer();

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchantId);
        if (!enchantEntry) {
            handler->PSendSysMessage("Could not find an enchantment for id: %u", enchantId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Item* mainHandItem = source->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        mainHandItem->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchantId, 0, 0);
        source->ApplyEnchantment(mainHandItem, PERM_ENCHANTMENT_SLOT, true);

        handler->PSendSysMessage("Applied enchant id '%u' to your main hand weapon.", enchantId);
        return true;
    }


    static bool HandleFreedomMountListCommand(ChatHandler* handler, Optional<std::string> mountName)
    {
        const MountDataContainer mountList = sFreedomMgr->GetMountContainer(handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
        uint64 count = 0;

        if (!mountName)
        {
            for (auto mountData : mountList)
            {
                handler->PSendSysMessage(FREEDOM_CMDI_MORPH_LIST_ITEM, mountData.displayId, mountData.name);
                count++;
            }
        }
        else
        {
            for (auto mountData : mountList)
            {
                if (boost::istarts_with(mountData.name, mountName.value()))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_MORPH_LIST_ITEM, mountData.displayId, mountData.name);
                    count++;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Mounts");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }

    static bool HandleFreedomMountAddCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage("Syntax: .freedom mount add $name $displayId [$playerName]");
            return true;
        }

        ArgumentTokenizer tokenizer(args);

        if (tokenizer.size() < 2)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_NOT_ENOUGH_PARAMS);
            handler->PSendSysMessage("Syntax: .freedom mount add $name $displayId [$playerName]");
            return true;
        }

        std::string mountName = tokenizer.TryGetParam<std::string>(0);
        uint32 displayId = tokenizer.TryGetParam<uint32>(1);
        Player* source = handler->GetSession()->GetPlayer();
        std::string targetNameArg = tokenizer.size() > 2 ? tokenizer.TryGetParam<std::string>(2) : "";
        std::string targetName;
        ObjectGuid targetGuid;

        if (!displayId)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_INVALID_ARGUMENT_X, "$displayId");
            return true;
        }

        if (!sCreatureDisplayInfoStore.HasRecord(displayId))
        {
            handler->SendSysMessage(LANG_NO_MOUNT);
            return true;
        }

        if (!handler->extractPlayerTarget(&targetNameArg[0], nullptr, &targetGuid, &targetName))
            return true;

        // Check if morph already exists
        const MountData* mountDataByName = sFreedomMgr->GetMountByName(targetGuid.GetCounter(), mountName);
        const MountData* mountDataByDisplayId = sFreedomMgr->GetMountByDisplayId(targetGuid.GetCounter(), displayId);

        if (mountDataByName)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_ALREADY_EXISTS, "Mount", mountName);
            return true;
        }

        if (mountDataByDisplayId)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_ALREADY_EXISTS, "Mount", displayId);
            return true;
        }

        // Create mount
        MountData newMountData;
        newMountData.name = mountName;
        newMountData.displayId = displayId;
        newMountData.gmBnetAccId = source->GetSession()->GetBattlenetAccountId();

        sFreedomMgr->AddMount(targetGuid.GetCounter(), newMountData);

        handler->PSendSysMessage("Mount (Name: %s, Display Id: %u) succesfully added to player '%s'.", mountName, displayId, targetName);
        return true;
    }

    static bool HandleFreedomMountDelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage("Syntax: .freedom mount delete $name/$displayId [$playerName]");
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string mountName = tokenizer.TryGetParam(0);
        uint32 displayId = tokenizer.TryGetParam<uint32>(0);
        std::string targetNameArg = tokenizer.size() > 1 ? tokenizer.TryGetParam(1) : "";
        std::string targetName;
        ObjectGuid targetGuid;

        if (!handler->extractPlayerTarget(&targetNameArg[0], nullptr, &targetGuid, &targetName))
            return true;

        // Check if morph actually exists
        const MountData* mountData = sFreedomMgr->GetMountByName(targetGuid.GetCounter(), mountName);

        if (displayId && !mountData) // get by displayId only if name search doesn't turn up anything
        {
            mountData = sFreedomMgr->GetMountByDisplayId(targetGuid.GetCounter(), displayId);

            if (!mountData)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Mount", displayId);
                return true;
            }
        }
        else if (!mountData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_NOT_FOUND, "Mount", mountName);
            return true;
        }

        mountName = mountData->name;
        displayId = mountData->displayId;

        sFreedomMgr->DeleteMountByName(targetGuid.GetCounter(), mountData->name);

        handler->PSendSysMessage("Mount (Name: %s, Display Id: %u) succesfully removed from player '%s'.", mountName, displayId, targetName);
        return true;
    }

    static bool HandleFreedomMountCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage("Syntax: .freedom mount $name/$displayId");
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        std::string mountName = tokenizer.TryGetParam(0);
        uint32 displayId = tokenizer.TryGetParam<uint32>(0);
        Player* source = handler->GetSession()->GetPlayer();

        // Check if morph actually exists
        const MountData* mountData = sFreedomMgr->GetMountByName(source->GetGUID().GetCounter(), mountName);

        if (displayId && !mountData) // get by displayId only if name search doesn't turn up anything
        {
            mountData = sFreedomMgr->GetMountByDisplayId(source->GetGUID().GetCounter(), displayId);

            if (!mountData)
            {
                handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_ID_NOT_FOUND, "Mount", displayId);
                return true;
            }
        }
        else if (!mountData)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_X_WITH_NAME_NOT_FOUND, "Mount", mountName);
            return true;
        }
                                                

        source->Mount(mountData->displayId);

        handler->PSendSysMessage("You have now mounted the mount '%s' (Display ID: %u).", mountData->name, mountData->displayId);
        return true;
    }

    static bool HandleFreedomArtifactAppearancesCommand(ChatHandler* handler)
    {
        std::vector<uint32> artiIds = {
            120978, 127829, 127830, 127857, 128288,128289, 128292, 128293,128306, 128402, 128403, 128476, 128479, 128808, 128819,
            128820, 128821, 128822, 128823, 128825, 128826, 128827, 128831, 128832, 128858,
            128859, 128860, 128861, 128862, 128866, 128867, 128868, 128869, 128870, 128872, 128873, 128908, 128910, 128911, 128934,
            128935, 128936, 128937, 128938, 128940, 128941, 128942, 128943, 129735, 129736,
            129737, 129738, 129752, 129753, 129754, 129755, 129899, 129967, 129968, 129969, 129970, 130162, 132560, 133948, 133958,
            133959, 134552, 134553, 134562, 136593, 136858, 137246, 139275, 139439,
            139468, 139555, 139556, 139621, 139671, 139672, 139706, 139891, 140199, 140200, 140652, 140656, 143840, 144395
        };

        for (uint32 artifactId : artiIds) {
            for (uint32 bonusId = 9; bonusId <= 32; bonusId++) {
                auto itemModAppearance = sDB2Manager.GetItemModifiedAppearance(artifactId, bonusId);
                if (itemModAppearance) {
                    handler->GetPlayer()->GetSession()->GetCollectionMgr()->AddItemAppearance(itemModAppearance);
                }
            }
        }

        return true;
    }

    static bool HandleFreedomListPetCommand(ChatHandler* handler, char const* args)
    {
        uint64 count = 0;

        if (!*args)
        {
            for (auto petData : sBattlePetSpeciesStore)
            {
                auto creatureTemplate = sObjectMgr->GetCreatureTemplate(petData->CreatureID);
                if (!creatureTemplate) {
                    continue;
                }
                std::string petName = creatureTemplate->Name;
                auto petLink = fmt::format("|cffffffff|Hbattlepet:{}:1:{}:100;10:10|h[{}]|h|r",
                    petData->ID, BattlePets::BattlePetMgr::GetDefaultPetQuality(petData->ID),
                    petName
                );

                handler->PSendSysMessage(FREEDOM_CMDI_BPET_LIST_ITEM, petData->ID, petLink);
                count++;

                if (count == 1000) {
                    break;
                }
            }
        }
        else
        {
            ArgumentTokenizer tokenizer(args);
            std::string name = tokenizer.TryGetParam(0);

            for (auto petData : sBattlePetSpeciesStore)
            {
                auto creatureTemplate = sObjectMgr->GetCreatureTemplate(petData->CreatureID);
                if (!creatureTemplate) {
                    continue;
                }

                std::string petName = creatureTemplate->Name;
                auto petLink = fmt::format("|cffffffff|Hbattlepet:{}:1:{}:100;10:10|h[{}]|h|r",
                    petData->ID, BattlePets::BattlePetMgr::GetDefaultPetQuality(petData->ID),
                    petName
                );
                if (boost::icontains(petName, name))
                {
                    handler->PSendSysMessage(FREEDOM_CMDI_BPET_LIST_ITEM, petData->ID, petLink);
                    count++;
                }

                if (count == 1000) {
                    break;
                }
            }
        }

        if (count == 0)
            handler->PSendSysMessage(FREEDOM_CMDI_X_NOT_FOUND, "Pets");
        else
            handler->PSendSysMessage(FREEDOM_CMDI_SEARCH_QUERY_RESULT, count);

        return true;
    }

    static bool HandleFreedomAddBPetCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->PSendSysMessage("You need to provide a pet id to add.");
            return true;
        }

        ArgumentTokenizer tokenizer(args);
        int petId = tokenizer.TryGetParam<uint32>(0);
        auto entry = sBattlePetSpeciesStore.LookupEntry(petId);
        if (!entry) {
            handler->PSendSysMessage("Could not find a pet with id: '%u'.", petId);
            return true;
        }

        handler->GetPlayer()->GetSession()->GetBattlePetMgr()->AddPet(entry->ID, BattlePets::BattlePetMgr::SelectPetDisplay(entry),
            BattlePets::BattlePetMgr::RollPetBreed(entry->ID), BattlePets::BattlePetMgr::GetDefaultPetQuality(entry->ID));

        return true;
    }
};

void AddSC_F_freedom_commandscript()
{
    new F_freedom_commandscript();
}
