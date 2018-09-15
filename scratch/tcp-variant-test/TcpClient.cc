#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "TcpClient.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpClientApplication");
NS_OBJECT_ENSURE_REGISTERED (TcpClient);

TypeId
TcpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpClient")
    .SetParent<Application> ()
    .AddConstructor<TcpClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&TcpClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&TcpClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Ipv4Address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&TcpClient::m_peerAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&TcpClient::SetDataSize,
                                         &TcpClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&TcpClient::m_txTrace))
  ;
  return tid;
}

TcpClient::TcpClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_bytesSent = 0;
  m_recvBack = 0;
  m_bytesRecvBack = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
}

TcpClient::~TcpClient()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
TcpClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
TcpClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
TcpClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

  m_socket->SetRecvCallback (MakeCallback (&TcpClient::ReceivePacket, this));
  m_socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),MakeNullCallback<void, Ptr<Socket> > () );
  ScheduleTransmit (Seconds (0.));
}

void
TcpClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
TcpClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
TcpClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

void
TcpClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
TcpClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
TcpClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
TcpClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sendEvent = Simulator::Schedule (dt, &TcpClient::Send, this);
}

void
TcpClient::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "TcpClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "TcpClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
      m_bytesSent += m_dataSize;
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that she doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding atribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
      m_bytesSent += m_size;
    }
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  m_socket->Send (p);

  ++m_sent;

  NS_LOG_INFO ("Sent " << m_size << " bytes to " << m_peerAddress);

  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }
}

void
TcpClient::ReceivePacket (Ptr<Socket> socket)
{
  //NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from)) {
      NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " <<
              InetSocketAddress::ConvertFrom (from).GetIpv4 ());

      // dont check if data returned is the same data sent earlier
      //m_recvBack++;
      //m_bytesRecvBack += packet->GetSize ();
  }

  /*
    if (m_count == m_recvBack) {
      socket->Close();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      socket = 0;
    }
  */
}

}
