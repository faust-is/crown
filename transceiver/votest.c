
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
 *  ======== App_enableRealTime ========
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
		printf("Error: App_enableRealTime: error=%d\n", status);
	}
	return(status);
}



/*
 *  ======== App_disableRealTime ========
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
		printf("Error: App_disableRealTime: error=%d\n", status);
	}

	return(status);
}

static int
check_wav_header(char *header, int expected_sr)
{
    int sr;

    if (header[34] != 0x10) {
        printf("Input audio file has [%d] bits per sample instead of 16\n", header[34]);
        return 0;
    }
    if (header[20] != 0x1) {
        printf("Input audio file has compression [%d] and not required PCM\n", header[20]);
        return 0;
    }
    if (header[22] != 0x1) {
        printf("Input audio file has [%d] channels, expected single channel mono\n", header[22]);
        return 0;
    }
    sr = ((header[24] & 0xFF) | ((header[25] & 0xFF) << 8) | ((header[26] & 0xFF) << 16) | ((header[27] & 0xFF) << 24));
    if (sr != expected_sr) {
        printf("Input audio file has sample rate [%d], but decoder expects [%d]\n", sr, expected_sr);
        return 0;
    }
    return 1;
}
int
main (int argc, char **argv)
{
	void* encoder;
	void* decoder;
	int status = 0;
	int vocoder_identification = 0;
	int rtmode = 0;
	char * in_file = NULL;
	char * out_file = NULL;
	struct timeval   tv0, tv1, tv2;
	uint32_t  sec, usec;
	uint32_t delta = 0;
	uint32_t max, min, sum, avg, n_samples;
	uint32_t max1,min1,sum1,avg1;
	size_t frame_sp = 0;
	size_t frame_cb = 0;
	int i, num_of_vocoders = sizeof(supported_vocoders) / sizeof(vocoder_info);
	
	
	printf("Vocoders v1.0\n");
	if (argc !=4 && argc !=5) {
		printf("usage: %s codec_string input_pcm output_pcm [RT prio=0]\n", argv[0]);
		return 0;
	}
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
		for (i=0; i<num_of_vocoders; i++) fprintf(stderr, "%s ", supported_vocoders[i].vocoder_string);
		fprintf(stderr, "\n");
		return -1;
	}
	frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
	frame_cb = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);
	printf("frame_sp = %x\n", frame_sp);
	printf("frame_cb = %x\n", frame_cb);
	if ((frame_sp == 0) || (frame_cb == 0)) {
		fprintf (stderr, "Cannot determine IO size for codec %d\n", vocoder_identification);
		return -1;
	}

	in_file = argv[2];
	out_file = argv[3];
	
	if (argc == 5) rtmode = atoi(argv[4]);
	
	
	if ( in_file == NULL && out_file == NULL)
	{
		fprintf (stderr, "Specify input and output\n");
		return -1;
	}

	FILE * _in = fopen(in_file,"r");
	if( _in == NULL)
	{
		fprintf (stderr, "Can't open input\n");
		return -1;
	}

	FILE * _out = fopen(out_file,"w");
	if( _out == NULL)
	{
		fprintf (stderr, "Can't open output\n");
		return -1;
	}

	// Проверяем наличие заголовка, а также валидность входных аудиоданных
	char waveheader[44];
	fread(waveheader,1,44,_in);
	if(!check_wav_header(waveheader,8000)){
		fprintf(stderr,"Failed to process file %s due to format mismatch.\n",in_file);
		return -1;
	}
	// Заголовок второго файла будет точно таким же
	fwrite(waveheader,1,44,_out);

	status = vocoder_library_setup();
	if (status)
	{
		fprintf (stderr, "Can't setup frontend, status=%d\n", status);
		return -1;
	}
	

	encoder = vocoder_create(vocoder_identification, VOCODER_DIRECTION_ENCODER);
	if (encoder == NULL) {
		fprintf (stderr, "Can't create encoder\n");
		vocoder_library_destroy();
		return -1;
	}

	decoder = vocoder_create(vocoder_identification, VOCODER_DIRECTION_DECODER);
	if (decoder == NULL) {
		fprintf (stderr, "Can't create decoder\n");
		vocoder_library_destroy();
		return -1;
	}

	short * buffer = (short *)malloc(frame_sp);
	unsigned char * c_frame = (unsigned char *)malloc(frame_cb);
	if (buffer == 0 || c_frame == 0) 
	{
		fprintf (stderr, "Can't allocate memory\n"); exit(1);
	}

	/* enable real-time mode */
	if (rtmode) 
	{
		status = App_enableRealTime(rtmode);
	
		if (status < 0) {
		    fprintf (stderr, "Can't enable real-time mode, status=%d\n", status);
		}
	}
	
	max = 0; min = 0xFFFFFFFF; sum = 0; n_samples = 0; avg = 0;
	max1 =0; min1 = 0xFFFFFFFF; sum1 = 0; avg1=0;
	while( fread(buffer,1,frame_sp,_in) == frame_sp )
	{
		gettimeofday(&tv0, NULL);
		/* encoding  */
		status = vocoder_process(encoder, c_frame, buffer);
		if (status < 0 ) 
		{
			fprintf (stderr, "Encoder failed, status=%d\n", status);
			break;
		}
		gettimeofday(&tv1, NULL);
		/* decoding */
		status = vocoder_process(decoder, c_frame, buffer);
		if (status < 0 ) 
		{
			fprintf (stderr, "Decoder failed, status=%d\n", status);
			break;
		}
		gettimeofday(&tv2, NULL);
		
		/* calculate statistics */
		sec = tv1.tv_sec - tv0.tv_sec;
		usec = (sec * 1000000) + tv1.tv_usec;
		delta = usec - tv0.tv_usec;
		if (delta > max) max = delta;
		if (delta < min) min = delta;
		sum += delta;
		sec = tv2.tv_sec - tv1.tv_sec;
		usec = (sec * 1000000) + tv2.tv_usec;
		delta = usec - tv1.tv_usec;
		if (delta > max1) max1 = delta;
		if (delta < min1) min1 = delta;
		sum1 += delta;
		n_samples ++;
		fwrite(buffer,1,frame_sp,_out);
	}

	/* disable real-time mode */
	if (rtmode) App_disableRealTime();
	if (n_samples) {
		avg = sum / n_samples;
		avg1 = sum1 / n_samples;
	}
	printf("encoding stat: min=%d us, max=%d us, avg=%d us samples=%d\n", min, max, avg, n_samples);
	printf("decoding stat: min=%d us, max=%d us, avg=%d us\n", min1, max1, avg1);
	
	vocoder_free(encoder);
	vocoder_free(decoder);
	
	vocoder_library_destroy();
	free(buffer);
	free(c_frame);
	fclose(_in);
	fclose(_out);
	return 0;
}
