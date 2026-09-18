#ifndef __wrappedlibxcursorTYPES_H_
#define __wrappedlibxcursorTYPES_H_

#ifndef LIBNAME
#error You should only #include this file inside a wrapped*.c file
#endif
#ifndef ADDED_FUNCTIONS
#define ADDED_FUNCTIONS() 
#endif

typedef void* (*pFpp_t)(void*, void*);

#define SUPER() ADDED_FUNCTIONS() \
	GO(XcursorLibraryLoadCursor, pFpp_t)

#endif // __wrappedlibxcursorTYPES_H_

