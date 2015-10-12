/*
 * impulse.h
 *
 *  Created on: Oct 2, 2015
 *      Author: Dawson
 */

#ifndef IMPULSE_H_
#define IMPULSE_H_

#define SAMPLE_RATE			44100
#define MONO				1
#define STEREO				2
#define MIN_FFT_BLOCK_SIZE	128


typedef struct BlockData {
	float **audioBlocks1;
	float **audioBlocks2;
	int size;
} BlockData;

typedef struct FFTData {
	complex **fftBlocks1;
	complex **fftBlocks2;
	int size;
} FFTData;

BlockData *allocateBlockBuffers(Vector vector, audioData *impulse);

void partitionImpulseIntoBlocks(Vector vector, BlockData* data_ptr,
		audioData* impulse);

FFTData* allocateFFTBuffers(BlockData* data_ptr, Vector vector, audioData *impulse);

Vector determineBlockLengths(audioData* impulse);

#endif /* IMPULSE_H_ */
