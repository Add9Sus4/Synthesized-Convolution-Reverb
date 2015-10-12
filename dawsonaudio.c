/*
 * dawsonaudio.c
 *
 *  Created on: Sep 20, 2015
 *      Author: Dawson
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sndfile.h>
#include <math.h>
#include <dirent.h>
#include <unistd.h>
#include "dawsonaudio.h"
#include "convolve.h"

// This function takes a buffer, finds its maximum absolute value, and normalizes it so that the maximum
// is 1 (or -1)
float *normalizeBuffer(float *buffer, int length) {
	float max = 0;
	int i;
	// Find maximum
	for (i = 0; i < length; i++) {
		if (fabs(buffer[i]) > max) {
			max = fabs(buffer[i]);
		}
	}
	// Normalize
	for (i = 0; i < length; i++) {
		buffer[i] /= max;
	}
	return buffer;
}

//-----------------------------------------------------------------------------
// name: fileToBuffer()
// desc: This function opens a .wav file and places it in an appropriately sized
// buffer. It returns a pointer to an audioData struct containing information
// about the file.
//-----------------------------------------------------------------------------
audioData *fileToBuffer(char *fileName) {

	SNDFILE *file;
	SF_INFO fileInfo;

	audioData *newAudioFile;
	newAudioFile = (audioData *) malloc(sizeof(audioData));
	memset(&fileInfo, 0, sizeof(SF_INFO));
	file = sf_open(fileName, SFM_READ, &fileInfo);

	// Exit if the file could not be opened.
	if (file == NULL) {
		printf("Error: could not open file: %s\n", fileName);
		puts(sf_strerror(NULL));
		exit(1);
	}

	int length = fileInfo.frames * fileInfo.channels;

	newAudioFile->numChannels = fileInfo.channels;
	newAudioFile->numFrames = (int) fileInfo.frames;
	newAudioFile->sampleRate = fileInfo.samplerate;
	newAudioFile->buffer1 = (float *) malloc(sizeof(float) * length);
	newAudioFile->buffer2 = (float *) malloc(sizeof(float) * length);
	newAudioFile->fileName = malloc(sizeof(char *) * strlen(fileName));
	strcpy(newAudioFile->fileName, fileName);

	// If the file is a mono file
	if (fileInfo.channels == MONO) {

		// Copy the mono audio into buffer 1
		sf_readf_float(file, newAudioFile->buffer1, length);

	}

	// If the file is a stereo file
	if (fileInfo.channels == STEREO) {

		// Create temporary buffer to store interleaved stereo audio
		float *stereoAudio = (float *) malloc(sizeof(float) * length);

		// Read file into temp buffer
		sf_readf_float(file, stereoAudio, length);

		int i;
		for (i = 0; i < length / 2; i++) {

			// Copy left channel into buffer 1
			newAudioFile->buffer1[i] = stereoAudio[2 * i];

			// Copy right channel into buffer 2
			newAudioFile->buffer2[i] = stereoAudio[2 * i + 1];
		}

		free(stereoAudio);

	}

	sf_close(file);

//	printf("File name: %s\n", newAudioFile->fileName);
//	printf("	Channels: %d\n", newAudioFile->numChannels);
//	printf("	Frames: %d\n", newAudioFile->numFrames);
//	printf("	Sample rate: %d\n", newAudioFile->sampleRate);
//	printf("	Length: %f seconds\n", (float)newAudioFile->numFrames/newAudioFile->sampleRate);

	return newAudioFile;
}

//-----------------------------------------------------------------------------
// name: writeWavFile()
// desc: This function takes an array of floats and writes the data to a .wav file.
//-----------------------------------------------------------------------------
void writeWavFile(float *audio, int sample_rate, int numChannels, int numFrames,
		int numOutChannels, char *outFileName) {
	int i;

	SNDFILE *outfile;
	SF_INFO sfinfo_out;

	memset(&sfinfo_out, 0, sizeof(SF_INFO));
	sfinfo_out.samplerate = sample_rate;
	sfinfo_out.channels = numChannels;
	sfinfo_out.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	if (!sf_format_check(&sfinfo_out)) {
		printf("error: incorrect audio file format\n");
		return;
	}

	if ((outfile = sf_open(outFileName, SFM_WRITE, &sfinfo_out)) == NULL) {
		printf("error, couldn't open the file\n");
		return;
	}

	// get the number of samples in the input audio
	int inputLength = numChannels * numFrames;

	// mono input to mono output
	if (numChannels == 1 && numOutChannels == 1) {
		// allocate the appropriate number of samples in the output audio
		float *output = (float *) malloc(sizeof(float) * inputLength);
		for (i = 0; i < inputLength; i++) {
			output[i] = audio[i];
		}
		sf_writef_float(outfile, output, inputLength);
		free(output);
	}
	// sTEREO INPUT to MONO OUTPUT
	else if (numChannels == 2 && numOutChannels == 1) {
		// Allocate the appropriate number of samples in the output audio
		float *output = (float *) malloc(sizeof(float) * inputLength / 2);
		for (i = 0; i < inputLength / 2; i++) {
			output[i] = (audio[2 * i] + audio[2 * i + 1]) / 2;
		}
		sf_writef_float(outfile, output, inputLength / 2);
		free(output);
	}
	// MONO INPUT to STEREO OUTPUT
	else if (numChannels == 1 && numOutChannels == 2) {
		// Allocate the appropriate number of samples in the output audio
		float *output = (float *) malloc(sizeof(float) * inputLength * 2);
		for (i = 0; i < inputLength; i++) {
			output[2 * i] = audio[i];
			output[2 * i + 1] = audio[i];
		}
		sf_writef_float(outfile, output, inputLength);
		free(output);
	}
	// STEREO INPUT to STEREO OUTPUT
	else if (numChannels == 2 && numOutChannels == 2) {
		// Allocate the appropriate number of samples in the output audio
		float *output = (float *) malloc(sizeof(float) * inputLength);
		for (i = 0; i < inputLength; i++) {
			output[i] = audio[i];
		}
		sf_writef_float(outfile, output, inputLength / 2);
		free(output);
	}
	sf_close(outfile);
}

// Takes audioData struct containing a buffer with audio data in it and zero-pads
// the buffer to the next power of two, updating the numFrames information as well
audioData *zeroPadToNextPowerOfTwo(audioData *audio) {
	int i;

	// If the audio data is mono
	if (audio->numChannels == MONO) {

		// find the next power of two
		int newLength = calculateNextPowerOfTwo(
				audio->numChannels * audio->numFrames);
		// Create new buffer
		float *newBuffer = (float *) calloc(newLength, sizeof(float));
		// initialize the buffer with all zeros
		for (i = 0; i < newLength; i++) {
			newBuffer[i] = 0.0f;
		}
		// Copy data into new buffer
		for (i = 0; i < audio->numChannels * audio->numFrames; i++) {
			newBuffer[i] = audio->buffer1[i];
		}
		// free the old buffer
		free(audio->buffer1);
		// add the new buffer to the audioData struct
		audio->buffer1 = newBuffer;
		// calculate new numFrames variable
		audio->numFrames = newLength;
		// return the new buffer

	}

	// If the audio data is stereo
	if (audio->numChannels == STEREO) {

		int newLength = calculateNextPowerOfTwo(audio->numFrames);

		float *newLeftBuffer = (float *) calloc(newLength, sizeof(float));
		float *newRightBuffer = (float *) calloc(newLength, sizeof(float));

		for (i=0; i<newLength; i++) {
			newLeftBuffer[i] = 0.0f;
			newRightBuffer[i] = 0.0f;
		}

		for (i=0; i<audio->numFrames; i++) {
			newLeftBuffer[i] = audio->buffer1[i];
			newRightBuffer[i] = audio->buffer2[i];
		}

		free(audio->buffer1);
		free(audio->buffer2);

		audio->buffer1 = newLeftBuffer;
		audio->buffer2 = newRightBuffer;

		audio->numFrames = newLength;

	}

	return audio;
}

// Takes an integer (length) and determines the next power of 2 that will be reached
// if we continue to increase the integer
int calculateNextPowerOfTwo(int length) {
	unsigned int x = length;
	while (x != (x & (~x + 1))) {
		x++;
	}
	return x;
}

// This function convolves two signals together (frequency domain multiplication)
// dry_wet is a measure of the ratio between the dry and wet signals. 0 is completely dry
// and 1 is completely wet
void fastConvolve(audioData *signal, audioData *impulse, float dry_wet,
		char *outFileName) {

	// Check for realistic dry_wet values
	if (dry_wet < 0 || dry_wet > 1) {
		printf("Error: dry_wet must be between 0 and 1\n");
		return;
	}

	int i;
	int outputLength = 0;

	// If the signal is mono and the impulse is mono
	if (signal->numChannels == 1 && impulse->numChannels == 1) {
		float max = 0;
//		printf("signal: mono\nimpulse: mono\n");
		float *outputBuffer;
		// Convolve
		int length = convolve(signal->buffer1, impulse->buffer1,
				signal->numFrames, impulse->numFrames, &outputBuffer);
		// For normalizing later on
		for (i = 0; i < signal->numFrames; i++) {
			if (fabs(signal->buffer1[i]) > max) {
				max = fabs(signal->buffer1[i]);
			}
		}
		// Normalize signal buffer, multiply by dry/wet coefficient
		for (i = 0; i < signal->numFrames; i++) {
			signal->buffer1[i] *= (1 - dry_wet) / max;
		}
		// Add dry signal to output buffer
		for (i = 0; i < signal->numFrames; i++) {
			outputBuffer[i] += signal->buffer1[i];
		}
		// Normalize output buffer again
		max = 0;
		for (i = 0; i < length; i++) {
			if (fabs(outputBuffer[i]) > max) {
				max = fabs(outputBuffer[i]);
			}
		}
		for (i = 0; i < length; i++) {
			outputBuffer[i] /= max;
		}
		// Write to wav file
		writeWavFile(outputBuffer, 44100, 1, length, 1, outFileName);
	}

	// If the signal is stereo but the impulse is mono
	if (signal->numChannels == 2 && impulse->numChannels == 1) {
		float max = 0;
//		printf("signal: stereo\nimpulse: mono\n");
		float *outLeft;
		float *outRight;
		// Create buffer for left channel of signal
		float *signal_left = (float *) malloc(
				sizeof(float) * signal->numFrames);
		// Create buffer for right channel of signal
		float *signal_right = (float *) malloc(
				sizeof(float) * signal->numFrames);
		// Copy signal data into buffers
		for (i = 0; i < signal->numFrames; i++) {
			signal_left[i] = signal->buffer1[2 * i];
			signal_right[i] = signal->buffer1[2 * i + 1];
			// For normalizing later on
			if (fabs(signal->buffer1[2 * i]) > max) {
				max = fabs(signal->buffer1[2 * i]);
			}
			if (fabs(signal->buffer1[2 * i + 1]) > max) {
				max = fabs(signal->buffer1[2 * i + 1]);
			}
		}
		// Convolve
		int length = convolve(signal_left, impulse->buffer1, signal->numFrames,
				impulse->numFrames, &outLeft);
		convolve(signal_right, impulse->buffer1, signal->numFrames,
				impulse->numFrames, &outRight);
		// Recombine left and right channels
		float *outputBuffer = (float *) malloc(sizeof(float) * length * 2);
		for (i = 0; i < length; i++) {
			outputBuffer[2 * i] = outLeft[i] * dry_wet;
			outputBuffer[2 * i + 1] = outRight[i] * dry_wet;
		}
		// Normalize signal buffer, multiply by dry/wet coefficient
		for (i = 0; i < signal->numFrames * 2; i++) {
			signal->buffer1[i] *= (1 - dry_wet) / max;
		}
		// Add dry signal to output buffer
		for (i = 0; i < signal->numFrames; i++) {
			outputBuffer[2 * i] += signal->buffer1[2 * i];
			outputBuffer[2 * i + 1] += signal->buffer1[2 * i + 1];
		}
		// Normalize output buffer again
		max = 0;
		for (i = 0; i < length * 2; i++) {
			if (fabs(outputBuffer[i]) > max) {
				max = fabs(outputBuffer[i]);
			}
		}
		for (i = 0; i < length * 2; i++) {
			outputBuffer[i] /= max;
		}
		// Write to wav file
		writeWavFile(outputBuffer, 44100, 2, length, 2, outFileName);
	}

	// If the signal is mono but the impulse is stereo
	if (signal->numChannels == 1 && impulse->numChannels == 2) {
		float max = 0;
//		printf("signal: mono\nimpulse: stereo\n");
		float *outLeft;
		float *outRight;
		// Create buffer for left channel of impulse
		float *impulse_left = (float *) malloc(
				sizeof(float) * impulse->numFrames);
		// Create buffer for right channel of impulse
		float *impulse_right = (float *) malloc(
				sizeof(float) * impulse->numFrames);
		// Copy impulse data into buffers
		for (i = 0; i < impulse->numFrames; i++) {
			impulse_left[i] = impulse->buffer1[2 * i];
			impulse_right[i] = impulse->buffer1[2 * i + 1];
		}
		// For normalizing later on
		for (i = 0; i < signal->numFrames; i++) {
			if (fabs(signal->buffer1[i]) > max) {
				max = fabs(signal->buffer1[i]);
			}
		}
		// Convolve
		int length = convolve(signal->buffer1, impulse_left, signal->numFrames,
				impulse->numFrames, &outLeft);
		convolve(signal->buffer1, impulse_right, signal->numFrames,
				impulse->numFrames, &outRight);
		// Recombine left and right channels
		float *outputBuffer = (float *) malloc(sizeof(float) * length * 2);
		for (i = 0; i < length; i++) {
			outputBuffer[2 * i] = outLeft[i] * dry_wet;
			outputBuffer[2 * i + 1] = outRight[i] * dry_wet;
		}
		// Normalize signal buffer, multiply by dry/wet coefficient
		for (i = 0; i < signal->numFrames; i++) {
			signal->buffer1[i] *= (1 - dry_wet) / max;
		}
		// Add dry signal to output buffer
		for (i = 0; i < signal->numFrames; i++) {
			outputBuffer[2 * i] += signal->buffer1[i];
			outputBuffer[2 * i + 1] += signal->buffer1[i];
		}
		// Normalize output buffer again
		max = 0;
		for (i = 0; i < length * 2; i++) {
			if (fabs(outputBuffer[i]) > max) {
				max = fabs(outputBuffer[i]);
			}
		}
		for (i = 0; i < length * 2; i++) {
			outputBuffer[i] /= max;
		}
		// Write to wav file
		writeWavFile(outputBuffer, 44100, 2, length, 2, outFileName);
	}

	// If the signal is stereo and the impulse is stereo
	if (signal->numChannels == 2 && impulse->numChannels == 2) {
		float max = 0;
//		printf("signal: stereo\nimpulse: stereo\n");
		float *outLeft;
		float *outRight;
		// Create buffer for left channel of signal
		float *signal_left = (float *) malloc(
				sizeof(float) * signal->numFrames);
		// Create buffer for right channel of signal
		float *signal_right = (float *) malloc(
				sizeof(float) * signal->numFrames);
		// Create buffer for left channel of impulse
		float *impulse_left = (float *) malloc(
				sizeof(float) * impulse->numFrames);
		// Create buffer for right channel of impulse
		float *impulse_right = (float *) malloc(
				sizeof(float) * impulse->numFrames);
		// Copy signal data into buffers
		for (i = 0; i < signal->numFrames; i++) {
			signal_left[i] = signal->buffer1[2 * i];
			signal_right[i] = signal->buffer1[2 * i + 1];
			// For normalizing later on
			if (fabs(signal->buffer1[2 * i]) > max) {
				max = fabs(signal->buffer1[2 * i]);
			}
			if (fabs(signal->buffer1[2 * i + 1]) > max) {
				max = fabs(signal->buffer1[2 * i + 1]);
			}
		}
		// Copy impulse data into buffers
		for (i = 0; i < impulse->numFrames; i++) {
			impulse_left[i] = impulse->buffer1[2 * i];
			impulse_right[i] = impulse->buffer1[2 * i + 1];
		}
		// Convolve
		int length = convolve(signal_left, impulse_left, signal->numFrames,
				impulse->numFrames, &outLeft);
		convolve(signal_right, impulse_right, signal->numFrames,
				impulse->numFrames, &outRight);
		// Recombine left and right channels
		float *outputBuffer = (float *) malloc(sizeof(float) * length * 2);
		for (i = 0; i < length; i++) {
			outputBuffer[2 * i] = outLeft[i] * dry_wet;
			outputBuffer[2 * i + 1] = outRight[i] * dry_wet;
		}
		// Normalize signal buffer, multiply by dry/wet coefficient
		for (i = 0; i < signal->numFrames * 2; i++) {
			signal->buffer1[i] *= (1 - dry_wet) / max;
		}
		// Add dry signal to output buffer
		for (i = 0; i < signal->numFrames; i++) {
			outputBuffer[2 * i] += signal->buffer1[2 * i];
			outputBuffer[2 * i + 1] += signal->buffer1[2 * i + 1];
		}
		// Normalize output buffer again
		max = 0;
		for (i = 0; i < length * 2; i++) {
			if (fabs(outputBuffer[i]) > max) {
				max = fabs(outputBuffer[i]);
			}
		}
		for (i = 0; i < length * 2; i++) {
			outputBuffer[i] /= max;
		}
		// Write to wav file
		writeWavFile(outputBuffer, 44100, 2, length, 2, outFileName);
	}
}

// This function performs time-domain multiplication (slow convolution)
// dry_wet is a measure of the ratio between the dry and wet signals. 0 is completely dry
// and 1 is completely wet
void slowConvolve(audioData *signal, audioData *impulse, float dry_wet,
		char *outFileName) {

	// Check for realistic dry_wet values
	if (dry_wet < 0 || dry_wet > 1) {
		printf("Error: dry_wet must be between 0 and 1\n");
		return;
	}

	int i, j, k;
	if (signal->numChannels == 1 && impulse->numChannels == 1) {
	}
	if (signal->numChannels == 2 && impulse->numChannels == 1) {
	}
	if (signal->numChannels == 1 && impulse->numChannels == 2) {
	}
	if (signal->numChannels == 2 && impulse->numChannels == 2) {
		// Find new length
		int newLength = signal->numFrames + impulse->numFrames - 1;
		// Create buffer for output
		float *outputBuffer = (float *) malloc(sizeof(float) * newLength * 2);
		// Zero out buffer
		memset(outputBuffer, 0, sizeof(float) * newLength * 2);
		// Perform convolution
		for (j = 0; j < signal->numFrames; j++) {
			for (k = 0; k < impulse->numFrames; k++) {
				outputBuffer[2 * (j + k)] += signal->buffer1[2 * j]
						* impulse->buffer1[2 * k];
				outputBuffer[2 * (j + k) + 1] += signal->buffer1[2 * j + 1]
						* impulse->buffer1[2 * k + 1];
			}
		}
		// Multiply by dry/wet coefficient
		for (i = 0; i < newLength * 2; i++) {
			outputBuffer[i] *= dry_wet;
		}
		// Add dry signal to output buffer, multiply by dry/wet coefficient
		for (i = 0; i < signal->numFrames * 2; i++) {
			outputBuffer[i] += signal->buffer1[i] * (1 - dry_wet);
		}
		// Normalize
		outputBuffer = normalizeBuffer(outputBuffer, newLength * 2);
		// Write to wav file
		writeWavFile(outputBuffer, 44100, 2, newLength, 2, outFileName);
	}

}

