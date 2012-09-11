#include "ScriptPCH.h"
#include "MapManager.h"

struct GOMove_Data
{
    GameObject* object;
    uint32 Entry, Phase, map;
    float x, y, z, o;
};
static std::map<uint64, GOMove_Data> GOMove;

class GOMove_commandscript : public CommandScript
{
public:
    GOMove_commandscript() : CommandScript("GOMove_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand GOMoveCommandTable[] =
        {
            { "gomove",			SEC_GAMEMASTER,		true,   &GOMove_Command,				"", NULL },
            { NULL,				0,					false,	NULL,							"", NULL }
        };
        return GOMoveCommandTable;
    }

    static bool GOMove_Command(ChatHandler* handler, const char* args)
    {
        if (!args)
            return false;

        char* ID_t = strtok((char*)args, " "); // change (char*)args incase of a crash
        if (!ID_t)
            return false;
        uint32 ID = atoi(ID_t);

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
                if (!GOMove[playerGUID].object)
                    session->SendNotification("No object selected");
                else
                {
                    GameObject* target = GOMove[playerGUID].object;
                    float x,y,z,o;
                    target->GetPosition(x,y,z,o);
                    uint32 p = target->GetPhaseMask();
                    switch(ID)
                    {
                    case DELET: DeleteObject(player, true);                         break;
                    case X: SpawnObject(player,player->GetPositionX(),y,z,o,p);      break;
                    case Y: SpawnObject(player,x,player->GetPositionY(),z,o,p);      break;
                    case Z: SpawnObject(player,x,y,player->GetPositionZ(),o,p);      break;
                    case O: SpawnObject(player,x,y,z,player->GetOrientation(),p);    break;
                    case GOTO: player->TeleportTo(target->GetMapId(), x,y,z,o);     break;
                    }
                }
            }
            else
            {
                switch(ID)
                {
                case TEST: session->SendAreaTriggerMessage(player->GetName());      break;
                case FACE: { float piper2 = M_PI/2; float multi = player->GetOrientation()/piper2; float multi_int = floor(multi); float new_ori = (multi-multi_int > 0.5f) ? (multi_int+1)*piper2 : multi_int*piper2; player->SetFacingTo(new_ori); } break;
                case RESPAWN:
                    {
                        if (!GOMove[playerGUID].Entry)
                        {
                            session->SendNotification("No objects used");
                            return true;
                        }
                        if (!MapManager::IsValidMapCoord(GOMove[playerGUID].map, GOMove[playerGUID].x, GOMove[playerGUID].y, GOMove[playerGUID].z))
                        {
                            ChatHandler(player).PSendSysMessage(LANG_INVALID_TARGET_COORD, GOMove[playerGUID].x, GOMove[playerGUID].y, GOMove[playerGUID].map);
                            ChatHandler(player).SetSentErrorMessage(true);
                            return true;
                        }
                        SpawnObject(player, GOMove[playerGUID].x, GOMove[playerGUID].y, GOMove[playerGUID].z, GOMove[playerGUID].o, GOMove[player->GetGUID()].Phase, GOMove[playerGUID].Entry, true);
                    } break;
                case SELECTNEAR:
                    {
                        GameObject* object = handler->GetNearbyGameObject();
                        if (!object)
                            session->SendNotification("No objects found");
                        else
                        {
                            GOMove[playerGUID].object = object;
                            session->SendAreaTriggerMessage("Selected %s", object->GetName());
                            SendUsed(player);
                        }
                    } break;
                }
            }
        }
        else if (ARG && ID >= SPAWN)
        {
            if (ID >= NORTH && ID <= PHASE)
            {
                if (!GOMove[playerGUID].object)
                    session->SendNotification("No object selected");
                else
                {
                    GameObject* target = GOMove[playerGUID].object;
                    float x,y,z,o;
                    target->GetPosition(x,y,z,o);
                    uint32 p = target->GetPhaseMask();
                    switch(ID)
                    {
                    case NORTH: SpawnObject(player,x+((float)ARG/100),y,z,o,p);                        break;
                    case EAST: SpawnObject(player,x,y-((float)ARG/100),z,o,p);                         break;
                    case SOUTH: SpawnObject(player,x-((float)ARG/100),y,z,o,p);                        break;
                    case WEST: SpawnObject(player,x,y+((float)ARG/100),z,o,p);                         break;
                    case NORTHEAST: SpawnObject(player,x+((float)ARG/100),y-((float)ARG/100),z,o,p);   break;
                    case SOUTHEAST: SpawnObject(player,x-((float)ARG/100),y-((float)ARG/100),z,o,p);   break;
                    case SOUTHWEST: SpawnObject(player,x-((float)ARG/100),y+((float)ARG/100),z,o,p);   break;
                    case NORTHWEST: SpawnObject(player,x+((float)ARG/100),y+((float)ARG/100),z,o,p);   break;
                    case UP: SpawnObject(player,x,y,z+((float)ARG/100),o,p);                           break;
                    case DOWN: SpawnObject(player,x,y,z-((float)ARG/100),o,p);                         break;
                    case RIGHT: SpawnObject(player,x,y,z,o-((float)ARG/100),p);                        break;
                    case LEFT: SpawnObject(player,x,y,z,o+((float)ARG/100),p);                         break;
                    case PHASE: SpawnObject(player,x,y,z,o,ARG);                                        break;
                    }
                }
            }
            else
            {
                switch(ID)
                {
                case SPAWN:
                    {
                        SpawnObject(player, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),  player->GetOrientation(), player->GetPhaseMaskForSpawn(), ARG, true);
                        if (GOMove[playerGUID].object)
                            SendUsed(player);
                    } break;
                }
            }
        }
        else
            return false;

        if (GOMove[playerGUID].object) // save last used
        {
            GOMove[playerGUID].Entry = GOMove[playerGUID].object->GetEntry();
            GOMove[playerGUID].Phase = GOMove[playerGUID].object->GetPhaseMask();
            GOMove[playerGUID].map = GOMove[playerGUID].object->GetMapId();
            GOMove[playerGUID].object->GetPosition(GOMove[playerGUID].x, GOMove[playerGUID].y, GOMove[playerGUID].z, GOMove[playerGUID].o);
        }
        return true;
    }

private:

    enum eEnums
    {
        TEST,
        SELECTNEAR,
        DELET,
        X,
        Y,
        Z,
        O,
        GOTO,
        RESPAWN,
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
    };

    static void SendUsed(Player* player) // Sends an addon message for used objects list
    {
        if (!player || !GOMove[player->GetGUID()].object)
            return;
        uint32 entry = GOMove[player->GetGUID()].object->GetEntry();
        if (! entry)
            return;
        std::string origName = GOMove[player->GetGUID()].object->GetName();
        std::string name;
        uint32 len = origName.length();
        if (len > 19)
        {
            name = origName.substr(0,17);
            name += "..";
        }
        else
            name = origName;
        WorldPacket data;
        char msg[250];
        snprintf(msg, 250, "GOMOVE %s %u", name.c_str(), entry);
        ChatHandler(player).FillMessageData(&data, CHAT_MSG_SYSTEM, LANG_ADDON, player->GetGUID(), msg);
        player->GetSession()->SendPacket(&data);
    }

    static void DeleteObject(Player* player, bool message = false)
    {
        if (!player || !GOMove[player->GetGUID()].object)
            return;

        GameObject* object = GOMove[player->GetGUID()].object;
        uint64 ownerGuid = object->GetOwnerGUID();
        if (ownerGuid)
        {
            Unit* owner = ObjectAccessor::GetUnit(*player, ownerGuid);
            if (!owner || !IS_PLAYER_GUID(ownerGuid))
            {
                ChatHandler(player).PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, GUID_LOPART(ownerGuid), object->GetGUIDLow());
                ChatHandler(player).SetSentErrorMessage(true);
                return;
            }
            owner->RemoveGameObject(object, false);
        }
        if (message)
            ChatHandler(player).PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, object->GetGUIDLow());
        object->SetRespawnTime(0);
        object->Delete();
        object->DeleteFromDB();
        GOMove[player->GetGUID()].object = NULL;
    }

    static void SpawnObject(Player* player,float x,float y,float z,float o,uint32 p, uint32 e = 0, bool message = false)
    {
        if (!player)
            return;

        if(!e) // move
        {
            if (!GOMove[player->GetGUID()].object)
                return;
            else
            {
                e = GOMove[player->GetGUID()].object->GetEntry();
                DeleteObject(player);
            }
        }

        if (p < 1)
            p = player->GetPhaseMaskForSpawn();

        const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(e);
        if (!objectInfo)
        {
            ChatHandler(player).PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, e);
            ChatHandler(player).SetSentErrorMessage(true);
            return;
        }
        if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
        {
            sLog->outError(LOG_FILTER_SQL, "Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.", e, objectInfo->type, objectInfo->displayId);
            ChatHandler(player).PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA, e);
            ChatHandler(player).SetSentErrorMessage(true);
            return;
        }
        Map* map = player->GetMap();
        GameObject* object = new GameObject;
        uint32 guidLow = sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT);
        if (!object->Create(guidLow, objectInfo->entry, map, p, x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
        {
            delete object;
            return;
        }
        object->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), p);
        if (!object->LoadGameObjectFromDB(guidLow, map))
        {
            delete object;
            return;
        }
        sObjectMgr->AddGameobjectToGrid(guidLow, sObjectMgr->GetGOData(guidLow));
        if(message)
            ChatHandler(player).PSendSysMessage(LANG_GAMEOBJECT_ADD, e, objectInfo->name.c_str(), guidLow, x, y, z);

        GOMove[player->GetGUID()].object = object;
        return;
    }
};

void AddSC_GOMove_commandscript()
{
    new GOMove_commandscript();
}
