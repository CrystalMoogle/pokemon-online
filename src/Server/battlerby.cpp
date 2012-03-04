#include "battlerby.h"
#include "../Shared/battlecommands.h"

using namespace BattleCommands;

BattleRBY::BattleRBY(Player &p1, Player &p2, const ChallengeInfo &additionnalData, int id, int nteam1, int nteam2, PluginManager *p)
{
    ChallengeInfo c = additionnalData;
    c.mode = ChallengeInfo::Singles;

    init(p1, p2, c, id, nteam1, nteam2, p);
}

BattleRBY::~BattleRBY()
{
    onDestroy();
}

void BattleRBY::beginTurn()
{
    turn() += 1;
    /* Resetting temporary variables */
    for (int i = 0; i < numberOfSlots(); i++) {
        turnMem(i).reset();
        tmove(i).reset();
    }

    attackCount() = 0;

    requestChoices();

    /* preventing the players from cancelling (like when u-turn/Baton pass) */
    for (int i = 0; i < numberOfSlots(); i++)
        couldMove[i] = false;

    analyzeChoices();
}

void BattleRBY::endTurn()
{

}

void BattleRBY::initializeEndTurnFunctions()
{

}

void BattleRBY::changeStatus(int player, int status, bool tell, int turns)
{
    if (poke(player).status() == status) {
        return;
    }

    //Sleep clause
    if (status != Pokemon::Asleep && currentForcedSleepPoke[this->player(player)] == currentInternalId(player)) {
        currentForcedSleepPoke[this->player(player)] = -1;
    }

    notify(All, StatusChange, player, qint8(status), turns > 0, !tell);
    notify(All, AbsStatusChange, this->player(player), qint8(this->slotNum(player)), qint8(status), turns > 0);
    poke(player).addStatus(status);

    if (turns != 0) {
        poke(player).statusCount() = turns;
    }
    else if (status == Pokemon::Asleep) {
        poke(player).statusCount() = 1 + (randint(6));
    }
    else {
        poke(player).statusCount() = 0;
    }
}

int BattleRBY::getStat(int poke, int stat)
{
    return fpoke(poke).stats[stat];
}

void BattleRBY::sendPoke(int slot, int pok, bool silent)
{
    int player = this->player(slot);
    int snum = slotNum(slot);

    /* reset temporary variables */
//    pokeMemory(slot).clear();

//    /* Reset counters */
//    counters(slot).clear();

    notify(All, SendOut, slot, silent, quint8(pok), opoke(slot, player, pok));

    team(player).switchPokemon(snum, pok);

    PokeBattle &p = poke(slot);

    /* Give new values to what needed */
    fpoke(slot).init(p, gen());

    if (p.status() != Pokemon::Asleep)
        p.statusCount() = 0;

    /* Increase the "switch count". Switch count is used to see if the pokemon has switched
       (like for an attack like attract), it is imo more effective that other means */
    slotMemory(slot).switchCount += 1;

    turnMem(slot).flags &= TurnMemory::Incapacitated;
}

BattleChoice& BattleRBY::choice(int p)
{
    return choices[p];
}

BattleChoices BattleRBY::createChoice(int slot)
{
    /* First let's see for attacks... */
    if (koed(slot)) {
        return BattleChoices::SwitchOnly(slot);
    }

    BattleChoices ret;
    ret.numSlot = slot;

    for (int i = 0; i < 4; i++) {
        if (!isMovePossible(slot,i)) {
            ret.attackAllowed[i] = false;
        }
    }

    return ret;
}

void BattleRBY::analyzeChoices()
{
    setupChoices();

    std::map<int, std::vector<int>, std::greater<int> > priorities;
    std::vector<int> switches;

    std::vector<int> playersByOrder = sortedBySpeed();

    foreach(int i, playersByOrder) {
        if (choice(i).switchChoice())
            switches.push_back(i);
        else if (choice(i).attackingChoice()){
            priorities[tmove(i).priority].push_back(i);
        } else {
            /* Shifting choice */
            priorities[0].push_back(i);
        }
    }

    foreach(int player, switches) {
        analyzeChoice(player);
        //callEntryEffects(player);

        personalEndTurn(player);
        notify(All, BlankMessage, Player1);
    }

    std::map<int, std::vector<int>, std::greater<int> >::const_iterator it;
    std::vector<int> &players = speedsVector;
    players.clear();

    for (it = priorities.begin(); it != priorities.end(); ++it) {
        foreach (int player, it->second) {
            players.push_back(player);
        }
    }

    for(unsigned i = 0; i < players.size(); i++) {
        if (!multiples()) {
            if (koed(0) || koed(1))
                break;
        } else {
            requestSwitchIns();
        }

        if (!hasMoved(players[i])) {
            analyzeChoice(players[i]);

            if (!multiples() && (koed(0) || koed(1))) {
                testWin();
                selfKoer() = -1;
                break;
            }

            personalEndTurn(players[i]);
            notify(All, BlankMessage, Player1);
        }
        testWin();
        selfKoer() = -1;
    }
}

void BattleRBY::personalEndTurn(int player)
{
    if (koed(player))
        return;

    switch(poke(player).status())
    {
    case Pokemon::Burnt:
        notify(All, StatusMessage, player, qint8(HurtBurn));
        //HeatProof: burn does only 1/16, also Gen 1 only does 1/16
        inflictDamage(player, poke(player).totalLifePoints()/16, player);
        break;
    case Pokemon::Poisoned:
        notify(All, StatusMessage, player, qint8(HurtPoison));

        if (poke(player).statusCount() == 0)
            inflictDamage(player, poke(player).totalLifePoints()/16, player); // 1/16 in gen 1
        else {
            inflictDamage(player, poke(player).totalLifePoints() * (15-poke(player).statusCount()) / 16, player);
            poke(player).statusCount() = std::max(1, poke(player).statusCount() - 1);
        }
        break;
    }

    testWin();
}


void BattleRBY::inflictDamage(int player, int damage, int source, bool straightattack, bool goForSub)
{
    if (koed(player)) {
        return;
    }

    if (damage == 0) {
        damage = 1;
    }

    bool sub = hasSubstitute(player);

    if (sub && (player != source || goForSub) && straightattack) {
        inflictSubDamage(player, damage, source);
    } else {
        damage = std::min(int(poke(player).lifePoints()), damage);

        int hp  = poke(player).lifePoints() - damage;

        if (hp <= 0) {
            koPoke(player, source, straightattack);
        } else {
            if (straightattack) {
                notify(this->player(player), StraightDamage,player, qint16(damage));
                notify(AllButPlayer, StraightDamage,player, qint16(damage*100/poke(player).totalLifePoints()));
            }

            changeHp(player, hp);
        }
    }

    if (straightattack && player != source) {
        if (!sub) {
            /* If there's a sub its already taken care of */
            turnMem(player).damageTaken = damage;
        }

        if (damage > 0) {
            inflictRecoil(source, player);
        }
    }
}
