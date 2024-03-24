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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteLogComponent");

void UEsConnectionStatus(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
    std::cout << context << " UE IMSI " << imsi << " connected to cellId " << cellId << " with rnti " << rnti << "Time:" << Simulator::Now() << std::endl;
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

    NodeContainer enbNodes;
    enbNodes.Create(4);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));

    NodeContainer ueNodes;
    ueNodes.Create(40);

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(40));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(20));

    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                       BooleanValue(true));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                       BooleanValue(true));

    uint8_t RBs = 50;
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(RBs));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(RBs));

    // Set operating frequency
    lteHelper->SetEnbDeviceAttribute("DlEarfcn", UintegerValue(100));
    lteHelper->SetEnbDeviceAttribute("UlEarfcn", UintegerValue(18100));

    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel");
    ueMobility.Install(ueNodes);

    // Set the position of the eNBs as a sqaure with sides 5km
    MobilityHelper enbMobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(5000.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 5000.0, 0.0));
    positionAlloc->Add(Vector(5000.0, 5000.0, 0.0));
    enbMobility.SetPositionAllocator(positionAlloc);
    enbMobility.Install(enbNodes);

    /*Above instances at this point still don't have an LTE protocol stack installed, they
    are just empty nodes*/

    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes); // Install an LTE protocol stack on the eNbs

    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice(ueNodes); // Install an LTE protocol stack on the UEs

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

    // LTE EPC setup
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Pgw node
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Making internet node to connect with pgw
    NodeContainer internetNode;
    internetNode.Create(1);
    InternetStackHelper internet;
    internet.Install(internetNode);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(5)));
    NetDeviceContainer internetDevices = p2p.Install(pgw, internetNode.Get(0));

    // to exchange traffic between internet and LTE network
    Ipv4AddressHelper ip;
    ip.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces = ip.Assign(internetDevices);
    // Ipv4Address internetHostAddr = interfaces.GetAddress(1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> internetHostRouting = ipv4RoutingHelper.GetStaticRouting(internetNode.Get(0)->GetObject<Ipv4>());
    internetHostRouting->AddNetworkRouteTo(epcHelper->GetUeDefaultGatewayAddress(), Ipv4Mask("255.0.0.0"), 1); // epcHelper->___ ??

    // setting up ue now
    InternetStackHelper internetUe;
    internetUe.Install(ueNodes);
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        Ptr<NetDevice> ueLteDevice = ueDevs.Get(u);
        Ipv4InterfaceContainer ueIpIface;
        ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevice));
        Ptr<Ipv4StaticRouting> ueStaticRouting;
        ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    //trace tracking
    lteHelper->EnableTraces();
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                    MakeCallback(&UEsConnectionStatus));

    //Flow monitor
    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    flowmon = flowmonHelper.InstallAll();
    

    Simulator::Stop(Seconds(30));
    Simulator::Run();
    flowmon->SerializeToXmlFile("lte_flowmon.xml", true, true);
    Simulator::Destroy();
    return 0;
}
