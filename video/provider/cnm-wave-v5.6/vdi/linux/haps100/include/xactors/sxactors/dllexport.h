#pragma once
#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#ifdef _WIN32
#ifdef EXPORTING
//define EXPORTING when compiling the library
#define DLLEXPORT __declspec(dllexport)
#else
//do not define EXPORTING when using the library
#define DLLEXPORT __declspec(dllimport)
#endif
#else
#define DLLEXPORT
#endif
