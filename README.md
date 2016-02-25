# Synthesized Convolution Reverb

This application synthesizes room impulses for use in convolution reverb.
It works according to the following procedure:

* Read an audio file containing a recorded impulse.

* Extract from the impulse frequency data over time for each frequency bin.

* Display the frequency data on a graph that the user can see.

* Provide the user with the ability to adjust each bin separately or together on the graph.

* Provide the user to change other parameters, such as impulse length.

* Allow the user to re-synthesize the impulse based on changes they have made.

* Provide continuous real-time convolution of incoming audio with the impulse so the user can hear the result of the 
changes they have made.

### Current project state

##### Impulse types

The project currently works for mono and stereo impulses. Audio I/O is performed using the libsndfile library, so all
file formats supported by [libsndfile](http://www.mega-nerd.com/libsndfile/) should in theory work for this application.

##### User Interface

###### Frequency Graph

The user interface displays a graph of the current impulse's frequency content. The x-axis represents ln(frequency), and
the y-axis represents magnitude. The graph is interactive and the user can adjust any of the frequencies by clicking and dragging
them up or down.

When the mouse is in the graph area, the frequency bins that will be affected on a click are highlighted. Furthermore, the
center frequency bin is highlighted most strongly, and the frequency associated with that bin is displayed next to the pointer
on the screen.

###### Buttons

* Recompute Impulse
 * This button synthesizes a new impulse based on the current parameters in the model.

* Randomize
 * This button randomizes the magnitude each frequency bin, within a range.
 * Currently, the range of randomization is specified, but future work will include giving the user control over this parameter.

###### Sliders

* Smoothness

 * This slider controls how many frequency bins can be edited at once. When set to 1, each frequency bin can be edited individually.
 When set to _n_, clicking on a frequency bin will also change the values of the closest _n - 1_ frequency bins on either side.
 * This control can be useful because the frequency bins in the upper range of the frequency spectrum are drawn very close together and are
 difficult to target individually.
 * Additional improvements may include adding an 'auto-smoothness' control that automatically increases the smoothness amount in proportion
 with how close together the frequency bins are drawn.
 
* Interpolation

 * This slider changes the visual appearance of the impulse. When set to _n_, it draws _n-1_ additional frequency bins on the graph between
 each two existing bins.
 * Note that this is purely a visual enhancement and does not affect the digital signal processing in any way. The number of frequency bins
 is determined by the FFT size used by the synthesis engine, and this parameter is not affected by this slider.
 * Also note that drawing these extra bins does take more CPU power and may result in laggy performance if the interpolation amount is too high.

* Length

 * This slider changes the length of the impulse. When dragging the slider, the user can see the new impulse length, in seconds, displayed next to
 the slider. Upon release of the mouse, the new impulse will automatically be synthesized, using the new length and any other parameters that may
 have been changed.
 * Currently, setting the impulse length to a low amount (less than 1 second) will sometimes cause the program to crash. I have not identified
 the exact source of the problem, but I believe it stems from memory mismanagement due to the faulty multithreading scheme that the software uses.
 
* Channel

 * This slider changes the channel from which frequency data will be displayed on the screen.
 * In the future, this value should be probably controlled by a set of radio buttons or a dropdown menu instead of a slider.
 * Additionally, for stereo impulses there should be a way to view and edit both channels at once, and this feature is on the current list of improvements.
 
### Current issues/bugs
 
Currently, there are some clicks that occur during the course of running the application. I am not sure exactly what is causing the clicks, but
it might have to do with the multithreading problems mentioned earlier. The clicks are soft enough that the user can still easily hear the intended
result of the convolution reverb, but loud enough that the application in its current state would be unsuitable for any real-world application.

Sometimes, synthesizing a new impulse will result in absurdly high output values. To handle this until it is fixed, I have written code to detect
these glitches and mute the output until they are gone. Often, simply resynthesizing the same impulse again fixes the problem, so in these
situations the program automatically tries to resynthesize the impulse if this happens. Again, it is probably the multithreading that is the source
of this problem.

### Future improvements

###### Fixing the multithreading

Obviously, this application in its current state misuses multithreading technology, and memory corruption occurs as a result.
I am working on learning more about multithreaded applications so I can make the software thread-safe while still ensuring that the
main audio callback function is not impeded by any of the other threads performing convolution.

###### Fixing the overall structure of the program

Given the scope of this project, the program should really be a C++ application, not a C application. Furthermore, the code is absolutely atrocious, 
and quite frankly is an insult to everyone who worked to develop the concept of object oriented programming. Excerpts from this application could quite
readily be pasted into a book on how not to write code.

However, I plan to redesign the entire application and rewrite it in C++, obeying OOP principles like "don't repeat yourself" and "use the lowest level
of scope possible" and other best practices. I believe this will make it much easier to identify bugs in the code, and it will make the entire
application easier to read and understand, both for me and for anyone else that works on it.

###### Fixing the clicks, the bursts of noise, and other glitches

There is no reason why this program has to allow glitches in the audio. Plenty of convolution reverb plugins already exist and none of them have that
problem, so it's just a matter of identifying the source of glitches and correcting them. This might take additional time and knowledge, but eventually
I will fix these problems.

###### Developing the application into a plugin (AU, VST, RTAS, etc)

The software is currently a standalone application, but it would be much more useful to many people if it were also available as a plugin that could be
hosted in a digital audio workstation environment such as Logic Pro, Ableton, Pro Tools, Cubase, FL Studio, etc. This would involve writing a new user
interface (although some current elements from OpenGL could probably be re-used), but the overall digital signal processing at the heart of the application
would be very similar.

###### Add intuitive controls

One of my main goals when creating this software was to provide users with a convolution reverb application that was more intuitive than most existing software.
Resynthesis of the impulse gives the user precise control of its frequency content, and this fact could be used to provide the user with controls that
change the impulse in intuitive ways. Some possible ideas are listed below:

* Room modes

The term 'Room modes' refers to the the natural resonances that a room creates when a sound is propagated through it. It would be relatively simple to
provide the user with the ability to set values for room width, height, and depth, as well as microphone location, and then calculate the frequencies that 
would be boosted and cut as a result of the mic's placement in the room. To make it more intuitive, these parameters could be set by allowing the
user to interact with a graph.

* Descriptive EQ

Many reverb controls have EQ parameters such as 'high-shelf', 'low-shelf', and 'mid frequencies'. To make this more intuitive, I plan on having various sliders
with common frequency-specific descriptors such as 'brightness', 'presence', '

###### Other (less drastic) improvements

* Add a file system navigator so users can load their own impulses from the user interface.
* Add the ability for users to save and reload presets.
* Add the ability to view the frequency graph as a line graph rather than a bar graph. This would make it easier to display both channels on the screen
at the same time, with the current channel highlighted and the other faded slightly into the background.
* Improve the 'randomize' algorithm, and maybe provide several randomize options:
 * Total randomness - new random values which are not correlated at all with the current impulse.
 * Correlated randomness - new random values which are chosen based on deviations from the current impulse values.
 * Smoothness of random values - a way to control the continuity between adjacent random values.
* Add the ability to choose the overlap point between the synthesized tail and the recorded beginning of the impulse.
* Add the ability to change the beginning of the impulse without affecting the synthesized tail.

