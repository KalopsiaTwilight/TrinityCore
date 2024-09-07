#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "Player.h"
#include "AreaTrigger.h"
#include "Config.h"
#include "Log.h"
#include "GameTime.h"

using namespace Trinity::ChatCommands;

class F_spell_limits : public PlayerScript
{
public:
    F_spell_limits() : PlayerScript("F_spell_limits") { }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) {
        const SpellInfo* spellInfo = spell->GetSpellInfo();
        uint32 spellId = spellInfo->Id;
        TC_LOG_DEBUG("freedom", "Player %s is casting spell %u...", player->GetName().c_str(), spellId);

        if (spellInfo->HasEffect(SPELL_EFFECT_APPLY_AURA) && spell->m_targets.GetUnitTarget() && spell->m_targets.GetUnitTarget() != player) {
            TC_LOG_DEBUG("freedom", "Player %s is affecting a different target '" SZFMTD "' with its spell", player->GetName().c_str(), spell->m_targets.GetUnitTarget()->GetGUID().GetCounter());
            // Keep track of applied auras on others for players
            sFreedomMgr->AddAuraApplication(player, spellId, spell->m_targets.GetUnitTarget());
        }

        if (spellInfo->HasEffect(SPELL_EFFECT_CREATE_AREATRIGGER) || spellInfo->IsAffectingArea()) {
            TC_LOG_DEBUG("freedom", "Player %s is casting spell %u that affects an area...", player->GetName().c_str(), spellId);

            int32 maxSame = sConfigMgr->GetIntDefault("Freedom.Spell.Area.SameLimit", 2);

            // Check if we already have an area trigger for the same id
            std::vector<AreaTrigger*> existingTriggers = player->GetAreaTriggers(spellId);
            TC_LOG_DEBUG("freedom", "Retrieved %u existing triggers for spell id: %u", existingTriggers.size(), spellId);
            if (existingTriggers.size() == maxSame) {
                AreaTrigger* trigger = existingTriggers.front();
                TC_LOG_DEBUG("freedom", "Removing area trigger with spell id: %u...", trigger->GetSpellId());
                trigger->Remove();
                return;
            }

            std::vector<DynamicObject*> existingObjects = player->GetDynObjects(spellId);
            if (existingObjects.size() == maxSame) {
                DynamicObject* dynObj = existingObjects.front();
                TC_LOG_DEBUG("freedom", "Removing dynamic object with spell id: %u...", dynObj->GetSpellId());
                dynObj->Remove();
                return;
            }

            int32 activeEffects = player->m_areaTrigger.size() + player->m_dynObj.size();
            // Check if we are at the area trigger limit and if so, remove the first one.
            if (activeEffects > sConfigMgr->GetIntDefault("Freedom.Spell.Area.TotalLimit", 5)) {
                AreaTrigger* trigger = player->m_areaTrigger.front();
                DynamicObject* dynObj = player->m_dynObj.front();

                if (!dynObj) {
                    TC_LOG_DEBUG("freedom", "Player has reached the limit, removing area trigger with spell id: %u...", trigger->GetSpellId());
                    trigger->Remove();
                    return;
                }
                if (!trigger) {
                    TC_LOG_DEBUG("freedom", "Player has reached the limit, removing dynamicobject with spell id: %u...", dynObj->GetSpellId());
                    dynObj->Remove();
                    return;
                }

                auto dynamicObjectData = dynObj->m_values.ModifyValue(&DynamicObject::m_dynamicObjectData);
                uint32 dynObjCreateTime = dynamicObjectData.ModifyValue(&UF::DynamicObjectData::CastTime).GetValue();
                if (trigger->GetTimeSinceCreated() > (GameTime::GetGameTimeMS() - dynObjCreateTime)) {
                    TC_LOG_DEBUG("freedom", "Player has reached the limit, removing area trigger with spell id: %u...", trigger->GetSpellId());
                    trigger->Remove();
                }
                else {
                    TC_LOG_DEBUG("freedom", "Player has reached the limit, removing dynamicobject with spell id: %u...", dynObj->GetSpellId());
                    dynObj->Remove();
                }
            }
        }

        if (spellInfo->HasEffect(SPELL_EFFECT_TRANS_DOOR) || spellInfo->HasEffect(SPELL_EFFECT_SUMMON)) {
            TC_LOG_DEBUG("freedom", "Player %s is casting spell %u that summons something...", player->GetName().c_str(), spellId);

            int32 maxSame = sConfigMgr->GetIntDefault("Freedom.Spell.Summon.SameLimit", 3);

            // Check if we already have an area trigger for the same id
            std::vector<GameObject*> existingObjs = player->GetGameObjects(spellId);
            TC_LOG_DEBUG("freedom", "Retrieved %u existing gameobjects for spell id: %u", existingObjs.size(), spellId);
            if (existingObjs.size() == maxSame) {
                GameObject* obj = existingObjs.front();
                TC_LOG_DEBUG("freedom", "Removing gameobject with spell id: %u...", obj->GetSpellId());
                player->RemoveGameObject(obj, true);
                return;
            }

            int32 activeEffects = player->m_gameObj.size();
            TC_LOG_DEBUG("freedom", "Player is at %u effects.", activeEffects);

            // Check if we are at the area trigger limit and if so, remove the first one.
            if (activeEffects > sConfigMgr->GetIntDefault("Freedom.Spell.Summon.TotalLimit", 8)) {
                GameObject* obj = player->m_gameObj.front();
                TC_LOG_DEBUG("freedom", "Removing gameobject with spell id: %u...", obj->GetSpellId());
                player->RemoveGameObject(obj, true);
                return;
            }
        }

    }
};

class F_denycast_spellscript_loader : public SpellScriptLoader
{
public:
    F_denycast_spellscript_loader() : SpellScriptLoader("F_denycast_spellscript_loader") { }

    class F_denycast_spellscript : public SpellScript
    {
        PrepareSpellScript(F_denycast_spellscript);

        SpellCastResult HandleCheckCast()
        {
            TC_LOG_DEBUG("freedom", "Denying spell cast for spell: %u", GetSpell()->GetSpellInfo()->Id);
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_NONE);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(F_denycast_spellscript::HandleCheckCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new F_denycast_spellscript();
    }
};


void AddSC_F_spell_limits()
{
    new F_spell_limits();
    new F_denycast_spellscript_loader();
}
