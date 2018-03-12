
#ifndef _SCALER_H
#define _SCALER_H

#include <stdint.h>
#include <SDL/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void fullscreen_upscale(uint32_t *to, uint32_t *from);
void scale15x(uint32_t *to, uint32_t *from);


#ifdef __cplusplus
}
#endif

#endif
