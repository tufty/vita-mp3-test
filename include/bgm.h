#pragma once

#include <psp2/types.h>

/* Event flag used to synchronise threads */
extern SceUID bgm_event_flag;

/* Flags we can set */
#define EVF_START_DECODE 1
#define EVF_FRAME_DECODED 2
#define EVF_TRACK_DECODED 4
#define EVF_START_PLAYBACK 8
#define EVF_FRAME_PLAYED 16
#define EVF_START_ANALYSE 32
#define EVF_FRAME_ANALYSED 64


#define SCE_KERNEL_HIGHEST_PRIORITY_USER 64
#define SCE_KERNEL_LOWEST_PRIORITY_USER 191
#define SCE_KERNEL_DEFAULT_PRIORITY_USER 0x10000100
#define SCE_KERNEL_CURRENT_THREAD_PRIORITY 0

#define SCE_KERNEL_THREAD_STACK_SIZE_MIN 0x1000 
#define SCE_KERNEL_THREAD_STACK_SIZE_MAX 0x2000000

/* Pointers into the PCM data block */ 
volatile extern SceInt32 pcm_decoder_frame_counter;
volatile extern SceInt32 pcm_playback_frame_counter;
volatile extern SceInt32 pcm_analysis_frame_counter;

void bgm_init ();
void bgm_start ();

int bgm_decode_thread_worker (SceSize args, void * arg);
int bgm_play_thread_worker (SceSize args, void * arg);
int bgm_analyse_thread_worker (SceSize args, void * arg);
