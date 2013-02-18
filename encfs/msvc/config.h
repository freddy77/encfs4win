/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Building static library */
/* #undef BUILD_STATIC */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define to 1 if you have the <attr/xattr.h> header file. */
/* #undef HAVE_ATTR_XATTR_H */

/* define if the Boost library is available */
#define HAVE_BOOST /**/

/* define if the Boost::Filesystem library is available */
#define HAVE_BOOST_FILESYSTEM /**/

/* define if the Boost::Serialization library is available */
#define HAVE_BOOST_SERIALIZATION /**/

/* define if the Boost::System library is available */
#define HAVE_BOOST_SYSTEM /**/

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
/* #undef HAVE_DCGETTEXT */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Have EVP AES interfaces */
#define HAVE_EVP_AES 1

/* Define to 1 if you have the `EVP_aes_128_cbc' function. */
#define HAVE_EVP_AES_128_CBC 1

/* Define to 1 if you have the `EVP_aes_192_cbc' function. */
#define HAVE_EVP_AES_192_CBC 1

/* Define to 1 if you have the `EVP_aes_256_cbc' function. */
#define HAVE_EVP_AES_256_CBC 1

/* Have EVP Blowfish interfaces */
#define HAVE_EVP_BF 1

/* Define to 1 if you have the `EVP_bf_cbc' function. */
#define HAVE_EVP_BF_CBC 1

/* Define to 1 if you have the `EVP_CIPHER_CTX_set_padding' function. */
/* #undef HAVE_EVP_CIPHER_CTX_SET_PADDING */

/* Define if the GNU gettext() function is already present or preinstalled. */
/* #undef HAVE_GETTEXT */

/* Define to 1 if you have the `HMAC_Init_ex' function. */
/* #undef HAVE_HMAC_INIT_EX */

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Linking with OpenSSL */
#define HAVE_SSL 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/xattr.h> header file. */
/* #undef HAVE_SYS_XATTR_H */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <valgrind/memcheck.h> header file. */
/* #undef HAVE_VALGRIND_MEMCHECK_H */

/* Define to 1 if you have the <valgrind/valgrind.h> header file. */
/* #undef HAVE_VALGRIND_VALGRIND_H */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "encfs"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.7.4"

/* xattr functions have additional options */
/* #undef XATTR_ADD_OPT */

/* Avoid problem with MingW define */
#define _FILE_OFFSET_BITS_SET_FTRUNCATE 1
