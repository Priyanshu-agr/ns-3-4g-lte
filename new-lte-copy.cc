#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
// #include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-module.h"
// #include "ns3/netanim-module.h"

using namespace ns3;

uint32_t ByteCounter = 0;    //!< Byte counter.
uint32_t oldByteCounter = 0; //!< Old Byte counter,

/**
 * Receive a packet.
 *
 * \param packet The packet.
 */
void ReceivePacket(Ptr<const Packet> packet, const Address &)
{
    ByteCounter += packet->GetSize();
}

/**
 * Write the throughput to file.
 *
 * \param firstWrite True if first time writing.
 * \param binSize Bin size.
 * \param fileName Output filename.
 */
void Throughput(bool firstWrite, Time binSize, std::string fileName)
{
    std::ofstream output;

    if (firstWrite)
    {
        output.open(fileName, std::ofstream::out);
        firstWrite = false;
    }
    else
    {
        output.open(fileName, std::ofstream::app);
    }

    // Instantaneous throughput every 200 ms

    double throughput = (ByteCounter - oldByteCounter) * 8 / binSize.GetSeconds() / 1024 / 1024;
    output << Simulator::Now().As(Time::S) << " " << throughput << std::endl;
    oldByteCounter = ByteCounter;
    Simulator::Schedule(binSize, &Throughput, firstWrite, binSize, fileName);
}

/**
 * Function called when there is a course change
 * \param context event context
 * \param mobility a pointer to the mobility model
 */
static void
CourseChange(std::string context, Ptr<const MobilityModel> mobility)
{
    Vector pos = mobility->GetPosition();
    Vector vel = mobility->GetVelocity();
    std::cout << Simulator::Now() << ", model=" << mobility << ", POS: x=" << pos.x
              << ", y=" << pos.y << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
              << ", z=" << vel.z << std::endl;
}

int main(int argc, char *argv[])
{
    uint32_t numOfUEs = 40;
    double simTime = 30.000;
    uint16_t bandwidth = 50;
    bool useIdealRrc = true;
    std::string outputDir = "./";
    std::string simTag = "new-lte-copy";
    double udpPacketSize = 1500;
    std::string schedulerType = "PF";

    CommandLine cmd(__FILE__);
    cmd.AddValue("SchedulerType", "Scheduler Type PF,RR,PSS,MT", schedulerType);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(40));

    RngSeedManager::SetSeed(3);
    RngSeedManager::SetRun(33);

    // Enable Logging
    auto logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

    // LogComponentEnable("LteHelper", logLevel);
    // LogComponentEnable("EpcHelper", logLevel);
    // LogComponentEnable("EpcEnbApplication", logLevel);
    // LogComponentEnable("EpcMmeApplication", logLevel);
    // LogComponentEnable("EpcPgwApplication", logLevel);
    // LogComponentEnable("EpcSgwApplication", logLevel);
    // LogComponentEnable("LteEnbRrc", logLevel);
    // LogComponentEnable("LteUeRrc", logLevel);

    // Generating LTE Helper and adding epcHelper to it
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // PGateway from epcHelper
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // RemoteHost creation
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Creating Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    // Routing Internet towards LTE n/w
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    // interface 0 is localhost, 1 is the p2p device
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Create Nodes: eNodeB and UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(4);
    ueNodes.Create(numOfUEs);

    // Mobility model for enb
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(0.0, 0.0, 0.0));
    enbPositionAlloc->Add(Vector(5000.0, 0.0, 0.0));
    enbPositionAlloc->Add(Vector(0.0, 5000.0, 0.0));
    enbPositionAlloc->Add(Vector(5000.0, 5000.0, 0.0));
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.Install(enbNodes);

    NodeContainer ueGroup1, ueGroup2, ueGroup3, ueGroup4;

    for (uint32_t i = 0; i < numOfUEs; i++)
    {
        if (i < 10)
        {
            ueGroup1.Add(ueNodes.Get(i));
        }
        else if (i < 20)
        {
            ueGroup2.Add(ueNodes.Get(i));
        }
        else if (i < 30)
        {
            ueGroup3.Add(ueNodes.Get(i));
        }
        else
        {
            ueGroup4.Add(ueNodes.Get(i));
        }
    }

    // Mobility model for ue

    for (uint32_t i = 0; i < 4; i++)
    {
        Vector enbPosition = enbNodes.Get(i)->GetObject<MobilityModel>()->GetPosition();
        MobilityHelper ueMobility;
        ueMobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                        "X",
                                        DoubleValue(enbPosition.x),
                                        "Y",
                                        DoubleValue(enbPosition.y),
                                        "rho",
                                        DoubleValue(500.0));
        // ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        //                             "Bounds",
        //                             StringValue("-500|5500|-500|5500"),
        //                             // "Bounds",
        //                             // RectangleValue(Rectangle(-500, 500, -500, 500)),
        //                             "Speed",
        //                             StringValue("ns3::ConstantRandomVariable[Constant=10.0]"));
        ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        if (i == 0)
        {
            ueMobility.Install(ueGroup1);
        }
        else if (i == 1)
        {
            ueMobility.Install(ueGroup2);
        }
        else if (i == 2)
        {
            ueMobility.Install(ueGroup3);
        }
        else
        {
            ueMobility.Install(ueGroup4);
        }
    }

    // Create Devices and install them in nodes enb and ue
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;

    std::cout<< schedulerType << std::endl;

    if(schedulerType == "PF")
    {
        lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
        std::cout << "PF" << std::endl;
    }
    else if (schedulerType == "RR")
    {
        lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
        std::cout << "RR" << std::endl;
    }
    else if(schedulerType == "PSS")
    {
        lteHelper->SetSchedulerType("ns3::PssFfMacScheduler");
        std::cout << "PSS" << std::endl;
    }
    else
    {
        lteHelper->SetSchedulerType("ns3::TdMtFfMacScheduler");
        std::cout << "MT" << std::endl;
    }

    lteHelper->SetSchedulerAttribute("HarqEnabled", BooleanValue(true));

    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth));

    // // TBU
    // lteHelper->SetFfrAlgorithmType("ns3::LteFfrDistributedAlgorithm");
    // lteHelper->SetFfrAlgorithmAttribute("CalculationInterval", TimeValue(MilliSeconds(200)));
    // lteHelper->SetFfrAlgorithmAttribute("RsrpDifferenceThreshold", UintegerValue(5));
    // lteHelper->SetFfrAlgorithmAttribute("RsrqThreshold", UintegerValue(25));
    // lteHelper->SetFfrAlgorithmAttribute("EdgeRbNum", UintegerValue(6));
    // lteHelper->SetFfrAlgorithmAttribute("CenterPowerOffset",
    //                                     UintegerValue(LteRrcSap::PdschConfigDedicated::dB_3));
    // lteHelper->SetFfrAlgorithmAttribute("EdgePowerOffset",
    //                                     UintegerValue(LteRrcSap::PdschConfigDedicated::dB3));

    // lteHelper->SetFfrAlgorithmAttribute("CenterAreaTpc", UintegerValue(0));
    // lteHelper->SetFfrAlgorithmAttribute("EdgeAreaTpc", UintegerValue(3));
    // TBU

    // ns3::LteFfrDistributedAlgorithm works with Absolute Mode Uplink Power Control
    // Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(false));

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // X2 Interface
    lteHelper->AddX2Interface(enbNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    // Attach UEs to the closest eNB
    lteHelper->AttachToClosestEnb(ueDevs, enbDevs);
    // for (uint32_t i = 0; i < numOfUEs; i++)
    // {
    //     for (uint32_t j = 0; j < 4; j++)
    //     {
    //         lteHelper->Attach(ueDevs.Get(i), enbDevs.Get(j));
    //     }
    // }


    // randomize a bit start times to avoid simulation artifacts
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.010));

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);

        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Install and start applications on UEs and remote host
    // uint16_t ulPort = 20000;
    uint16_t dlPort = 10000;
    ++dlPort;
    // ++ulPort;

    ApplicationContainer serverApps;

    // The sink will always listen to the specified ports
    UdpServerHelper dlPacketSinkHelper(dlPort);
    serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(0)));

    UdpClientHelper dlClient;
    dlClient.SetAttribute("RemotePort", UintegerValue(dlPort));
    dlClient.SetAttribute("PacketSize", UintegerValue(udpPacketSize));
    dlClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));

    bool udpFullBuffer = true;
    double ueNumPereNb = 10;

    if(udpFullBuffer)
    {
        double bitRate = 12000000;
        bitRate /= ueNumPereNb;    // Divide the cell capacity among UEs
        if (bandwidth > 20e6)
        {
            bitRate *= bandwidth / 20e6;
        }
        // lambda = bitRate / static_cast<double>(udpPacketSize * 8);
    }
    dlClient.SetAttribute("Interval", TimeValue(Seconds(0.01)));

    // The bearer that will carry low latency traffic
    EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);

    Ptr<EpcTft> tft = Create<EpcTft>();
    EpcTft::PacketFilter dlpf;
    dlpf.localPortStart = dlPort;
    dlpf.localPortEnd = dlPort;
    tft->Add(dlpf);

    ApplicationContainer clientApps;

    for (uint32_t i = 0; i < ueNodes.GetN(); ++i)
    {
        Ptr<Node> ue = ueNodes.Get(i);
        Ptr<NetDevice> ueDevice = ueDevs.Get(i);
        Address ueAddress = ueIpIfaces.GetAddress(i);

        // The client, who is transmitting, is installed in the remote host,
        // with destination address set to the address of the UE
        dlClient.SetAttribute("RemoteAddress", AddressValue(ueAddress));
        clientApps.Add(dlClient.Install(remoteHost));

        // Activate a dedicated bearer for the traffic type
        lteHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tft);
    }

    // Time startTime = Seconds(startTimeSeconds->GetValue());
    double startTime = startTimeSeconds->GetValue();
    // start server and client apps
    serverApps.Start(Seconds(startTime));
    clientApps.Start(Seconds(startTime));
    serverApps.Stop(Seconds(simTime));
    clientApps.Stop(Seconds(simTime));

    lteHelper->EnableTraces();
    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    endpointNodes.Add(remoteHost);
    endpointNodes.Add(ueNodes);

    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));

    // AnimationInterface anim("new-lte.xml");

    // To log the course change of UEs movements
    // Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::ofstream outFile;
    std::string filename = outputDir + "/" + simTag + "_" + schedulerType ;
    outFile.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!outFile.is_open())
    {
        std::cout << "Error opening file" << filename << std::endl;
        // NS_LOG_ERROR("Can't open file " << filename);
        return 1;
    }
    outFile.setf(std::ios_base::fixed);


    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
        {
            protoStream.str("TCP");
        }
        if (t.protocol == 17)
        {
            protoStream.str("UDP");
        }

        outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":" << t.destinationPort << ") proto " << protoStream.str() << "\n";
        outFile << "  Tx Packets: " << i->second.txPackets << "\n";
        outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
        outFile << "  TxOffered:  " << i->second.txBytes * 8.0 / (simTime - startTime) / 1000 / 1000 << " Mbps\n";
        outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";

        if (i->second.rxPackets > 0)
        {
            // Measure the duration of the flow from receiver's perspective
            double rxDuration =
                i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();

            averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

            outFile << "  Throughput: " << i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000
                    << " Mbps\n";
            outFile << "  Mean delay:  "
                    << 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets << " ms\n";
            // outFile << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << "
            // Mbps \n";
            outFile << "  Mean jitter:  "
                    << 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets << " ms\n";
        }
        else
        {
            outFile << "  Throughput:  0 Mbps\n";
            outFile << "  Mean delay:  0 ms\n";
            outFile << "  Mean upt:  0  Mbps \n";
            outFile << "  Mean jitter: 0 ms\n";
        }
        outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
    }

    double meanFlowThroughput = averageFlowThroughput / stats.size();
    double meanFlowDelay = averageFlowDelay / stats.size();
    Ptr<UdpServer> serverApp = serverApps.Get(0)->GetObject<UdpServer>();
    double totalUdpThroughput =
        ((serverApp->GetReceived() * udpPacketSize * 8) / (simTime - startTime)) * 1e-6;

    outFile << "\n\n  Mean flow throughput: " << meanFlowThroughput << "\n";
    outFile << "\n\n  Average flow throughput: " << averageFlowThroughput << "\n";
    outFile << "  Mean flow delay: " << meanFlowDelay << "\n";
    outFile << "\n UDP throughput (bps) for UE with node ID 0:" << totalUdpThroughput << std::endl;

    outFile.close();

    std::ifstream f(filename.c_str());

    if (f.is_open())
    {
        std::cout << f.rdbuf();
    }

    Simulator::Destroy();
    return 0;
}