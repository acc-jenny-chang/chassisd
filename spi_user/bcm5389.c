#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/ioctl.h>

int DEBUG_GPIO=0;

#define MAX_GPIOMEM_SIZE 0x00001000
#define TIMER_PORT 	0x00010000


/* graceliu modify it*/
#define GPIO1_BASE_ADDR 0x500
#define GPIO1_USE_SEL 0x0 /* 1 is GPIO , 0 is nactive */
#define GPIO1_IO_SEL 0x4 /* input=1 or output=0 */ 
#define GPIO1_IO_LVL 0xc  /* 1/0 */

#define GPIO2_BASE_ADDR 0x580
#define GPIO2_USE_SEL 0x0 /* 1 is GPIO , 0 is nactive */
#define GPIO2_IO_SEL 0x4 /* input=1 or output=0 */ 
#define GPIO2_IO_LVL 0x8

unsigned short g_usCpu_Frequency  = 1000;    /*Enter your CPU frequency here, unit in MHz.*/

#define GPIO_MODE_OUTPUT 0
#define GPIO_MODE_INPUT 1
#define GPIO_CLOCK  	0+1
#define GPIO_MOSI  	1+1
#define GPIO_MISO  	26+1 
#define GPIO_CS  	27+1

#define DIAG_READ 001
#define DIAG_WRITE 002

#define SPI_MODE_READ 0x0
#define SPI_MODE_WRITE 0x1

unsigned int g_default_one=0x0;
unsigned int g_default=0x0;

#define TIMER_VALUE_1US		500


void writePort( unsigned char a_ucPins, unsigned char a_ucValue );
unsigned char readPort();
void sclock();
void ispVMDelay( unsigned short a_usTimeDelay );



void set_gpio_level(int bank, int pin, int mode)
{
	unsigned int num_bit=0;
	unsigned int base_addr_lvl;
	unsigned int val,tmp;

	if (bank==0)
	{
 		 base_addr_lvl = GPIO1_BASE_ADDR+GPIO1_IO_LVL;		 

	}else if (bank==1)
	{
		 base_addr_lvl = GPIO2_BASE_ADDR+GPIO2_IO_LVL;		 
	}

	 num_bit = pin%32;

	 if (DEBUG_GPIO)
        	printf("\n\rSetGPIO bank=%2d, pin=%2d, mode=%2d", bank, pin, mode);
	 
	 tmp = 0;
	/* set the gpio mode = 1*/
	 if (mode==0)
	 {
		 val = inl(base_addr_lvl);
	  	 tmp=(val & ~(1<<(num_bit-1)));
	        outl(tmp, base_addr_lvl); 

	 }
	 else if (mode==1)
	 {
		 val = inl(base_addr_lvl);
	  	 tmp=(val|(1<<(num_bit-1)));
	        outl(tmp, base_addr_lvl); 
	 }

	 if (DEBUG_GPIO)
	 {
        	printf("\n\rSetGPIO base_addr_lvl=%8x old=%8x new=%8x", base_addr_lvl, val, tmp);
		 val = inl(base_addr_lvl);
        	printf("\n\rReadback %8x=%8x", base_addr_lvl, val);
	 }



}


unsigned char get_gpio_level(int bank, int pin)
{
	unsigned int num_bit=0;
	unsigned int base_addr_lvl;
	unsigned int val,tmp=0;
	
	if (bank==0)
	{
 		 base_addr_lvl = GPIO1_BASE_ADDR+GPIO1_IO_LVL;		 

	}else if (bank==1)
	{
		 base_addr_lvl = GPIO2_BASE_ADDR+GPIO2_IO_LVL;		 
	}

	 num_bit = pin%32;
	 val = inl(base_addr_lvl);
	 tmp = (val&(1<<(num_bit-1)))>>(num_bit-1);

 	 if (DEBUG_GPIO)
    		 printf("\n\rGetGPIO %8x at pin %d = %8x  %x",base_addr_lvl,pin,val,tmp);

	return (unsigned char)tmp;


}




/* the bank is 0/0x500, 1/0x530 in cpu
*   pin is the definition in the GPIO 
*   mode is input and output
*/

void set_gpio_direction(int bank, int pin, int mode)
{
	unsigned int num_bit=0;
	unsigned int base_addr_sel;
	unsigned int base_addr_io;
	unsigned int val,tmp;
	
	if (bank==0)
	{
		 base_addr_sel = GPIO1_BASE_ADDR+GPIO1_USE_SEL;		 
 		 base_addr_io = GPIO1_BASE_ADDR+GPIO1_IO_SEL;		 

	}else if (bank==1)
	{
		 base_addr_sel = GPIO2_BASE_ADDR+GPIO1_USE_SEL;		 
		 base_addr_io = GPIO2_BASE_ADDR+GPIO1_IO_SEL;		 
	}

	 num_bit = pin%32;
	/* set the gpio mode = 1*/
	 tmp=0;
	 val = inl(base_addr_sel);
  	 tmp=(val|(1<<(num_bit-1)));
        outl(tmp, base_addr_sel); 

	
	 /* set to input/ output*/
	 val = inl(base_addr_io);
	 tmp=0;
	 if (mode==GPIO_MODE_OUTPUT)
	 {
	 	tmp=(val & ~(1<<(num_bit-1)));
	 }else if (mode==GPIO_MODE_INPUT)
	 {
	 	tmp=(val|(1<<(num_bit-1)));
	 }
	 outl(tmp,base_addr_io);

}

void write_level( unsigned char a_ucPins, unsigned char a_ucValue )
{
	switch(a_ucPins)
		{
			case GPIO_CS:
				{
					if (a_ucValue==1)
					{
						g_default_one|=0x8000000;
					}
					else if (a_ucValue==0)
					{
	
						g_default_one&=~0x8000000;
					}
					outl(g_default_one, 0x588); 					
				}
				break;

			case GPIO_MOSI:
				{
					if (a_ucValue==1)
					{
						g_default_one|=0x2;
					}
					
					else if (a_ucValue==0)
					{
						g_default_one|=~0x2;
	
					}
					outl(g_default_one, 0x588); 					
				}
				break;
			case GPIO_CLOCK:
				{
					if (a_ucValue==1)
					{
						g_default_one|=0x1;
					}
					else if (a_ucValue==0)
					{

						g_default_one&=~0x1;
					}
					outl(g_default_one, 0x588); 					

				}
				break;
		}

}


int device_gpio_init()
{
	unsigned int base_addr_sel;
	unsigned int base_addr_lvl;

       base_addr_sel = GPIO1_BASE_ADDR;
       base_addr_lvl = GPIO2_BASE_ADDR+GPIO2_IO_LVL+4;

	if (ioperm(base_addr_sel, base_addr_lvl, 1)<0)
	{
	       perror(base_addr_sel);
	       return 1;
	}

	set_gpio_direction(1, GPIO_CLOCK, GPIO_MODE_OUTPUT);
	set_gpio_direction(1, GPIO_CS, GPIO_MODE_OUTPUT);
	set_gpio_direction(1, GPIO_MOSI, GPIO_MODE_OUTPUT);
	set_gpio_direction(1, GPIO_MISO, GPIO_MODE_INPUT);

	write_level(GPIO_CLOCK,1);
	write_level(GPIO_MOSI,0);	
	write_level(GPIO_CS,1);
	
	ispVMDelay(1000);
	g_default_one=inl(0x588);

}


void writePort( unsigned char a_ucPins, unsigned char a_ucValue )
{
	int i=0;


	switch(a_ucPins)
		{
			case GPIO_CS:
				{
					if (a_ucValue==1)
					{
						/* disable CS pull high*/
						g_default_one|=0x8000000;
					       g_default_one|=0x1; 
						outl(g_default_one, 0x588); 					

					}
					else if (a_ucValue==0)
					{
	
					      /* enable CS pull low*/
						g_default_one&=0xF7FFFFFF;
						outl(g_default_one, 0x588);						
					}
					return ;

				}
				break;

			case GPIO_MOSI:
				{
					/* clock pull low */
				       g_default_one&=0xFFFFFFFE; 
					outl(g_default_one, 0x588); 
					
					if (a_ucValue==1)
					{
						g_default_one|=0x02;
					}
					else if (a_ucValue==0)
					{
	
						g_default_one&=0xFFFFFFFD;
					}
					outl(g_default_one, 0x588); 

				}

			       /* clock pull high */
			       g_default_one|=0x1; 
				outl(g_default_one, 0x588); 
				outl(g_default_one, 0x588); /* balance clock */

				break;
		}	
}

/*********************************************************************************
*
* readPort
*
* Returns the value of the TDO from the device.
*
**********************************************************************************/
unsigned char readPort()
{
        unsigned char ucRet;

        g_default_one&=0xFFFFFFFE; 
	 outl(g_default_one, 0x588); 	
        ucRet= (inb(0x58b)&0x4)>>2; 		
	 g_default_one|=0x1; 
	 outl(g_default_one, 0x588); 
        return ucRet;

}

/*********************************************************************************
* sclock
*
* Apply a pulse to TCK.
*
* This function is located here so that users can modify to slow down TCK if
* it is too fast (> 25MHZ). Users can change the IdleTime assignment from 0 to
* 1, 2... to effectively slowing down TCK by half, quarter...
*
*********************************************************************************/

void ispVMDelay( unsigned short a_usTimeDelay )
{

	unsigned short loop_index     = 0;
	unsigned short ms_index       = 0;
	unsigned short us_index       = 0;
        unsigned short useless;

	if ( a_usTimeDelay & 0x8000 ) /*Test for unit*/
	{
		a_usTimeDelay &= ~0x8000; /*unit in milliseconds*/
	}
	else { /*unit in microseconds*/
		a_usTimeDelay = (unsigned short) (a_usTimeDelay/1000); /*convert to milliseconds*/
		if ( a_usTimeDelay <= 0 ) {
			 a_usTimeDelay = 1; /*delay is 1 millisecond minimum*/
		}
	}
	/*Users can replace the following section of code by their own*/
	for( ms_index = 0; ms_index < a_usTimeDelay; ms_index++)
	{
		/*Loop 1000 times to produce the milliseconds delay*/
		for (us_index = 0; us_index < 1000; us_index++)
		{ /*each loop should delay for 1 microsecond or more.*/
			loop_index = 0;
			do {
				/*The NOP fakes the optimizer out so that it doesn't toss out the loop code entirely*/
				//	asm NOP
				useless = loop_index;
			}while (loop_index++ < ((g_usCpu_Frequency/8)+(+ ((g_usCpu_Frequency % 8) ? 1 : 0))));/*use do loop to force at least one loop*/
		}
	}

}
  
void spi_write_byte(unsigned char b)  
{  
    int i;  
    unsigned char value=0;

    for (i=7; i>=0; i--) 
    {  
	value=(b&(1<<i));
	if (value!=0)	
	    writePort(GPIO_MOSI,1 );  
	else
	    writePort(GPIO_MOSI,0);  	
    }  
}  
unsigned char spi_read_byte()  
{  
    int i;  
    unsigned char r = 0;  
    for (i=0; i<8; i++) {  
        r = (r <<1) | readPort();  
    }
    return r;
}  


void print_usage(void)
{
	fprintf(stderr, "\r\nUsage: [options]\r\n");
	fprintf(stderr, "Options: -r <offset>	(read register)\r\n");
	fprintf(stderr, "Options: -w <offset>	(write register)\r\n");
	fprintf(stderr, "Options: -v <value>	(set value)\r\n");
	fprintf(stderr, "Options: -p <page>	(page)\r\n");	
	fprintf(stderr, "Options: -l <size>		(size)\r\n");	
	fprintf(stderr, "  \r\n");

}

#define PAGE_STATUS 0xFE
#define PAGE_REGISTER 0xFF
#define PAGE_DATAIO 0xF0


void bcm_page_prefix(uint8_t mode, uint8_t registerss)
{
	/* mode is 0 --> read */
	spi_write_byte(0x60|mode);
	spi_write_byte(registerss);
}

int bcm5389_read(uint8_t page, uint8_t offset, uint8_t* value,int8_t size )
{

	uint8_t temp=0;
	uint8_t i=0;
	
       writePort(GPIO_CS,0);     	
	bcm_page_prefix(SPI_MODE_READ, PAGE_STATUS);
	temp=spi_read_byte();
       writePort(GPIO_CS,1);     

	//if ((temp&0x80)==0x80)
	//	return 1;
       writePort(GPIO_CS,0);     			
  	bcm_page_prefix(SPI_MODE_WRITE, PAGE_REGISTER);
	spi_write_byte(page);
       writePort(GPIO_CS,1);     

       writePort(GPIO_CS,0);     			
	bcm_page_prefix(SPI_MODE_READ, offset);
	bcm_page_prefix(SPI_MODE_READ, PAGE_STATUS);
	temp=spi_read_byte();
       writePort(GPIO_CS,1);     			

	//if ((temp&0x20)!=0x20)
	//	return 1;
	
       writePort(GPIO_CS,0);     				
	bcm_page_prefix(SPI_MODE_READ, PAGE_DATAIO);

	while(size!=0)
	{
		temp=spi_read_byte();
		value[i++]=temp;
		size--;
	}
	
       writePort(GPIO_CS,1);     

	return 0;
}

//int bcm5389_write(uint8_t page, uint8_t offset,unsigned int value ,uint8_t size)
int bcm5389_write(uint8_t page, uint8_t offset, uint8_t* value ,uint8_t size)
{

	uint8_t temp=0;	
	uint8_t i=0;

       writePort(GPIO_CS,0);     					
	bcm_page_prefix(SPI_MODE_READ, PAGE_STATUS);
	temp=spi_read_byte();
       writePort(GPIO_CS,1);     				

	//if ((temp&0x80)==0x80)
	//	return 1;
       writePort(GPIO_CS,0);     				
  	bcm_page_prefix(SPI_MODE_WRITE, PAGE_REGISTER);
	spi_write_byte(page);
       writePort(GPIO_CS,1); 
	   
       writePort(GPIO_CS,0);     
	bcm_page_prefix(SPI_MODE_WRITE, offset);
	while (size!=0)
	{
		//temp=(value>>8*i)&0xff;
		temp=value[i];
		spi_write_byte(temp);
		i++;
		size--;
	}
	writePort(GPIO_CS,1);
	return 0;
}

uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    int i=0;

    printf("hex2int %x\n",hex);
            #if 1
    while (hex&&i<2) {
        
        // get current character then increment
        uint8_t byte = *(hex++);
        printf("byte=%c\n",byte);
        i++;
        
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        val = (val << 4) | (byte & 0xF);

    }
            #endif
    return val;
}

int converStrtoArray(char * array, char * val, int len)
{
    int val_len = strlen(val);
    int i =val_len-1;
    int have_x=0;
    int j=0;
    uint8_t value = 0;
    uint8_t byte1,byte2;
    
    //printf("Array is %x, str is %s\r\n",*array,val);        
    if(val[1]=='x'||val[1]=='X'){
    //    i=2;
        have_x =2;
        //printf("have 0x\r\n");
    }
    if((val_len-have_x)/2>len)
        val_len = len*2;
    for(i;i>=0;i=i-2,j++)
    {
        //printf("i=%d, %x%x\n",i,val[i],val[i+1]);
        byte1 = val[i-1];
        byte2 = val[i];
        
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte1 >= '0' && byte1 <= '9') byte1 = byte1 - '0';
        else if (byte1 >= 'a' && byte1 <='f') byte1 = byte1 - 'a' + 10;
        else if (byte1 >= 'A' && byte1 <='F') byte1 = byte1 - 'A' + 10;
        else byte1=0;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        value = (value << 4) | (byte1 & 0xF);
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte2 >= '0' && byte2 <= '9') byte2 = byte2 - '0';
        else if (byte2 >= 'a' && byte2 <='f') byte2 = byte2 - 'a' + 10;
        else if (byte2 >= 'A' && byte2 <='F') byte2 = byte2 - 'A' + 10;    
        else byte2=0;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        value = (value << 4) | (byte2 & 0xF);

        array[j] = value;
        //array[j]=hex2int(val[i]);
        //printf("scan %x\n",array[j]);
    }

    return 0;
}

int run_diag_argument(int argc, char **argv)
{
	int opt = -1;
	
	uint8_t  page_num=0;
	uint8_t  reg_num=0;
	//unsigned int  reg_data=0;
       uint8_t  reg_data[8]={0};
	uint8_t  *data_u8_ptr=NULL;
	int8_t  size=1;
	int mode;
	
	if ((argc == 1 ) || (argc == 2 )) {
		print_usage();
		return 0;
	}

	while ((opt = getopt(argc, argv, "r:w:p:v:l:")) != -1) {
		switch (opt) {
			
			case 'r':
				reg_num = (uint8_t) strtoul(optarg, NULL, 16);
				mode=DIAG_READ;
				break;

			case 'w':
				mode=DIAG_WRITE;				
				reg_num = (uint8_t) strtoul(optarg, NULL, 16);
				break;
				
			case 'p':		
				page_num = (uint8_t) strtoul(optarg, NULL, 16);
				break;
				
			case 'l':		
				size = (uint8_t) strtoul(optarg, NULL, 16);
				break;

			case 'v':
				//reg_data =  (unsigned int) strtoull(optarg, NULL, 16);
				converStrtoArray(&reg_data,optarg,size);
    //printf("Output Array is %02x%02x%02x%02x%02x%02x%02x%02x, str is %s\r\n",
    //    reg_data[0],
    //    reg_data[1],
    //    reg_data[2],
    //    reg_data[3],
    //    reg_data[4],
    //    reg_data[5],
    //    reg_data[6],
   //     reg_data[7],
   //     optarg);                
				break;
				
			default:
				print_usage();
				return 0;

		}
		
	   } /* end while ((opt = getopt(argc, argv, "r:w:v:"))*/

	  device_gpio_init();
	  data_u8_ptr = calloc(sizeof(uint8_t), size);
	  if (data_u8_ptr == NULL)
			return -1;

	  switch (mode){		
			case DIAG_READ:	

				if (bcm5389_read(page_num, reg_num, data_u8_ptr,size ))
					return -1;

				printf("\n\rpage_num=0x%02x, reg_num=0x%02x Data=",
				page_num, reg_num);
				while(size!=0)
				{
					printf(" 0x%x ",*(data_u8_ptr+(size-1)) );
					size--;
				}
				printf("\n\r");
				break;

			case DIAG_WRITE:

				if (bcm5389_write(page_num, reg_num,reg_data,size))
					return -1;

				break;
				
			case 'h':
			default:
				print_usage();
				return 0;

	  	}
	  
	  return 0;
   
}

int main(int argc, char *argv[])
{

	return run_diag_argument(argc, argv);

}
