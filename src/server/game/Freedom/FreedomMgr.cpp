#include "AccountMgr.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "MovementPackets.h"
#include "MoveSpline.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "Transport.h"
#include "World.h"
#include "Unit.h"
#include "Log.h"

#include "FreedomMgr.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>
#include <G3D/Quat.h>
#include "QueryPackets.h"
#include "CreatureGroups.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"
#include <CharacterCache.h>
#include "ObjectAccessor.h"

FreedomMgr::FreedomMgr()
{
    _phaseListStore =
    {
        {1, 169},
        {2, 170},
        {4, 171},
        {8, 172},
        {16, 173},
        {32, 174},
        {64, 175},
        {128, 176},
        {256, 177}
    };
}

FreedomMgr::~FreedomMgr()
{
}

FreedomMgr* FreedomMgr::instance()
{
    static FreedomMgr instance;
    return &instance;
}

void FreedomMgr::LoadAllTables()
{
    uint32 oldMSTime = getMSTime();

    LoadPublicTeleports();
    LoadPrivateTeleports();
    LoadPublicSpells();
    LoadMorphs();
    LoadItemTemplateExtras();
    LoadGameObjectTemplateExtras();
    LoadGameObjectExtras();
    LoadCreatureExtras();
    LoadCreatureTemplateExtras();
    LoadCustomNpcs();
    LoadFormations();
    LoadNpcCasts();
    LoadAnimationMappings();
    LoadMounts();
    LoadAddonLoginMessages();

    TC_LOG_INFO("server.loading", ">> Loaded FreedomMgr tables in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

int FreedomMgr::GetPhaseMask(uint32 phaseId)
{
    for (auto phase : _phaseListStore)
    {
        if (phase.second == phaseId)
            return phase.first;
    }

    return -1;
}

int FreedomMgr::GetPhaseId(uint32 phaseMask)
{
    for (auto phase : _phaseListStore)
    {
        if (phase.first == phaseMask)
            return phase.second;
    }

    return -1;
}

bool FreedomMgr::IsValidPhaseId(uint32 phaseId)
{
    for (auto phase : _phaseListStore)
    {
        if (phase.second == phaseId)
            return true;
    }

    return false;
}

bool FreedomMgr::IsValidPhaseMask(uint32 phaseMask)
{
    for (auto phase : _phaseListStore)
    {
        if (phase.first == phaseMask)
            return true;
    }

    return false;
}

void FreedomMgr::CreaturePhase(Creature* creature, uint32 phaseMask)
{
    if (!phaseMask)
        phaseMask = 1;

    PhasingHandler::ResetPhaseShift(creature);

    for (int i = 1; i < 512; i = i << 1)
    {
        uint32 phase = phaseMask & i;

        if (phase)
            //creature->SetInPhase(GetPhaseId(phase), false, true);
            PhasingHandler::AddPhase(creature, phase, true);
    }

    //    creature->SetPhaseMask(phaseMask, true);

    _creatureExtraStore[creature->GetSpawnId()].phaseMask = phaseMask;
}

void FreedomMgr::GameObjectPhase(GameObject* go, uint32 phase)
{
    PhasingHandler::ResetPhaseShift(go);
    PhasingHandler::AddPhase(go, phase, true);
    go->SetDBPhase(phase);
    _gameObjectExtraStore[go->GetSpawnId()].phaseMask = phase;
}

void FreedomMgr::PlayerPhase(Player* player, uint32 phase)
{
    player->GetPhaseShift().ClearPhases();
    PhasingHandler::AddPhase(player, phase, true);


    FreedomDatabase.PExecute("INSERT INTO character_extra (guid,phase) VALUES ('%u',%u) ON DUPLICATE KEY UPDATE phase=%u", player->GetGUID().GetCounter(), phase, phase);
    _playerExtraDataStore[player->GetGUID().GetCounter()].phaseMask = phase;
}

void FreedomMgr::ClearPlayerPhase(Player* player)
{
    player->GetPhaseShift().ClearPhases();
    FreedomDatabase.PExecute("INSERT INTO character_extra (guid,phase) VALUES ('%u',-1) ON DUPLICATE KEY UPDATE phase=-1", player->GetGUID().GetCounter());
    _playerExtraDataStore[player->GetGUID().GetCounter()].phaseMask = 0;
}

uint32 FreedomMgr::GetPlayerPhase(Player* player)
{
    return _playerExtraDataStore[player->GetGUID().GetCounter()].phaseMask;
}

bool FreedomMgr::IsPhaseLocked(Player* player) {
    return _playerExtraDataStore[player->GetGUID().GetCounter()].phaseLock;
}

void FreedomMgr::SetPhaseLock(Player* player, bool val) {
    _playerExtraDataStore[player->GetGUID().GetCounter()].phaseLock = val;
}

void FreedomMgr::LoadCreatureExtras()
{
    // clear current storage
    _creatureExtraStore.clear();

    // guid, scale, id_creator_bnet, id_creator_player, id_modifier_bnet, id_modifier_player,
    // UNIX_TIMESTAMP(created), UNIX_TIMESTAMP(modified), phaseMask, displayLock, displayId,
    // nativeDisplayId, genderLock, gender, swim, gravity, fly
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_CREATUREEXTRA);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        CreatureExtraData data;
        uint64 guid = fields[0].GetUInt64();
        data.scale = fields[1].GetFloat();
        data.creatorBnetAccId = fields[2].GetUInt32();
        data.creatorPlayerId = fields[3].GetUInt64();
        data.modifierBnetAccId = fields[4].GetUInt32();
        data.modifierPlayerId = fields[5].GetUInt64();
        data.created = fields[6].GetUInt64();
        data.modified = fields[7].GetUInt64();
        data.phaseMask = fields[8].GetUInt32();
        data.displayLock = fields[9].GetBool();
        data.displayId = fields[10].GetUInt32();
        data.nativeDisplayId = fields[11].GetUInt32();
        data.genderLock = fields[12].GetBool();
        data.gender = fields[13].GetUInt8();
        data.swim = fields[14].GetBool();
        data.gravity = fields[15].GetBool();
        data.fly = fields[16].GetBool();

        _creatureExtraStore[guid] = data;
    } while (result->NextRow());
}

void FreedomMgr::LoadCreatureTemplateExtras()
{
    // clear current storage
    _creatureTemplateExtraStore.clear();

    // id_entry, disabled
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_CREATUREEXTRA_TEMPLATE);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        CreatureTemplateExtraData data;
        uint32 entry = fields[0].GetUInt32();
        data.disabled = fields[1].GetBool();

        _creatureTemplateExtraStore[entry] = data;
    } while (result->NextRow());
}

void FreedomMgr::CreatureSetEmote(Creature* creature, uint32 emoteId)
{
    uint32 spawnId = creature->GetSpawnId();
    auto addonData = &(sObjectMgr->_creatureAddonStore[spawnId]);
    addonData->emote = emoteId;
    creature->SetEmoteState(Emote(emoteId));
}

void FreedomMgr::CreatureSetMount(Creature* creature, uint32 mountId)
{
    uint32 spawnId = creature->GetSpawnId();
    auto addonData = &(sObjectMgr->_creatureAddonStore[spawnId]);
    addonData->mount = mountId;

    if (mountId)
        creature->Mount(mountId);
    else
        creature->Dismount();
}

void FreedomMgr::CreatureSetAuraToggle(Creature* creature, uint32 auraId, bool toggle)
{
    uint32 spawnId = creature->GetSpawnId();
    auto addonData = &(sObjectMgr->_creatureAddonStore[spawnId]);

    auto it = addonData->auras.begin();
    for (; it != addonData->auras.end(); it++)
    {
        if (*it == auraId && toggle) // refresh already existing aura
        {
            creature->AddAura(auraId, creature);
            return;
        }

        if (*it == auraId && !toggle) // we found auraId we want to remove
            break;
    }

    if (toggle)
    {
        creature->AddAura(auraId, creature);
        addonData->auras.push_back(auraId);
    }
    else if (it != addonData->auras.end())
    {
        creature->RemoveAura(auraId);
        addonData->auras.erase(it);
    }
}

void FreedomMgr::CreatureSetBytes1(Creature* creature, uint32 bytes1)
{
    uint32 spawnId = creature->GetSpawnId();
    auto addonData = &(sObjectMgr->_creatureAddonStore[spawnId]);
    addonData->bytes1 = bytes1;


    // TODO: Check this.
    // Copied from Creature::LoadCreaturesAddon like it seemed the original function had been
    if (bytes1)
    {
        // 0 StandState
        // 1 FreeTalentPoints   Pet only, so always 0 for default creature
        // 2 StandFlags
        // 3 StandMiscFlags
        creature->SetStandState(UnitStandStateType(bytes1 & 0xFF));
        creature->ReplaceAllVisFlags(UnitVisFlags((bytes1 >> 16) & 0xFF));
        creature->SetAnimTier(AnimTier((bytes1 >> 24) & 0xFF), false);

        //! Suspected correlation between UNIT_FIELD_BYTES_1, offset 3, value 0x2:
        //! If no inhabittype_fly (if no MovementFlag_DisableGravity or MovementFlag_CanFly flag found in sniffs)
        //! Check using InhabitType as movement flags are assigned dynamically
        //! basing on whether the creature is in air or not
        //! Set MovementFlag_Hover. Otherwise do nothing.
        if (creature->CanHover())
            creature->AddUnitMovementFlag(MOVEMENTFLAG_HOVER);
    }
    else
    {
        creature->SetStandState(UnitStandStateType(0));
        creature->ReplaceAllVisFlags(UnitVisFlags(0));
        creature->SetAnimTier(AnimTier(0), false);
    }
}

void FreedomMgr::CreatureSetBytes2(Creature* creature, uint32 bytes2)
{
    uint32 spawnId = creature->GetSpawnId();
    auto addonData = &(sObjectMgr->_creatureAddonStore[spawnId]);
    addonData->bytes2 = bytes2;

    // TODO: Check this.
    // Copied from Creature::LoadCreaturesAddon like it seemed the original function had been
    if (bytes2)
    {
        // 0 SheathState
        // 1 PvpFlags
        // 2 PetFlags           Pet only, so always 0 for default creature
        // 3 ShapeshiftForm     Must be determined/set by shapeshift spell/aura

        creature->SetSheath(SheathState(bytes2 & 0xFF));
        creature->ReplaceAllPvpFlags(UnitPVPStateFlags((bytes2 >> 8) & 0xFF));
        creature->ReplaceAllPetFlags(UNIT_PET_FLAG_NONE);
        creature->SetShapeshiftForm(FORM_NONE);
    }
    else
    {
        creature->SetSheath(SheathState(0));
        creature->ReplaceAllPvpFlags(UnitPVPStateFlags((0)));
        creature->ReplaceAllPetFlags(UNIT_PET_FLAG_NONE);
        creature->SetShapeshiftForm(FORM_NONE);
    }
}

void FreedomMgr::CreatureSetGravity(Creature* creature, bool toggle)
{
    _creatureExtraStore[creature->GetSpawnId()].gravity = toggle;
    creature->SetDisableGravity(!toggle);

    if (toggle)
    {
        if (!creature->IsInWater() || !creature->CanSwim())
            creature->GetMotionMaster()->MoveFall();
    }
}

void FreedomMgr::CreatureSetSwim(Creature* creature, bool toggle)
{
    _creatureExtraStore[creature->GetSpawnId()].swim = toggle;
    creature->SetSwim(toggle && creature->IsInWater());

    if (toggle)
    {
        if (!creature->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
            creature->SetUnitMovementFlags(MOVEMENTFLAG_SWIMMING);

        creature->SetSwim(creature->IsInWater() || CreatureCanFly(creature));
    }
    else
    {
        if (creature->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
            creature->RemoveUnitMovementFlag(MOVEMENTFLAG_SWIMMING);

        if (!CreatureCanFly(creature))
        {
            creature->SetSwim(false);

            if (creature->IsInWater())
                creature->GetMotionMaster()->MoveFall();
        }
    }
}

void FreedomMgr::CreatureSetFly(Creature* creature, bool toggle)
{
    _creatureExtraStore[creature->GetSpawnId()].fly = toggle;

    if (!creature->IsInWater())
    {
        creature->SetSwim(toggle);
    }

    // Just to be sure, send animation update, because commands such as .npc move will cancel it
    WorldPackets::Movement::MoveSplineSetFlag packet(toggle ? SMSG_MOVE_SPLINE_START_SWIM : SMSG_MOVE_SPLINE_STOP_SWIM);
    packet.MoverGUID = creature->GetGUID();
    creature->SendMessageToSet(packet.Write(), true);
}

void FreedomMgr::CreatureSetAnimKitId(Creature* creature, uint16 animKitId)
{
    uint32 spawnId = creature->GetSpawnId();
    auto addonData = &(sObjectMgr->_creatureAddonStore[spawnId]);
    addonData->aiAnimKit = animKitId;

    creature->SetAIAnimKitId(animKitId);
}

void FreedomMgr::CreatureSetModel(Creature* creature, uint32 displayId) {
    creature->SetDisplayId(displayId);
    creature->SetNativeDisplayId(displayId);

    _creatureExtraStore[creature->GetSpawnId()].displayLock = true;
    _creatureExtraStore[creature->GetSpawnId()].displayId = displayId;
    _creatureExtraStore[creature->GetSpawnId()].nativeDisplayId = displayId;
}

bool FreedomMgr::CreatureCanSwim(Creature const* creature)
{
    return _creatureExtraStore[creature->GetSpawnId()].swim;
}

bool FreedomMgr::CreatureCanWalk(Creature const* creature)
{
    // Todo: Check this. Based off Creature::UpdateMovementFlags since InhabitType seems to no longer exist.
    return creature->GetMovementTemplate().IsGroundAllowed();
    // return (creature->GetCreatureTemplate()->InhabitType & INHABIT_GROUND) != 0;
}

bool FreedomMgr::CreatureCanFly(Creature const* creature)
{
    auto it = _creatureExtraStore.find(creature->GetSpawnId());
    if (it == _creatureExtraStore.end())
    {
        // Todo: Check this. Based off Creature::UpdateMovementFlags since InhabitType seems to no longer exist.
        _creatureExtraStore[creature->GetSpawnId()].fly = creature->GetMovementTemplate().IsFlightAllowed();
    }

    return _creatureExtraStore[creature->GetSpawnId()].fly;
}

void FreedomMgr::SetCreatureTemplateExtraDisabledFlag(uint32 entryId, bool disabled)
{
    auto it = _creatureTemplateExtraStore.find(entryId);
    if (it == _creatureTemplateExtraStore.end())
        return;

    _creatureTemplateExtraStore[entryId].disabled = disabled;

    // DB update
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_UPD_CREATUREEXTRA_TEMPLATE);
    stmt->setBool(0, disabled);
    stmt->setUInt32(1, entryId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::SaveCreature(Creature* creature)
{
    creature->SaveToDB();

    // Save extra attached data if it exists
    auto it = _creatureExtraStore.find(creature->GetSpawnId());

    if (it != _creatureExtraStore.end())
    {
        int index = 0;
        CreatureExtraData data = it->second;
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_REP_CREATUREEXTRA);
        stmt->setUInt64(index++, creature->GetSpawnId());
        stmt->setFloat(index++, data.scale);
        stmt->setUInt32(index++, data.creatorBnetAccId);
        stmt->setUInt64(index++, data.creatorPlayerId);
        stmt->setUInt32(index++, data.modifierBnetAccId);
        stmt->setUInt64(index++, data.modifierPlayerId);
        stmt->setUInt64(index++, data.created);
        stmt->setUInt64(index++, data.modified);
        stmt->setUInt32(index++, data.phaseMask);
        stmt->setBool(index++, data.displayLock);
        stmt->setUInt32(index++, data.displayId);
        stmt->setUInt32(index++, data.nativeDisplayId);
        stmt->setBool(index++, data.genderLock);
        stmt->setUInt8(index++, data.gender);
        stmt->setBool(index++, data.swim);
        stmt->setBool(index++, data.gravity);
        stmt->setBool(index++, data.fly);

        FreedomDatabase.Execute(stmt);
    }
}

void FreedomMgr::CreatureSetModifyHistory(Creature* creature, Player* modifier, bool isCreate)
{
    if (!creature || !modifier)
        return;

    CreatureExtraData data = _creatureExtraStore[creature->GetSpawnId()];
    data.modifierBnetAccId = modifier->GetSession()->GetBattlenetAccountId();
    data.modifierPlayerId = modifier->GetGUID().GetCounter();
    data.modified = time(NULL);
    if (isCreate) {
        data.creatorBnetAccId = modifier->GetSession()->GetBattlenetAccountId();
        data.creatorPlayerId = modifier->GetGUID().GetCounter();
        data.created = time(NULL);
    }
    _creatureExtraStore[creature->GetSpawnId()] = data;
}

void FreedomMgr::CreatureMove(Creature* creature, float x, float y, float z, float o)
{
    if (!creature)
        return;

    // if (CreatureData const* data = sObjectMgr->GetCreatureData(creature->GetSpawnId()))
    // {
    //     const_cast<CreatureData*>(data)->posX = x;
    //     const_cast<CreatureData*>(data)->posY = y;
    //     const_cast<CreatureData*>(data)->posZ = z;
    //     const_cast<CreatureData*>(data)->orientation = o;
    // }
    // TODO: Check if this works
    creature->Relocate(x, y, z, o);

    //! If hovering, always increase our server-side Z position
    //! Client automatically projects correct position based on Z coord sent in monster move
    //! and UNIT_FIELD_HOVERHEIGHT sent in object updates
    if (creature->HasUnitMovementFlag(MOVEMENTFLAG_HOVER))
        z += creature->GetHoverOffset();
    creature->Relocate(x, y, z, o);
    //creature->GetMotionMaster()->Initialize();

    //if (creature->IsAlive())                            // dead creature will reset movement generator at respawn
    //{
    //    creature->setDeathState(JUST_DIED);
    //    creature->Respawn();
    //}

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_POSITION);

    stmt->setFloat(0, x);
    stmt->setFloat(1, y);
    stmt->setFloat(2, z);
    stmt->setFloat(3, o);
    stmt->setUInt64(4, creature->GetSpawnId());

    WorldDatabase.Execute(stmt);

    creature->SendTeleportPacket(creature->GetPosition());
}

void FreedomMgr::CreatureTurn(Creature* creature, float o)
{
    CreatureMove(creature, creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), o);
}

void FreedomMgr::CreatureScale(Creature* creature, float scale)
{
    if (!creature)
        return;

    float maxScale = sConfigMgr->GetFloatDefault("Freedom.Creature.MaxScale", 15.0f);
    float minScale = sConfigMgr->GetFloatDefault("Freedom.Creature.MinScale", 0.0f);

    if (scale < minScale)
        scale = minScale;
    if (scale > maxScale)
        scale = maxScale;

    creature->SetObjectScale(scale);
    _creatureExtraStore[creature->GetSpawnId()].scale = scale;
}

void FreedomMgr::CreatureDelete(Creature* creature)
{
    creature->CombatStop();
    creature->DeleteFromDB(creature->GetSpawnId());
    // TODO: This should already happen in DeleteFromDB, check this.
    creature->AddObjectToRemoveList();
    // Remove spawn from custom npc spawns
    for (auto it : _customNpcStore)
    {
        if (it.second.templateId == creature->GetEntry()) {
            std::vector<uint64> spawns = it.second.spawns;
            spawns.erase(std::remove(spawns.begin(), spawns.end(), creature->GetSpawnId()), spawns.end());
            it.second.spawns = spawns;
            _customNpcStore[it.first] = it.second;
            break;
        }
    }
}

Creature* FreedomMgr::CreatureCreate(Player* creator, CreatureTemplate const* creatureTemplate)
{
    uint32 entryId = creatureTemplate->Entry;
    Map* map = creator->GetMap();

    if (Transport* trans = dynamic_cast<Transport*>(creator->GetTransport()))
    {
        ObjectGuid::LowType guid = sObjectMgr->GenerateCreatureSpawnId();
        CreatureData& data = sObjectMgr->NewOrExistCreatureData(guid);
        data.spawnId = guid;
        data.spawnGroupData = sObjectMgr->GetDefaultSpawnGroup();
        data.id = entryId;
        data.spawnPoint.Relocate(creator->GetTransOffsetX(), creator->GetTransOffsetY(), creator->GetTransOffsetZ(), creator->GetTransOffsetO());
        if (Creature* creature = trans->CreateNPCPassenger(guid, &data))
        {
            creature->SaveToDB(trans->GetGOInfo()->moTransport.SpawnMap, { map->GetDifficultyID() });
            sObjectMgr->AddCreatureToGrid(&data);
            return creature;
        }
    }

    Creature* creature = Creature::CreateCreature(entryId, map, creator->GetPosition());
    if (!creature)
        return creature;

    PhasingHandler::InheritPhaseShift(creature, creator);
    creature->SaveToDB(map->GetId(), { map->GetDifficultyID() });

    ObjectGuid::LowType db_guid = creature->GetSpawnId();

    sFreedomMgr->CreatureScale(creature, creature->GetObjectScale());
    sFreedomMgr->CreatureSetFly(creature, creature->GetMovementTemplate().IsFlightAllowed());

    // To call _LoadGoods(); _LoadQuests(); CreateTrainerSpells()
    // current "creature" variable is deleted and created fresh new, otherwise old values might trigger asserts or cause undefined behavior
    creature->CleanupsBeforeDelete();
    delete creature;

    creature = Creature::CreateCreatureFromDB(db_guid, map, true, true);
    if (!creature)
        return creature;

    // Creation history and straight update
    CreatureExtraData data;
    data.scale = creatureTemplate->scale;
    data.creatorBnetAccId = creator->GetSession()->GetBattlenetAccountId();
    data.creatorPlayerId = creator->GetGUID().GetCounter();
    data.modifierBnetAccId = creator->GetSession()->GetBattlenetAccountId();
    data.modifierPlayerId = creator->GetGUID().GetCounter();
    data.created = time(NULL);
    data.modified = time(NULL);
    _creatureExtraStore[creature->GetSpawnId()] = data;

    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_REP_CREATUREEXTRA);
    stmt->setUInt64(index++, creature->GetSpawnId());
    stmt->setFloat(index++, data.scale);
    stmt->setUInt32(index++, data.creatorBnetAccId);
    stmt->setUInt64(index++, data.creatorPlayerId);
    stmt->setUInt32(index++, data.modifierBnetAccId);
    stmt->setUInt64(index++, data.modifierPlayerId);
    stmt->setUInt64(index++, data.created);
    stmt->setUInt64(index++, data.modified);
    stmt->setUInt64(index++, data.phaseMask);
    stmt->setBool(index++, data.displayLock);
    stmt->setUInt32(index++, data.displayId);
    stmt->setUInt32(index++, data.nativeDisplayId);
    stmt->setBool(index++, data.genderLock);
    stmt->setUInt8(index++, data.gender);
    stmt->setBool(index++, data.swim);
    stmt->setBool(index++, data.gravity);
    stmt->setBool(index++, data.fly);

    FreedomDatabase.Execute(stmt);

    sObjectMgr->AddCreatureToGrid(sObjectMgr->GetCreatureData(db_guid));
    return creature;
}

void FreedomMgr::CreatureRefresh(Creature* creature)
{
    if (!creature)
        return;

    // ObjectGuid::LowType guidLow = creature->GetSpawnId();
    Map* map = creature->GetMap();
    map->GetObjectsStore().Remove<Creature>(creature->GetGUID());
    creature->DestroyForNearbyPlayers();

    auto newGuidLow = map->GenerateLowGuid<HighGuid::Creature>();
    auto newObjectGuid = ObjectGuid::Create<HighGuid::Creature>(map->GetId(), creature->GetEntry(), newGuidLow);

    // TODO: Test this
    creature->SetGuid(newObjectGuid);
    // No longer works
    //creature->SetGuidValue(OBJECT_FIELD_GUID, newObjectGuid);
    //creature->SetPackGUID(newObjectGuid);
    map->GetObjectsStore().Insert(newObjectGuid, creature);
}

CreatureExtraData const* FreedomMgr::GetCreatureExtraData(uint64 guid)
{
    auto it = _creatureExtraStore.find(guid);

    if (it != _creatureExtraStore.end())
    {
        return &it->second;
    }
    else
    {
        return nullptr;
    }
}

CreatureTemplateExtraData const* FreedomMgr::GetCreatureTemplateExtraData(uint32 entry)
{
    auto it = _creatureTemplateExtraStore.find(entry);

    if (it != _creatureTemplateExtraStore.end())
    {
        return &it->second;
    }
    else
    {
        return nullptr;
    }
}

Creature* FreedomMgr::GetAnyCreature(ObjectGuid::LowType lowguid)
{
    auto data = sObjectMgr->GetCreatureData(lowguid);
    if (!data)
    {
        TC_LOG_DEBUG("freedom", "FreedomMgr::GetAnyCreature failed to find creatureData for GUID: " SZFMTD, lowguid);
        return nullptr;
    }

    auto objectGuid = ObjectGuid::Create<HighGuid::Creature>(data->mapId, data->id, lowguid);
    Map* map = sMapMgr->FindMap(data->mapId, 0);

    if (!map)
    {
        TC_LOG_DEBUG("freedom", "FreedomMgr::GetAnyCreature failed to find map %u for GUID: " SZFMTD, data->mapId, lowguid);
        return nullptr;
    }

    Creature* creature = map->GetCreature(objectGuid);

    // guid is DB guid of creature
    if (!creature)
    {
        auto bounds = map->GetCreatureBySpawnIdStore().equal_range(lowguid);
        if (bounds.first == bounds.second)
        {
            TC_LOG_DEBUG("freedom", "FreedomMgr::GetAnyCreature failed to find creature in spawnidstore on map %u for GUID: " SZFMTD, data->mapId, lowguid);
            return nullptr;
        }

        return bounds.first->second;
    }

    return creature;
}

Creature* FreedomMgr::GetAnyCreature(Map* map, ObjectGuid::LowType lowguid, uint32 entry)
{
    auto objectGuid = ObjectGuid::Create<HighGuid::Creature>(map->GetId(), entry, lowguid);

    Creature* creature = map->GetCreature(objectGuid);

    // guid is DB guid of creature
    if (!creature)
    {
        auto bounds = map->GetCreatureBySpawnIdStore().equal_range(lowguid);
        if (bounds.first == bounds.second)
            return nullptr;

        return bounds.first->second;
    }

    return creature;
}


Unit* FreedomMgr::GetAnyUnit(ObjectGuid::LowType guidLow)
{
    Creature* creature = GetAnyCreature(guidLow);
    if (creature) {
        return creature;
    }

    Player* player = ObjectAccessor::FindPlayerByLowGUID(guidLow);
    if (player) {
        return player;
    }

     return nullptr;
}

void FreedomMgr::SetCreatureSelectionForPlayer(ObjectGuid::LowType playerId, ObjectGuid::LowType creatureId)
{
    _playerExtraDataStore[playerId].selectedCreatureGuid = creatureId;
}

ObjectGuid::LowType FreedomMgr::GetSelectedCreatureGuidFromPlayer(ObjectGuid::LowType playerId)
{
    return _playerExtraDataStore[playerId].selectedCreatureGuid;
}

void FreedomMgr::LoadGameObjectExtras()
{
    // clear current storage
    _gameObjectExtraStore.clear();

    // guid, scale, id_creator_bnet, id_creator_player, id_modifier_bnet, id_modifier_player, UNIX_TIMESTAMP(created), UNIX_TIMESTAMP(modified), phaseMask
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_GAMEOBJECTEXTRA);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        GameObjectExtraData data;
        uint64 guid = fields[0].GetUInt64();
        data.scale = fields[1].GetFloat();
        data.creatorBnetAccId = fields[2].GetUInt32();
        data.creatorPlayerId = fields[3].GetUInt64();
        data.modifierBnetAccId = fields[4].GetUInt32();
        data.modifierPlayerId = fields[5].GetUInt64();
        data.created = fields[6].GetUInt64();
        data.modified = fields[7].GetUInt64();
        data.phaseMask = fields[8].GetUInt32();
        data.usesQuat = fields[9].GetBool();
        data.roll = fields[10].GetFloat();
        data.pitch = fields[11].GetFloat();
        data.yaw = fields[12].GetFloat();

        _gameObjectExtraStore[guid] = data;
    } while (result->NextRow());
}

void FreedomMgr::LoadGameObjectTemplateExtras()
{
    // clear current storage
    _gameObjectTemplateExtraStore.clear();

    // id_entry, disabled, model_name, model_type, is_default
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_GAMEOBJECTEXTRA_TEMPLATE);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        GameObjectTemplateExtraData data;
        uint32 entry = fields[0].GetUInt32();
        data.disabled = fields[1].GetBool();
        data.modelName = fields[2].GetString();
        data.modelType = fields[3].GetString();
        data.isDefault = fields[4].GetBool();

        _gameObjectTemplateExtraStore[entry] = data;
    } while (result->NextRow());
}

GameObjectTemplateExtraData const* FreedomMgr::GetGameObjectTemplateExtraData(uint32 entry)
{
    auto it = _gameObjectTemplateExtraStore.find(entry);

    if (it != _gameObjectTemplateExtraStore.end())
    {
        return &it->second;
    }

    return nullptr;
}

void FreedomMgr::SetGameobjectTemplateExtraDisabledFlag(uint32 entryId, bool disabled)
{
    auto it = _gameObjectTemplateExtraStore.find(entryId);
    if (it == _gameObjectTemplateExtraStore.end())
        return;

    _gameObjectTemplateExtraStore[entryId].disabled = disabled;

    // DB update
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_UPD_GAMEOBJECTEXTRA_TEMPLATE);
    stmt->setBool(0, disabled);
    stmt->setUInt32(1, entryId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::SaveGameObject(GameObject* go)
{
    go->SaveToDB();

    // Save extra attached data if it exists
    auto it = _gameObjectExtraStore.find(go->GetSpawnId());

    if (it != _gameObjectExtraStore.end())
    {
        int index = 0;
        GameObjectExtraData data = it->second;
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_REP_GAMEOBJECTEXTRA);
        stmt->setUInt64(index++, go->GetSpawnId());
        stmt->setFloat(index++, data.scale);
        stmt->setUInt32(index++, data.creatorBnetAccId);
        stmt->setUInt64(index++, data.creatorPlayerId);
        stmt->setUInt32(index++, data.modifierBnetAccId);
        stmt->setUInt64(index++, data.modifierPlayerId);
        stmt->setUInt64(index++, data.created);
        stmt->setUInt64(index++, data.modified);
        stmt->setUInt32(index++, data.phaseMask);
        stmt->setUInt8(index++, data.usesQuat);
        stmt->setFloat(index++, data.roll);
        stmt->setFloat(index++, data.pitch);
        stmt->setFloat(index++, data.yaw);

        FreedomDatabase.Execute(stmt);
    }
}

GameObject* FreedomMgr::GetAnyGameObject(Map* objMap, ObjectGuid::LowType lowguid, uint32 entry)
{
    GameObject* obj = nullptr;

    obj = objMap->GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(objMap->GetId(), entry, lowguid));

    // guid is DB guid of object
    if (!obj && sObjectMgr->GetGameObjectData(lowguid))
    {
        auto bounds = objMap->GetGameObjectBySpawnIdStore().equal_range(lowguid);
        if (bounds.first == bounds.second)
            return nullptr;

        return bounds.first->second;
    }

    if (!obj)
        objMap->GetGameObject(ObjectGuid::Create<HighGuid::Transport>(lowguid));

    return obj;
}

void FreedomMgr::SetGameobjectSelectionForPlayer(ObjectGuid::LowType playerId, ObjectGuid::LowType gameobjectId)
{
    _playerExtraDataStore[playerId].selectedGameobjectGuid = gameobjectId;
}

ObjectGuid::LowType FreedomMgr::GetSelectedGameobjectGuidFromPlayer(ObjectGuid::LowType playerId)
{
    return _playerExtraDataStore[playerId].selectedGameobjectGuid;
}

// https://github.com/TrinityCore/TrinityCore/commit/e68ff4186e685de00362b12bc0b5084a4d6065dd
GameObject* FreedomMgr::GameObjectRefresh(GameObject* go)
{
    ObjectGuid::LowType guidLow = go->GetSpawnId();
    Map* map = go->GetMap();
    go->Delete();
    go = GameObject::CreateGameObjectFromDB(guidLow, map);
    if (!go)
        return nullptr;

    return go;
}

void FreedomMgr::GameObjectMove(GameObject* go, float x, float y, float z, float o)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: GameObjectMove to x:'%f', y:'%f', z: '%f', o: '%f'", x, y, z, o);
    go->Relocate(x, y, z, o);

    if (GameObjectUsesQuatRotation(go))
    {
        auto extraData = _gameObjectExtraStore[go->GetSpawnId()];
        // Preserve roll and pitch, use new orientation (z angle) for yaw
        extraData.yaw = o;
        go->SetLocalRotationAngles(extraData.yaw, extraData.pitch, extraData.roll);
        _gameObjectExtraStore[go->GetSpawnId()] = extraData;
    }
    else
    {
        go->SetLocalRotationAngles(z, y, x);
    }
}

void FreedomMgr::GameObjectTurn(GameObject* go, float o)
{
    GameObjectMove(go, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), o);
}

void FreedomMgr::GameObjectRotate(GameObject* go, float deg_x, float deg_y, float deg_z, bool addDeg)
{
    auto extraData = &_gameObjectExtraStore[go->GetSpawnId()];
    bool firstTimeQuat = !extraData->usesQuat;
    extraData->usesQuat = true;

    float rad_x = deg_x * M_PI / 180;
    float rad_y = deg_y * M_PI / 180;
    float rad_z = deg_z * M_PI / 180;

    if (addDeg)
    {
        float roll, pitch, yaw;
        auto quat = go->GetWorldRotation();
        quat.toEulerAnglesZYX(yaw, pitch, roll);

        if (firstTimeQuat)
        {
            extraData->roll = rad_x;
            extraData->pitch = rad_y;
            extraData->yaw = rad_z + go->GetOrientation();
        }
        else
        {
            extraData->roll += rad_x;
            extraData->pitch += rad_y;
            extraData->yaw += rad_z;
        }
    }
    else
    {
        extraData->roll = rad_x;
        extraData->pitch = rad_y;
        extraData->yaw = rad_z;
    }

    TC_LOG_DEBUG("freedom", "FREEDOMMGR: GameObjectRotate to '%f'", extraData->yaw);

    go->Relocate(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), extraData->yaw);
    // TODO Check if localrotationangle works same as WorldRotationAngles previously
    go->SetLocalRotationAngles(extraData->yaw, extraData->pitch, extraData->roll);
}

void FreedomMgr::GameObjectRotateSingleAxis(GameObject* go, float deg, RotationAxis axis, bool addDeg)
{
    auto extraData = &_gameObjectExtraStore[go->GetSpawnId()];
    bool firstTimeQuat = !extraData->usesQuat;
    extraData->usesQuat = true;

    float rad = deg * M_PI / 180;

    if (firstTimeQuat)
    {
        extraData->roll = 0;
        extraData->pitch = 0;
        extraData->yaw = go->GetOrientation();
    }

    switch (axis)
    {
    case AXIS_ROLL:
        extraData->roll = (addDeg ? (extraData->roll + rad) : rad);
        break;
    case AXIS_PITCH:
        extraData->pitch = (addDeg ? (extraData->pitch + rad) : rad);
        break;
    case AXIS_YAW:
        extraData->yaw = (addDeg ? (extraData->yaw + rad) : rad);
        break;
    }
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: GameObjectRotateSingleAxis to '%f'", extraData->yaw);

    go->Relocate(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), extraData->yaw);
    go->SetLocalRotationAngles(extraData->yaw, extraData->pitch, extraData->roll);
}

void FreedomMgr::GetGameObjectEulerAnglesDeg(GameObject* go, float& deg_x, float& deg_y, float& deg_z)
{
    if (GameObjectUsesQuatRotation(go))
    {
        auto extraData = _gameObjectExtraStore[go->GetSpawnId()];

        // Convert to degrees and round inaccuracies to 2 decimal places
        deg_x = round((extraData.roll * 180 / M_PI) * 100) / 100.0f;
        deg_y = round((extraData.pitch * 180 / M_PI) * 100) / 100.0f;
        deg_z = round((extraData.yaw * 180 / M_PI) * 100) / 100.0f;
    }
    else
    {
        deg_x = 0.0f;
        deg_y = 0.0f;
        deg_z = round((go->GetOrientation() * 180 / M_PI) * 100) / 100.0f;
    }
}

void FreedomMgr::GetGameObjectEulerAnglesRad(GameObject* go, float& rad_x, float& rad_y, float& rad_z)
{
    if (GameObjectUsesQuatRotation(go))
    {
        auto extraData = _gameObjectExtraStore[go->GetSpawnId()];
        rad_x = extraData.roll;
        rad_y = extraData.pitch;
        rad_z = extraData.yaw;
    }
    else
    {
        rad_x = 0.0f;
        rad_y = 0.0f;
        rad_z = go->GetOrientation();
    }
}

bool FreedomMgr::GameObjectUsesQuatRotation(GameObject* go)
{
    // Save extra attached data if it exists
    auto it = _gameObjectExtraStore.find(go->GetSpawnId());

    if (it != _gameObjectExtraStore.end())
    {
        return (*it).second.usesQuat;
    }
    else
    {
        return false;
    }
}

void FreedomMgr::GameObjectScale(GameObject* go, float scale)
{
    if (!go)
        return;

    float maxScale = sConfigMgr->GetFloatDefault("Freedom.Gameobject.MaxScale", 15.0f);
    float minScale = sConfigMgr->GetFloatDefault("Freedom.Gameobject.MinScale", 0.0f);

    if (scale < minScale)
        scale = minScale;
    if (scale > maxScale)
        scale = maxScale;

    go->SetObjectScale(scale);
    _gameObjectExtraStore[go->GetSpawnId()].scale = scale;
}

void FreedomMgr::GameObjectDelete(GameObject* go)
{
    go->SetRespawnTime(0);
    go->Delete();
    go->DeleteFromDB(go->GetSpawnId());
}

void FreedomMgr::GameObjectSetModifyHistory(GameObject* go, Player* modifier)
{
    if (!go || !modifier)
        return;

    GameObjectExtraData data = _gameObjectExtraStore[go->GetSpawnId()];
    data.modifierBnetAccId = modifier->GetSession()->GetBattlenetAccountId();
    data.modifierPlayerId = modifier->GetGUID().GetCounter();
    data.modified = time(NULL);
    _gameObjectExtraStore[go->GetSpawnId()] = data;
}

GameObject* FreedomMgr::GameObjectCreate(Player* creator, GameObjectTemplate const* gobTemplate, uint32 spawnTimeSecs, float scale)
{
    if (gobTemplate->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gobTemplate->displayId))
    {
        // report to DB errors log as in loading case
        TC_LOG_ERROR("sql.sql", "Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.", gobTemplate->entry, gobTemplate->type, gobTemplate->displayId);
        return nullptr;
    }

    //float x, y, z, o;
    //creator->GetPosition(x, y, z, o);
    Player* player = creator->GetSession()->GetPlayer();
    Map* map = creator->GetMap();

    G3D::Quat rot = G3D::Matrix3::fromEulerAnglesZYX(creator->GetOrientation(), 0.f, 0.f);

    GameObject* object = new GameObject;
    object = GameObject::CreateGameObject(gobTemplate->entry, map, *player, QuaternionData(rot.x, rot.y, rot.z, rot.w), 255, GO_STATE_READY);
    if (!object)
    {
        delete object;
        return nullptr;
    }

    PhasingHandler::InheritPhaseShift(object, player);
    object->SetDBPhase(sFreedomMgr->GetPlayerPhase(player));

    if (spawnTimeSecs)
    {
        object->SetRespawnTime(spawnTimeSecs);
    }

    // fill the gameobject data and save to the db
    object->SaveToDB(map->GetId(), { map->GetDifficultyID() });
    ObjectGuid::LowType spawnId = object->GetSpawnId();

    sFreedomMgr->GameObjectScale(object, scale == -1.0f ? object->GetObjectScale() : scale);

    // delete the old object and do a clean load from DB with a fresh new GameObject instance.
    // this is required to avoid weird behavior and memory leaks
    delete object;

    object = GameObject::CreateGameObjectFromDB(spawnId, map);
    // this will generate a new guid if the object is in an instance
    if (!object)
    {
        delete object;
        return nullptr;
    }

    /// @todo is it really necessary to add both the real and DB table guid here ?
    sObjectMgr->AddGameobjectToGrid(ASSERT_NOTNULL(sObjectMgr->GetGameObjectData(spawnId)));

    // Creation history and straight update
    GameObjectExtraData data;
    data.scale = scale == -1.0f ? object->GetGOInfo()->size : scale;
    data.creatorBnetAccId = creator->GetSession()->GetBattlenetAccountId();
    data.creatorPlayerId = creator->GetGUID().GetCounter();
    data.modifierBnetAccId = creator->GetSession()->GetBattlenetAccountId();
    data.modifierPlayerId = creator->GetGUID().GetCounter();
    data.created = time(NULL);
    data.modified = time(NULL);

    _gameObjectExtraStore[spawnId] = data;

    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_REP_GAMEOBJECTEXTRA);
    stmt->setUInt64(index++, spawnId);
    stmt->setFloat(index++, data.scale);
    stmt->setUInt32(index++, data.creatorBnetAccId);
    stmt->setUInt64(index++, data.creatorPlayerId);
    stmt->setUInt32(index++, data.modifierBnetAccId);
    stmt->setUInt64(index++, data.modifierPlayerId);
    stmt->setUInt64(index++, data.created);
    stmt->setUInt64(index++, data.modified);
    stmt->setUInt64(index++, data.phaseMask);
    stmt->setUInt8(index++, data.usesQuat);
    stmt->setFloat(index++, data.roll);
    stmt->setFloat(index++, data.pitch);
    stmt->setFloat(index++, data.yaw);

    FreedomDatabase.Execute(stmt);

    return object;
}

GameObjectExtraData const* FreedomMgr::GetGameObjectExtraData(uint64 guid)
{
    auto it = _gameObjectExtraStore.find(guid);

    if (it != _gameObjectExtraStore.end())
    {
        return &it->second;
    }
    else
    {
        return nullptr;
    }
}

void FreedomMgr::LoadItemTemplateExtras()
{
    // clear current storage
    _itemTemplateExtraStore.clear();

    // entry_id, hidden
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_ITEMTEMPLATEEXTRA);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        ItemTemplateExtraData data;
        uint32 itemId = fields[0].GetUInt32();
        data.hidden = fields[1].GetBool();

        _itemTemplateExtraStore[itemId] = data;
    } while (result->NextRow());
}

ItemTemplateExtraData const* FreedomMgr::GetItemTemplateExtraById(uint32 itemId)
{
    auto it = _itemTemplateExtraStore.find(itemId);
    if (it != _itemTemplateExtraStore.end())
        return &it->second;
    else
        return nullptr;
}

void FreedomMgr::SetItemTemplateExtraHiddenFlag(uint32 itemId, bool hidden)
{
    auto it = _itemTemplateExtraStore.find(itemId);
    if (it == _itemTemplateExtraStore.end())
        return;

    _itemTemplateExtraStore[itemId].hidden = hidden;

    // DB update
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_UPD_ITEMTEMPLATEEXTRA);
    stmt->setBool(0, hidden);
    stmt->setUInt32(1, itemId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::RemoveHoverFromPlayer(Player* player)
{
    Unit* source_unit = player->ToUnit();
    float hoverHeight = source_unit->GetHoverOffset();

    source_unit->RemoveUnitMovementFlag(MOVEMENTFLAG_HOVER);

    if (hoverHeight)
    {
        float newZ = source_unit->GetPositionZ() - hoverHeight;
        source_unit->UpdateAllowedPositionZ(source_unit->GetPositionX(), source_unit->GetPositionY(), newZ);
        source_unit->UpdateHeight(newZ);
    }

    WorldPackets::Movement::MoveSetFlag packet(SMSG_MOVE_UNSET_HOVERING);
    packet.MoverGUID = source_unit->GetGUID();
    source_unit->SendMessageToSet(packet.Write(), true);

    // Force player on the ground after removing hover
    source_unit->SetUnitMovementFlags(MOVEMENTFLAG_FALLING);
    WorldPackets::Movement::MoveUpdate moveUpdate;
    moveUpdate.Status = &source_unit->m_movementInfo;
    source_unit->SendMessageToSet(moveUpdate.Write(), true);
}

void FreedomMgr::RemoveWaterwalkFromPlayer(Player* player)
{
    Unit* source_unit = player->ToUnit();
    source_unit->RemoveUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);

    WorldPackets::Movement::MoveSetFlag packet(SMSG_MOVE_SET_LAND_WALK);
    packet.MoverGUID = source_unit->GetGUID();
    source_unit->SendMessageToSet(packet.Write(), true);
}

void FreedomMgr::RemoveFlyFromPlayer(Player* player)
{
    Unit* source_unit = player->ToUnit();
    source_unit->RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_MASK_MOVING_FLY);

    //TODO: Used to be isLevitating, check if works the same
    if (!source_unit->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY))
        source_unit->SetFall(true);

    WorldPackets::Movement::MoveSetFlag packet(SMSG_MOVE_UNSET_CAN_FLY);
    packet.MoverGUID = source_unit->GetGUID();
    source_unit->SendMessageToSet(packet.Write(), true);
}

std::string FreedomMgr::GetMapName(uint32 mapId)
{
    const MapEntry* map = sMapStore.LookupEntry(mapId);

    if (map)
        return map->MapName[sWorld->GetDefaultDbcLocale()];
    else
        return "Unknown";
}

std::string FreedomMgr::GetChatLinkKey(std::string const& src, std::string type)
{
    if (src.empty())
        return "";

    std::string typePart = "|" + type + ":";
    std::string key = "";
    std::size_t pos = src.find(typePart); // find start pos of "|Hkeytype:" fragment first

    // Check for plain number first
    std::string plainNumber = src;
    boost::trim(plainNumber); // trim spaces
    plainNumber = plainNumber.substr(0, plainNumber.find(' ')); // get first token in case src had multiple ones
    if (isNumeric(plainNumber.c_str()))
        return plainNumber;

    // Do ChatLink check
    if (pos != std::string::npos)
    {
        auto it = src.begin();
        std::advance(it, pos + typePart.length());

        // if key part iteration ends without encountering ':' or '|',
        // then link was malformed and we return empty string later on
        for (; it != src.end(); it++)
        {
            char c = *it;

            if (c == ':' || c == '|')
                return key;

            key += c;
        }
    }

    return "";
}

std::string FreedomMgr::ToDateTimeString(time_t t)
{
    tm aTm;
    localtime_r(&t, &aTm);
    //       YYYY   year
    //       MM     month (2 digits 01-12)
    //       DD     day (2 digits 01-31)
    //       HH     hour (2 digits 00-23)
    //       MM     minutes (2 digits 00-59)
    //       SS     seconds (2 digits 00-59)
    char buf[20];
    snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d", aTm.tm_year + 1900, aTm.tm_mon + 1, aTm.tm_mday, aTm.tm_hour, aTm.tm_min, aTm.tm_sec);
    return std::string(buf);
}

std::string FreedomMgr::ToDateString(time_t t)
{
    tm aTm;
    localtime_r(&t, &aTm);
    //       YYYY   year
    //       MM     month (2 digits 01-12)
    //       DD     day (2 digits 01-31)
    //       HH     hour (2 digits 00-23)
    //       MM     minutes (2 digits 00-59)
    //       SS     seconds (2 digits 00-59)
    char buf[14];
    snprintf(buf, 14, "%04d-%02d-%02d", aTm.tm_year + 1900, aTm.tm_mon + 1, aTm.tm_mday);
    return std::string(buf);
}

PublicTeleData const* FreedomMgr::GetPublicTeleport(std::string const& name)
{
    auto it = std::find_if(_publicTeleStore.begin(), _publicTeleStore.end(), [name](PublicTeleData t)->bool { return boost::iequals(name, t.name); });

    if (it != _publicTeleStore.end())
        return &(*it);
    else
        return nullptr;
}

PublicTeleData const* FreedomMgr::GetFirstClosestPublicTeleport(std::string const& name)
{
    // Priority to exact match
    auto it = std::find_if(_publicTeleStore.begin(), _publicTeleStore.end(), [name](PublicTeleData t)->bool { return boost::iequals(name, t.name); });

    if (it != _publicTeleStore.end())
    {
        return &(*it);
    }
    else // Find first starts-with match, if exact match fails
    {
        it = std::find_if(_publicTeleStore.begin(), _publicTeleStore.end(), [name](PublicTeleData t)->bool { return boost::istarts_with(t.name, name); });

        if (it != _publicTeleStore.end())
            return &(*it);
        else
            return nullptr;
    }
}

void FreedomMgr::AddPublicTeleport(PublicTeleData const& data)
{
    _publicTeleStore.push_back(data);

    // name, position_x, position_y, position_z, orientation, map, id_bnet_gm
    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_PUBLIC_TELE);
    stmt->setString(index++, data.name);
    stmt->setFloat(index++, data.x);
    stmt->setFloat(index++, data.y);
    stmt->setFloat(index++, data.z);
    stmt->setFloat(index++, data.o);
    stmt->setUInt32(index++, data.map);
    stmt->setUInt32(index++, data.gmBnetAccId);
    stmt->setUInt32(index++, data.phaseId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::DeletePublicTeleport(std::string const& name)
{
    auto it = std::find_if(_publicTeleStore.begin(), _publicTeleStore.end(), [name](PublicTeleData t)->bool { return boost::iequals(name, t.name); });

    if (it != _publicTeleStore.end())
    {
        // WHERE name = ?
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_PUBLIC_TELE_NAME);
        stmt->setString(0, name);
        FreedomDatabase.Execute(stmt);

        _publicTeleStore.erase(it);
    }
}

void FreedomMgr::LoadPublicTeleports()
{
    // clear current storage
    _publicTeleStore.clear();

    // name, position_x, position_y, position_z, orientation, map, id_bnet_gm
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_PUBLIC_TELE);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        PublicTeleData data;
        data.name = fields[0].GetString();
        data.x = fields[1].GetFloat();
        data.y = fields[2].GetFloat();
        data.z = fields[3].GetFloat();
        data.o = fields[4].GetFloat();
        data.map = fields[5].GetUInt32();
        data.gmBnetAccId = fields[6].GetUInt32();
        data.phaseId = fields[7].GetUInt32();

        _publicTeleStore.push_back(data);
    } while (result->NextRow());
}

PrivateTeleData const* FreedomMgr::GetPrivateTeleport(uint32 bnetAccountId, std::string const& name)
{
    auto it = std::find_if(_privateTeleStore[bnetAccountId].begin(), _privateTeleStore[bnetAccountId].end(), [name](PrivateTeleData t)->bool { return boost::iequals(name, t.name); });

    if (it != _privateTeleStore[bnetAccountId].end())
        return &(*it);
    else
        return nullptr;
}

PrivateTeleData const* FreedomMgr::GetFirstClosestPrivateTeleport(uint32 bnetAccountId, std::string const& name)
{
    // Priority to exact match
    auto it = std::find_if(_privateTeleStore[bnetAccountId].begin(), _privateTeleStore[bnetAccountId].end(), [name](PrivateTeleData t)->bool { return boost::iequals(name, t.name); });

    if (it != _privateTeleStore[bnetAccountId].end())
    {
        return &(*it);
    }
    else // Find first starts-with match, if exact match fails
    {
        it = std::find_if(_privateTeleStore[bnetAccountId].begin(), _privateTeleStore[bnetAccountId].end(), [name](PrivateTeleData t)->bool { return boost::istarts_with(t.name, name); });

        if (it != _privateTeleStore[bnetAccountId].end())
            return &(*it);
        else
            return nullptr;
    }
}

void FreedomMgr::AddPrivateTeleport(uint32 bnetAccountId, PrivateTeleData const& data)
{
    _privateTeleStore[bnetAccountId].push_back(data);

    // position_x, position_y, position_z, orientation, map, name, id_bnet_gm
    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_PRIVATE_TELE);
    stmt->setString(index++, data.name);
    stmt->setFloat(index++, data.x);
    stmt->setFloat(index++, data.y);
    stmt->setFloat(index++, data.z);
    stmt->setFloat(index++, data.o);
    stmt->setUInt32(index++, data.map);
    stmt->setUInt32(index++, bnetAccountId);
    stmt->setUInt32(index++, data.phaseId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::DeletePrivateTeleport(uint32 bnetAccountId, std::string const& name)
{
    auto it = std::find_if(_privateTeleStore[bnetAccountId].begin(), _privateTeleStore[bnetAccountId].end(), [name](PrivateTeleData t)->bool { return boost::iequals(name, t.name); });

    if (it != _privateTeleStore[bnetAccountId].end())
    {
        // WHERE name = ? AND id_bnet_account = ?
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_PRIVATE_TELE_NAME);
        stmt->setString(0, name);
        stmt->setUInt32(1, bnetAccountId);
        FreedomDatabase.Execute(stmt);

        _privateTeleStore[bnetAccountId].erase(it);
    }
}

void FreedomMgr::LoadPrivateTeleports()
{
    // clear current storage
    _privateTeleStore.clear();

    // name, position_x, position_y, position_z, orientation, map, id_bnet_account
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_PRIVATE_TELE);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        PrivateTeleData data;
        data.name = fields[0].GetString();
        data.x = fields[1].GetFloat();
        data.y = fields[2].GetFloat();
        data.z = fields[3].GetFloat();
        data.o = fields[4].GetFloat();
        data.map = fields[5].GetUInt32();
        uint32 bnetAccId = fields[6].GetUInt32();
        data.phaseId = fields[7].GetUInt32();

        _privateTeleStore[bnetAccId].push_back(data);
    } while (result->NextRow());
}

PublicSpellData const* FreedomMgr::GetPublicSpell(uint32 spellId)
{
    auto it = _publicSpellStore.find(spellId);
    if (it != _publicSpellStore.end())
        return &it->second;
    else
        return nullptr;
}

void FreedomMgr::AddPublicSpell(uint32 spellId, PublicSpellData const& data)
{
    _publicSpellStore[spellId] = data;

    // spell_id, allow_targeting, name, id_bnet_gm
    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_PUBLIC_SPELL);
    stmt->setUInt32(index++, spellId);
    stmt->setUInt8(index++, data.targetOthers);
    stmt->setString(index++, data.name);
    stmt->setUInt32(index++, data.gmBnetAccId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::DeletePublicSpell(uint32 spellId)
{
    if (_publicSpellStore.find(spellId) != _publicSpellStore.end())
    {
        // WHERE spell_id = ?
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_PUBLIC_SPELL_ID);
        stmt->setUInt32(0, spellId);
        FreedomDatabase.Execute(stmt);

        _publicSpellStore.erase(spellId);
    }
}

void FreedomMgr::LoadPublicSpells()
{
    // clear current storage
    _publicSpellStore.clear();

    // spell_id, allow_targeting, name, id_bnet_gm
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_PUBLIC_SPELL);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        PublicSpellData data;
        uint32 spellId = fields[0].GetUInt32();
        data.targetOthers = fields[1].GetUInt8();
        data.name = fields[2].GetString();
        data.gmBnetAccId = fields[3].GetUInt32();

        _publicSpellStore[spellId] = data;
    } while (result->NextRow());
}

MorphData const* FreedomMgr::GetMorphByName(ObjectGuid::LowType playerId, std::string const& name)
{
    auto it = std::find_if(_playerExtraDataStore[playerId].morphDataStore.begin(), _playerExtraDataStore[playerId].morphDataStore.end(), [name](MorphData m)->bool { return boost::iequals(name, m.name); });

    if (it != _playerExtraDataStore[playerId].morphDataStore.end())
        return &(*it);
    else
        return nullptr;
}

MorphData const* FreedomMgr::GetMorphByDisplayId(ObjectGuid::LowType playerId, uint32 displayId)
{
    auto it = std::find_if(_playerExtraDataStore[playerId].morphDataStore.begin(), _playerExtraDataStore[playerId].morphDataStore.end(), [displayId](MorphData m)->bool { return m.displayId == displayId; });

    if (it != _playerExtraDataStore[playerId].morphDataStore.end())
        return &(*it);
    else
        return nullptr;
}

void FreedomMgr::AddMorph(ObjectGuid::LowType playerId, MorphData const& data)
{
    _playerExtraDataStore[playerId].morphDataStore.push_back(data);

    // guid, name, id_display, id_bnet_gm
    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_MORPHS);
    stmt->setUInt64(index++, playerId);
    stmt->setString(index++, data.name);
    stmt->setUInt32(index++, data.displayId);
    stmt->setUInt32(index++, data.gmBnetAccId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::DeleteMorphByName(ObjectGuid::LowType playerId, std::string const& name)
{
    auto it = std::find_if(_playerExtraDataStore[playerId].morphDataStore.begin(), _playerExtraDataStore[playerId].morphDataStore.end(), [name](MorphData m)->bool { return boost::iequals(name, m.name); });

    if (it != _playerExtraDataStore[playerId].morphDataStore.end())
    {
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_MORPHS_NAME);
        stmt->setString(0, name);
        stmt->setUInt64(1, playerId);
        FreedomDatabase.Execute(stmt);

        _playerExtraDataStore[playerId].morphDataStore.erase(it);
    }
}

void FreedomMgr::LoadMorphs()
{
    // clear current Morph storage for each player
    for (auto it : _playerExtraDataStore)
    {
        it.second.morphDataStore.clear();
    }

    // guid, name, id_display, id_bnet_gm
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_MORPHS);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        MorphData data;
        ObjectGuid::LowType charGuid = fields[0].GetUInt64();
        data.name = fields[1].GetString();
        data.displayId = fields[2].GetUInt32();
        data.gmBnetAccId = fields[3].GetUInt32();

        _playerExtraDataStore[charGuid].morphDataStore.push_back(data);
    } while (result->NextRow());
}

void FreedomMgr::LoadCustomNpcSpawn(uint32 templateId, ObjectGuid::LowType spawn)
{
    for (auto it : _customNpcStore)
    {
        if (it.second.templateId == templateId) {
            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Identified custom npc key '%s' for entry id '%u', adding spawn '%lu'", it.second.key.c_str(), templateId, spawn);
            it.second.spawns.push_back(spawn);
            _customNpcStore[it.first] = it.second;
            break;
        }
    }
}

void FreedomMgr::LoadCustomNpcs()
{
    // clear current storage
    _customNpcStore.clear();

    uint32 oldMSTime = getMSTime();
    QueryResult result = FreedomDatabase.Query("SELECT `Key`, Entry from custom_npcs");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 custom ncps. DB table `custom_npcs` is empty!");
        return;
    }
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[1].GetUInt32();

        CustomNpcData npcData;
        npcData.templateId = entry;
        npcData.key = fields[0].GetString();
        _customNpcStore[npcData.key] = npcData;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u custom npcs in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void FreedomMgr::CreateCustomNpcFromPlayer(Player* player, std::string const& key)
{
    std::shared_ptr<CreatureOutfit> co(new CreatureOutfit());

    uint32 outfitId = sConfigMgr->GetInt64Default("Freedom.CustomNpc.OutfitIdStart", 3000000001);
    if (!sObjectMgr->_creatureOutfitStore.empty()) {
        using pairtype = std::pair<uint32, std::shared_ptr<CreatureOutfit>>;
        outfitId = std::max_element(sObjectMgr->_creatureOutfitStore.begin(), sObjectMgr->_creatureOutfitStore.end(),
            [](pairtype a, pairtype b) { return a.second->id < b.second->id; })->second->id + 1;
    }

    co->id = outfitId;
    co->npcsoundsid = 0;
    co->race = player->GetRace();
    co->Class = player->GetClass();

    auto* maleModel = sDB2Manager.GetChrModel(co->race, GENDER_MALE);
    auto* femaleModel = sDB2Manager.GetChrModel(co->race, GENDER_FEMALE);

    co->gender = player->GetGender();
    switch (co->gender)
    {
    case GENDER_FEMALE: co->displayId = femaleModel->DisplayID; break;
    case GENDER_MALE:   co->displayId = maleModel->DisplayID; break;
    }

    co->SpellVisualKitID = 0;
    for (auto&& customization : player->m_playerData->Customizations)
    {
        co->Customizations.push_back(customization);
    }

    for (EquipmentSlots slot : CreatureOutfit::item_slots)
    {
        if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            int32 displayId = pItem->GetDisplayId(player);
            if (displayId == 0) {
                // weird case with items that don't have appearancemod of 0, just pick first appearance in the list
                for (ItemModifiedAppearanceEntry const* appearanceMod : sItemModifiedAppearanceStore)
                {
                    if ((uint32)appearanceMod->ItemID == pItem->GetEntry() && appearanceMod->OrderIndex == 0) {
                        displayId = sDB2Manager.GetItemDisplayId(pItem->GetEntry(), appearanceMod->ItemAppearanceModifierID);
                        break;
                    }
                }
            }
            co->outfitdisplays[slot] = displayId;
        }
        else {
            co->outfitdisplays[slot] = 0;
        }
    }
    co->guild = player->GetGuildId();


    sObjectMgr->_creatureOutfitStore[outfitId] = std::move(co);

    CreatureTemplate creatureTemplate;

    uint32 npcCreatureTemplateId = sConfigMgr->GetInt64Default("Freedom.CustomNpc.CreatureTemplateIdStart", 400000);
    if (!_customNpcStore.empty()) {
        using pairtype = std::pair<std::string, CustomNpcData>;
        npcCreatureTemplateId = std::max_element(_customNpcStore.begin(), _customNpcStore.end(),
            [](pairtype a, pairtype b) { return a.second.templateId < b.second.templateId; })->second.templateId + 1;
    }
    creatureTemplate.Entry = npcCreatureTemplateId;

    for (uint32 i = 0; i < MAX_CREATURE_DIFFICULTIES; ++i)
        creatureTemplate.DifficultyEntry[i] = 0;

    for (uint8 i = 0; i < MAX_KILL_CREDIT; ++i)
        creatureTemplate.KillCredit[i] = 0;

    creatureTemplate.Name = key;
    creatureTemplate.GossipMenuId = 0;
    creatureTemplate.minlevel = 1;
    creatureTemplate.maxlevel = 1;
    creatureTemplate.HealthScalingExpansion = EXPANSION_SHADOWLANDS;
    creatureTemplate.RequiredExpansion = EXPANSION_CLASSIC;
    creatureTemplate.VignetteID = 0;
    creatureTemplate.faction = 35;
    creatureTemplate.npcflag = 0;
    creatureTemplate.speed_walk = 1.0f;
    creatureTemplate.speed_run = 1.14286f;
    creatureTemplate.scale = 1.0f;
    creatureTemplate.rank = 0;
    creatureTemplate.dmgschool = 0;
    creatureTemplate.BaseAttackTime = 0;
    creatureTemplate.RangeAttackTime = 0;
    creatureTemplate.BaseVariance = 1;
    creatureTemplate.RangeVariance = 1;
    creatureTemplate.unit_class = UnitClass::UNIT_CLASS_WARRIOR;
    creatureTemplate.unit_flags = 0;
    creatureTemplate.unit_flags2 = 0;
    creatureTemplate.unit_flags3 = 0;
    creatureTemplate.dynamicflags = 0;
    creatureTemplate.family = CREATURE_FAMILY_NONE;
    creatureTemplate.trainer_class = 0;
    creatureTemplate.type = CreatureType::CREATURE_TYPE_HUMANOID;
    creatureTemplate.type_flags = 0;
    creatureTemplate.type_flags2 = 2;
    creatureTemplate.lootid = 0;
    creatureTemplate.pickpocketLootId = 0;
    creatureTemplate.SkinLootId = 0;

    for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        creatureTemplate.resistance[i] = 0;

    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
        creatureTemplate.spells[i] = 0;

    creatureTemplate.VehicleId = 0;
    creatureTemplate.mingold = 0;
    creatureTemplate.maxgold = 0;
    creatureTemplate.AIName = "";
    creatureTemplate.MovementType = 0;
    creatureTemplate.HoverHeight = 1;
    creatureTemplate.ModHealth = 1.0f;
    creatureTemplate.ModHealthExtra = 1.0f;
    creatureTemplate.ModMana = 1.0f;
    creatureTemplate.ModManaExtra = 1.0f;
    creatureTemplate.ModArmor = 1.0f;
    creatureTemplate.ModDamage = 1.0f;
    creatureTemplate.ModExperience = 1.0f;
    creatureTemplate.RacialLeader = false;
    creatureTemplate.movementId = 100;
    creatureTemplate.CreatureDifficultyID = 204488;
    creatureTemplate.WidgetSetID = 0;
    creatureTemplate.WidgetSetUnitConditionID = 0;
    creatureTemplate.RegenHealth = 1;
    creatureTemplate.MechanicImmuneMask = 0;
    creatureTemplate.SpellSchoolImmuneMask = 0;
    creatureTemplate.flags_extra = 0;
    creatureTemplate.ScriptID = sObjectMgr->GetScriptId("");

    creatureTemplate.Models.push_back(CreatureModel(outfitId, 1.0f, 1.0f));

    if (sWorld->getBoolConfig(CONFIG_CACHE_DATA_QUERIES)) {
        for (uint8 loc = LOCALE_enUS; loc < TOTAL_LOCALES; ++loc)
            creatureTemplate.QueryData[loc] = creatureTemplate.BuildQueryData(static_cast<LocaleConstant>(loc));
    }

    sObjectMgr->CheckCreatureTemplate(&creatureTemplate);

    sObjectMgr->_creatureTemplateStore[creatureTemplate.Entry] = creatureTemplate;

    EquipmentInfo _equipmentInfo;
    for (uint8 equipmentInfoSlot = 0; equipmentInfoSlot < MAX_EQUIPMENT_ITEMS; equipmentInfoSlot++) {
        _equipmentInfo.Items[equipmentInfoSlot].ItemId = 0;
        _equipmentInfo.Items[equipmentInfoSlot].AppearanceModId = 0;
        _equipmentInfo.Items[equipmentInfoSlot].ItemVisual = 0;
    }
    sObjectMgr->_equipmentInfoStore[creatureTemplate.Entry][1] = _equipmentInfo;

    CustomNpcData npcData;
    npcData.key = key;
    npcData.templateId = npcCreatureTemplateId;

    _customNpcStore[key] = npcData;
    SaveNpcOutfitToDb(npcCreatureTemplateId, 1);
    SaveCustomNpcDataToDb(npcData);
    SaveNpcCreatureTemplateToDb(creatureTemplate);
    SaveNpcEquipmentInfoToDb(creatureTemplate.Entry, 1);
}

void FreedomMgr::SetCustomNpcOutfitEquipmentSlot(std::string const& key, uint8 variationId, EquipmentSlots slot, int32 displayId)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcOutfitExists(templateId, variationId);
    uint32 outfitId = sObjectMgr->_creatureTemplateStore[templateId].Models[variationId - 1].CreatureDisplayID;
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Setting equipmentslot '%u' for custom npc '%s' with outfitId '%u' to '%u'", slot, key.c_str(), outfitId, displayId);
    std::shared_ptr<CreatureOutfit> co = sObjectMgr->_creatureOutfitStore[outfitId];
    co->outfitdisplays[slot] = displayId;
    sObjectMgr->_creatureOutfitStore[outfitId] = std::move(co);
    SaveNpcOutfitToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcOutfitRace(std::string const& key, uint8 variationId, Races race)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcOutfitExists(templateId, variationId);
    uint32 outfitId = sObjectMgr->_creatureTemplateStore[templateId].Models[variationId - 1].CreatureDisplayID;
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Setting race for custom npc '%s' with outfitId '%u'", key.c_str(), outfitId);
    std::shared_ptr<CreatureOutfit> co = sObjectMgr->_creatureOutfitStore[outfitId];
    co->race = race;

    auto* maleModel = sDB2Manager.GetChrModel(co->race, GENDER_MALE);
    auto* femaleModel = sDB2Manager.GetChrModel(co->race, GENDER_FEMALE);
    switch (co->gender)
    {
    case GENDER_FEMALE: co->displayId = femaleModel->DisplayID; break;
    case GENDER_MALE:   co->displayId = maleModel->DisplayID; break;
    }
    sObjectMgr->_creatureOutfitStore[outfitId] = std::move(co);
    SaveNpcOutfitToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcOutfitGender(std::string const& key, uint8 variationId, Gender gender)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcOutfitExists(templateId, variationId);
    uint32 outfitId = sObjectMgr->_creatureTemplateStore[templateId].Models[variationId - 1].CreatureDisplayID;
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Setting gender for custom npc '%s' with outfitId '%u'", key.c_str(), outfitId);
    std::shared_ptr<CreatureOutfit> co = sObjectMgr->_creatureOutfitStore[outfitId];

    co->gender = gender;
    auto* maleModel = sDB2Manager.GetChrModel(co->race, GENDER_MALE);
    auto* femaleModel = sDB2Manager.GetChrModel(co->race, GENDER_FEMALE);
    switch (co->gender)
    {
    case GENDER_FEMALE: co->displayId = femaleModel->DisplayID; break;
    case GENDER_MALE:   co->displayId = maleModel->DisplayID; break;
    }
    sObjectMgr->_creatureOutfitStore[outfitId] = std::move(co);
    SaveNpcOutfitToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcLeftHand(std::string const& key, uint8 variationId, int32 itemId, int32 appearanceModId)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureEquipmentInfoExists(templateId, variationId);
    EquipmentInfo _equipmentInfo = sObjectMgr->_equipmentInfoStore[templateId][variationId];
    _equipmentInfo.Items[1].ItemId = itemId;
    _equipmentInfo.Items[1].AppearanceModId = appearanceModId;
    sObjectMgr->_equipmentInfoStore[templateId][variationId] = _equipmentInfo;
    SaveNpcEquipmentInfoToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcRightHand(std::string const& key, uint8 variationId, int32 itemId, int32 appearanceModId)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureEquipmentInfoExists(templateId, variationId);
    EquipmentInfo _equipmentInfo = sObjectMgr->_equipmentInfoStore[templateId][variationId];
    _equipmentInfo.Items[0].ItemId = itemId;
    _equipmentInfo.Items[0].AppearanceModId = appearanceModId;
    sObjectMgr->_equipmentInfoStore[templateId][variationId] = _equipmentInfo;
    SaveNpcEquipmentInfoToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcRanged(std::string const& key, uint8 variationId, int32 itemId, int32 appearanceModId)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureEquipmentInfoExists(templateId, variationId);
    EquipmentInfo _equipmentInfo = sObjectMgr->_equipmentInfoStore[templateId][variationId];
    _equipmentInfo.Items[2].ItemId = itemId;
    _equipmentInfo.Items[2].AppearanceModId = appearanceModId;
    sObjectMgr->_equipmentInfoStore[templateId][variationId] = _equipmentInfo;
    SaveNpcEquipmentInfoToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcDisplayId(std::string const& key, uint8 variationId, uint32 displayId)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcModelExists(templateId, variationId);
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Setting model display id for custom npc '%s' variation '%u' to '%u'", key.c_str(), variationId, displayId);
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    CreatureModel model = cTemplate.Models[variationId - 1];
    model.CreatureDisplayID = displayId;
    cTemplate.Models[variationId - 1] = model;
    sObjectMgr->_creatureTemplateStore[templateId] = cTemplate;
    SaveNpcModelInfo(model, templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcModelScale(std::string const& key, uint8 variationId, float displayScale)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcModelExists(templateId, variationId);
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Setting model scale for custom npc '%s' variation '%u' to '%f'", key.c_str(), variationId, displayScale);
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    CreatureModel model = cTemplate.Models[variationId - 1];
    model.DisplayScale = displayScale;
    cTemplate.Models[variationId - 1] = model;
    sObjectMgr->_creatureTemplateStore[templateId] = cTemplate;
    SaveNpcModelInfo(model, templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcRank(std::string const& key, uint32 rank)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    cTemplate.rank = rank;

    sObjectMgr->_creatureTemplateStore[cTemplate.Entry] = cTemplate;
    SaveNpcCreatureTemplateToDb(cTemplate);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcGuild(std::string const& key, uint8 variationId, uint64 guild)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcOutfitExists(templateId, variationId);
    uint32 outfitId = sObjectMgr->_creatureTemplateStore[templateId].Models[variationId - 1].CreatureDisplayID;
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Setting guild for custom npc '%s' with outfitId '%u'", key.c_str(), outfitId);

    std::shared_ptr<CreatureOutfit> co = sObjectMgr->_creatureOutfitStore[outfitId];
    co->guild = guild;
    sObjectMgr->_creatureOutfitStore[outfitId] = std::move(co);
    SaveNpcOutfitToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcTameable(std::string const& key, bool tameable)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    cTemplate.type = tameable ? 1 : 0;
    cTemplate.family = tameable ? CREATURE_FAMILY_GORILLA : CREATURE_FAMILY_NONE;
    cTemplate.type_flags = tameable ? 1 : 0;

    sObjectMgr->_creatureTemplateStore[cTemplate.Entry] = cTemplate;
    SaveNpcCreatureTemplateToDb(cTemplate);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcName(std::string const& key, std::string const& displayName)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    cTemplate.Name = displayName;

    sObjectMgr->_creatureTemplateStore[cTemplate.Entry] = cTemplate;
    SaveNpcCreatureTemplateToDb(cTemplate);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcSubName(std::string const& key, std::string const& subName)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    cTemplate.SubName = subName;

    sObjectMgr->_creatureTemplateStore[cTemplate.Entry] = cTemplate;
    SaveNpcCreatureTemplateToDb(cTemplate);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcCustomizations(std::string const& key, uint8 variationId, Player* player)
{
    uint32 templateId = _customNpcStore[key].templateId;
    EnsureNpcOutfitExists(templateId, variationId);
    uint32 outfitId = sObjectMgr->_creatureTemplateStore[templateId].Models[variationId - 1].CreatureDisplayID;
    std::shared_ptr<CreatureOutfit> co = sObjectMgr->_creatureOutfitStore[outfitId];

    std::vector<UF::ChrCustomizationChoice> customizations;
    for (auto&& customization : player->m_playerData->Customizations)
    {
        customizations.push_back(customization);
    }
    co->Customizations = customizations;
    sObjectMgr->_creatureOutfitStore[outfitId] = std::move(co);
    SaveNpcOutfitToDb(templateId, variationId);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::ToggleCustomNpcAura(std::string const& key, uint32 auraId, bool active)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureAddon cAddon = sObjectMgr->_creatureTemplateAddonStore[templateId];

    auto it = cAddon.auras.begin();
    for (; it != cAddon.auras.end(); it++)
    {
        if (*it == auraId && active) // Nothing to do aura already on
        {
            return;
        }

        if (*it == auraId && !active) // we found auraId we want to remove
            break;
    }
    if (active)
    {
        cAddon.auras.push_back(auraId);
    }
    else if (it != cAddon.auras.end())
    {
        cAddon.auras.erase(it);
    }

    sObjectMgr->_creatureTemplateAddonStore[templateId] = cAddon;
    SaveNpcCreatureTemplateAddonToDb(templateId, cAddon);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcMount(std::string const& key, uint32 mount)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureAddon cAddon = sObjectMgr->_creatureTemplateAddonStore[templateId];

    cAddon.mount = mount;

    sObjectMgr->_creatureTemplateAddonStore[templateId] = cAddon;
    SaveNpcCreatureTemplateAddonToDb(templateId, cAddon);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SetCustomNpcEmote(std::string const& key, uint32 emote)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureAddon cAddon = sObjectMgr->_creatureTemplateAddonStore[templateId];

    cAddon.emote = emote;

    sObjectMgr->_creatureTemplateAddonStore[templateId] = cAddon;
    SaveNpcCreatureTemplateAddonToDb(templateId, cAddon);
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::RemoveCustomNpcVariation(std::string const& key, uint8 variationId)
{
    uint32 templateId = _customNpcStore[key].templateId;
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    uint8 currentModel = variationId;
    while (currentModel != cTemplate.Models.size()) {
        // Shift models lower
        cTemplate.Models[currentModel - 1] = cTemplate.Models[currentModel];
        SaveNpcModelInfo(cTemplate.Models[currentModel - 1], templateId, currentModel);
        currentModel++;
    }

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_TEMPLATE_MODEL);
    stmt->setUInt32(0, templateId);
    stmt->setUInt8(1, currentModel - 1);
    WorldDatabase.Execute(stmt);

    cTemplate.Models.pop_back();
    sObjectMgr->_creatureTemplateStore[templateId] = cTemplate;
    ReloadSpawnedCustomNpcs(key);
}

void FreedomMgr::SaveNpcOutfitToDb(uint32 templateId, uint8 variationId)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Saving variation '%u' for templateid '%u' to DB...", variationId, templateId);
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    uint32 outfitId = cTemplate.Models[variationId - 1].CreatureDisplayID;
    std::shared_ptr<CreatureOutfit> co = sObjectMgr->_creatureOutfitStore[outfitId];
    std::string customizations_iss;
    for (auto&& customization : co->Customizations)
    {
        if (customizations_iss.size()) {
            customizations_iss.append(" ");
        }
        customizations_iss.append(std::to_string(customization.ChrCustomizationOptionID) + " " + std::to_string(customization.ChrCustomizationChoiceID));
    }

    WorldDatabaseTransaction trans = WorldDatabase.BeginTransaction();
    // "REPLACE INTO creature_template_outfits (entry, race, class, gender, customizations, head, shoulders, body, chest, waist, legs, feet, wrists, hands, back, tabard, guildid)
    //  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_DRESSNPC_OUTFIT);
    int index = 0;
    stmt->setUInt32(index++, outfitId);
    stmt->setUInt8(index++, co->race);
    stmt->setUInt8(index++, co->Class);
    stmt->setUInt8(index++, co->gender);
    stmt->setString(index++, customizations_iss);
    for (EquipmentSlots slot : CreatureOutfit::item_slots)
    {
        stmt->setInt32(index++, -((int)co->outfitdisplays[slot]));
    }
    stmt->setUInt64(index++, co->guild);
    trans->Append(stmt);

    // "REPLACE INTO creature_template_model (CreatureId, Idx, CreatureDisplayId, DisplayScale, Probability) VALUES (?, ?, ?, ?, 1)"
    stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_TEMPLATE_MODEL);
    stmt->setUInt32(0, templateId);
    stmt->setUInt8(1, variationId - 1);
    stmt->setUInt32(2, outfitId);
    stmt->setFloat(3, cTemplate.Models[variationId - 1].DisplayScale);
    trans->Append(stmt);
    WorldDatabase.CommitTransaction(trans);
}

void FreedomMgr::SaveNpcModelInfo(CreatureModel model, uint32 creatureTemplateId, uint8 variationId)
{
    // "REPLACE INTO creature_template_model (CreatureId, Idx, CreatureDisplayId, DisplayScale, Probability) VALUES (?, ?, ?, ?, 1)"
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_TEMPLATE_MODEL);
    stmt->setUInt32(0, creatureTemplateId);
    stmt->setUInt8(1, variationId - 1);
    stmt->setUInt32(2, model.CreatureDisplayID);
    stmt->setFloat(3, model.DisplayScale);
    WorldDatabase.Execute(stmt);
}

void FreedomMgr::SaveCustomNpcDataToDb(CustomNpcData outfitData)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Saving custom npc data for key '%s' to DB...", outfitData.key.c_str());
    // "REPLACE INTO custom_npcs (Key, Entry) VALUES (?, ?)"
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_REP_CUSTOMNPCDATA);
    int index = 0;
    stmt->setString(index++, outfitData.key);
    stmt->setUInt32(index++, outfitData.templateId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::SaveNpcCreatureTemplateToDb(CreatureTemplate cTemplate)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Saving creature template id '%u' to DB...", cTemplate.Entry);
    // REPLACE INTO creature_template
    // (entry, name, subname, HealthScalingExpansion, RequiredExpansion, faction, unit_class, type, type_flags, type_flags2, movementId, CreatureDifficultyID, minlevel, maxlevel, family)
    // VALUES (?, ?, ?, 8, 0, 35, 1, ?, ?, 2, 100, 204488, 60, 60, ?)
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_TEMPLATE);
    int index = 0;
    stmt->setUInt32(index++, cTemplate.Entry);
    stmt->setString(index++, cTemplate.Name);
    stmt->setString(index++, cTemplate.SubName);
    stmt->setUInt32(index++, cTemplate.type);
    stmt->setUInt32(index++, cTemplate.type_flags);
    stmt->setUInt32(index++, cTemplate.family);
    WorldDatabase.Execute(stmt);

    // Redo cached creatureTemplates
    cTemplate.InitializeQueryData();

    // Resend creature template query packet for npc to all players. Best update we can do atm.
    SessionMap const& smap = sWorld->GetAllSessions();
    for (SessionMap::const_iterator iter = smap.begin(); iter != smap.end(); ++iter)
    {
        TC_LOG_DEBUG("freedom", "FREEDOMMGR: Sending query packet for creatureTemplate '%s' to '%s'.", cTemplate.Name.c_str(), iter->second->GetPlayer()->GetName().c_str());
        if (sWorld->getBoolConfig(CONFIG_CACHE_DATA_QUERIES)) {
            iter->second->SendPacket(&cTemplate.QueryData[static_cast<uint32>(iter->second->GetSessionDbLocaleIndex())]);
        }
        else
        {
            WorldPacket response = cTemplate.BuildQueryData(iter->second->GetSessionDbLocaleIndex());
            iter->second->SendPacket(&response);
        }
    }
}

void FreedomMgr::SaveNpcEquipmentInfoToDb(uint32 templateId, uint8 variationId)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Saving equipment info '%u' for creature template id '%u' to DB...", variationId, templateId);
    // "REPLACE INTO creature_equip_template (CreatureId, ID, ItemID1, AppearanceModID1, ItemVisual1, ItemID2, AppearanceModID2, ItemVisual2, ItemID3, AppearanceModID3, ItemVisual3) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"

    EquipmentInfo equipInfo = sObjectMgr->_equipmentInfoStore[templateId][variationId];

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_EQUIP_TEMPLATE);
    int index = 0;
    stmt->setUInt32(index++, templateId);
    stmt->setUInt8(index++, variationId);
    for (uint8 equipmentInfoSlot = 0; equipmentInfoSlot < MAX_EQUIPMENT_ITEMS; equipmentInfoSlot++) {
        stmt->setUInt32(index++, equipInfo.Items[equipmentInfoSlot].ItemId);
        stmt->setUInt16(index++, equipInfo.Items[equipmentInfoSlot].AppearanceModId);
        stmt->setUInt16(index++, equipInfo.Items[equipmentInfoSlot].ItemVisual);
    }
    WorldDatabase.Execute(stmt);
}

void FreedomMgr::SaveNpcCreatureTemplateAddonToDb(uint32 templateId, CreatureAddon cAddon)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Saving creature addon template for creature template id '%u' to DB...", templateId);
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_TEMPLATE_ADDON);

    // REPLACE INTO creature_template_addon (entry, path_id, mount, bytes1, bytes2, emote, aiAnimKit, movementAnimKit, meleeAnimkit, visibilityDistanceType, auras) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    int index = 0;
    stmt->setUInt32(index++, templateId);
    stmt->setUInt32(index++, cAddon.path_id);
    stmt->setUInt32(index++, cAddon.mount);
    stmt->setUInt32(index++, cAddon.bytes1);
    stmt->setUInt32(index++, cAddon.bytes2);
    stmt->setUInt32(index++, cAddon.emote);
    stmt->setUInt16(index++, cAddon.aiAnimKit);
    stmt->setUInt16(index++, cAddon.movementAnimKit);
    stmt->setUInt16(index++, cAddon.meleeAnimKit);
    stmt->setUInt8(index++, uint8(cAddon.visibilityDistanceType));
    if (!cAddon.auras.empty())
    {
        std::string auraList = "";
        for (auto aura : cAddon.auras)
        {
            if (auraList.empty())
                auraList = std::to_string(aura);
            else
                auraList += " " + std::to_string(aura);
        }
        stmt->setString(index, auraList);
    }
    else
    {
        stmt->setNull(index);
    }
    WorldDatabase.Execute(stmt);
}

void FreedomMgr::ReloadSpawnedCustomNpcs(std::string const& key)
{
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Reloading custom npc '%s'", key.c_str());
    CustomNpcData data = _customNpcStore[key];
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Iterated spawns has " SZFMTD " elements in it.", data.spawns.size());
    for (auto spawn : data.spawns) {
        Creature* creature = GetAnyCreature(spawn);
        if (!creature) {
            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Unable to find creature id '%lu'", spawn);
        }
        else {
            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Reloading creature id '%lu'", spawn);
            uint8 modelId = urand(0u, sObjectMgr->_creatureTemplateStore[data.templateId].Models.size() - 1);
            CreatureModel model = sObjectMgr->_creatureTemplateStore[data.templateId].Models[modelId];
            CreatureAddon cAddon = sObjectMgr->_creatureTemplateAddonStore[data.templateId];
            creature->SetDisplayId(model.CreatureDisplayID, model.DisplayScale);
            creature->SetName(sObjectMgr->_creatureTemplateStore[data.templateId].Name);
            creature->LoadEquipment(urand(1u, sObjectMgr->_equipmentInfoStore[data.templateId].size()));

            creature->RemoveAllAuras();
            for (auto auraId : cAddon.auras) {
                creature->AddAura(auraId, creature);
            }

            creature->SetEmoteState(Emote(cAddon.emote));
            if (cAddon.mount)
                creature->Mount(cAddon.mount);
            else
                creature->Dismount();

            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Reloaded creature id '%lu'", spawn);
        }
    }
}

void FreedomMgr::DeleteCustomNpc(std::string const& key)
{
    CustomNpcData data = _customNpcStore[key];
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Deleting custom npc '%s' with entry '%u'", key.c_str(), data.templateId);
    // Remove spawns
    for (auto spawn : data.spawns) {
        TC_LOG_DEBUG("freedom", "FREEDOMMGR: Deleting spawn " UI64FMTD, spawn);
        Creature* creature = GetAnyCreature(spawn);
        if (creature) {
            CreatureDelete(creature);
        }
        else {
            Creature::DeleteFromDB(spawn);
        }
    }
    // Cleanup database
    WorldDatabaseTransaction trans = WorldDatabase.BeginTransaction();

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_ALL_CREATURE_TEMPLATE_MODEL);
    stmt->setUInt32(0, data.templateId);
    trans->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_EQUIP_TEMPLATE);
    stmt->setUInt32(0, data.templateId);
    trans->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_TEMPLATE_ADDON);
    stmt->setUInt32(0, data.templateId);
    trans->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_TEMPLATE);
    stmt->setUInt32(0, data.templateId);
    trans->Append(stmt);

    for (uint8 modelId = 0; modelId < sObjectMgr->_creatureTemplateStore[data.templateId].Models.size(); modelId++)
    {
        uint32 outfitId = sObjectMgr->_creatureTemplateStore[data.templateId].Models[modelId].CreatureDisplayID;
        stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_DRESSNPC_OUTFIT);
        stmt->setUInt32(0, outfitId);
        trans->Append(stmt);
    }

    WorldDatabase.CommitTransaction(trans);

    FreedomDatabasePreparedStatement* fStmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_CUSTOMNPC);
    fStmt->setString(0, key);
    FreedomDatabase.Execute(fStmt);

    _customNpcStore.erase(key);
    for (uint8 modelId = 0; modelId < sObjectMgr->_creatureTemplateStore[data.templateId].Models.size(); modelId++)
    {
        uint32 outfitId = sObjectMgr->_creatureTemplateStore[data.templateId].Models[modelId].CreatureDisplayID;
        sObjectMgr->_creatureOutfitStore.erase(outfitId);
    }
    sObjectMgr->_equipmentInfoStore.erase(data.templateId);
    sObjectMgr->_creatureTemplateAddonStore.erase(data.templateId);
    sObjectMgr->_creatureTemplateStore.erase(data.templateId);
}


void FreedomMgr::EnsureNpcOutfitExists(uint32 templateId, uint8 variationId, float displayScale)
{
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    uint32 modelsSize = cTemplate.Models.size();

    std::vector<uint8> toCreate;

    // Ensure the variation exists
    if (modelsSize < variationId) {
        for (uint8 i = modelsSize; i < variationId; i++) {
            toCreate.push_back(i);
        }
    }   // Ensure model is an outfit
    else if (!CreatureOutfit::IsFake(cTemplate.Models[variationId - 1].CreatureDisplayID)) {
        toCreate.push_back(variationId - 1);
    }

    if (!toCreate.empty())
    {
        using pairtype = std::pair<uint32, std::shared_ptr<CreatureOutfit>>;
        uint32 maxOutfitId = std::max_element(sObjectMgr->_creatureOutfitStore.begin(), sObjectMgr->_creatureOutfitStore.end(),
            [](pairtype a, pairtype b) { return a.second->id < b.second->id; })->second->id;
        std::shared_ptr<CreatureOutfit> lastOutfit;
        bool setOutfit = false;
        for (uint8 i = 1; i <= modelsSize; i++) {
            uint32 displayId = cTemplate.Models[modelsSize - i].CreatureDisplayID;
            if (CreatureOutfit::IsFake(displayId)) {
                lastOutfit = sObjectMgr->_creatureOutfitStore[displayId];
                setOutfit = true;
                break;
            }
        }
        if (!setOutfit) {
            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Custom NPC template '%u' has no outfits, selecting first option in store for variation '%u'.", templateId, variationId);
            // Custom NPC has only used displayids, in this case we'll just take the first creatureoutfit available.
            lastOutfit = sObjectMgr->_creatureOutfitStore.begin()->second;
        }

        uint8 created = 0;
        for (uint8 variation : toCreate)
        {
            std::shared_ptr<CreatureOutfit> outfit(new CreatureOutfit());
            uint32 outfitId = maxOutfitId + ++created;
            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Adding outfit '%u' to create variation '%u' for template '%u'.", outfitId, variation, templateId);
            outfit->id = outfitId;
            outfit->npcsoundsid = 0;
            outfit->race = lastOutfit->race;
            outfit->Class = lastOutfit->Class;
            outfit->gender = lastOutfit->gender;
            outfit->displayId = lastOutfit->displayId;
            outfit->SpellVisualKitID = 0;
            outfit->Customizations = lastOutfit->Customizations;
            for (EquipmentSlots slot : CreatureOutfit::item_slots)
            {
                outfit->outfitdisplays[slot] = lastOutfit->outfitdisplays[slot];
            }
            outfit->guild = lastOutfit->guild;
            outfit->id = outfitId;
            sObjectMgr->_creatureOutfitStore[outfitId] = std::move(outfit);
            if (variation >= modelsSize) {
                cTemplate.Models.push_back(CreatureModel(outfitId, displayScale, 1));
            }
            else {
                cTemplate.Models[variation] = CreatureModel(outfitId, displayScale, 1);
            }
            sObjectMgr->_creatureTemplateStore[templateId] = cTemplate;
        }
    }


    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Model variation '%u' for template '%u' ensured.", variationId, templateId);
}

void FreedomMgr::EnsureEquipmentInfoExists(uint32 templateId, uint8 variationId)
{
    if (!sObjectMgr->_equipmentInfoStore[templateId].count(variationId)) {
        TC_LOG_DEBUG("freedom", "FREEDOMMGR: Adding equipment variation '%u' for template '%u'.", variationId, templateId);
        EquipmentInfo equipInfo;
        for (uint8 equipmentInfoSlot = 0; equipmentInfoSlot < MAX_EQUIPMENT_ITEMS; equipmentInfoSlot++) {
            equipInfo.Items[equipmentInfoSlot].ItemId = 0;
            equipInfo.Items[equipmentInfoSlot].AppearanceModId = 0;
            equipInfo.Items[equipmentInfoSlot].ItemVisual = 0;
        }
        sObjectMgr->_equipmentInfoStore[templateId][variationId] = equipInfo;
    }
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Equipment variation '%u' for template '%u' ensured.", variationId, templateId);
}

void FreedomMgr::EnsureNpcModelExists(uint32 templateId, uint8 variationId)
{
    CreatureTemplate cTemplate = sObjectMgr->_creatureTemplateStore[templateId];
    uint32 modelsSize = cTemplate.Models.size();
    if (modelsSize < variationId) {
        for (uint8 i = modelsSize; i < variationId; i++) {
            uint32 prevDisplayId = cTemplate.Models[modelsSize - 1].CreatureDisplayID;
            TC_LOG_DEBUG("freedom", "FREEDOMMGR: Creating model variation '%u' for template '%u' using displayid: '%u'.", i, templateId, prevDisplayId);
            cTemplate.Models.push_back(CreatureModel(prevDisplayId, 1, 1));
            sObjectMgr->_creatureTemplateStore[templateId] = cTemplate;
        }
    }
    TC_LOG_DEBUG("freedom", "FREEDOMMGR: Model variation '%u' for template '%u' ensured.", variationId, templateId);
}

uint8 FreedomMgr::GetModelVariationCountForNpc(std::string const& key) {
    return sObjectMgr->_creatureTemplateStore[_customNpcStore[key].templateId].Models.size();
}
uint8 FreedomMgr::GetEquipmentVariationCountForNpc(std::string const& key) {
    return sObjectMgr->_equipmentInfoStore[_customNpcStore[key].templateId].size();
}

void FreedomMgr::StoreMarkerLocationForPlayer(Player* player, const WorldLocation* marker)
{
    _playerExtraDataStore[player->GetGUID().GetCounter()].markerLocation = *marker;
}

FormationData* FreedomMgr::GetFormationByLeader(ObjectGuid::LowType leaderGuid)
{
    FormationDataContainer::iterator res = std::find_if(_formationStore.begin(), _formationStore.end(),
        [leaderGuid](std::pair<const std::string, FormationData> data) { return data.second.leader == leaderGuid; });
    if (res != _formationStore.end()) {
        return &res->second;
    }
    return nullptr;
}

void FreedomMgr::AddFormation(std::string const& key, ObjectGuid::LowType leaderGuid)
{
    FormationData data;
    data.key = key;
    data.leader = leaderGuid;
    _formationStore[key] = data;

    Creature* leader = GetAnyCreature(leaderGuid);
    sFormationMgr->AddFormationMember(leaderGuid, 0, 0, leaderGuid, FLAG_IDLE_IN_FORMATION);
    leader->SearchFormation();

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_CREATURE_FORMATION);
    stmt->setUInt64(0, leaderGuid);
    stmt->setUInt64(1, leaderGuid);
    stmt->setFloat(2, 0);
    stmt->setFloat(3, 0);
    stmt->setUInt32(4, FLAG_IDLE_IN_FORMATION);
    WorldDatabase.Execute(stmt);

    FreedomDatabasePreparedStatement* fstmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_FORMATION);
    fstmt->setString(0, key);
    fstmt->setUInt64(1, leaderGuid);
    FreedomDatabase.Execute(fstmt);
}

void FreedomMgr::DeleteFormation(std::string const& key)
{
    FormationData data = _formationStore[key];
    sFormationMgr->DeleteFormationWithLeader(data.leader);
    _formationStore.erase(key);

    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_FORMATION);
    stmt->setString(0, key);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::LoadFormations()
{
    // clear current storage
    _formationStore.clear();

    uint32 oldMSTime = getMSTime();
    QueryResult result = FreedomDatabase.Query("SELECT `Key`, LeaderGuid from formations");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 formations. DB table `formations` is empty!");
        return;
    }
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        FormationData formationData;
        formationData.key = fields[0].GetString();
        formationData.leader = fields[1].GetUInt64();
        _formationStore[formationData.key] = formationData;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u formations in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void FreedomMgr::SaveFormationPosition(std::string const& key, Player* player)
{
    FormationData data = _formationStore[key];
    Creature* leader = GetAnyCreature(data.leader);
    Map* map = leader->GetMap();

    auto itr = map->CreatureGroupHolder.find(data.leader);
    if (itr == map->CreatureGroupHolder.end())
    {
        TC_LOG_ERROR("freedom", "No creature group found for formation %s", key.c_str());
    }
    else {
        for (Creature* member : itr->second->GetMembers())
        {
            CreatureSetModifyHistory(member, player);
            SaveCreature(member);
        }
    }
}

void FreedomMgr::AddAuraApplication(Player* player, uint32 spellId, Unit* target)
{
    AppliedAuraData auraData;
    auraData.spellId = spellId;
    auraData.target = target->IsPlayer() ? target->GetGUID().GetCounter() : target->ToCreature()->GetSpawnId();

    AppliedAuraContainer auraContainer = _playerExtraDataStore[player->GetGUID().GetCounter()].appliedAuraStore;
    auraContainer.push_back(auraData);
    _playerExtraDataStore[player->GetGUID().GetCounter()].appliedAuraStore = auraContainer;
}

void FreedomMgr::RemoveAllAuraApplications(Player* player)
{
    AppliedAuraContainer auraContainer = _playerExtraDataStore[player->GetGUID().GetCounter()].appliedAuraStore;

    TC_LOG_DEBUG("freedom", "RemoveAllAuraApplications :: AuraContainer size = " SZFMTD, auraContainer.size());
    while (!auraContainer.empty()) {
        AppliedAuraData auraData = auraContainer.front();
        Unit* target = GetAnyUnit(auraData.target);
        if (target && target->IsInWorld()) {
            TC_LOG_DEBUG("freedom", "Removing aura %u cast on " SZFMTD, auraData.spellId, target->GetGUID().GetCounter());
            target->RemoveAura(auraData.spellId, player->GetGUID());
        }
        auraContainer.erase(auraContainer.begin());
    }

    _playerExtraDataStore[player->GetGUID().GetCounter()].appliedAuraStore = auraContainer;
}

void FreedomMgr::RemoveAuraApplications(Player* player, uint32 spellId)
{
    AppliedAuraContainer auraContainer = _playerExtraDataStore[player->GetGUID().GetCounter()].appliedAuraStore;
    TC_LOG_DEBUG("freedom", "RemoveAllAuraApplications spellId :: AuraContainer size = " SZFMTD, auraContainer.size());

    std::vector<int> toRemove;
    for (auto i = auraContainer.begin(); i != auraContainer.end();)
    {
        AppliedAuraData auraData = *i;
        Unit* target = GetAnyUnit(auraData.target);
        if (auraData.spellId == spellId && target && target->IsInWorld()) {
            TC_LOG_DEBUG("freedom", "Removing aura %u cast on " SZFMTD, auraData.spellId, target->GetGUID().GetCounter());
            target->RemoveAura(auraData.spellId, player->GetGUID());
            auraContainer.erase(i);
            i = auraContainer.begin();
        }
        else
            ++i;
    }

    _playerExtraDataStore[player->GetGUID().GetCounter()].appliedAuraStore = auraContainer;
}

void FreedomMgr::LoadNpcCasts() {
    // clear current storage
    _npcCastStore.clear();

    uint32 oldMSTime = getMSTime();
    QueryResult result = FreedomDatabase.Query("SELECT `id`, source_spawn, target_spawn, spell_id, duration, restInterval, initialRest FROM npc_casts");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 NPC spell casts. DB table `npc_casts` is empty!");
        return;
    }
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        std::shared_ptr<NpcCastData> castData(new NpcCastData());
        castData->id = fields[0].GetUInt32();
        castData->source = fields[1].GetUInt64();
        castData->target = fields[2].GetUInt64();
        castData->spell_id = fields[3].GetUInt32();
        castData->duration = fields[4].GetUInt32();
        castData->restInterval = fields[5].GetUInt32();
        castData->initialRest = fields[6].GetUInt32();

        castData->currentRest = castData->initialRest;

        _npcCastStore.push_back(std::move(castData));
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u npc spell casts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void FreedomMgr::AddNpcCast(Creature* source, Unit* target, uint32 spellId, uint32 duration, uint32 restInterval, uint32 initialRest, bool persist)
{
    std::shared_ptr<NpcCastData> castData(new NpcCastData());
    castData->id = 0;
    castData->source = source->GetSpawnId();
    castData->target = target->IsPlayer() ? target->GetGUID().GetCounter() : target->ToCreature()->GetSpawnId();
    castData->spell_id = spellId;
    castData->duration = duration;
    castData->restInterval = restInterval;
    castData->initialRest = initialRest;
    castData->currentRest = initialRest;

    _npcCastStore.push_back(castData);
    _activeNpcCastStore.push_back(castData);

    Creature* targetCreature = target->ToCreature();
    if (!persist || !targetCreature || source->GetSpawnId() == 0 || targetCreature->GetSpawnId() == 0) {
        return;
    }

    TC_LOG_DEBUG("freedom", "FreedomMgr::AddNpcCast | Adding npc cast to db...");
    // INSERT INTO npc_casts (source_spawn, target_spawn, spell_id, duration, restInterval, initialRest) VALUES (?, ?, ?, ?, ?, ?)
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_NPC_CAST);
    stmt->setUInt64(0, source->GetSpawnId());
    stmt->setUInt64(1, targetCreature->GetSpawnId());
    stmt->setUInt32(2, castData->spell_id);
    stmt->setUInt32(3, castData->duration);
    stmt->setUInt32(4, castData->restInterval);
    stmt->setUInt32(5, castData->initialRest);
    FreedomDatabase.DirectExecute(stmt);
    PreparedQueryResult result = FreedomDatabase.Query(FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_NPC_CAST_MAX_ID));
    castData->id = result->Fetch()[0].GetUInt32();
    TC_LOG_DEBUG("freedom", "FreedomMgr::AddNpcCast | Npc cast received id: %u", castData->id);
}


void FreedomMgr::StopNpcCast(std::shared_ptr<NpcCastData> cast)
{
    TC_LOG_DEBUG("freedom", "FreedomMgr::StopNpcCast | Stopping npc cast %u", cast->spell_id);
    Unit* caster = GetAnyUnit(cast->source);
    if (caster) {
        RemoveSpellEffects(caster, cast->spell_id);
        Unit* target = GetAnyUnit(cast->target);
        if (target) {
            target->RemoveAura(cast->spell_id, caster->GetGUID());
        }
    }
}

void FreedomMgr::DisableNpcCast(std::shared_ptr <NpcCastData> cast, bool stop)
{
    if (stop) {
        StopNpcCast(cast);
    }
    _activeNpcCastStore.erase(std::remove(_activeNpcCastStore.begin(), _activeNpcCastStore.end(), cast), _activeNpcCastStore.end());
}

void FreedomMgr::DeleteNpcCast(Creature* source, uint32 spellId)
{
    ObjectGuid sourceGuid = source->GetGUID();
    TC_LOG_DEBUG("freedom", "FreedomMgr::RemoveNpcCast | Removing npc cast '%u' from creature " SZFMTD, spellId, sourceGuid.GetCounter());
    for (auto i = _npcCastStore.begin(); i != _npcCastStore.end();)
    {
        if ((*i)->spell_id == spellId && (*i)->source == source->GetSpawnId()) {
            TC_LOG_DEBUG("freedom", "FreedomMgr::RemoveNpcCast | Stopping cast...");
            DisableNpcCast((*i));
            if ((*i)->id > 0) {
                TC_LOG_DEBUG("freedom", "FreedomMgr::RemoveNpcCast | Removing npc cast '%u' from db...", (*i)->id);
                FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_NPC_CAST);
                stmt->setUInt32(0, (*i)->id);
                FreedomDatabase.Execute(stmt);
            }
            i = _npcCastStore.erase(i);
        }
        else {
            ++i;
        }
    }
}

void FreedomMgr::EnableNpcCastsForCreature(Creature* creature)
{
    ObjectGuid::LowType spawnId = creature->GetSpawnId();
    if (!spawnId) {
        return;
    }
    TC_LOG_DEBUG("freedom", "FreedomMgr::EnableNpcCastsForCreature | Enabling casts for creature: " SZFMTD "...", spawnId);
    for (auto i = _npcCastStore.begin(); i != _npcCastStore.end();)
    {
        if ((*i)->source == spawnId) {
            if (GetAnyUnit((*i)->target)) {
                _activeNpcCastStore.push_back((*i));
            }
        }
        else if ((*i)->target == spawnId) {
            if (GetAnyUnit((*i)->source)) {
                _activeNpcCastStore.push_back((*i));
            }
        }
        ++i;
    }
}

void FreedomMgr::DisableNpcCastsForCreature(Creature* creature)
{
    ObjectGuid::LowType spawnId = creature->GetSpawnId();
    if (!spawnId) {
        return;
    }
    TC_LOG_DEBUG("freedom", "FreedomMgr::DisableNpcCastsForCreature | Disabling casts for creature: " SZFMTD "...", spawnId);
    for (auto i = _activeNpcCastStore.begin(); i != _activeNpcCastStore.end();)
    {
        if ((*i)->source == spawnId) {
            _removedNpcCastStore.insert((*i));
        }
        else if ((*i)->target == spawnId) {
            _removedNpcCastStore.insert((*i));
        }
        ++i;
    }
}

void FreedomMgr::CleanNpcCastsRemoveList()
{
    for (auto i = _removedNpcCastStore.begin(); i != _removedNpcCastStore.end();)
    {
        DisableNpcCast((*i));
        i = _removedNpcCastStore.erase(i);
    }
}

void FreedomMgr::RemoveSpellEffects(Unit* unit, uint32 spellId) {
    TC_LOG_DEBUG("freedom", "FreedomMgr::RemoveSpellEffects | Removing spell effects of '%u' from unit " SZFMTD, spellId, unit->GetGUID().GetCounter());
    if (!unit) {
        return;
    }
    unit->RemoveAura(spellId, unit->GetGUID());
    unit->RemoveAreaTrigger(spellId);
    unit->RemoveDynObject(spellId);
    unit->RemoveGameObject(spellId, true);
    // Remove temp summons created by spell
    for (auto i = unit->m_Controlled.begin(); i != unit->m_Controlled.end();)
    {
        Unit* unit = (*i);
        i++;
        if (unit && unit->IsSummon() && (uint32)unit->m_unitData->CreatedBySpell == spellId) {
            unit->ToTempSummon()->UnSummon();
        }
    }

    // Remove aura's applied by players
    Player* player = unit->ToPlayer();
    if (player) {
        sFreedomMgr->RemoveAuraApplications(player, spellId);
    }

    // Recursively remove effects from spells that cast spells...
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellId, unit->GetMap()->GetDifficultyID());
    if (spellInfo) {
        for (auto effect : spellInfo->GetEffects()) {
            if (effect.TriggerSpell) {
                RemoveSpellEffects(unit, effect.TriggerSpell);
            }
        }
    }
}

void FreedomMgr::LoadAnimationMappings()
{
    // clear current storage
    _animationKitMappingStore.clear();

    uint32 oldMSTime = getMSTime();
    QueryResult result = FreedomDatabase.Query("SELECT `animationId`, `animkitId` FROM animation_mapping");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 animation to animationkit mappings. DB table `animation_mapping` is empty!");
        return;
    }
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        int32 animationId = fields[0].GetInt32();
        int16 animkitId = fields[1].GetInt16();

        _animationKitMappingStore[animationId] = animkitId;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u animation to animationkit mappings in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}


MountData const* FreedomMgr::GetMountByName(ObjectGuid::LowType playerId, std::string const& name)
{
    auto it = std::find_if(
        _playerExtraDataStore[playerId].mountDataStore.begin(), _playerExtraDataStore[playerId].mountDataStore.end(),
        [name](MountData m)->bool { return boost::iequals(name, m.name); }
    );

    if (it != _playerExtraDataStore[playerId].mountDataStore.end())
        return &(*it);
    else
        return nullptr;
}

MountData const* FreedomMgr::GetMountByDisplayId(ObjectGuid::LowType playerId, uint32 displayId)
{
    auto it = std::find_if(
        _playerExtraDataStore[playerId].mountDataStore.begin(), _playerExtraDataStore[playerId].mountDataStore.end(),
        [displayId](MountData m)->bool { return m.displayId == displayId; }
    );

    if (it != _playerExtraDataStore[playerId].mountDataStore.end())
        return &(*it);
    else
        return nullptr;
}

void FreedomMgr::AddMount(ObjectGuid::LowType playerId, MountData const& data)
{
    _playerExtraDataStore[playerId].mountDataStore.push_back(data);

    // guid, name, id_display, id_bnet_gm
    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_MOUNT);
    stmt->setUInt64(index++, playerId);
    stmt->setString(index++, data.name);
    stmt->setUInt32(index++, data.displayId);
    stmt->setUInt32(index++, data.gmBnetAccId);
    FreedomDatabase.Execute(stmt);
}

void FreedomMgr::DeleteMountByName(ObjectGuid::LowType playerId, std::string const& name)
{
    auto it = std::find_if(
        _playerExtraDataStore[playerId].mountDataStore.begin(), _playerExtraDataStore[playerId].mountDataStore.end(),
        [name](MountData m)->bool { return boost::iequals(name, m.name); }
    );

    if (it != _playerExtraDataStore[playerId].mountDataStore.end())
    {
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_MOUNTS_NAME);
        stmt->setString(0, name);
        stmt->setUInt64(1, playerId);
        FreedomDatabase.Execute(stmt);

        _playerExtraDataStore[playerId].mountDataStore.erase(it);
    }
}

void FreedomMgr::LoadMounts()
{
    // clear current Morph storage for each player
    for (auto it : _playerExtraDataStore)
    {
        it.second.mountDataStore.clear();
    }

    // guid, name, id_display, id_bnet_gm
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_MOUNTS);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        MountData data;
        ObjectGuid::LowType charGuid = fields[0].GetUInt64();
        data.name = fields[1].GetString();
        data.displayId = fields[2].GetUInt32();
        data.gmBnetAccId = fields[3].GetUInt32();

        _playerExtraDataStore[charGuid].mountDataStore.push_back(data);
    } while (result->NextRow());
}

void FreedomMgr::LoadAddonLoginMessages()
{
    _addonLoginMessageStore.clear();

    // SELECT id, prefix, message, senderGuid FROM addon_login_msgs
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_ADDON_LOGIN_MSGS);
    PreparedQueryResult result = FreedomDatabase.Query(stmt);

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        AddonLoginMessageData data;
        data.id = fields[0].GetUInt32();
        data.prefix = fields[1].GetString();
        data.message = fields[2].GetString();
        data.senderGuid = ObjectGuid::Create<HighGuid::Player>(fields[3].GetUInt64());
        data.senderAccountGuid = ObjectGuid::Create<HighGuid::WowAccount>(fields[4].GetUInt64());
        data.senderGuildGuid = ObjectGuid::Create<HighGuid::Guild>(fields[5].GetUInt64());
        _addonLoginMessageStore.push_back(data);
    } while (result->NextRow());
}

void FreedomMgr::AddAddonLoginMessage(std::string const& prefix, std::string_view const& message, Player* player)
{
    AddonLoginMessageData data;

    // prefix, message, senderGuid, senderAccountGuid, senderGuildGuid
    int index = 0;
    FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_INS_ADDON_LOGIN_MSG);
    stmt->setString(index++, prefix);
    stmt->setStringView(index++, message);
    stmt->setUInt64(index++, player->GetGUID().GetCounter());
    stmt->setUInt64(index++, player->GetSession()->GetAccountGUID().GetCounter());
    stmt->setUInt64(index++, player->GetGuildId());
    FreedomDatabase.DirectExecute(stmt);

    PreparedQueryResult result = FreedomDatabase.Query(FreedomDatabase.GetPreparedStatement(FREEDOM_SEL_ADDON_LOGIN_MSGS_MAX_ID));
    data.id = result->Fetch()[0].GetUInt32();
    data.prefix = prefix;
    data.message = message;
    data.senderGuid = player->GetGUID();
    data.senderAccountGuid = player->GetSession()->GetAccountGUID();
    data.senderGuildGuid = ObjectGuid::Create<HighGuid::Guild>(player->GetGuildId());

    _addonLoginMessageStore.push_back(data);
}

bool FreedomMgr::DeleteAddonLoginMessage(uint32 id)
{
    auto it = std::find_if(
        _addonLoginMessageStore.begin(), _addonLoginMessageStore.end(),
        [id](AddonLoginMessageData msg)->bool { return msg.id == id; }
    );

    if (it != _addonLoginMessageStore.end())
    {
        FreedomDatabasePreparedStatement* stmt = FreedomDatabase.GetPreparedStatement(FREEDOM_DEL_ADDON_LOGIN_MSG);
        stmt->setUInt32(0, id);
        FreedomDatabase.Execute(stmt);
        _addonLoginMessageStore.erase(it);
    }
    return it != _addonLoginMessageStore.end();
}
