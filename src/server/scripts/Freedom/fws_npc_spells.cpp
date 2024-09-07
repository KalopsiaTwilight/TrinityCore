#include "ScriptMgr.h"
#include "Log.h"
#include "FreedomMgr.h"

// Goes over queue to cast / uncast spells
class F_WS_npc_spells : public WorldScript
{
public:
    F_WS_npc_spells() : WorldScript("F_WS_npc_spells") { }

    void OnStartup() {
        TC_LOG_DEBUG("freedom", "F_WS_npc_spells starting up...");
    }

    void OnCreatureAddedToWorld(Creature* creature) override
    {
        sFreedomMgr->EnableNpcCastsForCreature(creature);
    }

    void OnCreatureRemovedFromWorld(Creature* creature) override
    {
        sFreedomMgr->DisableNpcCastsForCreature(creature);
    }

    void OnUpdate(uint32 diff) override {
        sFreedomMgr->CleanNpcCastsRemoveList();
        NpcCastContainer container = sFreedomMgr->GetActiveNpcCasts();
        auto it = container.begin();
        while (it != container.end()) {
            std::shared_ptr<NpcCastData> castData = (*it);
            Unit* source = sFreedomMgr->GetAnyUnit(castData->source);
            Unit* target = sFreedomMgr->GetAnyUnit(castData->target);
            if (!source || !target) {
                TC_LOG_WARN("freedom.npc.spells", "F_WS_npc_spells::OnUpdate | Target %u or source %u no longer exists for npc cast %u, disabling casting.", castData->target, castData->source, castData->id);
                sFreedomMgr->DisableNpcCast(castData);
                it++;
                continue;
            }
            if (castData->currentDuration > 0) {
                castData->currentDuration -= std::min(castData->currentDuration, diff);
                // We have reached the end of the spell duration
                if (castData->currentDuration == 0) {
                    if (castData->duration != 0) {
                        TC_LOG_DEBUG("freedom.npc.spells", "F_WS_npc_spells::OnUpdate | Stopping cast %u for creature " SZFMTD "...", castData->spell_id, castData->source);
                        sFreedomMgr->StopNpcCast(castData);
                    }
                    if (castData->restInterval > 0) {
                        castData->currentRest = castData->restInterval;
                    }
                    else {
                        sFreedomMgr->DisableNpcCast(castData, false);
                    }
                }
            }
            else {
                castData->currentRest -= std::min(castData->currentRest, diff);
                // We have reached the end of the rest interval, cast spell.
                if (castData->currentRest == 0)
                {
                    TC_LOG_DEBUG("freedom.npc.spells", "F_WS_npc_spells::OnUpdate | Starting cast %u for creature " SZFMTD "...", castData->spell_id, castData->source);
                    source->CastSpell(target, castData->spell_id);
                    castData->currentDuration = std::max(castData->duration, 1u);
                }
            }
            it++;
        }
    }
};
void AddSC_F_npc_spells()
{
    new F_WS_npc_spells();
}
