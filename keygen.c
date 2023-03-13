//#########################################################
// Author: Devon Miller
// Date: 3/6/23
// Course: CS_344
// Assignment: OTP
//#########################################################

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// valid chars to generate capital A- Z and space
const char valid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWQYZ ";

int is_valid(const char *str, int len);

int main(int argc, char const *argv[]) {
  // set random seed
	srand(time(NULL));

	if (argc != 2) {        // check for correct number of arguments
		fprintf(stderr, "Invalid. Program takes one argument eg: keygen <length to generate> \n");
		exit(1);
	}
	else if (is_valid(argv[1], strlen(argv[1])) == -1) {   // check input is only integers
		fprintf(stderr, "Invalid. Second argument must only be integers 0-9 \n");
		exit(1);
	}

	int input_num = atoi(argv[1]);
	for (int i = 0; i < input_num; ++i) {
		int k = rand() % 27;        // random index must be 0-26
		printf("%c", valid_chars[k]);
	}
	printf("\n");
}

// checks input is only integers
int is_valid(const char *str, int len) {
	for (int i = 0; i < len; ++i)
		if (str[i] > 57 || str[i] < 48)  // 48 = 0, 57 = 9
			return -1;
	return 0;
}
