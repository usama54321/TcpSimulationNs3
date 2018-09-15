#include <iostream>
#include <string>
#include <unistd.h>
#include <assert.h>
#include <ctime>
#include <cstdio>

#include "main.h"
#include "TcpClient.h"
#include "TcpHelper.h"

//#include "tcp-cubic.h"

using namespace ns3;
using namespace std;

const string SIMU_NAME = "SingleLinkMultipleTcpTest";
NS_LOG_COMPONENT_DEFINE (SIMU_NAME);


/***
 *
 * There are Point2Point connections between all clients and server[0],
 * and a CSMA connection between the two server nodes to model a shared link
 *
 */
int main(int argc, char *argv[]) {
    unsigned NUM_CLIENTS = 6;
    float START_TIME = 0.1;
    float STOP_TIME = 10;
    float LOSS_RATE = 5;
    float BW = 1;
    float DELAY = 1;
    unsigned CUBIC_CONN = 1;
    unsigned BBR_CONN = 1;
    unsigned RENO_CONN = 1;
    unsigned VEGAS_CONN = 1;

    unsigned CONN_TIMEOUT = 3;
    unsigned INIT_CWND = 1;
    bool PACING = 0;
    bool NODELAY = 0;
    
    char *temp = getenv("ROOT_DIR");
    unsigned PORT = 15002;
    string LOG_DIR(temp);

    std::clock_t start;
    double duration;

    CommandLine cmd;
    //cmd.AddValue("numClients", "Number of connections to simulate to the server", NUM_CLIENTS);
    cmd.AddValue("duration", "Duration of the simulation in seconds", STOP_TIME);
    cmd.AddValue("bandwidth", "Bandwidth of bottleneck link", BW);
    cmd.AddValue("latency", "Latency of bottleneck link", DELAY);
    cmd.AddValue("loss_rate", "Loss Rate of bottleneck link as percentage e.g. 5", LOSS_RATE);
    cmd.AddValue("cubic", "Number of cubic connections", CUBIC_CONN);
    cmd.AddValue("bbr", "Number of cubic connections", BBR_CONN);
    cmd.AddValue("reno", "Number of reno connections", RENO_CONN);
    cmd.AddValue("vegas", "Number of vegas connections", VEGAS_CONN);
    cmd.AddValue("rto", "Retransmission timeout", CONN_TIMEOUT);
    cmd.AddValue("cwnd", "Initial CWND", INIT_CWND);
    cmd.AddValue("pacing", "Pacing", PACING);
    cmd.AddValue("no-delay", "No Delay", NODELAY);

    cmd.Parse(argc, argv);
    //cout << tid.GetName();

    start = std::clock();

    //if(DELAY > 0)
    //STOP_TIME = STOP_TIME * DELAY/50;

    NUM_CLIENTS = BBR_CONN + VEGAS_CONN + RENO_CONN + CUBIC_CONN;

    //Config::SetDefault ("ns3::TcpSocketState::EnablePacing", BooleanValue (false));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue(INIT_CWND)); 
    Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue(Seconds(CONN_TIMEOUT))); 
    Config::SetDefault ("ns3::TcpSocket::TcpNoDelay", BooleanValue(NODELAY)); 

    LogComponentEnable(&SIMU_NAME[0], LOG_LEVEL_ALL);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    NodeContainer router;
    router.Create(2);

    NodeContainer servers;
    servers.Create(NUM_CLIENTS);

    NodeContainer clients;
    clients.Create(NUM_CLIENTS);



    //add internet stacks


    InternetStackHelper stack;
    stack.Install(servers);
    stack.Install(clients);
    stack.Install(router);

    TypeId tid = TypeId::LookupByName ("ns3::TcpCubic");

    unsigned estimated_rtt = 2 * DELAY/1000;
    unsigned megabitToBytes = 125000;

    //Establish bottleneck link
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (to_string(BW) + "Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue(to_string(DELAY) + "ms"));
    bottleneckLink.SetQueue("ns3::DropTailQueue","Mode",EnumValue(DropTailQueue<Packet>::QUEUE_MODE_BYTES),"MaxBytes",UintegerValue (megabitToBytes * BW * estimated_rtt));

    auto bottleneckLinkDevices = bottleneckLink.Install(router);
    auto bottleneckInterfaces = address.Assign(bottleneckLinkDevices);

    //    //set error model
    Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel> ();
    errorModel->SetRate(LOSS_RATE/100.0);
    errorModel->SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
    bottleneckLinkDevices.Get(1)->GetObject<PointToPointNetDevice>()->SetReceiveErrorModel(errorModel);

    address.NewNetwork();

    //connections.SetChannelAttribute ("Delay", StringValue ("2ms"));

    Ipv4InterfaceContainer serverInterfaces;
    //for now NUM_CLIENTS = 2
    for(unsigned i = 0; i < NUM_CLIENTS; i++) {
        PointToPointHelper connections;
        string rate;
        NetDeviceContainer devices;

        connections.SetDeviceAttribute ("DataRate", StringValue("5Mbps"));
        devices = connections.Install(clients.Get(i), router.Get(0));
        Ipv4InterfaceContainer interface = address.Assign(devices);
        address.NewNetwork();

        PointToPointHelper serverConnection;
        auto serverDevices = serverConnection.Install(router.Get(1), servers.Get(i));
        auto serverInterface = address.Assign(serverDevices);
        serverInterfaces.Add(serverInterface.Get(1));
        address.NewNetwork();
    }


    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    //enable logging

    bottleneckLink.EnablePcap(LOG_DIR + "/logs/cubic-" + to_string(CUBIC_CONN) + "-bbr-" + to_string(BBR_CONN) + "-vegas-" + to_string(VEGAS_CONN) + "-reno-" + to_string(RENO_CONN) + "-bw-" + to_string(BW) + "-lr-" + to_string(LOSS_RATE) + "-delay-" + to_string(DELAY) + ".pcap", bottleneckLinkDevices.Get(1), true, true);
    //bottleneckLink.EnableAscii(LOG_DIR + "/logs/" + to_string(BW) + "-lr-" + to_string(LOSS_RATE) + "-delay-" + to_string(DELAY) + ".ascii", bottleneckLinkDevices.Get(1), true);
    //connections.EnablePcapAll(LOG_DIR+ "/logs/connections", true);


    //server applications
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpBbr"));
    //ApplicationContainer serverAppBbr = initServerApplication(server.Get(0), PORT + 1);

    //serverAppCubic.Start (Seconds (START_TIME));
    //serverAppCubic.Stop (Seconds (STOP_TIME));

    //serverAppBbr.Start (Seconds (START_TIME));
    //serverAppBbr.Stop (Seconds (STOP_TIME));

    //AddressValue remoteAddressBbr (InetSocketAddress (bottleneckInterfaces.GetAddress(1,0), PORT + 1) );

    //Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));
    //reset for clients
    NS_LOG_LOGIC("Creating " + to_string(CUBIC_CONN) + " cubic connections");
    NS_LOG_LOGIC("Creating " + to_string(BBR_CONN) + " bbr connections");
    NS_LOG_LOGIC("Creating " + to_string(VEGAS_CONN) + " vegas connections");
    NS_LOG_LOGIC("Creating " + to_string(RENO_CONN) + " reno connections");
    for(unsigned i = 0; i < NUM_CLIENTS; i++) {
        TypeId tid;
        if(i >= 0 && i < CUBIC_CONN) {
            tid = TypeId::LookupByName ("ns3::TcpCubic");
        } else if (i >= CUBIC_CONN && i < CUBIC_CONN + BBR_CONN) {
            tid = TypeId::LookupByName ("ns3::TcpBbr");
        } else if (i >= CUBIC_CONN + BBR_CONN && i < CUBIC_CONN + BBR_CONN + VEGAS_CONN) {
            tid = TypeId::LookupByName ("ns3::TcpVegas");
        } else {
            tid = TypeId::LookupByName ("ns3::TcpReno");
        }
 
        Config::Set ("/NodeList/" + to_string(servers.Get(i)->GetId()) +"/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));
        ApplicationContainer serverAppCubic = initServerApplication(servers.Get(i), PORT + i);
        //AddressValue remoteAddressCubic (InetSocketAddress (serverInterfaces.GetAddress(i,0), PORT+i));

        serverAppCubic.Start (Seconds (START_TIME));
        serverAppCubic.Stop (Seconds (STOP_TIME));

        ApplicationContainer clientApp;
        clientApp = initClientApp(clients.Get(i), serverInterfaces.GetAddress(i,0), PORT+i);
        clientApp.Start (Seconds (START_TIME));
        clientApp.Stop (Seconds (STOP_TIME));

    }

    NS_LOG_LOGIC("Created nodes. Starting simulation at " + to_string(( std::clock() - start ) / (double) CLOCKS_PER_SEC));
    /*
    NS_LOG_LOGIC("Creating " + to_string(BBR_CONN) + " bbr connections");
    for (unsigned i = CUBIC_CONN; i < CUBIC_CONN + BBR_CONN; i++) {

        TypeId tid = TypeId::LookupByName ("ns3::TcpBbr");
        ApplicationContainer serverAppBbr = initServerApplication(servers.Get(i), PORT + i);
        //AddressValue remoteAddressBbr (InetSocketAddress (serverInterfaces.GetAddress(i,0), PORT + i));
        string temp = "/NodeList/" + to_string(servers.Get(i)->GetId()) +"/$ns3::TcpL4Protocol/SocketType";
        Config::Set (temp, TypeIdValue (tid));

        serverAppBbr.Start (Seconds (START_TIME));
        serverAppBbr.Stop (Seconds (STOP_TIME));

        ApplicationContainer clientApp;
        clientApp = initClientApp(clients.Get(i), serverInterfaces.GetAddress(i,0), PORT + i);
        clientApp.Start (Seconds (START_TIME));
        clientApp.Stop (Seconds (STOP_TIME));
    }
    */
    Simulator::Stop (Seconds (STOP_TIME));

    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));

    /*
    std::string prefix_file_name = LOG_DIR + "/logs/TcpVariantsComparison";
    Simulator::Schedule (Seconds (0.00001), &TraceCwnd, prefix_file_name + "-cwnd.data");
    Simulator::Schedule (Seconds (0.00001), &TraceSsThresh, prefix_file_name + "-ssth.data");
    Simulator::Schedule (Seconds (0.00001), &TraceRtt, prefix_file_name + "-rtt.data");
    Simulator::Schedule (Seconds (0.00001), &TraceRto, prefix_file_name + "-rto.data");
    Simulator::Schedule (Seconds (0.00001), &TraceNextTx, prefix_file_name + "-next-tx.data");
    Simulator::Schedule (Seconds (0.00001), &TraceInFlight, prefix_file_name + "-inflight.data");
    Simulator::Schedule (Seconds (0.1), &TraceNextRx, prefix_file_name + "-next-rx.data");
    */
    Simulator::Run ();

    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
    std::cout<<"duration: "<< duration <<'\n';

    Simulator::Destroy ();
    return 0;
}

ApplicationContainer initClientApp(Ptr<Node> client, Ipv4Address address, uint16_t port) {
    TcpClientHelper ftp (address, port);
    //AttributeValue temp = 1;
    ftp.SetAttribute ("MaxPackets", UintegerValue(1));
    return ftp.Install (client);
}

ApplicationContainer initServerApplication(Ptr<Node> server, unsigned port) {
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    TcpServerHelper sinkHelper (port);
    sinkHelper.SetAttribute("MaxBytes", UintegerValue(0));
    auto sinkApp = sinkHelper.Install(server);
    return sinkApp;
}

inline double round( double val ) {
    if( val < 0 ) return ceil(val - 0.5);
    return floor(val + 0.5);
}
