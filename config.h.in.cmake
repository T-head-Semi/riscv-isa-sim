/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* define if subproject MCPPBS_SPROJ_NORM is enabled */
#define CUSTOMEXT_ENABLED /**/

/* Default value for --isa switch */
#cmakedefine DEFAULT_ISA "@DEFAULT_ISA@"

/* Default value for --priv switch */
#cmakedefine DEFAULT_PRIV "@DEFAULT_PRIV@"

/* Default value for --varch switch */
#cmakedefine DEFAULT_VARCH "@DEFAULT_VARCH@"

/* Default value for --prefix switch */
#cmakedefine PREFIX "@PREFIX@"

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine DISASM_ENABLED /**/

/* Executable name of device-tree-compiler */
#cmakedefine DTC "dtc"

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine FDT_ENABLED /**/

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine FESVR_ENABLED /**/

/* cmakedefine if the Boost library is available */
#cmakedefine HAVE_BOOST /**/

/* cmakedefine if the Boost::ASIO library is available */
#cmakedefine HAVE_BOOST_ASIO /**/

/* Dynamic library loading is supported */
#cmakedefine HAVE_DLOPEN /**/

/* cmakedefine to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* cmakedefine to 1 if you have the `boost_regex' library (-lboost_regex). */
#cmakedefine HAVE_LIBBOOST_REGEX 1

/* cmakedefine to 1 if you have the `boost_system' library (-lboost_system). */
#cmakedefine HAVE_LIBBOOST_SYSTEM 1

/* cmakedefine to 1 if you have the `pthread' library (-lpthread). */
#cmakedefine HAVE_LIBPTHREAD 1

/* cmakedefine to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* cmakedefine to 1 if struct statx exists. */
#cmakedefine HAVE_STATX

/* cmakedefine to 1 if struct statx has stx_mnt_id. */
#cmakedefine HAVE_STATX_MNT_ID

/* cmakedefine to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* cmakedefine to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* cmakedefine to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* cmakedefine to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* cmakedefine to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* cmakedefine to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* cmakedefine to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* cmakedefine to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "Andrew Waterman"

/* cmakedefine to the full name of this package. */
#cmakedefine PACKAGE_NAME "RISC-V ISA Simulator"

/* cmakedefine to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "RISC-V ISA Simulator ?"

/* cmakedefine to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "spike"

/* cmakedefine to the home page for this package. */
#cmakedefine PACKAGE_URL ""

/* cmakedefine to the version of this package. */
#cmakedefine PACKAGE_VERSION "?"

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine RISCV_ENABLED /**/

/* Executable name of device-tree-compiler */
#cmakedefine DTC "@DTC@"

/* Enable commit log generation */
#cmakedefine RISCV_ENABLE_COMMITLOG

/* Enable hardware management of PTE accessed and dirty bits */
#cmakedefine RISCV_ENABLE_DIRTY

/* Enable support for running target in either endianness */
#cmakedefine RISCV_ENABLE_DUAL_ENDIAN

/* Enable PC histogram generation */
#cmakedefine RISCV_ENABLE_HISTOGRAM

/* Enable hardware support for misaligned loads and stores */
#cmakedefine RISCV_ENABLE_MISALIGNED

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine SOFTFLOAT_ENABLED

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine SPIKE_DASM_ENABLED

/* cmakedefine if subproject MCPPBS_SPROJ_NORM is enabled */
#cmakedefine SPIKE_MAIN_ENABLED

/* cmakedefine to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Enable hardware support for have int128 */
#cmakedefine HAVE_INT128

/* Default value for --with-target switch */
#cmakedefine TARGET_ARCH "riscv64-unknown-elf"

#if defined ENABLE_FORCE_RISCV
# ifndef RISCV_ENABLE_COMMITLOG
#  define RISCV_ENABLE_COMMITLOG
# endif
#endif

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

