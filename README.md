audio_playback_player

-----

This code is mainly copied from video tutorial about how to deal with audio decoding with FFmpeg and MiniAudio, it can only play audio

https://www.youtube.com/watch?v=-jugPJ_O8iM&t=1127s

Shout out to Bayo Code!

The code is developed and can be run on MacOS Sonoma, not tested for other platforms, theoretically crossplatform compatible

The sample music used is from

https://pixabay.com/music/beats-sad-soul-chasing-a-feeling-185750/

I'm planing to futher develop and mainly for understand the structure for the video player

About how to use:

in the root project folder
'''
mkdir build
cd build
cmake ..
make
./audio_playback_player
'''

if fail, see if you installed the dependencies
'''
brew install ffmpeg
(and potientially more)
'''
