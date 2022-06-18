#include "wav.h"
#include "err.h"
#include <vocoderAPI.h>
#include <ctype.h>
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

/*
typedef struct _vocoder_info {
    const char* vocoder_string;
    short vocoder_identification;
} vocoder_info;

vocoder_info supported_vocoders[] = {
    {MELPE_RATE_2400_STR, MELPE_RATE_2400},
    {MELPE_RATE_1200_STR, MELPE_RATE_1200},
    {TETRA_RATE_4800_STR, TETRA_RATE_4800},
    {TETRA_RATE_4666_STR, TETRA_RATE_4666},
    // add more vocoders here!
};
*/

#include <getopt.h>
#include "ini.h"
#include "handler.h"

#define CHANNEL_FL_STR "C1_FL"
#define CHANNEL_TH_STR "C1_TH"

/*
 * Main utterance processing loop:
 *     for (;;) {
 *        start utterance and wait for speech to process
 *        decoding till end-of-utterance silence will be detected
 *        print utterance result;
 *     }
 */

static void 
usage(void) {
	printf("\t-f <filename> if you want save WAV-file\n");
	printf("\t-c <\"%s\" or \"%s\">\n",CHANNEL_FL_STR, CHANNEL_TH_STR);
	printf("\t-l <s> interval time\n");
}

typedef struct
{
    const char* port;
    int vocoder_identification; // from rate
	const char * channel;

} configuration;

static int 
handler(void* user, const char* section, const char* name,const char* value)
{
    configuration* pconfig = (configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH(pconfig->channel, "port")) {
        pconfig->port = strdup(value);
    } else if (strcmp(pconfig->channel, CHANNEL_FL_STR) == 0 && MATCH(pconfig->channel, "rate")) {
		int n = sizeof(supported_vocoders) / sizeof(vocoder_info);
		for (int i = 0; i < n; i++){
			if (!strcmp(value, supported_vocoders[i].vocoder_string)){
				pconfig->vocoder_identification = supported_vocoders[i].vocoder_identification;
				break;
			}
		}
    } else {
		//E_FATAL("unknown section %s or name %s\n",section,name);
        return 0;
    }

    return 1;
}

int
main (int argc, char** argv)
{   
    void* dec;
    //short vocoder_identification = 0;
    int opt, i, num_of_vocoders = sizeof(supported_vocoders) / sizeof(vocoder_info);
 
    size_t frame_sp = 0;
    size_t frame_cbit = 0;

	configuration config;
    config.port = NULL;
    FILE * _out = NULL;

    double t;

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
    if(argc == 1){
		usage();
		return 3;
	}	

    // Разбор параметров командной строки
	while ((opt = getopt(argc, argv, "c:f:l:")) != EOF) {
		switch (opt) {
		case 'c': // считываем канал: ТЧ/ФЛ
		{
			
			if(!strcmp(optarg, CHANNEL_FL_STR ) && !strcmp(optarg, CHANNEL_TH_STR ))
				E_FATAL("Unknow channel % s.\n",opt);

			config.channel = strdup(optarg);
			break;
		}
		case 'f': // чтение потока из файла
		{
    		if ((_out = fopen(optarg, "w")) == NULL)
        		E_FATAL_SYSTEM("Failed to open file '%s' for writing", optarg);

			if (strlen(optarg) > 4 && strcmp(optarg + strlen(optarg) - 4, ".mp3") == 0)
				E_FATAL("Can not create mp3 files, only WAV 16kHz 16-bit or 8-bit mono.\n");
    
			break;
		}
		case 'l': // длина записи
		{
            t = strtod(optarg, NULL);
			// TODO:
			break;
		}
		default:
			usage();
			return 3;
		}
	}

	if(ini_parse("continuous.ini", handler, &config) < 0){
		E_FATAL("Can't load *.ini");
	}else{
		//E_INFO("Config loaded from 'continuous.ini': port = %s, rate = %s\n",config.port, config.rate);
		E_INFO("Config loaded from 'continuous.ini': port = %s\n",config.port);
	}

    frame_sp = vocoder_get_input_size(config.vocoder_identification, VOCODER_DIRECTION_ENCODER);
    frame_cbit = vocoder_get_input_size(config.vocoder_identification, VOCODER_DIRECTION_DECODER);

    E_INFO("frame_sp = %d\tframe_cb = %d\n", frame_sp,frame_cbit);

    if ((frame_sp == 0) || (frame_cbit == 0)) {
        fprintf (stderr, "Cannot determine IO size for codec %d\n", config.vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        fprintf (stderr, "cannot setup library for codec %d\n", config.vocoder_identification);
        return -1;
    }
    dec = vocoder_create(config.vocoder_identification, VOCODER_DIRECTION_DECODER);
    if (dec == NULL) {
        fprintf (stderr, "codec %d cannot be created\n", config.vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }

    short * buffer = (short *)malloc(frame_sp);
    unsigned char * c_frame = (unsigned char *)malloc(sizeof(unsigned char)*frame_cbit);

    // Работа с COM-портом
    struct termios tty0;
	int handle = open(config.port, O_RDWR| O_NOCTTY );
	memset (&tty0, 0, sizeof(tty0));

	// Error Handling
	if (tcgetattr ( handle, &tty0 ) != 0 ) {
		fprintf (stderr,"Error: %d from tcgetattr %s", errno, strerror(errno));
        return -1;
	}

	//tty1 = tty0;

	// Set Baud Rate
	cfsetospeed (&tty0, (speed_t)B9600);
	cfsetispeed (&tty0, (speed_t)B9600);

	// Setting other Port Stuff
	tty0.c_cflag     &=  ~PARENB;            // Make 8n1
	tty0.c_cflag     &=  ~CSTOPB;
	tty0.c_cflag     &=  ~CSIZE;
	tty0.c_cflag     |=  CS8;

	tty0.c_cflag     &=  ~CRTSCTS;           // no flow control
	tty0.c_cc[VMIN]   =  1;                  // read doesn't block
	tty0.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
	tty0.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

//  tty0.c_lflag &= ~(ECHO|ICANON|ISIG)

	// Make raw
	cfmakeraw(&tty0);

	// Flush Port, then applies attributes
	tcflush(handle, TCIFLUSH);
	if (tcsetattr(handle, TCSANOW, &tty0) != 0) {
   		fprintf(stderr,"Error: %d from tcgetattr %s", errno, strerror(errno));
        return -1;
	}

//    printf("Ready...\n");
    write_wav_stereo_header(_out, SAMPLE_RATE, SAMPLE_RATE * (int32_t)t , 1);

    int n = 0, spot = 0;
    int32_t n_blocks_count = 0;
    // Расчет количества отсчетов в wav-файле
    int32_t n_block_max = floor(2 * SAMPLE_RATE * t / frame_sp);

    do {
        n = read(handle, &c_frame[n], frame_cbit - spot);

//      printf("spot: %d n: %d", spot,n);

        spot += n;
        if(spot == frame_cbit){
//            printf("%d ",n_blocks);
//            for (int i = 0; i < spot; i++)
//                printf("%02X",c_frame[i]);
//            printf("\n");
          
            vocoder_process(dec, c_frame, buffer);
            fwrite(buffer,1,frame_sp,_out);
            fflush(_out);

            n_blocks_count++;
            spot = 0; n = 0;
            
//          printf("\nn_frame: %d\n",n_samples);
        }

    } while((n >= 0) &&(n_blocks_count < n_block_max));

    //E_INFO("decoding stat: min=%d us, max=%d us, avg=%d us samples=%d\n", min, max, avg, n_samples);
    printf("decoding stat: samples=%d\n", n_blocks_count);

    fclose(handle);
    fclose(_out);

    vocoder_free(dec);
    free(buffer);
    free(c_frame);
    vocoder_library_destroy();
    return 0;
}
