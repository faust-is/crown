#include "wav.h"
#include "err.h"
#include <vocoderAPI.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <getopt.h>
#include <math.h>
#include "ini.h"
#include "handler.h"
#include "convert.h"

#define CHANNEL_FL_STR "C1_FL"
#define CHANNEL_TH_STR "C1_TH"

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
//    } else if (strcmp(pconfig->channel, CHANNEL_FL_STR) == 0 && MATCH(pconfig->channel, "rate")) {
    } else if (MATCH(pconfig->channel, "rate")) {
		for (int i = 0; i < num_of_vocoders; i++){
			if (!strcmp(value, supported_vocoders[i].vocoder_string)){
				pconfig->vocoder_identification = supported_vocoders[i].vocoder_identification;
				break;
			}
		}
    } else {
        return 0;
    }

    return 1;
}

static void 
usage(void) {
	printf("\t-f <filename> if you want save WAV-file\n");
	printf("\t-c <\"%s\" or \"%s\">\n",CHANNEL_FL_STR, CHANNEL_TH_STR);
	printf("\t-l <s> interval time\n");
}

int
main (int argc, char** argv)
{   
    void* vocoder = NULL;

    int opt, status;
    short * buffer = NULL;
    unsigned char * c_frame = NULL;
    int frame_sp = 0;
    int frame_cbit = 0;

	configuration config;
    config.port = NULL;
    FILE * _out = NULL;

    uint16_t time;
    

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);
    
    if(argc == 1){
		usage();
		return 3;
	}	

    /*
    / Разбор параметров командной строки:
    /   'c' - выбор канала (ФЛ или ТЧ)
    /   'f' - файл вывода
    /   'l' - время записи [сек]
    */

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
            time = (uint16_t)strtoul(optarg, NULL,10);
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
        E_FATAL("Cannot determine IO size for codec %d\n", config.vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        E_FATAL("cannot setup library for codec %d\n", config.vocoder_identification);
        return -1;
    }

    buffer = (short *)malloc(frame_sp);
    c_frame = (unsigned char *)malloc(sizeof(unsigned char)*frame_cbit);

    if (!strcmp(config.channel, CHANNEL_FL_STR)){

        /*
        / В канале ФЛ данные передаются в сжатом вокодером виде,
        / поэтому (при необходимости) нужно расшифровать.
        / 
        / Создаем декодер, тип (скорость) которого задается в инишнике
        */

        vocoder = vocoder_create(config.vocoder_identification, VOCODER_DIRECTION_DECODER);

        status = write_wav_stereo_header(_out, SAMPLE_RATE, SAMPLE_RATE * time , AUDIO_FORMAT_PCM);
        if (status){ E_WARN("output *.wav file is false\n");}
    }else{
        /*
        / В канале ТЧ данные передаются в сжатом виде (A-law),
        / поэтому перед тем как использовать вокодер, нужно привести данные в формат PCM.
        / 
        / Если ... то на выходе будет сжатые вокодером данные.
        / Создаем кодер, тип (скорость) которого задается в инишнике
        */

        vocoder = vocoder_create(config.vocoder_identification, VOCODER_DIRECTION_ENCODER);
        
        //status = write_wav_stereo_header(_out, SAMPLE_RATE, SAMPLE_RATE * time , AUDIO_FORMAT_ALAW);
        status = write_wav_stereo_header(_out, SAMPLE_RATE, SAMPLE_RATE * time , AUDIO_FORMAT_PCM);
        if (status){ E_WARN("output *.wav file is false\n");}
    }


    if (vocoder == NULL) {
        E_FATAL("codec %d cannot be created\n", config.vocoder_identification);
        vocoder_library_destroy();
        return 101;
    }
    

    // Работа с COM-портом
    struct termios tty0;
	int handle = open(config.port, O_RDWR| O_NOCTTY );
    //int handle = open(config.port, O_RDWR| O_NOCTTY| O_NONBLOCK); // в таком случае, функция сразу возвращает управление

	memset (&tty0, 0, sizeof(tty0));

	// Error Handling
	if (tcgetattr ( handle, &tty0 ) != 0 ) {
		E_FATAL("Error: %d from tcgetattr %s", errno, strerror(errno));
        return 102;
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

    // Расчет количества отсчетов в wav-файле
    int n_block_max = floor(2 * SAMPLE_RATE * time / frame_sp);

    if (!strcmp(config.channel, CHANNEL_FL_STR)){
        
        int n = 0, spot = 0;
        int n_blocks_count = 0;
        
        do {
            n = read(handle, &c_frame[n], frame_cbit - spot);
            spot += n;
            if(spot == frame_cbit){
    //            printf("%d ",n_blocks);
    //            for (int i = 0; i < spot; i++)
    //                printf("%02X",c_frame[i]);
    //            printf("\n");
            
                status = vocoder_process(vocoder, c_frame, buffer);
                if (status < 0 ) 
                {
                    E_FATAL("Encoder failed, status=%d\n", status);
                    break;
                }
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
        E_INFO("Write commant to COM-port\n");

        int i,count1, count2;
        int n_samples = frame_sp / sizeof(short);
        int n_read_bytes;
        int n_blocks,n_frames;
        const uint32_t marker =  0x99699699;
        uint16_t size;
        unsigned char code, byte;

        unsigned char command[15] = { 
            0x99, // Маркер
            0x96, // 
            0x69, //
            0x99, //
            0x09, // Длина последующей части посылки (младший байт)
            0x00, // Длина последующей части посылки (старший байт)
            0xB1, // Номер функции, адрес модема
            0x01, // Последовательный номер посылки
            0x08, // Номер команды: 8 - включение/отключение записи сигнала ТЧ
            0x01, // 1 - включение
            0x01, // Время [сек], младший байт 
            0x00, // Время [сек], старший байт
            0x67, // Пороговый уровень речи (младший байт)
            0x00, // Пороговый уровень речи (старший байт)
            0xdd  // Контрольная сумма // 0xdd
        };
        /*
        / Перед отправкой команды нужно указать (изменить) время записи,
        / после пересчитать контрольную сумму
        */

        //time = htobe16(time);
        memcpy(&command[10],&time, sizeof(uint16_t));

        code = 0;
        for (i = 6; i < 6 + 8; i++){
            code +=command[i];
        }
        command[14] = ~code + 1;

        for (i = 6; i < 6 + 9; i++){
            printf("\t%d: %02x \n",i,command[i]);
        }

        /*
        / Отправляем команду в модем
        */

        n_read_bytes = write(handle, &command,15);
        if (n_read_bytes != 15){
            E_INFO("Error write command\n");
            goto out;
        }
        

        E_INFO("Reeding answer from COM-port\n");
        
        count2 = 0; n_blocks = 0; n_frames = 0;
        do{

            uint32_t tmarker = 0;
            // 1. читаем и сверяем маркер
            n_read_bytes = read(handle, &tmarker, sizeof(uint32_t));
            if((sizeof(marker) != n_read_bytes) || (tmarker != marker)){
                E_INFO("Error reading marker: %x\n",tmarker);
                goto out;
            }
            
            // 2. длина фрейма
            n_read_bytes = read(handle, &size, sizeof(size));
            if(sizeof(size) != n_read_bytes){
                E_INFO("Error reading length of frame body\n");
                goto out;
            }


            // 3. тело фрейма
            count1 = 0; code = 0;
            
            /*
            / Первые четыре байта несут информацию  о адресе модема, порядковом номере фрейма и т.д.
            / Эту информацию никак не обрабатываем, пробрасываем
            */
            for (i = 0; i < 4; i++)
            {
                n_read_bytes = read(handle, &byte, 1);
                                if (n_read_bytes < 1) {
                    E_INFO("Error reading frame body: %d из %d\n",count1,size);
                    goto out;
                }

                /*
                / При расчете контрольной суммы необходимо выполнить сложение по модулю 256
                / каждого байта в фрейме
                */
                code += byte;
                count1 += n_read_bytes;
            }
            

            while(count1 < size - 1){

                n_read_bytes = read(handle, &byte, 1);
                if (n_read_bytes < 1) {
                    E_INFO("Error reading frame body: %d из %d\n",count1,size);
                    goto out;
                }

                /*
                / При расчете контрольной суммы необходимо выполнить сложение по модулю 256
                / каждого байта в фрейме
                */
                code += byte;

                /*
                / При конвертации A-law в PCM один байт расширяется в два (unsigned char -> short)
                */
                buffer[count2] = alaw2linear(byte);
                count1 += n_read_bytes; count2 += n_read_bytes;

                if(n_samples == count2){
                    /*
                    / Буффер имеет размер, равный одному блоку данных на входе вокодера
                    / При заполнении буфера происходит кодирование,
                    / а после результат с выхода вокодера записывается в файл
                    */

                    // TODO: вынести в отдельный поток

                    //status = vocoder_process(vocoder, c_frame, buffer);		
		            //if (status < 0 ) 
                    //{
                    //    E_FATAL("Encoder failed, status=%d\n", status);
                    //    break;
                    //}

                    /* 
                    / Запись в WAV-file блока данных
                    */

                    //fwrite(c_frame, 1, frame_cbit,_out);
                    fwrite(buffer,1,frame_sp,_out);
                    fflush(_out);

                    count2 = 0;
                    n_blocks++;
                }
            }
            
            // 4. контрольная сумма - дополнение до двух
            n_read_bytes = read(handle, &byte, 1);
            code = UINT8_MAX - code + 1;
            if((n_read_bytes != 1) || (byte != code)){
                E_INFO("Error check sum: %x vs %x\n",byte, code); // ~code+1
                goto out;
            }

            //E_INFO("frame read successfully, size %d\n",size);
            n_frames++;

        }while (TRUE);
        
    out:
        E_INFO("last number byte reading: %d\n",n_read_bytes);
        E_INFO("n_blocks: %d from %d\tn_frames: %d\n",n_blocks,n_block_max, n_frames);
    }

    fclose(handle);
    if (_out) fclose(_out);

    vocoder_free(vocoder);
    if(buffer) free(buffer);
    if(c_frame) free(c_frame);
    vocoder_library_destroy();
    return 0;
}
