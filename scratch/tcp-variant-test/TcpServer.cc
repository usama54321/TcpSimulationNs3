#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <assert.h>

#include "TcpServer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpServerApplication");
NS_OBJECT_ENSURE_REGISTERED (TcpServer);

TypeId
TcpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpServer")
    .SetParent<Application> ()
    .AddConstructor<TcpServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming connections.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&TcpServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute("MaxBytes", "Number of packets to send. 0 means infinite",
                   UintegerValue(4096),
                   MakeUintegerAccessor (&TcpServer::m_bytes),
                   MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

TcpServer::TcpServer ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  m_running = false;
}

TcpServer::~TcpServer()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

void
TcpServer::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
TcpServer::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_running = true;

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress listenAddress = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (listenAddress);
      m_socket->Listen();
    }

  m_socket->SetAcceptCallback (
      MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
      MakeCallback (&TcpServer::HandleAccept, this));
}

void
TcpServer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_running = false;

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetAcceptCallback (
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeNullCallback<void, Ptr<Socket>, const Address &> () );
    }
}

void
TcpServer::ReceivePacket (Ptr<Socket> s)
{
  NS_LOG_FUNCTION (this << s);

  Ptr<Packet> packet;
  Address from;
  while (packet = s->RecvFrom (from))
  {
      if (packet->GetSize () > 0)
      {
          NS_LOG_INFO ("Server Received " << packet->GetSize () << " bytes from " <<
          InetSocketAddress::ConvertFrom (from).GetIpv4 ());

          packet->RemoveAllPacketTags ();
          packet->RemoveAllByteTags ();

          //NS_LOG_LOGIC ("Echoing packet");
          //s->Send (packet);
      }
  }
    
  DataSend(s);
}

void TcpServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  sentData[s] = 0;
  connected[s] = true;
//Ipv4Address dst = s->m_orgDestIP;
  NS_LOG_INFO("ACCEPT IN ECHO SERVER from " << InetSocketAddress::ConvertFrom(from).GetIpv4());
    s->SetSendCallback (
    MakeCallback (&TcpServer::DataSend, this));
  
  s->SetRecvCallback (MakeCallback (&TcpServer::ReceivePacket, this));
}

void TcpServer::HandleSuccessClose(Ptr<Socket> s)
{
  NS_LOG_FUNCTION (this << s);
  NS_LOG_LOGIC ("Client close received");
  s->Close();
  s->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > () );
  s->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),
      MakeNullCallback<void, Ptr<Socket> > () );
}

void TcpServer::DataSend(Ptr<Socket> s, uint32_t t) {
  unsigned sent = sentData[s];
  while (connected[s] && (m_bytes == 0 || sent < m_bytes)) {
      // uint64_t to allow the comparison later.
      // the result is in a uint32_t range anyway, because
      // m_sendSize is uint32_t.
      uint64_t toSend = 512;
      // Make sure we don't send too many
      if(m_bytes) {
        uint64_t rest = m_bytes - sent;
        toSend = std::min (toSend, rest);
      }

    Address addr;
    s->GetSockName (addr);
            InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (addr);
      NS_LOG_LOGIC ("sending packet at " << Simulator::Now () << iaddr.GetIpv4());
      Ptr<Packet> packet = Create<Packet> (toSend);
      int actual = s->Send (packet);
      if (actual > 0)
      {
          sent += actual;
          sentData[s] = sent;
          //m_txTrace (packet);
      }
      // We exit this loop when actual < toSend as the send side
      // buffer is full. The "DataSent" callback will pop when
      // some buffer space has freed ip.
      if ((unsigned)actual != toSend)
      {
          break;
      }

  }
  sentData[s] = sent;
  if (m_bytes && sent == m_bytes && connected[s])
  {
      s->Close ();
      connected[s] = false;
  }

}
} // Namespace ns3
/*
class TcpServer: public TcpServer {

    map< Ptr<Socket>, unsigned> sentData;
    map< Ptr<Socket>, bool> connected;

    

    void ReceivePacket (Ptr<Socket> s)
    {

    }


    void StopApplication ()
    {

        for(auto it = connected.begin(); end = connected.end(); it != end; it++) {
            if(it->second) {
                it->first->Close(); 
                it->second = false;
            }
        }

    }
};
*/
