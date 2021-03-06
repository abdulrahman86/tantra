#include <fb.h>
#include <stddef.h>

#include <kdebug.h>

const char* banner =
#include <banner.h>
;

static uint16_t *FB = (uint16_t *) FB_ADDRESS;
static uint16_t s_fbx = 0;
static uint16_t s_fby = 0;

static uint16_t s_attribute = 0x0200;

static inline uint16_t fb_attribute_opts(uint8_t bg, uint8_t fg)
{
    return ((((bg & 0x0f) << 4) | (fg & 0x0f)) << 8) & 0xff00;
}

static inline uint16_t fb_blank()
{
   return ' ' | s_attribute;
}

static inline uint16_t fb_position()
{
   return (s_fby * FB_COLUMNS) + s_fbx;
}

static inline void fb_move_cursor()
{
    uint16_t location = fb_position();
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT,    ((location >> 8) & 0x00ff));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT,    location & 0x00ff);
}

static inline void fb_scroll()
{
    if (s_fby >= FB_ROWS)
    {
        uint16_t blank = fb_blank();
        for (uint32_t i = 8 * FB_COLUMNS; i < FB_COLUMNS * (FB_ROWS - 1); i++)
        {
            FB[i] = FB[i+80];
        }

        for (uint32_t i = FB_COLUMNS * (FB_ROWS - 1); i < FB_COLUMNS * FB_ROWS; i++)
        {
            FB[i] = blank;
        }
        s_fby = (FB_ROWS - 1);
    }
}

void fb_put(char c)
{
    uint32_t location = fb_position();

    if (c == 0x08 && s_fbx)
    {
        s_fbx--;
    }
    else if (c == '\r')
    {
        s_fbx = 0;
    }
    else if (c == '\n')
    {
        s_fbx = 0;
        s_fby++;
    }
    else if (c >= ' ')
    {
        FB[location] = c | s_attribute;
        s_fbx++;
    }

    if (s_fbx >= FB_COLUMNS)
    {
        s_fbx = 0;
        s_fby++;
    }
    fb_scroll();
    fb_move_cursor();
}

void fb_clear()
{
    uint32_t blank  = fb_blank();
    for (uint32_t i = 0; i < FB_COLUMNS * FB_ROWS; i++)
    {
        FB[i] = blank;
    }
    s_fbx = 0;
    s_fby = 0;
}

void fb_print(const char *c)
{
    uint32_t i = 0;
    while (c[i] != '\0')
    {
        fb_put(c[i++]);
    }
}

static void fb_putcolor(char c, uint8_t bg, uint8_t fg)
{
    uint16_t original_attribute = s_attribute;
    s_attribute = fb_attribute_opts(bg, fg);
    fb_put(c);
    s_attribute = original_attribute;
}

void fb_update_timer(uint32_t tick)
{
    uint16_t tx = s_fbx;
    uint16_t ty = s_fby;
    uint16_t ta = s_attribute;

    s_fbx = 64;
    s_fby = 1;
    s_attribute = 0xe800;

    kprintf(" Clock: %us ", tick);

    s_fbx = tx;
    s_fby = ty;
    s_attribute = ta;
}

void init_fb()
{
    fb_clear();
    for (int idx = 0; banner[idx] != '\0'; idx++)
    {
        char c = banner[idx];
        if (c == '\0') return;
        uint8_t bg = FB_DGRAY;
        uint8_t fg = FB_LGRAY;

        if (c == 'w')
        {
            bg = FB_LMAGENTA;
            fg = FB_BROWN;
        }
        fb_putcolor(c, bg, fg);
    }
}

