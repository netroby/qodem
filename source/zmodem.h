/*
 * zmodem.h
 *
 * This module is licensed under the GNU General Public License
 * Version 2.  Please see the file "COPYING" in this directory for
 * more information about the GNU General Public License Version 2.
 *
 *     Copyright (C) 2015  Kevin Lamonte
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __ZMODEM_H__
#define __ZMODEM_H__

/* Includes --------------------------------------------------------------- */

#include "common.h"     /* Q_BOOL */
#include "forms.h"      /* struct file_info * */

/* Defines ---------------------------------------------------------------- */

typedef enum {
        Z_CRC16,        /* Zmodem 16-bit CRC */
        Z_CRC32         /* Zmodem 32-bit CRC */
} ZMODEM_FLAVOR;

/* Globals ---------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

extern void zmodem(unsigned char * input, const int input_n, unsigned char * output, int * output_n, const int output_max);

extern Q_BOOL zmodem_start(struct file_info * file_list, const char * pathname, const Q_BOOL send, const ZMODEM_FLAVOR in_flavor);

extern void zmodem_stop(const Q_BOOL save_partial);

#endif /* __ZMODEM_H__ */