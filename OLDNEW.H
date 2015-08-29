// Oldnew.h

// FS: For toggling old code

#define OLDNET // FS: Do NOT use WATT320
#undef OLDSOUND // FS: Use the new S_Sound.c from linuxdoom

// FS: Just some paranoia to remove other operating system specific code
#undef NORMALUNIX
#undef LINUX
#undef __NeXT__
