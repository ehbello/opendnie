OPENSC_FEATURES = pcsc

#Include support of minidriver 'cardmod'
MINIDRIVER_DEF = /DENABLE_CARDMOD

#Build MSI with the Windows Installer XML (WIX) toolkit, requires WIX >= 3.6
WIX_PATH = "C:\Program Files\Windows Installer XML v3.6"


# If you want support for OpenSSL (needed for pkcs15-init tool, software hashing in PKCS#11 library and verification):
# - download and build OpenSSL
# - uncomment the line starting with OPENSSL_DEF
# - set the OPENSSL_INCL_DIR below to your openssl include directory, preceded by "/I"
# - set the OPENSSL_LIB below to your openssl lib file
OPENSSL_DEF = /DENABLE_OPENSSL
!IF "$(OPENSSL_DEF)" == "/DENABLE_OPENSSL"
OPENSSL_INCL_DIR = /IC:\OpenSSL-Win32\include
OPENSSL_LIB = C:\OpenSSL-Win32\lib\VC\static\libeay32MT.lib C:\OpenSSL-Win32\lib\VC\static\ssleay32MT.lib user32.lib advapi32.lib crypt32.lib

PROGRAMS_OPENSSL = pkcs15-init.exe cryptoflex-tool.exe netkey-tool.exe piv-tool.exe westcos-tool.exe dnie-tool.exe
OPENSC_FEATURES = $(OPENSC_FEATURES) openssl
!ENDIF


# If you want support for zlib (Used for PIV, infocamere and actalis):
# - Download zlib and build
# - uncomment the line starting with ZLIB_DEF 
# - set the ZLIB_INCL_DIR below to the zlib include lib proceeded by "/I"
# - set the ZLIB_LIB  below to your zlib lib file
#ZLIB_DEF = /DENABLE_ZLIB
!IF "$(ZLIB_DEF)" == "/DENABLE_ZLIB"
ZLIB_INCL_DIR = /IC:\ZLIB\INCLUDE
ZLIB_LIB = C:\ZLIB\LIB\zlib.lib 
OPENSC_FEATURES = $(OPENSC_FEATURES) zlib
!ENDIF

# Used for MiniDriver
CNGSDK_INCL_DIR = "/IC:\Program Files\Microsoft CNG Development Kit\Include"

# Mandatory path to 'ISO C9x compliant stdint.h and inttypes.h for Microsoft Visual Studio'
# http://msinttypes.googlecode.com/files/msinttypes-r26.zip
# INTTYPES_INCL_DIR =  /IC:\opensc\dependencies\msys\local

ALL_INCLUDES = /I$(TOPDIR)\win32 /I$(TOPDIR)\src $(OPENSSL_INCL_DIR) $(ZLIB_INCL_DIR) $(LIBLTDL_INCL) $(INTTYPES_INCL_DIR) $(CNGSDK_INCL_DIR)
COPTS =  /D_CRT_SECURE_NO_DEPRECATE /MT /nologo /DHAVE_CONFIG_H $(ALL_INCLUDES) /D_WIN32_WINNT=0x0502 /DWIN32_LEAN_AND_MEAN $(OPENSSL_DEF) $(ZLIB_DEF) $(MINIDRIVER_DEF) /DOPENSC_FEATURES="\"$(OPENSC_FEATURES)\""
LINKFLAGS = /NOLOGO /INCREMENTAL:NO /MACHINE:IX86 /MANIFEST:NO /NODEFAULTLIB:MSVCRTD  /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:LIBCMTD

.c.obj::
	cl $(COPTS) /c $<

.rc.res::
	rc /l 0x0409 $<

clean::
	del /Q *.obj *.dll *.exe *.pdb *.lib *.def *.manifest
