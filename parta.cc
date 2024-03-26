#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
// #include <ns3/net-device-container.h>
#include <ns3/netanim-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-static-routing-helper.h>
#include <ns3/flow-monitor.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/udp-client-server-helper.h>
#include <ns3/packet-sink-helper.h>
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteLogComponent");

/**
 * UE Connection established notification.
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The Cell ID.
 * \param rnti The RNTI.
 */
void
NotifyConnectionEstablishedUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi
              << ": connected to CellId " << cellid << " with RNTI " << rnti << std::endl;
}

/**
 * UE Start Handover notification.
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The actual Cell ID.
 * \param rnti The RNTI.
 * \param targetCellId The target Cell ID.
 */
void
NotifyHandoverStartUe(std::string context,
                      uint64_t imsi,
                      uint16_t cellid,
                      uint16_t rnti,
                      uint16_t targetCellId)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi
              << ": previously connected to CellId " << cellid << " with RNTI " << rnti
              << ", doing handover to CellId " << targetCellId << std::endl;
}

/**
 * UE Handover end successful notification.
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The Cell ID.
 * \param rnti The RNTI.
 */
void
NotifyHandoverEndOkUe(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " UE IMSI " << imsi
              << ": successful handover to CellId " << cellid << " with RNTI " << rnti << std::endl;
}

/**
 * eNB Connection established notification.
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The Cell ID.
 * \param rnti The RNTI.
 */
void
NotifyConnectionEstablishedEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << ": successful connection of UE with IMSI " << imsi << " RNTI " << rnti
              << std::endl;
}

/**
 * eNB Start Handover notification.
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The actual Cell ID.
 * \param rnti The RNTI.
 * \param targetCellId The target Cell ID.
 */
void
NotifyHandoverStartEnb(std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << ": start handover of UE with IMSI " << imsi << " RNTI " << rnti << " to CellId "
              << targetCellId << std::endl;
}

/**
 * eNB Handover end successful notification.
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The Cell ID.
 * \param rnti The RNTI.
 */
void
NotifyHandoverEndOkEnb(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << ": completed handover of UE with IMSI " << imsi << " RNTI " << rnti << std::endl;
}

/**
 * Handover failure notification
 *
 * \param context The context.
 * \param imsi The IMSI of the connected terminal.
 * \param cellid The Cell ID.
 * \param rnti The RNTI.
 */
void
NotifyHandoverFailure(std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti)
{
    std::cout << Simulator::Now().As(Time::S) << " " << context << " eNB CellId " << cellid
              << " IMSI " << imsi << " RNTI " << rnti << " handover failure" << std::endl;
}

int main(int argc, char *argv[])
{
    // NS_LOG_INFO("Starting LTE simulation");

    // LteHelper provides the methods to add eNBs and UEs and configure them
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>(); // Create an LteHelper object

    // NS_LOG_INFO("Creating Topology");
    // LogComponentEnableAll(LOG_LEVEL_INFO);
    // LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
    // LogComponentEnable("LteEnbNetDevice", LOG_LEVEL_INFO);
    // LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);
    // LogComponentEnable("LteEnbPhy", LOG_LEVEL_INFO);
    // LogComponentEnable("LteRlc", LOG_LEVEL_INFO);
    // LogComponentEnable("MobilityHelper", LOG_LEVEL_INFO);
    // LogComponentEnable("EpcHelper", LOG_LEVEL_INFO);
    LogComponentEnable("LteHelper", LOG_LEVEL_INFO);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));


    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(40));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(20));

    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                       BooleanValue(true));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                       BooleanValue(true));

    // LTE EPC setup
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Scheduling
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

    //Handover
    lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("ServingCellThreshold", UintegerValue(30));
    lteHelper->SetHandoverAlgorithmAttribute("NeighbourCellOffset", UintegerValue(1));
    
    // Set operating frequency
    lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));
    lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100));

    uint8_t RBs = 50;
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(RBs));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(RBs));

    //pathloss model
    // lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));

    // EPS Bearer actvation
    // EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    // EpsBearer bearer(q);
    // lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    // Pgw node
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create single remote host
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Creating internet
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(5)));
    NetDeviceContainer internetDevices = p2p.Install(pgw, remoteHost);

    // to exchange traffic between internet and LTE network
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpInterface = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpInterface.GetAddress(1);

    // Routing of internet host towards lte network
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1); // interface 0 is localhost, 1 is p2p device

    uint32_t numberOfUes = 40;
    uint32_t numberOfEnbs = 4;
    uint32_t numBearersPerUe = 1;
    Time simTime = MilliSeconds(490);
    bool disableDl = false;
    bool disableUl = false;

    // Creating Nodes
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbs);

    NodeContainer ueNodes;
    ueNodes.Create(numberOfUes);

    // Mobility model adddition
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel");
    ueMobility.Install(ueNodes);


    //interference management
    // lteHelper->SetFfrAlgorithmType("ns3::LteFrHardAlgorithm");

    // Set the position of the eNBs as a sqaure with sides 5km
    MobilityHelper enbMobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(5000.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 5000.0, 0.0));
    positionAlloc->Add(Vector(5000.0, 5000.0, 0.0));
    enbMobility.SetPositionAllocator(positionAlloc);
    enbMobility.Install(enbNodes);

    // Installing LTE devices in eNB and UE
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes); // Install an LTE protocol stack on the eNbs
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice(ueNodes); // Install an LTE protocol stack on the UEs

    // IP stack on UE
    InternetStackHelper internetUe;
    internetUe.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));
    // for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    // {
    //     Ptr<Node> ueNode = ueNodes.Get(u);
    //     Ptr<NetDevice> ueLteDevice = ueDevs.Get(u);
    //     Ipv4InterfaceContainer ueIpIface;
    //     ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevice));
    //     Ptr<Ipv4StaticRouting> ueStaticRouting;
    //     ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
    //     ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    // }


    // Attach uenodes to enb
    int k = 0;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            lteHelper->Attach(ueDevs.Get(j + k), enbDevs.Get(i)); // Attach 10 UEs per eNB
            /*This will configure each UE according to the eNB configuration, and create
            an RRC connection between them*/
        }
        k += 10;
    }

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;

    // randomize a bit start times to avoid simulation artifacts
    // (e.g., buffer overflows due to packet transmissions happening
    // exactly at the same time)
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetAttribute("Min", DoubleValue(0.05));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.06));


    for (uint32_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
            ApplicationContainer clientApps;
            ApplicationContainer serverApps;
            Ptr<EpcTft> tft = Create<EpcTft>();

            if (!disableDl)
            {
                ++dlPort;

                NS_LOG_LOGIC("installing UDP DL app for UE " << u);
                UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
                clientApps.Add(dlClientHelper.Install(remoteHost));
                PacketSinkHelper dlPacketSinkHelper(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), dlPort));
                serverApps.Add(dlPacketSinkHelper.Install(ue));

                EpcTft::PacketFilter dlpf;
                dlpf.localPortStart = dlPort;
                dlpf.localPortEnd = dlPort;
                tft->Add(dlpf);
            }

            if (!disableUl)
            {
                ++ulPort;

                NS_LOG_LOGIC("installing UDP UL app for UE " << u);
                UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
                clientApps.Add(ulClientHelper.Install(ue));
                PacketSinkHelper ulPacketSinkHelper(
                    "ns3::UdpSocketFactory",
                    InetSocketAddress(Ipv4Address::GetAny(), ulPort));
                serverApps.Add(ulPacketSinkHelper.Install(remoteHost));

                EpcTft::PacketFilter ulpf;
                ulpf.remotePortStart = ulPort;
                ulpf.remotePortEnd = ulPort;
                tft->Add(ulpf);
            }

            EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
            lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);

            Time startTime = Seconds(startTimeSeconds->GetValue());
            serverApps.Start(startTime);
            clientApps.Start(startTime);
            clientApps.Stop(simTime);

        } // end for b
    }

    // Configure Radio Environment Map (REM) output
    // for LTE-only simulations always use /ChannelList/0 which is the downlink channel
    // Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper>();
    // remHelper->SetAttribute("ChannelPath", StringValue("/ChannelList/0"));
    // remHelper->SetAttribute("OutputFile", StringValue("rem.out"));
    // remHelper->SetAttribute("XMin", DoubleValue(-3000.0));
    // remHelper->SetAttribute("XMax", DoubleValue(7000.0));
    // remHelper->SetAttribute("YMin", DoubleValue(-3000.0));
    // remHelper->SetAttribute("YMax", DoubleValue(7000.0));
    // remHelper->SetAttribute("Z", DoubleValue(0.0));
    // remHelper->Install();


    //trace tracking
    lteHelper->EnableTraces();
    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats();
    rlcStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));
    Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats();
    pdcpStats->SetAttribute("EpochDuration", TimeValue(Seconds(0.05)));

    // connect custom trace sinks for RRC connection establishment and handover notification
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                    MakeCallback(&NotifyConnectionEstablishedUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                    MakeCallback(&NotifyHandoverStartUe));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkEnb));
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                    MakeCallback(&NotifyHandoverEndOkUe));

    // Hook a trace sink (the same one) to the four handover failure traces
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureNoPreamble",
                    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureMaxRach",
                    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureLeaving",
                    MakeCallback(&NotifyHandoverFailure));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverFailureJoining",
                    MakeCallback(&NotifyHandoverFailure));

    //Flow monitor
    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    flowmon = flowmonHelper.InstallAll();
    
    AnimationInterface anim("lte.xml");

    Simulator::Stop(simTime + Seconds(1));
    Simulator::Run();
    flowmon->SerializeToXmlFile("/home/tusharc/lte_flowmon.xml", true, true);
    Simulator::Destroy();
    return 0;
}
