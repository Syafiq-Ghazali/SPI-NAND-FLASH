/*
|===============================================================================
|
| File:         DATATYPE.h
|
| Project:      SPI_NAND
|
| Description:  Common fixed-width data types.
|
|=============================================================================*/

#ifndef DATATYPE_H
#define DATATYPE_H

/*=== INCLUDE FILES ==========================================================*/

#include <stdbool.h>
#include <stdint.h>

/*=== #DEFINES ===============================================================*/

#ifndef TRUE
#define TRUE                    ((Bool_t)1u)
#endif

#ifndef FALSE
#define FALSE                   ((Bool_t)0u)
#endif

#ifndef NULL
#define NULL                    (0u)
#endif

/*=== TYPE DEFINITIONS =======================================================*/

typedef bool           Bool_t;
typedef uint16_t       Uint16_t;
typedef uint32_t       Uint32_t;
typedef uint64_t       Uint64_t;
typedef int16_t        Int16_t;
typedef int32_t        Int32_t;
typedef int64_t        Int64_t;
typedef float          Float32_t;
typedef long double    Float64_t;

#endif /* DATATYPE_H */
