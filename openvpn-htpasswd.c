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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void process_tmp_file(char *tf, char *un, char *pw) {
    FILE *fp;
    int i = 0;
    fp = fopen(tf, "r");
    if (fp == NULL) {
        printf("Error reading from file %s: %s\n", tf, strerror(errno));
        exit(1);
    }
    for (i; i<2; i++) {
        if (i == 0) {
            fgets(un, sizeof(un), fp);
            un[strcspn(un, "\n")] = '\0';
        }
        if (i == 1) {
            fgets(pw, sizeof(pw), fp);
            pw[strcspn(pw, "\n")] = '\0';
        }
    }
}

int main(int argc, char *argv[]) {

    char username[100];
    char password[100];
    FILE *htpasswd_ptr = NULL; /* htpasswd file */
    char htpasswd_path[] = "./var/openvpn/users.htpasswd"; /* Path to htpasswd file relative to OpenVPN daemon */
    char line[100]; /* Line from htpasswd file */
    char *line_token_ptr = NULL; /* Token to use with fgets() */
    char *htpasswd_username_ptr = NULL; /* First field from htpasswd file */
    char *htpasswd_hash_ptr = NULL; /* Second field from htpasswd file */

    /* Exit if there isn't exactly 1 argument */
    if(argc != 2) {
        printf("Too many or too few arguments. Exiting.\n");
        exit(1);
    }

    /* Load the temporary file from OpenVPN and get the username and password */
    process_tmp_file(argv[1], username, password);
    printf("username is %s\n", username);
    printf("password is %s\n", password);
    
    /* Load the htpasswd file */
    htpasswd_ptr = fopen(htpasswd_path, "r");
    if (htpasswd_ptr == NULL) {
        printf("Error reading from file %s: %s\n", htpasswd_path, strerror(errno));
        exit(1);
    }

    /* Find the line in the htpasswd file that matches the username from OpenVPN */
    while (fgets(line, sizeof(line), htpasswd_ptr) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        line_token_ptr = line;
        htpasswd_username_ptr = strsep(&line_token_ptr, ":");
        /* Get the hash for the matched username and exit the while loop */
        if (strcmp(htpasswd_username_ptr, username) == 0) {
            htpasswd_hash_ptr = strsep(&line_token_ptr, ":");
            break;
        }
    }

    /* Compare the password from OpenVPN to the hash from the htpasswd file */
    if (crypt_checkpass(password, htpasswd_hash_ptr) == 0) {
        printf("Password is good!\n");
        exit(0);
    } else {
        printf("Password is bad :(.\n");
        exit(1);
    }

    return 0;
} 
