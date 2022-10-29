#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>


#include "err.h"
#include "ini.h"
#include "handler.h"


#define TIME_DEFAULT 1000
const char *const file_name_ini = "continuous.ini";

// TODO: default setting
typedef struct
{
	int is_tx;

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

	if (pconfig->is_tx)
	{
		if (MATCH("param_tx", "port")) {
        	pconfig->serial_port = strdup(value);
    	} else if (MATCH("param_tx", "rate")) {
			for (int i = 0; i < num_of_vocoders; i++){
				if (!strcmp(value, supported_vocoders[i].vocoder_string)){
					pconfig->vocoder_identification = supported_vocoders[i].vocoder_identification;
					break;
				}
			}
		} else if (MATCH("param_tx","nblocks")){
			pconfig->number_block_for_out = strtoul(value, NULL, 10);
		} else if (MATCH("addr_tx", "ip")) {
			pconfig->ip = strdup(value);
		} else if (MATCH("addr_tx", "port")) {
			pconfig->ip_port = strtoul(value, NULL, 10);
    	} else {
        	return 0;
    	}
	}else{
		if (MATCH("param_rx", "port")) {
        	pconfig->serial_port = strdup(value);
    	} else if (MATCH("param_rx", "rate")) {
			for (int i = 0; i < num_of_vocoders; i++){
				if (!strcmp(value, supported_vocoders[i].vocoder_string)){
					pconfig->vocoder_identification = supported_vocoders[i].vocoder_identification;
					break;
				}
			}
		} else if (MATCH("param_rx","nblocks")){
			pconfig->number_block_for_out = strtoul(value, NULL, 10);
		} else if (MATCH("addr_rx", "ip")) {
			pconfig->ip = strdup(value);
		} else if (MATCH("addr_rx", "port")) {
			pconfig->ip_port = strtoul(value, NULL, 10);
    	} else {
        	return 0;
    	}
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


static struct sockaddr_in sock_proxy, sock_client;
static int sock_handle;

static ssize_t send_to_socket(const char* data, size_t size){

	return sendto(sock_handle, data, size,
    0, (const struct sockaddr *) &sock_proxy, 
    sizeof(sock_proxy)); 
}

static ssize_t recv_from_socket(char* buffer, size_t size_buf){

	int size_from_client;
	ssize_t size_reading = recvfrom(sock_handle, (char *)buffer, size_buf, 
                0, (struct sockaddr *) &sock_client,
                &size_from_client);
	return size_reading;
}


static void print_usage(){
	E_INFO_NOFN("\nplease, use:");
	E_INFO_NOFN("--tx [flag for transmitter]\n");
	E_INFO_NOFN("--rx [flag for receiver]\n");
	E_INFO_NOFN("-t or --t [time for writenning from SG1 (in sec), default value: %d sec]\n", TIME_DEFAULT);
}

int main(int argc, char *argv[])
{

	static configuration config;
	static struct termios tty_orig;
	int opt, tty_handle;
	int time = TIME_DEFAULT; /* default value in sec */

	signal(SIGINT, sig_int_handler);

	E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
		/* parse command line arguments */
	static struct option long_options[] =
	{
		{"tx", no_argument, &config.is_tx, 1},
		{"t",required_argument, NULL, 't'},
		{"rx", no_argument, &config.is_tx, 0},
		{NULL, 0, NULL, 0}
	};


	while ((opt = getopt_long(argc, argv, "t:",long_options, NULL)) != EOF) {
		switch (opt) {
			case 1:
			case 0:
				break;
			case 't': // необязательный параметр
				time = atoi(optarg);
				break;
			default: 
				print_usage();
				exit(EXIT_FAILURE);
		}
	}

	if (time == TIME_DEFAULT){
		E_INFO("time writenning/reading (default): %d sec\n",TIME_DEFAULT);
	}

	/* parse *.ini */
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
	//if((sock_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
	if((sock_handle = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) < 0){
		E_FATAL("socket creation failed: %s\n", strerror(errno));
	} 


	/* set port and IP for UDP socket*/
	memset (&sock_proxy, 0, sizeof(sock_proxy));
	sock_proxy.sin_family = AF_INET; /* IPv4 */
	sock_proxy.sin_port = htons(config.ip_port);
	sock_proxy.sin_addr.s_addr = inet_addr(config.ip);

	E_INFO("IPv4:%s:%d\n",inet_ntoa(sock_proxy.sin_addr),config.ip_port);

	/* check IPv4 addres*/
	if (sock_proxy.sin_addr.s_addr == INADDR_NONE ) {
    	E_FATAL("bad address: %s\n", config.ip);
 	}

	if(config.is_tx){
		channel_to_socket((uint16_t)time, config.vocoder_identification, tty_handle, send_to_socket, config.number_block_for_out);
	}else{
		if(bind(sock_handle, (struct sockaddr*)&sock_proxy, sizeof(sock_proxy)) < 0) {
			E_FATAL("failed to bind socket %s\n", strerror(errno));
		}
	
		sock_to_channel((uint16_t)time, config.vocoder_identification, tty_handle, recv_from_socket, config.number_block_for_out);

	}
	
	close(sock_handle);
	
	/* перед выходом устанавливаем старые настройки */
	if (tcsetattr(tty_handle, TCSANOW, &tty_orig) != 0) {
   		E_FATAL("error: %d from tcsetattr %s\n", errno, strerror(errno));
	}

	/* close serial port */
	close(tty_handle);

	/* free config ini */
	free(config.serial_port);
	free(config.ip);

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
