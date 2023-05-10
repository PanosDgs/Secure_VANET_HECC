#include "messages.h"

using namespace ns3;

uint8_t hpk1[23] = {0x87,0x75,0x6e,0x0e,0x30,0x8e,0x59,0xa4,0x04,0x48,0x01,
0x17,0x4c,0x4f,0x01,0x4d,0x16,0x78,0xe8,0x56,0x6e,0x03,0x02};


void send_Join_g2(int u, int w){
    ZZ ptest = to_ZZ(pt);
    UnifiedEncoding enc(ptest, u, w, 2, ZZ_p::zero());

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    auto str = oss.str();
    std::string str11 = "Join ";
    std::string finalstr = str11 + str;

    NS_G2_NAMESPACE::divisor m, g, h, capub, mypub, a, b, rsupub;
    ZZ x, k, mypriv;
    ZZ capriv = to_ZZ("15669032110011017415376799675649225245106855015484313618141721121181084494176");

    int rt = text_to_divisor(m, finalstr, ptest, veh1g2.curve, enc);
    if(rt) {
      exit(1);
    }

    bytes_to_divisor(g, veh1g2.g, veh1g2.curve, ptest);
    bytes_to_divisor(capub, veh1g2.capub, veh1g2.curve, ptest);

    RandomBnd(x, ptest*ptest);
    h = x * g;
    

    g2HECQV cert2(veh1g2.curve, ptest, g);
    int size = NumBytes(ptest);
    uint8_t *certveh1g2 = new uint8_t[31 + 2*size+1];
    cert2.cert_generate(certveh1g2, "VEH0001", h, capriv);

    cert2.cert_pk_extraction(certveh1g2, capub);
    cert2.cert_reception(certveh1g2, x);

    mypub = cert2.get_calculated_Qu();
    mypriv = cert2.get_extracted_du();

    veh1g2.priv = mypriv;
    divisor_to_bytes(veh1g2.pub, mypub, veh1g2.curve, ptest);
    memcpy(veh1g2.cert, certveh1g2, 31 + 2*size+1);

    bytes_to_divisor(rsupub, veh1g2.rsupub, veh1g2.curve, ptest);

    RandomBnd(k, ptest*ptest);
    a = k*g;
    b = k*rsupub + m;

    int sizenosign = 2*(2*size + 1) + 31 + 2*size+1;
    uint8_t *temp = new uint8_t[sizenosign];
    divisor_to_bytes(temp, a, veh1g2.curve, ptest);
    divisor_to_bytes(temp+2*size+1, b, veh1g2.curve, ptest);
    memcpy(temp+2*(2*size+1), veh1g2.cert, 31 + 2*size+1);

    int signsize = NumBytes(to_ZZ(pg2));
    ZZ sigb;
    uint8_t *siga = new uint8_t[2*signsize+1];

    sign_genus2(siga, sigb, temp, sizenosign, ptest);
    verify_sig2(siga, sigb, temp, sizenosign, hpk1);

    int fullsize = sizenosign + 2*signsize + 1 + 61;
    uint8_t *cypherbuff = new uint8_t[fullsize+2];
    cypherbuff[0] = 0;
    cypherbuff[1] = 1;
    memcpy(cypherbuff+2, temp, sizenosign);
    memcpy(cypherbuff+sizenosign+2, siga, 2*signsize+1);
    BytesFromZZ(cypherbuff+sizenosign+2+2*signsize+1, sigb, 61);

    Ptr<Node> n1 =  ns3::NodeList::GetNode(1);
    Ptr <NetDevice> d0 = n1->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

    Ptr<Node> n0 = ns3::NodeList::GetNode(0);
    Ptr <NetDevice> nd0 = n0->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(cypherbuff, fullsize+2);
    Mac48Address dest	= Mac48Address::ConvertFrom (nd0->GetAddress());
    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 3; //When we define TxPowerStar
    wd0->SendX(packet_i, dest, protocol, tx);
    veh1g2.state = RECEIVE_ACCEPT_KEY;
}

void send_Join_g3(int u, int w) {
    ZZ ptest = to_ZZ(pg3);
    UnifiedEncoding enc(ptest, u, w, 3, ZZ_p::zero());

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    auto str = oss.str();
    std::string str11 = "Join ";
    std::string finalstr = str11 + str;

    g3HEC::g3divisor m, g, h, capub, mypub, a, b, rsupub;
    ZZ x, k, mypriv;
    ZZ capriv = to_ZZ("247253210584643408262663087671537517974691545498905118366998662050233012073014");

    int rt = text_to_divisorg3(m, finalstr, ptest, veh1g3.curve, enc);
    
    if(rt) {
      exit(1);
    }

    bytes_to_divisorg3(g, veh1g3.g, veh1g3.curve, ptest);
    bytes_to_divisorg3(capub, veh1g3.capub, veh1g3.curve, ptest);

    RandomBnd(x, ptest*ptest*ptest);
    h = x * g;
    

    g3HECQV cert2(veh1g3.curve, ptest, g);
    int size = NumBytes(ptest);
    uint8_t *certveh1g3 = new uint8_t[31 + 6*size];
    cert2.cert_generate(certveh1g3, "VEH0001", h, capriv);

    cert2.cert_pk_extraction(certveh1g3, capub);
    cert2.cert_reception(certveh1g3, x);

    mypub = cert2.get_calculated_Qu();
    mypriv = cert2.get_extracted_du();

    veh1g3.priv = mypriv;
    divisorg3_to_bytes(veh1g3.pub, mypub, veh1g3.curve, ptest);
    memcpy(veh1g3.cert, certveh1g3, 31 + 6*size);

    bytes_to_divisorg3(rsupub, veh1g3.rsupub, veh1g3.curve, ptest);

    RandomBnd(k, ptest*ptest*ptest);
    a = k*g;
    b = k*rsupub + m;

    int sizenosign = 2*(6*size) + 31 + 6*size;
    uint8_t *temp = new uint8_t[sizenosign];
    divisorg3_to_bytes(temp, a, veh1g3.curve, ptest);
    divisorg3_to_bytes(temp+6*size, b, veh1g3.curve, ptest);
    memcpy(temp+2*(6*size), veh1g3.cert, 31 + 6*size);

    int sizesign = NumBytes(to_ZZ(pg2));
    ZZ sigb;
    uint8_t *siga = new uint8_t[2*sizesign+1];

    sign_genus2(siga, sigb, temp, sizenosign, ptest);
    verify_sig2(siga, sigb, temp, sizenosign, hpk1);

    int fullsize = sizenosign + 2*sizesign + 1 + 61;
    uint8_t *cypherbuff = new uint8_t[fullsize+2];
    cypherbuff[0] = 0;
    cypherbuff[1] = 1;
    memcpy(cypherbuff+2, temp, sizenosign);
    memcpy(cypherbuff+sizenosign+2, siga, 2*sizesign+1);
    BytesFromZZ(cypherbuff+sizenosign+2+2*sizesign+1, sigb, 61);

    Ptr<Node> n1 =  ns3::NodeList::GetNode(1);
    Ptr <NetDevice> d0 = n1->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

    Ptr<Node> n0 = ns3::NodeList::GetNode(0);
    Ptr <NetDevice> nd0 = n0->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(cypherbuff, fullsize+2);
    Mac48Address dest	= Mac48Address::ConvertFrom (nd0->GetAddress());
    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 3; //When we define TxPowerStar
    wd0->SendX(packet_i, dest, protocol, tx);
    veh1g3.state = RECEIVE_ACCEPT_KEY;
}


void send_Join_ec() {

    CryptoPP::AutoSeededRandomPool prng;  
    int size = veh1ec.group.GetCurve().FieldSize().ByteCount();

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    auto str = oss.str();
    std::string str11 = "Join ";
    std::string finalstr = str11 + str;

    Element m = text_to_ecpoint(finalstr, finalstr.length(), veh1ec.group, size);
    

    CryptoPP::Integer capriv("99904945320188894543539641655649253921899278606834393872940151579788317849983");
    Element capub = veh1ec.capub;

    CryptoPP::Integer x(prng, CryptoPP::Integer::One(), veh1ec.group.GetMaxExponent());
    Element h = veh1ec.group.ExponentiateBase(x);

    ECQV cert(veh1ec.group);
    uint8_t *certec = new uint8_t[31 + 2*size + 1];
    cert.cert_generate(certec, "VEH0001", h, capriv);
    
    cert.cert_pk_extraction(certec, capub);
    int nok = cert.cert_reception(certec, x);

    if(nok) {
      return;
    }

    memcpy(veh1ec.cert, certec, 31 + 2*size+1);

    veh1ec.pub = cert.get_calculated_Qu();
    veh1ec.priv = cert.get_extracted_du();

    CryptoPP::Integer k(prng, CryptoPP::Integer::One(), veh1ec.group.GetMaxExponent());

    Element a,b,btemp;
    a = veh1ec.group.ExponentiateBase(k);
    btemp = veh1ec.group.GetCurve().ScalarMultiply(veh1ec.rsupub, k);
    b = veh1ec.group.GetCurve().Add(btemp, m);

    int sizenosign = 2*(2*size+1) + 31 + 2*size + 1;
    uint8_t *temp = new uint8_t[sizenosign];

    veh1ec.group.GetCurve().EncodePoint(temp, a, false);
    veh1ec.group.GetCurve().EncodePoint(temp+2*size+1, b, false);

    memcpy(temp+2*(2*size+1), certec, 31 + 2*size+1);

    std::string sigecc;
    sign_ec(sigecc, veh1ec.priv, temp, sizenosign);
    verify_ec(sigecc, veh1ec.pub, temp, sizenosign);

    int fullsize = sizenosign + sigecc.length() + 1;
    uint8_t *cypherbuff = new uint8_t[fullsize+2];
    cypherbuff[0] = 0;
    cypherbuff[1] = 1;
    memcpy(cypherbuff+2, temp, sizenosign);
    memcpy(cypherbuff+sizenosign+2, sigecc.c_str(), sigecc.length());

    Ptr<Node> n1 =  ns3::NodeList::GetNode(1);
    Ptr <NetDevice> d0 = n1->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

    Ptr<Node> n0 = ns3::NodeList::GetNode(0);
    Ptr <NetDevice> nd0 = n0->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(cypherbuff, fullsize+2);
    Mac48Address dest	= Mac48Address::ConvertFrom (nd0->GetAddress());
    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 3; //When we define TxPowerStar
    wd0->SendX(packet_i, dest, protocol, tx);
    veh1ec.state = RECEIVE_ACCEPT_KEY;
}

void receive_Cert_Send_Join(uint8_t *buffrc, int ec_algo) {

  if(ec_algo == 0) {
    ZZ ptest = to_ZZ(pt);
    field_t::init(ptest);
    int size = NumBytes(ptest);

    uint8_t *buffcert = new uint8_t[31 + 2*size+1];
    uint8_t w, u;
    

    memcpy(buffcert, buffrc+9, 31 + 2*size+1);
    u = buffrc[9+31+2*size+1];
    w = buffrc[9+31+2*size+2];

    veh1g2.u = u;
    veh1g2.w = w;
       
    char ok[17];
    memcpy(ok, buffrc + 9+31+2*size+3, 16);
    ok[16] = '\0';

    std::string base1(ok);

    UnifiedEncoding enc(ptest, (int)u, (int)w, 2, ZZ_p::zero());

    NS_G2_NAMESPACE::g2hcurve curve;
    
    curve = enc.getcurve();
    veh1g2.curve = curve;
    NS_G2_NAMESPACE::divisor rsupub, g, capub;

    int rt = text_to_divisor(g, base1, ptest, curve, enc);
    if(rt) {
      exit(1);
    }

    divisor_to_bytes(veh1g2.g, g, veh1g2.curve, ptest);

    poly_t uca, vca;
    SetCoeff(uca, 2, 1);
    SetCoeff(uca, 1, to_ZZ_p(to_ZZ("10449495449821289081705384679193221605")));
    SetCoeff(uca, 0, to_ZZ_p(to_ZZ("18201279118423512585508597806469124467")));

    SetCoeff(vca, 1, to_ZZ_p(to_ZZ("25428464405508011187614906973766682181")));
    SetCoeff(vca, 0, to_ZZ_p(to_ZZ("332059286561908222198963453047829588368")));

    capub.set_curve(curve);
    capub.set_upoly(uca);
    capub.set_vpoly(vca);
    capub.update();

    divisor_to_bytes(veh1g2.capub, capub, veh1g2.curve, ptest);

    g2HECQV cert2(curve, ptest, g);
    cert2.cert_pk_extraction(buffcert, capub);
    rsupub = cert2.get_calculated_Qu();
    std::cout << "Received RSU public key!" << std::endl;
    divisor_to_bytes(veh1g2.rsupub, rsupub, veh1g2.curve, ptest);
    send_Join_g2(u, w);
  }

  else if(ec_algo == 1) {
    CryptoPP::AutoSeededRandomPool prng;    
    GroupParameters group;
    group.Initialize(CryptoPP::ASN1::secp256r1());

    veh1ec.group = group;

    int size = group.GetCurve().FieldSize().ByteCount();
    ECQV cert(group);
    uint8_t *buffcert = new uint8_t[31 + 2*size+1];
    memcpy(buffcert, buffrc+9, 31 + 2*size+1);

    CryptoPP::Integer cax("78988430312110551112312640562478615015063904707272294092537225346022844037941");
    CryptoPP::Integer cay("58350698637878652075905464639281560400171480414848423595036745311979378541663");
    Element capub(cax, cay);

    veh1ec.capub = capub;

    cert.cert_pk_extraction(buffcert, capub);
    Element rsupub = cert.get_calculated_Qu();
    std::cout << "Received RSU public key!" << std::endl;
    veh1ec.rsupub = rsupub;
    send_Join_ec();
  }

  else {
    ZZ ptest = to_ZZ(pg3);
    field_t::init(ptest);
    int size = NumBytes(ptest);

    uint8_t *buffcert = new uint8_t[31 + 6*size];
    uint8_t w, u;
    


    memcpy(buffcert, buffrc+9, 31 + 6*size);
    u = buffrc[9+31+6*size];
    w = buffrc[9+31+6*size+1];

    veh1g3.u = u;
    veh1g3.w = w;

    char ok[17];
    memcpy(ok, buffrc + 9+31+6*size+2, 16);
    ok[16] = '\0';

    std::string base1(ok);

    UnifiedEncoding enc(ptest, (int)u, (int)w, 3, ZZ_p::zero());

    g3HEC::g3hcurve curve;
    
    
    curve = enc.getcurveg3();
    veh1g3.curve = curve;
    g3HEC::g3divisor rsupub, g, capub;

    int rt = text_to_divisorg3(g, base1, ptest, curve, enc);
    if(rt) {
      exit(1);
    }

    divisorg3_to_bytes(veh1g3.g, g, veh1g3.curve, ptest);

    poly_t uca, vca;
    SetCoeff(uca, 3, 1);
    SetCoeff(uca, 2, to_ZZ_p(to_ZZ("55629943785401490078502501")));
    SetCoeff(uca, 1, to_ZZ_p(to_ZZ("35473227905585386587465512")));
    SetCoeff(uca, 0, to_ZZ_p(to_ZZ("77188891722769218853049502")));

    SetCoeff(vca, 2, to_ZZ_p(to_ZZ("70859488533841223131993434")));
    SetCoeff(vca, 1, to_ZZ_p(to_ZZ("66197154102527372970808728")));
    SetCoeff(vca, 0, to_ZZ_p(to_ZZ("64357326462217845080479858")));

    capub.set_curve(curve);
    capub.set_upoly(uca);
    capub.set_vpoly(vca);
    capub.update();

    divisorg3_to_bytes(veh1g3.capub, capub, veh1g3.curve, ptest);

    g3HECQV cert2(curve, ptest, g);
    cert2.cert_pk_extraction(buffcert, capub);
    rsupub = cert2.get_calculated_Qu();
    std::cout << "Received RSU public key!" << std::endl;
    divisorg3_to_bytes(veh1g3.rsupub, rsupub, veh1g3.curve, ptest);

    send_Join_g3((int)u, (int)w);
    
  }
}

void extract_RSU_SendAccept_g2(uint8_t *buffrc, int vid) {
    ZZ ptest = to_ZZ(pt);
    int size = NumBytes(ptest);
    UnifiedEncoding enc(ptest, 10, 4, 2, ZZ_p::zero());

    int signsize = NumBytes(to_ZZ(pg2));
    int sizenosign = 2*(2*size + 1) + 31 + 2*size+1;

    // int fullsize = sizenosign + 2*signsize + 1 + signsize;

    uint8_t *siga = new uint8_t[2*signsize+1];
    ZZ sigb;

    memcpy(siga, buffrc+sizenosign, 2*signsize+1);
    sigb = -ZZFromBytes(buffrc+sizenosign+2*signsize+1, 61);
    int nok = verify_sig2(siga, sigb, buffrc, sizenosign, hpk1);

    if(nok) {
      sigb = -sigb;
      nok = verify_sig2(siga, sigb, buffrc, sizenosign, hpk1);
      if(nok)
        return;
    }

    NS_G2_NAMESPACE::divisor a, b, m, x;
    
    nok = bytes_to_divisor(a, buffrc, rsu1g2.curve, ptest);
    nok = bytes_to_divisor(b, buffrc+2*size+1, rsu1g2.curve, ptest);
    
    if(nok)
      return;

    m = b - rsu1g2.priv*a;
    std::string rec;
    divisor_to_text(rec, m, ptest, enc);
    std::string tocmp = "Join";

    if(memcmp(rec.c_str(), tocmp.c_str(), 4) != 0) {
      return;
    }
    std::cout << "Received Join from vehicle: " << vid << std::endl;
    rsu1g2.numveh++;

    std::string tmstmp = rec.substr(5);
    nok = validate_timestamp(tmstmp);
    if(nok) {
      std::cout << "Message not fresh" << std::endl;
      return;
    }
    else {
      std::cout << "Timestamp is valid." << std::endl;
    }

    NS_G2_NAMESPACE::divisor g, capub, vehpk;
    bytes_to_divisor(g, rsu1g2.g, rsu1g2.curve, ptest);
    bytes_to_divisor(capub, rsu1g2.capub, rsu1g2.curve, ptest);

    g2HECQV recert(rsu1g2.curve, ptest, g);
    uint8_t *received_cert = new uint8_t[31 + 2*size+1];
    memcpy(received_cert, buffrc+4*size+2, 31 + 2*size+1);

    recert.cert_pk_extraction(received_cert, capub);
    vehpk = recert.get_calculated_Qu();
    if(!vehpk.is_valid_divisor()) {
      return;
    }

    using namespace CryptoPP;


    AutoSeededRandomPool prng;

    SecByteBlock key(AES::DEFAULT_KEYLENGTH);
    SecByteBlock iv(AES::BLOCKSIZE);

    prng.GenerateBlock(key, key.size());
    prng.GenerateBlock(iv, iv.size());

    std::string keystr, ivstr;
    HexEncoder encoder(new StringSink(keystr));
    encoder.Put(key, key.size());
    encoder.MessageEnd();

    HexEncoder encoder2(new StringSink(ivstr));
    encoder2.Put(iv, iv.size());
    encoder2.MessageEnd();
 
    std::string str1 = "Accept ";
    str1 += keystr.substr(0, 16);

    NS_G2_NAMESPACE::divisor mess1, a1, b1;
    
    text_to_divisor(mess1, str1, ptest, rsu1g2.curve, enc);
    
    ZZ k;
    RandomBnd(k, ptest*ptest);
    a1 = k*g;
    b1 = k*vehpk + mess1;


    
    std::string str2 = "Accept ";
    str2 += keystr.substr(16);


    NS_G2_NAMESPACE::divisor mess2, a2, b2;
    
    text_to_divisor(mess2, str2, ptest, rsu1g2.curve, enc);
    
    a2 = k*g;
    b2 = k*vehpk + mess2;



    std::string str3 = "Accept ";
    str3 += ivstr.substr(0, 16);


    NS_G2_NAMESPACE::divisor mess3, a3, b3;
    
    text_to_divisor(mess3, str3, ptest, rsu1g2.curve, enc);
    
    a3 = k*g;
    b3 = k*vehpk + mess3;



    std::string str4 = "Accept ";
    str4 += ivstr.substr(16);

    NS_G2_NAMESPACE::divisor mess4, a4, b4;
    
    text_to_divisor(mess4, str4, ptest, rsu1g2.curve, enc);
    
    a4 = k*g;
    b4 = k*vehpk + mess4;


    int onedivsize = 2*size+1;
    int size1no = 8*onedivsize;
    int fullsize1 = size1no + 2*signsize + 62;

    uint8_t cypherbuff[fullsize1+1];
    uint8_t temp[size1no];

    divisor_to_bytes(temp, a1, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+onedivsize, b1, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+2*onedivsize, a2, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+3*onedivsize, b2, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+4*onedivsize, a3, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+5*onedivsize, b3, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+6*onedivsize, a4, rsu1g2.curve, ptest);
    divisor_to_bytes(temp+7*onedivsize, b4, rsu1g2.curve, ptest);

    uint8_t mysiga[2*signsize+1];
    ZZ mysigb;
    sign_genus2(mysiga, mysigb, temp, size1no, ptest);
    verify_sig2(mysiga, mysigb, temp, size1no, hpk1);

    cypherbuff[0] = 1;
    memcpy(cypherbuff+1, temp, size1no);
    memcpy(cypherbuff+size1no+1, mysiga, 2*signsize+1);
    BytesFromZZ(cypherbuff+size1no+2*signsize+2, mysigb, 61);

    Ptr<Node> n0 =  ns3::NodeList::GetNode(0);
    Ptr <NetDevice> d0 = n0->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

    Ptr<Node> n1 = ns3::NodeList::GetNode(vid);
    Ptr <NetDevice> nd0 = n1->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(cypherbuff, fullsize1+1);
    Mac48Address dest	= Mac48Address::ConvertFrom (nd0->GetAddress());
    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 3; //When we define TxPowerStar
    wd0->SendX(packet_i, dest, protocol, tx);
    rsu1g2.symm_perveh[vid-1] = keystr;
    rsu1g2.iv_perveh[vid-1] = ivstr;
    rsu1g2.states[vid-1] = RECEIVE_ACCEPT_KEY;

}

void extract_RSU_SendAccept_g3(uint8_t *buffrc, int vid) {
    ZZ ptest = to_ZZ(pg3);
    int size = NumBytes(ptest);
    UnifiedEncoding enc(ptest, 10, 4, 3, ZZ_p::zero());

    int signsize = NumBytes(to_ZZ(pg2));
    int sizenosign = 12*size + 31 + 6*size;

    // int fullsize = sizenosign + 2*signsize + 1 + signsize;

    uint8_t *siga = new uint8_t[2*signsize+1];
    ZZ sigb;

    memcpy(siga, buffrc+sizenosign, 2*signsize+1);
    sigb = -ZZFromBytes(buffrc+sizenosign+2*signsize+1, 61);
    int nok = verify_sig2(siga, sigb, buffrc, sizenosign, hpk1);

    if(nok) {
      sigb = -sigb;
      nok = verify_sig2(siga, sigb, buffrc, sizenosign, hpk1);
      if(nok)
        return;
    }

    g3HEC::g3divisor a, b, m, x;
    
    nok = bytes_to_divisorg3(a, buffrc, rsu1g3.curve, ptest);
    nok = bytes_to_divisorg3(b, buffrc+6*size, rsu1g3.curve, ptest);
    
    if(nok)
      return;

    m = b - rsu1g3.priv*a;
    std::string rec;
    divisorg3_to_text(rec, m, ptest, enc);
    std::string tocmp = "Join";

    if(memcmp(rec.c_str(), tocmp.c_str(), 4) != 0) {
      return;
    }
    std::cout << "Received Join from vehicle: " << vid << std::endl;
    rsu1g2.numveh++;

    std::string tmstmp = rec.substr(5);
    nok = validate_timestamp(tmstmp);
    if(nok) {
      std::cout << "Message not fresh" << std::endl;
      return;
    }
    else {
      std::cout << "Timestamp is valid." << std::endl;
    }

    g3HEC::g3divisor g, capub, vehpk;
    bytes_to_divisorg3(g, rsu1g3.g, rsu1g3.curve, ptest);
    bytes_to_divisorg3(capub, rsu1g3.capub, rsu1g3.curve, ptest);

    g3HECQV recert(rsu1g3.curve, ptest, g);
    uint8_t *received_cert = new uint8_t[31 + 6*size];
    memcpy(received_cert, buffrc+12*size, 31 + 6*size);

    recert.cert_pk_extraction(received_cert, capub);
    vehpk = recert.get_calculated_Qu();
    if(!vehpk.is_valid_divisor()) {
      return;
    }

    using namespace CryptoPP;


    AutoSeededRandomPool prng;

    SecByteBlock key(AES::DEFAULT_KEYLENGTH);
    SecByteBlock iv(AES::BLOCKSIZE);

    prng.GenerateBlock(key, key.size());
    prng.GenerateBlock(iv, iv.size());

    std::string keystr, ivstr;
    HexEncoder encoder(new StringSink(keystr));
    encoder.Put(key, key.size());
    encoder.MessageEnd();

    HexEncoder encoder2(new StringSink(ivstr));
    encoder2.Put(iv, iv.size());
    encoder2.MessageEnd();
 
    std::string str1 = "Accept ";
    str1 += keystr.substr(0, 16);

    g3HEC::g3divisor mess1, a1, b1;
    
    text_to_divisorg3(mess1, str1, ptest, rsu1g3.curve, enc);
    
    ZZ k;
    RandomBnd(k, ptest*ptest*ptest);
    a1 = k*g;
    b1 = k*vehpk + mess1;


    
    std::string str2 = "Accept ";
    str2 += keystr.substr(16);


    g3HEC::g3divisor mess2, a2, b2;
    
    text_to_divisorg3(mess2, str2, ptest, rsu1g3.curve, enc);
    
    a2 = k*g;
    b2 = k*vehpk + mess2;



    std::string str3 = "Accept ";
    str3 += ivstr.substr(0, 16);


    g3HEC::g3divisor mess3, a3, b3;
    
    text_to_divisorg3(mess3, str3, ptest, rsu1g3.curve, enc);
    
    a3 = k*g;
    b3 = k*vehpk + mess3;



    std::string str4 = "Accept ";
    str4 += ivstr.substr(16);

    g3HEC::g3divisor mess4, a4, b4;
    
    text_to_divisorg3(mess4, str4, ptest, rsu1g3.curve, enc);
    
    a4 = k*g;
    b4 = k*vehpk + mess4;


    int onedivsize = 6*size;
    int size1no = 8*onedivsize;
    int fullsize1 = size1no + 2*signsize + 62;

    uint8_t cypherbuff[fullsize1+1];
    uint8_t temp[size1no];

    divisorg3_to_bytes(temp, a1, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+onedivsize, b1, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+2*onedivsize, a2, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+3*onedivsize, b2, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+4*onedivsize, a3, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+5*onedivsize, b3, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+6*onedivsize, a4, rsu1g3.curve, ptest);
    divisorg3_to_bytes(temp+7*onedivsize, b4, rsu1g3.curve, ptest);

    uint8_t mysiga[2*signsize+1];
    ZZ mysigb;
    sign_genus2(mysiga, mysigb, temp, size1no, ptest);
    verify_sig2(mysiga, mysigb, temp, size1no, hpk1);

    cypherbuff[0] = 1;
    memcpy(cypherbuff+1, temp, size1no);
    memcpy(cypherbuff+size1no+1, mysiga, 2*signsize+1);
    BytesFromZZ(cypherbuff+size1no+2*signsize+2, mysigb, 61);

    Ptr<Node> n0 =  ns3::NodeList::GetNode(0);
    Ptr <NetDevice> d0 = n0->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

    Ptr<Node> n1 = ns3::NodeList::GetNode(vid);
    Ptr <NetDevice> nd0 = n1->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(cypherbuff, fullsize1+1);
    Mac48Address dest	= Mac48Address::ConvertFrom (nd0->GetAddress());
    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 3; //When we define TxPowerStar
    wd0->SendX(packet_i, dest, protocol, tx);
    rsu1g3.symm_perveh[vid-1] = keystr;
    rsu1g3.iv_perveh[vid-1] = ivstr;
    rsu1g3.states[vid-1] = RECEIVE_ACCEPT_KEY;
}

void extract_RSU_SendAccept_ec(uint8_t *buffrc, int vid) {
    GroupParameters group = rsu1ec.group;
    int size = group.GetCurve().FieldSize().ByteCount();

    int sizenosign = 2*(2*size+1) + 31 + 2*size + 1;
    
    std::string sigecc = (char*)buffrc + sizenosign;
}




void extract_Symmetric(uint8_t *buffrc, int ec_algo) {
  if(ec_algo == 0) {
    ZZ ptest = to_ZZ(pt);
    int size = NumBytes(ptest);
    UnifiedEncoding enc(ptest, veh1g2.u, veh1g2.w, 2, ZZ_p::zero());
    NS_G2_NAMESPACE::divisor a1, b1, a2, b2, a3, b3, a4, b4, mess1, mess2, mess3, mess4;
    int divsize = 2*size+1;


    int signsize = NumBytes(to_ZZ(pg2));
    uint8_t mysiga[2*signsize+1];
    ZZ mysigb;
    memcpy(mysiga, buffrc+8*divsize, 2*signsize+1);
    mysigb = -ZZFromBytes(buffrc+8*divsize+2*signsize+1, 61);
    int nok = verify_sig2(mysiga, mysigb, buffrc, 8*divsize, hpk1);
    if(nok) {
      return;
    }

    bytes_to_divisor(a1, buffrc, veh1g2.curve, ptest);
    bytes_to_divisor(b1, buffrc+divsize, veh1g2.curve, ptest);
    
    mess1 = b1 - veh1g2.priv*a1;
    std::string rec1;
    divisor_to_text(rec1, mess1, ptest, enc);


    bytes_to_divisor(a2, buffrc+2*divsize, veh1g2.curve, ptest);
    bytes_to_divisor(b2, buffrc+3*divsize, veh1g2.curve, ptest);
    
    mess2 = b2 - veh1g2.priv*a2;
    std::string rec2;
    divisor_to_text(rec2, mess2, ptest, enc);


    bytes_to_divisor(a3, buffrc+4*divsize, veh1g2.curve, ptest);
    bytes_to_divisor(b3, buffrc+5*divsize, veh1g2.curve, ptest);
    
    mess3 = b3 - veh1g2.priv*a3;
    std::string rec3;
    divisor_to_text(rec3, mess3, ptest, enc);


    bytes_to_divisor(a4, buffrc+6*divsize, veh1g2.curve, ptest);
    bytes_to_divisor(b4, buffrc+7*divsize, veh1g2.curve, ptest);
    
    mess4 = b4 - veh1g2.priv*a4;
    std::string rec4;
    divisor_to_text(rec4, mess4, ptest, enc);

    std::string key, iv;
    key = rec1.substr(7);
    key += rec2.substr(7);
    iv = rec3.substr(7);
    iv += rec4.substr(7);

    std::cout << "Received symmetric key from RSU" << std::endl;
    veh1g2.symm = key;
    veh1g2.iv = iv;
  }
  else if (ec_algo == 1) {

  }
  else {
    ZZ ptest = to_ZZ(pg3);
    int size = NumBytes(ptest);
    UnifiedEncoding enc(ptest, veh1g3.u, veh1g3.w, 3, ZZ_p::zero());
    g3HEC::g3divisor a1, b1, a2, b2, a3, b3, a4, b4, mess1, mess2, mess3, mess4;
    int divsize = 6*size;


    int signsize = NumBytes(to_ZZ(pg2));
    uint8_t mysiga[2*signsize+1];
    ZZ mysigb;
    memcpy(mysiga, buffrc+8*divsize, 2*signsize+1);
    mysigb = -ZZFromBytes(buffrc+8*divsize+2*signsize+1, 61);
    int nok = verify_sig2(mysiga, mysigb, buffrc, 8*divsize, hpk1);
    if(nok) {
      return;
    }

    bytes_to_divisorg3(a1, buffrc, veh1g3.curve, ptest);
    bytes_to_divisorg3(b1, buffrc+divsize, veh1g3.curve, ptest);
    
    mess1 = b1 - veh1g3.priv*a1;
    std::string rec1;
    divisorg3_to_text(rec1, mess1, ptest, enc);


    bytes_to_divisorg3(a2, buffrc+2*divsize, veh1g3.curve, ptest);
    bytes_to_divisorg3(b2, buffrc+3*divsize, veh1g3.curve, ptest);
    
    mess2 = b2 - veh1g3.priv*a2;
    std::string rec2;
    divisorg3_to_text(rec2, mess2, ptest, enc);


    bytes_to_divisorg3(a3, buffrc+4*divsize, veh1g3.curve, ptest);
    bytes_to_divisorg3(b3, buffrc+5*divsize, veh1g3.curve, ptest);
    
    mess3 = b3 - veh1g3.priv*a3;
    std::string rec3;
    divisorg3_to_text(rec3, mess3, ptest, enc);


    bytes_to_divisorg3(a4, buffrc+6*divsize, veh1g3.curve, ptest);
    bytes_to_divisorg3(b4, buffrc+7*divsize, veh1g3.curve, ptest);
    
    mess4 = b4 - veh1g3.priv*a4;
    std::string rec4;
    divisorg3_to_text(rec4, mess4, ptest, enc);

    std::string key, iv;
    key = rec1.substr(7);
    key += rec2.substr(7);
    iv = rec3.substr(7);
    iv += rec4.substr(7);

    std::cout << "Received symmetric key from RSU" << std::endl;
    veh1g3.symm = key;
    veh1g3.iv = iv;
  }
}