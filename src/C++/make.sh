#!/bin/bash
OUTPUT_PATH=../LabVIEW/G-Audio/lib

mkdir -p $OUTPUT_PATH
g++ -shared -fPIC -o g_audio_64.so *.cpp -lm -lpthread -ldl -O3
mv g_audio_64.so $OUTPUT_PATH
