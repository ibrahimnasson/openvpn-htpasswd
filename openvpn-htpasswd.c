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

#define MAX_LEN 4096

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Read two lines from file fn. Store the first in un and the second in pw.
*/
void tmp_file(char *fn, char *un, char *pw) {
    FILE *fp;
    int i;
    ssize_t ll;
    char *lp = NULL;
    size_t ls = 0;
    char buf[MAX_LEN];
    fp = fopen(fn, "r");
    if (fp == NULL) {
        printf("Error reading from file %s: %s\n", fn, strerror(errno));
        exit(EXIT_FAILURE);
    }
    i = 0;
    /*
     * getline() uses realloc() to dynamically resize the memory used by lp.
     * freezero() only exists in OpenBSD-current right now so we can use
     * explicit_bzero() and free() on lp to make sure associated memory is
     * securely zeroed out.
    */
    while ((ll = getline(&lp, &ls, fp)) != -1) {
        strlcpy(buf, lp, sizeof(buf));
        if (strcspn(buf, "\n") == 0) {
            printf("Got an empty line from temp file. Exiting.\n");
            explicit_bzero(un, MAX_LEN);
            explicit_bzero(pw, MAX_LEN);
            explicit_bzero(buf, MAX_LEN);
            explicit_bzero(lp, strlen(lp));
            free(lp);
            exit(EXIT_FAILURE);
        }
        buf[strcspn(buf, "\n")] = '\0';
        if (i == 0) {
            strlcpy(un, buf, MAX_LEN);
        } else if (i == 1) {
            strlcpy(pw, buf, MAX_LEN);
        }
        ++i;
    }
    explicit_bzero(lp, strlen(lp));
    free(lp);
    if (i != 2) {
        printf("Too many or too few lines in temp file. Exiting.\n");
        explicit_bzero(un, MAX_LEN);
        explicit_bzero(pw, MAX_LEN);
        explicit_bzero(buf, MAX_LEN);
        exit(EXIT_FAILURE);
    }
}

/*
 * Read lines from file fn until un is found and store associated hash.
*/
void htpasswd_file(char *un, char *hash) {
    char fn[] = "./var/openvpn/users.htpasswd";
    FILE *fp;
    ssize_t ll;
    char *lp = NULL;
    char buf[MAX_LEN];
    char *buf_ptr = NULL;
    size_t ls = 0;
    char *un_ptr = NULL;
    fp = fopen(fn, "r");
    if (fp == NULL) {
        printf("Error reading from file %s: %s\n", fn, strerror(errno));
        /*
         * Should probably return something and go back to main() here so
         * we can explicit_bzero() sensitive stuff read from the temp file.
        */
        exit(EXIT_FAILURE);
    }
    while ((ll = getline(&lp, &ls, fp)) != -1) {
        strlcpy(buf, lp, sizeof(buf));
        buf[strcspn(buf, "\n")] = '\0';
        buf_ptr = buf;
        un_ptr = strsep(&buf_ptr, ":");
        if (strcmp(un_ptr, un) == 0) {
            strlcpy(hash, buf_ptr, MAX_LEN);
            break;
        }
    }
    explicit_bzero(lp, strlen(lp));
    free(lp);
}

int main(int argc, char *argv[]) {
    char username[MAX_LEN];
    char password[MAX_LEN];
    char hash[MAX_LEN];
    if(argc != 2) {
        printf("Too many or too few arguments. Exiting.\n");
        return 1;
    }
    tmp_file(argv[1], username, password);
    htpasswd_file(username, hash);
    if (crypt_checkpass(password, hash) == 0) {
        explicit_bzero(username, sizeof(username));
        explicit_bzero(password, sizeof(password));
        explicit_bzero(hash, sizeof(hash));
        printf("Password is good!\n");
        return 0;
    } else {
        explicit_bzero(username, sizeof(username));
        explicit_bzero(password, sizeof(password));
        explicit_bzero(hash, sizeof(hash));
        printf("Password is bad :(.\n");
        return 1;
    }
} 
