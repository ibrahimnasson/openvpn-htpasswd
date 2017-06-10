openvpn-htpasswd
================

A small C program to use with OpenVPN's auth-user-pass-verify configuration option and an htpasswd file. It will probably only work on OpenBSD 5.9 and newer because it uses the [crypt_checkpass(3)](http://man.openbsd.org/crypt_checkpass.3) and [pledge(2)](http://man.openbsd.org/pledge.2) functions.

tmp-file.example is an example of the temporary file that OpenVPN should provide as the first argument to the program if you use the "via-file" method. There is more detail in the [OpenVPN 2.4 man page](https://community.openvpn.net/openvpn/wiki/Openvpn24ManPage).

users.htpasswd is an example of an htpasswd file that can be used for authentcation. There are three example username:password pairs in it for testing: user:test, user2:test2, and user3:test3. This file was generated with [htpasswd(1)](http://man.openbsd.org/htpasswd) on OpenBSD 6.1.

Disclaimer
==========

This is the first C program I've written since 2005. It is probably insecure and full of bugs. It is also being actively developed, so please don't use it for anything important.

Compiling and using
===================

Compile an executable with ```gcc -static -o openvpn-htpasswd openvpn-htpasswd.c```. Copy the executable to ```/var/openvpn/openvpn-htpasswd``` (relative to the chroot, in my case ```/var/openvpn/chrootjail/var/openvpn/openvpn-htpasswd```) on your OpenVPN server. Generate an htpasswd file with ```htpasswd /var/openvpn/chrootjail/var/openvpn/users.htpasswd <username>``` where ```<username>``` is the initial username you'd like to authorize to connect. Add the following lines to your OpenVPN server configuration:

```
script-security 2
auth-user-pass-verify "/var/openvpn/openvpn-htpasswd" via-file
```

Restart the OpenVPN daemon. Add the following line to your OpenVPN client configuration:

```
auth-user-pass
```

You should now be able to connect from a client and authenticate with the username and password you provided to the ```htpasswd``` command.

To test the program without making any OpenVPN configuration changes, you can run ```openvpn-htpasswd tmp-file.example```. With a copy of tmp-file.example in the current directory and users.htpasswd in ```./var/openvpn/```, this should indicate that password authentication was successful. To test failure, edit tmp-file.example and provide a username and password that do not match any of the example pairs in users.htpasswd.

Resources
=========

The [Setting up OpenVPN (free community version) on OpenBSD](http://www.openbsdsupport.org/openvpn-on-openbsd.html) guide is super helpful re: OpenVPN on OpenBSD.
