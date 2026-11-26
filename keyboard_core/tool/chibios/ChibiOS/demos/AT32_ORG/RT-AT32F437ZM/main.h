/*
 * main.h
 *
 *  Created on: 2023 Sep 8
 *      Author: I0364
 */

#ifndef MAIN_H_
#define MAIN_H_


//typedef unsigned char                         bool;
typedef signed char                           int8_t;
typedef short int                             int16_t;
typedef long int                              int32_t;
typedef unsigned char                         uint8_t;
typedef unsigned short int                    uint16_t;
typedef unsigned long int                     uint32_t;

#define true	1
#define false	0

int uprintf(const char* fmt, ...);

#endif /* MAIN_H_ */
