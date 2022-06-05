#include "handler.h"

#include "err.h"

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
/*
 *  App_enableRealTime:
 *  Improve execution performance by locking memory pages, changing
 *  scheduling policy to FIFO, and raising the priority.
 */

static void* encoder = NULL;
static int status = 0, rtmode = 0;

static struct timeval   tv0, tv1;
static uint32_t  sec, usec;
static uint32_t delta = 0;
static uint32_t max, min, sum, avg, n_samples, n_frames;

// Размеры блоков на входе кодера и после кодирования
static size_t frame_sp = 0, frame_cb = 0;
static size_t written_cb, spot_cb;

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

static int
check_wav_header(char *header, int expected_sr)
{
    int sr;

    if (header[34] != 0x10) {
        E_ERROR("Input audio file has [%d] bits per sample instead of 16\n", header[34]);
        return 0;
    }
    if (header[20] != 0x1) {
        E_ERROR("Input audio file has compression [%d] and not required PCM\n", header[20]);
        return 0;
    }
    if (header[22] != 0x1) {
        E_ERROR("Input audio file has [%d] channels, expected single channel mono\n", header[22]);
        return 0;
    }
    sr = ((header[24] & 0xFF) | ((header[25] & 0xFF) << 8) | ((header[26] & 0xFF) << 16) | ((header[27] & 0xFF) << 24));
    if (sr != expected_sr) {
        E_ERROR("Input audio file has sample rate [%d], but decoder expects [%d]\n", sr, expected_sr);
        return 0;
    }
    return 1;
}

/*
 * Continuous recognition from a file
 */
void
recognize_from_file(FILE * rawfd, int samplerate, int vocoder_identification, int tty_handle)
{
    // Всяческие проверки формата файла
    //if (strlen(fname) > 4 && strcmp(fname + strlen(fname) - 4, ".wav") == 0) {
    	char waveheader[44];
		fread(waveheader, 1, 44, rawfd);
		if (!check_wav_header(waveheader, samplerate))
    	    E_FATAL("Failed to process file due to format mismatch.\n");
    //}


    
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

	// enable real-time mode
	if (rtmode) {
		status = App_enableRealTime(rtmode);
	
		if (status < 0)
		    E_FATAL("Can't enable real-time mode, status=%d\n", status);
	}
	
	max = 0; min = 0xFFFFFFFF; sum = 0; n_frames = 0; avg = 0; n_samples = frame_sp / sizeof(short);
	
	E_INFO("n_samples: %d\n",n_samples);
	E_INFO("Ready...\n");


	while(fread(buffer,1,frame_sp,rawfd) == frame_sp ){

		gettimeofday(&tv0, NULL);
		// encoding
		status = vocoder_process(encoder, c_frame, buffer);		
		if (status < 0 ) 
		{
			E_FATAL("Encoder failed, status=%d\n", status);
			break;
		}
		gettimeofday(&tv1, NULL);

		// Расчет статистики
		sec = tv1.tv_sec - tv0.tv_sec;
		usec = (sec * 1000000) + tv1.tv_usec;
		delta = usec - tv0.tv_usec;
		if (delta > max) max = delta;
		if (delta < min) min = delta;
		sum += delta;

		spot_cb = 0;
		written_cb = 0;

		// Запись сжатых кодером данных в COM-порт
		do {
    		spot_cb = write(tty_handle, &c_frame[written_cb], frame_cb - written_cb);
			written_cb += spot_cb;
		} while (frame_cb != written_cb && spot_cb > 0);
		
		if (!(spot_cb > 0)){
			E_FATAL("Write failed, return value=%d\n", spot_cb);
		}
	}

	// disable real-time mode
	if (rtmode) App_disableRealTime();
	if (n_frames) {
		avg = sum / n_frames;
	}

	E_INFO("encoding stat: min=%d us, max=%d us, avg=%d us frames=%d\n", min, max, avg, n_frames);

	fclose(rawfd);

	// Освобождаем память за собой
	vocoder_free(encoder);
	free(buffer);
	free(c_frame);
	vocoder_library_destroy();
 
}

void
recognize_from_microphone(ad_rec_t *ad, int vocoder_identification, int tty_handle)
{

/*
	int status = 0;
	
	int rtmode = 0;
	
	struct timeval   tv0, tv1;
	uint32_t  sec, usec;
	uint32_t delta = 0;
	uint32_t max, min, sum, avg, n_samples, n_frames;
	size_t frame_sp = 0;
	size_t frame_cb = 0;
	size_t written_cb, spot_cb;
*/
	//E_INFO("samplerate = %d\n",samplerate);
	
    printf("TODO: %d\n",vocoder_identification);


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

	// enable real-time mode
	if (rtmode) {
		status = App_enableRealTime(rtmode);
	
		if (status < 0)
		    E_FATAL("Can't enable real-time mode, status=%d\n", status);
	}
	
	max = 0; min = 0xFFFFFFFF; sum = 0; n_frames = 0; avg = 0; n_samples = frame_sp / sizeof(short);
	
	E_INFO("n_samples: %d\n",n_samples);
	E_INFO("Ready...\n");

	while(ad_read(ad, buffer,n_samples) == n_samples)
	{
		gettimeofday(&tv0, NULL);
		// encoding
		status = vocoder_process(encoder, c_frame, buffer);		
		if (status < 0 ) 
		{
			E_FATAL("Encoder failed, status=%d\n", status);
			break;
		}
		gettimeofday(&tv1, NULL);

		// calculate statistics
		sec = tv1.tv_sec - tv0.tv_sec;
		usec = (sec * 1000000) + tv1.tv_usec;
		delta = usec - tv0.tv_usec;
		if (delta > max) max = delta;
		if (delta < min) min = delta;
		sum += delta;

		spot_cb = 0;
		written_cb = 0;

		do {
    		spot_cb = write(tty_handle, &c_frame[written_cb], frame_cb - written_cb);
//			printf("spot: %d n: %d\n",written_cb, spot_cb);
//			for (int i = 0; i < spot_cb; i++){
//				printf("%02X",c_frame[i]);
//			}
//			printf("\n");

			written_cb += spot_cb;
		} while (frame_cb != written_cb && spot_cb > 0);
		
		if (!(spot_cb > 0)){
			E_FATAL("Write failed, return value=%d\n", spot_cb);
		}

		n_frames ++;
		if(n_frames >= 300) break;
	}

	// disable real-time mode
	if (rtmode) App_disableRealTime();
	if (n_frames) {
		avg = sum / n_frames;
	}

	E_INFO("encoding stat: min=%d us, max=%d us, avg=%d us frames=%d\n", min, max, avg, n_frames);

	ad_close(ad);

	// Освобождаем память за собой
	vocoder_free(encoder);
	free(buffer);
	free(c_frame);
	vocoder_library_destroy();
}
