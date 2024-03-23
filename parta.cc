#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
// #include <ns3/net-device-container.h>

using namespace ns3;

int main(int argc, char *argv[])
{
    //LteHelper provides the methods to add eNBs and UEs and configure them
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>(); //Create an LteHelper object

    NodeContainer enbNodes;
    enbNodes.Create(4);
    NodeContainer ueNodes;
    ueNodes.Create(40);

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
    enbDevs = lteHelper->InstallEnbDevice(enbNodes); //Install an LTE protocol stack on the eNbs

    NetDeviceContainer ueDevs;
    ueDevs = lteHelper->InstallUeDevice(ueNodes); //Install an LTE protocol stack on the UEs

    for(int i=0;i<4;i++)
    {
        int k=0;
        for(int j=0;j<10;j++)
        {
            lteHelper->Attach(ueDevs.Get(j+k),enbDevs.Get(i)); //Attach 10 UEs per eNB
            /*This will configure each UE according to the eNB configuration, and create
            an RRC connection between them*/
        }
        k+=10;
    }

    for(int i=0;i<4;i++)
    {
        Ptr<LteEnbPhy> enbiPhy=enbDevs.Get(i)->GetObject<LteEnbNetDevice>()->GetPhy();
        enbiPhy->SetTxPower(40);
    }


    Simulator::Stop(Seconds(30));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
