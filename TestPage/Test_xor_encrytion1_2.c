#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//this version is encrypt string to 8bytes

int main(int argc, char *argv[]){
	unsigned long long key = strtoll(argv[1], NULL, 16);
	char *data = argv[2];
	int len = strlen(data);
	int Quotient = (len >> 3);
	int Remainder = (len & 7);

	printf("Original %s Quotient: %d, Remainder: %d\n", data, Quotient, Remainder);
	unsigned long long *ptr = (unsigned long long *)data;
	for (int i = 0; i < Quotient; i++){
		ptr[i] = ptr[i] ^ key;
	}
	for (int i = len - Remainder; i < len; i++){
		data[i] = data[i] ^ (unsigned char)(key & 0xFF);
	}
	printf("Encrypted data: %s\n", data);

	ptr = (unsigned long long *)data;
	for (int i = 0; i < Quotient; i++){
		ptr[i] = ptr[i] ^ key;
	}
	for (int i = len - Remainder; i < len; i++){
		data[i] = data[i] ^ (unsigned char)(key & 0xFF);
	}
	printf("Decrypted data: %s\n", data);
	return (0);
}