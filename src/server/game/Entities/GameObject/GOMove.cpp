/*
GOMove By Rochet2
Original idea by Mordred

http://rochet2.github.io/
*/

#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include "Chat.h"
#include "GameObject.h"
#include "Language.h"
#include "Map.h"
#include "MapManager.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Position.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "GOMove.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "DB2Stores.h"
#include "ObjectAccessor.h"
#include "FreedomMgr.h"


GameObjectStore GOMove::Store;

void GOMove::SendAddonMessage(Player * player, const char * msg)
{
    if (!player || !msg)
        return;

    player->WhisperAddon(msg, "GOMOVE", player);
}

GameObject * GOMove::GetGameObject(Player * player, ObjectGuid::LowType lowguid)
{
    if (GameObjectData const* data = sObjectMgr->GetGOData(lowguid))
        return ChatHandler(player->GetSession()).GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid, data->id);
    return nullptr;
}

void GOMove::SendAdd(Player * player, ObjectGuid::LowType lowguid)
{
    GameObjectData const* data = sObjectMgr->GetGOData(lowguid);
    if (!data)
        return;
    GameObjectTemplate const* temp = sObjectMgr->GetGameObjectTemplate(data->id);
    if (!temp)
        return;
    char msg[1024];
    snprintf(msg, 1024, "ADD|%s|%s|%u", std::to_string(lowguid).c_str(), temp->name.c_str(), data->id);
    SendAddonMessage(player, msg);
}

void GOMove::SendRemove(Player * player, ObjectGuid::LowType lowguid)
{
    char msg[1024];
    snprintf(msg, 1024, "REMOVE|%s||0", std::to_string(lowguid).c_str());

    SendAddonMessage(player, msg);
}

void GOMove::DeleteGameObject(GameObject * object)
{
    if (!object)
        return;

    // copy paste .gob del command
    ObjectGuid ownerGuid = object->GetOwnerGUID();
    if (!ownerGuid.IsEmpty())
    {
        Unit* owner = ObjectAccessor::GetUnit(*object, ownerGuid);
        if (!owner || !ownerGuid.IsPlayer())
            return;

        owner->RemoveGameObject(object, false);
    }

    object->SetRespawnTime(0);                                 // not save respawn time
    object->Delete();
    object->DeleteFromDB();
}

GameObject * GOMove::SpawnGameObject(Player* player, float x, float y, float z, float o, uint32 p, uint32 entry)
{
    if (!player || !entry)
        return nullptr;

    if (!MapManager::IsValidMapCoord(player->GetMapId(), x, y, z))
        return nullptr;

    // copy paste .gob add command

    const GameObjectTemplate* objectInfo = sObjectMgr->GetGameObjectTemplate(entry);

    if (!objectInfo)
        return nullptr;

    if (objectInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(objectInfo->displayId))
        return nullptr;

    Map* map = player->GetMap();

    ChatHandler* handler;
    if (auto extraData = sFreedomMgr->GetGameObjectTemplateExtraData(entry))
    {
        if (extraData->disabled)
        {
            handler->PSendSysMessage(FREEDOM_CMDE_GAMEOBJECT_IS_BLACKLISTED);
            return nullptr;
        }
    }

    GameObject* object = sFreedomMgr->GameObjectCreate(player, objectInfo);

    if (!object)
    {
        delete object;
        return nullptr;
    }

    ObjectGuid::LowType guidLow = object->GetSpawnId();
    if (object)
        SendAdd(player, guidLow);
    return object;
}

GameObject * GOMove::MoveGameObject(Player* player, float x, float y, float z, float o, uint32 /*p*/, ObjectGuid::LowType lowguid)
{
    if (!player)
        return nullptr;
    GameObject* object = nullptr;
    if (GameObjectData const* data = sObjectMgr->GetGOData(lowguid))
        object = ChatHandler(player->GetSession()).GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid, data->id);
    if (!object)
    {
        SendRemove(player, lowguid);
        return nullptr;
    }

    Map* map = object->GetMap();
    if (!MapManager::IsValidMapCoord(object->GetMapId(), x, y, z))
        return nullptr;

    // copy paste .gob move command
    // copy paste .gob turn command
    /*object->Relocate(x, y, z, o);
    object->RelocateStationaryPosition(x, y, z, o);
    object->SetWorldRotationAngles(o, 0.0f, 0.0f);
    object->UpdateModelPosition();
    object->UpdateObjectVisibility();
    object->SaveToDB();*/

    sFreedomMgr->GameObjectMove(object, x, y, z, o);
    sFreedomMgr->GameObjectSetModifyHistory(object, player);
    sFreedomMgr->SaveGameObject(object);
    object = sFreedomMgr->GameObjectRefresh(object);

    // Generate a completely new spawn with new guid
    // 3.3.5a client caches recently deleted objects and brings them back to life
    // when CreateObject block for this guid is received again
    // however it entirely skips parsing that block and only uses already known location
    object->Delete();

    object = new GameObject();
    if (!object->LoadGameObjectFromDB(lowguid, map))
    {
        delete object;
        SendRemove(player, lowguid);
        return nullptr;
    }

    // copy paste from .gob phase command
    // object->SetPhaseMask(p, true);
    // object->SaveToDB();

    return object;
}

GameObject * GOMove::RotateGameObject(Player* player, float deg, uint8 a, ObjectGuid::LowType lowguid)
{
    if (!player)
        return nullptr;
    GameObject* object = nullptr;
    if (GameObjectData const* data = sObjectMgr->GetGOData(lowguid))
        object = ChatHandler(player->GetSession()).GetObjectGlobalyWithGuidOrNearWithDbGuid(lowguid, data->id);
    if (!object)
    {
        SendRemove(player, lowguid);
        return nullptr;
    }

    // Set axial angle, save and refresh
    // 1-Roll, 2-Pitch, 3-Yaw
    if (a == 1)
        sFreedomMgr->GameObjectRotateSingleAxis(object, deg, AXIS_ROLL, true);
    if (a == 2)
        sFreedomMgr->GameObjectRotateSingleAxis(object, deg, AXIS_PITCH, true);
    if (a == 3)
        sFreedomMgr->GameObjectRotateSingleAxis(object, deg, AXIS_YAW, true);

    sFreedomMgr->GameObjectSetModifyHistory(object, player);
    sFreedomMgr->SaveGameObject(object);
    object = sFreedomMgr->GameObjectRefresh(object);

    // Generate a completely new spawn with new guid
    // 3.3.5a client caches recently deleted objects and brings them back to life
    // when CreateObject block for this guid is received again
    // however it entirely skips parsing that block and only uses already known location
    object->Delete();

    Map* map = object->GetMap();
    object = new GameObject();
    if (!object->LoadGameObjectFromDB(lowguid, map))
    {
        delete object;
        SendRemove(player, lowguid);
        return nullptr;
    }

    // copy paste from .gob phase command
    // object->SetPhaseMask(p, true);
    // object->SaveToDB();

    return object;
}

void GameObjectStore::SpawnQueAdd(ObjectGuid const & guid, uint32 entry)
{
    WriteGuard lock(_objectsToSpawnLock);
    objectsToSpawn[guid] = entry;
}

void GameObjectStore::SpawnQueRem(ObjectGuid const & guid)
{
    WriteGuard lock(_objectsToSpawnLock);
    objectsToSpawn.erase(guid);
}

uint32 GameObjectStore::SpawnQueGet(ObjectGuid const & guid)
{
    WriteGuard lock(_objectsToSpawnLock);
    auto it = objectsToSpawn.find(guid);
    if (it != objectsToSpawn.end())
        return it->second;
    return 0;
}

std::list<GameObject*> GOMove::GetNearbyGameObjects(Player* player, float range)
{
    float x, y, z;
    player->GetPosition(x, y, z);

    std::list<GameObject*> objects;
    Trinity::GameObjectInRangeCheck check(x, y, z, range);
    Trinity::GameObjectListSearcher<Trinity::GameObjectInRangeCheck> searcher(player, objects, check);
    player->VisitNearbyGridObject(SIZE_OF_GRIDS, searcher);
    return objects;
}