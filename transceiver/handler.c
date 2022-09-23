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

int
send_command(short unsigned int time, int handle){

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

	/*
	/ Отправляем команду в модем
	*/

	return write(handle, &command,15);
}


static unsigned int SG1_count_voice_frame = 0; //
static unsigned char SG1_count_com = 1; // номер команды

void
write_SG1_alaw(int tty_handle, unsigned char * data, int len){
	
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

	memcpy(&buffer2[10], data,len);

	/* Расчет CRC */
	unsigned char crc = 0;
	for (int i = 6; i < 6 + SG1_len_L3 - 1; i++){
		crc += buffer2[i];
	}
	buffer2[6 + SG1_len_L3 - 1] = ~crc + 1;
	

	// Отправка данных в модем
	int retval = write(tty_handle, buffer2, SG1_len_L3 + 6);
	if(SG1_len_L3 + 6 != retval){
		E_INFO("error write to COM-port\n");
	}
	free(buffer2);
}


void
write_SG1_pcm(int tty_handle, const short * data, int len){

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

	/* Преобразование PCM в Alaw*/
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
	int retval = write(tty_handle, buffer2, SG1_len_L3 + 6);
	if(SG1_len_L3 + 6 != retval){
		E_INFO("error write to COM-port\n");
	}

	free(buffer2);
}


struct pthread_arg
{
	int handle;
	short *data;
	unsigned int size;
};

/* Потоковая функция */
void* pthread_write_SG1_pcm(void * data){

	struct pthread_arg *in = (struct pthread_arg *)data;
	int still_128 = in->size / 128;

	/* записываем блоками по 128 */
	for (int i = 0; i < still_128; i++){
		write_SG1_pcm(in->handle, &in->data[128*i], 128);
	}

	pthread_exit(0);
}



int
recv_voice_from_sock_to_channel(short vocoder_identification, int handle, recv_from_handler receiving, int n_frames){

	
	pthread_t thread;

	
	// vocoder initialization and memory allocation
	
    
	size_t frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    size_t frame_cbit = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);

    if ((frame_sp == 0) || (frame_cbit == 0)) {
        E_FATAL("Cannot determine IO size for codec %d\n", vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        E_FATAL("cannot setup library for codec %d\n", vocoder_identification);
        return -1;
    }

    void *dec = vocoder_create(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if (dec == NULL) {
        E_FATAL("codec %d cannot be created\n", vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }

	// делаем 2 буфера, работаем поочередно с ними
    short * buffer1 = (short *)malloc(frame_sp*(n_frames + 1));
	short * buffer2 = (short *)malloc(frame_sp*(n_frames + 1));

	short *b1, *b2;


    unsigned char * c_frame = (unsigned char *)malloc(frame_cbit * n_frames);

	size_t frame_samples = frame_sp / sizeof(buffer1[0]);
	E_INFO("frame_cbit: %zu frame_sp: %zu\n",frame_cbit, frame_sp);

	
	int tail0, tail = 0; // длина хвоста
	for (int s = 0;;s++)
	{
		
		receiving(c_frame, frame_cbit*n_frames);
		// TODO: запись 20 сек
		if(s == 0){
			sleep_msec(100);
			if(send_command(20, handle) < 0){
				E_FATAL("failed send command to SG1%s\n");
			}
		}

		// В зависимости от номера прохода выбираем буфер
		if(s & 1){
			b1 = buffer1;
			b2 = buffer2;
		}else{
			b1 = buffer2;
			b2 = buffer1;
		}


		int status, even_128, odd_128, i;
		for (i = 0, even_128 = 0, odd_128 = 0; i < n_frames; i++)
		{

			// В зависимости от номера прохода выбираем буфер
			status = vocoder_process(dec, &c_frame[frame_cbit*i], &b1[tail + frame_samples*i]);


			if (status < 0 ) 
			{
				E_FATAL("decoder failed, status = %d\n", status);
				goto out;
			}

			// количество блоков (по 128), готовое для записи
			even_128 = ((tail + frame_samples*(i + 1)) / 128) - odd_128;

			if(even_128){
				
				// Если данные ранее уже отправлялись в модем, нужно дождаться их записи
				if(s) pthread_join(thread, NULL);

				struct pthread_arg pthread_data = { handle, &b1[128 * odd_128], 128 * even_128 };
				pthread_create(&thread, NULL, pthread_write_SG1_pcm, &pthread_data);

				odd_128 += even_128;
			
//				for (int j = 0; j < even_128; j++){
//					write_SG1_pcm(handle, &buffer1[128 * (odd_128 + j)], 128);
//					E_INFO("write: i = %d, j = %d, tail = %d\n",i,j, tail);
//				}
//				odd_128 += even_128;

				
			}
		}

		// Перемещаем хвост в начало буфера
		tail0 = (tail + n_frames * frame_samples) % 128;
		memcpy(b2, &b1[tail + (n_frames * frame_samples) - tail0], sizeof(short)*tail0);
		tail = tail0;
		}
out:
	free(buffer1);
	free(buffer2);
	free(c_frame);

    return 0;
}


int
recv_voice_from_sock_to_file(short vocoder_identification, FILE* file, recv_from_handler receiving, int n_frames){

	
	//pthread_t thread;

	
	// vocoder initialization and memory allocation
	
    
	size_t frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    size_t frame_cbit = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);

    if ((frame_sp == 0) || (frame_cbit == 0)) {
        E_FATAL("Cannot determine IO size for codec %d\n", vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        E_FATAL("cannot setup library for codec %d\n", vocoder_identification);
        return -1;
    }

    void *dec = vocoder_create(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if (dec == NULL) {
        E_FATAL("codec %d cannot be created\n", vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }

	// делаем 2 буфера, работаем поочередно с ними
    short * buffer1 = (short *)malloc(frame_sp*(n_frames + 1));
	short * buffer2 = (short *)malloc(frame_sp*(n_frames + 1));

	short *b1, *b2;


    unsigned char * c_frame = (unsigned char *)malloc(frame_cbit * n_frames);

	size_t frame_samples = frame_sp / 2;
	E_INFO("frame_cbit: %zu frame_sp: %zu\n",frame_cbit, frame_sp);

	
	int tail = 0; // длина хвоста
	int tail0;
	for (int s = 0;;s++)
	{
		
		receiving(c_frame, frame_cbit*n_frames);
		/*
		// TODO: запись 20 сек
		if(s == 0){
			sleep_msec(500);
			if(send_command(20, handle) < 0){
				E_FATAL("failed send command to SG1%s\n");
			}
		}
		*/
	
		// В зависимости от номера прохода выбираем буфер
		if(s & 1){
			b1 = buffer1;
			b2 = buffer2;
		}else{
			b1 = buffer2;
			b2 = buffer1;
		}
	

		int status, even_128, odd_128, i;
		even_128 = 0;
		odd_128 = 0;
		for (i = 0; i < n_frames; i++)
		{

			// В зависимости от номера прохода выбираем буфер
			status = vocoder_process(dec, &c_frame[frame_cbit*i], &b1[tail + (frame_samples*i)]);

			for(int g = 0; g < frame_samples; g++ ){
				b1[tail + (frame_samples*i) + g] = 50*g;
			}


			if (status < 0 ) 
			{
				E_FATAL("decoder failed, status = %d\n", status);
				goto out;
			}

			// количество блоков (по 128), готовое для записи
			even_128 = ((tail + frame_samples*(i + 1)) / 128) - odd_128;

			if(even_128){
				
				// Если данные ранее уже отправлялись в модем, нужно дождаться их записи
//				if(s) pthread_join(thread, NULL);

//				struct pthread_arg pthread_data = { handle, &b1[128 * odd_128], 128 * even_128 };
//				pthread_create(&thread, NULL, pthread_write_SG1_pcm, &pthread_data);

		
//				for (int j = 0; j < even_128; j++){
//					write_SG1_pcm(handle, &buffer1[128 * (odd_128 + j)], 128);
//					E_INFO("write: i = %d, j = %d, tail = %d\n",i,j, tail);
//				}
				//unsigned char * b3 = (unsigned char*)malloc(128 * even_128);
				
				// Преобразование PCM в Alaw
				//for (int j = 0; j < 128 * even_128; j++){
				//	b3[j] = linear2alaw(b1[128 * odd_128 + j]);
				//}

				//int l = fwrite(buffer1, 1, frame_sp, file);
				int l = fwrite(&b1[128 * odd_128], sizeof(short), 128 * even_128, file);
				E_INFO_NOFN("[%d\t%d]\n",b1[128 * odd_128],b1[128 * (odd_128 + even_128) - 1]);
				odd_128 += even_128;

				
			}
		}

		// Перемещаем хвост в начало буфера
		tail0 = (tail + n_frames * frame_samples) % 128;
		memcpy(b2, &b1[tail + (n_frames * frame_samples) - tail0], sizeof(short)*tail0);
		tail = tail0;

		// Перемещаем хвост в начало буфера
		//tail = (tail + n_frames * frame_samples) % 128;
		//memcpy(b2, &b1[(n_frames * frame_samples) - tail], sizeof(short)*tail);

		}
out:
	free(buffer1);
	free(buffer2);
	free(c_frame);

    return 0;
}

/*
// Первый рабочий вариант 
int
recv_voice_from_sock_to_channel(short vocoder_identification, int tty_handle, recv_from_handler receiving, int n_frames){

	
	// vocoder initialization and memory allocation
	
    
	size_t frame_sp = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_ENCODER);
    size_t frame_cbit = vocoder_get_input_size(vocoder_identification, VOCODER_DIRECTION_DECODER);

    if ((frame_sp == 0) || (frame_cbit == 0)) {
        E_FATAL("Cannot determine IO size for codec %d\n", vocoder_identification);
        return -1;
    }

    if (vocoder_library_setup()) {
        E_FATAL("cannot setup library for codec %d\n", vocoder_identification);
        return -1;
    }

    void *dec = vocoder_create(vocoder_identification, VOCODER_DIRECTION_DECODER);
    if (dec == NULL) {
        E_FATAL("codec %d cannot be created\n", vocoder_identification);
        vocoder_library_destroy();
        return -1;
    }

    short * buffer1 = (short *)malloc(frame_sp*(n_frames + 1));
    unsigned char * c_frame = (unsigned char *)malloc(frame_cbit * n_frames);

	size_t frame_samples = frame_sp / sizeof(buffer1[0]);
	E_INFO("frame_cbit: %zu frame_sp: %zu\n",frame_cbit, frame_sp);

	

	//pthread_t thread;
	//struct pthread_arg pthread_data;
	//pthread_data.handle = tty_handle;
	//pthread_data.ptr = buffer1;
	//pthread_data.len = frame_sp/2;
	

	int tail = 0; // длина хвоста
	for (int s = 0;;s++)
	{
		
		receiving(c_frame, frame_cbit*n_frames);
		// TODO: запись 10 сек
		if(s == 0){
			sleep_msec(100);
			if(send_command(20, tty_handle) < 0){
				E_FATAL("failed send command to SG1%s\n");
			}
		}


		int status, j = 0;
		for (int i = 0; i < n_frames; i++)
		{
			status = vocoder_process(dec, &c_frame[frame_cbit*i], &buffer1[tail + frame_samples*i]);
			if (status < 0 ) 
			{
				E_FATAL("decoder failed, status = %d\n", status);
				goto out;
			}


			for( ; 128*(j+1) <= tail + frame_samples*(i+1) ; j++){
				write_SG1_pcm(tty_handle, &buffer1[128*j], 128);
				E_INFO("write: i = %d, j = %d, tail = %d\n",i,j, tail);
			}

			


			//write_SG1_pcm(tty_handle, buffer1, frame_sp / 2);
			//запускаем поток
			//pthread_create(&thread, NULL, pthread_write_SG1, &pthread_data);


		}

		// Перемещаем хвост в начало буфера
		tail = (tail + n_frames * frame_samples) % 128;
		memcpy(buffer1, &buffer1[(n_frames * frame_samples) - tail], tail);

		}
out:
	free(buffer1);
	free(c_frame);

    return 0;
}

*/
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

