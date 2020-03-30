/*
 * CLE_2.c
 *
 *  Created on: Mar 24, 2020
 *      Author: franc
 */


include <stdio.h>
#include <wchar.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct dirent dirent_t;

bool startswith (const char *, const char *);
bool endswith (const char *, const char *);
wint_t validchar(wint_t);

int main (int argc, char *argv[])
{
    char *foldername;
    if (argc > 1)
    {
        bool has_not_slash = !endswith(argv[1], "/");
        foldername = malloc(strlen(argv[1]) + ((size_t) has_not_slash) + 1);
        strcpy(foldername, argv[1]);
        if (has_not_slash)
            strcat(foldername, "/");
    }
    else
    {
        foldername = malloc(sizeof("./") + 1);
        strcpy(foldername, "./");
    }

    DIR *folder = opendir(foldername);
    char *locale = setlocale(LC_ALL, "");

    size_t wordlength = 0;
    size_t maxwordlength = 0;
    size_t vowels[5] = { 0 };
    size_t *wordlengths = (size_t *) malloc(maxwordlength * sizeof(size_t));

    dirent_t *dircontent;
    while ((dircontent = readdir(folder)) != NULL)
        if ((dircontent->d_type & DT_REG) && startswith(dircontent->d_name, "text") && endswith(dircontent->d_name, ".txt"))
        {
            const size_t pathlength = sizeof(foldername) + strlen(dircontent->d_name);
            char filepath[pathlength + 1];

            strcpy(filepath, foldername);
            strcat(filepath, dircontent->d_name);

            wint_t character;
            FILE *file = fopen(filepath, "r");

            // iterate over file characters
            while ((character = getwc(file)) != WEOF)
            {
                character = validchar(character);

                switch (character)
                {
                case L'a':
                case L'A':
                    ++vowels[0];
                    break;
                case L'e':
                case L'E':
                    ++vowels[1];
                    break;
                case L'i':
                case L'I':
                    ++vowels[2];
                    break;
                case L'o':
                case L'O':
                    ++vowels[3];
                    break;
                case L'u':
                case L'U':
                    ++vowels[4];
                    break;
                default:
                    break;
                }

                if (character >= L'A' && character <= L'Z' || character >= L'a' && character <= L'z')
                    ++wordlength;
                else if (wordlength)
                {
                    if (wordlength > maxwordlength)
                        wordlengths = (size_t *) realloc(wordlengths, (maxwordlength = wordlength) * sizeof(size_t));
                    ++wordlengths[wordlength - 1];
                    wordlength = 0;
                }
            }

            if (wordlength)
            {
                if (wordlength > maxwordlength)
                    wordlengths = (size_t *) realloc(wordlengths, (maxwordlength = wordlength) * sizeof(size_t));
                ++wordlengths[wordlength - 1];
                wordlength = 0;
            }

            fclose(file);
        }


    closedir(folder);

    printf("Número de ocurrências de '%c': %lu\n", 'a', vowels[0]);
    printf("Número de ocurrências de '%c': %lu\n", 'e', vowels[1]);
    printf("Número de ocurrências de '%c': %lu\n", 'i', vowels[2]);
    printf("Número de ocurrências de '%c': %lu\n", 'o', vowels[3]);
    printf("Número de ocurrências de '%c': %lu\n", 'u', vowels[4]);

    for (size_t i = 0; i < maxwordlength; ++i)
        printf("Número de ocurrências de palavras com %lu letras: %lu\n", i + 1, wordlengths[i]);

    free(wordlengths);

    return EXIT_SUCCESS;
}

bool startswith (const char *string, const char *prefix)
{
    return strncmp(string, prefix, strlen(prefix)) == 0;
}

bool endswith (const char *string, const char *suffix)
{
    const size_t suffix_length = strlen(suffix);
    const size_t string_length = strlen(string);
    return suffix_length > string_length ? false : strncmp(string + (string_length - suffix_length), suffix, strlen(suffix)) == 0;
}

wint_t validchar(wint_t character)
{
    switch (character)
    {
    case L'á':
    case L'à':
    case L'ã':
    case L'â':
        return L'a';
    case L'Á':
    case L'À':
    case L'Ã':
    case L'Â':
        return L'A';
    case L'é':
    case L'è':
    case L'ẽ':
    case L'ê':
        return L'e';
    case L'É':
    case L'È':
    case L'Ẽ':
    case L'Ê':
        return L'E';
    case L'í':
    case L'ì':
    case L'ĩ':
    case L'î':
        return L'i';
    case L'Í':
    case L'Ì':
    case L'Ĩ':
    case L'Î':
        return L'I';
    case L'ó':
    case L'ò':
    case L'õ':
    case L'ô':
        return L'o';
    case L'Ó':
    case L'Ò':
    case L'Õ':
    case L'Ô':
        return L'O';
    case L'ú':
    case L'ù':
    case L'ũ':
    case L'û':
        return L'u';
    case L'Ú':
    case L'Ù':
    case L'Ũ':
    case L'Û':
        return L'U';
    default:
        return character;
    }
}