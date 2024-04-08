#include <stdio.h>
#ifdef UNIX
#include <stdlib.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#pragma warning( disable: 4251 )
#include <windows.h>
#endif
#include <math.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "xactors.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUG

#ifdef HAPS60
// #define XTOR_DW_32
#define XTOR_DW_64
// #define XTOR_DW_128
// #define XTOR_DW_256
// #define XTOR_DW_512
#else
#define XTOR_DW_64
#endif

//#define GPIO
#define AXI_M2

#ifdef XTOR_DW_32
#define XTOR_DW 32
#define XTOR_SIZE e_BITS32
#endif
#ifdef XTOR_DW_64
#define XTOR_DW 64
#define XTOR_SIZE e_BITS64
#endif
#ifdef XTOR_DW_128
#define XTOR_DW 128
#define XTOR_SIZE e_BITS128
#endif
#ifdef XTOR_DW_256
#define XTOR_DW 256
#define XTOR_SIZE e_BITS256
#endif
#ifdef XTOR_DW_512
#define XTOR_DW 512
#define XTOR_SIZE e_BITS512
#endif

//#define MAX_BURST_LENGTH 4096 ; // 4K boundary
//#define MAX_BURST_LENGTH 65536;
#define MAX_BURST_LENGTH 128*1024 ; // MAX 256k
#define MAX_BURST_LENGTH_SIM 256
//#define MAX_BURST_LENGTH 32768 maximum, but 4k warning
//#define MAX_BURST_LENGTH 65536 error
#define ERROR_OUTPUT_RANGE 1024

/* all capims used by this program */
#define MASTER_ID1 1
#define MASTER_ID2 2
#define APB_ID3   3
#define GPIO_ID4   4

/* global variables and switches */
int umr_device = 0;
int umr_bus = 1;
//  int capim1address = 1;
int use_axi  = 1;
int use_gpio = 0;

int umr_device_def = umr_device;
int umr_bus_def = umr_bus;
//  int capim1address_def = capim1address;

#define MODE_READ 1
#define MODE_WRITE 2
#define MODE_WRITE_READ_COMPARE 3
#define MODE_READ_COMPARE 4
#define MODE_WRITE_FROM_FILE 5
int mode = MODE_WRITE_READ_COMPARE;

char * filename = NULL;

#define DATAMODE_RANDOM 1
#define DATAMODE_NIBBLEPATTERN 2
#define DATAMODE_STRESS_00FF 3
#define DATAMODE_STRESS_55AA 4

int datamode  = DATAMODE_RANDOM;

UMRBUS_LONG max_burst  = MAX_BURST_LENGTH;
#if __WORDSIZE == 64
long long offset     = 0;
long long offset_def = offset;
long long p_offset   = 0;
#else
UMRBUS_LONG offset     = 0;
UMRBUS_LONG offset_def = offset;
UMRBUS_LONG p_offset   = 0;
#endif
UMRBUS_LONG length     = 268435456; // 256 MByte data
UMRBUS_LONG length_def = length;

UMRBUS_LONG delay      = 0;

long long p_index  = 0;
long long p_length = 0;

char * errormessage;

void usage (char * program)
{
	printf ("usage: %s [options]\n\n", program);
	printf ("\t-d device         UMRBus device number (default is %i)\n", umr_device_def);
	printf ("\t-b umrbus         UMRBus number (default is %i)\n", umr_bus_def);
	//    printf ("\t-c capim1address  address from the CAPIM1 of dramport client (default is %i)\n", capim1address_def);
	printf ("\t-x transactor ID  access axi master ID 1 (32bit) or ID 2 (64bit) (default is %i)\n", use_axi);
	printf ("\t-m mode           access mode (default is write_read_compare)\n");
	printf ("\t    -m read                reads data from RAM and store they into a file\n");
	printf ("\t                           (option -f required)\n");
	printf ("\t    -m write               write generated data (option -g) into a file\n");
	printf ("\t                           (option -f) and into the RAM\n");
	printf ("\t    -m write_from_file     read data from a file (option -f) and write into the RAM\n");
	printf ("\t    -m write_read_compare  writes, reads and compares generated data (option -g)\n");
	printf ("\t    -m read_compare        reads data from RAM and compares they with data\n");
	printf ("\t                           from a file (option -f required)\n");
	printf ("\t-f filename       filename which is required by some modi\n");
	printf ("\t-g datamode       mode how data is generated (random is default)\n");
	printf ("\t    -g random              randomized data will be generated\n");
	printf ("\t    -g 00ff                data like 00, FF, 00, ... will be generated\n");
	printf ("\t    -g 55aa                data like 55, AA, 55, ... will be generated\n");
	printf ("\t    -g nibblepattern       data like 11, 22, 33, ... will be generated\n");
	printf ("\t-o offset         start byte address for RAM access (default is %li)\n", (long int) offset_def);
	printf ("\t-l length         number of bytes that will be processed (default is %li = %li MByte)\n", (long int) length_def, (long int) length_def / 1024 / 1024);
	printf ("\t-s simdelay       change to simulation mode, add delay between UMRBus accesses (in sec)\n");
}

#ifdef WINDOWS
#define SLEEP(iValue) Sleep (iValue*1000)
#else
#define SLEEP(iValue) sleep (iValue)
#endif

//#define XTOR_A_WIDTH 64
#define XTOR_A_WIDTH 32
#define XTOR_D32_WIDTH 32
#define XTOR_D64_WIDTH 64
#define XTOR_D128_WIDTH 128

MasterXactor<XTOR_A_WIDTH,XTOR_D128_WIDTH> *m1 = NULL;
MasterXactor<XTOR_A_WIDTH,XTOR_D128_WIDTH> *m2 = NULL;
MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>  *m3 = NULL;
GpioXactor<32,32> *gpio = NULL;

int XtorSend_32(MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH> *m, unsigned long address, unsigned char *ptSrcData, long Size){

	if ( use_gpio == 1 ) {	
					gpio->setOutput(1);
	}

	TLMResponse<XTOR_D32_WIDTH>             rsp;
	TLMRequest<XTOR_A_WIDTH,XTOR_D32_WIDTH> req;
	req.m_header.m_command    = TLMCommand::e_WRITE;
	req.m_header.m_b_size     = TLMBSize::e_BITS32;
	req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
	req.m_b_length            = unsigned ((address % 4 + Size + 3) / 4);
	req.m_address             = address;

	req.setPayload();

	req.fillPayload((char*) ptSrcData, Size * sizeof(char));

	m->send(req, rsp);
	if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
					printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
					return 1;
	}

	if (delay != 0) SLEEP (delay);

	if ( use_gpio == 1 ) {	
					gpio->setOutput(0);
	}

	return 0;

}

int XtorSend_128(MasterXactor<XTOR_A_WIDTH,XTOR_D128_WIDTH> *m, unsigned long address, unsigned char *ptSrcData, long Size){

	if ( use_gpio == 1 ) {	
					gpio->setOutput(1);
	}

	TLMResponse<XTOR_D128_WIDTH>             rsp;
	TLMRequest<XTOR_A_WIDTH,XTOR_D128_WIDTH> req;
	req.m_header.m_command    = TLMCommand::e_WRITE;
	req.m_header.m_b_size     = TLMBSize::e_BITS128;
	req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
	req.m_b_length            = unsigned ((address % 16 + Size + 15) / 16);	// 128
	req.m_address             = address;

	req.setPayload();

	req.fillPayload((char*) ptSrcData, Size * sizeof(char));

	m->send(req, rsp);
	if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
					printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
					return 1;
	}

	if (delay != 0) SLEEP (delay);

	if ( use_gpio == 1 ) {	
					gpio->setOutput(0);
	}

	return 0;

}

int XtorRead_32(MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH> *m, unsigned long address, unsigned char *ptDstData, long Size){

	if ( use_gpio == 1 ) {	
					gpio->setOutput(1);
	}

	TLMResponse<XTOR_D32_WIDTH>             rsp;
	TLMRequest<XTOR_A_WIDTH,XTOR_D32_WIDTH> req;
	req.m_header.m_command    = TLMCommand::e_READ;
	req.m_header.m_b_size     = TLMBSize::e_BITS32;
	req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
	req.m_b_length            = unsigned ((address % 4 + Size + 3) / 4);
	req.m_address             = address;

	m->send(req, rsp);
	if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
					printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
					return 1;
	}

	rsp.fillMemory(address, (char*) ptDstData, Size * sizeof(char));

	if (delay != 0) SLEEP (delay);

	if ( use_gpio == 1 ) {	
					gpio->setOutput(0);
	}

	return 0;

}

int XtorRead_128(MasterXactor<XTOR_A_WIDTH,XTOR_D128_WIDTH> *m, unsigned long address, unsigned char *ptDstData, long Size){

	if ( use_gpio == 1 ) {	
		gpio->setOutput(1);
	}

	TLMResponse<XTOR_D128_WIDTH>             rsp;
	TLMRequest<XTOR_A_WIDTH,XTOR_D128_WIDTH> req;
	req.m_header.m_command    = TLMCommand::e_READ;
	req.m_header.m_b_size     = TLMBSize::e_BITS128;
	req.m_header.m_burst_mode = TLMBurstMode::e_INCR;
	req.m_b_length            = unsigned ((address % 16 + Size + 15) / 16);	// 128
	req.m_address             = address;

	m->send(req, rsp);
	if (rsp.m_header.m_status != TLMStatus::e_SUCCESS) {
		printf("@Error: %s = %s\n", rsp.m_header.m_error.getClassName(), rsp.m_header.m_error.getName());
		return 1;
	}

	rsp.fillMemory(address, (char*) ptDstData, Size * sizeof(char));

	if (delay != 0) SLEEP (delay);

	if ( use_gpio == 1 ) {	
		gpio->setOutput(0);
	}

	return 0;

}

/* main routine */
int main (int argc, char * argv[]){
	printf("maining......\n");
	XactorLog::SetOutputLevel(XACTOR_MSG_ERROR); 

	int retval = 1;
	int print_error_pattern = 0;
	UMRBUS_LONG error_word_count = 0;
	UMRBUS_LONG error_bit_count = 0;
	UMRBUS_LONG ulBitErrors[32];
	UMRBUS_LONG ulPreviousBitErrors[32]; 
	unsigned int ui;
	int i = 1;
	unsigned char value;
	UMRBUS_LONG li, start, lli, error_pattern, previous_error_pattern;
	UMRBUS_LONG ulOddErrors = 0;
	UMRBUS_LONG ulEvenErrors = 0;
	unsigned char * inbuffer = NULL;
	unsigned char * outbuffer = NULL;
	FILE * file = NULL;
	BitT<32u> gpi_data = 0;

	fflush(stdout);

	i = 1;
	/* parse command line arguments */
	while (i < argc) {
		if (i == argc-1) goto usage_exit;
		if (*argv[i] != '-') goto usage_exit;
		if (*(argv[i]+2) != 0) goto usage_exit;
	
		switch (*(argv[i]+1)) {
			case 'd': umr_device = strtoul(argv[i+1], NULL, 0); break;
			case 'b': umr_bus = strtoul(argv[i+1], NULL, 0); break;
								//        case 'c': capim1address = strtoul (argv[i+1], NULL, 0); break;
			case 'a': use_axi = strtoul (argv[i+1], NULL, 0); break;
			case 'x': use_axi = strtoul (argv[i+1], NULL, 0); break;
			case 'm': 
				if (strcmp (argv[i+1], "read") == 0) mode = MODE_READ;
				else if (strcmp (argv[i+1], "write") == 0) mode = MODE_WRITE;
				else if (strcmp (argv[i+1], "write_from_file") == 0) mode = MODE_WRITE_FROM_FILE;
				else if (strcmp (argv[i+1], "write_read_compare") == 0) mode = MODE_WRITE_READ_COMPARE;
				else if (strcmp (argv[i+1], "read_compare") == 0) mode = MODE_READ_COMPARE;
				else goto usage_exit;
				break;
			case 'f': filename = argv[i+1]; break;
			case 'g': 
				if (strcmp (argv[i+1], "random") == 0) datamode = DATAMODE_RANDOM;
				else if (strcmp (argv[i+1], "nibblepattern") == 0) datamode = DATAMODE_NIBBLEPATTERN;
				else if (strcmp (argv[i+1], "00ff") == 0) datamode = DATAMODE_STRESS_00FF;
				else if (strcmp (argv[i+1], "55aa") == 0) datamode = DATAMODE_STRESS_55AA;
				else goto usage_exit;
				break;
			case 'o': offset = strtoul (argv[i+1], NULL, 0); break;
			case 'l': length = strtoul (argv[i+1], NULL, 0); break;
			case 's': printf ("INFO: Simulation mode on!\n");
								max_burst = MAX_BURST_LENGTH_SIM; delay = atoi (argv[i+1]) - 1; break;
		}
		i += 2;
	}

	{
		/* check for valid mode options */
		if ((mode == MODE_READ) && (filename == NULL)) goto usage_exit;
		if ((mode == MODE_WRITE) && (filename == NULL)) goto usage_exit;
		if ((mode == MODE_WRITE_FROM_FILE) && (filename == NULL)) goto usage_exit;
		if ((mode == MODE_READ_COMPARE) && (filename == NULL)) goto usage_exit;

		printf("Opening...\n");

		/* trying to open all xactors */
		m1 = new MasterXactor<XTOR_A_WIDTH,XTOR_D128_WIDTH>("master", umr_device, umr_bus, MASTER_ID1);
		if (!m1->isOpen()) {
			printf ("  Error: Cannot open transactor 'm1'\n");   
			goto exit;
		}
		m2 = new MasterXactor<XTOR_A_WIDTH,XTOR_D128_WIDTH>("master", umr_device, umr_bus, MASTER_ID2);
		if (!m2->isOpen()) {
			printf("@WARNING: Cannot open transactor 'm2'\n");
			use_axi = 1;
		}

		m3 = new MasterXactor<XTOR_A_WIDTH,XTOR_D32_WIDTH>("master", umr_device, umr_bus, APB_ID3);
		if (!m3->isOpen()) {
			printf("@WARNING: Cannot open transactor 'm3'\n");
		}

		fflush(stdout);

		printf("Opening : Done...\n");

#ifdef GPIO
		use_gpio = 1;      
		gpio = new GpioXactor<32,32>("gpio", umr_device, umr_bus, GPIO_ID4);
		if (!gpio->isOpen()) {
			printf("@WARNING: Cannot open 'gpio' transactor\n");   
			use_gpio = 0;
		}

		fflush(stdout);

		if ( use_gpio == 1 ) {	
			gpio->getInput(gpi_data);
#ifdef DEBUG
			printf ("@D:      GPIO Inut data: 0x%04x\n", (unsigned int) gpi_data);
			printf ("@D:           DDR3 Init: %d\n",     (unsigned int) gpi_data & 0x0001);
			printf ("@D:       Clocks Locked: %d\n",     ((unsigned int) gpi_data >> 1) & 0x0001);
			printf ("@D:             Reset_N: %d\n",     ((unsigned int) gpi_data >> 4) & 0x0001);
			printf ("@D:        IO_Delay_RDY: %d\n",     ((unsigned int) gpi_data >> 5) & 0x0001);
			printf ("@D:     Phaser_Ref_Lock: %d\n",     ((unsigned int) gpi_data >> 6) & 0x0001);
			for (i = 0; i < 12; i ++) 
							printf ("@D:   Phaser_In_Lock_%02d: %d\n", i,  ((unsigned int) gpi_data >> (7 + i)) & 0x0001); 
			for (i = 0; i < 12; i ++) 
							printf ("@D: Phaser_DQS_Found_%02d: %d\n", i,  ((unsigned int) gpi_data >> (19 + i)) & 0x0001); 
#endif

			if ( !((unsigned int) gpi_data & 0x0001) ) {
				printf("@ERROR: DDR3 Initialization not done!\n");   
				goto exit;
			}  else {    
				printf("DDR3 Initialization done!\n");
			}
		} else {
				printf("@WARNING: No GPIO Transactor found to detect if DDR3 Initialization is done!\n");   
		}
#endif

		/* malloc memory for data buffer */
		if ((inbuffer = (unsigned char *) malloc (sizeof (char) * (length + 8))) == NULL)	{
			printf ("  Error: Out of memory\n");
			goto exit;
		}

		if ((outbuffer = (unsigned char *) malloc (sizeof (char) * (length + 8))) == NULL) {
			printf ("  Error: Out of memory\n");
			goto exit;
		}

		
		/* there are data to read from file? */
		if ((mode == MODE_READ_COMPARE) || (mode == MODE_WRITE_FROM_FILE)) {
			printf ("Read data ");
			if (length < 1024)
				printf ("(%ld Byte)", (long int) (length) );
			else if (length < (1024 * 1024))
				printf ("(%ld KByte)", (long int) (length / ( 1024 )) );
			else if (length < (1024 * 1024 * 1024))
				printf ("(%ld MByte)", (long int) (length / ( 1024 * 1024 )) );
			else
				printf ("(%ld GByte)", (long int) (length / ( 1024 * 1024 * 1024 )) );
			printf (" from \"%s\" ...\n", filename);

			if ((file = fopen (filename, "rb")) == NULL) {
				printf ("  Error opening file: %s\n", strerror (errno));
				goto exit;
			}
			if (fread ((void *) (outbuffer), sizeof (char), length, file) != length) {
				printf ("  Error reading file: %s\n", strerror (errno));
				goto exit;
			}
			fclose (file);
			file = NULL;
		}

		/* there are data to generate? */
		if ((mode == MODE_WRITE) || (mode == MODE_WRITE_READ_COMPARE)) {
			printf ("Generate data ");
			if (length < 1024)
				printf ("(%ld Byte)", (long int) (length) );
			else if (length < (1024 * 1024))
				printf ("(%ld KByte)", (long int) (length / ( 1024 )) );
			else if (length < (1024 * 1024 * 1024))
				printf ("(%ld MByte)", (long int) (length / ( 1024 * 1024 )) );
			else
				printf ("(%ld GByte)", (long int) (length / ( 1024 * 1024 * 1024 )) );
			printf ("\n");
			fflush(stdout);

			switch (datamode)	{
				case DATAMODE_RANDOM:
#ifdef UNIX
					srandom (time (NULL));
#else
					srand ((unsigned) time (NULL));
#endif
					for (li = 0; li < length; li ++) {
#ifdef UNIX
						value = (unsigned char) random ();
#else
						value = (unsigned char) rand ();
#endif
						*(outbuffer+li) = value;
					}
					break;
				case DATAMODE_NIBBLEPATTERN:
					value = 0x11;
					for (li = 0; li < length; li ++) {
						*(outbuffer+li) = value;
						value = (value == 0xFF ? 0x11 : value + 0x11);
					}
					break;
				case DATAMODE_STRESS_00FF:
					value = 0x00;
					for (li = 0; li < length; li++) {
						*(outbuffer+li) = value;
						value = (value == 0x00 ? 0xFF : 0x00);
					}
					break;
				case DATAMODE_STRESS_55AA:
					value = 0x55;
					for (li = 0; li < length; li++)	{
						*(outbuffer+li) = value;
						value = (value == 0x55 ? 0xAA : 0x55);
					}
				}
			}	

			printf("Sending...\n");
            memset(outbuffer, 0x1f, 200);
            memset(inbuffer, 0xe0, 200);
            for(int m=0; m<3; m++){
				XtorSend_32(m3, m*4, &outbuffer[m*4],  1);
				printf("%2d Sending...(%x)\n", m, outbuffer[m*4]);

				printf("%2d Reading... before (%x)\n", m, inbuffer[m*4]);
				XtorRead_32(m3, m*4, &inbuffer[m*4],  1);
				printf("%2d Reading...(%x)\n", m, inbuffer[m*4]);
				if(outbuffer[m*4] != inbuffer[m*4]) {
                    printf("error... m = %2d n = %2d outbuffer[m*4] = %x inbuffer[m*4] = %x\n", m, outbuffer[m*4], inbuffer[m*4]);
                    break;
                }
				else	
                    printf("%2d matched...\n\n", m);
			}
#if 0 
			/* there are data to write into RAM? */
			if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_WRITE) || (mode == MODE_WRITE_FROM_FILE)) {
				if ( use_axi == 2 )
					printf ("XTOR AXI Master2: Send data ");
				else
					printf ("XTOR AXI Master1: Send data ");

				if (length < 1024)
					printf ("(%ld Byte)", (long int) (length) );
				else if (length < (1024 * 1024))
					printf ("(%ld KByte)", (long int) (length / ( 1024 )) );
				else if (length < (1024 * 1024 * 1024))
					printf ("(%ld MByte)", (long int) (length / ( 1024 * 1024 )) );
				else
					printf ("(%ld GByte)", (long int) (length / ( 1024 * 1024 * 1024 )) );
				printf (" to Memory (StartAddress: 0x%016lx) ...\n", (unsigned long) offset);
	
				p_index = 0;
				p_length = length;
				p_offset = offset;
	
				/* split length into MAX_BURST_LENGTH packages */ 
				while ( ( p_length + p_offset % max_burst ) >= max_burst)	{
					if ( use_axi == 1 ) {
						// dummy single send because of bug in transactor lib
						if ( p_offset % 16 > 0 ) {
							XtorSend_128(m1, p_offset, &outbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorSend_128(m1, p_offset, &outbuffer[p_index], max_burst - (p_offset % max_burst));
					} else {
						// dummy single send because of bug in transactor lib
						if ( p_offset % 16 > 0 ) {
							XtorSend_128(m2, p_offset, &outbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorSend_128(m2, p_offset, &outbuffer[p_index], max_burst - (p_offset % max_burst));
					}
	
					p_length = p_length - max_burst + p_offset % max_burst;
					p_index  = p_index  + max_burst - p_offset % max_burst;
					p_offset = p_offset + max_burst - p_offset % max_burst;
				}

				if ( p_length > 0 ) { 
					if ( use_axi == 1 ) {
						// dummy single send because of bug in transactor lib
						if ( p_offset % 16 > 0 && p_offset % 16 + p_length > 16 ) {
							XtorSend_128(m1, p_offset, &outbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorSend_128(m1, p_offset, &outbuffer[p_index], p_length);
					} else {
						if ( p_offset % 16 > 0 && p_offset % 16 + p_length > 16 ) {
							XtorSend_128(m2, p_offset, &outbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorSend_128(m2, p_offset, &outbuffer[p_index], p_length);
					}
				}
			}

			/* there are data to read from RAM? */
			if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE) || (mode == MODE_READ))	{
				if ( use_axi == 2 )
					printf ("XTOR AXI Master2: Read data ");
				else
					printf ("XTOR AXI Master1: Read data ");

				if (length < 1024)
					printf ("(%ld Byte)", (long int) (length) );
				else if (length < (1024 * 1024))
					printf ("(%ld KByte)", (long int) (length / ( 1024 )) );
				else if (length < (1024 * 1024 * 1024))
					printf ("(%ld MByte)", (long int) (length / ( 1024 * 1024 )) );
				else
					printf ("(%ld GByte)", (long int) (length / ( 1024 * 1024 * 1024 )) );
				printf (" from Memory (StartAddress: 0x%016lx) ...\n", (unsigned long) offset);

				p_index = 0;
				p_length = length;
				p_offset = offset;

				/* split length into MAX_BURST_LENGTH packages */ 
				while ( ( p_length + p_offset % max_burst ) > max_burst) {
					if ( use_axi == 1 ) {
						// dummy single read because of bug in transactor lib
						if ( p_offset % 16 > 0 ) {
							XtorRead_128(m1, p_offset, &inbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorRead_128(m1, p_offset, &inbuffer[p_index], max_burst - p_offset % max_burst);
					} else {
						// dummy single read because of bug in transactor lib
						if ( p_offset % 16 > 0 ) {
							XtorRead_128(m2, p_offset, &inbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorRead_128(m2, p_offset, &inbuffer[p_index], max_burst - p_offset % max_burst);
					}

					p_length = p_length - max_burst + p_offset % max_burst;
					p_index  = p_index  + max_burst - p_offset % max_burst;
					p_offset = p_offset + max_burst - p_offset % max_burst;
				}

				if ( p_length > 0 ) {
					if ( use_axi == 1 ) {
						// dummy single read because of bug in transactor lib
						if ( p_offset % 16 > 0 && p_offset % 16 + p_length > 16 ) {
							XtorRead_128(m1, p_offset, &inbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorRead_128(m1, p_offset, &inbuffer[p_index], p_length);
					} else {
						// dummy single read because of bug in transactor lib
						if ( p_offset % 16 > 0 && p_offset % 16 + p_length > 16 ) {
							XtorRead_128(m2, p_offset, &inbuffer[p_index], 16 - p_offset % 16);	
							p_index  = p_index  - p_offset % 16 + 16;
							p_length = p_length + p_offset % 16 - 16;
							p_offset = p_offset - p_offset % 16 + 16;
						}
						XtorRead_128(m2, p_offset, &inbuffer[p_index], p_length);
					}
				}	  
			}

			/* there are compare operations? */
			if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE)) {
				printf ("Compare data ");
				if (length < 1024)
					printf ("(%ld Byte)", (long int) (length) );
				else if (length < (1024 * 1024))
					printf ("(%ld KByte)", (long int) (length / ( 1024 )) );
				else if (length < (1024 * 1024 * 1024))
					printf ("(%ld MByte)", (long int) (length / ( 1024 * 1024 )) );
				else
					printf ("(%ld GByte)", (long int) (length / ( 1024 * 1024 * 1024 )) );

				for (ui = 0; ui < 32; ui ++) {
					ulBitErrors[ui] = 0;
					ulPreviousBitErrors[ui] = 0;
				}

				for (li = 0; li < length; li ++)
					if (* (outbuffer + li) != * (inbuffer + li)) {
						if (print_error_pattern == 0)	{
							print_error_pattern = 1;
							printf ("\nFirst data error at address 0x%08lx\n\n", (unsigned long) (li + offset));
							printf ("address             expected values                      received values          errors\n");
							printf ("----------------------------------------------------------------------------------------\n");
							lli = start = (li < (ERROR_OUTPUT_RANGE >> 1) ? 0 : li - (ERROR_OUTPUT_RANGE >> 1)) & 0xFFFFFFF0;
							while (lli < start + ERROR_OUTPUT_RANGE) {
								printf ("%08lx  ", (unsigned long) (lli + offset));
								for (i = 0; i < 16; i += 4)
									if (lli + i < length) 
										printf ("%02x%02x%02x%02x ", * (outbuffer + lli+i+3), * (outbuffer + lli+i+2), * (outbuffer + lli+i+1), * (outbuffer + lli+i));
									else
										printf ("         ");
								printf (" ");
								for (i = 0; i < 16; i += 4)
									if (lli + i < length) 
										printf ("%02x%02x%02x%02x ", * (inbuffer + lli+i+3), * (inbuffer + lli+i+2), * (inbuffer + lli+i+1), * (inbuffer + lli+i));
									else
										printf ("         ");
									printf (" ");
								for (i = 0; i < 16; i += 4)
									if (lli + i < length)	{
										if ((* (outbuffer + lli+i+3) != * (inbuffer + lli+i+3)) | (* (outbuffer + lli+i+2) != * (inbuffer + lli+i+2)) | (* (outbuffer + lli+i+1) != * (inbuffer + lli+i+1)) | (* (outbuffer + lli+i) != * (inbuffer + lli+i)))
											printf ("#");
										else
											printf ("-");
									}
									else
										printf ("         ");
										printf ("\n");
										lli += 16;
										if ((lli & 0x0000003F) == 0)
											printf ("----------------------------------------------------------------------------------------\n");
										if (lli >= length) break;
									}
								}
								error_word_count = error_word_count + 1;
								if (li % 2)
									ulOddErrors ++;
								else
									ulEvenErrors ++;

								/* get bit error count */
								error_pattern = *(outbuffer + li) ^ *(inbuffer + li);
								if (li != 0)
									previous_error_pattern = *(outbuffer + li - 1) ^ *(inbuffer + li);
								else 
									previous_error_pattern = 0;
								for (i = 0; i < 32; i++) {
									if ((error_pattern & (1 << i)) != 0) {
										error_bit_count = error_bit_count + 1;
										ulBitErrors[i] ++;
										if ((previous_error_pattern & (1 << i)) != 0)
											ulPreviousBitErrors[i] ++;
										}
									}
								}
							}

							/* there are data to write into the file? */
							if ((mode == MODE_WRITE) || (mode == MODE_READ)) {
								printf ("Writing data into \"%s\" ...\n", filename);
								if ((file = fopen (filename, "wb")) == NULL) {
									printf ("  Error opening file: %s\n", strerror (errno));
									goto exit;
								}
								if (fwrite ((void *) (mode == MODE_WRITE ? (outbuffer) : (inbuffer)), sizeof (char), length, file) != length)
								{
									printf ("  Error writing file: %s\n", strerror (errno));
									goto exit;
								}
							fclose (file);
							file = NULL;
							retval = 0;
						}

						if ((mode == MODE_WRITE_READ_COMPARE) || (mode == MODE_READ_COMPARE)) {
							if (error_word_count == 0) {
								retval = 0;
								printf ("\nComparing data successfully completed.\n\n");
							} else {
								retval = 1;
								printf ("\nBit error statistic:\n");
								for (ui = 0; ui < 32 ; ui ++) {
									printf ("  Bit # %2i %10lu errors %10lu errors compared to the previous value\n", ui, (unsigned long) ulBitErrors[ui], (unsigned long) ulPreviousBitErrors[ui]);
								}
								printf ("\nComparing data finished with error(s).\n");
								printf ("  Number of even data word errors : %lu\n", (unsigned long) ulEvenErrors);
								printf ("  Number of odd  data word errors : %lu\n", (unsigned long) ulOddErrors);
								printf ("  Number of data word errors      : %lu\n", (unsigned long) error_word_count);
								printf ("  Number of data bit errors       : %lu\n", (unsigned long) error_bit_count);
			}
		}
#endif
	}

exit:
				/* clean up */
	if (inbuffer) free (inbuffer);
	if (outbuffer) free (outbuffer);
	if (file) fclose (file);
	if (m1) delete (m1);
	if (m2) delete (m2);
	if (m3) delete (m3);
	if (gpio) delete (gpio);
	return (retval);

usage_exit:
	usage (argv[0]);
	goto exit;
}

#ifdef __cplusplus
}
#endif
