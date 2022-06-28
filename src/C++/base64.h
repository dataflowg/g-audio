/*
 * Written by Manuel Badzong. If you have suggestions or improvements, let me
 * know.
 */
#ifndef _BASE64_H_
#define _BASE64_H_

int base64_encode(char *dest, int size, unsigned char *src, int slen);
char *base64_enc_malloc(unsigned char *src, int slen);
int base64_decode(unsigned char *dest, int size, char *src);
unsigned char *base64_dec_malloc(char *src, int32_t* dec_size);

#endif
