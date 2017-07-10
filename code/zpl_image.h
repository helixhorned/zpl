/*

ZPL - Image module

License:
    This software is dual-licensed to the public domain and under the following
    license: you are granted a perpetual, irrevocable license to copy, modify,
    publish, and distribute this file as you see fit.
    
Warranty:
    No warranty is implied, use at your own risk.
    
Usage:
    #define ZPL_IMAGE_IMPLEMENTATION exactly in ONE source file right BEFORE including the library, like:
    
    #define ZPL_IMAGE_IMPLEMENTATION
    #include"zpl_image.h"
    
Dependencies:
    zpl.h
    stb_image.h
    
    Make sure you properly include them!
    
Optional switches:
    ZPL_NO_GIF
    ZPL_NO_IMAGE_OPS
    
Filter options:
    ZPL_FILTER_FACTOR
    ZPL_FILTER_BIAS
    
Credits:
    Dominik Madarasz (GitHub: zaklaus)
    Sean Barrett (GitHub: nothings)
    GitHub: urraka
    
Version History:
    1.10 - Small changes
    1.00 - Initial version
    
*/

#ifndef ZPL_INCLUDE_ZPL_IMAGE_H
#define ZPL_INCLUDE_ZPL_IMAGE_H

#if defined(__cplusplus)
extern "C" {
#endif
    
    typedef union zplRgbColour { 
        u32 colour;
        struct { u8 r, g, b;};
    } zplRgbColour;
    
    typedef struct zplHsvColour { 
        u32 colour;
        struct { u8 h, s, v; };
    } zplHsvColour;
    
    ZPL_DEF zplRgbColour zpl_image_rgb_lerp(zplRgbColour a, zplRgbColour b, f32 t);
    
    ////////////////////////////////////////////////////////////////
    //
    // GIF Loader
    //
    // Uses stb_image.h for loading gif frames.
    //
    
#ifndef ZPL_NO_GIF
    typedef struct zplGifResult {
        i32 delay;
        u8 *data;
        struct zplGifResult *next;
    } zplGifResult;
    
    ZPL_DEF zplGifResult *zpl_image_gif_load(char const *filename, i32 *x, i32 *y, i32 *frames);
    ZPL_DEF void          zpl_image_gif_free(zplGifResult *gif, b32 aligned);
#endif

    ////////////////////////////////////////////////////////////////
    //
    // Image Operations
    //
    // Uses stb_image.h for loading gif frames.
    //
    
#ifndef ZPL_NO_IMAGE_OPS
    // NOTE(ZaKlaus): This is not sRGB aware!
    ZPL_DEF void zpl_image_rgb_resize(u32 *source, i32 source_w, i32 source_h,
                                      u32 *dest, i32 dest_w, i32 dest_h,
                                      i32 blur_iter, u32 *blur_mem);
    
#ifndef ZPL_FILTER_FACTOR
#define ZPL_FILTER_FACTOR 1.0
#endif
    
#ifndef ZPL_FILTER_BIAS
#define ZPL_FILTER_BIAS 0.0
#endif
    
    ZPL_DEF void zpl_image_rgb_filter(u32 *source, i32 source_w, i32 source_h,
                                      u32 *dest, 
                                      f64 *filter, i32 filter_w, i32 filter_h,
                                      f64 factor, f64 bias);
    
    ZPL_DEF void         zpl_image_init_srgb_table(u8 **table);
    ZPL_DEF zplRgbColour zpl_image_lin_to_srgb    (u8 *table, f64 vals[3]);
    ZPL_DEF zplHsvColour zpl_image_rgb_to_hsv     (zplRgbColour colour);
    ZPL_DEF zplRgbColour zpl_image_hsv_to_rgb     (zplHsvColour colour);
#endif
    
#if defined(__cplusplus)
}
#endif

#if defined(ZPL_IMAGE_IMPLEMENTATION) && !defined(ZPL_IMAGE_IMPLEMENTATION_DONE)
#define ZPL_IMAGE_IMPLEMENTATION_DONE

#if defined(__cplusplus)
extern "C" {
#endif
    
    zpl_inline zplRgbColour zpl_image_rgb_lerp(zplRgbColour a, zplRgbColour b, f32 t) {
#define LERP(c1, c2, c3) c1*(1.0-c3) + c2*c3
        zplRgbColour result = {0};
        
        result.r = LERP(a.r, b.r, t);
        result.g = LERP(a.g, b.g, t);
        result.b = LERP(a.b, b.b, t);
        
        return result;
#undef LERP
    }
    
    // NOTE(ZaKlaus): Gif
    
#ifndef ZPL_NO_GIF
    zpl_inline zplGifResult *zpl_image_gif_load(char const *filename, i32 *x, i32 *y, i32 *frames) {
        FILE *file;
        stbi__context s;
        zplGifResult *result;
        
        if (!(file = stbi__fopen(filename, "rb"))) {
            stbi__errpuc("can't open file", "Unable to open file"); return 0;
        }
        
        stbi__start_file(&s, file);
        
        if (stbi__gif_test(&s)) {
            i32 c;
            stbi__gif g;
            zplGifResult *head = stbi__malloc(zpl_size_of(zplGifResult));
            zplGifResult *prev = 0, *gr = head;
            
            zpl_zero_item(&g);
            zpl_zero_item(head);
            
            *frames = 0;
            
            while(gr->data = stbi__gif_load_next(&s, &g, &c, 4)) {
                if (gr->data == cast(u8 *)&s) {
                    gr->data = 0;
                    break;
                }
                
                if (prev) prev->next = gr;
                gr->delay = g.delay;
                prev = gr;
                gr = cast(zplGifResult *)stbi__malloc(zpl_size_of(zplGifResult));
                zpl_zero_item(gr);
                ++(*frames);
            }
            
            STBI_FREE(g.out);
            
            if (gr != head) {
                //STBI_FREE(gr);
            }
            
            if (*frames > 0) {
                *x = g.w;
                *y = g.h;
            }
            
            result = head;
        }
        else {
            // TODO(ZaKlaus): Add support.
            result = 0; //stbi__load_main(&s, x, y, frames, &bpp, 4, &ri, 8);
            *frames = !!result;
        }
        
        fclose(file);
        return result;
    }
    
    zpl_inline void zpl_image_gif_free(zplGifResult *gif, b32 aligned) {
        zplGifResult *elem, *prev = 0;
        for (elem = gif; elem; elem = elem->next) {
            if (aligned) {
                zpl_mfree(elem->data);
            }
            else {
                STBI_FREE(elem->data);
            }
            STBI_FREE(prev);
            prev = elem;
        }
        
        STBI_FREE(prev);
    }
#endif
    
    zpl_inline zplHsvColour zpl_image_rgb_to_hsv(zplRgbColour colour) {
        zplHsvColour result = {0};
        u8 rgb_min, rgb_max;
        
        rgb_min = colour.r < colour.g ? (colour.r < colour.b ? colour.r : colour.b) : (colour.g < colour.b ? colour.g : colour.b);
        rgb_max = colour.r > colour.g ? (colour.r > colour.b ? colour.r : colour.b) : (colour.g > colour.b ? colour.g : colour.b);
        
        result.v = rgb_max;
        if (result.v == 0) {
            result.h = result.s = 0;
            return result;
        }
        
        result.s = 255 * cast(i64)(rgb_max - rgb_min) / result.v;
        if (result.s == 0) {
            result.h = 0;
            return result;
        }
        
        /**/ if (rgb_max == colour.r) {
            result.h = 0   + 43 * (colour.g - colour.b) / (rgb_max - rgb_min);
        }
        else if (rgb_max == colour.g) {
            result.h = 85  + 43 * (colour.b - colour.r) / (rgb_max - rgb_min);
        }
        else {
            result.h = 171 + 43 * (colour.r - colour.g) / (rgb_max - rgb_min);
        }
        
        return result;
    }
    
    zpl_inline zplRgbColour zpl_image_hsv_to_rgb(zplHsvColour colour) {
        zplRgbColour result = {0};
        u8 region, rem, p, q, t;
        
        if (colour.s == 0) {
            result.r = result.g = result.b = colour.v;
            return result;
        }
        
        region =  colour.h / 43;
        rem    = (colour.h - (region * 43)) * 6;
        
        p = (colour.v * (255 - colour.s)) >> 8;
        q = (colour.v * (255 - ((colour.s * rem) >> 8))) >> 8;
        t = (colour.v * (255 - ((colour.s * (255 - rem)) >> 8))) >> 8;
        
        switch (region)
        {
            case 0: result.r = colour.v; result.g = t; result.b = p; break;
            case 1: result.r = q; result.g = colour.v; result.b = p; break;
            case 2: result.r = p; result.g = colour.v; result.b = t; break;
            case 3: result.r = p; result.g = q; result.b = colour.v; break;
            case 4: result.r = t; result.g = p; result.b = colour.v; break;
            default:result.r = colour.v; result.g = p; result.b = q; break;
        }
        
        return result;
    }
    
#ifndef ZPL_NO_IMAGE_OPS
    zpl_inline void zpl_image_rgb_resize(u32 *source, i32 source_w, i32 source_h,
                                         u32 *dest, i32 dest_w, i32 dest_h,
                                         i32 blur_iter, u32 *blur_mem) {
        
        zplRgbColour *src = cast(zplRgbColour *)&(source); 
        zplRgbColour *dst = cast(zplRgbColour *)&(dest); 
        
        b32 x_down = dest_w < source_w;
        b32 y_down = dest_h < source_h;
        
        i32 step_x;
        i32 step_y;
        
        if(x_down) {
            step_x = cast(i32)(source_w / cast(f32)dest_w);
        }
        else {
            step_x = cast(i32)(dest_w / cast(f32)source_w);
        }
        
        if(y_down) {
            step_y = cast(i32)(source_h / cast(f32)dest_h);
        }
        else {
            step_y = cast(i32)(dest_h / cast(f32)source_h);
        }
        
        
        
        for (i32 y = 0; y < dest_h; ++y) {
            for(i32 x = 0; x < dest_w; ++x) {
                zplRgbColour colour = {0};
                
                i32 o_x = x/step_x;
                if (x_down) o_x = x*step_x;
                i32 o_y = y/step_y;
                if (y_down) o_y = y*step_y;
                
                colour = src[o_y*source_w + o_x];
                
                dst[y*dest_w + x] = colour;
            }
        }
        
        if (blur_iter > 0) {
            ZPL_ASSERT_NOT_NULL(blur_mem);
            zpl_local_persist f64 filter[5][5] = {
                0, 0, 1, 0, 0,
                0, 1, 1, 1, 0,
                1, 1, 1, 1, 1,
                0, 1, 1, 1, 0,
                0, 0, 1, 0, 0,
            };
            
            f64 factor = 1.0 / 13.0;
            f64 bias = 0.0;
            
            zpl_memcopy(blur_mem, dest, dest_w*dest_h*4);
            
            zpl_image_rgb_filter(blur_mem, dest_w, dest_h,
                                 cast(u32 *)dest,
                                 &filter[0][0], 5, 5,
                                 factor, bias);
            
            
        }
    }

    zpl_inline void zpl_image_rgb_filter(u32 *source, i32 source_w, i32 source_h,
                                         u32 *dest, 
                                         f64 *filter, i32 filter_w, i32 filter_h,
                                         f64 factor, f64 bias) {
        
        zplRgbColour *src = cast(zplRgbColour *)(source); 
        zplRgbColour *dst = cast(zplRgbColour *)(dest); 
        
        for (i32 y = 0; y < source_h; ++y) {
            for(i32 x = 0; x < source_w; ++x) {
                i32 r = 0, g = 0, b = 0;
                
                for (i32 fy = 0; fy < filter_h; ++fy) {
                    for (i32 fx = 0; fx < filter_w; ++fx) {
                        i32 img_x = (x - filter_w / 2 + fx + source_w) % source_w;
                        i32 img_y = (y - filter_h / 2 + fy + source_h) % source_h;
                        
                        r += src[img_y * source_w + img_x].r * filter[fy*filter_w + fx];
                        g += src[img_y * source_w + img_x].g * filter[fy*filter_w + fx];
                        b += src[img_y * source_w + img_x].b * filter[fy*filter_w + fx];
                    }
                }
                
                dst[y * source_w + x].r = zpl_min(zpl_max(cast(i32)(factor * r + bias), 0), 255);
                dst[y * source_w + x].g = zpl_min(zpl_max(cast(i32)(factor * g + bias), 0), 255);
                dst[y * source_w + x].b = zpl_min(zpl_max(cast(i32)(factor * b + bias), 0), 255);
            }
        }
    }
    
#endif
    
#if defined(__cplusplus)
}
#endif

#endif

#endif // ZPL_INCLUDE_ZPL_GIF_H