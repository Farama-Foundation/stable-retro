/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_PLUGIN_H__
#define __LIGHTREC_PLUGIN_H__

#ifdef LIGHTREC

#define drc_is_lightrec() 1

#else /* if !LIGHTREC */

#define drc_is_lightrec() 0

#endif

#endif /* __LIGHTREC_PLUGIN_H__ */

