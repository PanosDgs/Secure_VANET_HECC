#include "sign.h"

static ZZ from_divisor_to_ZZ(const NS_G2_NAMESPACE::divisor& div, const ZZ& n)
{
  poly_t u = div.get_upoly();
  ZZ temp = AddMod(sqr(rep(u.rep[0])), sqr(rep(u.rep[1])), n);
  return ( IsZero(temp) ? to_ZZ(1) : temp );
}

static ZZ from_g3divisor_to_ZZ(const g3HEC::g3divisor& div, const ZZ& n)
{
  poly_t u = div.get_upoly();
  ZZ temp = AddMod(sqr(rep(u.rep[0])), sqr(rep(u.rep[1])), n);
  temp = AddMod(temp, sqr(rep(u.rep[2])), n);
  return ( IsZero(temp) ? to_ZZ(1) : temp );
}

int sign_genus2 (uint8_t *asig, ZZ& b, uint8_t *mess, int size, ZZ pch) {
    NTL::ZZ_pContext context;
    context.save();
    CryptoPP::SHA3_224 hash;
    hash.Update(mess, size);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);
    
    ZZ m;
    m = ZZFromBytes((unsigned char*)digest.data(), 28);
    SetSeed(to_ZZ(1234567890));

    ZZ p = to_ZZ(pg2); 
    field_t::init(p); 

    ZZ order = to_ZZ(Ng2);

    ZZ x, k; 
    // Private key x, random number k

    ZZ f_a;

    NS_G2_NAMESPACE::g2hcurve curve;

    NS_G2_NAMESPACE::divisor a, g, h;

    poly_t f;

    SetCoeff(f, 5, 1);
    SetCoeff(f, 4, 0);
    SetCoeff(f, 3, str_to_ZZ_p(f3g2));
    SetCoeff(f, 2, str_to_ZZ_p(f2g2));
    SetCoeff(f, 1, str_to_ZZ_p(f1g2));
    SetCoeff(f, 0, str_to_ZZ_p(f0g2));
    curve.set_f(f);

    curve.update();
    g.set_curve(curve);
    poly_t gu, gv;
    SetCoeff(gu, 2, 1);
    SetCoeff(gu, 1, str_to_ZZ_p(gu1g2));
    SetCoeff(gu, 0, str_to_ZZ_p(gu0g2));
    SetCoeff(gv, 1, str_to_ZZ_p(gv1g2));
    SetCoeff(gv, 0, str_to_ZZ_p(gv0g2));
    g.set_upoly(gu);
    g.set_vpoly(gv);
    g.update();
    
    x = to_ZZ(xpk);

    h = x * g;

    do {
        RandomBnd(k, order);
    } while (IsZero(k));

    a = k * g;

    f_a = from_divisor_to_ZZ(a, order);

    /* b = (m - x*f(a))/k mod N */
    b = ((m - x*f_a)*InvMod(k, order))%order;

    if ( f_a * h + b * a == m * g ) {
        //cout << "Created ElGamal signature!" << endl;
        divisor_to_bytes(asig, a, curve, p);
        context.restore();
        return 0;
    }
    else {
        cout << "Could not create ElGamal signature." << endl;
        context.restore();
        return 1;
    }
}



int verify_sig2(uint8_t *siga, ZZ sigb, uint8_t *mess, int size, uint8_t *pk) {
    NTL::ZZ_pContext context;
    context.save();
    CryptoPP::SHA3_224 hash;
    hash.Update(mess, size);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);
    
    ZZ m;
    m = ZZFromBytes((unsigned char*)digest.data(), 28);
    SetSeed(to_ZZ(1234567890));

    ZZ p = to_ZZ(pg2); 
    
    field_t::init(p); 
    ZZ order = to_ZZ(Ng2);

    NS_G2_NAMESPACE::g2hcurve curve;

    NS_G2_NAMESPACE::divisor a, g, h;
    poly_t f;

    SetCoeff(f, 5, 1);
    SetCoeff(f, 4, 0);
    SetCoeff(f, 3, str_to_ZZ_p(f3g2));
    SetCoeff(f, 2, str_to_ZZ_p(f2g2));
    SetCoeff(f, 1, str_to_ZZ_p(f1g2));
    SetCoeff(f, 0, str_to_ZZ_p(f0g2));
    curve.set_f(f);

    curve.update();
    g.set_curve(curve);
    poly_t gu, gv;
    SetCoeff(gu, 2, 1);
    SetCoeff(gu, 1, str_to_ZZ_p(gu1g2));
    SetCoeff(gu, 0, str_to_ZZ_p(gu0g2));
    SetCoeff(gv, 1, str_to_ZZ_p(gv1g2));
    SetCoeff(gv, 0, str_to_ZZ_p(gv0g2));
    g.set_upoly(gu);
    g.set_vpoly(gv);
    g.update();

    bytes_to_divisor(a, siga, curve, p);
    ZZ f_a = from_divisor_to_ZZ(a, order);
    bytes_to_divisor(h, pk, curve, p);

    if ( f_a * h + sigb * a == m * g ) {
        //cout << "ElGamal signature verification succeeded!" << endl;
        context.restore();
        return 0;
    }
    else {
        cout << "ElGamal signature verification did not succeed!" << endl;
        context.restore();
        return 1;
    }
}


int sign_ec(std::string &sig, CryptoPP::Integer kecc, uint8_t *message, int size) {
    using namespace CryptoPP;
    
    AutoSeededRandomPool prng;
    GroupParameters group;
    group.Initialize(CryptoPP::ASN1::secp256r1());

    ECDSA<ECP, SHA256>::PrivateKey k1;
    k1.Initialize(ASN1::secp256r1(), kecc );
    ECDSA<ECP, SHA256>::Signer signer(k1);

    size_t siglen = signer.MaxSignatureLength();
    std::string signature(siglen, 0x00);

    siglen = signer.SignMessage( prng, message, size, (byte*)&signature[0] );

    signature.resize(siglen);
    sig = signature;
    
    return 0;
}

int verify_ec(std::string sig, Element Pk, uint8_t *message, int size) {
    using namespace CryptoPP;
    
    AutoSeededRandomPool prng;
    GroupParameters group;
    group.Initialize(CryptoPP::ASN1::secp256r1());

    ECDSA<ECP, SHA256>::PublicKey publicKey;
    publicKey.Initialize(ASN1::secp256r1(), Pk);
    ECDSA<ECP, SHA256>::Verifier verifier(publicKey);

    bool result = verifier.VerifyMessage( message, size, (const byte*)&sig[0], sig.length());
    
    // Verification failure?
    if( !result ) {
        std::cout << "Failed to verify signature on message" << std::endl;
        return 1;
    } else {
        //std::cout << "Verified signature!" << std::endl;
        return 0;
    }
}

int sign_genus3(uint8_t *siga, ZZ& sigb, uint8_t *mess, int size, ZZ pch) {

    NTL::ZZ_pContext context;
    context.save();
    CryptoPP::SHA3_224 hash;
    hash.Update(mess, size);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);

    using namespace g3HEC;    
    ZZ m;
    m = ZZFromBytes((unsigned char*)digest.data(), 28);
    SetSeed(to_ZZ(1234567890));

    ZZ p = to_ZZ(psign3); 
    field_t::init(p); 

    ZZ order = to_ZZ(Nsign3)/8;

    ZZ x, k; // Private key x, random number k, parameter b, message m

    ZZ f_a;

    g3hcurve curve;

    g3divisor g, h, a;

    poly_t f;

    SetCoeff(f, 7, 1);
    SetCoeff(f, 6, 0);
    SetCoeff(f, 5, str_to_ZZ_p(f5g3));
    SetCoeff(f, 4, 0);
    SetCoeff(f, 3, str_to_ZZ_p(f3g3));
    SetCoeff(f, 2, 0);
    SetCoeff(f, 1, str_to_ZZ_p(f1g3));
    SetCoeff(f, 0, 0);
    curve.set_f(f);
    curve.update();

    g.set_curve(curve);

    /* Base point g */
    poly_t gu, gv;
    SetCoeff(gu, 3, 1);
    SetCoeff(gu, 2, str_to_ZZ_p("58357352621437838"));
    SetCoeff(gu, 1, str_to_ZZ_p("21425791699477544"));
    SetCoeff(gu, 0, str_to_ZZ_p("20774870158739"));
    SetCoeff(gv, 2, str_to_ZZ_p("86441353997400432"));
    SetCoeff(gv, 1, str_to_ZZ_p("56397561880067344"));
    SetCoeff(gv, 0, str_to_ZZ_p("98926713531841630"));
    g.set_upoly(gu);
    g.set_vpoly(gv);
    g.update();

    /* private key x <> 0 */
    x = to_ZZ("27042584100361508324090725462401442475697463425802");

    /* random number k <> 0*/
    do {
        RandomBnd(k, order);
    } while (IsZero(k));

    a = k * g;
    h = x*g;

    f_a = from_g3divisor_to_ZZ(a, order);

    sigb = ((m - x*f_a)*InvMod(k, order))%order;
    
    /* Signature verification */
    if ( f_a*h + sigb*a == m*g ) {
        // cout << "HECg3 ElGamal signature verification succeeded!" << endl;
        divisorg3_to_bytes(siga, a, curve, p);
        context.restore();
        return 0;
    }
    else
        cout << "Could not create ElGamal signature of HEC genus 3." << endl;

    context.restore();
    return 0;

}

int verify_sig3(uint8_t *siga, ZZ sigb, uint8_t *mess, int size, uint8_t *pk) {
    NTL::ZZ_pContext context;
    context.save();
    CryptoPP::SHA3_224 hash;
    hash.Update(mess, size);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);
    
    ZZ m;
    m = ZZFromBytes((unsigned char*)digest.data(), 28);
    SetSeed(to_ZZ(1234567890));

    using namespace g3HEC;
    ZZ p = to_ZZ(psign3); 
    field_t::init(p); 

    ZZ order = to_ZZ(Nsign3)/8;

    ZZ f_a;

    g3hcurve curve;

    g3divisor g, h, a;

    poly_t f;

    SetCoeff(f, 7, 1);
    SetCoeff(f, 6, 0);
    SetCoeff(f, 5, str_to_ZZ_p(f5g3));
    SetCoeff(f, 4, 0);
    SetCoeff(f, 3, str_to_ZZ_p(f3g3));
    SetCoeff(f, 2, 0);
    SetCoeff(f, 1, str_to_ZZ_p(f1g3));
    SetCoeff(f, 0, 0);
    curve.set_f(f);
    curve.update();

    g.set_curve(curve);

    /* Base point g */
    poly_t gu, gv;
    SetCoeff(gu, 3, 1);
    SetCoeff(gu, 2, str_to_ZZ_p("58357352621437838"));
    SetCoeff(gu, 1, str_to_ZZ_p("21425791699477544"));
    SetCoeff(gu, 0, str_to_ZZ_p("20774870158739"));
    SetCoeff(gv, 2, str_to_ZZ_p("86441353997400432"));
    SetCoeff(gv, 1, str_to_ZZ_p("56397561880067344"));
    SetCoeff(gv, 0, str_to_ZZ_p("98926713531841630"));
    g.set_upoly(gu);
    g.set_vpoly(gv);
    g.update();

    bytes_to_divisorg3(a, siga, curve, p);
    f_a = from_g3divisor_to_ZZ(a, order);
    bytes_to_divisorg3(h, pk, curve, p);

    if ( f_a * h + sigb * a == m * g ) {
        // cout << "ElGamal signature verification succeeded!" << endl;
        context.restore();
        return 0;
    }
    else {
        cout << "ElGamal signature verification did not succeed!" << endl;
        context.restore();
        return 1;
    }
}