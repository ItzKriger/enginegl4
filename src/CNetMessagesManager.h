#pragma once
#include "U_Typedefs.h"
#include "CNetMessage.h"

#include "CTime.h"

class CNetMessagesManager
{
public:
    CNetMessagesManager();

    #pragma pack(push, 1)
    struct CPacketHeader
    {
        packetid_t ID;
        crc32_t Checksum;
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct CReliableReport
    {
        netmsgid_t ID;
        std::uint16_t Type;
        crc32_t Checksum;
    };
    #pragma pack(pop)

    struct CSentMessage
    {
        netmsgid_t ID;
        std::uint16_t Type;
        std::shared_ptr<CNetMessage> Message;
        crc32_t Checksum;
        CTimePoint WriteTime;

        size_t SkippedPackets = 0;
        size_t Resent = 0;
    };

    struct CReceivedReliableMessage
    {
        netmsgid_t ID;
        crc32_t Crc;
        size_t Size;
        std::unique_ptr<CNetMessage> Message;
    };

    struct CPacket
    {
        CTimePoint NetTime;
        std::vector<std::uint8_t> Buffer;
    };

    std::vector<CReliableReport> Reports; //reports from received messages
    std::vector<CReceivedReliableMessage> ReliablesMessages; //received
    std::vector<CReceivedReliableMessage> ResentMessages; //received
    std::vector<std::unique_ptr<CNetMessage>> UnreliablesMessages; //received
    
    std::unordered_map<netmsgid_t, CSentMessage> SentMessages; //sent

    void ProcessPackets();
    void ProcessMessages();

    bool ProcessSinglePacket(CPacket& packet);

    void AddReceivedPacket(std::vector<std::uint8_t>&& buf, CTimePoint _nettime);

    void AddMessage(std::shared_ptr<CNetMessage> msg, bool reliable = true); //to send
    void SendReliable(std::shared_ptr<CNetMessage> msg);
    void SendUnreliable(std::shared_ptr<CNetMessage> msg);

    bool PackSnapshot(std::vector<std::uint8_t>& buf);

    std::vector<CPacket> ReceivedQueue; //received
    std::vector<std::shared_ptr<CNetMessage>> ReliableQueue; //to send
    std::vector<std::shared_ptr<CNetMessage>> UnreliableQueue; //to send

    //std::vector<CReliableReport> Reports;

    packetid_t PacketID = 0;
    netmsgid_t MsgID = 0;

    CTimePoint LastPack;
    double PackDelay = (1.0 / 33.0); //TODO not hardcoded

    size_t UserData = 1488;
};
