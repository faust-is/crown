
#include <string.h>
#include <assert.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <sys/select.h>
#endif

#include "err.h"

/*
#include "vocoderAPI.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
//#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include <semaphore.h>
*/
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

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
	printf("\t-m if you want use microphone\n");
	printf("\t-f <filename> if you want load WAV-file\n");
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
		for (int i = 0; i < num_of_vocoders; i++){
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
main(int argc, char *argv[])
{

	configuration config;
    config.port = NULL;
	

	FILE *fd = NULL;
	ad_rec_t *ad = NULL;

    int opt, samplerate = 8000;

    
	struct termios tty0, tty1;

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
	if(argc == 1){
		usage();
		return 3;
	}	

	// Разбор параметров командной строки
	while ((opt = getopt(argc, argv, "c:f:ml:")) != EOF) {
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
    		if ((fd = fopen(optarg, "rb")) == NULL)
        		E_FATAL_SYSTEM("Failed to open file '%s' for reading", optarg);

			if (strlen(optarg) > 4 && strcmp(optarg + strlen(optarg) - 4, ".mp3") == 0)
				E_FATAL("Can not decode mp3 files, convert input file to WAV 16kHz 16-bit mono before decoding.\n");
    
			break;
		}
		case 'm': // запись звука из микрофона
		{
			if ((ad = ad_open_dev(NULL, samplerate)) == NULL)
				E_FATAL("Failed to open audio device\n");

			if (ad_start_rec(ad) < 0)
				E_FATAL("Failed to start recording\n");

			break;
		}
		case 'l': // длина записи
		{
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
    
	// Работа с COM-портом
	//int handle = open( "/dev/ttyUSB0", O_RDWR| O_NOCTTY );
	int handle = open(config.port, O_RDWR| O_NOCTTY );
	memset (&tty0, 0, sizeof(tty0));

	// Error Handling
	if (tcgetattr ( handle, &tty0 ) != 0 ) {
		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
	}

	tty1 = tty0;

	if (!strcmp(config.channel, CHANNEL_FL_STR))
	{
		// Set Baud Rate
		cfsetospeed (&tty0, (speed_t)B9600);
		cfsetispeed (&tty0, (speed_t)B9600);

		// Setting other Port Stuff
		tty0.c_cflag     &=  ~PARENB;            // Make 8n1
		tty0.c_cflag     &=  ~CSTOPB;
		tty0.c_cflag     &=  ~CSIZE;
		tty0.c_cflag     |=  CS8;

		tty0.c_cflag     &=  ~CRTSCTS;           // no flow control
		tty0.c_cc[VMIN]   =  1;                  // (1) read doesn't block
		tty0.c_cc[VTIME]  =  5;                  // (5) 0.5 seconds read timeout
		tty0.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
	}else{ // C1_TH
		// TODO:
	}
	
	// Make raw
	cfmakeraw(&tty0);

	// Flush Port, then applies attributes
	tcflush(handle, TCIFLUSH);
	if (tcsetattr(handle, TCSANOW, &tty0) != 0) {
   		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
	}

	if (fd) {
        recognize_from_file(fd, samplerate, config.vocoder_identification, handle);
    }else if(ad) {
        recognize_from_microphone(ad, config.vocoder_identification, handle);
	}else{
		usage();
    }
    
	fclose(handle);
	free(config.port);
	free(config.channel);

    return 0;
}

#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")
#include <windows.h>
//Windows Mobile has the Unicode main only
int
wmain(int32 argc, wchar_t * wargv[])
{
    char **argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc * sizeof(char *));
    for (i = 0; i < argc; i++) {
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len + 1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    //assuming ASCII parameters
    return main(argc, argv);
}
#endif
