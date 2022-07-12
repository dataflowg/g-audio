#!/bin/bash
OUTPUT_PATH=/usr/lib

g++ -shared -fPIC -o g_audio_32.so *.cpp -lm -lpthread -ldl -std=c++11 -mfpu=neon -mfloat-abi=softfp -O3
mv g_audio_32.so $OUTPUT_PATH
