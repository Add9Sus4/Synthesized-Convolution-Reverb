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

BlockData *allocateBlockBuffers(Vector vector, audioData *impulse) {

	BlockData* data_ptr = (BlockData*) malloc(sizeof(BlockData));
	data_ptr->audioBlocks1 = (float**) malloc(sizeof(float*) * vector.size);
	data_ptr->audioBlocks2 = (float**) malloc(sizeof(float*) * vector.size);
	data_ptr->size = vector.size;

	// If the impulse was mono
	if (impulse->numChannels == MONO) {

		int i;
		for (i = 0; i < vector.size; i++) {
			data_ptr->audioBlocks1[i] = (float*) calloc(vector_get(&vector, i),
					sizeof(float));
		}

	}

	// If the impulse was stereo
	if (impulse->numChannels == STEREO) {

		int i;
		for (i = 0; i < vector.size; i++) {
			data_ptr->audioBlocks1[i] = (float*) calloc(vector_get(&vector, i),
					sizeof(float));
			data_ptr->audioBlocks2[i] = (float*) calloc(vector_get(&vector, i),
					sizeof(float));
		}

	}

	return data_ptr;
}

void partitionImpulseIntoBlocks(Vector vector, BlockData* data_ptr,
		audioData* impulse) {

	int blockNumber, sampleIndex;
	int offset = 0;

	// If impulse is mono
	if (impulse->numChannels == MONO) {

		// For each block
		for (blockNumber = 0; blockNumber < vector.size; blockNumber++) {
			// Copy the appropriate samples from the impulse
			for (sampleIndex = 0;
					sampleIndex < vector_get(&vector, blockNumber) / 2;
					sampleIndex++) {
				data_ptr->audioBlocks1[blockNumber][sampleIndex] =
						impulse->buffer1[sampleIndex + offset];
				//			printf("data_ptr->audioBlocks[%d][%d] = %f, impulse->buffer[%d]\n",
				//					blockNumber, sampleIndex,
				//					impulse->buffer[sampleIndex + offset],
				//					(sampleIndex + offset));
			}
			// Increase the offset by the size of the last block added
			offset += vector_get(&vector, blockNumber) / 2;
		}

	}

	// If impulse is stereo
	if (impulse->numChannels == STEREO) {

		// For each block
		for (blockNumber = 0; blockNumber < vector.size; blockNumber++) {
			// Copy the appropriate samples from the impulse
			for (sampleIndex = 0;
					sampleIndex < vector_get(&vector, blockNumber) / 2;
					sampleIndex++) {

				// Copy left channel of impulse
				data_ptr->audioBlocks1[blockNumber][sampleIndex] =
						impulse->buffer1[sampleIndex + offset];

				// Copy right channel of impulse
				data_ptr->audioBlocks2[blockNumber][sampleIndex] =
						impulse->buffer2[sampleIndex + offset];

			}
			// Increase the offset by the size of the last block added
			offset += vector_get(&vector, blockNumber) / 2;
		}

	}

}

FFTData* allocateFFTBuffers(BlockData* data_ptr, Vector vector,
		audioData *impulse) {

	// Pre-compute ffts of each impulse block
	FFTData* fftData_ptr = (FFTData*) malloc(sizeof(FFTData));

	fftData_ptr->size = data_ptr->size;

	fftData_ptr->fftBlocks1 = (complex**) malloc(
			sizeof(complex*) * fftData_ptr->size);

	fftData_ptr->fftBlocks2 = (complex**) malloc(
			sizeof(complex*) * fftData_ptr->size);

	int i, j;
	for (i = 0; i < fftData_ptr->size; i++) {

		// all blockSizes are already powers of 2
		fftData_ptr->fftBlocks1[i] = (complex*) malloc(
				sizeof(complex) * vector_get(&vector, i));

		fftData_ptr->fftBlocks2[i] = (complex*) malloc(
				sizeof(complex) * vector_get(&vector, i));

	}

	// If the impulse is mono
	if (impulse->numChannels == MONO) {

		// Actually calculate FFTs
		complex *temp = NULL;
		for (i = 0; i < fftData_ptr->size; i++) {
			temp = calloc(vector_get(&vector, i), sizeof(complex));
			for (j = 0; j < vector_get(&vector, i); j++) {
				fftData_ptr->fftBlocks1[i][j].Re = data_ptr->audioBlocks1[i][j];
			}
			fft(fftData_ptr->fftBlocks1[i], vector_get(&vector, i), temp);
		}

		free(temp);

	}

	// If the impulse is stereo
	if (impulse->numChannels == STEREO) {

		// Actually calculate FFTs
		complex *temp = NULL;
		complex *temp2 = NULL;
		for (i = 0; i < fftData_ptr->size; i++) {
			temp = calloc(vector_get(&vector, i), sizeof(complex));
			temp2 = calloc(vector_get(&vector, i), sizeof(complex));
			for (j = 0; j < vector_get(&vector, i); j++) {
				fftData_ptr->fftBlocks1[i][j].Re = data_ptr->audioBlocks1[i][j]; // left channel
				fftData_ptr->fftBlocks2[i][j].Re = data_ptr->audioBlocks2[i][j]; // right channel
			}
			fft(fftData_ptr->fftBlocks1[i], vector_get(&vector, i), temp);
			fft(fftData_ptr->fftBlocks2[i], vector_get(&vector, i), temp2);
		}

		free(temp);
		free(temp2);

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
//	printf("block size: %d\n", blockSize * 2);
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
//		printf("block size: %d\n", blockSize * 2);
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
