#include "ns3/wave-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "custom-mobility-model.h"
#include "ns3/node.h"
#include "hec_cert.h"
#include "sign.h"
#include "messages.h"

#include <set>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WaveExample1");

int ec_algo = 0;
int rsuid = 63;

Vehicle_data_g2 vehg2[100];
RSU_data_g2 rsug2[5];

Vehicle_data_g3 vehg3[100];
RSU_data_g3 rsug3[5];

Vehicle_data_ec vehec[100];
RSU_data_ec rsuec[5];

uint8_t hpk[23] = {0x87,0x75,0x6e,0x0e,0x30,0x8e,0x59,0xa4,0x04,0x48,0x01,
0x17,0x4c,0x4f,0x01,0x4d,0x16,0x78,0xe8,0x56,0x6e,0x03,0x02};


//Note: this is a promiscuous trace for all packet reception. This is also on physical layer, so packets still have WifiMacHeader
void Rx (std::string context, Ptr <const Packet> packet, uint16_t channelFreqMhz,  WifiTxVector txVector,MpduInfo aMpdu, SignalNoiseDbm signalNoise)
{

  //context will include info about the source of this event. Use string manipulation if you want to extract info.
  
  
  Ptr <Packet> myPacket = packet->Copy();
  //Print the info.
  

  //We can also examine the WifiMacHeader
  WifiMacHeader hdr;
  WifiMacHeader hdr1;
  WifiMacTrailer trl;

  uint8_t *buffrc = new uint8_t[packet->GetSize()];
  int vid = 0;
  if(context[11] == '/') 
    vid = context[10] - 48;
  else
    vid = (context[10] - 48)*10 + (context[11] - 48);

  int state;
  if(ec_algo == 0) {
    state = vehg2[vid].state;
  } 
  else if(ec_algo == 1) {
    state = vehec[vid].state;
  }
  else{
    state = vehg3[vid].state;
  }
  
  if (packet->PeekHeader(hdr))
  {
    myPacket->RemoveHeader(hdr1);
    myPacket->RemoveTrailer(trl);
    myPacket->CopyData(buffrc, packet->GetSize());

    Ptr<Node> n1 = ns3::NodeList::GetNode(vid);
    Ptr <NetDevice> nd0 = n1->GetDevice(0);
    if(hdr1.GetAddr1() != nd0->GetAddress() && hdr1.GetAddr1() != "ff:ff:ff:ff:ff:ff") {
      return;
    }
    std::cout << std::endl << BOLD_CODE <<  context << END_CODE << std::endl;

    std::cout << "\tSize=" << packet->GetSize()
        << " Freq="<<channelFreqMhz
        << " Mode=" << txVector.GetMode()
        << " Signal=" << signalNoise.signal
        << " Noise=" << signalNoise.noise << std::endl;
    std::cout << "\tDestination MAC : " << hdr.GetAddr1() << "\tSource MAC : " << hdr.GetAddr2() << std::endl << std::endl;
  }
  else {
    return;
  }

  if(buffrc[8] == RECEIVE_CERT && state == RECEIVE_CERT && (packet->GetSize()) > 20) {
    receive_Cert_Send_Join(buffrc, ec_algo, vid);
  }

  else if(buffrc[8] == RECEIVE_ACCEPT_KEY && state == RECEIVE_ACCEPT_KEY && (packet->GetSize()) > 20) {
    int rid = buffrc[9];
    extract_Symmetric(buffrc+10, ec_algo, vid, rid);
  }
}

void Rx1(std::string context, Ptr <const Packet> packet, uint16_t channelFreqMhz,  WifiTxVector txVector,MpduInfo aMpdu, SignalNoiseDbm signalNoise) {
  
  Ptr <Packet> myPacket = packet->Copy();


  //We can also examine the WifiMacHeader
  WifiMacHeader hdr;
  WifiMacHeader hdr1;
  WifiMacTrailer trl;

  uint8_t *buffrc = new uint8_t[packet->GetSize()];
  int prot,vid, rid=0;
  
  if (packet->PeekHeader(hdr))
  {
    myPacket->RemoveHeader(hdr1);
    myPacket->RemoveTrailer(trl);
    myPacket->CopyData(buffrc, packet->GetSize());

    Ptr<Node> n0 = ns3::NodeList::GetNode(rsuid);
    Ptr <NetDevice> nd0 = n0->GetDevice(0);
    if(hdr1.GetAddr1() != nd0->GetAddress() && hdr1.GetAddr1() != "ff:ff:ff:ff:ff:ff") {
      return;
    }
    std::cout << std::endl << BOLD_CODE <<  context << END_CODE << std::endl;
    std::cout << "\tSize=" << packet->GetSize()
        << " Freq="<<channelFreqMhz
        << " Mode=" << txVector.GetMode()
        << " Signal=" << signalNoise.signal
        << " Noise=" << signalNoise.noise << std::endl;
    std::cout << "\tDestination MAC : " << hdr.GetAddr1() << "\tSource MAC : " << hdr.GetAddr2() << std::endl << std::endl;

    prot = (int)buffrc[8];
    vid = (int)buffrc[9];

  }
  else {
    return;
  }
  if(prot == RECEIVE_CERT && (packet->GetSize()) > 20) {
    if(ec_algo == 0 && rsug2[rid].states[vid] == RECEIVE_CERT) {
      extract_RSU_SendAccept_g2(buffrc+10, vid, rid);
    }
    else if (ec_algo == 1 && rsuec[rid].states[vid] == RECEIVE_CERT) {
      extract_RSU_SendAccept_ec(buffrc+10, vid, rid);
    }
    else if (ec_algo == 2 && rsug3[rid].states[vid] == RECEIVE_CERT){
      extract_RSU_SendAccept_g3(buffrc+10, vid, rid);
    }
  }
}


/*
 * This function works for ns-3.30 onwards. For previous version, remove the last parameter (the "WifiPhyRxfailureReason")
 */
void RxDrop (std::string context, Ptr<const Packet> packet, ns3::WifiPhyRxfailureReason reason)
{
	std::cout << std::endl << BOLD_CODE << YELLOW_CODE << "Packet Rx Dropped!" << END_CODE << std::endl;
	//From ns-3.30, the reasons are defined in an enum type in ns3::WifiPhy class.
	std::cout << " Reason : " << reason << std::endl;
	std::cout << context << std::endl;

	WifiMacHeader hdr;
	if (packet->PeekHeader(hdr))
	{

		std::cout << " Destination MAC : " << hdr.GetAddr1() << "\tSource MAC : " << hdr.GetAddr2() << "\tSeq No. " << hdr.GetSequenceNumber() << std::endl << std::endl;
	}
}

//Fired when a packet is Enqueued in MAC
void EnqueueTrace(std::string context, Ptr<const WifiMacQueueItem> item)
{
	//std::cout << TEAL_CODE << "A Packet was enqueued : " << context << END_CODE << std::endl;

	Ptr <const Packet> p = item->GetPacket();
	/*
	 * Do something with the packet, like attach a tag. ns3 automatically attaches a timestamp for enqueued packets;
	 */

}
//Fired when a packet is Dequeued from MAC layer. A packet is dequeued before it is transmitted.
void DequeueTrace(std::string context, Ptr<const WifiMacQueueItem> item)
{
	//std::cout << TEAL_CODE << "A Packet was dequeued : " << context << END_CODE << std::endl;

	Ptr <const Packet> p = item->GetPacket();
	Time queue_delay = Simulator::Now() - item->GetTimeStamp();

	//Keep in mind that a packet might get dequeued (dropped_ if it exceeded MaxDelay (default is 500ms)
	//std::cout << "\tQueuing delay=" << queue_delay << std::endl;


}


void PrintInfo ()
{
    std::set<int> vehreg;
    int num, numacc=0;
    if(ec_algo == 0) {
      num = rsug2[0].numveh;
    }
    else if(ec_algo == 1) {
      num = rsuec[0].numveh;
    }
    else{
      num = rsug3[0].numveh;
    }

    for(int i=0; i < rsuid-1; i++) {
      switch (ec_algo)
      {
      case 0:
        if(vehg2[i].state == 2) {
          numacc++;
          vehreg.insert(i);
        }
        break;
      case 1:
        if(vehec[i].state == 2) {
          numacc++;
          vehreg.insert(i);
        }
        break;
      case 2:
        if(vehg3[i].state == 2) {
          numacc++;
          vehreg.insert(i);
        }
        break;
      default:
        break;
      }
    }

    std::cout << BOLD_CODE << " Registered vehicles in RSU: " << num << " Received Symmetric: ";
    for(auto& id : vehreg) {
      std::cout << id << ' ';
    }
    std::cout << END_CODE << std::endl;

    if(Now() > Seconds(150)) {
      std::cout << vehg2[13].symm << " " << vehg2[44].symm << std::endl;
    }
    
    // for(int i=0; i<10; i++){
    //   std::cout << "Vehicle " << i+1 << " is in state: " << vehg2[i].state << " "; 
    // }
    // std::cout << std::endl;

    Simulator::Schedule (Seconds (4), &PrintInfo);

}


int main (int argc, char *argv[])
{
  CommandLine cmd;
  int fullsize=0;
  uint8_t *cypher_buff;

  if(ec_algo == 0) {
    std::cout << "Using ElGamal with Genus 2 HEC for message encryption\nHECQV for certificates\nElGamal HEC genus 2 signatures" << std::endl;
    ZZ ptest = to_ZZ(pt);
    field_t::init(ptest);
    std::cout << "Using p: " << ptest << " of size: " << NumBits(ptest) << std::endl;
            
    NS_G2_NAMESPACE::g2hcurve curve;

    NS_G2_NAMESPACE::divisor g, h, rsupub, capub;
    ZZ rsupriv;
    UnifiedEncoding enc(ptest, 10, 4, 2);
    rsug2[0].u = 10;
    rsug2[0].w = 4;
    std::string base = "BaseforGenerator";
    int rt = text_to_divisor(g, base, ptest, curve, enc);
    if(rt) {
      exit(1);
    }


    ZZ capriv = to_ZZ("15669032110011017415376799675649225245106855015484313618141721121181084494176");
    ZZ x;
    capub = capriv*g;
    /* private key x */
    RandomBnd(x, ptest*ptest);
    divisor_to_bytes(rsug2[0].capub, capub, curve, ptest);

    h = x * g;

    g2HECQV cert2(curve, ptest, g);
    int size = NumBytes(ptest);
    uint8_t *encoded2 = new uint8_t[31 + 2*size+1];
    cert2.cert_generate(encoded2, "RSU0001", h, capriv);

    cert2.cert_pk_extraction(encoded2, capub);
    cert2.cert_reception(encoded2, x);
    
    rsupub = cert2.get_calculated_Qu();
    rsupriv = cert2.get_extracted_du();

    rsug2[0].priv = rsupriv;
    rsug2[0].curve = curve;
    divisor_to_bytes(rsug2[0].rsupub, rsupub, curve, ptest);
    divisor_to_bytes(rsug2[0].g, g, curve, ptest);

    fullsize = 1 + 31 + 2*size+1 + 2 + base.length() + 1;

    cypher_buff = new uint8_t[fullsize];
    cypher_buff[0] = 0;
    memcpy(cypher_buff+1, encoded2, 31 + 2*size+1);
    uint8_t w, u;
    w = 10;
    u = 4;
    cypher_buff[31+2*size+2] = w;
    cypher_buff[31+2*size+3] = u;
    memcpy(cypher_buff+31+2*size+4, base.c_str(), base.length()+1);
  }

  else if (ec_algo == 1) {
    std::cout << "Using ElGamal with ECC for message encryption\nECQV for certificates\nECDSA signatures" << std::endl;
    std::cout << "Using curve secp256r1 parameters" << std::endl;
    
    CryptoPP::AutoSeededRandomPool prng;    
    GroupParameters group;
    group.Initialize(CryptoPP::ASN1::secp256r1());

    rsuec[0].group = group;

    ECQV cert(group);

    // private key
    CryptoPP::Integer priv_ecc(prng, CryptoPP::Integer::One(), group.GetMaxExponent());

    CryptoPP::Integer capriv("99904945320188894543539641655649253921899278606834393872940151579788317849983");
    
    Element pub = group.ExponentiateBase(priv_ecc);

    int size = group.GetCurve().FieldSize().ByteCount();
    uint8_t *encoded = new uint8_t[31 + 2*size+1];
    cert.cert_generate(encoded, "RSU0001", pub, capriv);
    cert.cert_pk_extraction(encoded, group.ExponentiateBase(capriv));
    cert.cert_reception(encoded, priv_ecc);

    rsuec[0].capub = group.ExponentiateBase(capriv);

    Element rsupub = cert.get_calculated_Qu();
    CryptoPP::Integer rsupriv = cert.get_extracted_du();

    rsuec[0].priv = rsupriv;
    rsuec[0].rsupub = rsupub;
    
    fullsize = 1 + 31 + 2*size + 1;

    cypher_buff = new uint8_t[fullsize];
    cypher_buff[0] = 0;
    memcpy(cypher_buff+1, encoded, 31 + 2*size+1);
  }

  else{
    std::cout << "Using ElGamal with Genus 3 HEC for message encryption\nHECQV for certificates\nElGamal HEC genus 2 signatures" << std::endl;
    ZZ ptest = to_ZZ(pg3);
    field_t::init(ptest);
    std::cout << "Using p: " << ptest << " of size: " << NumBits(ptest) << std::endl;
    g3HEC::g3hcurve curve;

    g3HEC::g3divisor g, h, rsupub, capub;
    ZZ rsupriv;
    UnifiedEncoding enc(ptest, 10, 4, 3);
    rsug3[0].u = 10;
    rsug3[0].w = 4;

    std::string base = "BaseforGenerator";
    int rt = text_to_divisorg3(g, base, ptest, curve, enc);
    if(rt) {
      exit(1);
    }

    ZZ capriv = to_ZZ("247253210584643408262663087671537517974691545498905118366998662050233012073014");
    ZZ x;

    capub = capriv*g;

    divisorg3_to_bytes(rsug3[0].capub, capub, curve, ptest);
    /* private key x */
    RandomBnd(x, ptest*ptest*ptest);

    h = x * g;

    rsug3[0].curve = curve;
    divisorg3_to_bytes(rsug3[0].g, g, curve, ptest);

    g3HECQV cert2(curve, ptest, g);
    int size = NumBytes(ptest);
    uint8_t *encoded2 = new uint8_t[31 + 6*size];
    cert2.cert_generate(encoded2, "RSU0001", h, capriv);

    cert2.cert_pk_extraction(encoded2, capub);
    cert2.cert_reception(encoded2, x);
    
    rsupub = cert2.get_calculated_Qu();
    rsupriv = cert2.get_extracted_du();

    rsug3[0].priv = rsupriv;
    divisorg3_to_bytes(rsug3[0].rsupub, rsupub, curve, ptest);

    fullsize = 1 + 31 + 6*size + 2 + base.length() + 1;

    cypher_buff = new uint8_t[fullsize];
    cypher_buff[0] = 0;
    memcpy(cypher_buff+1, encoded2, 31 + 6*size);
    uint8_t w, u;
    w = 10;
    u = 4;
    cypher_buff[31+6*size+1] = w;
    cypher_buff[31+6*size+2] = u;
    memcpy(cypher_buff+31+6*size+3, base.c_str(), base.length()+1);
  }

  //Number of nodes
  uint32_t nNodes = 64;

  cmd.AddValue ("n","Number of nodes", nNodes);

  cmd.Parse (argc, argv);

  std::string sumo_file = "/home/el18018/sumo/tools/temp1/ns2mobility.tcl";

  ns3::PacketMetadata::Enable ();
  double simTime = 426;
  NodeContainer nodes;
  nodes.Create(nNodes);

  //Nodes MUST have some sort of mobility because that's needed to compute the received signal strength
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (100.0, 250.0, 0.0));
  // positionAlloc->Add (Vector (0.0, 5.0, 0.0));
  // positionAlloc->Add (Vector (0.0, -5.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::CustomMobilityModel");
  mobility.Install (nodes.Get(rsuid));

  Ptr<CustomMobilityModel> m0 = DynamicCast<CustomMobilityModel>(nodes.Get(rsuid)->GetObject<MobilityModel> ());
  // Ptr<CustomMobilityModel> m1 = DynamicCast<CustomMobilityModel>(nodes.Get(1)->GetObject<MobilityModel> ());
  // Ptr<CustomMobilityModel> m2 = DynamicCast<CustomMobilityModel>(nodes.Get(2)->GetObject<MobilityModel> ());
  

  m0->SetVelocityAndAcceleration (Vector (0,0,0), Vector (0,0,0));
  // m1->SetVelocityAndAcceleration (Vector (0,0,0), Vector (3,0,0));


  Ns2MobilityHelper sumo_trace (sumo_file);
  sumo_trace.Install();

  for(uint32_t i=0; i < nNodes-1; i++) {
    Ptr<MobilityModel> mob = nodes.Get(i)->GetObject<MobilityModel>();
    Vector pos = mob->GetPosition();
    pos.x *= 0.5;
    pos.y *= 0.5;
    mob->SetPosition(pos);
  }
  //I prefer using WaveHelper to create WaveNetDevice
  YansWifiChannelHelper waveChannel = YansWifiChannelHelper::Default ();
  YansWavePhyHelper wavePhy =  YansWavePhyHelper::Default ();
  wavePhy.SetChannel (waveChannel.Create ());
  wavePhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  /*
   * If you create applications that control TxPower, define the low & high end of TxPower.
   * This is done by using 'TxInfo' as shown below.
   * 33 dBm is the highest allowed for non-government use (as per 802.11-2016 standard, page 3271
   * 44.8 dBm is for government use.
   *
   * Setting them to the same value is the easy way to go.
   * I can instead set TxPowerStart to a value lower than 33, but then I need to set the number of levels for each PHY
   */
  wavePhy.Set ("TxPowerStart", DoubleValue (8) );
  wavePhy.Set ("TxPowerEnd", DoubleValue (33) );



  QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();
  WaveHelper waveHelper = WaveHelper::Default ();

  waveHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
  						"DataMode", StringValue ("OfdmRate6MbpsBW10MHz"	),
  						"ControlMode",StringValue ("OfdmRate6MbpsBW10MHz"),
  						"NonUnicastMode", StringValue ("OfdmRate6MbpsBW10MHz"));


  NetDeviceContainer devices = waveHelper.Install (wavePhy, waveMac, nodes);
  wavePhy.EnablePcap ("WaveTest", devices);

  //prepare a packet with a payload of 500 Bytes. Basically it has zeros in the payload
  Ptr <Packet> packet 	= Create <Packet> (1000);

  //destination MAC
  Mac48Address dest	= Mac48Address::GetBroadcast();

  /*
   * 0x88dc is the ethertype corresponding to WSMP. IPv4's etherType is 0x0800, and IPv6 is 0x86DD
   * The standard doesn't allow sending IP packets over CCH channel
   */
  uint16_t protocol = 0x88dc;

  //We can also set the transmission parameters at the higher layeres
  TxInfo tx;
  tx.preamble = WIFI_PREAMBLE_LONG;
  //We set the channel on which the packet is sent. The WaveNetDevice must have access to the channel
  //CCH is enabled by default.
  tx.channelNumber = CCH;

  //We can set per-packet data rate. This packet will have a rate of 12Mbps.
  tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");

  /*
   * Set the Access Catogory (AC) of the packet.
   * The 802.11e EDCA standard defines 4 AC's named Voice, Video, Best Effort & Background in order of priority.
   * The value determines the EdcaQueue in which the packet will be enqueued.
   *
   * The 'tid' is a value from 0-7 that maps to ACs as follows
   * 1 or 2 : Background (Lowest priority)
   * 0 or 3 : Best effort
   * 4 or 5 : Video
   * 6 or 7 : Voice (Highest priority)
   */
  tx.priority = 7;	//We set the AC to highest priority. We can set this per packet.

  /*
   * We can also set TxPower. This maps to the user define TxPowerStart & TxPowerEnd.
   * 7 : corresponds to the highest user-defined power (TxPowerEnd). In this code, it's 33 dBm
   * 1 : lowest (TxPowerStart). In this code, it's 8.3 dBm
   *
   * We'll have N equally spaced power levels.
   * A value of 8 indicates application don't want to set power or data rate. Values >8 are invalid.
   */
  tx.txPowerLevel = 7; //When we define TxPowerStart & TxPowerEnd for a WifiPhy, 7 is correspond to TxPowerEnd, and 1 TxPowerStart, and numbers in between are levels.

  /*************** Sending a packet ***************/

  /*
   * In order to send a packet, we will call SendX function of WaveNetDevice.
   */

  //Get the WaveNetDevice for the first devices, using node 0.
  Ptr <NetDevice> d0 = devices.Get (rsuid);
  Ptr <WaveNetDevice> wd0 = DynamicCast <WaveNetDevice> (d0);
  /*
   * We want to call
   *     wd0->SendX (packet, destination, protocol, tx);
   * By scheduling a simulator event as follows:
   */
  
   //Simulator::Schedule ( Seconds (2) , &WaveNetDevice::SendX, wd0, packet_i, dest, protocol, tx);
   Ptr <Packet> packet_j;

   for (uint32_t t=2; t<simTime; t+=5) {
      packet_j = Create<Packet>((uint8_t*)cypher_buff, fullsize);
      Simulator::Schedule ( Seconds (t) , &WaveNetDevice::SendX, wd0, packet_j, dest, protocol, tx);
   }

  /* Using tracesources to trace some simulation events */

  /*
   * Connecting to a promiscous Rx trace source. This will invoke the 'Rx' function everytime a packet is received.
   *
   * The MonitorSnifferRx trace is defined in WifiPhy.
   */

  for(uint32_t i=0; i < nNodes-1; i++) {
    std::string conn = "/NodeList/";
    conn = conn + to_string(i);
    conn = conn + "/DeviceList/*/$ns3::WaveNetDevice/PhyEntities/*/MonitorSnifferRx";
    Config::Connect(conn, MakeCallback(&Rx));
  }
  
  std::string rsuconn = "/NodeList/" + to_string(rsuid) + "/DeviceList/*/$ns3::WaveNetDevice/PhyEntities/*/MonitorSnifferRx";
  Config::Connect(rsuconn, MakeCallback (&Rx1) );
  
  //Set the number of power levels.
  Config::Set("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/PhyEntities/*/TxPowerLevels", ns3::UintegerValue(7));


  /*
   * What if some packets were dropped due to collision, or whatever? We use this trace to fire RxDrop function
   */
  
  Config::Connect("/NodeList/18/DeviceList/*/$ns3::WaveNetDevice/PhyEntities/*/PhyRxDrop", MakeCallback (&RxDrop) );

  /*
   * We can also trace some MAC layer details
   */
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/MacEntities/*/$ns3::OcbWifiMac/*/Queue/Enqueue", MakeCallback (&EnqueueTrace));

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/MacEntities/*/$ns3::OcbWifiMac/*/Queue/Dequeue", MakeCallback (&DequeueTrace));


  Simulator::Schedule (Seconds (4), &PrintInfo);
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();

}
