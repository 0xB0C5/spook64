#ifndef SPOOK64_LIBDRAGON_HAX
#define SPOOK64_LIBDRAGON_HAX

static inline uint32_t __rdp_round_to_power( uint32_t number )
{
    if( number <= 4   ) { return 4;   }
    if( number <= 8   ) { return 8;   }
    if( number <= 16  ) { return 16;  }
    if( number <= 32  ) { return 32;  }
    if( number <= 64  ) { return 64;  }
    if( number <= 128 ) { return 128; }

    /* Any thing bigger than 256 not supported */
    return 256;
}
static inline uint32_t __rdp_log2( uint32_t number )
{
    switch( number )
    {
        case 4:
            return 2;
        case 8:
            return 3;
        case 16:
            return 4;
        case 32:
            return 5;
        case 64:
            return 6;
        case 128:
            return 7;
        default:
            /* Don't support more than 256 */
            return 8;
    }
}
#define ROUND_UP(n, d) ({ \
	typeof(n) _n = n; typeof(d) _d = d; \
	(((_n) + (_d) - 1) / (_d) * (_d)); \
})
static uint32_t __rdp_load_texture( uint32_t texslot, uint32_t texloc, mirror_t mirror_enabled, sprite_t *sprite, void *buffer, int sl, int tl, int sh, int th )
{
    /* Point the RDP at the actual sprite data */
    rdpq_set_texture_image_raw(0, PhysicalAddr(buffer), (tex_format_t)(sprite->flags & SPRITE_FLAGS_TEXFORMAT), sprite->width, sprite->height);

    /* Figure out the s,t coordinates of the sprite we are copying out of */
    int twidth = sh - sl + 1;
    int theight = th - tl + 1;

    /* Figure out the power of two this sprite fits into */
    uint32_t real_width  = __rdp_round_to_power( twidth );
    uint32_t real_height = __rdp_round_to_power( theight );
    uint32_t wbits = __rdp_log2( real_width );
    uint32_t hbits = __rdp_log2( real_height );

    uint32_t tmem_pitch = ROUND_UP(real_width * TEX_FORMAT_BITDEPTH((tex_format_t)(sprite->flags & SPRITE_FLAGS_TEXFORMAT)) / 8, 8);

    /* Instruct the RDP to copy the sprite data out */
    rdpq_set_tile_full(
        texslot,
        (tex_format_t)(sprite->flags & SPRITE_FLAGS_TEXFORMAT),
        texloc,
        tmem_pitch,
        0, 
        0, 
        mirror_enabled != MIRROR_DISABLED ? 1 : 0,
        hbits,
        0,
        0,
        mirror_enabled != MIRROR_DISABLED ? 1 : 0,
        wbits,
        0);

    /* Copying out only a chunk this time */
    rdpq_load_tile(0, sl, tl, sh+1, th+1);

    /* Return the amount of texture memory consumed by this texture */
    return tmem_pitch * real_height;
}

uint32_t rdp_load_texture_stride_hax( uint32_t texslot, uint32_t texloc, mirror_t mirror, sprite_t *sprite, void *buffer, int offset )
{
    if( !sprite ) { return 0; }

    /* Figure out the s,t coordinates of the sprite we are copying out of */
    int twidth = sprite->width / sprite->hslices;
    int theight = sprite->height / sprite->vslices;

    int sl = (offset % sprite->hslices) * twidth;
    int tl = (offset / sprite->hslices) * theight;
    int sh = sl + twidth - 1;
    int th = tl + theight - 1;

    return __rdp_load_texture( texslot, texloc, mirror, sprite, buffer, sl, tl, sh, th );
}

void rdp_load_texture_hax(tex_format_t format, uint32_t hbits, uint32_t wbits, void *buff) {
	uint32_t width = 1 << hbits;
	uint32_t height = 1 << wbits;
	uint32_t tmem_pitch = width * TEX_FORMAT_BITDEPTH(format) / 8;

    rdpq_set_texture_image_raw(0, PhysicalAddr(buff), format, width, height);

    rdpq_set_tile_full(
        0, // texslot,
        format,
        0, // texloc,
        tmem_pitch,
        0, 
        0, 
        0, // mirror
        hbits,
        0,
        0,
        0, // mirror
        wbits,
        0);

    rdpq_load_tile(0, 0, 0, width, height);
}

#endif