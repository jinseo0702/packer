#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	int key = atoi(argv[1]);
	char *data = argv[2];

	for (int i = 0; data[i] != '\0'; i++){
		data[i] = data[i] ^ key;
	}
	printf("Encrypted data: %s\n", data);

	for (int i = 0; data[i] != '\0'; i++){
		data[i] = data[i] ^ key;
	}
	printf("Decrypted data: %s\n", data);
	return (0);
}