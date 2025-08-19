#ifndef _SSD1306_6448_MINI_H
#define _SSD1306_6448_MINI_H


#ifndef SSD1306_COLUMNADDR
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#endif

// SSD1306 I2C address
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x3c
#endif

#ifndef SSD1306_I2C_BITBANG_SDA
#define SSD1306_I2C_BITBANG_SDA PC1
#endif

#ifndef SSD1306_I2C_BITBANG_SCL
#define SSD1306_I2C_BITBANG_SCL PC2
#endif

#if 0
static inline void ssd1306_mini_i2c_setup(void)
{
	funPinMode( SSD1306_I2C_BITBANG_SDA, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( SSD1306_I2C_BITBANG_SDA, 1 );
	funPinMode( SSD1306_I2C_BITBANG_SCL, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( SSD1306_I2C_BITBANG_SCL, 1 );
	funPinMode( SSD1306_RST_PIN, GPIO_CFGLR_OUT_10Mhz_PP );
	funDigitalWrite( SSD1306_RST_PIN, 0 );
	Delay_Ms(10);
	funDigitalWrite( SSD1306_RST_PIN, 1 );
	Delay_Us(10);
}
#endif

#define SDA_HIGH funDigitalWrite( SSD1306_I2C_BITBANG_SDA, 1 );
#define SCL_HIGH funDigitalWrite( SSD1306_I2C_BITBANG_SCL, 1 );
#define SDA_LOW  funDigitalWrite( SSD1306_I2C_BITBANG_SDA, 0 );
#define SCL_LOW  funDigitalWrite( SSD1306_I2C_BITBANG_SCL, 0 );
#define SDA_IN   funDigitalRead( SSD1306_I2C_BITBANG_SDA );
#ifndef I2CSPEEDBASE
#define I2CSPEEDBASE 1
#endif
#ifndef I2CDELAY_FUNC
#define I2CDELAY_FUNC(x) ADD_N_NOPS(x*1)
#endif

static int ssd1306_mini_i2c_sendbyte( unsigned char data );// __attribute__((section(".srodata")));

static void ssd1306_mini_i2c_sendstart()
{
	SCL_HIGH
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SDA_LOW
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SCL_LOW
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
}

void ssd1306_mini_i2c_sendstop()
{
	SDA_LOW
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SCL_LOW
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SCL_HIGH
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SDA_HIGH
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
}

//Return nonzero on failure.
#if 0
static int ssd1306_mini_i2c_sendbyte( unsigned char data )
{
	int i;
	for( i = 0; i < 8; i++ )
	{
		I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
		if( data & 0x80 )
		{ SDA_HIGH; }
		else
		{ SDA_LOW; }
		data<<=1;
		I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
		SCL_HIGH
		I2CDELAY_FUNC( 2 * I2CSPEEDBASE );
		SCL_LOW
	}

	//Immediately after sending last bit, open up DDDR for control.
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
//	funPinMode( SSD1306_I2C_BITBANG_SDA, GPIO_CFGLR_IN_PUPD );
//	SDA_HIGH
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SCL_HIGH
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
//	i = SDA_IN;
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SCL_LOW
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	SDA_HIGH // Maybe?
//	funPinMode( SSD1306_I2C_BITBANG_SDA, GPIO_CFGLR_OUT_10Mhz_PP );
	I2CDELAY_FUNC( 1 * I2CSPEEDBASE );
	return 0;
}
#endif

static int ssd1306_mini_i2c_sendbyte( unsigned char data )
{
	//#define SSD1306_I2C_BITBANG_SDA PC1
	//#define SSD1306_I2C_BITBANG_SCL PC2
	asm volatile( "\
		\n\
		mv  t0, s0\n\
		mv  t1, s1\n\
		li  a2, 0x00020000 /* SDA = PC1 */ \n\
		li  s0, 0x0004 /* SCL = PC2 */ \n\
		li  s1, 0x00040000 /* SCL = PC2 */ \n\
		li	a4,8\n\
		lui	a5,0x40011 /* 0x40011010 = R32_GPIOC_BSHR */ \n\
1:			srli a3, %[data], 3\n\
			andi a3, a3, 0x10\n\
			sra  a3, a2, a3\n\
			sw	a3,16(a5) /* Set SDA appropriately */ \n\
			sw  s0,16(a5) /* SCL Hi */ \n\
			addi a4, a4, -1\n\
			slli %[data], %[data], 1\n\
			sw  s1,16(a5) /* SCL Low */ \n\
			bnez a4, 1b\n\
		sw  s0,16(a5) /* SCL Hi */ \n\
		mv  s0, t0\n\
		sw  s1,16(a5) /* SCL Low */ \n\
		mv  s1, t1\n\
		\n\
		" : : [data]"r"(data) : "a0", "a1", "a2", "a3", "a4", "a5", "t0", "t1", "memory" );
	return 0;
}


int ssd1306_mini_pkt_send(const uint8_t *data, int sz, int cmd)
{
	ssd1306_mini_i2c_sendstart();
	int r = ssd1306_mini_i2c_sendbyte( SSD1306_I2C_ADDR<<1 );
	if( r ) return r;
	//ssd1306_i2c_sendstart(); For some reason displays don't want repeated start
	if(cmd)
	{
		if( ssd1306_mini_i2c_sendbyte( 0x00 ) )
			return 1; // Control
	}
	else
	{
		if( ssd1306_mini_i2c_sendbyte( 0x40 ) )
			return 1; // Data
	}
	for( int i = 0; i < sz; i++ )
	{
		if( ssd1306_mini_i2c_sendbyte( data[i] ) )
			return 1;
	}
	ssd1306_mini_i2c_sendstop();
	return 0;
}

int ssd1306_mini_data(uint8_t *data, int sz)
{
	return ssd1306_mini_pkt_send(data, sz, 0);
}


#endif

