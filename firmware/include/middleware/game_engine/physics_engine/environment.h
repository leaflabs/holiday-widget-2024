#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#define ENVIRONMENT_MAX_X (int32_t) INT16_MAX
#define ENVIRONMENT_MIN_X (int32_t)(INT16_MIN + 1)
#define ENVIRONMENT_MAX_Y (int32_t) INT16_MAX
#define ENVIRONMENT_MIN_Y (int32_t)(INT16_MIN + 1)

#define GRID_SIZE (int32_t)7
#define GRID_UNIT_SIZE (int32_t)((UINT16_MAX - 1) / 7)
#define GRID_MIN (int32_t)(INT16_MIN + 1)

#endif