#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
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

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(40));

    // Enable Logging
    auto logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

    LogComponentEnable("LteHelper", logLevel);
    LogComponentEnable("EpcHelper", logLevel);
    // LogComponentEnable("EpcEnbApplication", logLevel);
    // LogComponentEnable("EpcMmeApplication", logLevel);
    // LogComponentEnable("EpcPgwApplication", logLevel);
    // LogComponentEnable("EpcSgwApplication", logLevel);
    LogComponentEnable("LteEnbRrc", logLevel);
    LogComponentEnable("LteUeRrc", logLevel);

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
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
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

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 10000;
    // uint16_t ulPort = 20000;

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

        for (uint32_t b = 0; b < 1; ++b)
        {
            ++dlPort;
            // ++ulPort;

            ApplicationContainer clientApps;
            ApplicationContainer serverApps;

            UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
            dlClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            dlClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1.0)));
            dlClientHelper.SetAttribute("PacketSize", UintegerValue(1500));
            clientApps.Add(dlClientHelper.Install(remoteHost));
            PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory",
                                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            serverApps.Add(dlPacketSinkHelper.Install(ue));

            // UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
            // ulClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
            // ulClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1.0)));
            // clientApps.Add(ulClientHelper.Install(ue));
            // PacketSinkHelper ulPacketSinkHelper("ns3::UdpSocketFactory",
            //                                     InetSocketAddress(Ipv4Address::GetAny(), ulPort));
            // serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

            Ptr<EpcTft> tft = Create<EpcTft>();
            EpcTft::PacketFilter dlpf;
            dlpf.localPortStart = dlPort;
            dlpf.localPortEnd = dlPort;
            tft->Add(dlpf);
            // EpcTft::PacketFilter ulpf;
            // ulpf.remotePortStart = ulPort;
            // ulpf.remotePortEnd = ulPort;
            // tft->Add(ulpf);
            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
        }
    }

    lteHelper->EnableTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));

    // // Trace sink for the packet sink of UE
    // std::ostringstream oss;
    // oss << "/NodeList/" << ueNodes.Get(0)->GetId() << "/ApplicationList/0/$ns3::PacketSink/Rx";
    // Config::ConnectWithoutContext(oss.str(), MakeCallback(&ReceivePacket));

    for (uint32_t i = 0; i < 40; ++i)
    {
        std::ostringstream oss;
        oss << "/NodeList/" << ueNodes.Get(i)->GetId() << "/ApplicationList/0/$ns3::PacketSink/Rx";
        Config::ConnectWithoutContext(oss.str(), MakeCallback(&ReceivePacket));
    }

    bool firstWrite = true;
    std::string rrcType = useIdealRrc ? "ideal_rrc" : "real_rrc";
    std::string fileName = "rlf_dl_thrput_speed_0_RrFfMacScheduler_" + std::to_string(enbNodes.GetN()) + "_eNB_" + rrcType;
    Time binSize = Seconds(0.2);
    Simulator::Schedule(Seconds(0.47), &Throughput, firstWrite, binSize, fileName);

    // AnimationInterface anim("new-lte.xml");

    // To log the course change of UEs movements
    // Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}