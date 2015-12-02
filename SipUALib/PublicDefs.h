#pragma once

#ifndef __PUBLICDEFS_H__
#define __PUBLICDEFS_H__

// set inlining
#ifndef   _INLINE
#define   _INLINE inline
#endif // _INLINE

#ifndef   _ENABLE_INLINES
#define   _ENABLE_INLINES
#endif // _ENABLE_INLINES


#if (defined(_DEBUG) || defined( _FORCE_NO_INLINING )) && !defined( _FORCE_INLINING )
	#define _INLINE
	#ifdef _ENABLE_INLINES
		#undef _ENABLE_INLINES
	#endif
#else
	#define _INLINE inline
	#define _ENABLE_INLINES
#endif

//lint -ident($) avoid PC-Lint error 27 (using special character $)
#define Stringize( L) #L 
#define MakeString(M, L) M(L) 
#define $LINE MakeString (Stringize, __LINE__) 
#define Reminder __FILE__ "(" $LINE "): Reminder: "
#define Warning __FILE__ "(" $LINE "): Warning: " 

#define _FILER_VERSION(x) (x)

#ifdef _DEBUG
#define NODEFAULT ASSERT(0); __assume(0)
#else
#define NODEFAULT __assume(0)
#endif

#define FENWAY_GRADING_FIXES 

#include <crtdefs.h>

#define NOALIAS _CRTNOALIAS
#define RESTRICT _CRTRESTRICT


#endif // __PUBLICDEFS_H__

