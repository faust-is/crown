#include <stdio.h>
#include <string.h>
#include <assert.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <sys/select.h>
#endif

#include "err.h"
#include <ad.h>

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

#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

typedef struct _vocoder_info {
	const char* vocoder_string;
	short vocoder_identification;
} vocoder_info;

vocoder_info supported_vocoders[] = {
	{MELPE_RATE_2400_STR, MELPE_RATE_2400},
	{MELPE_RATE_1200_STR, MELPE_RATE_1200},
	{TETRA_RATE_4800_STR, TETRA_RATE_4800},
	{TETRA_RATE_4666_STR, TETRA_RATE_4666},
	/* Add more vocoders here! */
};

/*
 *  App_enableRealTime:
 *  Improve execution performance by locking memory pages, changing
 *  scheduling policy to FIFO, and raising the priority.
 */

static int priNorm;
static int schedNorm;

static int App_enableRealTime(int prio)
{
	int status;
	struct sched_param schedParams;

	/* lock memory pages in RAM */
	status = mlockall(MCL_CURRENT | MCL_FUTURE);

	if (status < 0) {
		goto leave;
	}

	/* save current schedule policy */
	priNorm = getpriority(PRIO_PROCESS, 0);
	schedNorm = sched_getscheduler(0);

	/* enable real-time shedule policy */
	schedParams.sched_priority = prio;

	status = sched_setscheduler(0, SCHED_FIFO, &schedParams);

	if (status < 0) {
		goto leave;
	}

leave:
	/* report error */
	if (status < 0) {
		E_FATAL("App_enableRealTime: error=%d\n", status);
	}
	return(status);
}



/*
 *  App_disableRealTime:
 *  Restore the normal scheduling policy and priority.
 */
static int App_disableRealTime(void)
{
	int status;
	struct sched_param schedParams;

	/* unlock memory pages */
	munlockall();

	/* restore original priority and scheduling policy */
	schedParams.sched_priority = priNorm;

	status = sched_setscheduler(0, schedNorm, &schedParams);

	if (status < 0) {
		E_FATAL("App_disableRealTime: error=%d\n", status);
	}

	return(status);
}



/* Sleep for specified msec */
static void
sleep_msec(int32 ms)
{
#if (defined(_WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
    Sleep(ms);
#else
    /* ------------------- Unix ------------------ */
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
#endif
}

/*
 * Main utterance processing loop:
 *     for (;;) {
 *        start utterance and wait for speech to process
 *        decoding till end-of-utterance silence will be detected
 *        print utterance result;
 *     }
 */

static void
recognize_from_microphone(int samplerate, int vocoder_identification, int tty_handle)
{
	ad_rec_t *ad;

	void* encoder;
	int status = 0;
	
	int rtmode = 0;
	
	struct timeval   tv0, tv1;
	uint32_t  sec, usec;
	uint32_t delta = 0;
	uint32_t max, min, sum, avg, n_samples;
	size_t frame_sp = 0;
	size_t frame_cb = 0;
	size_t written_cb, spot_cb;

	if ((ad = ad_open_dev(NULL, samplerate)) == NULL)
		E_FATAL("Failed to open audio device\n");
	if (ad_start_rec(ad) < 0)
		E_FATAL("Failed to start recording\n");

	// Размеры блоков на входе кодера и после кодирования
	frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
	frame_cb = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);

	E_INFO("frame_sp = %d\n", frame_sp);
	E_INFO("frame_cb = %d\n", frame_cb);

	if ((frame_sp == 0) || (frame_cb == 0))
		E_FATAL("Cannot determine IO size for codec %d\n", vocoder_identification);


	// Инициализация библиотеки
	status = vocoder_library_setup();
	if (status)
	{
		E_FATAL("Can't setup frontend, status=%d\n", status);
		return;
	}

	encoder = vocoder_create(vocoder_identification, VOCODER_DIRECTION_ENCODER);
	if (encoder == NULL) {
		E_FATAL("Can't create encoder\n");
		vocoder_library_destroy();
		return;
	}

	// Выделяем место под буффер
	short * buffer = (short *)malloc(frame_sp);
	unsigned char * c_frame = (unsigned char *)malloc(sizeof(unsigned char)*frame_cb);

	if (buffer == 0 || c_frame == 0) {
		E_FATAL("Can't allocate memory\n"); exit(1);
	}

	/* enable real-time mode */
	if (rtmode) {
		status = App_enableRealTime(rtmode);
	
		if (status < 0)
		    E_FATAL("Can't enable real-time mode, status=%d\n", status);
	}


	E_INFO("Ready...\n");
	
	max = 0; min = 0xFFFFFFFF; sum = 0; n_samples = 0; avg = 0;
	while(ad_read(ad, buffer,frame_sp) == frame_sp)
	{
		gettimeofday(&tv0, NULL);
		/* encoding  */
		status = vocoder_process(encoder, c_frame, buffer);
		if (status < 0 ) 
		{
			E_FATAL("Encoder failed, status=%d\n", status);
			break;
		}
		gettimeofday(&tv1, NULL);

		
		/* calculate statistics */
		sec = tv1.tv_sec - tv0.tv_sec;
		usec = (sec * 1000000) + tv1.tv_usec;
		delta = usec - tv0.tv_usec;
		if (delta > max) max = delta;
		if (delta < min) min = delta;
		sum += delta;

		spot_cb = 0;
		written_cb = 0;
		do {
    		spot_cb = write(tty_handle, &c_frame[spot_cb], frame_cb - spot_cb);
			//printf("spot: %d n: %d\n",written_cb, spot_cb);
			written_cb += spot_cb;
		} while (frame_cb != written_cb && spot_cb > 0);
		
		if (!(spot_cb > 0)){
			E_FATAL("Write failed, return value=%d\n", spot_cb);
		}
		
		n_samples ++;
	}
	//    for (;;) {
	//        if ((k = ad_read(ad, adbuf, 2048)) < 0)
	//            E_FATAL("Failed to read audio\n");
	//        
	//        sleep_msec(100);
	//    }
	/* disable real-time mode */
	if (rtmode) App_disableRealTime();
	if (n_samples) {
		avg = sum / n_samples;
	//	avg1 = sum1 / n_samples;
	}

	E_INFO("encoding stat: min=%d us, max=%d us, avg=%d us samples=%d\n", min, max, avg, n_samples);

	ad_close(ad);

	// Освобождаем память за собой
	vocoder_free(encoder);
	free(buffer);
	free(c_frame);
	vocoder_library_destroy();

}


int
main(int argc, char *argv[])
{
    int samplerate = 8000;
    int vocoder_identification = 0;
    int i, num_of_vocoders = sizeof(supported_vocoders) / sizeof(vocoder_info);
    
	struct termios tty0; //, tty1;

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
    // Определяем тип вокодера и скорость
    if(argc > 1){
	for (i=0; i<num_of_vocoders; i++){
	    if (!strcmp(argv[1], supported_vocoders[i].vocoder_string)){
		vocoder_identification = supported_vocoders[i].vocoder_identification;
		break;
	    }
	}
    }
    
    if (vocoder_identification == 0)
    {
	E_INFO("Bad codec string, the following are supported:\n");
	for (i=0; i<num_of_vocoders; i++)
		E_INFO("%s\n", supported_vocoders[i].vocoder_string);
		return -1;
    }
    
	// Работа с COM-портом
	int handle = open( "/dev/ttyUSB0", O_RDWR| O_NOCTTY );
	memset (&tty0, 0, sizeof(tty0));

	// Error Handling
	if (tcgetattr ( handle, &tty0 ) != 0 ) {
		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
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


	// Make raw
	cfmakeraw(&tty0);

	// Flush Port, then applies attributes
	tcflush(handle, TCIFLUSH);
	if (tcsetattr(handle, TCSANOW, &tty0) != 0) {
   		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
	}

    recognize_from_microphone(samplerate, vocoder_identification, handle);
    
	fclose(handle);
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
