openvpn-htpasswd
================

A small C program to use with OpenVPN's auth-user-pass-verify configuration option. It will probably only work on OpenBSD because it uses the [crypt_checkpass(3)] [1] function.

openvpn-tmp-file.example is an example of the temporary file that OpenVPN should provide as the first argument to the program if you use the "via-file" method. There is more detail in the [OpenVPN 2.4 man page] [2].

users.htpasswd is an example of an htpasswd file that can be used for authentcation. There are three example user:password pairs in it for testing: user:test, user2:test2, and user3:test3. This file was generated with [htpasswd(1)] [3] on OpenBSD 6.1.

  [1]: http://man.openbsd.org/OpenBSD-6.1/crypt_checkpass
  [2]: https://community.openvpn.net/openvpn/wiki/Openvpn24ManPage
  [3]: http://man.openbsd.org/OpenBSD-6.1/htpasswd
