#include "ch32fun.h"
#include <stdio.h>

// Physical configuration:
//  
// PD3/PD4 -> Differential output.
// 
// PD3 - 100uF - 220 Ohm -+-- Output +
//                        |
// PD4 -+- 220nF ---+-----+-- Output -
//      |           |
//      +- 100 Ohm -+

#define F_SPS (48000000/256/2) // Confirmed 256.
#define AUDIO_BUFFER_SIZE 2048
#define WARNING(x...)

#define BADATA_DECORATOR const __attribute__((section(".fixedflash")))
#define BAS_DECORATOR    const __attribute__((section(".fixedflash")))
#define SECTION_DECORATOR __attribute__((section(".fixedflash")))

#include "ba_play.h"
#include "ba_play_audio.h"

volatile uint8_t out_buffer_data[AUDIO_BUFFER_SIZE];
ba_play_context ctx;

volatile uint32_t kas = 0;

////////////////////////////////////////////////////////////////////////////////////////////


#define SSD1306_CUSTOM_INIT_ARRAY 1
#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_FULLUSE
#define SSD1306_W (64) 
#define SSD1306_H (64)
#define SSD1306_OFFSET 32

#define SSD1306_RST_PIN PC0
#define SSD1306_I2C_BITBANG_SDA PC1
#define SSD1306_I2C_BITBANG_SCL PC2

#define I2CDELAY_FUNC( x )
// asm volatile( "nop\nnop\n");

#include "ssd1306mini.h"

const uint8_t ssd1306_init_array[] =
{
	0xAE, // Display off
	0x20, 0x00, // Horizontal addresing mode
	0x00, 0x12, 0x40, 0xB0,
	0xD5, 0xf0, // Function Selection   <<< This controls scan speed. F0 is fastest.  The LSN = D divisor.
	0xA8, 0x2F, // Set Multiplex Ratio
	0xD3, 0x00, // Set Display Offset
	0x40,
	0xA1, // Segment remap
	0xC8, // Set COM output scan direction
	0xDA, 0x12, // Set COM pins hardware configuration
	0x81, 0xcf, // Contrast control
	0xD9, 0x22, // Set Pre-Charge Period  (Not used)
	0xDB, 0x30, // Set VCOMH Deselect Level
	0xA4, // Entire display on (a5)/off(a4)
	0xA6, // Normal (a6)/inverse (a7)
	0x8D, 0x14, // Set Charge Pump
	0xAF, // Display On
	SSD1306_PAGEADDR, 0, 7, // Page setup, start at 0 and end at 7
};

// ??? No idea but if I don't put this in the main flash partition everything dies.
const uint8_t OledBufferReset[] SECTION_DECORATOR = 
	{0xD3, 0x32, 0xA8, 0x01,
	// Column start address (0 = reset)
	// Column end address (127 = reset)
	SSD1306_COLUMNADDR, SSD1306_OFFSET, SSD1306_OFFSET+SSD1306_W-1, 0xb0 };


//uint8_t ssd1306_buffer[64];

////////////////////////////////////////////////////////////////////////////////////////////

#if 0
void EmitEdge( uint32_t gou, uint32_t got, uint32_t gon, uint32_t subframe )
{
	int gouah = got | gou;
	int goual = got ^ gou; //Half-adder
	int gonah = got | gon;
	int gonal = got ^ gon; //Half-adder

	if( (subframe)&1 )
	{
		got>>= 8;
		gou>>= 8;
	}

	int go = 0;
	ssd1306_mini_i2c_sendbyte( go );
}
#endif

uint16_t pixelmap[64*6];

//pixelmap[64*6];
int pmp;
// From ttablegen.c

static inline void PMEmit( uint16_t pvo )
{

	// If we only cared about left/right blurring, we could just say 
	//	pixelmap[pmp++] = pvo; ... but we don't.
	pixelmap[pmp++] = pvo;
}

void EmitEdge( graphictype tgprev, graphictype tg, graphictype tgnext )
{
	// This should only need +2 regs (or 3 depending on how the optimizer slices it)
	// (so all should fit in working reg space)
	graphictype A = tgprev >> 8;
	graphictype B = tgprev;      // implied & 0xff
	graphictype C = tgnext >> 8;
	graphictype D = tgnext;      // implied & 0xff

//	graphictype E = (B&C)|(A&D); // 8 bits worth of MSB of (next+prev+1)/2
//	graphictype F = D|B;         // 8 bits worth of LSB of (next+prev+1)/2
	graphictype E = tg >> 8;
	graphictype F = tg;          // implied & 0xff

/*	graphictype G = tg >> 8;
	graphictype H = tg;          // implied & 0xff

	//if( subframe )
	int tghi = (F&G)|(E&H);     // 8 bits worth of MSB of this+(next+prev+1)/2-1
	//else
	int tglo = G|E|(F&H);       // 8 bits worth of MSB|LSB of this+(next+prev+1)/2-1
*/
	int tghi = (D&E)|(B&E)|(B&C&F)|(A&D&F);     // 8 bits worth of MSBs
	int tglo = E|C|A|(D&F)|(B&F)|(B&D);       // 8 bits worth of LSBs

	//ssd1306_mini_i2c_sendbyte( tg );
	PMEmit( (tghi << 8) | tglo );
}


void UpdatePixelMap()
{
	int y;
	glyphtype * gm = ctx.curmap;
	pmp = 0;
	for( y = 0; y < 6; y++ )
	{
		int x;
		for( x = 0; x < 8; x++ )
		{
			glyphtype gindex = *(gm);
			graphictype * g     = ctx.glyphdata[gindex];
			graphictype * gprev = ctx.glyphdata[gm[-1]];
			graphictype * gnext = ctx.glyphdata[gm[ 1]];
			gm++;
			
			//int go = (subframe & 1)?0xff:0x00;
			int lg;
			{
				uint32_t got = g[0];
				uint32_t gon = g[1];
				uint32_t gou = (x>0)?gprev[7]:gon;

				EmitEdge( gou, got, gon );
			}
			for( lg = 1; lg < 7; lg ++ )
			{
				int go = g[lg];

				// Bits in this word scan left-to-right.
				// LG is "x" from left-to-right
				//if( (subframe)&1 )
				//	go >>= 8;
				//ssd1306_mini_i2c_sendbyte( go );
				PMEmit( go );
			}
			{
				uint32_t got = g[7];
				uint32_t gou = g[6];
				uint32_t gon = (x<7)?gnext[0]:gou;
				EmitEdge( gou, got, gon );
			}
		}
	}
	
}

int main()
{
	// This is normally in SystemInit() but pulling out here to keep things tight.
	FLASH->ACTLR = FLASH_ACTLR_LATENCY_2;  // Can't run flash at full speed.

	#define RCC_CSS 0
	#define HSEBYP 0
	#define BASE_CTLR	(((FUNCONF_HSITRIM) << 3) | RCC_HSION | HSEBYP | RCC_CSS)
	RCC->CTLR = BASE_CTLR | RCC_HSION | RCC_HSEON | RCC_PLLON;  // Turn on HSE + PLL
	while((RCC->CTLR & RCC_PLLRDY) == 0);                       // Wait till PLL is ready
	RCC->CFGR0 = RCC_PLLSRC_HSE_Mul2 | RCC_SW_PLL | RCC_HPRE_DIV1; // Select PLL as system clock source

	RCC->CTLR = BASE_CTLR | RCC_HSEON | RCC_PLLON; // Turn off HSI (optional)

	// By assigning, instead of |='ing it uses less code.
	RCC->APB1PCENR = RCC_APB1Periph_TIM2;
	RCC->APB2PCENR = ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD ) | RCC_APB2Periph_TIM1;
	RCC->AHBPCENR = RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_SRAM;

	funGpioInitAll();

	// PC0,1,2 = GPIO_CFGLR_OUT_PP, 3,4=GPIO_CFGLR_OUT_AF_PP
	GPIOC->CFGLR = 0xbb111;

	funDigitalWrite( SSD1306_I2C_BITBANG_SDA, 1 );
	funDigitalWrite( SSD1306_I2C_BITBANG_SCL, 1 );
	funDigitalWrite( SSD1306_RST_PIN, 0 );
//	Delay_Ms(10);
	funDigitalWrite( SSD1306_RST_PIN, 1 );
	Delay_Us(10);

	// Trying another mode
	ssd1306_mini_pkt_send( ssd1306_init_array, sizeof(ssd1306_init_array), 1 );

	DMA1_Channel2->CFGR = 0;

	ba_play_setup( &ctx );
	ba_audio_setup();

	int frame = 0;

	int32_t nextFrame = SysTick->CNT;

	int subframe = 0;

	// Setup PD3/PD4 as TIM2_CH1/TIM2_CH2 as output
	// TIM2_RM=111 (Full)

	AFIO->PCFR1 = AFIO_PCFR1_TIM2_RM_0 | AFIO_PCFR1_TIM2_RM_1 | AFIO_PCFR1_TIM2_RM_2;

	TIM2->PSC = 0x0001;
	TIM2->ATRLR = 255; // Confirmed: 255 here = PWM period of 256.

	// for channel 1 and 2, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
	// enabling preload causes the new pulse width in compare capture register only to come into effect when UG bit in SWEVGR is set (= initiate update) (auto-clears)
	TIM2->CHCTLR1 = 
		TIM2_CHCTLR1_OC1M_2 | TIM2_CHCTLR1_OC1M_1 | TIM2_CHCTLR1_OC1PE |
		TIM2_CHCTLR1_OC2M_2 | TIM2_CHCTLR1_OC2M_1 | TIM2_CHCTLR1_OC2PE;

	// Enable Channel outputs, set default state (based on TIM2_DEFAULT)
	TIM2->CCER = TIM2_CCER_CC1E // | (TIM_CC1P & TIM2_DEFAULT);
	           | TIM2_CCER_CC2E;// | (TIM_CC2P & TIM2_DEFAULT);

	// initialize counter
	TIM2->SWEVGR = TIM2_SWEVGR_UG | TIM2_SWEVGR_TG;
	TIM2->DMAINTENR = TIM1_DMAINTENR_TDE | TIM1_DMAINTENR_UDE;

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM2->CTLR1 = TIM2_CTLR1_ARPE | TIM2_CTLR1_CEN;

	// Enable TIM2

	TIM2->CH1CVR = 128;
	TIM2->CH2CVR = 128; 

	// Weeeird... PUPD works better than GPIO_CFGLR_OUT_AF_PP
	//funPinMode( PD3, GPIO_CFGLR_OUT_AF_PP );
	//funPinMode( PD4, GPIO_CFGLR_OUT_AF_PP );

	TIM1->CHCTLR2 = 
		TIM1_CHCTLR2_OC3M_2 | TIM1_CHCTLR2_OC3M_1 | TIM1_CHCTLR2_OC3PE |
		TIM1_CHCTLR2_OC4M_2 | TIM1_CHCTLR2_OC4M_1 | TIM1_CHCTLR2_OC4PE;

	// Enable TIM1 outputs
	TIM1->BDTR = TIM1_BDTR_MOE;

	TIM1->PSC = 0x0000;
	TIM1->ATRLR = 31;

	// Enable Channel outputs, set default state (based on TIM2_DEFAULT)
	TIM1->CCER = TIM1_CCER_CC3E
	           | TIM1_CCER_CC4E;

	// initialize counter
	TIM1->SWEVGR = TIM2_SWEVGR_UG;

	TIM1->CTLR1 = TIM2_CTLR1_CEN;

	// Enable TIM2

	TIM1->CH3CVR = 16;
	TIM1->CH4CVR = 16;

	funDigitalWrite( PD7, 1 );
	// PD6 = Profiling (Debug pin)
	// PD7 = Red LED lights
	// PD3, PD4 = GPIO_CFGLR_OUT_AF_PP (Sound)
	GPIOD->CFGLR = 0x110bb444;

	while(1)
	{
		//PrintHex( DMA1_Channel2->CNTR );
		//TIM2->CH1CVR = subframe * 5;
		//out_buffer_data[frame&(AUDIO_BUFFER_SIZE-1)] = (frame&0x15)+128;

		if( subframe == 4 )
		{
			if( frame == FRAMECT ) asm volatile( "j 0" );
			if( frame == START_AUDIO_AT_FRAME )
			{

				ba_audio_fill_buffer( out_buffer_data, sizeof(out_buffer_data)-1 );

				// Start playing music at frame 37.
				// Triggered off TIM2UP
				DMA1_Channel2->CNTR = AUDIO_BUFFER_SIZE;
				DMA1_Channel2->MADDR = (uint32_t)out_buffer_data;
				DMA1_Channel2->PADDR = (uint32_t)&TIM2->CH2CVR; // This is the output register for out buffer.
				DMA1_Channel2->CFGR = 
					DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
					DMA_CFGR1_PL_0 |                     // Med priority.
					0 |                                  // 8-bit memory
					DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral  XXX TRICKY XXX You MUST do this when writing to a timer.
					DMA_CFGR1_MINC |                     // Increase memory.
					DMA_CFGR1_CIRC |                     // Circular mode.
					DMA_CFGR1_EN;                        // Enable
			}
			ba_play_frame( &ctx );
			subframe = 0;
			if( frame == 441 ) funDigitalWrite( PD7, 0 );
			if( frame == 459 ) funDigitalWrite( PD7, 1 );
			frame++;
		}
		else if( subframe & 1 )
		{
//			if( frame < 2 ) // Stop on frame 1.
				UpdatePixelMap();
		}

		funDigitalWrite( PD6, 1 ); // For time monitoring
		while( (int32_t)(SysTick->CNT - nextFrame) < 0 ) { }
		funDigitalWrite( PD6, 0 ); // For time monitoring

		nextFrame += 400000; 
		// 1600000 is 30Hz
		// 800000 is 60Hz
		// 533333 is 90Hz -- 90Hz seems about the max you can go with default 0xd5 settings.
		// 400000 is 120Hz -- Only possible when cranking D5 at 0xF0 and 0xa8 == 0x31

		// Move the cursor "off screen"
		// Scan over two scanlines to hide updates

		ssd1306_mini_pkt_send( OledBufferReset, sizeof(OledBufferReset), 1 );

		// Send data
		int y;

		ssd1306_mini_i2c_sendstart();
		ssd1306_mini_i2c_sendbyte( SSD1306_I2C_ADDR<<1 );
		ssd1306_mini_i2c_sendbyte( 0x40 ); // Data

		// New, filtered output. (Poorly documented, impenetrable)
#if 1
		int pvx;
		int pvy;

		int i;
		uint16_t * pmo = pixelmap;
		for( i = 0; i < sizeof(pixelmap)/2; i++ )
		{
			if( pvx == 64 ) { pvx = 0; pvy++; }
			int pvo = pmo[i];
			uint16_t pvr = pvo & 0x7e7e;

			int pprev, pnext, pthis;

			if( i < 64 )
				pprev = ((pvo>>8)&2) | ((pvo>>1)&1);
			else
			{
				int kpre = pmo[i-64];
				pprev = ((kpre>>14)&2) | ((kpre>>7)&1);
			}
			pnext = ((pvo>>8)&2) | ((pvo>>1)&1);
			pthis = (((pvo>>7)&2) | ((pvo>>0)&1))<<1;

			int pol = (potable[pnext+pprev*4]>>pthis)&3;
			pvr |= (pol & 1) | (pol&2)<<7;

			if( i >= 256 )
				pnext = ((pvo>>13)&2) | ((pvo>>6)&1);
			else
			{
				int knext = pmo[i+64];
				pnext = ((knext>>7)&2) | (knext &1);
			}
			pprev = ((pvo>>13)&2) | ((pvo>>6)&1);
			pthis = (((pvo>>14)&2) | ((pvo>>7)&1))<<1;

			pol = (potable[pnext+pprev*4]>>pthis)&3;
			pvr |= (pol & 1)<<7 | (pol&2)<<14;

			//pixelbase[pmp] = pvo;
			uint16_t po = pvr;

			if( subframe != 1 ) // Set this to &1 for 50/50 grey.
				ssd1306_mini_i2c_sendbyte( po>>8 );
			else
				ssd1306_mini_i2c_sendbyte( po );
		}
#else
		// No horizontal filters.
		glyphtype * gm = ctx.curmap;
		// 2.59ms
		for( y = 0; y < 6; y++ )
		{
			int x;
			for( x = 0; x < 8; x++ )
			{
				glyphtype gindex = *(gm++);
				graphictype * g = ctx.glyphdata[gindex];
				
				//int go = (subframe & 1)?0xff:0x00;
				int lg;
				for( lg = 0; lg< 8; lg ++ )
				{
					int go = g[lg];
					if( (subframe)&1 )
						go >>= 8;
					ssd1306_mini_i2c_sendbyte( go );
				}
			}
			//memset( ssd1306_buffer, n, sizeof( ssd1306_buffer ) );
			//int k;
			//for( k = 0; k < sizeof(ssd1306_buffer); k++ ) ssd1306_buffer[k] = frame;
			//ssd1306_mini_data(ssd1306_buffer, sizeof(ssd1306_buffer));
		}
#endif

		ssd1306_mini_i2c_sendstop();

		// Make it so it "would be" off screen but only by 2 pixels.
		// Overscan screen by 2 pixels, but release from 2-scanline mode.
		ssd1306_mini_pkt_send( (const uint8_t[]){0xD3, 0x3e, 0xA8, 0x31}, 4, 1 ); 

		int v = AUDIO_BUFFER_SIZE - DMA1_Channel2->CNTR - 1;
		if( v < 0 ) v = 0;

		ba_audio_fill_buffer( out_buffer_data, v );

		subframe++;
	}
}



void handle_reset( void )
{
	asm volatile( "\n\
.option push\n\
.option norelax\n\
	la gp, __global_pointer$\n\
.option pop\n\
	la sp, _eusrstack\n"
".option arch, +zicsr\n"
	// Setup the interrupt vector, processor status and INTSYSCR.

"	li a0, 0x1880\n\
	csrw mstatus, a0\n"
	: : : "a0", "a3", "memory");

	// Careful: Use registers to prevent overwriting of self-data.
	// This clears out BSS.
asm volatile(
"	la a0, _sbss\n\
	la a1, _ebss\n\
	li a2, 0\n\
	bge a0, a1, 2f\n\
1:	sw a2, 0(a0)\n\
	addi a0, a0, 4\n\
	blt a0, a1, 1b\n\
2:"
#if 0
	// This loads DATA from FLASH to RAM ---- BUT --- We don't use the .data section here.
"	la a0, _data_lma\n\
	la a1, _data_vma\n\
	la a2, _edata\n\
1:	beq a1, a2, 2f\n\
	lw a3, 0(a0)\n\
	sw a3, 0(a1)\n\
	addi a0, a0, 4\n\
	addi a1, a1, 4\n\
	bne a1, a2, 1b\n\
2:\n"
#endif
: : : "a0", "a1", "a2", "a3", "memory"
);

#if defined( FUNCONF_SYSTICK_USE_HCLK ) && FUNCONF_SYSTICK_USE_HCLK
	SysTick->CTLR = 5;
#else
	SysTick->CTLR = 1;
#endif

	asm volatile( "j main" );
	//main();
	// No interrupts, so no need to mret into main.

	// set mepc to be main as the root app.
//asm volatile(
//"	csrw mepc, %[main]\n"
//"	mret\n" : : [main]"r"(main) );
}

