/*
 * Copyright (c) 2018 naehrwert
 *
 * Copyright (c) 2018 CTCaer
 *
 * Copyright (c) 2024 robocassie
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gfx/di.h"
#include "gfx/gfx.h"
#include "mem/heap.h"
#include "power/max77620.h"
#include "rtc/max77620-rtc.h"
#include "soc/hw_init.h"
#include "soc/i2c.h"
#include "utils/btn.h"


gfx_ctxt_t gfx_ctxt;
gfx_con_t gfx_con;

boot_cfg_t *b_cfg;


void power_off()
{
    max77620_rtc_stop_alarm();
    i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, MAX77620_ONOFFCNFG1_PWR_OFF);
}

extern void pivot_stack(u32 stack_top);
extern void _start(void);

#define PATCHED_RELOC_SZ   0x94
#define IROM_START_ADDR 0x100000
#define IROM_END_ADDR   0x118000
#define NUM_ROWS 64
#define BYTES_PER_SCREEN (NUM_ROWS * 8)

void draw_irom(u32 from_addr) {
    // Clear the framebuffer and reset the cursor to 0, 0
    gfx_clear_grey(&gfx_ctxt, 0x00);
    gfx_con_setpos(&gfx_con, 0, 0);

    // Print IROM: in red
    gfx_printf(&gfx_con, "%kIROM:%k\n", COLOR_RED, 0xFFCCCCCC);
    u8* irom = (u8*) from_addr;
    for(int r = 0; r < NUM_ROWS; r++) {
        // Print the address in yellow
        gfx_printf(&gfx_con, "%k%X:%k ", COLOR_YELLOW, from_addr + (r * 8), 0xFFCCCCCC);
        for(int c = 0; c < 8; c++) {
            // Add a space after 4 colums
            if(c == 4)
                gfx_printf(&gfx_con, " ");
            u8 b = irom[c + r * 8];
            // If the byte is < 0x10 (one character),
            // pad it with a 0
            if(b < 0x10)
                gfx_printf(&gfx_con, "0");
            gfx_printf(&gfx_con, "%X ", b);
        }
        // Print the ASCII representation in blue
        gfx_printf(&gfx_con, "%k|", COLOR_BLUE);
        for(int c = 0; c < 8; c++) {
            char x = irom[c + r * 8];
            // Default to '.' if this isn't a printable ASCII character
            if(x < 32 || x > 126)
                x = 46;
            gfx_printf(&gfx_con, "%c", x);
        }
        gfx_printf(&gfx_con, "%k\n", 0xFFCCCCCC);
    }
}

void ipl_main() {
    b_cfg = (boot_cfg_t *)(IPL_LOAD_ADDR + PATCHED_RELOC_SZ);

    config_hw();
    pivot_stack(0x90010000);
    heap_init(0x90020000);

    display_init();
    u32* fb = display_init_framebuffer();
    gfx_init_ctxt(&gfx_ctxt, fb, 720, 1280, 720);
    gfx_con_init(&gfx_con, &gfx_ctxt);
    display_backlight_pwm_init();
    display_backlight_brightness(100, 1000);

    // Set pos to 0, 0
    gfx_con_setpos(&gfx_con, 0, 0);
    // Start IROM at the beginning
    u32 irom_from_addr = IROM_START_ADDR;
    // Draw the initial irom dump
    draw_irom(irom_from_addr);

    u32 old_btns = 0x0;

    while(1) {
        u32 btns = btn_read();
        // If we've just pressed volume down and
        // we aren't at the end of the IROM
        if((btns & BTN_VOL_DOWN) && !(old_btns & BTN_VOL_DOWN)) {
            if(irom_from_addr + BYTES_PER_SCREEN < IROM_END_ADDR) {
                irom_from_addr += BYTES_PER_SCREEN;
                draw_irom(irom_from_addr);
            }
        }
        // Or if we've just pressed volume up and
        // we aren't at the beginning of the IROM
        else if((btns & BTN_VOL_UP && !(old_btns & BTN_VOL_UP))) {
            if(irom_from_addr - BYTES_PER_SCREEN >= IROM_START_ADDR) {
                irom_from_addr -= BYTES_PER_SCREEN;
                draw_irom(irom_from_addr);
            }
        }
        // Power off if we press POWER
        else if(btns & BTN_POWER) {
            power_off();
        }

        old_btns = btns;
    }
}
