
#include "encryption_JuHai.h"

#define ENCRYPT_DELTA1		0x39BA
#define ENCRYPT_DELTA2		0x7E37
#define ENCRYPT_DELTA3		0xD6EC
#define ENCRYPT_DELTA4		0xED42

#define PS_YB_LICENSEKEY	(20)

void Cryptogram(unsigned short *in, unsigned short *out, uint16 n);

void Cryptogram(unsigned short *in, unsigned short *out, uint16 n)
{
	unsigned short	x = *(in);
	unsigned short	y = *(in+1);
	unsigned short	z = *(in+2);
	unsigned short	u =*(in+3);
	unsigned short	sum = 0;
 

	while (n != 0)
	{
		sum += ENCRYPT_DELTA1;
		x = (u<<2) + (ENCRYPT_DELTA4^x) + (sum^(u>>3));
		y = (z<<1) + (ENCRYPT_DELTA3^y) + (sum^(z>>3));
		z = (x<<3) + (ENCRYPT_DELTA1^z) + (sum^(x>>3));
		u = (y<<4) + (ENCRYPT_DELTA2^u) + (sum^(y>>3));
		n--;
	}

	*out	= x;
	*(out+1)= y;
	*(out+2)= z;
	*(out+3)= u;
    
 
}

uint16  CheckYBSecurityCode(uint16 encrypt_level, uint16 License_PSKey)
{
	uint16 psk[4];
	uint16 chk[4];
	bdaddr *bd_addr = malloc(sizeof(bdaddr));
	
	PsFullRetrieve(1, bd_addr, sizeof(bdaddr));
	
#if 1
	if(PsRetrieve(License_PSKey, psk, 4) == 0)
		return FALSE;
#endif
	Cryptogram((uint16 *)bd_addr, chk, encrypt_level);
	free(bd_addr);
	return ((chk[0] == psk[0]) && (chk[1] == psk[1]) && (chk[2] == psk[2])&&(chk[3]==psk[3]));
}
