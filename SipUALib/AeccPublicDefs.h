#pragma once

#ifndef __AECCPUBLICDEFS_H__
#define __AECCPUBLICDEFS_H__

/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright 2002 Autodesk, Inc. All rights reserved.
//
//                     ****  CONFIDENTIAL MATERIAL  ****
//
// The information contained herein is confidential, proprietary to
// Autodesk, Inc., and considered a trade secret.  Use of this information
// by anyone other than authorized employees of Autodesk, Inc. is granted
// only under a written nondisclosure agreement, expressly prescribing the
// the scope and manner of such use.
//
/////////////////////////////////////////////////////////////////////////////
//
// Description: 
//
/////////////////////////////////////////////////////////////////////////////

// set inlining
#ifndef   AEC_INLINE
#define   AEC_INLINE inline
#endif // AEC_INLINE

#ifndef   AEC_ENABLE_INLINES
#define   AEC_ENABLE_INLINES
#endif // AEC_ENABLE_INLINES


#if (defined(_DEBUG) || defined( AECC_FORCE_NO_INLINING )) && !defined( AECC_FORCE_INLINING )
	#define AECC_INLINE
	#ifdef AECC_ENABLE_INLINES
		#undef AECC_ENABLE_INLINES
	#endif
#else
	#define AECC_INLINE inline
	#define AECC_ENABLE_INLINES
#endif

//lint -ident($) avoid PC-Lint error 27 (using special character $)
#define Stringize( L) #L 
#define MakeString(M, L) M(L) 
#define $LINE MakeString (Stringize, __LINE__) 
#define Reminder __FILE__ "(" $LINE "): Reminder: "
#define Warning __FILE__ "(" $LINE "): Warning: " 

#define AECC_FILER_VERSION(x) (x)

#ifdef _DEBUG
#define NODEFAULT ASSERT(0); __assume(0)
#else
#define NODEFAULT __assume(0)
#endif

#define FENWAY_GRADING_FIXES 

#include <crtdefs.h>

#define AECCNOALIAS _CRTNOALIAS
#define AECCRESTRICT _CRTRESTRICT


/////////////////////////////////////////////////////////////////////////////

#endif // __AECCPUBLICDEFS_H__

