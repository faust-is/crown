#include "wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// по умолчанию: PCM
int write_wav_stereo_header(FILE *_out, int32_t sample_rate, int32_t frame_count, int16_t audio_format)
{
    int retval;
    
    wavfile_header_t wav_header;
    int32_t subchunk2_size;
    int32_t chunk_size;
    
    size_t write_count;
    
    if(_out == NULL)
        return -1;


    subchunk2_size  = frame_count * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    chunk_size      = 4 + (8 + SUBCHUNK1SIZE) + (8 + subchunk2_size);
    
    wav_header.ChunkID[0] = 'R';
    wav_header.ChunkID[1] = 'I';
    wav_header.ChunkID[2] = 'F';
    wav_header.ChunkID[3] = 'F';
    
    wav_header.ChunkSize = chunk_size;
    
    wav_header.Format[0] = 'W';
    wav_header.Format[1] = 'A';
    wav_header.Format[2] = 'V';
    wav_header.Format[3] = 'E';
    
    wav_header.Subchunk1ID[0] = 'f';
    wav_header.Subchunk1ID[1] = 'm';
    wav_header.Subchunk1ID[2] = 't';
    wav_header.Subchunk1ID[3] = ' ';
    
    wav_header.Subchunk1Size = SUBCHUNK1SIZE;
    wav_header.AudioFormat = audio_format;
    wav_header.NumChannels = NUM_CHANNELS;
    wav_header.SampleRate = sample_rate;
    wav_header.ByteRate = BYTE_RATE;
    wav_header.BlockAlign = BLOCK_ALIGN;
    wav_header.BitsPerSample = BITS_PER_SAMPLE;
    
    wav_header.Subchunk2ID[0] = 'd';
    wav_header.Subchunk2ID[1] = 'a';
    wav_header.Subchunk2ID[2] = 't';
    wav_header.Subchunk2ID[3] = 'a';
    wav_header.Subchunk2Size = subchunk2_size;
    
    write_count = fwrite( &wav_header, sizeof(wavfile_header_t), 1, _out);
    retval = (1 != write_count)? -1 : 0;
    fflush(_out);
    
    return retval;
}