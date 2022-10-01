#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>



#include <pthread.h>

#include "handler.h"
#include "wav.h"
#include "err.h"
#include "convert.h"

/**
 * @brief Global flag to stop applocation running 
 */
static volatile bool g_stopped = false;

/**
 * @brief Signal handler.
 */
void sig_int_handler(int val) { g_stopped = true; }

const uint32_t marker =  0x99699699;

/* Sleep for specified msec */
static void
sleep_msec(int32_t ms)
{
#if (defined(_WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
    Sleep(ms);
#else
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
#endif
}

//#ifdef notdef 
struct timeval point0;
struct timeval point1;

long
timeval_diff(struct timeval *difference,
             struct timeval *end_time,
             struct timeval *start_time
            )
{
  struct timeval temp_diff;

  if(difference==NULL)
  {
    difference=&temp_diff;
  }

  difference->tv_sec =end_time->tv_sec -start_time->tv_sec ;
  difference->tv_usec=end_time->tv_usec-start_time->tv_usec;

  // Using while instead of if below makes the code slightly more robust.

  while(difference->tv_usec<0)
  {
    difference->tv_usec+=1000000;
    difference->tv_sec -=1;
  }

  return (1000000LL* difference->tv_sec+
                   difference->tv_usec) / 1000;

} // timeval_diff()
//#endif


int
send_command(uint16_t time, int handle, bool started){

	uint8_t command[15] = { 
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
	/ On/off writing
	*/
	command[9] = (started)? 0x01: 0x00;

	/*
	/ Перед отправкой команды нужно указать (изменить) время записи,
	/ после пересчитать контрольную сумму
	*/
	
	memcpy(&command[10],&time, sizeof(short unsigned int ));

	uint8_t crc = 0;
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
	

	/* Отправка данных в модем */
	int retval = write(tty_handle, buffer2, SG1_len_L3 + 6);
	if(SG1_len_L3 + 6 != retval){
		E_INFO("error write to COM-port\n");
	}
	free(buffer2);
}


void
write_SG1_pcm(int tty_handle, const int16_t * data, int len){

	uint8_t * buffer2 = (uint8_t *)malloc(len + 11);

	/* Транспортный уровень*/
	memcpy(buffer2, &marker, 4);

	uint16_t SG1_len_L3 = len + 5;
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
	uint8_t crc = 0;
	for (int i = 6; i < 6 + SG1_len_L3 - 1; i++){
		crc += buffer2[i];
	}
	buffer2[6 + SG1_len_L3 - 1] = ~crc + 1;
	

	/* Отправка данных в модем */
	int retval = write(tty_handle, buffer2, SG1_len_L3 + 6);
	if(SG1_len_L3 + 6 != retval){
		E_INFO("error write to COM-port\n");
	}

	free(buffer2);
}
struct pthread_arg
{
	int handle;
	int16_t *data;
	size_t size;
};

/* Потоковая функция */
void* pthread_write_SG1_pcm(void * data){

	struct pthread_arg *in = (struct pthread_arg *)data;
	size_t still_128 = in->size / 128;

	/* записываем блоками по 128 */
	for (size_t i = 0; i < still_128; i++){
		write_SG1_pcm(in->handle, &in->data[128*i], 128);	
		sleep_msec(15);
	}

	pthread_exit(0);
}

int
sock_to_channel(short vocoder_identification, int handle, recv_from_handler receiving, int n_frames){

	
	pthread_t thread;
	int16_t *b1, *b2;
	int s,tail0, tail; // длина хвоста

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
	// TODO: tail max = 128
    int16_t * buffer1 = (int16_t *)malloc(frame_sp*(n_frames + 1));
	int16_t * buffer2 = (int16_t *)malloc(frame_sp*(n_frames + 1));
    uint8_t * c_frame = (uint8_t *)malloc(frame_cbit * n_frames);

	size_t frame_samples = frame_sp / sizeof(int16_t);

	
	// инициализация мьютекса

	E_INFO("frame_cbit: %zu frame_sp: %zu\n",frame_cbit, frame_sp);

	for (s = 0, tail = 0; !g_stopped; s++)
	{

	//	gettimeofday(&point0,NULL);
	wait_loop:
		if(g_stopped)
			break;

		ssize_t retval = receiving(c_frame, frame_cbit*n_frames);
		if (retval < 0) {
            if (errno == EWOULDBLOCK) {
                sleep_msec(100);
                goto wait_loop; // no UDP packet yet
            }
			E_ERROR("failed to receive UDP packet: %s",strerror(errno));
        }
	//	E_INFO("rec: %ld\n",retval);
	//	gettimeofday(&point1,NULL);

		E_INFO_NOFN("%d. rec %zu [%d.%06d s]\n",s, retval, point1.tv_sec, point1.tv_usec);
		//E_INFO_NOFN("%d.\t%d.%06d\n",s, point1.tv_sec, point1.tv_usec);

		if(!s){
			if(send_command(1000, handle, true) < 0){
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

			if(even_128){ // есть данные для записи
				
				// Если данные ранее уже отправлялись в модем, нужно дождаться их записи
				if(s) pthread_join(thread, NULL);
				struct pthread_arg pthread_data = { handle, &b1[128 * odd_128], 128 * even_128 };
				pthread_create(&thread, NULL, pthread_write_SG1_pcm, &pthread_data);
				odd_128 += even_128;			
			}
		}

		// Перемещаем хвост в начало буфера
		tail0 = (tail + n_frames * frame_samples) % 128;
		memcpy(b2, &b1[tail + (n_frames * frame_samples) - tail0], sizeof(int16_t)*tail0);
		tail = tail0;
	}
	
out:
	if(s){
		pthread_join(thread, NULL);
		if(send_command(1000, handle, false) < 0){
			E_FATAL("failed send command to SG1: off\n");
		}
		else{
			E_INFO("send command to SG1: off\n");
		}
	}
	//pthread_mutex_destroy(&mp);

	free(buffer1);
	free(buffer2);
	free(c_frame);

    return 0;
}



int channel_to_socket(short vocoder_identification, int tty_handle, send_to_handler sending, int n_frames_for_out){
        
	int n_read_bytes;
	uint16_t size_block;

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

    int16_t * buffer = (int16_t *)malloc(frame_sp);
    uint8_t * c_frame = (uint8_t *)malloc(frame_cbit * n_frames_for_out);

	
	int frame_samples = frame_sp / sizeof(int16_t);

	/*
	/ Отправляем в модем команду на выдачу выборки в течении INT16_MAX сек
	*/
	if(send_command(90, tty_handle, true) != 15){
		E_FATAL("failed send command to SG1%s\n");
	}
	else{
		E_INFO("send command to SG1: on\n");
	}



	int count_samples = 0;
	int count_frames = 0;

	while(!g_stopped){

		uint32_t tmarker;
		uint8_t byte, crc;

		/* Читаем и сверяем маркер */
		n_read_bytes = read(tty_handle, &tmarker, 4);
		if((4 != n_read_bytes) || (tmarker != marker)){
			E_INFO("error reading marker: %x\n",tmarker);
			goto out;
		}

		
		/* Длина блока */
		n_read_bytes = read(tty_handle, &size_block, sizeof(size_block));
		if(sizeof(size_block) != n_read_bytes){
			E_INFO("error reading length of frame body\n");
			goto out;
		}

		/* Тело блока */
		int count_bytes = 0; // количество байт считанное из блока с  SG1

		/*
		/ Первые четыре байта несут информацию  о адресе модема, порядковом номере фрейма и т.д.
		/ Эту информацию никак не обрабатываем, пробрасываем
		*/
		crc = 0;
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
				    goto out;
				}

				count_frames++;

				if ((count_frames % n_frames_for_out) == 0){

					/* Отправка блока данных по UDP */
					if(sending(c_frame,frame_cbit * n_frames_for_out) != frame_cbit * n_frames_for_out){
						E_FATAL("error: failed to send to socket\n");
					}
				}
				
				count_samples = 0;
			}
		}

		/* Проверка контрольной суммы - дополнение до двух */
		if (count_bytes == size_block - 1){
			n_read_bytes = read(tty_handle, &byte, 1);
			crc = ~crc + 1;
			if((n_read_bytes != 1) || (byte != crc)){
				E_INFO("error check sum: %x vs %x\n",byte, crc); // ~code+1
				goto out;
			}
		}else{
			E_INFO("error reading of length frame: %d from %d",count_bytes, size_block - 1);
			goto out;
		}
	}
   
out:
//sleep_msec(3000);
	if(send_command(90, tty_handle, false) != 15){
		E_FATAL("failed send command to SG1: off\n");
	}
	else{
		E_INFO("send command to SG1: off\n");
	}
	
	E_INFO("last number byte reading: %d\n",n_read_bytes);

	/*
	/ vocoder destroy and free memory
	*/
    
    vocoder_free(enc);
    free(buffer);
    free(c_frame);
    vocoder_library_destroy();
    return 0;
}


/*
struct pthread_arg
{
	int handle;
	short *b1;
	short *b2;
};
/*
// Потоковая функция
void* pthread_write_SG1_pcm(void * data){

	struct pthread_arg *in = (struct pthread_arg *)data;
	int still_128 = in->size / 128;
	//E_INFO_NOFN("[%d]",still_128);

	// записываем блоками по 128
	for (int i = 0; i < still_128; i++){

//gettimeofday(&y1,NULL);
		write_SG1_pcm(in->handle, &in->data[128*i], 128);
//gettimeofday(&y2,NULL);
//E_INFO_NOFN("write: \t%f ms\n", timeval_diff(NULL, &y2, &y1));	

		sleep_msec(15);
	}

	pthread_exit(0);
}
*/

/*
#include <stdatomic.h>
volatile static atomic_int wait_128 = 0;
static bool change_b = false;
static bool g_no_signal = true;
static pthread_mutex_t mp = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mp1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cv1 = PTHREAD_COND_INITIALIZER;

//
static bool have_data = false;
static bool write_end = false;
void pthread_write_SG1_pcm(void * _arg_){

	struct pthread_arg *in = (struct pthread_arg *)_arg_;

	int value, odd_128;
	int16_t *current_b = in->b1;
	
	if(send_command(INT16_MAX, in->handle, true) < 0){
		E_FATAL("failed send command to SG1: on\n");
	}

	value = atomic_load_explicit(&wait_128, memory_order_relaxed);
	odd_128 = 0;

	while (!g_stopped)
	{
		E_INFO_NOFN("[4]");
		pthread_mutex_lock(&mp);
		// записываем блоками по 128				
		for (;odd_128 < value; odd_128++){
			write_SG1_pcm(in->handle, &current_b[128 * odd_128], 128);
			//sleep_msec(15);
			// update value
			value = atomic_load_explicit(&wait_128, memory_order_relaxed);
		}
		E_INFO_NOFN("[5:%d]",value);
		pthread_mutex_unlock(&mp);
		//pthread_cond_wait(&cv, &mp);


		value = atomic_load_explicit(&wait_128, memory_order_relaxed);
		E_INFO_NOFN("[6:%d]",value);
		if (!value - odd_128)
		{
			E_INFO("No signal!\n");
			break;
		}else{
			E_INFO_NOFN("[7]");
			pthread_mutex_lock(&mp);
			if(change_b){
				// change buffer
				current_b = (current_b == in->b1)? in->b2 : in->b1;
				atomic_fetch_sub_explicit(&wait_128, odd_128, memory_order_relaxed);
				odd_128 = 0;	
			}
			pthread_mutex_unlock(&mp);
			E_INFO_NOFN("[8]");
		}
	}

	while (!g_stopped)
	{

		//pthread_mutex_lock(&mp);
		// записываем блоками по 128				
		for (;odd_128 < value; odd_128++){
			write_SG1_pcm(in->handle, &current_b[128 * odd_128], 128);
			//sleep_msec(15);
			// update value
			value = atomic_load_explicit(&wait_128, memory_order_relaxed);
			E_INFO("(+ %d:%d)\n",value,odd_128);
		}
		E_INFO("write_end = true\n");
		//pthread_mutex_lock(&mp1);
		write_end = true; // запись окончена
		have_data = false; // данных для записи нет
		pthread_cond_signal(&cv1);
		//pthread_mutex_unlock(&mp1);
	
		// проходим, как будут данные для записи (true)
		pthread_mutex_lock(&mp);
		while (have_data == false){
			pthread_cond_wait(&cv, &mp);
		}
		pthread_mutex_unlock(&mp);

		//(have_data = true
		write_end = false;
		
E_INFO("=============\n");

		

		//if (!(value - odd_128))
		//{
		//	E_INFO("No signal: %d, %d!\n", value, odd_128);
			//break;
		//}else{

			//pthread_mutex_lock(&mp1);
			
	//		if(change_b){
	//			E_INFO("Change buf[2]\n");
	//			// change buffer
	//			current_b = (current_b == in->b1)? in->b2 : in->b1;
	//			change_b = false;
				atomic_fetch_sub_explicit(&wait_128, odd_128, memory_order_relaxed);
				odd_128 = 0;	
	//		}else{
	//			E_INFO("No change buf[2]\n");
	//		}
			//pthread_mutex_unlock(&mp1);
		//}

		value = atomic_load_explicit(&wait_128, memory_order_relaxed);

	}

	//pthread_mutex_lock(&mp);
	if(send_command(INT16_MAX, in->handle, false) < 0){
		E_FATAL("failed send command to SG1: off\n");
	}
	g_no_signal = true;
	//pthread_mutex_unlock(&mp);

	pthread_exit(0);
}
*/

/*
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
			
			// Отправляем в модем команду на выдачу/запись выборки в течении INT16_MAX сек
			if(send_command(INT16_MAX, handle, true) < 0){
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


				odd_128 += even_128;			
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
*/

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
	E_INFO("write to file...");

	
	int tail = 0; // длина хвоста
	int tail0;
	for (int s = 0;;s++)
	{
		
		receiving(c_frame, frame_cbit*n_frames);
//		int l = fwrite(c_frame, 1, frame_cbit*n_frames, file);
//		fflush(file);		

//		if(l != frame_cbit*n_frames){
//			E_FATAL("Write file is false");
//		}else{
//			E_INFO("Write is true: %d\n",l);
//		}
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
		
		
		for (i = 0, even_128 = 0, odd_128 = 0; i < n_frames; i++)
		{

			// В зависимости от номера прохода выбираем буфер
			status = vocoder_process(dec, &c_frame[frame_cbit*i], &b1[tail + (frame_samples*i)]);

			if (status < 0 ) 
			{
				E_FATAL("decoder failed, status = %d\n", status);
				goto out;
			}

			// количество блоков (по 128), готовое для записи
			even_128 = ((tail + frame_samples*(i + 1)) / 128) - odd_128;

			if(even_128){
				int l = fwrite(&b1[128 * odd_128], sizeof(short), 128 * even_128, file);
				fflush(file);
				odd_128 += even_128;				
			}
		}

		// Перемещаем хвост в начало буфера
		tail0 = (tail + n_frames * frame_samples) % 128;
		memcpy(b2, &b1[tail + (n_frames * frame_samples) - tail0], sizeof(short) * tail0);
		tail = tail0;


	}
out:


	free(buffer1);
	free(buffer2);
	free(c_frame);

    return 0;
}




