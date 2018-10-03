/*  =========================================================================
    fty_common_translation_base - Singleton translation object

    Copyright (C) 2014 - 2018 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#ifndef FTY_COMMON_TRANSLATION_BASE_H_INCLUDED
#define FTY_COMMON_TRANSLATION_BASE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new fty_common_translation_base
FTY_COMMON_TRANSLATION_EXPORT fty_common_translation_base_t *
    fty_common_translation_base_new (void);

//  Destroy the fty_common_translation_base
FTY_COMMON_TRANSLATION_EXPORT void
    fty_common_translation_base_destroy (fty_common_translation_base_t **self_p);

//  Self test of this class
FTY_COMMON_TRANSLATION_EXPORT void
    fty_common_translation_base_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
