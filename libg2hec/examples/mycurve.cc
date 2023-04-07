/* Example: local ElGamal encryption */
/* Note that because we have a static g2hcurve member s_hcurve in the
divisor class and this memeber may result in chaos when we swith moduli, 
we require that there is only one modulus per process
*/

#include <g2hec_nsfieldtype.h>
#include <assert.h>
#include <g2hec_Genus2_ops.h>

#undef MAX_STRING_LEN 
#define MAX_STRING_LEN 300

NS_G2_CLIENT

int main() 
{
  /* Set PRNG seed */
  SetSeed(to_ZZ(19800729));

  char p[MAX_STRING_LEN];

  cout << "Please choose your modulus p (up to " 
       << MAX_STRING_LEN << " decimal digits):" << endl;
  cout << "p = ";
  cin.getline(p, MAX_STRING_LEN);

  ZZ pZZ = to_ZZ(p);

  field_t::init(pZZ); // define GF(p)

  ZZ x, k;

  g2hcurve curve;

  divisor m, g, h, a, b;
  poly_t hx;
  NTL::SetCoeff(hx, 0, 0);
  NTL::SetCoeff(hx, 1, 1);
  NTL::SetCoeff(hx, 2, 12);

  poly_t fx;
  NTL::SetCoeff(fx, 0, 7);
  NTL::SetCoeff(fx, 1, 7);
  NTL::SetCoeff(fx, 2, 8);
  NTL::SetCoeff(fx, 3, 12);
  NTL::SetCoeff(fx, 4, 6);
  NTL::SetCoeff(fx, 5, 1);
  curve.set_f(fx);
  curve.set_h(hx);
  curve.update();
  if(!curve.is_valid_curve()) {
    std::cout << "Not a valid curve." << std::endl;
    exit(1);
  }

   /* private key x */
  RandomBnd(x, pZZ*pZZ);
   /* random number k */
  RandomBnd(k, pZZ*pZZ);

  m.set_curve(curve);

   /* random message m as divisor */
  m.random();

   /* random base point */
  g.random();

   /* public key h */
  h = x * g;

   /* cipher text (a, b) */
  a = k * g;
  b = k * h + m;

  /* message decryption  */

  if ( b - x * a == m )
    cout << "ElGamal decryption succeeded!" << endl;
  else
    cout << "ElGamal decryption failed!" << endl;

   return 0;
}
