
#include <vocoderAPI.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>

#include <stdint.h>

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
    /* add more vocoders here! */
};

int
main (int argc, char** argv)
{
    void* dec;
    short vocoder_identification = 0;
    int i, num_of_vocoders = sizeof(supported_vocoders) / sizeof(vocoder_info);
    //FILE * _in = stdin;
    FILE * _out = stdout;
    size_t frame_sp = 0;
    size_t frame_cbit = 0;

    uint32_t n_samples;

    if (argc > 1) {
        for (i=0; i<num_of_vocoders; i++)
        {
            if (!strcmp(argv[1], supported_vocoders[i].vocoder_string))
            {
                vocoder_identification = supported_vocoders[i].vocoder_identification;
                printf("%s\n",supported_vocoders[i].vocoder_string);
                break;
            }
        }
        if (vocoder_identification == 0)
        {
            fprintf (stderr, "Bad codec string, supported are: ");
            for (i=0; i<num_of_vocoders; i++) fprintf(stderr, "%s ", supported_vocoders[i].vocoder_string);
            fprintf(stderr, "\n");
            return -1;
        }
    } else {
        vocoder_identification = TETRA_RATE_4800;
        // TODO:
        printf("%s\n", TETRA_RATE_4800_STR);
    }

    frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    frame_cbit = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if ((frame_sp == 0) || (frame_cbit == 0)) {
        fprintf (stderr, "Cannot determine IO size for codec %d\n", vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        fprintf (stderr, "cannot setup library for codec %d\n", vocoder_identification);
        return -1;
    }
    dec = vocoder_create(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if (dec == NULL) {
        fprintf (stderr, "codec %d cannot be created\n", vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }

    short * buffer = (short *)malloc(frame_sp);
    unsigned char * c_frame = (unsigned char *)malloc(sizeof(unsigned char)*frame_cbit);

    // Работа с COM-портом
    struct termios tty0;
	int handle = open( "/dev/ttyUSB0", O_RDWR| O_NOCTTY );
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

	// Make raw
	cfmakeraw(&tty0);

	// Flush Port, then applies attributes
	tcflush(handle, TCIFLUSH);
	if (tcsetattr(handle, TCSANOW, &tty0) != 0) {
   		fprintf(stderr,"Error: %d from tcgetattr %s", errno, strerror(errno));
        return -1;
	}

    printf("Ready...\n");

    int n = 0, spot = 0;
    n_samples = 0;
    do {
        n = read(handle, &c_frame[n], frame_cbit - n);
        //sprintf( &response[spot], "%c", buf );
        spot += n;

        if(spot == frame_cbit){
            vocoder_process(dec, c_frame, buffer);
            fwrite(buffer,1,frame_sp,_out);
            fflush(_out);

            spot = 0;
        }

    } while(n > 0);

    //E_INFO("decoding stat: min=%d us, max=%d us, avg=%d us samples=%d\n", min, max, avg, n_samples);
    printf("decoding stat: samples=%d\n", n_samples);
 /*   while( fread(c_frame,sizeof(unsigned char),frame_cbit,_in) == frame_cbit )
    {
      vocoder_process(dec, c_frame, buffer);
      fwrite(buffer,1,frame_sp,_out);
      fflush(_out);
    }
*/
    vocoder_free(dec);
    free(buffer);
    free(c_frame);
    vocoder_library_destroy();
    return 0;
}
