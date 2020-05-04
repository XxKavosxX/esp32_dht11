#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "inttypes.h"
#include "stdlib.h"

#define set_bit(y, bit)         (y|= (1<<bit))
#define clr_bit(y, bit)         (y&= ~(1<<bit))

#endif