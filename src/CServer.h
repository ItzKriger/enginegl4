#pragma once
#include "CNetTransceiver.h"
#include "CComponent.h"
#include "U_TypeDefs.h"
#include "U_Networking.h"
#include "CNetMessagesManager.h"

#include <memory>
#include <unordered_map>

class CNetServerInfo : public CNetMessage
{
public:
    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;
    double Rate = 33.0;

    DEFINE_NET_MESSAGE();
};

class CServer : public CComponent
{
public:
    class CClient
    {
    public:
        CClient(const CEndPoint& point);

        bool operator==(const CClient& other) const;
        void Update();

        CEndPoint EndPoint;
        bool Authorized = false;

        std::wstring NickName;
        std::uint16_t NextPacketID = 0;
        CNetMessagesManager MessagesManager;

        CTimePoint LastPacket;
    };

    void V_PostInit() override;
    void V_Update() override;

    bool IsClientExist(const CEndPoint& endpoint);
    CClient& GetClient(const CEndPoint& endpoint);

    void ProcessDatagram(boost::asio::ip::udp::endpoint point, std::vector<std::uint8_t>& buffer, CTimePoint ptime);
    void HandleClientPacket(CClient& client, std::vector<std::uint8_t>& buffer, CTimePoint ptime);

    void SendReliableMessageTo(CClient& client, std::shared_ptr<CNetMessage> msg);
    void SendUnreliableMessageTo(CClient& client, std::shared_ptr<CNetMessage> msg);

    void SendReliableMessageToAll(std::shared_ptr<CNetMessage> msg);
    void SendUnreliableMessageToAll(std::shared_ptr<CNetMessage> msg);

    void SendServerInfoTo(CClient& client);
    void SendServerInfoToAll();

    clientid_t GetFreeID();
    void SetRate(double timesPerSecond = 33.0);
    double GetRate() const;

    std::unique_ptr<CNetTransceiver> Transceiver;
    std::unordered_map<clientid_t, CClient> Clients;

    DEFINE_SOL_USERTYPE();
    DEFINE_COMPONENT();
private:
    double m_svRate = 33.0;
    std::unordered_map<clientid_t, CClient>::iterator m_getClientIterator(const CEndPoint& endpoint);
};

namespace std
{
    template<>
    struct hash<CServer::CClient>
    {
        size_t operator()(const CServer::CClient& p) const
        {
            size_t h1 = hash<string>{}(p.EndPoint.address().to_string());
            size_t h2 = hash<int>{}(p.EndPoint.port());
            return h1 ^ (h2 << 1);
        }
    };
}
