#include "networkstructs.h"
#include "../Utilities/coreclasses.h"
#include "../Shared/config.h"
#include "../Shared/networkcommands.h"

DataStream &operator << (DataStream &out, const BasicInfo& team)
{
    out << team.name;
    out << team.info;

    return out;
}

DataStream &operator >> (DataStream &in, BasicInfo& team)
{
    in >> team.name;
    in >> team.info;

    /* To avoid server overloads */
    if (team.info.length() > 250)
        team.info.resize(250);

    return in;
}


DataStream & operator >> (DataStream &in, PlayerInfo &p)
{
    in >> p.id;
    in >> p.team;
    in >> p.auth;
    in >> p.flags;
    in >> p.rating;

    in >> p.avatar;
    in >> p.tier;
    in >> p.color;
    in >> p.gen;

    return in;
}

DataStream & operator << (DataStream &out, const PlayerInfo &p)
{
    out << p.id;
    out << p.team;
    out << p.auth;
    out << p.flags;
    out << p.rating;

    out << p.avatar;
    out << p.tier;
    out << p.color;
    out << p.gen;

    return out;
}

Battle::Battle(int id1, int id2) : id1(id1), id2(id2)
{

}

DataStream & operator >> (DataStream &in, Battle &p)
{
    //in >> p.battleid >> p.id1 >> p.id2;
    in >> p.id1 >> p.id2;

    return in;
}

DataStream & operator << (DataStream &out, const Battle &p)
{
    //out << p.battleid << p.id1 << p.id2;
    out << p.id1 << p.id2;

    return out;
}

ProtocolVersion::ProtocolVersion()
{
    version = PROTOCOL_VERSION;
    subversion = PROTOCOL_SUBVERSION;
}

DataStream &operator >> (DataStream &in, ProtocolVersion &p)
{
    in >> p.version >> p.subversion;
    return in;
}

DataStream &operator << (DataStream &out, const ProtocolVersion &p)
{
    out << p.version << p.subversion;
    return out;
}

TrainerInfo::TrainerInfo() : avatar(0) {
}

DataStream & operator >> (DataStream &in, TrainerInfo &i)
{
    VersionControl v;
    in >> v;

    if (i.version != v.versionNumber) {
        return in;
    }

    Flags network;
    v.stream >> network >> i.avatar >> i.info;

    if (network[TrainerInfo::HasWinningMessages]) {
        v.stream >> i.winning >> i.losing >> i.tie;
    }

    if (i.info.length() > 300) {
        i.info.resize(300);
    }
    if (i.winning.length() > 200) {
        i.winning.resize(200);
    }
    if (i.losing.length() > 200) {
        i.losing.resize(200);
    }
    if (i.tie.length() > 200) {
        i.tie.resize(200);
    }

    return in;
}

DataStream & operator << (DataStream &out, const TrainerInfo &i)
{
    VersionControl v(i.version);
    Flags network;

    bool messages = !i.winning.isEmpty() || !i.losing.isEmpty() || !i.tie.isEmpty();

    network.setFlag(TrainerInfo::HasWinningMessages, messages);

    v.stream << network << i.avatar << i.info;
    if (messages) {
        v.stream << i.winning << i.losing << i.tie;
    }

    out << v;
    return out;
}


DataStream & operator << (DataStream & out, const PersonalTeam & team)
{
    out << quint8(team.gen());

    for(int index = 0;index<6;index++)
    {
        const PokePersonal & poke = team.poke(index);
        out << poke;
    }

    return out;
}

DataStream & operator >> (DataStream & in, PersonalTeam & team)
{
    quint8 gen;

    in >> gen;

    team.setGen(gen);

    for(int i=0;i<6;i++)
    {
        in >> team.poke(i);
    }

    return in;
}

PersonalTeam::PersonalTeam(): m_gen(GEN_MAX)
{
}

void PersonalTeam::setGen(int gen)
{
    if (this->gen() == gen)
        return;

    m_gen = gen;

    for (int i = 0; i < 6; i++) {
        poke(i).gen() = gen;
    }
}

LoginInfo::LoginInfo() : teams(0), channel(0), additionalChannels(0), trainerInfo(0), plugins(0)
{
}

LoginInfo::~LoginInfo()
{
    delete teams, delete channel, delete additionalChannels, delete trainerInfo, delete plugins;
}

DataStream & operator >> (DataStream &in, LoginInfo &l)
{
//                HasClientType = 0,
//                HasVersionNumber,
//                HasReconnect,
//                HasDefaultChannel,
//                HasAdditionalChannels,
//                HasColor,
//                HasTrainerInfo,
//                /* markerbit = 7 */
//                HasTeams = 8,
//                HasEventSpecification,
//                HasPluginList
    l.clientVersion = 0;
    in >> l.version >> l.network;

#define test(variable, flag) if (l.network[LoginCommand::flag]) { in >> l.variable;}
#define load(variable, flag) if (l.network[LoginCommand::flag]) { l.variable = new std::remove_reference<decltype(*l.variable)>::type(); in >> *l.variable;}

    test(clientType, HasClientType);
    test(clientVersion, HasVersionNumber);
    in >> l.trainerName;
    in >> l.data;
    test(reconnectBits, HasReconnect);

    load(channel, HasDefaultChannel);
    load(additionalChannels, HasAdditionalChannels);
    test(trainerColor, HasColor);
    load(trainerInfo, HasTrainerInfo);

    if (l.network[LoginCommand::HasTeams]) {
        l.teams = new QList<PersonalTeam>();
        qint8 count;
        in >> count;
        for (int i = 0; i < count; i++) {
            PersonalTeam t;
            in >> t;
            /* 6 teams tops */
            if (i < 6) {
                l.teams->push_back(t);
            }
        }
    }

    test(events, HasEventSpecification);
    load(plugins, HasPluginList);
#undef load
#undef test

    return in;
}
