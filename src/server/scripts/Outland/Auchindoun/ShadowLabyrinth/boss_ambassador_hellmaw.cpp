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

/* ScriptData
SDName: Boss_Ambassador_Hellmaw
SD%Complete: 80
SDComment: Enrage spell missing/not known
SDCategory: Auchindoun, Shadow Labyrinth
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "shadow_labyrinth.h"

enum Yells
{
    SAY_INTRO       = 0,
    SAY_AGGRO       = 1,
    SAY_HELP        = 2,
    SAY_SLAY        = 3,
    SAY_DEATH       = 4
};

enum Spells
{
    SPELL_BANISH            = 30231,
    SPELL_CORROSIVE_ACID    = 33551,
    SPELL_FEAR              = 33547,
    SPELL_ENRAGE            = 34970
};

enum Events
{
    EVENT_CORROSIVE_ACID = 1,
    EVENT_FEAR,
    EVENT_BERSERK
};

class boss_ambassador_hellmaw : public CreatureScript
{
    public:
        boss_ambassador_hellmaw() : CreatureScript("boss_ambassador_hellmaw") { }

        struct boss_ambassador_hellmawAI : public npc_escortAI
        {
            boss_ambassador_hellmawAI(Creature* creature) : npc_escortAI(creature)
            {
                _instance = creature->GetInstanceScript();
                _intro = false;
            }

            void Reset()
            {
                if (!me->isAlive())
                    return;

                _events.Reset();
                _instance->SetBossState(DATA_AMBASSADOR_HELLMAW, NOT_STARTED);

                _events.ScheduleEvent(EVENT_CORROSIVE_ACID, urand(5000, 10000));
                _events.ScheduleEvent(EVENT_FEAR, urand(25000, 30000));
                if (IsHeroic())
                    _events.ScheduleEvent(EVENT_BERSERK, 180000);

                DoAction(ACTION_AMBASSADOR_HELLMAW_BANISH);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (me->HasAura(SPELL_BANISH))
                    return;

                npc_escortAI::MoveInLineOfSight(who);
            }

            void WaypointReached(uint32 /*waypointId*/)
            {
            }

            void DoAction(const int32 actionId)
            {
                if (actionId == ACTION_AMBASSADOR_HELLMAW_INTRO)
                    DoIntro();
                else if (actionId == ACTION_AMBASSADOR_HELLMAW_BANISH)
                {
                    if (_instance->GetData(DATA_FEL_OVERSEER) && me->HasAura(SPELL_BANISH))
                        DoCast(me, SPELL_BANISH, true); // this will not work, because he is immune to banish
                }
            }

            void DoIntro()
            {
                if (_intro)
                    return;

                _intro = true;

                if (me->HasAura(SPELL_BANISH))
                    me->RemoveAurasDueToSpell(SPELL_BANISH);

                Talk(SAY_INTRO);
                    Start(true, false, ObjectGuid::Empty, NULL, false, true);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _instance->SetBossState(DATA_AMBASSADOR_HELLMAW, IN_PROGRESS);
                Talk(SAY_AGGRO);
            }

            void KilledUnit(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                _instance->SetBossState(DATA_AMBASSADOR_HELLMAW, DONE);
                Talk(SAY_DEATH);
            }

            void UpdateEscortAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasAura(SPELL_BANISH))
                {
                    EnterEvadeMode();
                    return;
                }

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CORROSIVE_ACID:
                            DoCastVictim(SPELL_CORROSIVE_ACID);
                            _events.ScheduleEvent(EVENT_CORROSIVE_ACID, urand(15000, 25000));
                            break;
                        case EVENT_FEAR:
                            DoCastAOE(SPELL_FEAR);
                            _events.ScheduleEvent(EVENT_FEAR, urand(20000, 35000));
                            break;
                        case EVENT_BERSERK:
                            DoCast(me, SPELL_ENRAGE, true);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
            bool _intro;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_ambassador_hellmawAI(creature);
        }
};

void AddSC_boss_ambassador_hellmaw()
{
    new boss_ambassador_hellmaw();
}