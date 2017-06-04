openvpn-htpasswd
================

A small C program to use with OpenVPN's auth-user-pass-verify configuration option and an htpasswd file. It will probably only work on OpenBSD because it uses the [crypt_checkpass(3)](http://man.openbsd.org/OpenBSD-6.1/crypt_checkpass) function.

tmp-file.example is an example of the temporary file that OpenVPN should provide as the first argument to the program if you use the "via-file" method. There is more detail in the [OpenVPN 2.4 man page](https://community.openvpn.net/openvpn/wiki/Openvpn24ManPage).

users.htpasswd is an example of an htpasswd file that can be used for authentcation. There are three example user:password pairs in it for testing: user:test, user2:test2, and user3:test3. This file was generated with [htpasswd(1)](http://man.openbsd.org/OpenBSD-6.1/htpasswd) on OpenBSD 6.1.

Compiling and using
===================

Compile an executable with ```gcc -static -o openvpn-htpasswd openvpn-htpasswd.c```. Copy the executable to ```/var/openvpn/openvpn-htpasswd``` (relative to the chroot) on your OpenVPN server. Generate an htpasswd file with ```htpasswd /var/openvpn/users.htpasswd <username>``` where ```<username>``` is the initial username you'd like to authorize to connect. Add the following lines to your OpenVPN server configuration:

```
script-security 2
auth-user-pass-verify "/var/openvpn/openvpn-htpasswd" via-file
```

Restart the OpenVPN daemon. Add the following line to your OpenVPN client configuration:

```
auth-user-pass
```

You should now be able to connect from a client and authenticate with the username and password you provided to the ```htpasswd``` command.
