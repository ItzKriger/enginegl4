#pragma once
#include "CComponent.h"
#include "CNetTransceiver.h"
#include "U_Networking.h"
#include "CNetMessagesManager.h"
#include "U_TypeDefs.h"

#include <vector>
#include <memory>
#include <string>
#include "boost/asio.hpp"

class CClient : public CComponent
{
public:
    void V_Update() override;

    void Connect(const CEndPoint& srv);
    void Connect(const std::string& hostname);

    void Reset();

    void ProcessDatagram(boost::asio::ip::udp::endpoint point, std::vector<std::uint8_t>& buffer, CTimePoint ptime);
    void HandleServerPacket(std::vector<std::uint8_t>& buffer, CTimePoint ptime);
    
    void SendConnectionCookie();
    
    void SendReliableMessage(std::shared_ptr<CNetMessage> msg);
    void SendUnreliableMessage(std::shared_ptr<CNetMessage> msg);

    void SetServerRate(double timesPerSecond = 33.0);
    double GetServerRate() const;

    std::unordered_map<entityid_t, entityid_t> ServerEntityMap; //sv_id -> cl_id
    std::unique_ptr<CNetTransceiver> Transceiver;
    DEFINE_COMPONENT();
private:
    CNetMessagesManager NetMessagesManager;
    CEndPoint ServerPoint;

    double m_svRate = 33.0;

    bool Authorized = false;
    bool Disconnected = false;
    bool SentCookie = false;
    std::uint32_t ConnectionAttempts = 0;

    CTimePoint LastPacket;
};
