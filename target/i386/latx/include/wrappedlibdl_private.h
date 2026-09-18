#if defined(GO) && defined(GOM) && defined(GO2) && defined(DATA)

GOM(dladdr, iFpp)
GOM(dladdr1, iFpppi)
GOM(dlclose, iFp)
GOM(dlerror, pFv)
DATAB(_dlfcn_hook, 8)
GOM(dlinfo, iFpip)
GOM(dlmopen, pFppi)
GOM(dlopen, pFpi)
GOM(dlsym, pFpp)
GOM(dlvsym, pFppp)   // Weak

#endif
