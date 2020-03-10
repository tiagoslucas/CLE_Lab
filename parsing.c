#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char **argv)
{
	char cha;
    int size;
    char *input = argv[1];
    FILE *input_file = fopen(input, "r");
    char separation[16] = { (char)0x20, (char)0x9, (char)0xA, '-', '"', '(', ')', '[', ']', '.', ':', ';', '?', '!', (char)0x93, (char)0xA6 };
    char merge[3] = { (char)0x27, (char)0x98, (char)0x99 };
    int found = 0;
    int sizes[50] = {0};
    int counters[27] = {0};

    while ((cha = getc(input_file)) != EOF) {
        // 0xc3 - 2 bytes ..... 0xe2 - 3 bytes
    	size = 0;
    	if( cha == (char)0xC3 ) {
    		size = 1;
    	} else if( cha == (char)0xE2 ) {
    		size = 2;
    	}

        if (size == 1) {
            if((cha = getc(input_file)) == EOF)
                exit(EXIT_FAILURE);
        } else if (size == 2) {
            if((cha = getc(input_file)) == EOF)
                exit(EXIT_FAILURE);
            if((cha = getc(input_file)) == EOF)
                exit(EXIT_FAILURE);
        }

    	for (int x = 0; x < 16; x++) {
    		if( (char)cha == separation[x] ) {
    			sizes[size]++;
    			size = 0;
    			found = 1;
    		}
    	}

    	for (int x = 0; x < 3; x++) {
    		if( (char)cha == merge[x] ) {
    			size++;
    			found = 1;
    		}
    	}


        if ( cha >= 65 && cha <= 90) {
            counters[cha - 64]++;
            found = 1;
        } else if ( cha >= 97 && cha <= 122 ){
            counters[cha - 96]++;
            found = 1;
        } else if (cha == (char)0xA1 || cha == (char)0xA0 || cha == (char)0xA2 || cha == (char)0xA3 || cha == (char)0x81 || cha == (char)0x80 || cha == (char)0x82 || cha == (char)0x83) {
        	counters[1]++;
        	found = 1;
        } else if (cha == (char)0xA9 || cha == (char)0xA8 || cha == (char)0xAA || cha == (char)0x89 || cha == (char)0x88 || cha == (char)0x8A) {
        	counters[5]++;
        	found = 1;
        } else if (cha == (char)0xAD || cha == (char)0xAC || cha == (char)0x8D || cha == (char)0x8C) {
        	counters[9]++;
        	found = 1;
        } else if (cha == (char)0xB3 || cha == (char)0xB2 || cha == (char)0xB4 || cha == (char)0xB5 || cha == (char)0x93 || cha == (char)0x92 || cha == (char)0x94 || cha == (char)0x95) {
        	counters[15]++;
        	found = 1;
        } else if (cha == (char)0xBA || cha == (char)0xB9 || cha == (char)0x94 || cha == (char)0x99) {
        	counters[21]++;
        	found = 1;
        } else if (cha == (char)0xA7 || cha == (char)0x87) {
            counters[3]++;
            found = 1;
        }

        if ( found == 0 ) {
        	size++;
        } else {
            found = 0;
        }
    }

    fclose(input_file);
    printf("Vowels: %d %d %d %d %d\n", counters[1], counters[5], counters[9], counters[15], counters[21]);
    return 0;
}
