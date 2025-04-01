#ifndef OS_TYPE_H
#define OS_TYPE_H

typedef unsigned char uint8_t;
typedef unsigned char UINT8_T;
typedef unsigned char bool;
typedef unsigned char u8;
typedef unsigned char U8;

typedef unsigned short uint16_t;
typedef unsigned short UINT16_T;
typedef unsigned short u16;
typedef unsigned short U16;

typedef unsigned int uint32_t;
typedef unsigned int UINT32_T;
typedef unsigned int u32;
typedef unsigned int U32;

typedef unsigned long long uint64_t;
typedef unsigned long long UINT64_T;
typedef unsigned long long u64;
typedef unsigned long long U64;

typedef float f32;
typedef double f64;


#ifndef NULL
#define NULL ((void *)0)
#endif // !NULL

#define TRUE 1
#define FALSE 0

#define true TRUE
#define false FALSE

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define AVR(X, Y) (((X) + (Y)) / 2)

#define SET_BIT(var, pos) (((var) |= (1 << (pos))))

#define CLEAR_BIT(var, pos) ((var) = ((var) & ~(1 << (pos))))

#endif // OS_TYPE_H