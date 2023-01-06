#ifndef __GPSIM_EXPORTS_H__
#define __GPSIM_EXPORTS_H__

#ifdef _WIN32
// Microsoft Visual C DLL export and import macros
// for classes exported from gpsim.exe.
// Supported by GCC on Windows.
// Each DLL will need its own versions with xxx_DLL_EXPORT
// defined for the modules that contain the code.
# if defined(LIBGPSIM_DLL_EXPORT)
#  define LIBGPSIM_EXT_CLASS __declspec( dllexport )
#  define LIBGPSIM_EXPORT    __declspec( dllexport )
// #pragma message ("dllexport")
# else
#  define LIBGPSIM_EXT_CLASS __declspec( dllimport )
#  define LIBGPSIM_EXPORT
// #pragma message ("dllimport")
# endif
#else
#  define LIBGPSIM_EXT_CLASS
#  define LIBGPSIM_EXPORT
// #pragma message ("LIBGPSIM_EXT_CLASS defined a nothing")
#endif

class ProgramFileType;
///
///   Exported functions
bool LIBGPSIM_EXPORT IsFileExtension(const char *pszFile, const char *pFileExt);
void LIBGPSIM_EXPORT RegisterProgramFileType(ProgramFileType * pPFT);
void LIBGPSIM_EXPORT EnableRealTimeMode(bool bEnable);
void LIBGPSIM_EXPORT EnableRealTimeModeWithGui(bool bEnable);

#endif
