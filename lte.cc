#include <ns3/epc-helper.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-helper.h>
#include <ns3/point-to-point-helper.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteLogComponent");

int
main(int argc, const char* argv[])
{
    LogComponentEnable("LteHelper", LOG_LEVEL_INFO);
    // LteHelper provides the methods to add eNBs and UEs and configure them
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>(); // Create an LteHelper object

    // LTE EPC setup
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(40));

    uint32_t numberOfEnbs = 4;
    uint32_t numberOfUes = 40;

    // Configure LTE operating UL and DL bandwidth in terms of RBs
    uint8_t RBs = 50;
    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(RBs));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(RBs));

    // Creating Nodes
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnbs);

    NodeContainer ueNodes;
    ueNodes.Create(numberOfUes);

    // Mobility model adddition
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

    // Installing LTE devices in eNB and UE
    NetDeviceContainer enbDevs;
    enbDevs = lteHelper->InstallEnbDevice(enbNodes); // Install an LTE protocol stack on the eNbs
    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice(ueNodes); // Install an LTE protocol stack on the UEs

    // Create single remote host
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Pgw node
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Connect the remote host with the P-GW using a P2P channel of 1 Gbps
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(5)));
    NetDeviceContainer internetDevices = p2p.Install(pgw, remoteHost);

    // To exchange traffic between internet and LTE network
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpInterface = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpInterface.GetAddress(1);

    // IP stack on UE
    InternetStackHelper internetUe;
    internetUe.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces;
    ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));            

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

    Simulator::Stop(Seconds(3.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
