
#ifndef _SCALER_H
#define _SCALER_H

#include <stdint.h>
#include <SDL/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif


void scale15x(uint32_t *to, uint32_t *from);
void scale15x_fast(uint32_t* dst, uint32_t* src);
void scale15x_pseudobilinear(uint32_t* dst, uint32_t* src);
void scale166x_fast(uint32_t* dst, uint32_t* src);
void scale166x_pseudobilinear(uint32_t* dst, uint32_t* src);
void fullscreen_upscale(uint32_t *to, uint32_t *from);
void fullscreen_upscale_pseudobilinear(uint32_t* dst, uint32_t* src);
void scaleborder15x(uint32_t* dst, uint32_t* src);
void scaleborder166x(uint32_t* dst, uint32_t* src);

void scale15x_2(uint32_t* dst, uint32_t* src);
void scale166x_2(uint32_t* dst, uint32_t* src);
void fullscreen_2(uint32_t* dst, uint32_t* src);

void scale15x_dotmatrix2(uint32_t* dst, uint32_t* src, const uint32_t gridcolor);
void scale166x_dotmatrix2(uint32_t* dst, uint32_t* src, const uint32_t gridcolor);
void fullscreen_dotmatrix2(uint32_t* dst, uint32_t* src, const uint32_t gridcolor);
void scaleborder15x_2(uint32_t* dst, uint32_t* src);
void scaleborder166x_2(uint32_t* dst, uint32_t* src);

void scale15x_crt2(uint32_t* dst, uint32_t* src);
void scale166x_crt2(uint32_t* dst, uint32_t* src);
void fullscreen_crt2(uint32_t* dst, uint32_t* src);
void scaleborder15x_crt2(uint32_t* dst, uint32_t* src);
void scaleborder166x_crt2(uint32_t* dst, uint32_t* src);

void scale15x_dotmatrix3(uint32_t* dst, uint32_t* src, const uint32_t gridcolor);
void scale166x_dotmatrix3(uint32_t* dst, uint32_t* src, const uint32_t gridcolor);
void fullscreen_dotmatrix3(uint32_t* dst, uint32_t* src, const uint32_t gridcolor);
void scaleborder15x_3(uint32_t* dst, uint32_t* src);
void scaleborder166x_3(uint32_t* dst, uint32_t* src);

void scale15x_crt3(uint32_t* dst, uint32_t* src);
void scale166x_crt3(uint32_t* dst, uint32_t* src);
void fullscreen_crt3(uint32_t* dst, uint32_t* src);
void scaleborder15x_crt3(uint32_t* dst, uint32_t* src);
void scaleborder166x_crt3(uint32_t* dst, uint32_t* src);

#ifdef __cplusplus
}
#endif

#endif
