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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include <semaphore.h>

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
recognize_from_microphone(int samplerate, int vocoder_identification)
{
    ad_rec_t *ad;
    int16 adbuf[2048];
    int32 k;
    char const *hyp;
    
    // размер блока на входе кодера
    size_t frame_sp = 0;
    // размер блока после кодирования
    size_t frame_cb = 0;

    if ((ad = ad_open_dev(NULL, samplerate)) == NULL)
        E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

    frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    frame_cb = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);
    
    E_INFO("frame_sp = %x\n", frame_sp);
    E_INFO("frame_cb = %x\n", frame_cb);
    
    if ((frame_sp == 0) || (frame_cb == 0))
	E_FATAL("Cannot determine IO size for codec %d\n", vocoder_identification);
	
    for (;;) {
        if ((k = ad_read(ad, adbuf, 2048)) < 0)
            E_FATAL("Failed to read audio\n");
        
        sleep_msec(100);
    }
    ad_close(ad);
}


int
main(int argc, char *argv[])
{
    int samplerate = 16000;
    int vocoder_identification = 0;
    int i, num_of_vocoders = sizeof(supported_vocoders) / sizeof(vocoder_info);
    
    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
    for (i=0; i<num_of_vocoders; i++)
    {
	if (!strcmp(argv[1], supported_vocoders[i].vocoder_string))
	{
		vocoder_identification = supported_vocoders[i].vocoder_identification;
		break;
	}
    }
    
    if (vocoder_identification == 0)
    {
	fprintf (stderr, "Bad codec string, the following are supported: ");
	for (i=0; i<num_of_vocoders; i++) fprintf(stderr, "%s ", 		supported_vocoders[i].vocoder_string);
	fprintf(stderr, "\n");
	return -1;
    }
    
    recognize_from_microphone(samplerate, vocoder_identification);
    
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
