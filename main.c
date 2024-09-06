#define DEFINE_INLINE
#define SYS_CLOCK_24_576

#include "iodefine.h"
#include "sci1.h"

#include "frser.h"

#include <stdlib.h>

unsigned char sci_txbuf[128];
unsigned char sci_rxbuf[128];

int check_memory(void)
{
	volatile unsigned int rsiz;
	volatile unsigned int rx,ra,wx,wa;
	
	rx=0;
	ra=0;
	wx=0;
	wa=0;

	unsigned long maddr = (MDCR.BYTE & 7) == 5 ? 0x60000 : 0x600000;
	
	for(rsiz = 0; rsiz < 131072; rsiz++)
	{
		volatile unsigned char c = rand() & 0xff;
		
		*((volatile unsigned char *) rsiz + maddr) = c;
		
		wx ^= c;
		wa += c;
	}
	
	for(rsiz = 0; rsiz < 131072; rsiz++)
	{
		volatile unsigned char c;
		
		c = *((volatile unsigned char *) rsiz + maddr);
		
		rx ^= c;
		ra += c;
	}
	
	return rx == wx && ra == wa ? 1 : 0;
}

int main(void)
{
	rs_init(br115200,sci_txbuf,128,sci_rxbuf,128);
	
	set_ccr(0);
	
	BSC.ABWCR.BIT.ABW3 = 1; // CS3 空間は８ビット幅
	BSC.ASTCR.BIT.AST3 = 1; // CS3 空間は３ステート
	BSC.WCR.BYTE = 0xF3; // プログラマブルウェイトモード、３クロック挿入
	BSC.WCER.BIT.WCE3 = 1; // CS3 空間の WSC は有効

	BSC.ABWCR.BIT.ABW2 = 1; // CS2 空間は８ビット幅
	BSC.ASTCR.BIT.AST2 = 1; // CS2 空間は３ステート
	BSC.WCER.BIT.WCE2 = 1; // CS2 空間の WSC は有効

	P1.DDR = 0xFF; // A0-A7 端子は有効
	P2.DDR = 0xFF; // A8-A15 端子は有効
	P5.DDR = 0x07; // A16-A18 端子は有効
	P8.DDR = 0x06; // CS2-CS3 端子は有効

	check_memory();
	
	setup();
	
	while (1)
	{
		loop();
	}

	return 0;
}
