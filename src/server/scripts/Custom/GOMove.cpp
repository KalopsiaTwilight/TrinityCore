#include "ScriptPCH.h"
#include "MapManager.h"
#include "Language.h"

/*
struct GOMove_Data
{
std::vector<uint32> objectList;
uint32 Entry, Phase, map;
float x, y, z, o;
};
static std::map<uint64, GOMove_Data> GOMove;
*/

UNORDERED_MAP<uint64, uint32> SpawnQue;

class GOMove_commandscript : public CommandScript
{
public:
    GOMove_commandscript() : CommandScript("GOMove_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand GOMoveCommandTable[] =
        {
            { "gomove",			SEC_GAMEMASTER,		false,   &GOMove_Command,				"", NULL },
            { NULL,				0,					false,	NULL,							"", NULL }
        };
        return GOMoveCommandTable;
    }

    static bool GOMove_Command(ChatHandler* handler, const char* args)
    {
        if (!args)
            return false;

        char* ID_t = strtok((char*)args, " ");
        if (!ID_t)
            return false;
        uint32 ID = atoi(ID_t);

        char* objectGUIDLow_t = strtok(NULL, " ");
        uint32 objectGUIDLow = 0;
        if (objectGUIDLow_t)
            objectGUIDLow = atoi(objectGUIDLow_t);

        char* ARG_t = strtok(NULL, " ");
        uint32 ARG = 0;
        if (ARG_t)
            ARG = atoi(ARG_t);

        WorldSession* session = handler->GetSession();
        Player* player = session->GetPlayer();
        uint64 playerGUID = player->GetGUID();

        if (ID < SPAWN) // no args
        {
            if (ID >= DELET && ID <= GOTO)
            {
                GameObject* target = GetObjectByGUIDLow(objectGUIDLow, player);
                if (!target)
                    ChatHandler(player->GetSession()).PSendSysMessage("Object GUID: %u not found", objectGUIDLow);
                else
                {
                    float x,y,z,o;
                    target->GetPosition(x,y,z,o);
                    uint32 p = target->GetPhaseMask();
                    switch(ID)
                    {
                    case DELET: DeleteObject(player, objectGUIDLow, true); SendSelectionInfo(player, objectGUIDLow, false); break;
                    case X: SpawnObject(player,player->GetPositionX(),y,z,o,p,true,objectGUIDLow);      break;
                    case Y: SpawnObject(player,x,player->GetPositionY(),z,o,p,true,objectGUIDLow);      break;
                    case Z: SpawnObject(player,x,y,player->GetPositionZ(),o,p,true,objectGUIDLow);      break;
                    case O: SpawnObject(player,x,y,z,player->GetOrientation(),p,true,objectGUIDLow);    break;
                    case GOTO: player->TeleportTo(target->GetMapId(), x,y,z,o);                         break;
                    case RESPAWN:
                        {
                            if(GameObject* gob = SpawnObject(player,x,y,z,o,p,false,target->GetEntry()))
                                SendSelectionInfo(player, gob->GetGUIDLow(), true);
                        } break;
                    case GROUND:
                        {
                            float ground = target->GetMap()->GetHeight(target->GetPhaseMask(), x, y, MAX_HEIGHT);
                            if(ground != INVALID_HEIGHT)
                                SpawnObject(player,x,y,ground,o,p,true,objectGUIDLow);
                        } break;
                    }
                }
            }
            else
            {
                switch(ID)
                {
                case TEST: session->SendAreaTriggerMessage(player->GetName().c_str());      break;
                case FACE: { float piper2 = M_PI/2; float multi = player->GetOrientation()/piper2; float multi_int = floor(multi); float new_ori = (multi-multi_int > 0.5f) ? (multi_int+1)*piper2 : multi_int*piper2; player->SetFacingTo(new_ori); } break;
                case SELECTNEAR:
                    {
                        GameObject* object = handler->GetNearbyGameObject();
                        if (!object)
                            ChatHandler(player->GetSession()).PSendSysMessage("No objects found");
                        else
                        {
                            SendSelectionInfo(player, object->GetGUIDLow(), true);
                            session->SendAreaTriggerMessage("Selected %s", object->GetName().c_str());
                        }
                    } break;
                }
            }
        }
        else if (ARG && ID >= SPAWN)
        {
            if (ID >= NORTH && ID <= PHASE)
            {
                GameObject* target = GetObjectByGUIDLow(objectGUIDLow, player);
                if (!target)
                    ChatHandler(player->GetSession()).PSendSysMessage("Object GUID: %u not found", objectGUIDLow);
                else
                {
                    float x,y,z,o;
                    target->GetPosition(x,y,z,o);
                    uint32 p = target->GetPhaseMask();
                    switch(ID)
                    {
                    case NORTH: SpawnObject(player,x+((float)ARG/100),y,z,o,p,true,objectGUIDLow);                        break;
                    case EAST: SpawnObject(player,x,y-((float)ARG/100),z,o,p,true,objectGUIDLow);                         break;
                    case SOUTH: SpawnObject(player,x-((float)ARG/100),y,z,o,p,true,objectGUIDLow);                        break;
                    case WEST: SpawnObject(player,x,y+((float)ARG/100),z,o,p,true,objectGUIDLow);                         break;
                    case NORTHEAST: SpawnObject(player,x+((float)ARG/100),y-((float)ARG/100),z,o,p,true,objectGUIDLow);   break;
                    case SOUTHEAST: SpawnObject(player,x-((float)ARG/100),y-((float)ARG/100),z,o,p,true,objectGUIDLow);   break;
                    case SOUTHWEST: SpawnObject(player,x-((float)ARG/100),y+((float)ARG/100),z,o,p,true,objectGUIDLow);   break;
                    case NORTHWEST: SpawnObject(player,x+((float)ARG/100),y+((float)ARG/100),z,o,p,true,objectGUIDLow);   break;
                    case UP: SpawnObject(player,x,y,z+((float)ARG/100),o,p,true,objectGUIDLow);                           break;
                    case DOWN: SpawnObject(player,x,y,z-((float)ARG/100),o,p,true,objectGUIDLow);                         break;
                    case RIGHT: SpawnObject(player,x,y,z,o-((float)ARG/100),p,true,objectGUIDLow);                        break;
                    case LEFT: SpawnObject(player,x,y,z,o+((float)ARG/100),p,true,objectGUIDLow);                         break;
                    case PHASE: SpawnObject(player,x,y,z,o,ARG,true,objectGUIDLow);                                       break;
                    }
                }
            }
            else
            {
                switch(ID)
                {
                case SPAWN:
                    {
                        if (GameObject* object = SpawnObject(player, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),  player->GetOrientation(), player->GetPhaseMgr().GetPhaseMaskForSpawn(), false, ARG, true))
                        {
                            SpawnQue[player->GetGUID()] = ARG;
                            SendSelectionInfo(player, object->GetGUIDLow(), true);
                        }
                    } break;
                case SPAWNSPELL:
                    {
                        SpawnQue[player->GetGUID()] = ARG;
                    } break;
                case SELECTALLNEAR:
                    { 
                        if (ARG > 5000)
                            ARG = 5000;

                        QueryResult result = WorldDatabase.PQuery("SELECT guid, (POW(position_x - '%f', 2) + POW(position_y - '%f', 2) + POW(position_z - '%f', 2)) AS order_ FROM gameobject WHERE map = '%u' AND position_x BETWEEN '%f'-'%u' AND '%f'+'%u' AND position_y BETWEEN '%f'-'%u' AND '%f'+'%u' AND position_z BETWEEN '%f'-'%u' AND '%f'+'%u' ORDER BY order_ ASC LIMIT 100",
                            player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), player->GetPositionX(), ARG, player->GetPositionX(), ARG, player->GetPositionY(), ARG, player->GetPositionY(), ARG, player->GetPositionZ(), ARG, player->GetPositionZ(), ARG);

                        if (result)
                        {
                            do
                            {
                                Field* fields   = result->Fetch();
                                uint32 guidLow     = fields[0].GetUInt32();

                                if (GameObject* object = GetObjectByGUIDLow(guidLow, player))
                                {
                                    SendSelectionInfo(player, guidLow, true);
                                }
                            }
                            while (result->NextRow());
                        }
                    } break;
                }
            }
        }
        else
            return false;
        return true;
    }

    enum eEnums
    {
        TEST,
        SELECTNEAR,
        DELET,
        X,
        Y,
        Z,
        O,
        GROUND,
        RESPAWN,
        GOTO,
        FACE,

        SPAWN,
        NORTH,
        EAST,
        SOUTH,
        WEST,
        NORTHEAST,
        NORTHWEST,
        SOUTHEAST,
        SOUTHWEST,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        PHASE,
        SELECTALLNEAR,
        SPAWNSPELL,
    };

    static GameObject* GetObjectByGUIDLow(uint32 guidLow, Player* player)
    {
        if (GameObjectData const* gameObjectData = sObjectMgr->GetGOData(guidLow))
            if (GameObject* object = ChatHandler(player->GetSession()).GetObjectGlobalyWithGuidOrNearWithDbGuid(guidLow, gameObjectData->id))
                if(object->GetMapId() == ChatHandler(player->GetSession()).GetSession()->GetPlayer()->GetMapId()) // cant move objects on different maps
                    return object;
        return NULL;
    }

    static void SendSelectionInfo(Player* player, uint32 guidLow, bool add) // Sends an addon message for selected objects list
    {
        if (!player || !guidLow)
            return;

        std::ostringstream ss;
        if (!add)
            ss << "GOMOVE REMOVE "<< guidLow <<"  0";
        else
        {
            GameObject* object = GetObjectByGUIDLow(guidLow, player);
            if(!object)
                return;
            ss <<"GOMOVE ADD "<< guidLow <<" "<< object->GetName() <<" "<< object->GetEntry();
        }

        WorldPacket data;
        ChatHandler(player->GetSession()).FillMessageData(&data, CHAT_MSG_SYSTEM, LANG_ADDON, player->GetGUID(), ss.str().c_str());
        player->GetSession()->SendPacket(&data);
    }

    static void DeleteObject(Player* player, uint32 guidLow, bool message = false)
    {
        if (!player || !guidLow)
            return;
        GameObject* object = GetObjectByGUIDLow(guidLow, player);
        if (!object)
            return;

        uint64 ownerGuid = object->GetOwnerGUID();
        if (ownerGuid)
        {
            Unit* owner = ObjectAccessor::GetUnit(*player, ownerGuid);
            if (!owner || !IS_PLAYER_GUID(ownerGuid))
            {
                ChatHandler(player->GetSession()).PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, GUID_LOPART(ownerGuid), object->GetGUIDLow());
                ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                return;
            }
            owner->RemoveGameObject(object, false);
        }
        if (message)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, object->GetGUIDLow());
        object->SetRespawnTime(0);
        object->Delete();
        object->DeleteFromDB();
    }

    static GameObject* SpawnObject(Player* player,float x,float y,float z,float o,uint32 p, bool move, uint32 e, bool message = false)
    {
        // e = !move && entry or move && guid (entryorguid)
        if (!player || !e)
            return NULL;

        uint32 oldGuidLow = 0;
        if (move)
        {
            oldGuidLow = e;
            GameObject* object = GetObjectByGUIDLow(oldGuidLow, player);
            if (!object)
                return NULL;
            e = object->GetEntry();
            DeleteObject(player, oldGuidLow);
        }

        if (p < 1)
            p = player->GetPhaseMgr().GetPhaseMaskForSpawn();

        const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(e);
        if (!objectInfo)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, e);
            ChatHandler(player->GetSession()).SetSentErrorMessage(true);
            return NULL;
        }
        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
        {
            sLog->outError(LOG_FILTER_GENERAL, "Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.", e, objectInfo->type, objectInfo->displayId);
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA, e);
            ChatHandler(player->GetSession()).SetSentErrorMessage(true);
            return NULL;
        }
        Map* map = player->GetMap();
        GameObject* object = new GameObject;
        uint32 guidLow = sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT);
        if (!object->Create(guidLow, objectInfo->entry, map, p, x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
        {
            delete object;
            return NULL;
        }
        object->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), p);
        if (!object->LoadGameObjectFromDB(guidLow, map))
        {
            delete object;
            return NULL;
        }
        sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGOData(guidLow));
        if (message)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_GAMEOBJECT_ADD, e, objectInfo->name.c_str(), guidLow, x, y, z);

        if (move) // Swap objects
        {
            std::ostringstream ss;
            ss <<"GOMOVE SWAP "<< oldGuidLow <<"  "<< object->GetGUIDLow();
            WorldPacket data;
            ChatHandler(player->GetSession()).FillMessageData(&data, CHAT_MSG_SYSTEM, LANG_ADDON, player->GetGUID(), ss.str().c_str());
            player->GetSession()->SendPacket(&data);
        }

        return object;
    }
};

// possible spells:
// 27651, 897

class spell_place_GOMove : public SpellScriptLoader
{
public:
    spell_place_GOMove() : SpellScriptLoader("spell_place_GOMove") { }

    class spell_place_GOMoveSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_place_GOMoveSpellScript);

        void HandleAfterCast()
        {
            if(GetCaster()->GetTypeId() != TYPEID_PLAYER)
                return;
            Player* player = GetCaster()->ToPlayer();
            WorldLocation summonPos = *GetExplTargetDest();
            if(SpawnQue.find(player->GetGUID()) != SpawnQue.end())
            {
                if(GameObject* object = GOMove_commandscript::SpawnObject(player, summonPos.GetPositionX(), summonPos.GetPositionY(), summonPos.GetPositionZ(), player->GetOrientation(), player->GetPhaseMgr().GetPhaseMaskForSpawn(), false, SpawnQue[player->GetGUID()], true))
                    GOMove_commandscript::SendSelectionInfo(player, object->GetGUIDLow(), true);
            }
        }

        void Register()
        {
            AfterCast += SpellCastFn(spell_place_GOMoveSpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_place_GOMoveSpellScript();
    }
};

void AddSC_GOMove_commandscript()
{
    new GOMove_commandscript();
    new spell_place_GOMove();
}