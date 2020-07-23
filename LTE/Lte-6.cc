#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/lte-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <sstream>
#include "ns3/epc-s1ap-sap.h"
#include "ns3/gnuplot.h"

using namespace std;
using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("4G-LTE");

int
main (int argc, char *argv[])
{
  uint16_t numNodePairs = 3;
  Time simTime = MilliSeconds (1900);
  double distance = 60.0;
  Time interPacketInterval = MilliSeconds (100);
  bool disableDl = false;
  bool disableUl = false;
  bool useHelper = false;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("numNodePairs", "Number of eNodeBs + UE pairs", numNodePairs);
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue ("interPacketInterval", "Inter packet interval", interPacketInterval);
  cmd.AddValue ("disableDl", "Disable downlink data flows", disableDl);
  cmd.AddValue ("disableUl", "Disable uplink data flows", disableUl);
  cmd.AddValue ("useHelper", "Build the backhaul network using the helper or "
                             "it is built in the example", useHelper);
  cmd.Parse (argc, argv);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);


  Ptr<EpcMmeApplication> MmeHelper=CreateObject<EpcMmeApplication>();
  Ptr<Node> mme = MmeHelper->GetNode();
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30));
  Config::SetDefault ("ns3::LteEnbPhy::NoiseFigure", DoubleValue (5));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (10.0));
  //Config::SetDefault ("ns3::LteUePhy:NoiseFigure",UintegerValue(8));
  Config::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue (true));
  Ptr<EpcHelper> epcHelper;
  if (!useHelper)
    {
      epcHelper = CreateObject<NoBackhaulEpcHelper> ();
    }
  else
    {
      epcHelper = CreateObject<PointToPointEpcHelper> ();
    }
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");  // FD-TBFQ scheduler
  lteHelper->SetSchedulerAttribute ("CqiTimerThreshold", UintegerValue (3));
  lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
  lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold", UintegerValue (30));
  lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",UintegerValue (1));


  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numNodePairs);
  ueNodes.Create (8);
  Ptr<Node> ueNode1 = ueNodes.Get(0);
  Ptr<Node> ueNode2 = ueNodes.Get(1);
  Ptr<Node> ueNode3 = ueNodes.Get(2);
  Ptr<Node> ueNode4 = ueNodes.Get(3);
  Ptr<Node> ueNode5 = ueNodes.Get(4);
  Ptr<Node> ueNode6 = ueNodes.Get(5);
  Ptr<Node> ueNode7 = ueNodes.Get(6);
  Ptr<Node> ueNode8 = ueNodes.Get(7);
  Ptr<Node> enbNode1 = enbNodes.Get(0);

  MobilityHelper mobility;
/*
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("-150.0"),
                                 "Y", StringValue ("-50.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=80|Max=100]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("0.01s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=100.0]"),
							  "Bounds", RectangleValue (Rectangle (-250, 300, -250, 300)));
  */
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("0.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=200|Max=300]"));
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                                "Bounds", RectangleValue (Rectangle (0, 100, 0, 300)),
                                "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=110]"),
                                "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.0001]"));
  	mobility.Install(ueNodes);
   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   //mobility.Install(ueNodes);
   mobility.Install(enbNodes);

   //for(int i=0;i<8;i++)
   //{
	 //  if(i==3)
		//   i++;
	   //else
		//   mobility.Install (ueNodes.Get(i));
   //}

  // SGW node
  Ptr<Node> sgw = epcHelper->GetSgwNode ();

  // Install Mobility Model for SGW
  Ptr<ListPositionAllocator> positionAlloc2 = CreateObject<ListPositionAllocator> ();
  positionAlloc2->Add (Vector (0.0,  50.0, 0.0));
  MobilityHelper mobility2;
  mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility2.SetPositionAllocator (positionAlloc2);
  mobility2.Install (sgw);


  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute("DlEarfcn",UintegerValue(100));
  lteHelper->SetEnbDeviceAttribute("UlEarfcn",UintegerValue(18100));

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  if (!useHelper)
    {
      Ipv4AddressHelper s1uIpv4AddressHelper;

      // Create networks of the S1 interfaces
      s1uIpv4AddressHelper.SetBase ("10.0.0.0", "255.255.255.252");

      for (uint16_t i = 0; i < numNodePairs; i++)
        {
          Ptr<Node> enb = enbNodes.Get (i);

          // Create a point to point link between the eNB and the SGW with
          // the corresponding new NetDevices on each side
          PointToPointHelper p2ph;
          DataRate s1uLinkDataRate = DataRate ("10Gb/s");
          uint16_t s1uLinkMtu = 2000;
          Time s1uLinkDelay = Time (0);
          p2ph.SetDeviceAttribute ("DataRate", DataRateValue (s1uLinkDataRate));
          p2ph.SetDeviceAttribute ("Mtu", UintegerValue (s1uLinkMtu));
          p2ph.SetChannelAttribute ("Delay", TimeValue (s1uLinkDelay));
          NetDeviceContainer sgwEnbDevices = p2ph.Install (sgw, enb);

          Ipv4InterfaceContainer sgwEnbIpIfaces = s1uIpv4AddressHelper.Assign (sgwEnbDevices);
          s1uIpv4AddressHelper.NewNetwork ();

          Ipv4Address sgwS1uAddress = sgwEnbIpIfaces.GetAddress (0);
          Ipv4Address enbS1uAddress = sgwEnbIpIfaces.GetAddress (1);

          // Create S1 interface between the SGW and the eNB
          epcHelper->AddS1Interface (enb, enbS1uAddress, sgwS1uAddress);
        }
    }
/*
  if (!useHelper)
    {
      Ipv4AddressHelper s1uIpv4AddressHelper;

      // Create networks of the S1 interfaces
      s1uIpv4AddressHelper.SetBase ("10.1.1.0", "255.255.255.252");

      for (uint16_t i = 0; i < numNodePairs; ++i)
        {
          Ptr<Node> enb = enbNodes.Get (i);

          // Create a point to point link between the eNB and the SGW with
          // the corresponding new NetDevices on each side
          PointToPointHelper p2ph1;
          DataRate s1apLinkDataRate = DataRate ("10Gb/s");
          uint16_t s1apLinkMtu = 2000;
          Time s1apLinkDelay = Time (0);
          p2ph.SetDeviceAttribute ("DataRate", DataRateValue (s1apLinkDataRate));
          p2ph.SetDeviceAttribute ("Mtu", UintegerValue (s1apLinkMtu));
          p2ph.SetChannelAttribute ("Delay", TimeValue (s1apLinkDelay));
          NetDeviceContainer mmeEnbDevices = p2ph.Install (mme, enb);

          Ipv4InterfaceContainer mmeEnbIpIfaces = s1uIpv4AddressHelper.Assign (mmeEnbDevices);
          s1uIpv4AddressHelper.NewNetwork ();

          Ipv4Address mmeS1apAddress = mmeEnbIpIfaces.GetAddress (0);
          Ipv4Address enbS1apAddress = mmeEnbIpIfaces.GetAddress (1);

          // Create S1 interface between the SGW and the eNB
          MmeHelper->AddEnb(enb, enbS1uAddress,);
        }
    }

  Ptr<Socket> mmeS11Socket =Socket::CreateSocket(mme,);

  Ipv4AddressHelper s11Ipv4AddressHelper;

  // Create networks of the S1 interfaces
  s11Ipv4AddressHelper.SetBase ("10.1.2.0", "255.255.255.0");
  	  	   PointToPointHelper p2ph2;
           DataRate s11LinkDataRate = DataRate ("10Gb/s");
           uint16_t s11LinkMtu = 2000;
           Time s11LinkDelay = Time (0);
           p2ph.SetDeviceAttribute ("DataRate", DataRateValue (s11LinkDataRate));
           p2ph.SetDeviceAttribute ("Mtu", UintegerValue (s11LinkMtu));
           p2ph.SetChannelAttribute ("Delay", TimeValue (s11LinkDelay));
           NetDeviceContainer sgwEnbDevices = p2ph2.Install (sgw, mme);

           Ipv4InterfaceContainer sgwMmeIpIfaces = s11Ipv4AddressHelper.Assign (sgwEnbDevices);
           s11Ipv4AddressHelper.NewNetwork ();

           Ipv4Address sgwS11Address = sgwMmeIpIfaces.GetAddress (0);
           Ipv4Address mmeS11Address = sgwMmeIpIfaces.GetAddress (1);

           // Create S1 interface between the SGW and the eNB
           MmeHelper->AddSgw(sgwS11Address,mmeS11Address,mmeS11Socket);
*/
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  lteHelper->Attach(ueLteDevs);
  lteHelper->AddX2Interface(enbNodes);


	enum EpsBearer::Qci q = EpsBearer::GBR_GAMING;  // define Qci type
	GbrQosInformation qos;
	qos.gbrDl = 80000000; // Downlink GBR
	qos.gbrUl = 56000000; // Uplink GBR
	qos.mbrDl = 120000000; // Downlink MBR
	qos.mbrUl = 80000000; // Uplink MBR
	EpsBearer bearer (q, qos);
	lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(3), bearer, EpcTft::Default());

	  enum EpsBearer::Qci q1= EpsBearer::GBR_NON_CONV_VIDEO;  // define Qci type
	  GbrQosInformation qom;
	  qom.gbrDl = 56000000; // Downlink GBR
	  qom.gbrUl = 32000000; // Uplink GBR
	  qom.mbrDl = 80000000; // Downlink MBR
	  qom.mbrUl = 56000000; // Uplink MBR
	  EpsBearer ebearer(q1, qom);
	  lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(1), ebearer, EpcTft::Default());


	  enum EpsBearer::Qci q2= EpsBearer::GBR_CONV_VOICE;  // define Qci type
	  GbrQosInformation qon;
	  qon.gbrDl = 48000000; // Downlink GBR
	  qon.gbrUl = 32000000; // Uplink GBR
	  qon.mbrDl = 64000000; // Downlink MBR
	  qon.mbrUl = 48000000; // Uplink MBR
	  EpsBearer mbearer(q2, qon);
	  lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(2), mbearer, EpcTft::Default());

	  enum EpsBearer::Qci q3= EpsBearer::GBR_CONV_VOICE;  // define Qci type
	  GbrQosInformation qot;
	  qot.gbrDl = 24000000; // Downlink GBR
	  qot.gbrUl = 8000000; // Uplink GBR
	  qot.mbrDl = 32000000; // Downlink MBR
	  qot.mbrUl = 18000000; // Uplink MBR
	  EpsBearer nbearer(q3, qot);
	  lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(0), nbearer, EpcTft::Default());


		enum EpsBearer::Qci q4 = EpsBearer::GBR_GAMING;  // define Qci type
		GbrQosInformation qoa;
		qoa.gbrDl = 96000000; // Downlink GBR
		qoa.gbrUl = 56000000; // Uplink GBR
		qoa.mbrDl = 112000000; // Downlink MBR
		qoa.mbrUl = 800000000; // Uplink MBR
		EpsBearer abearer (q4, qoa);
		lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(5), abearer, EpcTft::Default());


		enum EpsBearer::Qci q5 = EpsBearer::GBR_GAMING;  // define Qci type
		GbrQosInformation qob;
		qob.gbrDl = 72000000; // Downlink GBR
		qob.gbrUl = 48000000; // Uplink GBR
		qob.mbrDl = 96000000; // Downlink MBR
		qob.mbrUl = 88000000; // Uplink MBR
		EpsBearer bbearer (q5, qob);
		lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(6), bbearer, EpcTft::Default());



		enum EpsBearer::Qci q6 = EpsBearer::GBR_NON_CONV_VIDEO;  // define Qci type
		GbrQosInformation qoc;
		qoc.gbrDl = 80000000; // Downlink GBR
		qoc.gbrUl = 48000000; // Uplink GBR
		qoc.mbrDl = 120000000; // Downlink MBR
		qoc.mbrUl = 96000000; // Uplink MBR
		EpsBearer cbearer (q6, qoc);
		lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(7), cbearer, EpcTft::Default());



		enum EpsBearer::Qci q7 = EpsBearer::GBR_GAMING;  // define Qci type
		GbrQosInformation qod;
		qod.gbrDl = 88000000; // Downlink GBR
		qod.gbrUl = 40000000; // Uplink GBR
		qod.mbrDl = 112000000; // Downlink MBR
		qod.mbrUl = 88000000; // Uplink MBR
		EpsBearer dbearer (q7, qod);
		lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(4), dbearer, EpcTft::Default());



  AnimationInterface anim("Lte-6.xml");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1100;
  uint16_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
	  if (!disableDl)
        {
		  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
		  serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (u)));

		  UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
		  dlClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
		  dlClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
		  clientApps.Add (dlClient.Install (remoteHost));
        }

      if (!disableUl)
        {
          ++ulPort;
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          UdpClientHelper ulClient (remoteHostAddr, ulPort);
          ulClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
          ulClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
          clientApps.Add (ulClient.Install (ueNodes.Get(u)));
        }

    }

  serverApps.Start (MilliSeconds (500));
  clientApps.Start (MilliSeconds (500));


  lteHelper->SetFadingModel("ns3::TraceFadingLossModel");

  lteHelper->SetFadingModelAttribute ("TraceFilename", StringValue ("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
  lteHelper->SetFadingModelAttribute ("TraceLength", TimeValue (Seconds (10.0)));
  lteHelper->SetFadingModelAttribute ("SamplesNum", UintegerValue (10000));
  lteHelper->SetFadingModelAttribute ("WindowSize", TimeValue (Seconds (0.5)));
  lteHelper->SetFadingModelAttribute ("RbNum", UintegerValue (100));




  anim.SetConstantPosition(ueNodes.Get(0),-30.0,260.0);
  anim.SetConstantPosition(ueNodes.Get(1),20.0,220.0);
  anim.SetConstantPosition(ueNodes.Get(2),0.0,140.0);
  //anim.SetConstantPosition(ueNodes.Get(3),0.0,-30.0);
  anim.SetConstantPosition(ueNodes.Get(4),30.0,0.0);
  anim.SetConstantPosition(ueNodes.Get(5),-20.0,80.0);
  anim.SetConstantPosition(ueNodes.Get(6),70.0,170.0);
  anim.SetConstantPosition(ueNodes.Get(7),212.0,250.0);


  anim.SetConstantPosition(enbNodes.Get(0),190.0,10.0);
  anim.SetConstantPosition(enbNodes.Get(1),100.0,100.0);
  anim.SetConstantPosition(enbNodes.Get(2),190.0,280.0);

  anim.SetConstantPosition(pgw,320.0,5.0);
  anim.SetConstantPosition(remoteHost,380.0,5.0);
  anim.SetConstantPosition(sgw,280.0,110.0);
  //anim.SetConstantPosition(mme,330.0,280.0);

  anim.UpdateNodeDescription(6,"ENB-3");
  anim.UpdateNodeDescription(7,"MOBILE");
  anim.UpdateNodeDescription(4,"ENB-1");
  anim.UpdateNodeDescription(5,"ENB-2");
  anim.UpdateNodeDescription(1,"S-GW");
  anim.UpdateNodeDescription(0,"P-GW");
  anim.UpdateNodeDescription(3,"INTERNET");
  anim.UpdateNodeDescription(2,"MME");
  anim.UpdateNodeDescription(8,"LAPTOP");
  anim.UpdateNodeDescription(9,"TABLET");
  anim.UpdateNodeDescription(10,"CAR");
  anim.UpdateNodeDescription(11,"TABLET");
  anim.UpdateNodeDescription(12,"CAMERA");
  anim.UpdateNodeDescription(13,"DONGLE");
  anim.UpdateNodeDescription(14,"LAPTOP");
  anim.EnablePacketMetadata(true);



  anim.UpdateLinkDescription(4,5,"X2-INTERFACE");
  anim.UpdateLinkDescription(5,6,"X2-INTERFACE");
  anim.UpdateLinkDescription(4,6,"X2-INTERFACE");

  for(uint32_t i=0;i<15;i++)
  {
	  anim.UpdateNodeSize(i,82.0,72.0);
  }



  uint32_t resourceid1 = anim.AddResource("/home/tarun/Downloads/img/car.png");
  uint32_t resourceid2 = anim.AddResource("/home/tarun/Downloads/img/mobile.png");
  uint32_t resourceid3 = anim.AddResource("/home/tarun/Downloads/img/laptop.png");
  uint32_t resourceid4 = anim.AddResource("/home/tarun/Downloads/img/tower.png");
  uint32_t resourceid5 = anim.AddResource("/home/tarun/Downloads/img/server.png");
  uint32_t resourceid6 = anim.AddResource("/home/tarun/Downloads/img/Cloud.png");
  uint32_t resourceid7 = anim.AddResource("/home/tarun/Downloads/img/tablet.png");
  uint32_t resourceid8 = anim.AddResource("/home/tarun/Downloads/img/dongle.png");
  uint32_t resourceid9 = anim.AddResource("/home/tarun/Downloads/img/Camera.png");

  anim.UpdateNodeImage(7,resourceid2);
  anim.UpdateNodeImage(6,resourceid4);
  anim.UpdateNodeImage(4,resourceid4);
  anim.UpdateNodeImage(3,resourceid6);
  anim.UpdateNodeImage(2,resourceid5);
  anim.UpdateNodeImage(1,resourceid5);
  anim.UpdateNodeImage(0,resourceid5);
  anim.UpdateNodeImage(5,resourceid4);
  anim.UpdateNodeImage(8,resourceid3);
  anim.UpdateNodeImage(9,resourceid7);
  anim.UpdateNodeImage(10,resourceid1);
  anim.UpdateNodeImage(11,resourceid7);
  anim.UpdateNodeImage(12,resourceid9);
  anim.UpdateNodeImage(13,resourceid8);
  anim.UpdateNodeImage(14,resourceid3);

  anim.GetTracePktCount();

  Ptr<FlowMonitor> flowMonitor1;
   FlowMonitorHelper flowHelper1;
   flowMonitor1= flowHelper1.InstallAll();

   //flowMonitor1->SetAttribute("PacketSizeBinWidth",DoubleValue(20000));

   lteHelper->EnablePhyTraces ();
   lteHelper->EnableMacTraces ();
   lteHelper->EnableRlcTraces ();
   lteHelper->EnablePdcpTraces ();
   lteHelper->EnableDlPhyTraces();
   lteHelper->EnableUlPhyTraces();
   lteHelper->EnableDlRxPhyTraces();
   lteHelper->EnableDlTxPhyTraces();
   lteHelper->EnableUlRxPhyTraces();
   lteHelper->EnableUlTxPhyTraces();
   lteHelper->EnableUlPhyTraces();

  Simulator::Stop (Seconds(10));
  //GnuPlot
  Simulator::Run ();
    string fileNameWithNoExtension = "FLOWVSThroughput1";
          string graphicsFileName        = fileNameWithNoExtension + ".png";
          string plotFileName            = fileNameWithNoExtension + ".plt";
          string plotTitle               = "FLOW vs THROUGHPUT";
          string dataTitle               = "THROUGHPUT";

          // Instantiate the plot and set its title.
          Gnuplot gnuplot (graphicsFileName);
          gnuplot.SetTitle (plotTitle);

          // Make the graphics file, which the plot file will be when it
          // is used with Gnuplot, be a PNG file.
          gnuplot.SetTerminal ("png");

          // Set the labels for each axis.
          gnuplot.SetLegend ("FLOW", "Throughput");


          Gnuplot2dDataset dataset;
          dataset.SetTitle (dataTitle);
          dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

    //Flow Monitor ... continued


  	  double Throughput=0.0;

  	  flowMonitor1->CheckForLostPackets ();
  	  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper1.GetClassifier ());
  	  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor1->GetFlowStats ();

  	  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  	    {
  	    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

  	   	  NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
  	      NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
  	      NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
  	      NS_LOG_UNCOND("Delay Sum = " << iter->second.delaySum);
  	    //  NS_LOG_UNCOND("Delay = " << iter->second.delaySum / iter->second.rxPackets << "ns");
  	   //   NS_LOG_UNCOND("Delay = " << (iter->second.delaySum) / (iter->second.rxPackets)<< "ms");
  	      NS_LOG_UNCOND("Jitter Sum = " << iter->second.jitterSum);
  	      NS_LOG_UNCOND("Jitter = " << iter->second.jitterSum / (iter->second.rxPackets - 1) << "ns");
  	      NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 * 28/ 10000000 << " Mb/ps");
  	      Throughput=iter->second.rxBytes * 8.0*28 /
  	                ((iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())*1000000);

  	      for(uint16_t i=1;i<=8;i++)
  	      {
  	      dataset.Add(iter->first,(double) Throughput);
  	      }
  	      NS_LOG_UNCOND("Lost Packets = " << iter->second.lostPackets);
  	      NS_LOG_UNCOND("-------------------------------------------------");
  	      NS_LOG_UNCOND("-------------------------------------------------");

  	    }

  	  	  flowMonitor1->SerializeToXmlFile ("Lte-6.flowmon", true, true);



    //Gnuplot ...continued

        gnuplot.AddDataset (dataset);

                 // Open the plot file.
                 ofstream plotFile (plotFileName.c_str());

                 // Write the plot file.
                 gnuplot.GenerateOutput (plotFile);

                 // Close the plot file.
                 plotFile.close ();



  Simulator::Destroy ();
  return 0;
}

