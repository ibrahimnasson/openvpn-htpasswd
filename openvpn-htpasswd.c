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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Read two lines from file fn. Store the first in un and the second in pw.
*/
void tmp_file(char *fn, char *un, char *pw) {
    FILE *fp; /* Pointer to OpenVPN temporary file */
    int i; /* Counter for while loop */
    ssize_t ll; /* Line length returned by getline() */
    char *lp = NULL; /* For getline() to store stream data */
    size_t ls = 0; /* The size of the memory allocated by getline */
    char buf[MAX_LEN]; /* A buffer to copy the lines into */
    fp = fopen(fn, "r");
    if (fp == NULL) {
        err(1, "Error reading from file %s", fn);
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
            err(1, "Got an empty line from temp file.");
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
        explicit_bzero(un, MAX_LEN);
        explicit_bzero(pw, MAX_LEN);
        explicit_bzero(buf, MAX_LEN);
        err(1, "Too many or too few lines in temp file.");
    }
}

/*
 * Read lines from file fn until un is found and store associated hash.
*/
int htpasswd_file(char *un, char *hash) {
    char fn[] = "./var/openvpn/users.htpasswd"; /* Path to the htpasswd file */
    FILE *fp; /* Pointer to htpasswd file */
    ssize_t ll; /* Line length returned by getline() */
    char *lp = NULL; /* For getline() to store stream data */
    char buf[MAX_LEN]; /* Buffer to copy lines into */
    char *buf_ptr = NULL; /* Pointer to use with strsep */
    size_t ls = 0; /* The size of the memory allocated by getline() */
    char *un_ptr = NULL; /* Pointer to use for the first field from htpasswd */
    fp = fopen(fn, "r");
    if (fp == NULL) {
        err(1, "Error reading from file %s", fn);
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
    return 0;
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
    /*
     * Use pledge() because why not. We can probably add some paths for the
     * second argument here, but I don't know what they are yet.
    */
    if (pledge("stdio rpath", NULL) == -1) {
        err(1, "pledge");
    }
    if (argc != 2) {
        err(1, "Too many or too few arguments.");
    }
    /*
     * tmp_file() opens the file at the path in argv[1] and populates username
     * and password from the first and second lines, respectively.
    */
    tmp_file(argv[1], username, password);
    /*
     * htpasswd_file() finds the line matching username in the htpasswd file
     * and populates hash with the hash from that line. If it has a problem
     * reading the file (doesn't return 0), zero out sensitive data.
    */
    if (htpasswd_file(username, hash) != 0) {
        explicit_bzero(username, sizeof(username));
        explicit_bzero(password, sizeof(password));
        explicit_bzero(hash, sizeof(hash));
        return 1;
    }
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
