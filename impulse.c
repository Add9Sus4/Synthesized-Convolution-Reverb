/*
 * impulse.c
 *
 *  Created on: Oct 2, 2015
 *      Author: Dawson
 */

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
#include "impulse.h"

InputAudioData *allocateInputAudioBuffers(Vector vector) {
	InputAudioData *data_ptr = (InputAudioData *) malloc(
			sizeof(InputAudioData));
	data_ptr->audioBlocks = (float **) malloc(sizeof(float *) * vector.size);
	data_ptr->size = vector.size;
	int i;
	for (i = 0; i < vector.size; i++) {
		data_ptr->audioBlocks[i] = (float *) calloc(vector_get(&vector, i),
				sizeof(float));
	}
	return data_ptr;
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
	int blockIndex, sampleIndex;
	for (blockIndex = 0; blockIndex < vector.size; blockIndex++) {
		printf("\nBLOCK [%d]\n\n", blockIndex);
		for (sampleIndex = 0; sampleIndex < vector_get(&vector, blockIndex);
				sampleIndex++) {
			printf("Sample #[%d]: Re: %f, Im: %f\n", sampleIndex,
					fftData_ptr->fftBlocks[blockIndex][sampleIndex].Re,
					fftData_ptr->fftBlocks[blockIndex][sampleIndex].Im);
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
	// Actually calculate FFTs
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

Vector determineBlockLengths(audioData* impulse) {
	Vector vector;
	vector_init(&vector);
	int remaining_length = impulse->numFrames;
	bool increment = false;
	// Block 1
	int blockSize = 2 * MIN_FFT_BLOCK_SIZE;
	vector_append(&vector, blockSize * 2);
	remaining_length -= blockSize;
	printf("block size: %d\n", blockSize * 2);
	//	printf("Remaining length: %d\n", length);
	blockSize = MIN_FFT_BLOCK_SIZE;
	// Subsequent blocks
	while (remaining_length > 0) {
		remaining_length -= blockSize;

		/*
		 * The block size must be multiplied by 2 in order for
		 * there to be enough room for convolution down the line
		 */
		vector_append(&vector, blockSize * 2);
		printf("block size: %d\n", blockSize * 2);
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
