#include <iostream>
#include <string>
#include <unistd.h>

#include "main.h"

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
int main(int argc, char **argv) {
    unsigned NUM_CLIENTS = 6;
    float START_TIME = 0.1;
    float STOP_TIME = 10;
    char *temp = getenv("ROOT_DIR"); 
    unsigned PORT = 15002;
    string LOG_DIR(temp);

    CommandLine cmd;
    cmd.AddValue("num_clients", "Number of connections to simulate to the server", NUM_CLIENTS);
    cmd.AddValue("duration", "Duration of the simulation in seconds", STOP_TIME);
    cmd.Parse(argc, argv);

    LogComponentEnable(&SIMU_NAME[0], LOG_LEVEL_ALL);
    //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpBbr"));
    
    NodeContainer server;
    server.Create(2);

    NodeContainer clients;
    clients.Create(NUM_CLIENTS);
 
    //add internet stacks
    InternetStackHelper stack;
    stack.Install(server);
    stack.Install(clients);

    PointToPointHelper connections;
    connections.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    connections.SetChannelAttribute ("Delay", StringValue ("2ms"));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    for(unsigned i = 0; i < NUM_CLIENTS; i++) {
       NetDeviceContainer devices;
       devices = connections.Install(clients.Get(i), server.Get(0));

       Ipv4InterfaceContainer interface = address.Assign(devices);
       address.NewNetwork();
    }

    //connect the two server nodes through a csma channel
    CsmaHelper sharedLink;
    auto serverDevices = sharedLink.Install(server);
    auto serverInterfaces = address.Assign(serverDevices);
    sharedLink.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    //enable logging
    sharedLink.EnablePcapAll (LOG_DIR + "/logs/sharedLinkLog", true);
    connections.EnablePcapAll(LOG_DIR+ "/logs/connections", true);

    //server application

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpCubic::GetTypeId ()));
    ApplicationContainer serverAppCubic = initServerApplication(server.Get(1), PORT);
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpBbr"));
    ApplicationContainer serverAppBbr = initServerApplication(server.Get(1), PORT + 1);

    serverAppCubic.Start (Seconds (START_TIME));
    serverAppCubic.Stop (Seconds (STOP_TIME));

    serverAppBbr.Start (Seconds (START_TIME));
    serverAppBbr.Stop (Seconds (STOP_TIME));

    AddressValue remoteAddressCubic (InetSocketAddress (serverInterfaces.GetAddress(1,0), PORT));
    AddressValue remoteAddressBbr (InetSocketAddress (serverInterfaces.GetAddress(1,0), PORT + 1) );

    //reset for clients
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
    for (unsigned i = 0; i < clients.GetN (); i++) {
        ApplicationContainer clientApp;
        if(i % 2 == 0)
            clientApp = initClientApp(clients.Get(i), remoteAddressCubic);
        else
            clientApp = initClientApp(clients.Get(i), remoteAddressBbr);
        clientApp.Start (Seconds (START_TIME));
        clientApp.Stop (Seconds (STOP_TIME));
    }

    Simulator::Stop (Seconds (STOP_TIME));
    Simulator::Run ();

    Simulator::Destroy ();
    return 0;
}

ApplicationContainer initClientApp(Ptr<Node> client, AddressValue remoteAddress) {
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    ftp.SetAttribute ("Remote", remoteAddress);
    return ftp.Install (client);
}

ApplicationContainer initServerApplication(Ptr<Node> server, unsigned port) {
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
    sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    auto sinkApp = sinkHelper.Install(server);
    return sinkApp;
}
