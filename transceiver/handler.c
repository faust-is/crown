#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
//#include <pthread.h>

#include "handler.h"
#include "wav.h"
#include "err.h"
#include "convert.h"

const unsigned int marker =  0x99699699;

/* Sleep for specified msec */
static void
sleep_msec(int32_t ms)
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


//struct pthread_arg
//{
//	int handle;
//	short *ptr;
//	int len;
//};

static unsigned int SG1_count_voice_frame = 0; //
static unsigned char SG1_count_com = 1; // номер команды


//int write_SG1(int tty_handle, unsigned char * data, int len){
static int write_SG1_pcm(int tty_handle, const short * data, int len){

	int retval;
	unsigned char * buffer2 = (unsigned char *)malloc(len + 11);

	/* Транспортный уровень*/
	memcpy(buffer2, &marker, 4);

	unsigned short SG1_len_L3 = len + 5;
	memcpy(&buffer2[4], &SG1_len_L3, sizeof(SG1_len_L3));
	

	/* Тело посылки прикладного уровня */
	buffer2[6] = 0xA1; // адресс модема
	buffer2[7] = (0x7F & ++SG1_count_com);

	memcpy(&buffer2[8], &SG1_count_voice_frame, 2);
	SG1_count_voice_frame++;

	//
//	memcpy(&buffer2[10], data,len);
	for (int j = 0; j < len; j++){
		buffer2[10 + j] = linear2alaw(data[j]);
	}


	/* Расчет CRC */
	unsigned char crc = 0;
	for (int i = 6; i < 6 + SG1_len_L3 - 1; i++){
		crc += buffer2[i];
	}
	buffer2[6 + SG1_len_L3 - 1] = ~crc + 1;
	

	// Отправка данных в модем
	retval = write(tty_handle, buffer2, SG1_len_L3 + 6);
	if(SG1_len_L3 + 6 != retval){
		E_INFO("error write to COM-port\n");
	}
	free(buffer2);

	return retval;
}

//void* pthread_write_SG1(void * data){
//
//	struct pthread_arg *in = (struct pthread_arg *)data;
//	write_SG1(in->handle, in->ptr, in->len);
//	pthread_exit(0);
//}

int recv_voice_from_sock_to_channel(short vocoder_identification, int tty_handle, recv_from_handler receiving, int n_frames_for_out){

	/*
	/ vocoder initialization and memory allocation
	*/
    
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

    void *dec = vocoder_create(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if (dec == NULL) {
        fprintf (stderr, "codec %d cannot be created\n", vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }

    short * buffer1 = (short *)malloc(frame_sp*(n_frames_for_out + 1));
    unsigned char * c_frame = (unsigned char *)malloc(frame_cbit * n_frames_for_out);


	E_INFO("frame_cbit: %zu frame_sp: %zu\n",frame_cbit, frame_sp);

	//pthread_t thread;
	//struct pthread_arg pthread_data;
	//pthread_data.handle = tty_handle;
	//pthread_data.ptr = buffer1;
	//pthread_data.len = frame_sp/2;
	

	int tail = 0; // длина хвоста
	for (int s = 0;;s++)
	{
		
		receiving(c_frame, frame_cbit*n_frames_for_out);
		// TODO: запись 10 сек
		if(s == 0){
			//sleep_msec(1000);
			if(send_command(20, tty_handle) < 0){
				E_FATAL("failed send command to SG1%s\n");
			}
		}
		//else{
		//	pthread_join(thread, NULL);
		//}

		
		int status, j = 0;
		// TODO: многопточность только для одного блока 
		for (int i = 0; i < n_frames_for_out; i++)
		{
			status = vocoder_process(dec, &c_frame[i * frame_cbit], buffer1);
			// TODO: размер буфера на 1 блок больше
			//status = vocoder_process(dec, &c_frame[frame_cbit*i], &buffer1[tail + (frame_sp/2)*i]);
			if (status < 0 ) 
			{
				E_FATAL("decoder failed, status = %d\n", status);
				goto out;
			}


			for( ; 128*(j+1) <= tail + (frame_sp / 2)*(i+1) ; j++){
				write_SG1_pcm(tty_handle, &buffer1[128*j], 128);
				E_INFO("write: i = %d, j = %d, tail = %d\n",i,j, tail);
			}

			


			//write_SG1_pcm(tty_handle, buffer1, frame_sp / 2);
			//запускаем поток
			//pthread_create(&thread, NULL, pthread_write_SG1, &pthread_data);

			/*
			for (int j = 0; j < frame_samples; j++){
				buffer2[10 + i*frame_samples + j] = linear2alaw(buffer1[j]);
			}
			*/
		}
		tail = (tail + (n_frames_for_out * frame_sp / 2)) % 128;
		memcpy(buffer1, &buffer1[(n_frames_for_out * frame_sp / 2) - tail], tail);
		
	/*	
		// Расчет CRC
		unsigned char crc = 0;
	// ВАРИАНТ 2:
		for (int i = 6; i < 6 + SG1_len_L3 - 1; i++){
			crc += buffer2[i];
		}
		buffer2[6 + SG1_len_L3 - 1] = ~crc + 1;
		
	
		// Отправка данных в модем
		int l = write(tty_handle, buffer2, SG1_len_L3 + 6);
		if(SG1_len_L3 + 6 != l){
			E_INFO("error write to COM-port\n");
		}
	*/

		//int l = fwrite(&buffer2[10], sizeof(unsigned char), n_frames_for_out * frame_samples, (FILE*)tty_handle);
		
	// ВАРИАНТ 1:
	/*
		//unsigned short L133 = 128+5;
		//memcpy(&buffer2[4], &L133, sizeof(L133));

		//memcpy(&BUFFER[15 + s*(L133+6) + 4], &buffer2[4], L133);
		//memcpy(&BUFFER[4], &buffer2[4], L133);
		
		//BUFFER[15 + (L133+6)*s+50] = ~BUFFER[15 + (L133+6)*s+50];
		//BUFFER[50] = ~BUFFER[50];



		crc = 0;
		for (int j = 6; j < 6 + L133 - 1; j++){
			//crc += BUFFER[15 + s*(L133+6) + j];
			//crc += BUFFER[j];
			crc+=buffer2[j];
		}
       // BUFFER[15 + s*(L133+6) + 6 + L133 - 1] = ~crc + 1;
		//BUFFER[6 + L133 - 1] = ~crc + 1;
		buffer2[6 + L133 - 1] = ~crc + 1;
		E_INFO_NOFN("(%d)%02X]",L133,(~crc + 1) & 0xFF);

		//int l = write(tty_handle, &BUFFER[15 + (L133+6)*s], (L133+6));
		//int l = write(tty_handle, BUFFER, (L133+6));
		int l = write(tty_handle, buffer2, (L133+6));

		for (size_t i = 0; i < 10; i++)
		{
			E_INFO_NOFN("%02x ",BUFFER[i]);
		}
		E_INFO_NOFN("\n");
		E_INFO("s: %d l: %d\n",s,l);
	*/
		//E_INFO("s: %d l: %d\n",s,l);
	}
out:
	free(buffer1);
	free(c_frame);

		
    return 0;
}


int send_voice_from_channel_to_socket(short vocoder_identification, int tty_handle, send_to_handler sending, int n_frames_for_out){
        
	int n_read_bytes;
	unsigned char byte, crc;
	unsigned short size_block;

	E_INFO("start send voice from serial port to socket\n");

	/*
	/ vocoder initialization and memory allocation
	*/
    
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
		n_read_bytes = read(tty_handle, &tmarker, sizeof(unsigned int ));
		if((sizeof(marker) != n_read_bytes) || (tmarker != marker)){
			E_INFO("error reading marker: %x\n",tmarker);
			goto out;
		}
		
		// 2. длина блока
		n_read_bytes = read(tty_handle, &size_block, sizeof(size_block));
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
			n_read_bytes = read(tty_handle, &byte, 1);
			if(n_read_bytes < 1) {
				E_INFO("error reading frame body: %d from %d\n", count_bytes, size_block);
				goto out;
			}

			/*
			/ При расчете контрольной суммы необходимо выполнить сложение по модулю 256
			/ каждого байта в фрейме
			*/
			crc += byte;
			count_bytes += n_read_bytes;
		}

		while(count_bytes < size_block - 1){

			n_read_bytes = read(tty_handle, &byte, 1);
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
				    E_FATAL("encoder failed, status = %d\n", status);
				    break;
				}

				count_frames++;

				if ((count_frames % n_frames_for_out) == 0){
					/* Отправка блока данных по UDP */
					if(sending(c_frame,frame_cbit * n_frames_for_out) < 0){
						E_FATAL("error: failed to send to socket\n");
					}
				}
				
				count_samples = 0;
			}
		}
		
		// 4. контрольная сумма - дополнение до двух
		n_read_bytes = read(tty_handle, &byte, 1);
		crc = ~crc + 1;
		if((n_read_bytes != 1) || (byte != crc)){
			E_INFO("error check sum: %x vs %x\n",byte, crc); // ~code+1
			goto out;
		}
	}
        
out:
	E_INFO("last number byte reading: %d\n",n_read_bytes);
	E_INFO("number blocks reading from SG1: %d\n",count_blocks); 
	E_INFO("number frames send to socket: %d\n", count_frames);
    
	/*
	/ vocoder destroy and free memory
	*/
    
    vocoder_free(enc);
    free(buffer);
    free(c_frame);
    vocoder_library_destroy();
    return 0;
}

