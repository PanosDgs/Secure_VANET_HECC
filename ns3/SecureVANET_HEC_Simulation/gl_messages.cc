#include "messages.h"

bool dry[100];
bool already_here = false;
using namespace ns3;

void encrypt_message_AES (uint8_t *out, uint8_t *in, int size, std::string keystr, std::string ivstr) {
    using namespace CryptoPP;
    byte key[16], iv[16]; 
    CryptoPP::StringSource(keystr, true,
        new CryptoPP::HexDecoder(
            new CryptoPP::ArraySink(key, CryptoPP::AES::DEFAULT_KEYLENGTH)
        )
    );

    CryptoPP::StringSource(ivstr, true,
        new CryptoPP::HexDecoder(
            new CryptoPP::ArraySink(iv, CryptoPP::AES::DEFAULT_KEYLENGTH)
        )
    );

    CBC_Mode<AES>::Encryption e;
    e.SetKeyWithIV(key, 16, iv);

    StreamTransformationFilter encfilter(e, nullptr, BlockPaddingSchemeDef::PKCS_PADDING);
    encfilter.Put(in, size);
    encfilter.MessageEnd();
    encfilter.Get(out, size+16-size%16);
}

void decrypt_message_AES (uint8_t *out, uint8_t *in, int size, std::string keystr, std::string ivstr) {
    using namespace CryptoPP;
    byte key[16], iv[16]; 
    CryptoPP::StringSource(keystr, true,
        new CryptoPP::HexDecoder(
            new CryptoPP::ArraySink(key, CryptoPP::AES::DEFAULT_KEYLENGTH)
        )
    );

    CryptoPP::StringSource(ivstr, true,
        new CryptoPP::HexDecoder(
            new CryptoPP::ArraySink(iv, CryptoPP::AES::DEFAULT_KEYLENGTH)
        )
    );

    try {
        CBC_Mode<AES>::Decryption d;
        d.SetKeyWithIV(key, 16, iv);
        
        StreamTransformationFilter decfilter(d, nullptr, BlockPaddingSchemeDef::PKCS_PADDING);
        decfilter.Put(in, size);
        decfilter.MessageEnd();
        decfilter.Get(out, size);
    }
    catch(const Exception& ex) {
        std::cerr << "Decryption error: " << ex.what() << std::endl;
    }
}

void RSU_inform_GL(int ec_algo, int vid) {
    switch (ec_algo) {
        case 0:
        {
            ZZ ptest = to_ZZ(pt);
            field_t::init(ptest);
            int size = NumBytes(ptest);
            
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            
            std::ostringstream oss;
            oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
            auto str = oss.str();
            std::string str11 = "Leader ";
            std::string finalstr = str11 + str;


            int signsize = NumBytes(to_ZZ(pg2));
            int sizenosign = finalstr.length()+1 + 31 + 2*size+1;

            int sizemod16 = sizenosign + 16 - sizenosign%16;
            int fullsize = sizemod16 + 2*signsize + 22;

            uint8_t sendbuff[fullsize+2];

            uint8_t temp[sizenosign];
            memcpy(temp, finalstr.c_str(), finalstr.length());
            temp[finalstr.length()] = '\0';
            memcpy(temp + finalstr.length()+1, rsug2[0].certs[vid], 31+2*size+1);


            /* Encrypt using symmetric key: */
            uint8_t cypher[sizemod16];

            encrypt_message_AES(cypher, temp, sizenosign, rsug2[0].symm_perveh[vid], rsug2[0].iv_perveh[vid]);
            

            ZZ sigb;
            uint8_t *siga = new uint8_t[2*signsize+1];

            auto start = chrono::high_resolution_clock::now();
            sign_genus2(siga, sigb, temp, sizenosign, ptest);

            if(get_metrics != 0) {
                auto stop = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
            
                cout << "Signature generation: "
                    << duration.count() << " microseconds" << endl;
            }

            int nok = verify_sig2(siga, sigb, temp, sizenosign, hpk);
            
            if(nok)
                return;

            sendbuff[0] = GROUP_LEADER_INFORM;
            sendbuff[1] = vid;
            memcpy(sendbuff+2, cypher, sizemod16);
            memcpy(sendbuff+sizemod16+2, siga, 2*signsize+1);
            BytesFromZZ(sendbuff+sizemod16+2+2*signsize+1, sigb, 21);

            if(get_metrics != 0)
                std::cout << "RSU_INFORM_LEADER message size: " << fullsize+2 << std::endl;

            Ptr<Node> n1 =  ns3::NodeList::GetNode(rsuid);
            Ptr <NetDevice> d0 = n1->GetDevice(0);
            Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

            Ptr<Node> n0 = ns3::NodeList::GetNode(vid);
            Ptr <NetDevice> nd0 = n0->GetDevice(0);

            Ptr <Packet> packet_i = Create<Packet>(sendbuff, fullsize+2);
            Mac48Address dest = Mac48Address::ConvertFrom (nd0->GetAddress());

            uint16_t protocol = 0x88dc;
            TxInfo tx;
            tx.preamble = WIFI_PREAMBLE_LONG;
            tx.channelNumber = CCH;
            tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
            tx.priority = 7;	//We set the AC to highest prior
            tx.txPowerLevel = 7; //When we define TxPowerStar
            wd0->SendX(packet_i, dest, protocol, tx);
            rsug2[0].glid = vid;
            break;
        }
        case 1: {
            CryptoPP::AutoSeededRandomPool prng;  
            int size = rsuec[0].group.GetCurve().FieldSize().ByteCount();
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            
            std::ostringstream oss;
            oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
            auto str = oss.str();
            std::string str11 = "Leader ";
            std::string finalstr = str11 + str;

            int sizenosign = finalstr.length()+1 + 31 + size+1;
            int sizemod16 = sizenosign + 16 - sizenosign%16;

            uint8_t temp[sizenosign];
            memcpy(temp, finalstr.c_str(), finalstr.length());
            temp[finalstr.length()] = '\0';
            memcpy(temp + finalstr.length()+1, rsuec[0].certs[vid], 31+size+1);

            uint8_t cypher[sizemod16];
            encrypt_message_AES(cypher, temp, sizenosign, rsuec[0].symm_perveh[vid], rsuec[0].iv_perveh[vid]);
            
            std::string sigecc;

            auto start = chrono::high_resolution_clock::now();
            sign_ec(sigecc, rsuec[0].priv, temp, sizenosign);

            if(get_metrics != 0) {
                auto stop = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
            
                cout << "Signature generation: "
                    << duration.count() << " microseconds" << endl;
            }

            int nok = verify_ec(sigecc, rsuec[0].rsupub, temp, sizenosign);

            if (nok)
                return;
            
            int fullsize = sizemod16 + sigecc.length() +1;
            uint8_t sendbuff[fullsize+2];
            sendbuff[0] = GROUP_LEADER_INFORM;
            sendbuff[1] = vid;
            memcpy(sendbuff+2, cypher, sizemod16);
            memcpy(sendbuff+sizemod16+2, sigecc.c_str(), sigecc.length());
            sendbuff[fullsize+1] = 0;

            if(get_metrics != 0)
                std::cout << "RSU_INFORM_LEADER message size: " << fullsize+2 << std::endl;

            Ptr<Node> n1 =  ns3::NodeList::GetNode(rsuid);
            Ptr <NetDevice> d0 = n1->GetDevice(0);
            Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

            Ptr<Node> n0 = ns3::NodeList::GetNode(vid);
            Ptr <NetDevice> nd0 = n0->GetDevice(0);

            Ptr <Packet> packet_i = Create<Packet>(sendbuff, fullsize+2);
            Mac48Address dest = Mac48Address::ConvertFrom (nd0->GetAddress());

            uint16_t protocol = 0x88dc;
            TxInfo tx;
            tx.preamble = WIFI_PREAMBLE_LONG;
            tx.channelNumber = CCH;
            tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
            tx.priority = 7;	//We set the AC to highest prior
            tx.txPowerLevel = 7; //When we define TxPowerStar
            wd0->SendX(packet_i, dest, protocol, tx);
            rsuec[0].glid = vid;

            break;
        }
        case 2: {
            ZZ ptest = to_ZZ(pg3);
            field_t::init(ptest);
            int size = NumBytes(ptest);
            
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            
            std::ostringstream oss;
            oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
            auto str = oss.str();
            std::string str11 = "Leader ";
            std::string finalstr = str11 + str;


            int signsize = NumBytes(to_ZZ(psign3));
            int sizenosign = finalstr.length()+1 + 31 + 6*size;

            int sizemod16 = sizenosign + 16 - sizenosign%16;
            int fullsize = sizemod16 + 6*signsize + 21;

            uint8_t sendbuff[fullsize+2];

            uint8_t temp[sizenosign];
            memcpy(temp, finalstr.c_str(), finalstr.length());
            temp[finalstr.length()] = '\0';
            memcpy(temp + finalstr.length()+1, rsug3[0].certs[vid], 31+6*size);


            /* Encrypt using symmetric key: */
            uint8_t cypher[sizemod16];

            encrypt_message_AES(cypher, temp, sizenosign, rsug3[0].symm_perveh[vid], rsug3[0].iv_perveh[vid]);
            

            ZZ sigb;
            uint8_t *siga = new uint8_t[6*signsize];

            auto start = chrono::high_resolution_clock::now();
            sign_genus3(siga, sigb, temp, sizenosign, ptest);

            if(get_metrics != 0) {
                auto stop = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
            
                cout << "Signature generation: "
                    << duration.count() << " microseconds" << endl;
            }

            int nok = verify_sig3(siga, sigb, temp, sizenosign, hpk3);
            
            if(nok)
                return;

            sendbuff[0] = GROUP_LEADER_INFORM;
            sendbuff[1] = vid;
            memcpy(sendbuff+2, cypher, sizemod16);
            memcpy(sendbuff+sizemod16+2, siga, 6*signsize);
            BytesFromZZ(sendbuff+sizemod16+2+6*signsize, sigb, 21);

            if(get_metrics != 0)
                std::cout << "RSU_INFORM_LEADER message size: " << fullsize+2 << std::endl;

            Ptr<Node> n1 =  ns3::NodeList::GetNode(rsuid);
            Ptr <NetDevice> d0 = n1->GetDevice(0);
            Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

            Ptr<Node> n0 = ns3::NodeList::GetNode(vid);
            Ptr <NetDevice> nd0 = n0->GetDevice(0);

            Ptr <Packet> packet_i = Create<Packet>(sendbuff, fullsize+2);
            Mac48Address dest = Mac48Address::ConvertFrom (nd0->GetAddress());

            uint16_t protocol = 0x88dc;
            TxInfo tx;
            tx.preamble = WIFI_PREAMBLE_LONG;
            tx.channelNumber = CCH;
            tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
            tx.priority = 7;	//We set the AC to highest prior
            tx.txPowerLevel = 7; //When we define TxPowerStar
            wd0->SendX(packet_i, dest, protocol, tx);
            rsug3[0].glid = vid;

            break;
        }
        
        default:
            break;
    }
}



void extract_GLProof_Broadcast(uint8_t *buffrc, int ec_algo, int vid) {
    switch (ec_algo)
    {
    case 0:
    {
        Vehicle_data_g2 *veh1g2 = &vehg2[vid];
        ZZ ptest = to_ZZ(pt);
        field_t::init(ptest);
        int size = NumBytes(ptest);
        
        int sizenosign = 27 + 31 + 2*size + 1;
        int sizemod16 = sizenosign + 16 - sizenosign%16;
        uint8_t decrypted[sizemod16];
        
        decrypt_message_AES(decrypted, buffrc, sizemod16, veh1g2->symm, veh1g2->iv);
        
        std::string lead((char*)decrypted, 27);
        std::string tocmp = "Leader";
        if(memcmp(lead.c_str(), tocmp.c_str(), 6) != 0) {
            return;
        }
        else
            std::cout << BOLD_CODE << GREEN_CODE << "Node " << vid << " is selected as GL!" << END_CODE << std::endl;

        int nok = validate_timestamp(lead.substr(7, 27));
        if(nok) {
            std::cout << RED_CODE << "Invalid Timestamp." << END_CODE << std::endl;
            return;
        }
        else 
            std::cout << BOLD_CODE << GREEN_CODE << "Timestamp is valid." << END_CODE << std::endl;

        NS_G2_NAMESPACE::divisor g, capub, mypub;
        bytes_to_divisor(g, veh1g2->g, veh1g2->curve, ptest);
        bytes_to_divisor(capub, veh1g2->capub, veh1g2->curve, ptest);
        bytes_to_divisor(mypub, veh1g2->pub, veh1g2->curve, ptest);
        g2HECQV cert2(veh1g2->curve, ptest, g);

        vector<unsigned char> rec_cert;
        rec_cert.insert(rec_cert.end(), decrypted+27, decrypted + 27 + 31 + 2*size + 1);

        auto start = chrono::high_resolution_clock::now();
        cert2.cert_pk_extraction(rec_cert);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Certificate public key extraction: "
                << duration.count() << " microseconds" << endl;
        }

        if(mypub != cert2.get_calculated_Qu()) {
            std::cout << BOLD_CODE << RED_CODE << "Certificate provided to GL is incorrect." << END_CODE << std::endl;
            return;
        }

        int signsize = NumBytes(to_ZZ(pg2));
        uint8_t *siga = new uint8_t[2*signsize+1];
        ZZ sigb;

        memcpy(siga, buffrc+sizemod16, 2*signsize+1);
        sigb = ZZFromBytes(buffrc+sizemod16+2*signsize+1, 21);

        start = chrono::high_resolution_clock::now();
        nok = verify_sig2(siga, sigb, decrypted, sizenosign, hpk);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            sigb = -sigb;
            nok = verify_sig2(siga, sigb, decrypted, sizenosign, hpk);
            if(nok)
                return;
        }

        gl2.myid = vid;
        gl2.mydata = veh1g2;
        gl2.mydata->state = IS_GROUP_LEADER;
        std::cout << BOLD_CODE << YELLOW_CODE << "Group Leader " << vid << " broadcasting proof of leadership for new vehicles..." << END_CODE << std::endl << std::endl;

        int sendsize1 = 2 + sizenosign + 2*signsize + 22;
        int finalsendsize = sendsize1;
        uint8_t sendbuff[finalsendsize];

        sendbuff[0] = IS_GROUP_LEADER;
        sendbuff[1] = vid;
        memcpy(sendbuff+2, decrypted, sizenosign);
        memcpy(sendbuff+2+sizenosign, siga, 2*signsize+1);
        memcpy(sendbuff+2+sizenosign+2*signsize+1, buffrc+sizemod16+2*signsize+1, 21);
        
        if(get_metrics != 0) {
            std::cout << "GL_LEADERSHIP_PROOF message size: " << finalsendsize << std::endl;
            
            std::cout << fixed << "EXTRACT_GL_PROOF consumption: " << prev_energy[vid] - Vehicle_sources->Get(vid)->GetRemainingEnergy() << std::endl;
            cout << fixed << "EXTRACT_GL_PROOF power: "
                << (prev_energy[vid] - Vehicle_sources->Get(vid)->GetRemainingEnergy())/(Simulator::Now().GetSeconds() - prev_times[vid]) << " Watt" << endl;
            prev_energy[vid] = Vehicle_sources->Get(vid)->GetRemainingEnergy();
            prev_times[vid] = Simulator::Now().GetSeconds();
        }

        Ptr<Node> n1 =  ns3::NodeList::GetNode(vid);
        Ptr <NetDevice> d0 = n1->GetDevice(0);
        Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

        Ptr <Packet> packet_i;
        Mac48Address dest = Mac48Address::GetBroadcast();

        uint16_t protocol = 0x88dc;
        TxInfo tx;
        tx.preamble = WIFI_PREAMBLE_LONG;
        tx.channelNumber = CCH;
        tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
        tx.priority = 7;	//We set the AC to highest prior
        tx.txPowerLevel = 7; //When we define TxPowerStar

        for(uint32_t i=0; i < 250; i+=2) {
            packet_i = Create<Packet>(sendbuff, finalsendsize);
            Simulator::Schedule(Seconds(i), &WaveNetDevice::SendX, wd0, packet_i, dest, protocol, tx);
        }

        break;
    }
    
    case 1: {
        Vehicle_data_ec *veh1ec = &vehec[vid];
        CryptoPP::AutoSeededRandomPool prng;  
        int size = veh1ec->group.GetCurve().FieldSize().ByteCount();

        int sizenosign = 27 + 31 + size + 1;
        int sizemod16 = sizenosign + 16 - sizenosign%16;
        uint8_t decrypted[sizemod16];
        
        decrypt_message_AES(decrypted, buffrc, sizemod16, veh1ec->symm, veh1ec->iv);
        std::string lead((char*)decrypted, 27);
        std::string tocmp = "Leader";
        if(memcmp(lead.c_str(), tocmp.c_str(), 6) != 0) {
            return;
        }
        else
            std::cout << BOLD_CODE << GREEN_CODE << "Node " << vid << " is selected as GL!" << END_CODE << std::endl;

        int nok = validate_timestamp(lead.substr(7, 27));
        if(nok) {
            std::cout << RED_CODE << "Invalid Timestamp." << END_CODE << std::endl;
            return;
        }
        else 
            std::cout << BOLD_CODE << GREEN_CODE << "Timestamp is valid." << END_CODE << std::endl;


        ECQV cert(veh1ec->group);
        vector<unsigned char> cert_vec;
        cert_vec.insert(cert_vec.end(), decrypted+27, decrypted+27+31+size+1);

        auto start = chrono::high_resolution_clock::now();
        Element rec_pub = cert.cert_pk_extraction(cert_vec);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Certificate public key extraction: "
                << duration.count() << " microseconds" << endl;
        }

        if(veh1ec->pub.x != cert.get_calculated_Qu().x) {
            std::cout << BOLD_CODE << RED_CODE << "Certificate provided to GL is incorrect." << END_CODE << std::endl;
            return;
        }

        char sig[2*size+1];
        memcpy(sig, buffrc+sizemod16, 2*size+1);
        std::string sigecc(sig, 2*size+1);

        start = chrono::high_resolution_clock::now();
        nok = verify_ec(sigecc, veh1ec->rsupub, decrypted, sizenosign);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            return;
        }

        glec.myid = vid;
        glec.mydata = veh1ec;
        glec.mydata->state = IS_GROUP_LEADER;
        std::cout << BOLD_CODE << YELLOW_CODE << "Group Leader " << vid << " broadcasting proof of leadership for new vehicles..." << END_CODE << std::endl << std::endl;

        int sendsize = 2 + sizenosign + 2*size + 1;
        uint8_t sendbuff[sendsize];

        sendbuff[0] = IS_GROUP_LEADER;
        sendbuff[1] = vid;
        memcpy(sendbuff+2, decrypted, sizenosign);
        memcpy(sendbuff+2+sizenosign, buffrc+sizemod16, 2*size+1);

        if(get_metrics != 0) {
            std::cout << "GL_LEADERSHIP_PROOF message size: " << sendsize << std::endl;
            
            std::cout << fixed << "EXTRACT_GL_PROOF consumption: " << prev_energy[vid] - Vehicle_sources->Get(vid)->GetRemainingEnergy() << std::endl;
            cout << fixed << "EXTRACT_GL_PROOF power: "
                << (prev_energy[vid] - Vehicle_sources->Get(vid)->GetRemainingEnergy())/(Simulator::Now().GetSeconds() - prev_times[vid]) << " Watt" << endl;
            prev_energy[vid] = Vehicle_sources->Get(vid)->GetRemainingEnergy();
            prev_times[vid] = Simulator::Now().GetSeconds();
        }

        Ptr<Node> n1 =  ns3::NodeList::GetNode(vid);
        Ptr <NetDevice> d0 = n1->GetDevice(0);
        Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

        Ptr <Packet> packet_i;
        Mac48Address dest = Mac48Address::GetBroadcast();

        uint16_t protocol = 0x88dc;
        TxInfo tx;
        tx.preamble = WIFI_PREAMBLE_LONG;
        tx.channelNumber = CCH;
        tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
        tx.priority = 7;	//We set the AC to highest prior
        tx.txPowerLevel = 7; //When we define TxPowerStar

        for(uint32_t i=0; i < 250; i+=2) {
            packet_i = Create<Packet>(sendbuff, sendsize);
            Simulator::Schedule(Seconds(i), &WaveNetDevice::SendX, wd0, packet_i, dest, protocol, tx);
        }

        break;
    }

    case 2: {
        Vehicle_data_g3 *veh1g3 = &vehg3[vid];
        ZZ ptest = to_ZZ(pg3);
        field_t::init(ptest);
        int size = NumBytes(ptest);
        
        int sizenosign = 27 + 31 + 6*size;
        int sizemod16 = sizenosign + 16 - sizenosign%16;
        uint8_t decrypted[sizemod16];
        
        decrypt_message_AES(decrypted, buffrc, sizemod16, veh1g3->symm, veh1g3->iv);
        
        std::string lead((char*)decrypted, 27);
        std::string tocmp = "Leader";
        if(memcmp(lead.c_str(), tocmp.c_str(), 6) != 0) {
            return;
        }
        else
            std::cout << BOLD_CODE << GREEN_CODE << "Node " << vid << " is selected as GL!" << END_CODE << std::endl;

        int nok = validate_timestamp(lead.substr(7, 27));
        if(nok)
            return;
        else 
            std::cout << BOLD_CODE << GREEN_CODE << "Timestamp is valid." << END_CODE << std::endl;

        g3HEC::g3divisor g, capub, mypub;
        bytes_to_divisorg3(g, veh1g3->g, veh1g3->curve, ptest);
        bytes_to_divisorg3(capub, veh1g3->capub, veh1g3->curve, ptest);
        bytes_to_divisorg3(mypub, veh1g3->pub, veh1g3->curve, ptest);
        g3HECQV cert2(veh1g3->curve, ptest, g);

        auto start = chrono::high_resolution_clock::now();
        cert2.cert_pk_extraction(decrypted+27, capub);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Certificate public key extraction: "
                << duration.count() << " microseconds" << endl;
        }

        if(mypub != cert2.get_calculated_Qu()) {
            std::cout << BOLD_CODE << RED_CODE << "Certificate provided to GL is incorrect." << END_CODE << std::endl;
            return;
        }

        int signsize = NumBytes(to_ZZ(psign3));
        uint8_t *siga = new uint8_t[6*signsize];
        ZZ sigb;

        memcpy(siga, buffrc+sizemod16, 6*signsize);
        sigb = ZZFromBytes(buffrc+sizemod16+6*signsize, 21);

        start = chrono::high_resolution_clock::now();
        nok = verify_sig3(siga, sigb, decrypted, sizenosign, hpk3);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            sigb = -sigb;
            nok = verify_sig3(siga, sigb, decrypted, sizenosign, hpk3);
            if(nok)
                return;
        }

        gl3.myid = vid;
        gl3.mydata = veh1g3;
        gl3.mydata->state = IS_GROUP_LEADER;
        std::cout << BOLD_CODE << YELLOW_CODE << "Group Leader " << vid << " broadcasting proof of leadership for new vehicles..." << END_CODE << std::endl << std::endl;

        int sendsize1 = 2 + sizenosign + 6*signsize + 21;
        int finalsendsize = sendsize1;
        uint8_t sendbuff[finalsendsize];

        sendbuff[0] = IS_GROUP_LEADER;
        sendbuff[1] = vid;
        memcpy(sendbuff+2, decrypted, sizenosign);
        memcpy(sendbuff+2+sizenosign, siga, 6*signsize);
        memcpy(sendbuff+2+sizenosign+6*signsize, buffrc+sizemod16+6*signsize, 21);
        
        if(get_metrics != 0) {
            std::cout << "GL_LEADERSHIP_PROOF message size: " << finalsendsize << std::endl;
            
            std::cout << fixed << "EXTRACT_GL_PROOF consumption: " << prev_energy[vid] - Vehicle_sources->Get(vid)->GetRemainingEnergy() << std::endl;
            cout << fixed << "EXTRACT_GL_PROOF power: "
                << (prev_energy[vid] - Vehicle_sources->Get(vid)->GetRemainingEnergy())/(Simulator::Now().GetSeconds() - prev_times[vid]) << " Watt" << endl;
            prev_energy[vid] = Vehicle_sources->Get(vid)->GetRemainingEnergy();
            prev_times[vid] = Simulator::Now().GetSeconds();
        }

        Ptr<Node> n1 =  ns3::NodeList::GetNode(vid);
        Ptr <NetDevice> d0 = n1->GetDevice(0);
        Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (d0);

        Ptr <Packet> packet_i;
        Mac48Address dest = Mac48Address::GetBroadcast();

        uint16_t protocol = 0x88dc;
        TxInfo tx;
        tx.preamble = WIFI_PREAMBLE_LONG;
        tx.channelNumber = CCH;
        tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
        tx.priority = 7;	//We set the AC to highest prior
        tx.txPowerLevel = 7; //When we define TxPowerStar

        for(uint32_t i=0; i < 250; i+=2) {
            packet_i = Create<Packet>(sendbuff, finalsendsize);
            Simulator::Schedule(Seconds(i), &WaveNetDevice::SendX, wd0, packet_i, dest, protocol, tx);
        }

        break;
    }
    default:
        break;
    }
}

void schedule_inform_message(int ec_algo, int vid, int glid) {
    double posx, posy;
    Ptr<Node> currn = ns3::NodeList::GetNode(vid);
    Ptr<MobilityModel> mob = currn->GetObject<MobilityModel>();
    posx = mob->GetPosition().x;
    posy = mob->GetPosition().y;

    std::string inform = "Node " + to_string(vid) + " is at position: ";

    if(posx > 0 && posx < 100)
        inform += " " + to_string(posx); 
    else
        inform += to_string(posx); 

    if(posy > 0 && posy < 100)   
        inform += "  " + to_string(posy) + " ";
    else 
        inform += " " + to_string(posy) + " ";
        
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    auto str = oss.str();

    inform += str;

    int messagesize = inform.length() + 1;
    int sizemod16 = messagesize + 16 - messagesize%16;
    uint8_t cypherbuff[sizemod16];

    std::string key,iv;

    switch (ec_algo)
    {
    case 0: {
        key = vehg2[vid].symm;
        iv = vehg2[vid].iv;
        break;
    }
    case 1: {
        key = vehec[vid].symm;
        iv = vehec[vid].iv;
        break;
    }
    case 2: {
        key = vehg3[vid].symm;
        iv = vehg3[vid].iv;
        break;
    }
    default:
        break;
    }

    encrypt_message_AES(cypherbuff, (uint8_t*)inform.c_str(), inform.length()+1, key, iv);
    
    int fullsize=0;
    uint8_t *sendbuff;

    if(ec_algo == 1) {
        int size = vehec[vid].group.GetCurve().FieldSize().ByteCount();
        fullsize = sizemod16 + 2*size+1;
        sendbuff = new uint8_t[fullsize+2];

        std::string sigecc;

        auto start = chrono::high_resolution_clock::now();
        sign_ec(sigecc, vehec[vid].priv, (uint8_t*)inform.c_str(), inform.length());

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature generation: "
                << duration.count() << " microseconds" << endl;
        }

        int nok = verify_ec(sigecc, vehec[vid].pub, (uint8_t*)inform.c_str(), inform.length());
        if(nok)
            return;
        

        sendbuff[0] = INFORM_MSG;
        sendbuff[1] = vid;
        memcpy(sendbuff+2, cypherbuff, sizemod16);
        memcpy(sendbuff+2+sizemod16, sigecc.c_str(), 2*size+1);
    }
    else if(ec_algo == 0) {
        int size = NumBytes(to_ZZ(pg2));
        fullsize = sizemod16 + 2*size + 22;
        sendbuff = new uint8_t[fullsize+2];
        uint8_t siga[2*size+1];
        ZZ sigb;

        auto start = chrono::high_resolution_clock::now();
        sign_genus2(siga, sigb, (uint8_t*)inform.c_str(), inform.length(), to_ZZ(pg2));
        
        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature generation: "
                << duration.count() << " microseconds" << endl;
        }
        
        int nok = verify_sig2(siga, sigb, (uint8_t *)inform.c_str(), inform.length(), hpk);
        if(nok)
            return;

        sendbuff[0] = INFORM_MSG;
        sendbuff[1] = vid;
        memcpy(sendbuff+2, cypherbuff, sizemod16);
        memcpy(sendbuff+2+sizemod16, siga, 2*size+1);
        BytesFromZZ(sendbuff+sizemod16+2*size+3, sigb, 21);
    }
    else {
        int size = NumBytes(to_ZZ(psign3));
        fullsize = sizemod16 + 6*size + 21;
        sendbuff = new uint8_t[fullsize+2];
        uint8_t siga[6*size];
        ZZ sigb;

        auto start = chrono::high_resolution_clock::now();
        sign_genus3(siga, sigb, (uint8_t*)inform.c_str(), inform.length(), to_ZZ(psign3));
        
        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature generation: "
                << duration.count() << " microseconds" << endl;
        }

        int nok = verify_sig3(siga, sigb, (uint8_t *)inform.c_str(), inform.length(), hpk3);
        if(nok)
            return;

        sendbuff[0] = INFORM_MSG;
        sendbuff[1] = vid;
        memcpy(sendbuff+2, cypherbuff, sizemod16);
        memcpy(sendbuff+2+sizemod16, siga, 6*size);
        BytesFromZZ(sendbuff+sizemod16+6*size+2, sigb, 21);
    }

    if(get_metrics != 0) {
        std::cout << "VEHICLE_INFORM message size: " << fullsize+2 << std::endl;
    }

    Ptr<Node> n1 =  ns3::NodeList::GetNode(vid);
    Ptr <NetDevice> nd1 = n1->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (nd1);

    Ptr<Node> n0 = ns3::NodeList::GetNode(glid);
    Ptr <NetDevice> nd0 = n0->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(sendbuff, fullsize+2);
    Mac48Address dest = Mac48Address::ConvertFrom (nd0->GetAddress());

    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 7; //When we define TxPowerStar

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 3);
    
    float timerand = dis(gen);

    //wd0->SendX(packet_i, dest, protocol, tx);
    Simulator::Schedule(Seconds(timerand), &WaveNetDevice::SendX, wd0, packet_i, dest, protocol, tx);
}

void send_Aggregated_toRSU(int ec_algo, int glid, std::string aggstr) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
    auto str = oss.str();

    aggstr += str;
    
    int messagesize = aggstr.length() + 1;
    int sizemod16 = messagesize + 16 - messagesize%16;
    uint8_t cypherbuff[sizemod16];

    std::string key,iv;

    switch (ec_algo)
    {
    case 0: {
        key = vehg2[glid].symm;
        iv = vehg2[glid].iv;
        break;
    }
    case 1: {
        key = vehec[glid].symm;
        iv = vehec[glid].iv;
        break;
    }
    case 2: {
        key = vehg3[glid].symm;
        iv = vehg3[glid].iv;
        break;
    }
    default:
        break;
    }

    int infnum = aggstr.length()/46;
    encrypt_message_AES(cypherbuff, (uint8_t*)aggstr.c_str(), messagesize, key, iv);
    
    int fullsize=0;
    uint8_t *sendbuff;

    if(ec_algo == 1) {
        int size = vehec[glid].group.GetCurve().FieldSize().ByteCount();
        fullsize = sizemod16 + 2*size+1;
        sendbuff = new uint8_t[fullsize+3];

        std::string sigecc;

        auto start = chrono::high_resolution_clock::now();
        sign_ec(sigecc, vehec[glid].priv, (uint8_t*)aggstr.c_str(), aggstr.length());
        
        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature generation: "
                << duration.count() << " microseconds" << endl;
        }

        int nok = verify_ec(sigecc, vehec[glid].pub, (uint8_t*)aggstr.c_str(), aggstr.length());
        if(nok)
            return;

        sendbuff[0] = INFORM_MSG;
        sendbuff[1] = glid;
        sendbuff[2] = infnum;
        memcpy(sendbuff+3, cypherbuff, sizemod16);
        memcpy(sendbuff+3+sizemod16, sigecc.c_str(), 2*size+1);
    }
    else if(ec_algo==0) {
        int size = NumBytes(to_ZZ(pg2));
        fullsize = sizemod16 + 2*size + 22;
        sendbuff = new uint8_t[fullsize+3];
        uint8_t siga[2*size+1];
        ZZ sigb;

        auto start = chrono::high_resolution_clock::now();
        sign_genus2(siga, sigb, (uint8_t*)aggstr.c_str(), aggstr.length(), to_ZZ(pg2));
        
        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature generation: "
                << duration.count() << " microseconds" << endl;
        }

        int nok = verify_sig2(siga, sigb, (uint8_t *)aggstr.c_str(), aggstr.length(), hpk);
        if(nok)
            return;

        sendbuff[0] = INFORM_MSG;
        sendbuff[1] = glid;
        sendbuff[2] = infnum;
        memcpy(sendbuff+3, cypherbuff, sizemod16);
        memcpy(sendbuff+3+sizemod16, siga, 2*size+1);
        BytesFromZZ(sendbuff+sizemod16+2*size+4, sigb, 21);
    }
    else {
        int size = NumBytes(to_ZZ(psign3));
        fullsize = sizemod16 + 6*size + 21;
        sendbuff = new uint8_t[fullsize+3];
        uint8_t siga[6*size];
        ZZ sigb;

        auto start = chrono::high_resolution_clock::now();
        sign_genus3(siga, sigb, (uint8_t*)aggstr.c_str(), aggstr.length(), to_ZZ(psign3));
        
        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature generation: "
                << duration.count() << " microseconds" << endl;
        }

        int nok = verify_sig3(siga, sigb, (uint8_t *)aggstr.c_str(), aggstr.length(), hpk3);
        if(nok)
            return;

        sendbuff[0] = INFORM_MSG;
        sendbuff[1] = glid;
        sendbuff[2] = infnum;
        memcpy(sendbuff+3, cypherbuff, sizemod16);
        memcpy(sendbuff+3+sizemod16, siga, 6*size);
        BytesFromZZ(sendbuff+sizemod16+6*size+3, sigb, 21);
    }

    if(get_metrics != 0)
                std::cout << "GL_AGGREGATED_INFORM_RSU message size: " << fullsize+3 << std::endl;

    Ptr<Node> n1 =  ns3::NodeList::GetNode(glid);
    Ptr <NetDevice> nd1 = n1->GetDevice(0);
    Ptr <WaveNetDevice> wd0 = DynamicCast<WaveNetDevice> (nd1);

    Ptr<Node> n0 = ns3::NodeList::GetNode(rsuid);
    Ptr <NetDevice> nd0 = n0->GetDevice(0);

    Ptr <Packet> packet_i = Create<Packet>(sendbuff, fullsize+3);
    Mac48Address dest = Mac48Address::ConvertFrom (nd0->GetAddress());

    uint16_t protocol = 0x88dc;
    TxInfo tx;
    tx.preamble = WIFI_PREAMBLE_LONG;
    tx.channelNumber = CCH;
    tx.dataRate = WifiMode ("OfdmRate12MbpsBW10MHz");
    tx.priority = 7;	//We set the AC to highest prior
    tx.txPowerLevel = 7; //When we define TxPowerStar

    wd0->SendX(packet_i, dest, protocol, tx);
    // if(get_metrics != 0) {
    //     std::cout << fixed << "SEND_AGGREGATED_INFO consumption: " << prev_energy[glid] - Vehicle_sources->Get(glid)->GetRemainingEnergy() << std::endl;
    //     cout << fixed << "SEND_AGGREGATED_INFO power: "
    //             << (prev_energy[glid] - Vehicle_sources->Get(glid)->GetRemainingEnergy())/(Simulator::Now().GetSeconds() - prev_times[glid]) << " Watt" << endl;
    //     prev_energy[glid] = Vehicle_sources->Get(glid)->GetRemainingEnergy();
    //     prev_times[glid] = Simulator::Now().GetSeconds();
    // }
    //Simulator::Schedule(Seconds(timerand), &WaveNetDevice::SendX, wd0, packet_i, dest, protocol, tx);

}

void extract_Inform_Aggregate(uint8_t *buffrc, int ec_algo, int vid, int glid) {
    if(dry[vid] == true)
        return;

    std::string key,iv;

    switch (ec_algo)
    {
    case 0: {
        key = gl2.symm_perveh[vid];
        iv = gl2.iv_perveh[vid];
        break;
    }
    case 1: {
        key = glec.symm_perveh[vid];
        iv = glec.iv_perveh[vid];
        break;
    }
    case 2: {
        key = gl3.symm_perveh[vid];
        iv = gl3.iv_perveh[vid];
        break;
    }
    default:
        break;
    }

    int messagesize = 66;
    int sizemod16 = messagesize + 16 - messagesize%16;
    uint8_t decrypted[sizemod16];


    decrypt_message_AES(decrypted, buffrc, sizemod16, key, iv);
    std::cout << std::endl << BOLD_CODE << GREEN_CODE << "Received info message from node: " << vid << std::endl;

    std::string tmstmp((char*)decrypted+46);
    int nok = validate_timestamp(tmstmp);
    if(nok) {
      std::cout << BOLD_CODE << RED_CODE << "Message not fresh" << END_CODE << std::endl;
      return;
    }
    else {
      std::cout << BOLD_CODE << GREEN_CODE << "Timestamp is valid." << END_CODE << std::endl;
    }

    if (ec_algo == 1) {
        int size = glec.mydata->group.GetCurve().FieldSize().ByteCount();
        std::string sigecc((char*)buffrc+sizemod16, 2*size+1);

        auto start = chrono::high_resolution_clock::now();
        nok = verify_ec(sigecc, glec.vehpk[vid], decrypted, messagesize-1);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok)
            return;

        
        std::string informed((char*)decrypted, 46);
        glec.agg_messages += informed;
        //std::cout << glec.agg_messages << std::endl;
        dry[vid] = true;
    }
    else if(ec_algo==0){
        int size = NumBytes(to_ZZ(pg2));
        uint8_t siga[2*size+1];
        ZZ sigb;

        memcpy(siga, buffrc+sizemod16, 2*size+1);
        sigb = ZZFromBytes(buffrc+sizemod16+2*size+1, 21);
        
        auto start = chrono::high_resolution_clock::now();
        nok = verify_sig2(siga, sigb, decrypted, messagesize-1, hpk);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            sigb = -sigb;
            nok = verify_sig2(siga, sigb, decrypted, messagesize-1, hpk);
            if(nok)
                return;
        }
        std::string informed((char*)decrypted, 46);

        gl2.agg_messages += informed;
        //std::cout << gl2.agg_messages << std::endl;
        

        dry[vid] = true;
    }
    else {
        int size = NumBytes(to_ZZ(psign3));
        uint8_t siga[6*size];
        ZZ sigb;

        memcpy(siga, buffrc+sizemod16, 6*size);
        sigb = ZZFromBytes(buffrc+sizemod16+6*size, 21);

        auto start = chrono::high_resolution_clock::now();
        nok = verify_sig3(siga, sigb, decrypted, messagesize-1, hpk3);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            sigb = -sigb;
            nok = verify_sig3(siga, sigb, decrypted, messagesize-1, hpk3);
            if(nok)
                return;
        }
        std::string informed((char*)decrypted, 46);
        

        gl3.agg_messages += informed;
        //std::cout << gl3.agg_messages << std::endl;

        dry[vid] = true;
    }
    if(get_metrics != 0) {
        std::cout << fixed << "EXTRACT_INFO_GL consumption: " << prev_energy[glid] - Vehicle_sources->Get(glid)->GetRemainingEnergy() << std::endl;
        cout << fixed << "EXTRACT_INFO_GL power: "
                << (prev_energy[glid] - Vehicle_sources->Get(glid)->GetRemainingEnergy())/(Simulator::Now().GetSeconds() - prev_times[glid]) << " Watt" << endl;
        prev_energy[glid] = Vehicle_sources->Get(glid)->GetRemainingEnergy();
        prev_times[glid] = Simulator::Now().GetSeconds();
    }
    if(vid >= 62) {
        std::string aggstr;
        if(ec_algo == 0) {
            aggstr = gl2.agg_messages;
        }
        else if(ec_algo == 1) {
            aggstr = glec.agg_messages;
        }
        else{
            aggstr = gl3.agg_messages;
        }
        send_Aggregated_toRSU(ec_algo, glid, aggstr);
    } 
}


void extract_Info_RSU(uint8_t *buffrc, int infnum, int ec_algo, int glid) {
    if(already_here)
        return;
    
    std::string key,iv, received;

    switch (ec_algo)
    {
    case 0: {
        key = rsug2[0].symm_perveh[glid];
        iv = rsug2[0].iv_perveh[glid];
        break;
    }
    case 1: {
        key = rsuec[0].symm_perveh[glid];
        iv = rsuec[0].iv_perveh[glid];
        break;
    }
    case 2: {
        key = rsug3[0].symm_perveh[glid];
        iv = rsug3[0].iv_perveh[glid];
        break;
    }
    default:
        break;
    }

    int messagesize = infnum*46 + 20;
    int sizemod16 = messagesize + 16 - messagesize%16;
    uint8_t decrypted[sizemod16];


    decrypt_message_AES(decrypted, buffrc, sizemod16, key, iv);
    std::cout << BOLD_CODE << GREEN_CODE << "Received info message from GL!" << std::endl;

    std::string tmstmp((char*)decrypted+infnum*46);
    int nok = validate_timestamp(tmstmp);
    if(nok) {
      std::cout << BOLD_CODE << RED_CODE << "Message not fresh" << END_CODE << std::endl;
      return;
    }
    else {
      std::cout << BOLD_CODE << GREEN_CODE << "Timestamp is valid." << END_CODE << std::endl;
    }

    if (ec_algo == 1) {
        int size = rsuec[0].group.GetCurve().FieldSize().ByteCount();
        std::string sigecc((char*)buffrc+sizemod16, 2*size+1);

        auto start = chrono::high_resolution_clock::now();
        nok = verify_ec(sigecc, rsuec[0].vehpk[glid], decrypted, messagesize-1);
        
        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok)
            return;

        std::string informed((char*)decrypted, infnum*46);
        received = informed;
    }
    else if(ec_algo==0){
        int size = NumBytes(to_ZZ(pg2));
        uint8_t siga[2*size+1];
        ZZ sigb;

        memcpy(siga, buffrc+sizemod16, 2*size+1);
        sigb = ZZFromBytes(buffrc+sizemod16+2*size+1, 21);

        auto start = chrono::high_resolution_clock::now();
        nok = verify_sig2(siga, sigb, decrypted, messagesize-1, hpk);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            sigb = -sigb;
            nok = verify_sig2(siga, sigb, decrypted, messagesize-1, hpk);
            if(nok)
                return;
        }
        std::string informed((char*)decrypted, infnum*46);
        received = informed;
    }
    else {
        int size = NumBytes(to_ZZ(psign3));
        uint8_t siga[6*size];
        ZZ sigb;

        memcpy(siga, buffrc+sizemod16, 6*size);
        sigb = ZZFromBytes(buffrc+sizemod16+6*size, 21);

        auto start = chrono::high_resolution_clock::now();
        nok = verify_sig3(siga, sigb, decrypted, messagesize-1, hpk3);

        if(get_metrics != 0) {
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
        
            cout << "Signature verification: "
                << duration.count() << " microseconds" << endl;
        }

        if(nok) {
            sigb = -sigb;
            nok = verify_sig3(siga, sigb, decrypted, messagesize-1, hpk3);
            if(nok)
                return;
        }
        std::string informed((char*)decrypted, infnum*46);
        received = informed;
    }
    std::cout << std::endl << BOLD_CODE << YELLOW_CODE << received << END_CODE << std::endl << std::endl;

    already_here = true;
}