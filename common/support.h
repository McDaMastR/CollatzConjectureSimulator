/* 
 * Copyright (C) 2025 Seth McDonald
 * 
 * This file is part of Collatz Conjecture Simulator.
 * 
 * Collatz Conjecture Simulator is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * 
 * Collatz Conjecture Simulator is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with Collatz Conjecture
 * Simulator. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "def.h"

// Macros to aid in conditional compilation on FreeBSD

#if 1
#define CZ_FREE_BSD_USE_STDC_1989 1
#else
#define CZ_FREE_BSD_USE_STDC_1989 0
#endif

#if (                                                         \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) ||  \
	(                                                         \
		CZ_XOPEN_SOURCE >= CZ_SUS_2001 &&                     \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) ||  \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		defined(_ISOC23_SOURCE) &&                            \
		_ISOC23_SOURCE + 0 &&                                 \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		defined(_ISOC11_SOURCE) &&                            \
		_ISOC11_SOURCE + 0 &&                                 \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(14, 0, 14)) || \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		CZ_STDC_VERSION >= CZ_STDC_2011 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(14, 0, 14)) || \
	(                                                         \
		!defined(_POSIX_SOURCE) &&                            \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                  \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		!defined(_ANSI_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_STDC_1999 1
#else
#define CZ_FREE_BSD_USE_STDC_1999 0
#endif

#if (                                                         \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2024 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		CZ_XOPEN_SOURCE >= CZ_SUS_2024 &&                     \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		defined(_ISOC23_SOURCE) &&                            \
		_ISOC23_SOURCE + 0 &&                                 \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		defined(_ISOC11_SOURCE) &&                            \
		_ISOC11_SOURCE + 0 &&                                 \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(14, 0, 14)) || \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		CZ_STDC_VERSION >= CZ_STDC_2011 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(14, 0, 14)) || \
	(                                                         \
		!defined(_POSIX_SOURCE) &&                            \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                  \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		!defined(_ANSI_SOURCE) &&                             \
		!defined(_C99_SOURCE) &&                              \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(10, 0, 4))
#define CZ_FREE_BSD_USE_STDC_2011 1
#else
#define CZ_FREE_BSD_USE_STDC_2011 0
#endif

#if (                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		defined(_ISOC23_SOURCE) &&                            \
		_ISOC23_SOURCE + 0 &&                                 \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		(                                                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||             \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&                \
		CZ_STDC_VERSION >= CZ_STDC_2023 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		!defined(_POSIX_SOURCE) &&                            \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                  \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		!defined(_ANSI_SOURCE) &&                             \
		!defined(_C99_SOURCE) &&                              \
		!defined(_C11_SOURCE) &&                              \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28))
#define CZ_FREE_BSD_USE_STDC_2023 1
#else
#define CZ_FREE_BSD_USE_STDC_2023 0
#endif

#if (                                                        \
		defined(_POSIX_SOURCE) &&                            \
		!defined(_ANSI_SOURCE)) ||                           \
	(                                                        \
		(                                                    \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988 ||            \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&               \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_POSIX_1988 1
#else
#define CZ_FREE_BSD_USE_POSIX_1988 0
#endif

#if (                                                        \
		(                                                    \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1990 ||            \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&               \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_POSIX_1990 1
#else
#define CZ_FREE_BSD_USE_POSIX_1990 0
#endif

#if (                                                        \
		(                                                    \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1993 ||            \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&               \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_1992 &&                \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 38)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_POSIX_1992 1
#else
#define CZ_FREE_BSD_USE_POSIX_1992 0
#endif

#if (                                                        \
		(                                                    \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1993 ||            \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&               \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_POSIX_1993 1
#else
#define CZ_FREE_BSD_USE_POSIX_1993 0
#endif

#if (                                                        \
		(                                                    \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996 ||            \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&               \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_POSIX_1996 1
#else
#define CZ_FREE_BSD_USE_POSIX_1996 0
#endif

#if (                                                        \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 &&                \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		CZ_XOPEN_SOURCE >= CZ_SUS_2001 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_POSIX_2001 1
#else
#define CZ_FREE_BSD_USE_POSIX_2001 0
#endif

#if (                                                        \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&                \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0, 67)) || \
	(                                                        \
		CZ_XOPEN_SOURCE >= CZ_SUS_2008 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0, 67)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0, 67))
#define CZ_FREE_BSD_USE_POSIX_2008 1
#else
#define CZ_FREE_BSD_USE_POSIX_2008 0
#endif

#if (                                                         \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2024 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		CZ_XOPEN_SOURCE >= CZ_SUS_2024 &&                     \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		!defined(_POSIX_SOURCE) &&                            \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                  \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		!defined(_ANSI_SOURCE) &&                             \
		!defined(_C99_SOURCE) &&                              \
		!defined(_C11_SOURCE) &&                              \
		!defined(_C23_SOURCE) &&                              \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28))
#define CZ_FREE_BSD_USE_POSIX_2024 1
#else
#define CZ_FREE_BSD_USE_POSIX_2024 0
#endif

#if (                                                        \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_XSI_1997 1
#else
#define CZ_FREE_BSD_USE_XSI_1997 0
#endif

#if (                                                        \
		CZ_XOPEN_SOURCE >= CZ_SUS_2001 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(5, 0, 33))
#define CZ_FREE_BSD_USE_XSI_2001 1
#else
#define CZ_FREE_BSD_USE_XSI_2001 0
#endif

#if (                                                        \
		CZ_XOPEN_SOURCE >= CZ_SUS_2008 &&                    \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0, 67)) || \
	(                                                        \
		!defined(_POSIX_SOURCE) &&                           \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                 \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                     \
		!defined(_ANSI_SOURCE) &&                            \
		!defined(_C99_SOURCE) &&                             \
		!defined(_C11_SOURCE) &&                             \
		!defined(_C23_SOURCE) &&                             \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0, 67))
#define CZ_FREE_BSD_USE_XSI_2008 1
#else
#define CZ_FREE_BSD_USE_XSI_2008 0
#endif

#if (                                                         \
		CZ_XOPEN_SOURCE >= CZ_SUS_2024 &&                     \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28)) || \
	(                                                         \
		!defined(_POSIX_SOURCE) &&                            \
		CZ_POSIX_C_SOURCE < CZ_POSIX_1988 &&                  \
		CZ_XOPEN_SOURCE < CZ_SUS_1997 &&                      \
		!defined(_ANSI_SOURCE) &&                             \
		!defined(_C99_SOURCE) &&                              \
		!defined(_C11_SOURCE) &&                              \
		!defined(_C23_SOURCE) &&                              \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(15, 0, 28))
#define CZ_FREE_BSD_USE_XSI_2024 1
#else
#define CZ_FREE_BSD_USE_XSI_2024 0
#endif

#if !defined(_POSIX_SOURCE) &&           \
	CZ_POSIX_C_SOURCE < CZ_POSIX_1988 && \
	CZ_XOPEN_SOURCE < CZ_SUS_1997 &&     \
	!defined(_ANSI_SOURCE) &&            \
	!defined(_C99_SOURCE) &&             \
	!defined(_C11_SOURCE) &&             \
	!defined(_C23_SOURCE)
#define CZ_FREE_BSD_USE_BSD 1
#else
#define CZ_FREE_BSD_USE_BSD 0
#endif
