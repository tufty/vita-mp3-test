#include "bgm.h"
#include "debugScreen.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"

#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/audioout.h>
#include <psp2/audiodec.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#define SCE_OK 0
#define SCE_KERNEL_EVF_ATTR_MULTI 0x00001000U

#define PCM_CHANNELS 2
#define PCM_FRAMES 1024
#define PCM_FRAMES_BLOCK (PCM_FRAMES / 16)
#define PCM_FRAME_SIZE (SCE_AUDIODEC_ROUND_UP(sizeof(SceInt16) * PCM_CHANNELS * SCE_AUDIODEC_MP3_MAX_SAMPLES))

#define MP3_FRAME_HEADER_SIZE 4

#define MP3_BPS (SCE_AUDIODEC_MP3_MAX_SAMPLES / 8)
#define MP3_MAX_BITRATE 320000
#define MP3_MAX_SAMPRATE 48000
#define MP3_ES_SIZE (((MP3_BPS * MP3_MAX_BITRATE) /  MP3_MAX_SAMPRATE) + 1)

#define MP3_FRAME_SIZE (SCE_AUDIODEC_ROUND_UP(MP3_FRAME_HEADER_SIZE + MP3_ES_SIZE))

#define ID3V2_HEADER_SIZE (10 - MP3_FRAME_HEADER_SIZE)

#define FFT_BANDS 7
#define FFT_HISTORY_SIZE 32

#define bgm_file_name "ux0:/data/test.mp3"

SceSize _stack_size = 0x4000;

SceUID _decode_event_flag;
SceUID _analysis_event_flag;
SceUID _playback_event_flag;

SceUID _bgm_decode_thread;
SceUID _bgm_play_thread;
SceUID _bgm_analyse_thread;

volatile SceUInt32 pcm_decoder_frame_counter;
volatile SceUInt32 pcm_analysis_frame_counter;
volatile SceUInt32 pcm_playback_frame_counter;

SceInt16 * _pcm_buffer;
SceUInt8 * _mp3_buffer;

SceInt16 * pcm_pointer (SceUInt32 count) {
    return _pcm_buffer + ((PCM_FRAME_SIZE / sizeof(SceInt16)) * (count % PCM_FRAMES));
}

struct {
    int freq;
    int format;
} audio;

// Do all the allocations, and kick off the threads.
void bgm_init () {  
    int res = SCE_OK;
    /* and the mp3 buffer. Enough for the biggest frame possible */
    _mp3_buffer = memalign(SCE_AUDIODEC_ALIGNMENT_SIZE, MP3_FRAME_SIZE);
    if (NULL == _mp3_buffer) {
	psvDebugScreenPuts("Failed to allocate mp3 buffer\n");
	while (1);
    }
    bzero(_mp3_buffer, MP3_FRAME_SIZE);
  
    /* Create the PCM buffer, easy enough, space for N frames */
    _pcm_buffer = memalign(SCE_AUDIODEC_ALIGNMENT_SIZE, PCM_FRAME_SIZE * PCM_FRAMES);
    if (NULL == _pcm_buffer) {
	psvDebugScreenPuts("Failed to allocate pcm buffer\n");
	while (1);
    }
    bzero(_pcm_buffer, PCM_FRAME_SIZE * PCM_FRAMES);

    /* Create event flag */
    res = sceKernelCreateEventFlag("_decode_event_flag", SCE_KERNEL_EVF_ATTR_MULTI, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to create event flag : 0x%x\n", res);
	while (1);    
    }
    _decode_event_flag = res;
    res = sceKernelCreateEventFlag("_playback_event_flag", SCE_KERNEL_EVF_ATTR_MULTI, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to create event flag : 0x%x\n", res);
	while (1);    
    }
    _playback_event_flag = res;
    res = sceKernelCreateEventFlag("_analysis_event_flag", SCE_KERNEL_EVF_ATTR_MULTI, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to create event flag : 0x%x\n", res);
	while (1);    
    }
    _analysis_event_flag = res;

    // Create the threads
    res = sceKernelCreateThread("bgm_decode_thread", bgm_decode_thread_worker, SCE_KERNEL_CURRENT_THREAD_PRIORITY, _stack_size, 0, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to create decode thread : 0x%x\n", res);
	while (1);    
    }
    _bgm_decode_thread = res;

    res = sceKernelCreateThread("bgm_play_thread", bgm_play_thread_worker, SCE_KERNEL_CURRENT_THREAD_PRIORITY, _stack_size, 0, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to create play thread : 0x%x\n", res);
	while (1);    
    }
    _bgm_play_thread = res;
  
    res = sceKernelCreateThread("bgm_analyse_thread", bgm_analyse_thread_worker, SCE_KERNEL_CURRENT_THREAD_PRIORITY, _stack_size, 0, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to create analysis thread : 0x%x\n", res);
	while (1);    
    }
    _bgm_analyse_thread = res;

    // And start them.  Initially they will do nothing, waiting for an external event
    res = sceKernelStartThread(_bgm_decode_thread, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to start decode thread : 0x%x\n", res);
	while (1);    
    }
    res = sceKernelStartThread(_bgm_play_thread, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to start playback thread : 0x%x\n", res);
	while (1);    
    }
    res = sceKernelStartThread(_bgm_analyse_thread, 0, NULL);
    if (0 > res) {
	psvDebugScreenPrintf("Failed to start analysis thread : 0x%x\n", res);
	while (1);    
    }
}

int bitrates[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};
float samprates[4] = {44.1, 48, 32, 0};

int bgm_decode_thread_worker (SceSize args, void * arg) {
    int ret = SCE_OK;
    SceUID fdesc;
    int flen, foffset;
    SceAudiodecCtrl ctrl;
    SceAudiodecInfo info;
    SceAudiodecInitParam init;
  

    init.size = sizeof(init.mp3);
    init.mp3.totalStreams = 1;
  
    ret = sceAudiodecInitLibrary(SCE_AUDIODEC_TYPE_MP3, &init);
    if (0 > ret) {
	psvDebugScreenPrintf("Failed to initialise audiodecInitParam : 0x%x\n", ret);
	while (1);    
    }
 
    while (1 == 1) {
	/* Wait to be told to start */
	sceKernelWaitEventFlag(_decode_event_flag, EVF_START_DECODE, SCE_EVENT_WAITCLEAR_PAT, NULL, NULL);

	pcm_decoder_frame_counter = pcm_playback_frame_counter = pcm_analysis_frame_counter = 0;

	bzero (&ctrl, sizeof(SceAudiodecCtrl));
	bzero (&info, sizeof(SceAudiodecInfo));

	ctrl.size       = sizeof(SceAudiodecCtrl);
	ctrl.wordLength = SCE_AUDIODEC_WORD_LENGTH_16BITS;
	ctrl.pInfo      = &info;
	ctrl.pEs        = _mp3_buffer;
	ctrl.maxEsSize  = MP3_FRAME_SIZE;
	ctrl.pPcm       = pcm_pointer(pcm_decoder_frame_counter);
	ctrl.maxPcmSize = PCM_FRAME_SIZE;
	info.size       = sizeof(info.mp3);

	/* open file */
	if ((fdesc = sceIoOpen (bgm_file_name, SCE_O_RDONLY, 0)) < 0) {
	    psvDebugScreenPrintf("Failed to load file %s\n", bgm_file_name);
	    while (1);    
	} else {
	    /* find its length */
	    flen = sceIoLseek(fdesc, 0, SCE_SEEK_END) ;
	    foffset = 0;

	    /* quick sanity check */
	    if (MP3_FRAME_HEADER_SIZE >= flen) {
		psvDebugScreenPrintf("Wierd file length %i\n", flen);
		while (1);    
	    }

	    while (foffset < flen) {

		/* Read in the maximum frame size, we'll roll back from this to the actual end of frame later */
		sceIoPread(fdesc, _mp3_buffer, MP3_FRAME_SIZE, foffset);
	
		/* deal with IDv3 tags */
		if (('I' == _mp3_buffer[0]) &&
		    ('D' == _mp3_buffer[1]) &&
		    ('3' == _mp3_buffer[2]))
		{	    
		    /* how long is the tags block? */
		    int tags_length = ((_mp3_buffer[6] & 0x7f) << 21) +
			((_mp3_buffer[7] & 0x7f) << 14) +
			((_mp3_buffer[8] & 0x7f) << 7) +
			(_mp3_buffer[9] & 0x7f) +
			((_mp3_buffer[5] & 0x10) ? 20 : 10);
	    
		    /* skip the tags */
		    foffset = sceIoLseek(fdesc, tags_length, SCE_SEEK_SET);
		}
		/* ID3v1 tags at the end of the file */
		else if (('T' == _mp3_buffer[0] )&&
			 ('A' == _mp3_buffer[1] )&&
			 ('G' ==  _mp3_buffer[2] ))
		{
		    /* These occur at the end of the file, we're done */
		    foffset = flen;
		}
		/* is this a proper MP3 frame header */
		else if ((0xff == _mp3_buffer[0]) &&
			 (0xfa == (_mp3_buffer[1] & 0xfe)))
		{
		    /* Do we have a decoder? If not, make one, and set up the defaults for this file*/
		    if (0 == ctrl.handle) {
	      
			info.mp3.version = (_mp3_buffer[1] >> 3) & 0x03;
			info.mp3.ch = (_mp3_buffer[3] & 0xb0) == 0xb0 ? 1 : 2;

			audio.format = (_mp3_buffer[3] & 0xb0) == 0xb0 ? SCE_AUDIO_OUT_PARAM_FORMAT_S16_MONO : SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO;
			switch ((_mp3_buffer[2] >> 2) & 0x03) {
			case 0:
			    audio.freq = 44100;
			    break;
			case 1:
			    audio.freq = 48000;
			    break;
			case 2:
			    audio.freq = 32000;
			    break;
			default:
			    psvDebugScreenPrintf("MP3 header frequency broken\n");
			    while (1);    
			}

			/* create our decoder */
			ret = sceAudiodecCreateDecoder(&ctrl, SCE_AUDIODEC_TYPE_MP3);
			if (0 > ret) {
			    psvDebugScreenPrintf("Failed to create decoder 0x%x\n", ret);
			    while (1);    
			}

			sceKernelSetEventFlag(_playback_event_flag, EVF_START_PLAYBACK);
			sceKernelSetEventFlag(_analysis_event_flag, EVF_START_ANALYSE);
		    }
	      
		    /* And decode a frame */
		    ctrl.pPcm = pcm_pointer(pcm_decoder_frame_counter);
	    
		    ret = sceAudiodecDecodeNFrames(&ctrl, 1);
		    if (0 > ret) {
			psvDebugScreenPrintf("Failed to decode frame 0x%x\n", ret);
			while (1);    
		    }

		    /* fix up the offset */
		    foffset += ctrl.inputEsSize;	    

		    /* and the frame count / pcm buffer pointer */
		    pcm_decoder_frame_counter++;
	    	    
		    /* sceKernelSetEventFlag(_playback_event_flag, EVF_FRAME_DECODED); */
		    sceKernelSetEventFlag(_analysis_event_flag, EVF_FRAME_DECODED);

		    /* if we are stalled waiting for analysis or playback, wait */
		    while ((pcm_decoder_frame_counter % PCM_FRAMES) == (pcm_playback_frame_counter % PCM_FRAMES)) {
			sceKernelWaitEventFlag(_decode_event_flag, EVF_FRAME_PLAYED, SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR_PAT, NULL, NULL);
		    }
		} else {
		    /* Bad frame, crash out */
		    psvDebugScreenPrintf("Bad Frame, header %#02x %#02x %#02x %#02x at %#08x", _mp3_buffer[0], _mp3_buffer[1], _mp3_buffer[2], _mp3_buffer[3], foffset);
		    foffset = flen;
		}
	    }
	    sceKernelSetEventFlag(_playback_event_flag, EVF_TRACK_DECODED);
	    sceKernelSetEventFlag(_analysis_event_flag, EVF_TRACK_DECODED);
      
	}
    }
    return ret;
}

void play_available_frames(SceUID port) {
    /* Play frames until we hit the encoder pointer, then stall */
    while (pcm_playback_frame_counter < pcm_decoder_frame_counter) {
	sceKernelSetEventFlag(_analysis_event_flag, EVF_FRAME_PLAYED);
	int ret = sceAudioOutOutput(port, pcm_pointer(pcm_playback_frame_counter));
	if (0 > ret) {
	    psvDebugScreenPrintf("Play failed frame %u error %#08x", pcm_playback_frame_counter, ret);
	}

	pcm_playback_frame_counter++;

	/* After PCM_FRAMES_BLOCK frames played, let the decoder carry on if it needs to*/
	if (0 == (pcm_playback_frame_counter % PCM_FRAMES_BLOCK)) {
	    sceKernelSetEventFlag(_decode_event_flag, EVF_FRAME_PLAYED);
	} else {
	    sceKernelClearEventFlag(_decode_event_flag, EVF_FRAME_PLAYED);
	}   
    }
    /* If we get here we've caught up with the decoder, we're done */
}
  
int bgm_play_thread_worker (SceSize args, void * arg) {

    /* two volume values, one for exach channel */
    int vol[2] = {SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB};
  
    // Open a port for the new stream
    SceUID output_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, SCE_AUDIODEC_MP3_MAX_SAMPLES, 44100, SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
    if (0 > output_port) {
	psvDebugScreenPrintf("Failed to create output port 0x%x\n", output_port);
	while (1);    
    }

    while (1 == 1) {
    
	// Wait until we are supposed to start
	sceKernelWaitEventFlag(_playback_event_flag, EVF_START_PLAYBACK, SCE_EVENT_WAITCLEAR_PAT, NULL, NULL);
    
	// Set the port configuration.  The decode thread has already populated the audio structure from the mp3 frame header.
	sceAudioOutSetConfig(output_port, SCE_AUDIODEC_MP3_MAX_SAMPLES, audio.freq, audio.format);
	// And the volume
	sceAudioOutSetVolume(output_port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);

	// Play frames until the event flag EVF_TRACK_DECODED is set
	while ((sceKernelPollEventFlag(_playback_event_flag, EVF_TRACK_DECODED, SCE_EVENT_WAITCLEAR_PAT, 0)) != SCE_OK) {
	    play_available_frames(output_port);
	}

	/* Play any remaining frames */
	play_available_frames(output_port);

	/* and clean up */
	sceAudioOutOutput(output_port, NULL);

    }
    /* close down the port before exit */
    sceAudioOutReleasePort(output_port);
    return 0;
}

/* Fast integer distance calculation ripped off from Atari's vector game 'mathbox' processor */
/* Approximates sqrt (a^2 + b^2) by max (a, b) + 3/8 min (a, b) */
/* half-sdecent approximation, stays in integer bounds */
SceInt16 fdist_approx (SceInt16 a, SceInt16 b) {
    SceInt16 mn = min(a, b) >> 2;    /* 2/8 of minimum */
    SceInt16 ret = max(a, b) + mn;   /* max + 2/8 of minimum */
    mn >>= 1;                        /* the missing 1/8 of minimum, to get our 3/8) */
    return ret + mn;                
}

int bgm_analyse_thread_worker (SceSize args, void * arg) {
    /* Analysis data is the same size as one PCM frame */
    SceInt16 * _analysis_fft_data = memalign(SCE_AUDIODEC_ALIGNMENT_SIZE, PCM_FRAME_SIZE);
    
    SceInt16 _analysis_band_data[FFT_HISTORY_SIZE][FFT_BANDS] = {0};
    SceInt16 _band_averages[FFT_BANDS] = {0};
    SceUInt16 _bands[FFT_BANDS] = {2, 5, 5, 35, 46, 47, 372}; 
    int fft_counter = 0;

    kiss_fftr_cfg fft_cfg = kiss_fftr_alloc(1024, 0, NULL, NULL);
    
    sceKernelWaitEventFlag(_analysis_event_flag, EVF_START_ANALYSE, SCE_EVENT_WAITCLEAR_PAT, NULL, NULL);
    
    while (1) {
	sceKernelWaitEventFlag(_analysis_event_flag, EVF_FRAME_PLAYED, SCE_EVENT_WAITCLEAR_PAT, NULL, NULL);

	
	/* Distance calc and spread to make real / imaginary */
	SceInt16 * pcm = pcm_pointer(pcm_analysis_frame_counter);
	for (int i = 0; i < 1024; i ++) {
	  _analysis_fft_data[i] = pcm[(i << 1)]; //fdist_approx(abs(pcm[(i << 1)]), abs(pcm[(i << 1) + 1]));
	}

	kiss_fft_scalar * in_buf = (kiss_fft_scalar *)_analysis_fft_data;
	kiss_fft_cpx * out_buf = (kiss_fft_cpx *)(_analysis_fft_data + 1024);
	
	kiss_fftr(fft_cfg, in_buf, out_buf);
	
	for (int b = 0; b < FFT_BANDS; b++) {
	    SceInt32 total = 0;
	    SceInt32 variance = 0;
	    for (int i = 0; i < _bands[b]; i++) {
	      kiss_fft_cpx v = out_buf[(b * FFT_BANDS) + i];
	      //	      total += fdist_approx(abs(v.r), abs(v.i));
	      total += (v.r * v.r) + (v.i * v.i);
	    }
	    _analysis_band_data[fft_counter][b] = total / _bands[b];
	    	    
	    total = 0;
	    for (int i = 0; i < FFT_HISTORY_SIZE; i++) {
	      SceInt16 diff = _analysis_band_data[i][b] - _band_averages[b];
	      total += _analysis_band_data[i][b];
	      variance += diff * diff;
	    }
	    _band_averages[b] = total / FFT_HISTORY_SIZE;
	    variance /= (FFT_HISTORY_SIZE - 1);

	}
	
	pcm_analysis_frame_counter ++;
	fft_counter = (fft_counter + 1) % FFT_HISTORY_SIZE;

	sceKernelSetEventFlag(_decode_event_flag, EVF_FRAME_ANALYSED);

    }
    return 0;
}


// Trigger the load thread to start loading a file
void bgm_start ()
{
    sceKernelSetEventFlag(_decode_event_flag, EVF_START_DECODE);
}
