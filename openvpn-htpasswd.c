#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main (int argc, char *argv[]) {

  // Create some variables
  int openvpn_file_counter;
  char htpasswd_path[] = "users.htpasswd";
  char *file_flag_ptr = NULL, *htpasswd_field_ptr = NULL, *line_token_ptr = NULL, *htpasswd_hash_ptr = NULL;
  char openvpn_username[100], openvpn_password[100], line[100];
  FILE *openvpn_file_ptr = NULL, *htpasswd_file_ptr = NULL;

  // Exit if there isn't exactly 1 argument
  if(argc != 2) {
    printf("Too many or too few arguments. Exiting.\n");
    exit(1);
  }

  // Load the temporary file from OpenVPN and get the username and password
  file_flag_ptr = argv[1];
  openvpn_file_ptr = fopen(file_flag_ptr, "r");
  if (openvpn_file_ptr == NULL) {
    printf("Error reading from file %s: %s\n", file_flag_ptr, strerror(errno));
    exit(1);
  }
  for (openvpn_file_counter=0; openvpn_file_counter<2; ++openvpn_file_counter) {
    if (openvpn_file_counter == 0) {
      fgets(openvpn_username, sizeof(openvpn_username), openvpn_file_ptr);
      openvpn_username[strcspn(openvpn_username, "\n")] = '\0';
    }
    if (openvpn_file_counter == 1) {
      fgets(openvpn_password, sizeof(openvpn_password), openvpn_file_ptr);
      openvpn_password[strcspn(openvpn_password, "\n")] = '\0';
    }
  }
  
  // Load the htpasswd file
  htpasswd_file_ptr = fopen(htpasswd_path, "r");
  if (htpasswd_file_ptr == NULL) {
    printf("Error reading from file %s: %s\n", htpasswd_path, strerror(errno));
    exit(1);
  }

  // Find the line in the htpasswd file that matches the username from OpenVPN
  while (fgets(line, sizeof(line), htpasswd_file_ptr) != NULL) {
    line[strcspn(line, "\n")] = '\0';
    line_token_ptr = line;
    htpasswd_field_ptr = strsep(&line_token_ptr, ":");
    // Get the hash for the matched username and exit the while loop
    if (strcmp(htpasswd_field_ptr, openvpn_username) == 0) {
      htpasswd_hash_ptr = strsep(&line_token_ptr, ":");
      break;
    }
  }

  // Compare the password from OpenVPN to the hash from the htpasswd file
  if (crypt_checkpass(openvpn_password, htpasswd_hash_ptr) == 0) {
    printf("Password is good!\n");
    exit(0);
  } else {
    printf("Password is bad :(.\n");
    exit(1);
  }

  return 0;
} 
