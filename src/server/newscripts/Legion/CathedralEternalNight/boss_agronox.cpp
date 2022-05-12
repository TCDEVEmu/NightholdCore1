/*
 *###############################################################################
 *#                                                                             #
 *# Copyright (C) 2022 Project Nighthold <https://github.com/ProjectNighthold>  #
 *#                                                                             #
 *# This file is free software; as a special exception the author gives         #
 *# unlimited permission to copy and/or distribute it, with or without          #
 *# modifications, as long as this notice is preserved.                         #
 *#                                                                             #
 *# This program is distributed in the hope that it will be useful, but         #
 *# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the      #
 *# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    #
 *#                                                                             #
 *# Read the THANKS file on the source root directory for more info.            #
 *#                                                                             #
 *###############################################################################
 */

#include "cathedral_of_eternal_night.h"

enum Says
{
    SAY_AGGRO               = 0,
    SAY_FLUMINATING_DEBUFF  = 1,
    SAY_VINES               = 2,
    SAY_DEATH               = 3,
}; 

enum Spells
{
    SPELL_TIMBER_SMASH                  = 235751, // 5 + 31
    SPELL_FULMINATING_LASHERS           = 236527, // 10 + 30 (only 2 times)
    SPELL_FULMINATING_LASHERS_DEBUFF    = 236524, // 10 + 20 
    
    // mythic
    SPELL_SUCCULENT_LASHERS             = 236639, // 18 +/- 36
    SPELL_NATURE_HEAL                   = 242199, //! i don't know, where is it use
    
    // adds
    SPELL_FOCUS                         = 243613,
    SPELL_CHOKING_VINES_JUMP            = 236651, // 24 + 30 (+12)
    SPELL_CHOKING_VINES_DMG             = 238598,
};

Position const adds_pos[5]
{
    {-500.26f, 2541.83f, 388.13f, 4.49f},
    {-497.23f, 2534.99f, 388.13f, 4.01f},
    {-536.12f, 2551.06f, 388.13f, 4.89f},
    {-545.49f, 2548.70f, 388.13f, 4.99f},
    {-548.39f, 2539.13f, 388.13f, 5.13f}
};

Position const vines_pos[4]
{
    {-500.55f, 2536.22f, 388.27f, 4.31f},
    {-550.22f, 2534.58f, 388.13f, 5.28f},
    {-550.64f, 2501.10f, 388.17f, 5.98f},
    {-493.73f, 2507.01f, 388.16f, 3.95f}    
};

enum eEvents
{
    EVENT_TIMBER_SMASH      = 1,
    EVENT_FLUMINATING,
    EVENT_FLUMINATING_2,
    EVENT_FLUMINATING_DEBUFF,
    EVENT_CHOKING_VINES,
   
    EVENT_SUCCULENT_LAHSERS
};

// 117193
class boss_agronox : public CreatureScript
{
public:
    boss_agronox() : CreatureScript("boss_agronox") {}

    struct boss_agronoxAI : public BossAI
    {
        boss_agronoxAI(Creature* creature) : BossAI(creature, DATA_AGRONOX) {}

        void Reset() override
        {
            _Reset();
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FULMINATING_LASHERS_DEBUFF);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            Talk(SAY_AGGRO);
            _EnterCombat();

            events.RescheduleEvent(EVENT_TIMBER_SMASH, 5000); 
            events.RescheduleEvent(EVENT_FLUMINATING, 10000);
            events.RescheduleEvent(EVENT_FLUMINATING_2, 40000);
            events.RescheduleEvent(EVENT_FLUMINATING_DEBUFF, 10000);
            events.RescheduleEvent(EVENT_CHOKING_VINES, 24000);
            
            if (GetDifficultyID() != DIFFICULTY_HEROIC) // mythic and other. normal mode isn't exist
                events.RescheduleEvent(EVENT_SUCCULENT_LAHSERS, 18000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            _JustDied();
            
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FULMINATING_LASHERS_DEBUFF);
        }
        
        void JustSummoned(Creature* summon) override
        {            
            summons.Summon(summon);
            DoZoneInCombat(summon, 150.0f);
            
            summon->RemoveAurasDueToSpell(175057);
            
            switch(summon->GetEntry())
            {
                case NPC_FLUMINATING_LASHERS:
                    if (Unit* target = summon->GetTargetUnit())
                    {
                        summon->GetMotionMaster()->MoveFollow(target, 0, summon->GetFollowAngle());
                        summon->CastSpell(target, SPELL_FOCUS);
                    }
                    break;
                case NPC_CHOKING_VINES:
                    summon->SetControlled(true, UNIT_STATE_ROOT);
                    if (Unit* target = summon->GetTargetUnit())
                    {
                        target->CastSpell(summon, SPELL_CHOKING_VINES_JUMP);
                        summon->AddDelayedEvent(1000, [summon] () -> void
                        {
                            if (Unit* target = summon->GetTargetUnit()) // it's better, than use target, as ready
                                summon->CastSpell(target, SPELL_CHOKING_VINES_DMG);
                        });
                    }
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {

                    case EVENT_TIMBER_SMASH:
                        DoCastVictim(SPELL_TIMBER_SMASH);
                        events.RescheduleEvent(EVENT_TIMBER_SMASH, 31000);
                        break;
                    case EVENT_FLUMINATING:
                    case EVENT_FLUMINATING_2:
                    {
                        DoCast(SPELL_FULMINATING_LASHERS); // visual
                        
                        uint8 cur_position = 0;
                        Map::PlayerList const& pPlayers = me->GetMap()->GetPlayers();
                        for (Map::PlayerList::const_iterator itr = pPlayers.begin(); itr != pPlayers.end(); ++itr)
                            if (Player* player = itr->getSource())
                                if (Creature* add = me->SummonCreature(NPC_FLUMINATING_LASHERS, adds_pos[cur_position++],TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000))
                                    add->SetTargetGUID(player->GetGUID());
                        break;
                    }
                    case EVENT_FLUMINATING_DEBUFF:
                        DoCastAOE(SPELL_FULMINATING_LASHERS_DEBUFF);
                        events.RescheduleEvent(EVENT_FLUMINATING_DEBUFF, 20000);
                        break;
                    case EVENT_CHOKING_VINES:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            if (Player* player = target->ToPlayer())
                                if (Creature* add = me->SummonCreature(NPC_CHOKING_VINES, vines_pos[urand(0,3)], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000))
                                    add->SetTargetGUID(player->GetGUID());
                        
                        Talk(SAY_VINES);
                        events.RescheduleEvent(EVENT_CHOKING_VINES, 35000); 
                        break;
                    case EVENT_SUCCULENT_LAHSERS:
                        DoCast(SPELL_SUCCULENT_LASHERS);
                        events.RescheduleEvent(EVENT_SUCCULENT_LAHSERS, urand(30000, 42000)); // real random ?
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_agronoxAI (creature);
    }
};

// 238598
class spell_agronox_choking_vines : public SpellScriptLoader
{
    public:
        spell_agronox_choking_vines() : SpellScriptLoader("spell_agronox_choking_vines") { }

        class spell_agronox_choking_vines_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_agronox_choking_vines_AuraScript);

            void OnTick(AuraEffect const* /*aurEff*/)
            {
                if (Unit* target = GetTarget())
                    if (Unit* caster = GetCaster())
                        if (caster->GetDistance2d(target) >= 20.0f)
                        {
                            caster->RemoveAurasDueToSpell(GetSpellInfo()->Id);
                            target->RemoveAurasDueToSpell(GetSpellInfo()->Id);
                        }
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_agronox_choking_vines_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_agronox_choking_vines_AuraScript();
        }
};


void AddSC_boss_agronox()
{
    new boss_agronox();
    new spell_agronox_choking_vines();
}