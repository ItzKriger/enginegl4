#include "CServer.h"
#include "CEngine.h"
#include "CConVarManager.h"
#include "CBufferWrapper.h"
#include "U_Networking.h"
#include "CClient.h"

CServer::CClient::CClient(const CEndPoint& point) : EndPoint(point) {}

void CServer::V_PostInit()
{
    unsigned short port = Networking::HostPort;
    COMPONENT_CALL_GET(port, CConVarManager, GetConVarValue<unsigned short>("net.hostport", Networking::HostPort));

    Transceiver = std::make_unique<CNetTransceiver>(port);
    Transceiver->Queue.Enabled = true;

    Transceiver->StartAsync();
}

void CServer::ProcessDatagram(boost::asio::ip::udp::endpoint point, std::vector<std::uint8_t>& buffer, CTimePoint ptime)
{
    //Log::Instance() << "[SV] There's a datagram from " << point.address().to_string() << ":" << point.port() << Log::Endl;

    CBufferWrapper packet(buffer);
    if(IsClientExist(point))
    {
        auto& client = GetClient(point);
        client.LastPacket = ptime;

        if(!client.Authorized)
        {
            auto message = packet.ReadLenString<std::uint8_t, std::string>();
            if(message == "EngineGL 5.0 Ready For Transmit")
            {
                client.Authorized = true;
                Log::Instance() << "Client authorized!\n";
                SendServerInfoTo(client);
            }
        }
        else
        {
            HandleClientPacket(client, buffer, ptime);
        }
    }
    else
    {
        auto message = packet.ReadLenString<std::uint8_t, std::string>();
        if(message == "EngineGL 5.0 Connection Cookie")
        {
            auto nickname = packet.ReadLenString<std::uint8_t, std::u16string>();
            auto id = GetFreeID();

            CClient client(point);
            client.NickName = StringUtils::U16ToWstr(nickname);
            client.LastPacket = ptime;
            client.MessagesManager.UserData = static_cast<size_t>(id);
            client.MessagesManager.PackDelay = 1.0 / m_svRate;

            Clients.emplace(id, std::move(client));
            
            std::vector<std::uint8_t> answer_buf;
            CBufferWrapper answer(answer_buf);

            answer.WriteLenString<std::uint8_t, std::string>("EngineGL 5.0 Connection Accepted");

            Transceiver->SendAsync(point, answer_buf);

            Log::Instance() << "[Server] Client #" << id << " connected from " << point.address().to_string() << ":" << point.port();
            Log::Instance() << " with a nickname of \"" << StringUtils::U16ToWstr(nickname) << "\"\n";
        }
    }
}

void CServer::HandleClientPacket(CClient& client, std::vector<std::uint8_t>& buffer, CTimePoint ptime)
{
    client.MessagesManager.AddReceivedPacket(std::move(buffer), ptime);
}

bool CServer::CClient::operator==(const CClient& other) const
{
    return EndPoint == other.EndPoint;
}

void CServer::SendReliableMessageTo(CClient& client, std::shared_ptr<CNetMessage> msg)
{
    client.MessagesManager.SendReliable(msg);
}

void CServer::SendUnreliableMessageTo(CClient& client, std::shared_ptr<CNetMessage> msg)
{
    client.MessagesManager.SendUnreliable(msg);
}

void CServer::SendReliableMessageToAll(std::shared_ptr<CNetMessage> msg)
{
    for(auto& kv : Clients)
    {
        auto& client = kv.second;
        client.MessagesManager.SendReliable(msg); //DONE msg used to be invalid after first send
    }
}

void CServer::SendUnreliableMessageToAll(std::shared_ptr<CNetMessage> msg)
{
    for(auto& kv : Clients)
    {
        auto& client = kv.second;
        client.MessagesManager.SendUnreliable(msg); //DONE msg used to be invalid after first send
    }
}

void CServer::SendServerInfoTo(CClient& client)
{
    auto msg = std::make_shared<CNetServerInfo>();
    SendReliableMessageTo(client, msg);
}

void CServer::SendServerInfoToAll()
{
    auto msg = std::make_shared<CNetServerInfo>();
    SendReliableMessageToAll(msg);
}

void CServer::SetRate(double timesPerSecond)
{
    m_svRate = timesPerSecond;
    for(auto& kv : Clients)
    {
        auto& client = kv.second;
        client.MessagesManager.PackDelay = 1.0 / m_svRate;
    }
    SendServerInfoToAll();
}

void CServer::CClient::Update()
{
    if(Authorized)
    {
        auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
        MessagesManager.ProcessPackets();
        MessagesManager.ProcessMessages();

        std::vector<std::uint8_t> snapshot;
        if(MessagesManager.PackSnapshot(snapshot))
        {
            server->Transceiver->SendAsync(EndPoint, snapshot);
        }
    }
}

void CServer::V_Update()
{
    std::uint32_t timeout = 20000;
    COMPONENT_CALL_GET(timeout, CConVarManager, GetConVarValue<std::uint32_t>("net.timeout", 20000));

    std::vector<clientid_t> ToKick;
    for(auto& kv : Clients)
    {
        auto& client = kv.second;
        auto diff = CEngine::GetInstance()->Time.GetCurrent() - client.LastPacket;

        if(diff >= std::chrono::milliseconds(timeout))
        {
            ToKick.push_back(kv.first);
        }
    }

    for(auto tokick : ToKick)
    {
        //TODO kick client
    }

    std::vector<CNetTransceiver::CPacket> packets;
    {
        std::lock_guard<std::mutex> _lock(Transceiver->Queue.Mutex);
        packets.swap(Transceiver->Queue.Container);
    }

    for(auto& pack : packets)
    {
        ProcessDatagram(pack.From, pack.Data, pack.ReceivedAt);
    }

    for(auto& kv : Clients)
    {
        auto& client = kv.second;
        client.Update();
    }
}

clientid_t CServer::GetFreeID()
{
    clientid_t max = std::numeric_limits<clientid_t>::max();

    for(clientid_t i = 0; i <= max; i++)
    {
        auto it = Clients.find(i);
        if(it == Clients.end())
        {
            return i;
        }
    }
    return max;
}

bool CServer::IsClientExist(const CEndPoint& endpoint)
{
    return m_getClientIterator(endpoint) != Clients.end();
}

CServer::CClient& CServer::GetClient(const CEndPoint& endpoint)
{
    return m_getClientIterator(endpoint)->second;
}

std::unordered_map<clientid_t, CServer::CClient>::iterator CServer::m_getClientIterator(const CEndPoint& endpoint)
{
    return std::find_if(Clients.begin(), Clients.end(), [&endpoint](auto& kv) -> bool { return kv.second.EndPoint == endpoint; });
}

double CServer::GetRate() const
{
    return m_svRate;
}

void CNetServerInfo::V_Write(CBufferWrapper& wrapper)
{
    auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
    if(!server) { return; }

    wrapper.Write<double>(server->GetRate());
}

void CNetServerInfo::V_Read(CBufferWrapper& wrapper)
{
    Rate = wrapper.Read<double>();
}

void CNetServerInfo::Process()
{
    auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
    if(!client) { return; }

    client->SetServerRate(Rate);
}

LINK_SOL_USERTYPE(CServer);
LINK_NET_MESSAGE_TO_CLASS(CNetServerInfo, serverinfo);
LINK_COMPONENT_TO_CLASS(CServer, server);