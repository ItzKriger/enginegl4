#include "CNetTransceiver.h"
#include "CEngine.h"
#include "U_Log.h"

#include <algorithm>

CNetTransceiver::CNetTransceiver(unsigned short port, size_t buffsize) : IoThread([this]() { boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(boost::asio::make_work_guard(IoContext)); IoContext.run(); })
{
	State = STATE_IDLE;

	try
	{
		Socket = std::make_unique<boost::asio::ip::udp::socket>(IoContext, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
	}
	catch (const boost::system::system_error& ex)
	{
		bool ok = true;
		try
		{
			Socket = std::make_unique<boost::asio::ip::udp::socket>(IoContext, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0U));
		}
		catch (const boost::system::system_error& ex)
		{
			ok = false;
		}

		if (!ok)
		{
            Log::ErrInstance() << "Can't init network transceiver\n";
		}
		else
		{
            Log::ErrInstance() << "Network port " << port << " was busy, another one was chosen\n";
		}
	}

	LocalPort = Socket->local_endpoint().port();
	LocalIpAddress = Socket->local_endpoint().address();

    Queue.Container.reserve(128); //TODO not hardcoded
	Buffer.resize(buffsize);

    Log::Instance() << "Network transceiver is working on " << LocalIpAddress.to_string() << ":" << LocalPort << " with buffer size of " << Buffer.size() << " bytes\n";
}

CPeerProperty::CPeerProperty(boost::asio::ip::udp::endpoint endpoint, CType in, CType out, bool blockwholeip)
{
	EndPoint = endpoint;
	TypeIn = in;
	TypeOut = out;

	BlockWholeIP = blockwholeip;
}

void CNetTransceiver::SetNonBlocking(bool enable)
{
	Socket->non_blocking(enable);
}

bool CNetTransceiver::GetNonBlocking()
{
	return Socket->non_blocking();
}

CNetTransceiver::~CNetTransceiver()
{
	Stop();
}

void CPeerProperty::SetTimer(CTimePoint resettime)
{
	TimerSet = true;
	ResetTime = resettime;
}

void CPeerProperty::DisableTimer()
{
	TimerSet = false;
}

void CNetTransceiver::StartSync()
{
	m_Async = false;
	State = STATE_RUNNING;
}

void CNetTransceiver::StartAsync()
{
	m_Async = true;
	State = STATE_RUNNING;
	ReceiveAsync();
}

void CNetTransceiver::Stop()
{
	if (State == STATE_IDLE) { return; }

	State = STATE_STOP;
	//IoThread.detach();

	if (!InReceivingRoutine && !InSendingRoutine)
	{
		State = STATE_IDLE;
	}
	else
	{
		while (State.load() != STATE_IDLE) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
	}

	IoContext.stop();
	IoThread.join();
}

void CNetTransceiver::ReceiveAsync()
{
	if (State == STATE_STOP) { State = STATE_IDLE; return; }

	Receiving = true;

	Socket->async_receive_from(boost::asio::buffer(Buffer), RemoteEndpoint,
		[this](boost::system::error_code ec, std::size_t bytes_recvd)
		{
			Receiving = false;
			InReceivingRoutine = true;

			m_networkError = ec;

			UpdateSinglePeerTimer(RemoteEndpoint);
			if (CanReceiveFromPeer(RemoteEndpoint))
			{
				if (bytes_recvd && ec.value() == 0)
                {
					CTimePoint recvd_at = CEngine::GetInstance()->Time.GetCurrent();

                    if(Queue.Enabled)
                    {
                        CPacket pkt;
                        pkt.From = RemoteEndpoint;
                        pkt.Data.assign(Buffer.begin(), Buffer.begin() + bytes_recvd);
						pkt.ReceivedAt = recvd_at;

                        {
                            std::lock_guard<std::mutex> _lock(Queue.Mutex);
                            Queue.Container.push_back(std::move(pkt));
                        }
                    }

					if(ProcessPacket)
					{
                    	ProcessPacket(RemoteEndpoint, Buffer, bytes_recvd, recvd_at);
					}
                }
			}
			
			InReceivingRoutine = false;
			if (State == STATE_STOP) { State = STATE_IDLE; return; }
			boost::asio::post(Socket->get_executor(), [this]() { ReceiveAsync(); });
		});
}

void CNetTransceiver::SendAsync(boost::asio::ip::udp::endpoint point, const std::vector<std::uint8_t>& msg, std::function<void(boost::system::error_code, std::size_t, CTimePoint)> callback)
{
	UpdateSinglePeerTimer(point);
	if (!CanSendToPeer(point)) { return; }
	Sending = InSendingRoutine = true;
	Socket->async_send_to(boost::asio::buffer(msg), point, [this, callback](boost::system::error_code ec, std::size_t bytes_sent)
	{
		m_networkError = ec;
		Sending = false;
		InSendingRoutine = false;

		LastSend = CEngine::GetInstance()->Time.GetCurrent();
		if(callback)
		{
			callback(ec, bytes_sent, LastSend);
		}
	});
}

void CNetTransceiver::ReceiveSync()
{
	Receiving = InReceivingRoutine = true;
	size_t bytes_recvd = Socket->receive_from(boost::asio::buffer(Buffer), RemoteEndpoint, 0, m_networkError);
	Receiving = false;

	UpdateSinglePeerTimer(RemoteEndpoint);
	if (CanReceiveFromPeer(RemoteEndpoint))
	{
		if (bytes_recvd && m_networkError.value() == 0)
		{
			CTimePoint recvd_at = CEngine::GetInstance()->Time.GetCurrent();
			if(Queue.Enabled)
			{
				CPacket pkt;
				pkt.From = RemoteEndpoint;
				pkt.Data.assign(Buffer.begin(), Buffer.begin() + bytes_recvd);
				pkt.ReceivedAt = recvd_at;

				{
					std::lock_guard<std::mutex> _lock(Queue.Mutex);
					Queue.Container.push_back(std::move(pkt));
				}
			}

			if(ProcessPacket)
			{
				ProcessPacket(RemoteEndpoint, Buffer, bytes_recvd, recvd_at);
			}
		}
	}
	InReceivingRoutine = false;
}

CTimePoint CNetTransceiver::GetLastSendTime() const
{
	return LastSend;
}

boost::system::error_code CNetTransceiver::GetLastError() const
{
	return m_networkError;
}

void CNetTransceiver::SendSync(boost::asio::ip::udp::endpoint point, const std::vector<std::uint8_t>& msg)
{
	UpdateSinglePeerTimer(point);
	if (!CanSendToPeer(point)) { return; }

	Sending = InSendingRoutine = true;
	Socket->send_to(boost::asio::buffer(msg), point, 0, m_networkError);

	LastSend = CEngine::GetInstance()->Time.GetCurrent();
	Sending = InSendingRoutine = false;
}

bool CNetTransceiver::IsAsync() { return m_Async; }

bool CPeerProperty::IsValid()
{
	return TypeIn != CType::None || CPeerProperty::TypeOut != CType::None;
}

CPeerProperty CPeerProperty::GetInvalid()
{
	return { {}, CPeerProperty::CType::None, CPeerProperty::CType::None };
}

void CPeerProperty::SetTypeIn(CPeerProperty::CType typ)
{
	if (TypeIn == CType::Protect && typ == CType::Block) { return; }
	TypeIn = typ;
}

void CPeerProperty::SetTypeOut(CPeerProperty::CType typ)
{
	if (TypeOut == CType::Protect && typ == CType::Block) { return; }
	TypeOut = typ;
}

void CPeerProperty::SetTypes(CType typin, CType typout)
{
	SetTypeIn(typin);
	SetTypeOut(typout);
}

std::vector<CPeerProperty>::iterator CNetTransceiver::getPeerIterator(boost::asio::ip::udp::endpoint point)
{
	return std::find_if(Filter.begin(), Filter.end(), [point](const CPeerProperty& prop) -> bool
		{
			if (prop.BlockWholeIP) { return prop.EndPoint.address() == point.address(); }
			return prop.EndPoint.address() == point.address() && prop.EndPoint.port() == point.port();
		});
}

CPeerProperty& CNetTransceiver::SetPeerProperty(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typein, CPeerProperty::CType typeout)
{
	auto it = getPeerIterator(point);
	if (it == Filter.end()) { Filter.push_back({ point, typein, typeout }); return Filter.back(); }
	(*it).SetTypes(typein, typeout);
	return *it;
}

void CNetTransceiver::ResetPeerProperty(boost::asio::ip::udp::endpoint point)
{
	auto it = getPeerIterator(point);
	if (it != Filter.end()) { Filter.erase(it); }
}

CPeerProperty& CNetTransceiver::SetPeerPropertyTypeIn(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typein)
{
	auto it = getPeerIterator(point);
	if (it == Filter.end()) { Filter.push_back({ point, typein, CPeerProperty::CType::None }); return Filter.back(); }
	(*it).SetTypeIn(typein);
	return *it;
}

CPeerProperty& CNetTransceiver::SetPeerPropertyTypeOut(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typeout)
{
	auto it = getPeerIterator(point);
	if (it == Filter.end()) { Filter.push_back({ point, CPeerProperty::CType::None, typeout }); return Filter.back(); }
	(*it).SetTypeOut(typeout);
	return *it;
}

CPeerProperty& CNetTransceiver::SetPeerPropertyTypeBoth(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typeboth)
{
	auto it = getPeerIterator(point);
	if (it == Filter.end()) { Filter.push_back({ point, typeboth, typeboth }); return Filter.back(); }
	(*it).SetTypes(typeboth, typeboth);
	return *it;
}

bool CNetTransceiver::GetPeerProperty(boost::asio::ip::udp::endpoint point, CPeerProperty& prop)
{
	auto it = getPeerIterator(point);
	if (it != Filter.end()) { prop = *it; return true; }
	return false;
}

CPeerProperty CNetTransceiver::GetPeerProperty(boost::asio::ip::udp::endpoint point)
{
	auto it = getPeerIterator(point);
	if (it != Filter.end()) { return *it; }
	return CPeerProperty::GetInvalid();
}

bool CNetTransceiver::PeerHasProperty(boost::asio::ip::udp::endpoint point)
{
	auto it = getPeerIterator(point);
	return it != Filter.end();
}

bool CNetTransceiver::CanSendToPeer(boost::asio::ip::udp::endpoint point)
{
	CPeerProperty prop = GetPeerProperty(point);

	if (!m_InverseFilter)
	{
		if (!prop.IsValid()) { return true; }
		if (prop.TypeOut == CPeerProperty::CType::Block)
		{
			return false;
		}
	}
	else
	{
		if (prop.TypeOut != CPeerProperty::CType::Allow && prop.TypeOut != CPeerProperty::CType::Protect)
		{
			return false;
		}
	}
	return true;
}

size_t CNetTransceiver::CountIpsTypeIn(CPeerProperty::CType typein)
{
	size_t ret = 0U;
	for (CPeerProperty& prop : Filter) { if (prop.TypeIn == typein) { ret++; } }
	return ret;
}

size_t CNetTransceiver::CountIpsTypeOut(CPeerProperty::CType typeout)
{
	size_t ret = 0U;
	for (CPeerProperty& prop : Filter) { if (prop.TypeOut == typeout) { ret++; } }
	return ret;
}

size_t CNetTransceiver::CountIpsTypes(CPeerProperty::CType typein, CPeerProperty::CType typeout)
{
	size_t ret = 0U;
	for (CPeerProperty& prop : Filter) { if (prop.TypeIn == typein && prop.TypeOut == typeout) { ret++; } }
	return ret;
}

size_t CNetTransceiver::CountIpsTypeBoth(CPeerProperty::CType typeboth)
{
	size_t ret = 0U;
	for (CPeerProperty& prop : Filter) { if (prop.TypeIn == typeboth && prop.TypeOut == typeboth) { ret++; } }
	return ret;
}

CPeerProperty& CNetTransceiver::SetIpProperty(boost::asio::ip::address addr, CPeerProperty::CType typein, CPeerProperty::CType typeout)
{
	Filter.erase(std::remove_if(Filter.begin(), Filter.end(), [addr](const CPeerProperty& prop) -> bool
		{ return prop.EndPoint.address() == addr; }), Filter.end());
	
	Filter.push_back({{ addr, 11231 }, typein, typeout, true }); //TODO is 11231 default client port?
	return Filter.back();
}

CPeerProperty& CNetTransceiver::SetIpPropertyTypeIn(boost::asio::ip::address addr, CPeerProperty::CType typein)
{
	return SetIpProperty(addr, typein, CPeerProperty::CType::None);
}

CPeerProperty& CNetTransceiver::SetIpPropertyTypeOut(boost::asio::ip::address addr, CPeerProperty::CType typeout)
{
	return SetIpProperty(addr, CPeerProperty::CType::None, typeout);
}

CPeerProperty& CNetTransceiver::SetIpPropertyTypeBoth(boost::asio::ip::address addr, CPeerProperty::CType typeboth)
{
	return SetIpProperty(addr, typeboth, typeboth);
}

bool CNetTransceiver::CanReceiveFromPeer(boost::asio::ip::udp::endpoint point)
{
	CPeerProperty prop = GetPeerProperty(point);

	if (!m_InverseFilter)
	{
		if (!prop.IsValid()) { return true; }
		if (prop.TypeIn == CPeerProperty::CType::Block)
		{
			return false;
		}
	}
	else
	{
		if (prop.TypeIn != CPeerProperty::CType::Allow && prop.TypeIn != CPeerProperty::CType::Protect)
		{
			return false;
		}
	}
	return true;
}

void CNetTransceiver::UpdatePeerTimers()
{
	Filter.erase(std::remove_if(Filter.begin(), Filter.end(), [](const auto& p) { return p.TimerSet && CEngine::GetInstance()->Time.GetCurrent() >= p.ResetTime; }), Filter.end());
}

void CNetTransceiver::UpdateSinglePeerTimer(boost::asio::ip::udp::endpoint point)
{
	auto it = getPeerIterator(point);
	if (it != Filter.end())
	{
		if ((*it).TimerSet && CEngine::GetInstance()->Time.GetCurrent() >= (*it).ResetTime)
		{
			Filter.erase(it);
		}
	}
}

void CNetTransceiver::SetInverseFilter(bool inversefilter)
{
	m_InverseFilter = inversefilter;
}

void CNetTransceiver::ToggleInverseFilter()
{
	m_InverseFilter = !m_InverseFilter;
}

bool CNetTransceiver::IsFilterInverse()
{
	return m_InverseFilter;
}

char CNetTransceiver::STATE_IDLE = 0;
char CNetTransceiver::STATE_RUNNING = 1;
char CNetTransceiver::STATE_STOP = 2;