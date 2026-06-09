#pragma once
#include "boost/asio.hpp"
#include "IBinaryStream.h"
#include "CTime.h"

#include <mutex>
#include <queue>

using IpAddress = boost::asio::ip::address;
using IpAddressV4 = boost::asio::ip::address_v4;
using IpAddressV6 = boost::asio::ip::address_v6;

class CPeerProperty
{
public:
	enum class CType
	{
		None,
		Block,
		Protect,
		Allow
	};

	CPeerProperty(boost::asio::ip::udp::endpoint endpoint, CType in, CType out, bool blockwholeip = false);

	boost::asio::ip::udp::endpoint EndPoint;

	CType TypeIn = CType::Block;
	CType TypeOut = CType::Block;

	void SetTypeIn(CType typ);
	void SetTypeOut(CType typ);
	void SetTypes(CType typin, CType typout);

	bool IsValid();
	static CPeerProperty GetInvalid();

	template<typename _Rep, typename _Period>
	void SetTimerDuration(std::chrono::duration<_Rep, _Period> duration)
	{
		TimerSet = true;

		auto time = std::chrono::high_resolution_clock::now() + duration; //TODO HACK (std::chrono::high_resolution_clock::now() instead of Time.GetCurrent())
		auto highResTime = std::chrono::time_point_cast<std::chrono::high_resolution_clock::duration>(time);

		ResetTime = highResTime;
	}

	void SetTimerDurationSeconds(std::chrono::duration<float, std::ratio<1, 1>> duration)
	{
		TimerSet = true;

		auto time = std::chrono::high_resolution_clock::now() + duration; //TODO HACK (std::chrono::high_resolution_clock::now() instead of Time.GetCurrent())
		auto highResTime = std::chrono::time_point_cast<std::chrono::high_resolution_clock::duration>(time);

		ResetTime = highResTime;
	}

	void SetTimer(CTimePoint resettime);
	void DisableTimer();

	CTimePoint ResetTime;
	bool TimerSet = false;

	bool BlockWholeIP = false;
};

class CNetTransceiver
{
public:
	CNetTransceiver(unsigned short port, size_t buffsize = 524288);
	~CNetTransceiver();

	void StartSync();
	void StartAsync();
	void Stop();

	bool IsAsync();

	void ReceiveAsync();
	void SendAsync(boost::asio::ip::udp::endpoint point, const std::vector<std::uint8_t>& msg, std::function<void(boost::system::error_code, std::size_t, CTimePoint)> callback = {});

	void ReceiveSync();
	void SendSync(boost::asio::ip::udp::endpoint point, const std::vector<std::uint8_t>& msg);

	void UpdatePeerTimers();
	void UpdateSinglePeerTimer(boost::asio::ip::udp::endpoint point);

	boost::asio::ip::address LocalIpAddress;
	unsigned short LocalPort;
	boost::asio::ip::address RemoteIpAddress;
	unsigned short RemotePort;

	std::vector<std::uint8_t> Buffer;

	std::thread IoThread;
	boost::asio::io_context IoContext;

	std::unique_ptr<boost::asio::ip::udp::socket> Socket;
	boost::asio::ip::udp::endpoint RemoteEndpoint;

	static char STATE_IDLE;
	static char STATE_RUNNING;
	static char STATE_STOP;

	std::atomic_char State = STATE_IDLE;
	std::atomic_bool Sending = false;
	std::atomic_bool Receiving = false;
	std::atomic_bool InReceivingRoutine = false;
	std::atomic_bool InSendingRoutine = false;

	CPeerProperty& SetPeerProperty(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typein = CPeerProperty::CType::Block, CPeerProperty::CType typeout = CPeerProperty::CType::Block);
	void ResetPeerProperty(boost::asio::ip::udp::endpoint point);

	CPeerProperty& SetPeerPropertyTypeIn(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typein = CPeerProperty::CType::Block);
	CPeerProperty& SetPeerPropertyTypeOut(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typeout = CPeerProperty::CType::Block);
	CPeerProperty& SetPeerPropertyTypeBoth(boost::asio::ip::udp::endpoint point, CPeerProperty::CType typeboth = CPeerProperty::CType::Block);

	CPeerProperty& SetIpProperty(boost::asio::ip::address addr, CPeerProperty::CType typein = CPeerProperty::CType::Block, CPeerProperty::CType typeout = CPeerProperty::CType::Block);
	CPeerProperty& SetIpPropertyTypeIn(boost::asio::ip::address addr, CPeerProperty::CType typein = CPeerProperty::CType::Block);
	CPeerProperty& SetIpPropertyTypeOut(boost::asio::ip::address addr, CPeerProperty::CType typeout = CPeerProperty::CType::Block);
	CPeerProperty& SetIpPropertyTypeBoth(boost::asio::ip::address addr, CPeerProperty::CType typeboth = CPeerProperty::CType::Block);

	bool GetPeerProperty(boost::asio::ip::udp::endpoint point, CPeerProperty& prop);
	CPeerProperty GetPeerProperty(boost::asio::ip::udp::endpoint point);
	bool PeerHasProperty(boost::asio::ip::udp::endpoint point);

	bool CanSendToPeer(boost::asio::ip::udp::endpoint point);
	bool CanReceiveFromPeer(boost::asio::ip::udp::endpoint point);

	void SetNonBlocking(bool enable = true);
	bool GetNonBlocking();

	void SetInverseFilter(bool inversefilter = true);
	void ToggleInverseFilter();
	bool IsFilterInverse();

	size_t CountIpsTypeIn(CPeerProperty::CType typein = CPeerProperty::CType::Block);
	size_t CountIpsTypeOut(CPeerProperty::CType typeout = CPeerProperty::CType::Block);
	size_t CountIpsTypes(CPeerProperty::CType typein = CPeerProperty::CType::Block, CPeerProperty::CType typeout = CPeerProperty::CType::Block);
	size_t CountIpsTypeBoth(CPeerProperty::CType typeboth = CPeerProperty::CType::Block);

	boost::system::error_code GetLastError() const;

	CCallbackHandler<void, boost::asio::ip::udp::endpoint, std::vector<std::uint8_t>&, size_t, CTimePoint> ProcessPacket;

	//void (*ProcessPacket)(boost::asio::ip::udp::endpoint endpoint, const std::vector<std::uint8_t>& data, size_t datasize) = nullptr;
	std::vector<CPeerProperty> Filter;

    struct CPacket
    {
        boost::asio::ip::udp::endpoint From;
        std::vector<uint8_t> Data;

		CTimePoint ReceivedAt;
    };

    struct
    {
        bool Enabled = true;
        std::vector<CPacket> Container;
        std::mutex Mutex;
    } Queue;

	CTimePoint GetLastSendTime() const;
private:
	bool m_Async = false;
	bool m_InverseFilter = false;
	boost::system::error_code m_networkError;

	CTimePoint LastSend;

	std::vector<CPeerProperty>::iterator getPeerIterator(boost::asio::ip::udp::endpoint point);
};