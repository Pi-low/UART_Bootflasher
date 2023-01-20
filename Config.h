/*
 *  The UART Flashing tool aims to flash embedded software for the
 *  "Big Brain" board system.
 *  Copyright (C) 2023 Nello CHOMMANIVONG
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/* CONFIGURATION */
#define ALLOCATION_SIZE 1024
#define BLOCK_SIZE 128
#define EXTENSION 256
#define PRINT_GROUP_BYTE_PER_LINE 32


#ifdef DEBUG_CONFIG
/*  ---- USED FOR DEBUGGGING PURPOSES ---- */
#define SEND_UART 1u
#define PRINT_DEBUG_TRACE 1u
#else
/*  ---- DISABLE ALL DEBUG FEATURES ---- */
#define PRINT_DEBUG_TRACE 0u
#define SEND_UART 1u
#endif

#define PRINT_BLOCK_RAW 0u
#define PRINT_BLOCK_UART 0u
#define PRINT_BLOCK_CRC 0u
#endif // CONFIG_H_INCLUDED
