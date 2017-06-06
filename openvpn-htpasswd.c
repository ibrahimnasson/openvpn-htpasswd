/*
 * Copyright (c) 2017 Brian P. Curran <brian@brianpcurran.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define CREDLENGTH 4096 /* Set maximum length for username and password */
#define LINELENGTH 4096 /* Set maximum length for line from htpasswd file */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void tmp_file(char *fn, char *un, char *pw) {
    FILE *fp;
    int i;
    ssize_t ll;
    char *lp = NULL;
    size_t ls = 0;
    fp = fopen(fn, "r");
	if (fp == NULL) {
        printf("Error reading from file %s: %s\n", fn, strerror(errno));
        exit(1);
    }
    i = 0;
    while ((ll = getline(&lp, &ls, fp)) != -1) {
        lp[strcspn(lp, "\n")] = '\0';
        if (i == 0) {
            strlcpy(un, lp, CREDLENGTH);
        } else if (i == 1) {
            strlcpy(pw, lp, CREDLENGTH);
        }
        ++i;
    }
	free(lp);
    if (i != 2) {
        printf("Too many or too few lines in temp file. Exiting.\n");
        exit(1);
    }
}

void htpasswd_file(char *un, char *hash) {
    char fn[] = "./var/openvpn/users.htpasswd";
    FILE *fp;
    ssize_t ll;
    char *lp = NULL;
    size_t ls = 0;
    char *un_ptr;
    char *hash_ptr;
    fp = fopen(fn, "r");
    if (fp == NULL) {
        printf("Error reading from file %s: %s\n", fn, strerror(errno));
        exit(1);
    }
    while ((ll = getline(&lp, &ls, fp)) != -1) {
        lp[strcspn(lp, "\n")] = '\0';
        un_ptr = strsep(&lp, ":");
        if (strcmp(un_ptr, un) == 0) {
            strlcpy(hash, lp, CREDLENGTH);
        }
    }
    free(lp);
}

int main(int argc, char *argv[]) {

    char username[CREDLENGTH];
    char password[CREDLENGTH];
    char hash[CREDLENGTH];

    /* Exit if there isn't exactly 1 argument */
    if(argc != 2) {
        printf("Too many or too few arguments. Exiting.\n");
        exit(1);
    }

    /* Set username and password to values from temp file created by OpenVPN */
    tmp_file(argv[1], username, password);

    /* Given username get the hash from htpasswd file */
    htpasswd_file(username, hash);

    /* Compare the password to the hash */
    if (crypt_checkpass(password, hash) == 0) {
        printf("Password is good!\n");
        exit(0);
    } else {
        printf("Password is bad :(.\n");
        exit(1);
    }

    return 0;
} 
