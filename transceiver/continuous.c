
#include <string.h>
#include <assert.h>
//#if defined(_WIN32) && !defined(__CYGWIN__)
//#include <windows.h>
//#else
#include <sys/select.h>
//#endif

#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h> 

#include "err.h"
#include "ini.h"
#include "handler.h"

const char *const file_name_ini = "continuous.ini";

// TODO: default setting
typedef struct
{
    const char* serial_port;
    unsigned int vocoder_identification; // from rate
	const char* ip;				// for UDP socket
	unsigned short ip_port;
	unsigned short number_block_for_out;

} configuration;


static int handler(void* user, const char* section, const char* name,const char* value)
{
    configuration* pconfig = (configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH("param", "port")) {
        pconfig->serial_port = strdup(value);
    } else if (MATCH("param", "rate")) {
		for (int i = 0; i < num_of_vocoders; i++){
			if (!strcmp(value, supported_vocoders[i].vocoder_string)){
				pconfig->vocoder_identification = supported_vocoders[i].vocoder_identification;
				break;
			}
		}
	} else if (MATCH("param","nblocks")){
		pconfig->number_block_for_out = strtoul(value, NULL, 10);
	} else if (MATCH("addr", "ip")) {
		pconfig->ip = strdup(value);
	} else if (MATCH("addr", "port")) {
		pconfig->ip_port = strtoul(value, NULL, 10);
    } else {
        return 0;
    }

    return 1;
}


int set_serial_attrs(int tty_handle, speed_t speed, struct termios * tty_orig){

	struct termios tty0;
	memset (&tty0, 0, sizeof(tty0));

	/* error handling */
	if(tcgetattr (tty_handle, &tty0) != 0){
		E_FATAL("error: %d from tcgetattr %s\n", errno, strerror(errno));
	}

	/* save initial setting*/
	if (NULL != tty_orig){
		memcpy(tty_orig, &tty0, sizeof(tty0));
	}
	
	
	cfsetospeed (&tty0, speed);
    cfsetispeed (&tty0, speed);

	tty0.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
	tty0.c_cflag &= ~CSIZE;
	tty0.c_cflag |= CS8;         /* 8-bit characters */
	tty0.c_cflag &= ~PARENB;     /* no parity bit */
	tty0.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty0.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty0.c_iflag = IGNPAR;//&= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty0.c_lflag = 0;//&= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty0.c_oflag = 0;//&= ~OPOST;

	/* fetch bytes as they become available */
	tty0.c_cc[VMIN] = 0;
	tty0.c_cc[VTIME] = 10;

	/* flush port, then applies attributes */
	tcflush(tty_handle, TCIFLUSH);
	if (tcsetattr(tty_handle, TCSANOW, &tty0) != 0) {
   		E_FATAL("error: %d from tcsetattr %s\n", errno, strerror(errno));
	}

	/* all ok*/
	return 0;
}


static struct sockaddr_in sock0;
static int sock_handle;

static int handler_send_to(const char* data, size_t size){

	E_INFO("sending %zu bytes to socket\n", size);

	return sendto(sock_handle, data, size,
    MSG_CONFIRM, (const struct sockaddr *) &sock0, 
    sizeof(sock0)); 
}


int main(int argc, char *argv[])
{

	configuration config;
	static struct termios tty_orig;
	int tty_handle;

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
	if(ini_parse(file_name_ini, handler, &config) < 0){
		E_FATAL("can't load/parse %s\n",file_name_ini);
	}else{
		E_INFO("config loaded from %s: serial port = %s, rate = %d\n", file_name_ini, config.serial_port, config.vocoder_identification);
	}
    
	/* open serial port */
	tty_handle = open(config.serial_port, O_RDWR| O_NOCTTY );

	/* check handle and set speed */
	set_serial_attrs(tty_handle, (speed_t)B115200, &tty_orig);

	/* open UDP socket*/
	if((sock_handle = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) < 0){
        E_FATAL("socket creation failed\n");
    } 

	/* set port and IP for UDP socket*/
	memset (&sock0, 0, sizeof(sock0));
	sock0.sin_family = AF_INET;
    sock0.sin_port = htons(config.ip_port);
    sock0.sin_addr.s_addr = inet_addr(config.ip); // 


	// TODO: запись 10 сек
	send_command(10, tty_handle);
	send_voice_from_channel_to_socket(config.vocoder_identification, tty_handle, handler_send_to, config.number_block_for_out);
	

	/* перед выходом устанавливаем старые настройки */
	if (tcsetattr(tty_handle, TCSANOW, &tty_orig) != 0) {
   		E_FATAL("error: %d from tcsetattr %s\n", errno, strerror(errno));
	}

    /* close serial port */
	fclose(tty_handle);

	// free config ini
	//if(NULL != config.serial_port) free(config.serial_port);
	//if(NULL != config.ip) free(config.ip);

    return 0;
}

/*
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
*/
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
