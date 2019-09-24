# TTGO Mp3 Player 
This is a stripped down version of the TTGO MP3 Player app (https://github.com/LilyGO/TTGO-TAudio) to work only with mp3, no bluetooth or wifi connection. 

## SD Card 

The mp3 files on the SD card have to be numbered  and named xxx.mp3 (001.mp3, 002.mp3 and so on). Structure on the SD Card: 

index.txt 

001.mp3

002.mp3

003.mp3

...

xxx.mp3


The first line of the index file must contain the number of mp3 files on the sd card. It will be read first and later used for picking a random number between 0 and num_files.

