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
 
    short * buffer = NULL;
    unsigned char * c_frame = NULL;
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


    if (!strcmp(config.channel, CHANNEL_FL_STR)){
        // С1-ФЛ

        frame_sp = vocoder_get_input_size(config.vocoder_identification, VOCODER_DIRECTION_ENCODER);
        frame_cbit = vocoder_get_input_size(config.vocoder_identification, VOCODER_DIRECTION_DECODER);

        E_INFO("frame_sp = %d\tframe_cb = %d\n", frame_sp,frame_cbit);

        if ((frame_sp == 0) || (frame_cbit == 0)) {
            E_FATAL("Cannot determine IO size for codec %d\n", config.vocoder_identification);
            return -1;
        }

        if (vocoder_library_setup()) {
            E_FATAL("cannot setup library for codec %d\n", config.vocoder_identification);
            return -1;
        }
        dec = vocoder_create(config.vocoder_identification, VOCODER_DIRECTION_DECODER);
        if (dec == NULL) {
            E_FATAL("codec %d cannot be created\n", config.vocoder_identification);
            vocoder_library_destroy();
            return -1;
        }

        buffer = (short *)malloc(frame_sp);
        c_frame = (unsigned char *)malloc(sizeof(unsigned char)*frame_cbit);
    }else{
        // TODO: С1-ТЧ
    }

    int h = write_wav_stereo_header(_out, SAMPLE_RATE, SAMPLE_RATE * (int32_t)t , 1);
    if (h != 0)
    {
        E_WARN("output *.wav file is false\n");
        /* code */
    }
    


    // Работа с COM-портом
    struct termios tty0;
	int handle = open(config.port, O_RDWR| O_NOCTTY );
    //int handle = open(config.port, O_RDWR| O_NOCTTY| O_NONBLOCK); // в таком случае, функция сразу возвращает управление

	memset (&tty0, 0, sizeof(tty0));

	// Error Handling
	if (tcgetattr ( handle, &tty0 ) != 0 ) {
		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
        return -1;
	}

	//tty1 = tty0;


    if (!strcmp(config.channel, CHANNEL_FL_STR)){

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

    	// TODO: Make raw
	    cfmakeraw(&tty0);
    }else{
        // TODO: С1-ТЧ
        E_INFO("Setting speed 115200\n");
        // Set Baud Rate
        cfsetospeed (&tty0, (speed_t)B115200);
        cfsetispeed (&tty0, (speed_t)B115200);

        // Setting other Port Stuff
        /*
        tty0.c_cflag     &=  ~PARENB;            // Make 8n1
        tty0.c_cflag     &=  ~CSTOPB;
        tty0.c_cflag     &=  ~CSIZE;
        tty0.c_cflag     |=  CS8;

        tty0.c_cflag     &=  ~CRTSCTS;           // no flow control
        tty0.c_cc[VMIN]   =  1;                  // read doesn't block
        tty0.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
        tty0.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines
        
        tty0.c_lflag &= ~(ECHO|ICANON|ISIG);
*/
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
    }

	// Flush Port, then applies attributes
	tcflush(handle, TCIFLUSH);
	if (tcsetattr(handle, TCSANOW, &tty0) != 0) {
   		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
        return -1;
	}


    if (!strcmp(config.channel, CHANNEL_FL_STR)){
        
        int n = 0, spot = 0;
        int32_t n_blocks_count = 0;
        // Расчет количества отсчетов в wav-файле
        int32_t n_block_max = floor(2 * SAMPLE_RATE * t / frame_sp);

        do {
            n = read(handle, &c_frame[n], frame_cbit - spot);
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
            }

        } while((n >= 0) &&(n_blocks_count < n_block_max));

        //E_INFO("decoding stat: min=%d us, max=%d us, avg=%d us samples=%d\n", min, max, avg, n_samples);
        E_INFO("decoding stat: samples=%d\n", n_blocks_count);
    }else{
        // TODO: C1-ТЧ
        //
        // Запись сжатых кодером данных в COM-порт
        E_INFO("Write commant to COM-port\n");
        int i,count;
        uint32_t marker =  0x99699699;
        uint16_t size;
        unsigned char code;
/*
        const uint64_t marker = 0x99966999;
        uint16_t size = 9;
        // TODO: malloc
		unsigned char command[9] = {
                            0xB1,0x01,
                            0xE8,0x03,
                            0x67,0x00,
                            }; 
        unsigned char code = 0;
        for (size_t i = 0; i < size; i++)
        {
            code +=command[i];
        }
        code = -code;
        
        // TODO:
        size = htobe16(size);

 */
        unsigned char command[15] = {
            0x99,
            0x96,
            0x69,
            0x99,
            0x09,
            0x00,
            0xB1, //1
            0x01, //2
            0x08, //3
            0x01, //4
            0x01, //5 - 65
            0x00,
            0x67,
            0x00,
            0xdd //code - 7a
        };

        int s = write(handle, &command,15);
        printf("%d\n",s);

        E_INFO("Reeding answer from COM-port\n");
        
        do{

            uint32_t tmarker = UINT64_C(0);
            // 1. читаем и сверяем маркер
            s = read(handle, &tmarker, sizeof(uint32_t));
            if((sizeof(marker) != s) || (tmarker != marker)){
                E_INFO("Error reading marker: %x\n",tmarker);
                goto out;
            }
            
            // 2. длина фрейма
            s = read(handle, &size, sizeof(size));
            if(sizeof(size) != s){
                E_INFO("Error reading length of frame body\n");
                goto out;
            }

            // 3. тело фрейма
            count = 0; code = 0;
            do {
                // TODO: malloc для буффера
                s = read(handle, &command, 1);
                if (s < 1) {
                    E_INFO("Error reading frame body: %d из %d\n",count,size);
                    goto out;
                }

                count += s;
                code += command[0];
            } while(count < size - 1);

            // 4. контрольная сумма - дополнение до двух
            s = read(handle, &command, 1);
            if((s < 1) || (command[0] != (UINT8_MAX - code + 1))){
                
                E_INFO("Error check sum: %x vs %x\n",command[0], UINT8_MAX - code + 1); // ~code+1
                goto out;
            }

            E_INFO("frame read successfully, size %d\n",size);

        }while (TRUE);
        
    out:
        E_INFO("last number byte reading: %d\n",s);
    }

    fclose(handle);
    if (_out) fclose(_out);

    vocoder_free(dec);
    if(buffer) free(buffer);
    if(c_frame) free(c_frame);
    vocoder_library_destroy();
    return 0;
}
