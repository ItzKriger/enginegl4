#include "CNetMessagesManager.h"
#include "CEngine.h"

#include "boost/crc.hpp"

#include <algorithm>

namespace
{
    bool SeqLess(std::uint16_t a, std::uint16_t b)
    {
        return static_cast<std::int16_t>(a - b) < 0; //difference limit is 32767
    }
}

CNetMessagesManager::CNetMessagesManager()
{
    Reports.reserve(1024); //TODO hardcoded
}

void CNetMessagesManager::AddMessage(std::shared_ptr<CNetMessage> msg, bool reliable)
{
    if(reliable)
    {
        ReliableQueue.push_back(msg);
    }
    else
    {
        UnreliableQueue.push_back(msg);
    }
}

void CNetMessagesManager::SendReliable(std::shared_ptr<CNetMessage> msg)
{
    AddMessage(msg, true);
}

void CNetMessagesManager::SendUnreliable(std::shared_ptr<CNetMessage> msg)
{
    AddMessage(msg, false);
}

void CNetMessagesManager::AddReceivedPacket(std::vector<std::uint8_t>&& buf, CTimePoint _nettime)
{
    CPacket packet;
    
    packet.Buffer = std::move(buf);
    packet.NetTime = _nettime;

    ReceivedQueue.push_back(std::move(packet));
}

void CNetMessagesManager::ProcessPackets()
{
    std::sort(ReceivedQueue.begin(), ReceivedQueue.end(), [](auto& a, auto& b) -> bool
    {
        if(a.Buffer.size() < 2 || b.Buffer.size() < 2) { return false; }

        packetid_t id_a = *reinterpret_cast<packetid_t*>(&a.Buffer.front());
        packetid_t id_b = *reinterpret_cast<packetid_t*>(&b.Buffer.front());

        return SeqLess(id_a, id_b);
        //return id_a < id_b;
    });

    for(auto& packet : ReceivedQueue)
    {
        ProcessSinglePacket(packet);
    }
    ReceivedQueue.clear();
}

//sv
//adding reliable - adding to ReliableQueue (SendReliable)
//sending reliable - moving to SentMessages

//cl
//received reliable - sending Report

//sv
//received report

bool CNetMessagesManager::ProcessSinglePacket(CPacket& packet)
{
    size_t offset = sizeof(CPacketHeader);
    if(packet.Buffer.size() <= offset) { Log::ErrInstance() << "[DROP] packet is too small\n"; return false; } //drop packet (packet is too small)

    CPacketHeader* header = reinterpret_cast<CPacketHeader*>(&packet.Buffer.front());

    //size 12
    //offset 8
    //12 - 8 = 4
    //0 1 2 3 4 5 6 7 8 9 10 11
    //                x x x  x

    boost::crc_32_type result;
    result.process_bytes(&packet.Buffer[offset], packet.Buffer.size() - offset);

    auto packet_crc = result.checksum();
    if(packet_crc != header->Checksum)
    {
        //TODO failed checksum
        Log::ErrInstance() << "[DROP] failed checksum\n";
        return false; //drop packet (failed checksum)
    }

    CBufferWrapper wrapper(packet.Buffer);
    wrapper.SetSeek(sizeof(CPacketHeader));

    std::uint16_t reportsCount = wrapper.Read<std::uint16_t>();

    std::vector<CReliableReport> receivedReports;
    for(std::uint16_t i = 0; i < reportsCount; i++)
    {
        auto report = wrapper.Read<CReliableReport>();
        receivedReports.push_back(report);
    }

    std::vector<netmsgid_t> sentMessagesToRemove;
    for(auto& sentMessage : SentMessages)
    {
        auto it = std::find_if(receivedReports.begin(), receivedReports.end(), [&sentMessage](CReliableReport& report) -> bool { return report.ID == sentMessage.second.ID; });
        if(it == receivedReports.end())
        {
            sentMessage.second.SkippedPackets++;
        }
        else if(it->Checksum == sentMessage.second.Checksum && it->Type == sentMessage.second.Type)
        {
            sentMessagesToRemove.push_back(sentMessage.second.ID);
        }
    }

    for(auto id : sentMessagesToRemove)
    {
        std::erase_if(SentMessages, [id](auto& kv) -> bool { return kv.second.ID == id; });
    }

    std::uint16_t resentCount = wrapper.Read<std::uint16_t>();
    std::uint16_t reliableCount = wrapper.Read<std::uint16_t>();
    std::uint16_t unreliableCount = wrapper.Read<std::uint16_t>();

    //Log::Instance() << "ProcessSinglePacket " << UserData << " unrealiable " << unreliableCount << Log::Endl;
    auto& netFactory = CEngine::GetInstance()->NetMessagesFactory;

    for(std::uint16_t i = 0; i < resentCount; i++)
    {
        auto msgId = wrapper.Read<netmsgid_t>();
        auto it = std::find_if(ReliablesMessages.begin(), ReliablesMessages.end(), [msgId](auto& msg) -> bool { return msg.ID == msgId; });

        if(it != ReliablesMessages.end())
        {
            //TODO message duplicate!!
        }

        auto msgType = wrapper.Read<std::uint16_t>();

        auto msgTypeStr = netFactory.GetByID(msgType);
        if(msgTypeStr.empty()) { Log::ErrInstance() << "[DROPRESEND] invalid msg type 1\n"; return false; } //drop packet (invalid msg type)

        auto msg = netFactory.create<std::unique_ptr<CNetMessage>>(msgTypeStr);
        if(!msg) { Log::ErrInstance() << "[DROPRESEND] invalid msg type 2\n"; return false; } //drop packet (invalid msg type)

        auto msgCrc = msg->Read(wrapper);
        auto msgSize = msg->GetLastReadSize();

        CReceivedReliableMessage reliable;

        reliable.ID = msgId;
        reliable.Crc = msgCrc;
        reliable.Size = msgSize;
        reliable.Message = std::move(msg);

        CReliableReport report;

        report.Checksum = msgCrc;
        report.ID = msgId;
        report.Type = msgType;

        Reports.push_back(std::move(report));
        ReliablesMessages.push_back(std::move(reliable));
    }

    for(std::uint16_t i = 0; i < reliableCount; i++)
    {
        auto msgId = wrapper.Read<netmsgid_t>();
        auto it = std::find_if(ReliablesMessages.begin(), ReliablesMessages.end(), [msgId](auto& msg) -> bool { return msg.ID == msgId; });

        if(it != ReliablesMessages.end())
        {
            //TODO message duplicate!!
        }

        auto msgType = wrapper.Read<std::uint16_t>();

        auto msgTypeStr = netFactory.GetByID(msgType);
        if(msgTypeStr.empty()) { Log::ErrInstance() << "[DROP] invalid msg type 1\n"; return false; } //drop packet (invalid msg type)

        auto msg = netFactory.create<std::unique_ptr<CNetMessage>>(msgTypeStr);
        if(!msg) { Log::ErrInstance() << "[DROP] invalid msg type 2\n"; return false; } //drop packet (invalid msg type)

        auto msgCrc = msg->Read(wrapper);
        auto msgSize = msg->GetLastReadSize();

        CReceivedReliableMessage reliable;

        reliable.ID = msgId;
        reliable.Crc = msgCrc;
        reliable.Size = msgSize;
        reliable.Message = std::move(msg);

        CReliableReport report;

        report.Checksum = msgCrc;
        report.ID = msgId;
        report.Type = msgType;

        Reports.push_back(std::move(report));
        ReliablesMessages.push_back(std::move(reliable));
    }

    for(std::uint16_t i = 0; i < unreliableCount; i++)
    {
        auto msgType = wrapper.Read<std::uint16_t>();
        //Log::Instance() << "Reading unreliable" << Log::Endl;

        auto msgTypeStr = netFactory.GetByID(msgType);
        if(msgTypeStr.empty()) { break; } //(invalid msg type) -- unreliable, not dropping the packet

        auto msg = netFactory.create<std::unique_ptr<CNetMessage>>(msgTypeStr);
        if(!msg) { break; } //(invalid msg type) -- unreliable, not dropping the packet

        msg->Read(wrapper);

        //Log::Instance() << "Unreliable is " << msg->GetType() << Log::Endl;

        msg->NetTime = packet.NetTime;

        UnreliablesMessages.push_back(std::move(msg));
    }

    return true;
}

void CNetMessagesManager::ProcessMessages()
{
    std::sort(ReliablesMessages.begin(), ReliablesMessages.end(), [](auto& a, auto& b) -> bool
    {
        return SeqLess(a.ID, b.ID);
        //return a.ID < b.ID;
    });

    for(auto& msg : UnreliablesMessages)
    {
        msg->Process();
    }

    for(auto& msg : ReliablesMessages)
    {
        msg.Message->Process();
    }

    ReliablesMessages.clear();
    UnreliablesMessages.clear();
}

bool CNetMessagesManager::PackSnapshot(std::vector<std::uint8_t>& buf)
{
    auto diff = CEngine::GetInstance()->Time.GetCurrent() - LastPack;
    if(PackDelay != 0.0 && diff < std::chrono::duration<double>(PackDelay)) { return false; }

    CBufferWrapper packet(buf);

    packet.Write<packetid_t>(PacketID);
    packet.Write<crc32_t>(0); //placeholder

    packet.Write<std::uint16_t>(Reports.size()); //TODO: implement (reports_count)

    auto& factory = CEngine::GetInstance()->NetMessagesFactory;

    for(auto& report : Reports) //implemented (reports)
    {
        packet.Write<std::uint16_t>(report.ID);
        packet.Write<std::uint16_t>(report.Type);
        packet.Write<crc32_t>(report.Checksum);
    }
    Reports.clear();

    size_t maxSkipped = 10; //TODO not hardcoded
    size_t maxResent = 10; //TODO not hardcoded

    for(auto& kv : SentMessages)
    {
        auto& sentMessage = kv.second;
        if(sentMessage.SkippedPackets >= maxSkipped && sentMessage.Resent >= maxResent)
        {
            //TODO disconnect
        }
    }

    std::erase_if(SentMessages, [&maxSkipped, &maxResent](auto& kv) -> bool
    {
        auto& sentMessage = kv.second;
        return sentMessage.SkippedPackets >= maxSkipped && sentMessage.Resent >= maxResent; //TODO disconnect
    });

    std::vector<CSentMessage*> toResend;
    for(auto& kv : SentMessages)
    {
        auto msgId = kv.first;
        auto& sentMessage = kv.second;

        if(sentMessage.SkippedPackets >= maxSkipped)
        {
            toResend.push_back(&kv.second);
            //sentMessage.Resent++;
        }
    }

    packet.Write<std::uint16_t>(toResend.size()); //implemented (resend_reliable_count)
    packet.Write<std::uint16_t>(ReliableQueue.size()); //implemented (reliable_count)
    packet.Write<std::uint16_t>(UnreliableQueue.size());

    for(auto msg : toResend)
    {
        msg->SkippedPackets = 0;
        msg->Resent++;

        netmsgid_t ID = msg->ID;

        packet.Write<std::uint16_t>(ID);
        auto msgType = factory.GetIndex(msg->Message->GetType());

        packet.Write<std::uint16_t>(msgType);
        crc32_t crc = msg->Message->Write(packet);
    }

    for(auto& msg : ReliableQueue) //implemented (reliable_data)
    {
        netmsgid_t ID = MsgID;
        MsgID++;

        packet.Write<std::uint16_t>(ID);
        auto msgType = factory.GetIndex(msg->GetType());

        packet.Write<std::uint16_t>(msgType);
        crc32_t crc = msg->Write(packet);

        CSentMessage sent;

        sent.Checksum = crc;
        sent.ID = ID;
        sent.Message = msg;
        sent.Type = msgType;
        sent.WriteTime = CEngine::GetInstance()->Time.GetCurrent();

        SentMessages.emplace(ID, std::move(sent));
    }
    
    for(auto& msg : UnreliableQueue)
    {
        //Log::Instance() << "Sending unreliable " << msg->GetType() << Log::Endl;
        auto msgType = factory.GetIndex(msg->GetType());

        packet.Write<std::uint16_t>(msgType);
        msg->Write(packet);
    }

    size_t offset = sizeof(CPacketHeader);
    if(buf.size() <= offset) { Log::ErrInstance() << "[SENDDROP] packet is too small\n"; return false; } //drop packet (packet is too small)

    CPacketHeader* header = reinterpret_cast<CPacketHeader*>(&buf.front());

    boost::crc_32_type result;
    result.process_bytes(&buf[offset], buf.size() - offset);

    header->Checksum = result.checksum();
    PacketID++;

    ReliableQueue.clear();
    UnreliableQueue.clear();

    LastPack = CEngine::GetInstance()->Time.GetCurrent();
    return true;
}
