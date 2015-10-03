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
	float **audioBlocks;
	int size;
} BlockData;

typedef struct FFTData {
	complex **fftBlocks;
	int size;
} FFTData;

typedef struct InputAudioData {
	float **audioBlocks;
	int size;
} InputAudioData;

InputAudioData *allocateInputAudioBuffers(Vector vector);

BlockData *allocateBlockBuffers(Vector vector);

void partitionImpulseIntoBlocks(Vector vector, BlockData* data_ptr,
		audioData* impulse);

void printPartitionedImpulseData(Vector vector, BlockData* data_ptr);

void printPartitionedImpulseFFTData(Vector vector, FFTData* fftData_ptr);

void recombineBlocks(int length, Vector vector, BlockData* data_ptr);

FFTData* allocateFFTBuffers(BlockData* data_ptr, Vector vector);

Vector determineBlockLengths(audioData* impulse);

#endif /* IMPULSE_H_ */
