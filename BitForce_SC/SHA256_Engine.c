/*
 * SHA256_Engine.c
 *
 * Created: 10/01/2013 21:07:14
 *  Author: NASSER GHOSEIRI
 */ 

// WARNING:
// ------------------------------------------------------------------------------------------
// In this simulation, all used variables MUST BE UNSIGNED INT. A normal INT will become negative
// after rotation (this last bit of int indicates it's positivity or negativity)
// ------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "SHA256_Engine.h"

unsigned int H0_INITIAL[8] = {
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
 };
 
// Initialize table of round constants
// (first 32 bits of the fractional parts
// of the cube roots of the first 64 primes 2..311):
unsigned int K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
 };
// The first three DWORDs are our data (12Bytes) in correct order
// The MCU Must be able to atleast set these three DWORDs.
// The optimal situation is where MCU can access the first sixteen DWORs
// (W[0] to W[15]).
// IMPORTANT: Note: This W0 is shared among ALL SLICES on the same chip
unsigned int W0[64] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000
 };

// Function headers
unsigned int Sigma0(unsigned int a);
unsigned int Maj(unsigned int a, unsigned int b, unsigned int c);
unsigned int Sigma1(unsigned int e);
unsigned int Ch(unsigned int e, unsigned int f, unsigned int g);


// Implementation
inline unsigned int Sigma0(unsigned int a){
  unsigned int r2, r13, r22;
  r2  = ((a >> 2) | (a << (32-2)));
  r13 = ((a >> 13) | (a << (32-13)));
  r22 = ((a >> 22) | (a << (32-22)));
  return (r2 ^ r13) ^ r22;
}

inline unsigned int Maj(unsigned int a, unsigned int b, unsigned int c){
  unsigned int vab, vac, vbc;
  vab = a & b;
  vbc = b & c;
  vac = a & c;
  return((vab ^ vac) ^ vbc);
}

inline unsigned int Sigma1(unsigned int e){
  unsigned int r6, r11, r25;
  r6 = ((e >> 6) | (e << (32-6)));
  r11 = ((e >> 11) | (e << (32-11)));
  r25 = ((e >> 25) | (e << (32-25)));
  return ((r6 ^ r11) ^ r25);
}

inline unsigned int Ch(unsigned int e, unsigned int f, unsigned int g){
  unsigned int vef;
  unsigned int veg;
  vef = (e & f);
  veg = ((~e) & g);
  return (vef ^ veg);
}

// --- Extend the sixteen 32-bit words into sixty-four 32-bit words:
// Note: This stage must be pipe-lined on the chip.
void expandWord(){
  unsigned int i, x2, x15;
  unsigned int s0, s1;
  unsigned int rr7, rr18, rs3; // shifts
  unsigned int rr17, rr19, rs10; // shifts
 
  unsigned int* wTarget;
 
  wTarget = W0;
  for (i = 16; i < 64; i++)
  {
   x2  = wTarget[i-2];
   x15 = wTarget[i-15];
   rr7  = (x15 >> 7) | (x15 << (32-7));
   rr18 = (x15 >> 18) | (x15 << (32-18));
   rs3  = (x15 >> 3) & 0x1fffffff;
   s0 = ((rr7 ^ rr18) ^ rs3);
   rr17 = (x2 >> 17) | (x2 << (32-17));
   rr19 = (x2 >> 19) | (x2 << (32-19));
   rs10 = (x2 >> 10) & 0x003fffff;
   s1 = ((rr17 ^ rr19) ^ rs10);
   wTarget[i] = wTarget[i-16] + s0 + wTarget[i-7] + s1;
  }
}

/* ----------------------------------------------------------- */
void SHA2_Core(unsigned int inA,   unsigned int inB,   unsigned int inC,   unsigned int inD,   unsigned int inE,   unsigned int inF,   unsigned int inG,   unsigned int inH,
			   unsigned int* outA, unsigned int* outB, unsigned int* outC, unsigned int* outD, unsigned int* outE, unsigned int* outF, unsigned int* outG, unsigned int* outH)
{
  unsigned int i;
  unsigned int maj, ch, s0, s1, t1, t2;
  unsigned int a, b, c, d, e, f, g, h;
  *outA = a = inA;
  *outB = b = inB;
  *outC = c = inC;
  *outD = d = inD;
  *outE = e = inE;
  *outF = f = inF;
  *outG = g = inG;
  *outH = h = inH;
 
  for(i = 0; i < 64; i++)
  {
	s0  = Sigma0(a);
	maj = Maj(a,b,c);
	t2  = s0 + maj;
	s1  = Sigma1(e);
	ch  = Ch(e,f,g);

	t1 = h + s1 + ch + K[i] + W0[i];
	
	h = g;
	g = f;
	f = e;
	e = d + t1;
	d = c;
	c = b;
	b = a;
	a = t1 + t2;
  }

  // NOTICE: OutA = InA + A (which is the initially given A value to the function) + a (the calculated a)
  // This '+=' before was only '=' and *outA was not initialized to then given 'InA'
  // Note: This addition happens at the end of the 64th round. It must be applied to BOTH ENGINES!
  *outA += a;
  *outB += b;
  *outC += c;
  *outD += d;
  *outE += e;
  *outF += f;
  *outG += g;
  *outH += h;

}

/* ----------------------------------------------------------------------- */
/*
  h0 := h0 + a
  h1 := h1 + b
  h2 := h2 + c
  h3 := h3 + d
  h4 := h4 + e
  h5 := h5 + f
  h6 := h6 + g
  h7 := h7 + h
Produce the final hash value (big-endian):
hash = {h0, h1 , h2 , h3 , h4 , h5 , h6 , h7}
*/
/* ----------------------------------------------------------- */
/* Examples:
 SHA256("")
 0x e3b0c442 98fc1c14 9afbf4c8 996fb924 27ae41e4 649b934c a495991b 7852b855
SHA256("The quick brown fox jumps over the lazy dog")
0x d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592
SHA256("The quick brown fox jumps over the lazy dog.")
0x ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c
*/
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
int SHA256_Digest(char* szData, int iDataLen, int* retValsAtoH)
{
	
  // Proceed
  char counter_string[16];
  unsigned int i, j, k;
  unsigned int a, b, c, d, e, f, g, h;
  a = b = c = d = e = f = g = h = 0;
  /* setting the inputs for the first engine */
  // NOTE: H0 values are declared on top, and in our Chip, it will be provided by the MCU
  // Apply the expansion 
  
  // We have to create an extention here. a 64Byte array
  char sha_extention[64];
  for (unsigned int v = 0; v < 64; v++) sha_extention[v] = 0;
  
  // The first bit after data is 1
  sha_extention[0] = 0x080;
  
  // Ok, we have to tailor the extention here
  // First count the number of zeros we have to add. This depends on iLen modulus 64 (512Bit chunks)
  int iTotalBitsThatShouldBe = ((iDataLen * 8) + 1 + 64) ;
  int iTotalZerosToAdd = 512 - (iTotalBitsThatShouldBe % 512);
  int iStartPositionFor64BitSize = (iTotalZerosToAdd + 1) >> 3;
  int iFinalByteSize = (iTotalBitsThatShouldBe + iTotalZerosToAdd) >> 3;
  
  // Put the 64Bit bit-size of the stream in the sha_extention
  unsigned int iBitSize = (iDataLen << 3) & 0x0FFFFFFFF; // DataLen * 8
  sha_extention[iStartPositionFor64BitSize+0] = 0; // High-bytes are zero, as we are in a big-endian 64bit integer
  sha_extention[iStartPositionFor64BitSize+1] = 0; // "
  sha_extention[iStartPositionFor64BitSize+2] = 0; // "
  sha_extention[iStartPositionFor64BitSize+3] = 0; // "
  sha_extention[iStartPositionFor64BitSize+4] = (iBitSize >> 24) & 0x0FF;
  sha_extention[iStartPositionFor64BitSize+5] = (iBitSize >> 16) & 0x0FF;
  sha_extention[iStartPositionFor64BitSize+6] = (iBitSize >> 8)  & 0x0FF;
  sha_extention[iStartPositionFor64BitSize+7] = (iBitSize)       & 0x0FF;
  
  
  // Proceed with calculation
  unsigned int H_VALS[8];
  H_VALS[0] = H0_INITIAL[0]; H_VALS[1] = H0_INITIAL[1]; H_VALS[2] = H0_INITIAL[2]; H_VALS[3] = H0_INITIAL[3]; 
  H_VALS[4] = H0_INITIAL[4]; H_VALS[5] = H0_INITIAL[5]; H_VALS[6] = H0_INITIAL[6]; H_VALS[7] = H0_INITIAL[7]; 
  
  // Now we put the total size of the bytes
  for (unsigned int umx = 0; umx < iFinalByteSize; umx += 64)
  {
	  for (unsigned int byte_pos = umx; byte_pos < (umx + 64) && (umx < iDataLen); byte_pos += 4)	  
	  {
		  // Ok What we do here is we put the next 64 bytes in W0
		  // and then we expand the words
		  unsigned int iGenerateWord = 0x0FFFFFFFF;
		  iGenerateWord =  ((((byte_pos + 0 > iDataLen - 1) ? (sha_extention[byte_pos + 0 - iDataLen]) : szData[byte_pos + 0]) << 24) & 0x0FF000000);
		  iGenerateWord |= ((((byte_pos + 1 > iDataLen - 1) ? (sha_extention[byte_pos + 1 - iDataLen]) : szData[byte_pos + 1]) << 16) & 0x000FF0000);
		  iGenerateWord |= ((((byte_pos + 2 > iDataLen - 1) ? (sha_extention[byte_pos + 2 - iDataLen]) : szData[byte_pos + 2]) << 8 ) & 0x00000FF00);
		  iGenerateWord |= ((((byte_pos + 3 > iDataLen - 1) ? (sha_extention[byte_pos + 3 - iDataLen]) : szData[byte_pos + 3]))       & 0x0000000FF);
		  
		  // Set the word
		  W0[((byte_pos - umx) >> 2)] = iGenerateWord;
	  }		
	  
	  // Save some values
	  a = H_VALS[0]; b = H_VALS[1]; c = H_VALS[2]; d = H_VALS[3]; e = H_VALS[4]; f = H_VALS[5]; g = H_VALS[6]; h = H_VALS[7]; 
	  
	  // Perform SHA
	  expandWord();
	  SHA2_Core(a,b,c,d,e,f,g,h,
	    		&H_VALS[0], &H_VALS[1], &H_VALS[2], &H_VALS[3], &H_VALS[4], &H_VALS[5], &H_VALS[6], &H_VALS[7]);  
  }
  
  
  retValsAtoH[0] = H_VALS[0];
  retValsAtoH[1] = H_VALS[1];
  retValsAtoH[2] = H_VALS[2];
  retValsAtoH[3] = H_VALS[3];
  retValsAtoH[4] = H_VALS[4];
  retValsAtoH[5] = H_VALS[5];
  retValsAtoH[6] = H_VALS[6];
  retValsAtoH[7] = H_VALS[7];
  
  return 0;
}