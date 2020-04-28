// Debug.h

#ifndef _DEBUG_h
#define _DEBUG_h

#define DEBUG

#ifdef DEBUG
	#define debug(format, ...) Serial.printf_P((PGM_P)F(format), ## __VA_ARGS__)
#else
  #define debug(format, ...)
#endif

#endif

