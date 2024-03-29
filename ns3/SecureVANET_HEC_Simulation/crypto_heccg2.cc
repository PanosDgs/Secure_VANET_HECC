#include "crypto_heccg2.h"
#include "helpers.h"

/* --------------------------- Serialize helper --------------------------- */

void serialize_generic(divisor D, vector<unsigned char> &buff, g2hcurve crv, ZZ p)
{
    int size = NTL::NumBytes(p);

    /* Get divisor polys and curve poly f */
    poly_t u = D.get_upoly();
    poly_t v = D.get_vpoly();
    poly_t f = crv.get_f();

    /* u poly coefficients to bytes */
    ZZ_p c1, c2;
    GetCoeff(c1, u, 1);
    GetCoeff(c2, u, 0);
    uint8_t *c1z, *c2z;
    c1z = new uint8_t[size];
    NTL::BytesFromZZ(c1z, rep(c1), size);
    c2z = new uint8_t[size];
    NTL::BytesFromZZ(c2z, rep(c2), size);

    buff.insert(buff.end(), c1z, c1z+size);
    buff.insert(buff.end(), c2z, c2z+size);

    /* Based on Handbook of Elliptic and Hyperelliptic Curve Cryptography,
    chapter 14.2 - compression techniques */
    ZZ_p s0, v0, f0;
    GetCoeff(v0, v, 0);
    GetCoeff(f0, f, 0);

    ZZ_p f1, f2, f3, f4, v1;
    GetCoeff(v1, v, 1);
    GetCoeff(f1, f, 1);
    GetCoeff(f2, f, 2);
    GetCoeff(f3, f, 3);
    GetCoeff(f4, f, 4);
    if(c2 != 0)
        s0 = (v0*v0-f0)/c2;
    else {
        s0 = v1*v1 - f2 +f3*c1 + f4*(c2 - c1*c1) - c1*(2*c2 - c1*c1);
    }

    if((c1*c1 - 4*c2) != 0) {
        poly_t bsq, ap, gp;
        SetX(bsq);
        NTL::SetCoeff(bsq, 1, c1);
        NTL::SetCoeff(bsq, 0, (f1 - f3*c2 + f4*c2*c1 + c2*(c2 - c1*c1)));
        bsq = bsq*bsq;
        
        SetX(ap);
        NTL::SetCoeff(ap, 0, (f2 - f3*c1 - f4*(c2 - c1*c1) + c1*(2*c2 - c1*c1)));
        SetX(gp);
        NTL::SetCoeff(gp, 1, c2);
        NTL::SetCoeff(gp, 0, f0);

        poly_t ds0 = bsq - 4*ap*gp;
        MakeMonic(ds0);
        vec_ZZ_p roots = FindRoots(ds0);
        if((s0 == roots[0]) && (rep(roots[0]) < rep(roots[1])))
            buff.push_back(0);
        else if ((s0 == roots[0]) && (rep(roots[0]) > rep(roots[1])))
            buff.push_back(1);
        else if ((s0 == roots[1]) && (rep(roots[1]) > rep(roots[0])))
            buff.push_back(1);
        else
            buff.push_back(0);
    }
    else {
        buff.push_back(0);
    }

    if(v0!=0){
        if(rep(v0)%2 != 0) {
            buff[2*size] += 2;
        }
    }
    else {
        if(rep(v1)%2 != 0) {
            buff[2*size] += 2;
        }
    }
}


divisor deserialize_generic(vector<unsigned char> buff, g2hcurve crv, ZZ p)
{
    int size = NTL::NumBytes(p);

    /* Extract the coefficients of u poly */
    ZZ c1, c2;
    c1 = NTL::ZZFromBytes(buff.data(), size);
    c2 = NTL::ZZFromBytes(buff.data()+size, size);
    poly_t u,v;
    ZZ_p u1 = to_ZZ_p(c1);
    ZZ_p u0 = to_ZZ_p(c2);
    SetCoeff(u,2,1);
    SetCoeff(u,1,u1);
    SetCoeff(u,0,u0);
    
    /* Based on Handbook of Elliptic and Hyperelliptic Curve Cryptography,
    chapter 14.2 - compression techniques */
    uint8_t bits = buff[2*size];
    poly_t f = crv.get_f();
    poly_t bsq, ap, gp;
    ZZ_p f0, f1, f2, f3, f4;
    GetCoeff(f0, f, 0);
    GetCoeff(f1, f, 1);
    GetCoeff(f2, f, 2);
    GetCoeff(f3, f, 3);
    GetCoeff(f4, f, 4);
    SetX(bsq);
    SetCoeff(bsq, 1, u1);
    SetCoeff(bsq, 0, (f1 - f3*u0 + f4*u0*u1 + u0*(u0 - u1*u1)));
    bsq = bsq*bsq;
    
    SetX(ap);
    SetCoeff(ap, 0, (f2 - f3*u1 - f4*(u0 - u1*u1) + u1*(2*u0 - u1*u1)));
    SetX(gp);
    SetCoeff(gp, 1, u0);
    SetCoeff(gp, 0, f0);

    poly_t ds0 = bsq - 4*ap*gp;
    MakeMonic(ds0);

    if(DetIrredTest(ds0)) {
        throw std::runtime_error("Could not deserialize divisor. Compression is in wrong format.");
    }

    vec_ZZ_p roots = FindRoots(ds0);
    ZZ_p s0;
    if(roots[0] == roots[1])
        s0 = roots[0];
    else{
        if((bits & 1) == 1)
            s0 = (rep(roots[0]) < rep(roots[1])) ? roots[1] : roots[0];
        else 
            s0 = (rep(roots[0]) < rep(roots[1])) ? roots[0] : roots[1];
    }

    ZZ_p v0sq, v0, v1;
    v0sq = u0*s0 + f0;
    if(v0sq != 0) {
        v0sq = squareRoot(v0sq, p);
        if((bits & 2) == 2){
            v0 = (rep(v0sq)%2 == 1) ? v0sq : -v0sq; 
        }
        else
            v0 = (rep(v0sq)%2 == 1) ? -v0sq : v0sq;
        v1 = (u1*s0 + f1 - f3*u0 + f4*u0*u1 + u0*(u0 - u1*u1))/(2*v0);
    }
    else{
        v0 = 0;
        ZZ_p v1sq;
        v1sq = s0 + f2 - f3*u1 - f4*(u0 - u1*u1) + u1*(2*u0 - u1*u1);
        v1sq = squareRoot(v1sq, p);
        if((bits & 2) == 2){
            v1 = (rep(v1sq)%2 == 1) ? v1sq : -v1sq; 
        }
        else
            v1 = (rep(v1sq)%2 == 1) ? -v1sq : v1sq;
    }

    /* Build the v poly out of u poly and the bits */
    SetCoeff(v, 1, v1);
    SetCoeff(v, 0, v0);

    divisor D;
    D.set_curve(crv);
    D.set_upoly(u);
    D.set_vpoly(v);
    D.update();
    return D;
}

/* --------------------------- g2HECQV cert methods --------------------------- */


g2HECQV::g2HECQV(NS_G2_NAMESPACE::g2hcurve curve, ZZ p, NS_G2_NAMESPACE::divisor G) {
    assert(curve.is_valid_curve());
    assert(G.is_valid_divisor());
    this->curve = curve;
    this->p = p;
    this->G = G;
    ZZ capriv = to_ZZ("15669032110011017415376799675649225245106855015484313618141721121181084494176");
    this->capriv = capriv;
    this->capk = capriv*G;
}

void g2HECQV::encode_to_bytes(uint8_t *buff) {
    if( !this->pu.is_valid_divisor() || this->name.empty() || this->issued_by.empty() || this->issued_on.empty() || this->expires_on.empty()) {
        throw std::runtime_error("Cannot serialize certificate, some field(s) might be missing");
    }

    memcpy(buff, this->name.c_str(), 7);
    memcpy(buff+7, this->issued_by.c_str(), 4);
    memcpy(buff+11, this->issued_on.c_str(), 10);
    memcpy(buff+21, this->expires_on.c_str(), 10);
    vector<unsigned char> serialized;
    serialize_generic(this->pu, serialized, this->curve, this->p);
    memcpy(buff+31, serialized.data(), serialized.size());
}

vector<unsigned char> g2HECQV::cert_generate(std::string uname, divisor ru) {
    if(uname.length() != 7) {
        throw std::runtime_error("Name must consist of 7 chars");
    }

    ZZ k1;
    RandomBnd(k1, p*p);
    divisor kG = k1*this->G;

    divisor pu1 = ru + kG;
    this->pu = pu1;
    this->name = uname;
    this->issued_by = "DMV1";
    this->issued_on = "01-01-2023";
    this->expires_on = "31-12-2030";

    int element_size = NTL::NumBytes(p);
    
    byte encoded[31 + 2*element_size + 1];
    encode_to_bytes(encoded);
    
    hash.Update(encoded, 31 + 2*element_size + 1);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);
    
    ZZ hashed, n;
    hashed = ZZFromBytes((unsigned char*)digest.data(), 32);
     
    divisor chk = hashed*this->pu + this->capk;
    if(!chk.is_valid_divisor()) {
        throw std::runtime_error("Collision on certificate generation. Retry with a differenet key pair.");
    } 
    this->r = hashed*k1 + this->capriv;
    
    vector<unsigned char> buff;
    buff.insert(buff.end(), encoded, encoded + 31 + 2*element_size + 1);
    return buff;
}

divisor g2HECQV::cert_pk_extraction(vector<unsigned char> cert) {
    int element_size = NumBytes(this->p);

    size_t check_size = 2*element_size + 32;
    if(cert.size() != check_size) {
        throw std::runtime_error("Trying to extract public key from invalid certificate.");
    }

    vector<unsigned char> div;
    div.insert(div.end(), cert.data()+31, cert.data() + 31 + 2*element_size + 1);

    string name((char*)cert.data(), 7);
    this->name = name;

    string issued_by((char*)cert.data()+7, 4);
    this->issued_by = issued_by;

    string issued_on((char*)cert.data()+11, 10);
    this->issued_on = issued_on;

    string exp_on((char*)cert.data()+21, 10);
    this->expires_on = exp_on;

    this->pu = deserialize_generic(div, this->curve, this->p);

    hash.Update(cert.data(), 31 + 2*element_size + 1);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);

    ZZ hashed, n;
    hashed = ZZFromBytes((unsigned char*)digest.data(), 32);
    
    this->qu = hashed*this->pu + this->capk;

    return this->qu;
}

ZZ g2HECQV::cert_reception(vector<unsigned char> cert, ZZ ku) {
    int element_size = NumBytes(this->p);
    
    size_t check_size = 2*element_size + 32;
    if(cert.size() != check_size) {
        throw std::runtime_error("Trying to extract public key from invalid certificate.");
    }

    vector<unsigned char> div;
    div.insert(div.end(), cert.data()+31, cert.data()+31+2*element_size+1);
    divisor pdec = deserialize_generic(div, this->curve, this->p);

    hash.Update(cert.data(), 31 + 2*element_size + 1);
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);

    ZZ hashed, n;
    hashed = ZZFromBytes((unsigned char*)digest.data(), 32);
    ZZ du1 = (this->r + hashed*ku);
    divisor qut = du1*this->G;
    if(this->qu == qut) {
        this->du = du1;
        return this->du;
    }
    
    throw std::runtime_error("Could not extract private key.");
    return to_ZZ(1);
}


divisor g2HECQV::get_calculated_Qu() {
    return this->qu;
}

ZZ g2HECQV::get_extracted_du() {
    return this->du;
}

std::string g2HECQV::get_name() {
    return this->name;
}

std::string g2HECQV::get_issued_by() {
    return this->issued_by;
}

std::string g2HECQV::get_issued_on() {
    return this->issued_on;
}

std::string g2HECQV::get_expires_on() {
    return this->expires_on;
}



/* --------------------------- Crypto HECC genus 2 methods --------------------------- */


tuple<divisor, divisor> CryptoHECCg2::encrypt_ElGamal(divisor pub, divisor mess)
{
    ZZ k;
    RandomBnd(k, p*p);
    divisor a = k * base;
    divisor b = k * pub + mess;
    return tuple<divisor, divisor>(a, b);
}

divisor CryptoHECCg2::decrypt_ElGamal(ZZ priv, divisor a, divisor b)
{
    return (b - priv * a);
}

divisor CryptoHECCg2::points_to_divisor(ZZ_p x1, ZZ_p y1, ZZ_p x2, ZZ_p y2)
{
    divisor D;
    ZZ_p a = -x1 -x2;
    ZZ_p b = x1*x2;
    poly_t u,v;
    SetCoeff(u, 2, 1);
    SetCoeff(u, 1, a);
    SetCoeff(u, 0, b);

    ZZ_p c = (y1-y2)/(x1-x2);
    ZZ_p d = y1 - c*x1;
    SetCoeff(v, 1, c);
    SetCoeff(v, 0, d);

    D.set_curve(curve);
    D.set_upoly(u);
    D.set_vpoly(v);
    D.update();
    return D;
}

void CryptoHECCg2::divisor_to_points (divisor D, ZZ_p &x1, ZZ_p &y1, ZZ_p &x2, ZZ_p &y2)
{
    poly_t u,v;
    u = D.get_upoly();
    v = D.get_vpoly();

    ZZ_p a,b,c,d;
    GetCoeff(a,u,1);
    GetCoeff(d,v,0);
    GetCoeff(c,v,1);
    
    if(DetIrredTest(u)){
        throw std::runtime_error("Irreducible polyonym u for converting divisor to points.");
    }
    x1 = FindRoot(u);
    x2 = -x1 - a;

    y1 = d + c*x1;
    y2 = y1 - c*(x1-x2);
}

divisor CryptoHECCg2::encode(string txt)
{
    divisor D;
    int maxlen = MAX_ENCODING_LEN_G2;
    int len = txt.length();
    UnifiedEncoding enc(p, u_param, w_param, 2);

    if(len > maxlen) {
        throw std::runtime_error("Length of text to be encoded must be at maximum SIZE-4 bytes");
    }

    /* Pad the message to maxlen size with zeros-null bytes */
    uint8_t chk[maxlen];
    memcpy(chk, txt.c_str(), len);
    for(int i = 0; i < maxlen - len; i++) {
        chk[maxlen-1-i] = '\0';
    }
    ZZ chkzz = NTL::ZZFromBytes(chk, len);

    /* Split the message in half and set the first bytes as 1 and 2 to distinguish the correct order of the text halves */
    uint8_t *str1, *str2;
    str1 = new uint8_t [maxlen/2+1];
    str2 = new uint8_t [maxlen/2+1];
    str1[0] = '1';
    str2[0] = '2';

    memcpy(str1+1, chk, maxlen/2);
    memcpy(str2+1, chk + maxlen/2, maxlen/2);

    /* Convert text to ZZ type */
    ZZ msgzz1, msgzz2;
    msgzz1 = NTL::ZZFromBytes(str1, maxlen/2+1);
    msgzz2 = NTL::ZZFromBytes(str2, maxlen/2+1);

    if(msgzz1 >= p || msgzz2 >= p) {
        throw std::runtime_error("Message too big to encode. Please segment the message.");
    }

    /* Encode ZZs to HEC points using the UnifiedEncoding method */
    ZZ_p x1, y1, x2, y2;

    int fl = enc.encode(msgzz1, x1, y1);
    if(fl) {
        throw std::runtime_error("Could not encode message to HEC point");
    }

    fl = enc.encode(msgzz2, x2, y2);

    if(fl){
        throw std::runtime_error("Could not encode message to HEC point");
    }
    
    /* Convert points to a valid divisor */
    D = points_to_divisor(x1, y1, x2, y2);
    free(str1);
    free(str2);

    return D;
}

string CryptoHECCg2::decode(divisor D)
{
    int maxlen = MAX_ENCODING_LEN_G2;
    UnifiedEncoding enc(p, u_param, w_param, 2);

    /* Zero buffer for error checking the ret value of find_string */
    uint8_t *zer = new uint8_t[maxlen/2+1];
    memset(zer, '0', maxlen/2+1);

    /* Convert divisor to points in the HEC */
    ZZ_p x1, y1, x2, y2;
    divisor_to_points(D, x1, y1, x2, y2);

    /* Decode each point to 2 values, find_string is used to determine which one is the correct one */
    ZZ_p val1, val2, val3, val4;
    int fl = enc.decode(val1, val2, x1, y1);
    fl = enc.decode(val3, val4, x2, y2);

    if(fl) {
        throw std::runtime_error("Could not decode points");
    }

    /* Find the correct string from the first point. A pointer to a uint8_t buffer is returned */
    /* find_string is not safe to use. Needs to be refactored to use vectors */
    uint8_t *str1 = new uint8_t[maxlen/2+1];

    str1 = find_string(val1, val2, maxlen/2+1, 1);

    if(memcmp(str1, zer, maxlen/2+1) == 0) {
        throw std::runtime_error("Could not find string");
    }

    /* Find the correct string from the second point. A pointer to a uint8_t buffer is returned */
    uint8_t *str2 = new uint8_t[maxlen/2+1];

    str2 = find_string(val3, val4, maxlen/2+1, 1);

    if(memcmp(str2, zer, maxlen/2+1) == 0) {
        throw std::runtime_error("Could not find string");
    }

    /* After finding the correct strings, each first byte is either 1 or 2 indicating the order 
    they should be placed */
    uint8_t *ret = new uint8_t[maxlen+1];
    int p1, p2;
    p1 = str1[0] - 49;
    p2 = str2[0] - 49;

    if(p1 != 1 && p1 != 2)
        throw std::runtime_error("Valid string was not found.");
    
    if(p2 != 1 && p2 != 2)
        throw std::runtime_error("Valid string was not found.");
    
    /* Build the full text and insert a null byte at the end */
    memcpy(ret+p1*maxlen/2, str1+1, maxlen/2);
    memcpy(ret+p2*maxlen/2, str2+1, maxlen/2);
    ret[maxlen] = '\0';
    string txt = (char*)ret;

    free(zer);
    free(str1);
    free(str2);

    return txt;
}


ZZ CryptoHECCg2::from_divisor_to_ZZ(const divisor& div, const ZZ& n)
{
    poly_t u = div.get_upoly();
    ZZ temp = AddMod(sqr(rep(u.rep[0])), sqr(rep(u.rep[1])), n);
    return ( IsZero(temp) ? to_ZZ(1) : temp );
}

void CryptoHECCg2::serialize(divisor D, vector<unsigned char> &buff)
{
    serialize_generic(D, buff, this->curve, this->p);
}

divisor CryptoHECCg2::deserialize(vector<unsigned char> buff)
{
    return deserialize_generic(buff, this->curve, this->p);
}

string CryptoHECCg2::sign(ZZ priv, vector<unsigned char> mess)
{
    /* Save the NTL field context, because arithmetic here is done on a different field */
    NTL::ZZ_pContext context;
    context.save();

    /* Calculate hash digest */
    hash.Update(mess.data(), mess.size());
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);
    
    /* Convert hash digest to ZZ big integer of 28 bytes (224 bits) */
    ZZ m; 
    m = ZZFromBytes((unsigned char*)digest.data(), 28);

    /* Initialize NTL field */
    SetSeed(to_ZZ(1234567890));
    ZZ p = to_ZZ(pg2); 
    field_t::init(p); 
    ZZ order = to_ZZ(Ng2);

    int order_size = NTL::NumBytes(order);

    // Private key x, random number k
    ZZ x, k; 

    // f(a), bijection of divisor a to ZZ
    ZZ f_a;

    // The signature curve
    g2hcurve crv;

    // The divisor a of the signature, the base element g, the public key h
    divisor a, g, h;

    // Construct the f polyonym of the curve
    poly_t f;

    NTL::SetCoeff(f, 5, 1);
    NTL::SetCoeff(f, 4, 0);
    NTL::SetCoeff(f, 3, str_to_ZZ_p(f3g2));
    NTL::SetCoeff(f, 2, str_to_ZZ_p(f2g2));
    NTL::SetCoeff(f, 1, str_to_ZZ_p(f1g2));
    NTL::SetCoeff(f, 0, str_to_ZZ_p(f0g2));

    /* Construct the curve */
    crv.set_f(f);
    crv.update();

    /* Construct the base element */
    g.set_curve(crv);
    poly_t gu, gv;
    NTL::SetCoeff(gu, 2, 1);
    NTL::SetCoeff(gu, 1, str_to_ZZ_p(gu1g2));
    NTL::SetCoeff(gu, 0, str_to_ZZ_p(gu0g2));
    NTL::SetCoeff(gv, 1, str_to_ZZ_p(gv1g2));
    NTL::SetCoeff(gv, 0, str_to_ZZ_p(gv0g2));
    g.set_upoly(gu);
    g.set_vpoly(gv);
    g.update();
    
    /* The private key */
    x = to_ZZ(priv_g2);

    /* The public key */
    h = x * g;

    /* Random number k */
    do {
        RandomBnd(k, order);
    } while (IsZero(k));

    /* Divisor a of the signature */
    a = k * g;

    /* f(a) */
    f_a = from_divisor_to_ZZ(a, order);

    /* ZZ b = (m - x*f(a))/k mod N of the signature */
    ZZ b = ((m - x*f_a)*InvMod(k, order))%order;

    /* Check if the signature is correctly generated */
    if ( f_a * h + b * a == m * g ) {
        vector<unsigned char> sig;
        serialize_generic(a, sig, crv, p);

        byte bsig[order_size];
        BytesFromZZ(bsig, b, order_size);
        sig.insert(sig.end(), bsig, bsig+order_size);

        string ret((char*)sig.data(), sig.size());

        context.restore();
        return ret;
    }
    else {
        throw std::runtime_error("Could not create signature on message");
    }
}

bool CryptoHECCg2::verify(string sig, divisor Pk, vector<unsigned char> mess)
{
    /* Save the NTL field context, because arithmetic here is done on a different field */
    NTL::ZZ_pContext context;
    context.save();

    /* Calculate hash digest */
    hash.Update(mess.data(), mess.size());
    std::string digest;
    digest.resize(hash.DigestSize());
    hash.Final((byte*)&digest[0]);
    
    /* Convert hash digest to ZZ big integer of 28 bytes (224 bits) */
    ZZ m; 
    m = ZZFromBytes((unsigned char*)digest.data(), 28);

    /* Initialize NTL field */
    SetSeed(to_ZZ(1234567890));
    ZZ p = to_ZZ(pg2); 
    field_t::init(p); 
    ZZ order = to_ZZ(Ng2);

    int order_size = NTL::NumBytes(order);

    // Private key x, random number k
    ZZ x, k; 

    // f(a), bijection of divisor a to ZZ
    ZZ f_a;

    // The signature curve
    g2hcurve crv;

    // The divisor a of the signature, the base element g, the public key h
    divisor a, g, h;

    // Construct the f polyonym of the curve
    poly_t f;

    NTL::SetCoeff(f, 5, 1);
    NTL::SetCoeff(f, 4, 0);
    NTL::SetCoeff(f, 3, str_to_ZZ_p(f3g2));
    NTL::SetCoeff(f, 2, str_to_ZZ_p(f2g2));
    NTL::SetCoeff(f, 1, str_to_ZZ_p(f1g2));
    NTL::SetCoeff(f, 0, str_to_ZZ_p(f0g2));

    /* Construct the curve */
    crv.set_f(f);
    crv.update();

    /* Construct the base element */
    g.set_curve(crv);
    poly_t gu, gv;
    NTL::SetCoeff(gu, 2, 1);
    NTL::SetCoeff(gu, 1, str_to_ZZ_p(gu1g2));
    NTL::SetCoeff(gu, 0, str_to_ZZ_p(gu0g2));
    NTL::SetCoeff(gv, 1, str_to_ZZ_p(gv1g2));
    NTL::SetCoeff(gv, 0, str_to_ZZ_p(gv0g2));
    g.set_upoly(gu);
    g.set_vpoly(gv);
    g.update();

    /* Extract a divisor and b big integer of the signature */
    vector<unsigned char> siga(sig.begin(), sig.end()-order_size);
    byte sigb[order_size];
    memcpy(sigb, sig.data()+sig.size()-order_size, order_size);

    a = deserialize_generic(siga, crv, p);
    ZZ b = ZZFromBytes(sigb, order_size);

    /* Reconstruct the public key from the private key for simulation purposes */
    x = to_ZZ(priv_g2);
    h = x*g;

    f_a = from_divisor_to_ZZ(a, order);

    if ( f_a * h + b * a == m * g ) {
        context.restore();
        return true;
    }
    else {
        cout << "ElGamal signature verification did not succeed!" << endl;
        context.restore();
        return false;
    }
}

tuple<ZZ, divisor> CryptoHECCg2::generate_cert_get_keypair(vector<unsigned char> &gen_cert, string uname)
{
    /* Generate a random keypair */
    ZZ x;
    RandomBnd(x, p*p);

    divisor h = x*base;

    gen_cert = cert.cert_generate(uname, h);

    divisor pub = cert.cert_pk_extraction(gen_cert);
    ZZ priv = cert.cert_reception(gen_cert, x);

    return tuple<ZZ, divisor>(priv, pub);
}

divisor CryptoHECCg2::extract_public(vector<unsigned char> rec_cert)
{
    return cert.cert_pk_extraction(rec_cert);
}

