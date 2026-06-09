#include "CClient.h"
#include "CEngine.h"
#include "CConVarManager.h"
#include "U_Networking.h"
#include "CBufferWrapper.h"

void CClient::Connect(const CEndPoint& srv)
{
    ServerPoint = srv;

    unsigned short port = Networking::ClientPort;
    COMPONENT_CALL_GET(port, CConVarManager, GetConVarValue<unsigned short>("net.clientport", Networking::ClientPort));

    Transceiver = std::make_unique<CNetTransceiver>(port);
    Transceiver->Queue.Enabled = true;

    Transceiver->SetInverseFilter(true);
    Transceiver->SetPeerProperty(ServerPoint, CPeerProperty::CType::Allow, CPeerProperty::CType::Allow);

    Log::Instance() << "Connecting to " << ServerPoint.address().to_string() << ":" << ServerPoint.port() << "\n";

    Transceiver->StartAsync();
    SendConnectionCookie();
}

void CClient::Connect(const std::string& hostname)
{
    CEndPoint endpoint;
    bool success = Networking::ParseEndpoint(endpoint, Transceiver->IoContext, hostname, Networking::HostPort);

    if(!success)
    {
        Log::ErrInstance() << "Couldn't parse hostname \"" << hostname << "\"\n";
        return Reset();
    }
    return Connect(endpoint);
}

void CClient::SendReliableMessage(std::shared_ptr<CNetMessage> msg)
{
    NetMessagesManager.SendReliable(msg);
}

void CClient::SendUnreliableMessage(std::shared_ptr<CNetMessage> msg)
{
    NetMessagesManager.SendUnreliable(msg);
}

void CClient::SetServerRate(double timesPerSecond)
{
    m_svRate = timesPerSecond;
    Log::Instance() << "Set server rate to " << m_svRate << Log::Endl;
}

double CClient::GetServerRate() const
{
    return m_svRate;
}

void CClient::V_Update()
{
    if(!Transceiver) { return; }
    std::vector<CNetTransceiver::CPacket> packets;
    {
        std::lock_guard<std::mutex> _lock(Transceiver->Queue.Mutex);
        packets.swap(Transceiver->Queue.Container);
    }

    for(auto& pack : packets)
    {
        ProcessDatagram(pack.From, pack.Data, pack.ReceivedAt);
        if(pack.ReceivedAt > LastPacket)
        {
            LastPacket = pack.ReceivedAt;
        }
    }

    std::uint32_t maxAttempts = 3;
    std::uint32_t timeout = 10000;

    COMPONENT_CALL_GET(maxAttempts, CConVarManager, GetConVarValue<std::uint32_t>("net.retries", 3));
    COMPONENT_CALL_GET(timeout, CConVarManager, GetConVarValue<std::uint32_t>("net.connection_timeout", 10000));

    auto diff = CEngine::GetInstance()->Time.GetCurrent() - Transceiver->GetLastSendTime();
    if(packets.empty() && SentCookie && !Authorized && diff >= std::chrono::milliseconds(timeout))
    {
        if(ConnectionAttempts <= maxAttempts)
        {
            SentCookie = false;
            SendConnectionCookie();

            Log::Instance() << "Retrying...\n";
        }
        else
        {
            Disconnected = true;
            Log::ErrInstance() << "Connection timed out\n";
        }
    }

    if(Authorized)
    {
        NetMessagesManager.ProcessPackets();
        NetMessagesManager.ProcessMessages();
        //Log::Instance() << "CL PROCESS\n";
    }

    if(Disconnected)
    {
        Reset();
    }
    else
    {
        std::vector<std::uint8_t> snapshot;
        if(NetMessagesManager.PackSnapshot(snapshot))
        {
            //Log::Instance() << "CL SENT SNAPSHOT\n";
            Transceiver->SendAsync(ServerPoint, snapshot);
        }
    }
}

void CClient::ProcessDatagram(boost::asio::ip::udp::endpoint point, std::vector<std::uint8_t>& buffer, CTimePoint ptime)
{
    //Log::Instance() << "[CL] There's a datagram from " << point.address().to_string() << ":" << point.port() << Log::Endl;

    CBufferWrapper packet(buffer);
    if(!Authorized)
    {
        auto answer_string = packet.ReadLenString<std::uint8_t, std::string>();
        if(answer_string == "EngineGL 5.0 Connection Accepted")
        {
            std::vector<std::uint8_t> cl_answer_buf;
            CBufferWrapper cl_answer(cl_answer_buf);

            cl_answer.WriteLenString<std::uint8_t, std::string>("EngineGL 5.0 Ready For Transmit");
            Transceiver->SendAsync(ServerPoint, cl_answer_buf, [this](boost::system::error_code ec, std::size_t bs, CTimePoint tp)
            {
                Authorized = true;
                Log::Instance() << "[CL] I'm authorized!\n";
            });
        }
        else
        {
            auto reason_string_u16 = packet.ReadLenString<std::uint16_t, std::u16string>();
            auto reason_string = StringUtils::U16ToWstr(reason_string_u16);

            Log::ErrInstance() << "Couldn't connect with reason of \"" << reason_string << "\"\n";
            Disconnected = true;
        }
    }
    else
    {
        HandleServerPacket(buffer, ptime);
    }
}

void CClient::HandleServerPacket(std::vector<std::uint8_t>& buffer, CTimePoint ptime)
{
    NetMessagesManager.AddReceivedPacket(std::move(buffer), ptime);
}

void CClient::SendConnectionCookie()
{
    std::wstring nickname = L"player";
    COMPONENT_CALL_GET(nickname, CConVarManager, GetConVarValue<std::wstring>("cl.nickname"));

    std::vector<std::uint8_t> cookie_buf;
    CBufferWrapper cookie(cookie_buf);
    
    cookie.WriteLenString<std::uint8_t, std::string>("EngineGL 5.0 Connection Cookie");
    cookie.WriteLenString<std::uint8_t, std::u16string>(StringUtils::WstrToU16(nickname));

    Transceiver->SendAsync(ServerPoint, cookie_buf, [this](boost::system::error_code ec, std::size_t bs, CTimePoint tp) { SentCookie = true; ConnectionAttempts++; });
}

void CClient::Reset()
{
    ServerPoint = {};
    NetMessagesManager = {};

    Authorized = false;
    Disconnected = false;
    SentCookie = false;
    ConnectionAttempts = 0;

    Transceiver.reset();
}

LINK_COMPONENT_TO_CLASS(CClient, client);