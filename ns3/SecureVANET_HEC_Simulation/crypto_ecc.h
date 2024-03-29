#ifndef CRYPTO_ECC_H 
#define CRYPTO_ECC_H

#include "cryptopp/cryptlib.h"
#include "cryptopp/sha3.h"
#include "cryptopp/integer.h"
#include "cryptopp/eccrypto.h"
#include "cryptopp/osrng.h"
#include "cryptopp/oids.h"

#include "hec_cert.h"
#include "encoding.h"

#include <tuple>
#include <vector>

using namespace CryptoPP;

typedef DL_GroupParameters_EC<CryptoPP::ECP> GroupParameters;
typedef DL_GroupParameters_EC<CryptoPP::ECP>::Element Element;

/**
 * @brief Class representing the ECQV certificate structure and operations.
 * For the simulation the chosen format is: Name, Issued By, Issued On, Expires On, Public key.
 * Based on: https://www.secg.org/sec4-1.0.pdf
*/
class ECQV {
    private:
        GroupParameters group;
        Element capk, pu, qu;
        Integer capriv, ku, r, du;
        AutoSeededRandomPool prng;
        SHA3_256 hash;
        std::string name, issued_by, issued_on, expires_on;
    protected:
        void encode_to_bytes(uint8_t *buff);
    public:
        ECQV(GroupParameters group);
        vector<unsigned char> cert_generate(std::string uname, Element ru);
        Element cert_pk_extraction(vector<unsigned char> cert);
        Integer cert_reception(vector<unsigned char> cert, Integer ku);
        Element get_calculated_Qu();
        Integer get_extracted_du();
        std::string get_name();
        std::string get_issued_by();
        std::string get_issued_on();
        std::string get_expires_on();
};

/**
 * @brief Class for ECC cryptographic operations. ECQV Certificates, ECDSA signatures, ElGamal Encryption/Decription, Koblitz Encodings, Serialize
*/
class CryptoECC {
   
    private:
        GroupParameters _group;
        ECQV cert;
        AutoSeededRandomPool prng;
    public:
        /**
         * @brief Type alias used for generalization of cryptographic methods so that 
         * they can be used with an abstract class.
        */
        using element_type = Element;

        /**
         * @brief Constructor
         * @param group Group Parameters: Curve, Base Element etc.
        */
        CryptoECC(GroupParameters group) : _group(group), cert(group)
        {}

        /**
         * @brief Enrypt message using ElGamal
         * @param pub The public key.
         * @param mess The message as Element.
         * @return a tuple with encrypted message Elements a, b....
        */
        tuple<Element, Element> encrypt_ElGamal(Element pub, Element mess);

        /**
         * @brief Decrypt message using ElGamal
         * @param priv The private key
         * @param a,b The encrypted message 
         * @return The decrypted message as an Element
        */
        Element decrypt_ElGamal(Integer priv, Element a, Element b);

        /**
         * @brief Encode an Integer x to an EC Point
         * @param f The f polyonym of the EC equation: y^2 = f(x)
         * @param x The Integer to encode
         * @param k The parameter k for Koblitz encoding procedure
         * @param p The field characteristic
        */
        tuple<ZZ_p, ZZ_p> encode_koblitz(poly_t f, ZZ x, ZZ k, ZZ p);

        /**
         * @brief Convert to text to an Elliptic Curve Point using koblitz's method.
         * @param txt The text to convert, passed as string.
        */
        Element encode(string txt);

        /**
         * @brief Convert an Elliptic Curve Point to text using koblitz's method.
         * @param point The point to convert.
         * @param k The parameter k for Koblitz encoding procedure.
        */
        string decode(Element point, Integer k=1000);

        /**
         * @brief Produce ECDSA signature.
         * @param priv The private key.
         * @param mess The message to sign as a vector of unsigned chars.
         * @return The signature as a string.
        */
        string sign(Integer priv, vector<unsigned char> mess);

        /**
         * @brief Verify ECDSA signature.
         * @param sig The signature as a string.
         * @param Pk The public key.
         * @param mess The message to verify as a vector of unsigned chars.
         * @return True if the verification succeeds, false otherwise.
        */
        bool verify(string sig, Element Pk, vector<unsigned char> mess);

        /**
         * @brief Wrapper for generating a new certificate and obtaining the key-pair.
         * @param cert The certificate that is generated as a vector of unsigned chars.
         * @param uname The name to include to the certificate. Must be 7 characters exactly.
         * @return A tuple containing the private and public keys.
        */
        tuple<Integer, Element> generate_cert_get_keypair(vector<unsigned char> &gen_cert, string uname);

        /**
         * @brief Wrapper for obtaining the public key of a certificate, so that the user
         * does not have to use an ECQV instance.
         * @param rec_cert The received certificate as a vector of unsigned chars.
         * @return The extracted public key. 
        */
        Element extract_public(vector<unsigned char> rec_cert);

        /**
         * @brief Serialize a point. Bytes are pushed back on buff vector.
         * @param point The point to serialize.
         * @param buff The vector to push the serialized point.
        */
        void serialize(Element point, vector<unsigned char> &buff);

        /**
         * @brief Deserialize a point from bytes.
         * @param buff The vector that contains the point in serialized form.
        */
        Element deserialize(vector<unsigned char> buff);

};

#endif