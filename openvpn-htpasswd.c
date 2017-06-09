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
    while ((ll = getline(&lp, &ls, fp)) != -1) {
        /*
         * Since we have to free() the memory used by lp later, we should be
         * careful about potentially modifying it. If it's modified and then
         * we try to free() it, we'll get a "modified chunk-pointer" error.
         * None of the operations performed in this function should modify it,
         * but copying it into buf before doing anything with it seems like a
         * good idea anyway.
         *
         * Using strlcpy safely copies at most MAX_LEN-1 characters into buf.
         * If either line in the file is more than MAX_LEN-1 characters it is
         * silently truncated. We could check the return value of strlcpy()
         * to provide a friendlier and more accurate error in this case.
        */
        strlcpy(buf, lp, sizeof(buf));
        if (strcspn(buf, "\n") == 0) {
            printf("Got an empty line from temp file. Exiting.\n");
            /*
             * getline() uses realloc() to dynamically resize the memory used by
             * lp. freezero() only exists in OpenBSD-current right now so we'll
             * use explicit_bzero() and free() on lp to make sure associated
             * memory is securely zeroed out.
            */
            explicit_bzero(un, MAX_LEN);
            explicit_bzero(pw, MAX_LEN);
            explicit_bzero(buf, MAX_LEN);
            explicit_bzero(lp, strlen(lp));
            free(lp);
            exit(EXIT_FAILURE);
        }
        /*
         * This method of removing the line break character from a string is
         * from the fgets() and strcspn() man pages. strcscpn() returns the
         * index of the array where "\n" occurs, and assigns the NUL character
         * to it.
        */
        buf[strcspn(buf, "\n")] = '\0';
        if (i == 0) {
            strlcpy(un, buf, MAX_LEN);
        } else if (i == 1) {
            strlcpy(pw, buf, MAX_LEN);
        }
        ++i;
    }
    /*
     * Now that we've read all lines from the file, zero out memory that's
     * been allocated for lp, and free() it.
    */
    explicit_bzero(lp, strlen(lp));
    free(lp);
    /*
     * There shouldn't be more or fewer than two lines in the file, so if our
     * counter is isn't exactly 2, zero out un, pw, and buf, and exit.
    */
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
        /*
         * Unlike tmp_file(), this function uses strsep which modifies its
         * first argument, in this case &buf_ptr, so it's important to copy
         * lp into buf here.
         * 
         * This function will also (like tmp_file()) silently truncate a line
         * from the htpasswd file into MAX_LEN-1 characters.
        */
        strlcpy(buf, lp, sizeof(buf));
        buf[strcspn(buf, "\n")] = '\0';
        /*
         * strsep() replaces the first instance of the delimiter character (in
         * this case ":") with the NUL character '\0', updates its first
         * argument buf_ptr to point to the first character *after* the
         * delimiter, and returns the original value of buf_ptr.
         *
         * Because it works on pointers, we can't pass buf to it directly.
         *
         * un_ptr ends up pointing to the first character of the character
         * array buf and buf_ptr ends up pointing to the first character
         * after the first ":" in buf (the first character of the hash).
        */
        buf_ptr = buf;
        un_ptr = strsep(&buf_ptr, ":");
        /*
         * If the NUL-terminated strings pointed to by un_ptr and un match,
         * we've found a matching line in the htpasswd file, copy the hash
         * (pointed to by buf_ptr) into hash, and break out of the while loop.
        */
        if (strcmp(un_ptr, un) == 0) {
            strlcpy(hash, buf_ptr, MAX_LEN);
            break;
        }
    }
    explicit_bzero(lp, strlen(lp));
    free(lp);
}

int main(int argc, char *argv[]) {
    /*
     * Declare fixed-size character arrays to store a username and password
     * from the temporary file provided by OpenVPN, and a hash from the
     * htpasswd file.
    */
    char username[MAX_LEN];
    char password[MAX_LEN];
    char hash[MAX_LEN];
    if(argc != 2) {
        printf("Too many or too few arguments. Exiting.\n");
        return 1;
    }
    /*
     * tmp_file() opens the file at the path in argv[1] and populates username
     * and password from the first and second lines, respectively.
    */
    tmp_file(argv[1], username, password);
    /*
     * htpasswd_file() finds the line matching username in the htpasswd file
     * and populates hash with the hash from that line.
    */
    htpasswd_file(username, hash);
    /*
     * If the password from the temporary file matches the hash from the
     * htpasswd file, return 0 to indicate authentication success.
    */
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
