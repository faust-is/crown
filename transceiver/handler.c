#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
//#include <semaphore.h>

#include "handler.h"
#include "wav.h"
#include "err.h"
#include "convert.h"

const unsigned int marker =  0x99699699;


//static unsigned char _count1 = 2;
//static unsigned short _count2 = 0;

int send_command(short unsigned int time, int handle){

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

	memcpy(&command[10],&time, sizeof(short unsigned int ));

	unsigned char crc = 0;
	for (unsigned short i = 6; i < 6 + 8; i++){
		crc +=command[i];
	}
	command[14] = ~crc + 1;

	//for (unsigned short i = 6; i < 6 + 9; i++){
	//	printf("\t%d: %02x \n",i,command[i]);
	//}

	/*
	/ Отправляем команду в модем
	*/

	return write(handle, &command,15);
}

/*
void send_voice_to_channel(FILE * rawfd, int samplerate, int tty_handle){

		// TODO: сейчас выборку получаем из WAV-файла
	    wavfile_header_t wavheader;
		fread(&wavheader, 1, sizeof(wavfile_header_t), rawfd);

		// TODO: только для однокального A-law
		int n_samples = wavheader.Subchunk2Size;

		// TODO:
		short unsigned int time = (n_samples / samplerate) + 1;
		if (send_command(time, tty_handle) != 15){
            E_INFO("error send command\n");
            return;
        }
		///

		//int size_block = 240; // MELPE
		int size_block_samples = 540; // ACELP
		

		int n_blocks = n_samples / size_block_samples; 
		unsigned short _len = size_block_samples + 5;
		int size_buf = n_blocks * (6 + _len);

		unsigned char *buffer = (unsigned char *)malloc(size_buf);
		if(NULL == buffer){
			E_INFO("out of memory\n");
            return;
		}
		
		for (int i = 0; i < n_blocks; i++){

			memcpy(&buffer[(6 + _len)*i], &marker, 4);
			memcpy(&buffer[(6 + _len)*i + 4],&_len,2);
			buffer[(6 + _len)*i + 6] = 0xA1; // адресс модема
			buffer[(6 + _len)*i + 7] = _count1;
			memcpy(&buffer[(6 + _len)*i + 8], &_count2, 2);
			// TODO: PCM to A-law
			fread(&buffer[(6 + _len)*i + 10], 1, size_block_samples, rawfd);

			crc = 0;
			for (unsigned short j = 0; j < _len - 1; j++){
				crc += buffer[(6 + _len)*i + 10 + j];
			}
			buffer[(6 + _len)*i + 10 + _len] = ~crc + 1;

			_count1 += 1;
			_count2 += 1;
		}

		if(size_buf != write(tty_handle, buffer, size_buf)){
			E_INFO("error write to COM-port\n");
		}
	
		free(buffer); 
}
*/

int write_voice_from_channel(short vocoder_identification, int handle, int n_frames_for_out){
        
	int n_read_bytes;
	unsigned char byte, crc;
	unsigned short size_block;

	E_INFO("reeding answer from COM-port\n");

    // 
	
	size_t frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    size_t frame_cbit = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if ((frame_sp == 0) || (frame_cbit == 0)) {
        fprintf (stderr, "Cannot determine IO size for codec %d\n", vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        fprintf (stderr, "cannot setup library for codec %d\n", vocoder_identification);
        return -1;
    }

    void *enc = vocoder_create(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    if (enc == NULL) {
        fprintf (stderr, "codec %d cannot be created\n", vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }
    short * buffer = (short *)malloc(frame_sp);
    unsigned char * c_frame = (unsigned char *)malloc(sizeof(unsigned char)*frame_cbit * n_frames_for_out);


	int frame_samples = frame_sp / sizeof(unsigned short);


	int count_samples = 0;
	int count_frames = 0;
	int count_bytes = 0;
	int count_blocks = 0;

	for (count_blocks = 0;; count_blocks++)
	{

		unsigned int tmarker = 0;
		// 1. читаем и сверяем маркер
		n_read_bytes = read(handle, &tmarker, sizeof(unsigned int ));
		if((sizeof(marker) != n_read_bytes) || (tmarker != marker)){
			E_INFO("error reading marker: %x\n",tmarker);
			goto out;
		}
		
		// 2. длина блока
		n_read_bytes = read(handle, &size_block, sizeof(size_block));
		if(sizeof(size_block) != n_read_bytes){
			E_INFO("error reading length of frame body\n");
			goto out;
		}


		// 3. тело блока
		count_bytes = 0; // количество байт считанное из блока с  SG1
		crc = 0;
		
		/*
		/ Первые четыре байта несут информацию  о адресе модема, порядковом номере фрейма и т.д.
		/ Эту информацию никак не обрабатываем, пробрасываем
		*/
		for (int i = 0; i < 4; i++)
		{
			n_read_bytes = read(handle, &byte, 1);
			if(n_read_bytes < 1) {
				E_INFO("error reading frame body: %d from %d\n",count_bytes,size_block);
				goto out;
			}

			/*
			/ При расчете контрольной суммы необходимо выполнить сложение по модулю 256
			/ каждого байта в фрейме
			*/
			crc+= byte;
			count_bytes += n_read_bytes;
		}
            

		while(count_bytes < size_block - 1){

			n_read_bytes = read(handle, &byte, 1);
			if (n_read_bytes < 1) {
				E_INFO("error reading frame body: %d from %d\n",count_bytes,size_block);
				goto out;
			}

			/*
			/ При расчете контрольной суммы необходимо выполнить сложение по модулю 256
			/ каждого байта в фрейме
			*/
			crc += byte;

			/*
			/ При конвертации A-law в PCM один байт расширяется в два (unsigned char -> short)
			*/
			buffer[count_samples] = alaw2linear(byte);
			/*
			/ Количество байт, считанное из блока SG1: +1
			/ Количество отсчетов, записанное в буффер: +1 
			*/
			count_bytes += n_read_bytes; 
			count_samples += n_read_bytes;

			if(frame_samples == count_samples){
				/*
				/ Буффер имеет размер, равный одному блоку данных на входе вокодера
				/ При заполнении буфера происходит кодирование,
				/ а после результат с выхода вокодера записывается в файл
				*/

				// TODO: вынести в отдельный поток

				int status = vocoder_process(enc, &c_frame[frame_cbit * (count_frames % n_frames_for_out)], buffer);		
				if (status < 0 ) 
				{
				    E_FATAL("Encoder failed, status=%d\n", status);
				    break;
				}

				count_frames++;

				/* 
				/ Запись в WAV-file блока данных
				*/
				if ((count_frames % n_frames_for_out) == 0)
				{
					E_INFO("T:\n");
				}
				
				//fwrite(c_frame, 1, frame_cbit,_out);
				//fwrite(buffer,1,frame_sp,_out);
				//fflush(_out);

				count_samples = 0;
			}
		}
		
		// 4. контрольная сумма - дополнение до двух
		n_read_bytes = read(handle, &byte, 1);
		crc = ~crc + 1;
		if((n_read_bytes != 1) || (byte != crc)){
			E_INFO("error check sum: %x vs %x\n",byte, crc); // ~code+1
			goto out;
		}

		//E_INFO("frame read successfully, size %d\n",size);
		//count_blocks++;

	}//while (TRUE);
        
out:
	E_INFO("last number byte reading: %d\n",n_read_bytes);
	E_INFO("n_blocks: %d \tn_frames: %d\n",count_blocks, count_frames);
    

    fclose(handle);


    vocoder_free(enc);
    if(buffer) free(buffer);
    if(c_frame) free(c_frame);
    vocoder_library_destroy();
    return 0;
}

