// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

// if linking with the static libCZI-library, the variable "_LIBCZISTATICLIB" should be defined.
#if !defined(_LIBCZISTATICLIB)

	#ifdef LIBCZI_EXPORTS
		#ifdef __GNUC__
			#define LIBCZI_API __attribute__ ((visibility ("default")))
		#else
			#define LIBCZI_API __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define LIBCZI_API
		#else
			#define LIBCZI_API __declspec(dllimport)
		#endif
	#endif

#else

	#define LIBCZI_API 

#endif


