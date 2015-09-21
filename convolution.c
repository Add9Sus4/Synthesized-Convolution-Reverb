/*
 * convolution.c
 *
 *  Created on: Sep 20, 2015
 *      Author: Dawson
 */

#define SAMPLE_RATE			44100
#define MONO				1
#define STEREO				2
#define MIN_FFT_BLOCK_SIZE	128

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sndfile.h>
#include <math.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <portaudio.h>
#include <ncurses.h>
#include <sys/resource.h>
#include <sys/times.h>
#include "dawsonaudio.h"
#include "convolve.h"
#include "vector.h"

typedef struct BlockData {
	float **audioBlocks;
	int size;
} BlockData;

typedef struct FFTData {
	complex **fftBlocks;
	int size;
} FFTData;

/* Returns number of seconds between b and a. */
double calculate(struct rusage *b, struct rusage *a) {
	if (b == NULL || a == NULL)
		return 0;
	else
		return ((((a->ru_utime.tv_sec * 1000000 + a->ru_utime.tv_usec)
				- (b->ru_utime.tv_sec * 1000000 + b->ru_utime.tv_usec))
				+ ((a->ru_stime.tv_sec * 1000000 + a->ru_stime.tv_usec)
						- (b->ru_stime.tv_sec * 1000000 + b->ru_stime.tv_usec)))
				/ 1000000.);
}

BlockData *allocateBlockBuffers(Vector vector) {
	BlockData* data_ptr = (BlockData*) malloc(sizeof(BlockData));
	data_ptr->audioBlocks = (float**) malloc(sizeof(float*) * vector.size);
	data_ptr->size = vector.size;
	int i;
	for (i = 0; i < vector.size; i++) {
		data_ptr->audioBlocks[i] = (float*) malloc(
				sizeof(float) * vector_get(&vector, i));
//		printf("Block length[%d]: %d\n", i, vector_get(&vector, i));
	}

	return data_ptr;
}

void partitionImpulseIntoBlocks(Vector vector, BlockData* data_ptr,
		audioData* impulse) {
	int blockNumber, sampleIndex;
	int offset = 0;
	// For each block
	for (blockNumber = 0; blockNumber < vector.size; blockNumber++) {
		// Copy the appropriate samples from the impulse
		for (sampleIndex = 0; sampleIndex < vector_get(&vector, blockNumber);
				sampleIndex++) {
			data_ptr->audioBlocks[blockNumber][sampleIndex] =
					impulse->buffer[sampleIndex + offset];
		}
		// Increase the offset by the size of the last block added
		offset += vector_get(&vector, blockNumber);
	}
}

void printPartitionedImpulseData(Vector vector, BlockData* data_ptr) {
	int i, j;
	for (i = 0; i < vector.size; i++) {
		printf("\nBLOCK [%d]\n\n", i);
		for (j = 0; j < vector_get(&vector, i); j++) {
			printf("Sample #[%d]: %f\n", j, data_ptr->audioBlocks[i][j]);
		}
	}
}

void printPartitionedImpulseFFTData(Vector vector, FFTData* fftData_ptr) {
	int i, j;
	for (i = 0; i < vector.size; i++) {
		printf("\nBLOCK [%d]\n\n", i);
		for (j = 0; j < vector_get(&vector, i); j++) {
			printf("Sample #[%d]: Re: %f, Im: %f\n", j,
					fftData_ptr->fftBlocks[i][j].Re,
					fftData_ptr->fftBlocks[i][j].Im);
		}
	}
}

void recombineBlocks(int length, Vector vector, BlockData* data_ptr) {
	//
	//	writeWavFile(impulse, impulse->sampleRate, impulse->numChannels,
	//			"output.wav");
	audioData* output = (audioData*) malloc(sizeof(audioData));
	output->buffer = (float*) malloc(sizeof(float) * length);
	output->numChannels = MONO;
	output->numFrames = length;
	output->sampleRate = SAMPLE_RATE;
	int i, j;
	int counter = 0;
	for (i = 0; i < vector.size; i++) {
		for (j = 0; j < vector_get(&vector, i); j++) {
			output->buffer[counter++] = data_ptr->audioBlocks[i][j];
		}
	}
	writeWavFile(output->buffer, output->sampleRate, output->numChannels,
			output->numFrames, output->numChannels, "recombined.wav");
}

FFTData* allocateFFTBuffers(BlockData* data_ptr, Vector vector) {
	// Pre-compute ffts of each impulse block
	FFTData* fftData_ptr = (FFTData*) malloc(sizeof(FFTData));
	fftData_ptr->fftBlocks = (complex**) malloc(
			sizeof(complex*) * fftData_ptr->size);
	fftData_ptr->size = data_ptr->size;
	int i, j;
	for (i = 0; i < fftData_ptr->size; i++) {
		fftData_ptr->fftBlocks[i] = (complex*) malloc(
				sizeof(complex) * vector_get(&vector, i)); // all blockSizes are already powers of 2
	}
	complex *temp = NULL;
	for (i = 0; i < fftData_ptr->size; i++) {
		temp = calloc(vector_get(&vector, i), sizeof(complex));
		for (j = 0; j < vector_get(&vector, i); j++) {
//			printf("data_ptr->audioBlocks[%d][%d]: %f\n", i, j, data_ptr->audioBlocks[i][j]);
			fftData_ptr->fftBlocks[i][j].Re = data_ptr->audioBlocks[i][j];
		}
		fft(fftData_ptr->fftBlocks[i], vector_get(&vector, i), temp);
	}
	return fftData_ptr;
}

/*
 *  Description:  Callback for Port Audio
 */
static int paCallback(const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags, void *userData) {
	/* Implement a delay line and save results into the output file */
	int i;

	float *inBuf = (float*) inputBuffer;
	float *outBuf = (float*) outputBuffer;

	// This loop is used to test interrupting the paCallback
//	for (i=0;i<INT_MAX/2000; i++) {
//	}

	// FFT of block
	{
		complex *signal = calloc(framesPerBuffer, sizeof(complex));
		complex *temp = calloc(framesPerBuffer, sizeof(complex));
		for (i = 0; i < framesPerBuffer; i++) {
			signal[i].Re = inBuf[i];
		}

		fft(signal, framesPerBuffer, temp);

		ifft(signal, framesPerBuffer, temp);

		for (i = 0; i < framesPerBuffer; i++) {
			outBuf[i] = signal[i].Re / 50;
		}

		free(signal);
		free(temp);
	}

//	for (i = 0; i < framesPerBuffer; i++) {
//
//		// Send each sample to the output
//		outBuf[i] = inBuf[i];
//	}

	return paContinue;
}

Vector determineBlockLengths(audioData* impulse) {
	Vector vector;
	vector_init(&vector);
	int remaining_length = impulse->numFrames;
	bool increment = false;
	// Block 1
	int blockSize = 2 * MIN_FFT_BLOCK_SIZE;
	vector_append(&vector, blockSize);
	remaining_length -= blockSize;
//	printf("block size: %d\n", blockSize);
	//	printf("Remaining length: %d\n", length);
	blockSize = MIN_FFT_BLOCK_SIZE;
	// Subsequent blocks
	while (remaining_length > 0) {
		remaining_length -= blockSize;
		vector_append(&vector, blockSize);
//		printf("block size: %d\n", blockSize);
		//		printf("Remaining length: %d\n", length);
		if (increment) {
			blockSize *= 2;
			increment = false;
		} else {
			increment = true;
		}
	}
	return vector;
}

void runPortAudio() {
	PaStream* stream;
	PaStreamParameters outputParameters;
	PaStreamParameters inputParameters;
	PaError err;
	/* Initialize PortAudio */
	Pa_Initialize();
	/* Set output stream parameters */
	outputParameters.device = Pa_GetDefaultOutputDevice();
	outputParameters.channelCount = MONO;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(
			outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	/* Set input stream parameters */
	inputParameters.device = Pa_GetDefaultInputDevice();
	inputParameters.channelCount = MONO;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency =
			Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	/* Open audio stream */
	err = Pa_OpenStream(&stream, &inputParameters, &outputParameters,
	SAMPLE_RATE, MIN_FFT_BLOCK_SIZE, paNoFlag, paCallback, NULL);
	if (err != paNoError) {
		printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
	}
	/* Start audio stream */
	err = Pa_StartStream(stream);
	if (err != paNoError) {
		printf("PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
	}
	/* Get user input */
	char ch = '0';
	while (ch != 'q') {
		printf("Press 'q' to finish execution: ");
		ch = getchar();
	}
	err = Pa_StopStream(stream);
	/* Stop audio stream */
	if (err != paNoError) {
		printf("PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
	}
	/* Close audio stream */
	err = Pa_CloseStream(stream);
	if (err != paNoError) {
		printf("PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
	}
	/* Terminate audio stream */
	err = Pa_Terminate();
	if (err != paNoError) {
		printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
	}
}

void loadImpulse() {
	audioData* impulse = fileToBuffer("churchIR.wav");
	audioData* signal = fileToBuffer("what.wav");
	int length = calculateNextPowerOfTwo(
			impulse->numChannels * impulse->numFrames);
	impulse = zeroPadToNextPowerOfTwo(impulse);
	Vector blockLengthVector = determineBlockLengths(impulse);
	BlockData* data_ptr = allocateBlockBuffers(blockLengthVector);
//	partitionImpulseIntoBlocks(blockLengthVector, data_ptr, impulse);
	FFTData* fftData_ptr = allocateFFTBuffers(data_ptr, blockLengthVector);
//	printPartitionedImpulseFFTData(blockLengthVector, fftData_ptr);
}

int main(int argc, char **argv) {

	// structs for timing data
	struct rusage before, after;

	double time = 0;
	getrusage(RUSAGE_SELF, &before);

	int i;
	for (i = 0; i < INT_MAX / 2000; i++) {
	}

	getrusage(RUSAGE_SELF, &after);

	time += calculate(&before, &after);

	printf("\nTime: %f\n", time);
	printf("Time per block: %f\n", ((float) MIN_FFT_BLOCK_SIZE / SAMPLE_RATE));

	loadImpulse();

	runPortAudio();

	return 0;
}

