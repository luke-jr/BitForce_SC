/*
 * SHA256_Engine.h
 *
 * Created: 10/01/2013 21:07:00
 *  Author: NASSER GHOSEIRI
 * Company: Butterfly Labs
 */ 


#ifndef SHA256_ENGINE_H_
#define SHA256_ENGINE_H_

int  SHA256_Digest(char* szData, int iDataLen, int* retValsAtoH);
void SHA2_Core(unsigned int  inA, 
			   unsigned int  inB, 
			   unsigned int  inC, 
			   unsigned int  inD, 
			   unsigned int  inE, 
			   unsigned int  inF, 
			   unsigned int  inG, 
			   unsigned int  inH,
			   unsigned int* outA, 
			   unsigned int* outB, 
			   unsigned int* outC, 
			   unsigned int* outD, 
			   unsigned int* outE, 
			   unsigned int* outF, 
			   unsigned int* outG, 
			   unsigned int* outH);
			   

#endif /* SHA256_ENGINE_H_ */