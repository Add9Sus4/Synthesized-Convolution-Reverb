/*
 * dawsonaudio.h
 *
 *  Created on: Sep 20, 2015
 *      Author: Dawson
 */

#ifndef DAWSONAUDIO_H_
#define DAWSONAUDIO_H_

#define MONO				1
#define STEREO				2

// Struct for audio data
typedef struct audioData {

	int numChannels;
	int numFrames;
	int sampleRate;
	char *fileName;
	float *buffer1;
	float *buffer2;

} audioData;

void free_audioData(audioData *audio);

float *normalizeBuffer(float *buffer, int length);

// Read audio data to buffer
audioData *fileToBuffer( char *fileName );

// Write data in buffer to .wav file
void writeWavFile( float *audio, int sample_rate, int numChannels, int numFrames, int numOutChannels, char *outFileName );

// Zero-pad audioData buffer to next power of two
audioData *zeroPadToNextPowerOfTwo(audioData *audio);

// Calculates the next power of two
int calculateNextPowerOfTwo(int length);

// FFT convolution
void fastConvolve(audioData *signal, audioData *impulse, float dry_wet, char *outFileName);

// Direct convolution
void slowConvolve(audioData *signal, audioData *impulse, float dry_wet, char *outFileName);

#endif /* DAWSONAUDIO_H_ */
