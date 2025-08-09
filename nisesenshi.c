//  PC-6001mk2/SR Nise-Senshi cart w/Ext-Kanji ROM for Pico2
//
//  PC-6001 Cart connector
//
//  GP0-7:  D0-7
//  GP8-23: A0-15
//  GP25: RESET -> Interrupt
//  GP26: CS2
//  GP27: CS3
//  GP28: MERQ
//  GP29: IORQ
//  GP30: WR
//  GP31: RD
//  GP32

#define FLASH_INTERVAL 300      // 5sec

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"
//#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/vreg.h"

// ROM configuration
// WeACT RP2350B with 16MiB Flash
// ExtKanji ROM 128KiB
// Senshi-ROM   128KiB * 64 = 8MiB
// LittleFS       7.5MiB (Not Used)

#define ROMBASE 0x10080000u

uint8_t *kanjirom=(uint8_t *)(ROMBASE-0x20000);
uint8_t *romcarts=(uint8_t *)(ROMBASE);

// Define the flash sizes
// This is setup to read a block of the flash from the end 
#define BLOCK_SIZE_BYTES (FLASH_SECTOR_SIZE)
// for 16M flash
#define HW_FLASH_STORAGE_BYTES  (7680 * 1024)
#define HW_FLASH_STORAGE_BASE   (1024*1024*16 - HW_FLASH_STORAGE_BYTES) 

// RAM configuration (256KiB)

uint8_t ramcart[0x20000];
uint8_t kanjiram[0x20000];
uint8_t senshiram[0x2000];  // Senshi-RAM 8KB (for CS3 area)

#define MAXROMPAGE 64       // = 128KiB * 64 pages = 8MiB

volatile uint8_t rompage=0xff;  // Page No of Senshi-cart (128KiB/page)
volatile uint8_t rombank=0;     // Bank No of Senshi-cart

uint32_t kanjiptr=0;
volatile uint8_t kanji_enable=0;
volatile uint32_t flash_command=0;

// uint8_t __attribute__  ((aligned(sizeof(unsigned char *)*4096))) flash_buffer[4096];

//
//  reset

//void __not_in_flash_func(z80reset)(void) {
void __not_in_flash_func(z80reset)(uint gpio,uint32_t event) {

    // Reset Bankno to Senshi-Cart

    rombank=0;

    gpio_acknowledge_irq(25,GPIO_IRQ_EDGE_FALL);

    return;
}

#if 0
static inline uint8_t io_read( uint16_t address)
{

    uint8_t b;

    switch(address&0xff) {

        case 0x77:
            return rompage;

        case 0xfd:  // Ext Kanji (Even)

            if(kanji_enable) {
                return kanjiram[kanjiptr<<1];
            } else {
                return 0xff;
            }

        case 0xfe:  // Ext Kanji (Odd)

            if(kanji_enable) {
                return kanjiram[(kanjiptr<<1)+1];
            } else {
                return 0xff;
            }


        default:

            return 0xff;

    }

    return 0xff;
}
#endif

static inline void io_write(uint16_t address, uint8_t data)
{

    uint8_t b;

    switch(address&0xff) {

        case 0x70:  // Senshi-cart control
        case 0x7f:  // For XeGrader

            rombank=data&0x1f;  // Max 32 Banks/cart
            return;

        case 0x77: 

            if(data!=0xff) {

                rompage=data&0x3f;
                flash_command=0x10000000|rompage;     
                return;

            } else {

                rompage=0xff;      
                return;                

            }

        case 0xfc:  // Kanji ptr

            kanjiptr=( (address&0xff00)>>8 ) + (data <<8 );

            return;

        case 0xff:  // Ext kanji control

            if(data==0xff) {
                kanji_enable=0;
            } else {
                kanji_enable=1;
            }

    }
 
    return;

}

void init_emulator(void) {

    // Erase Gamepad info

    kanji_enable=0;
    kanjiptr=0;
    rombank=0;

    // Copy KANJI ROM to RAM

    memcpy(kanjiram,kanjirom,0x20000);

    // Clear ROM Cart

    memset(ramcart,0,0x20000);

}

// Main thread (Core1)

void __not_in_flash_func(main_core1)(void) {

    volatile uint32_t bus;

    uint32_t control,address,data,response;

//    multicore_lockout_victim_init();

    gpio_init_mask(0xffffffff);
    gpio_set_dir_all_bits(0x00000000);  // All pins are INPUT

    while(1) {

        bus=gpio_get_all();

        // Check CS2# (GP26) & RD#

        if((bus&0x84000000)==0) {

            address=(bus&0x1fff00)>>8;

            // Set GP0-7 to OUTPUT
            
            gpio_set_dir_masked(0xff,0xff);

            data=ramcart[address+rombank*0x2000];

            // Set Data

            gpio_put_masked(0xff,data);

            // Wait while RD# is low

            while(control==0) {
                bus=gpio_get_all();
                control=bus&0x80000000;
            }

            // Set GP0-7 to INPUT

            gpio_set_dir_masked(0xff,0x00);

            continue;
        }

        // Check CS3# (GP27) & RD# (Senshi-RAM Read)

        if((bus&0x88000000)==0) {

            address=(bus&0x1fff00)>>8;

            // Set GP0-7 to OUTPUT
            
            gpio_set_dir_masked(0xff,0xff);

            data=senshiram[address];

            // Set Data

            gpio_put_masked(0xff,data);

            // Wait while RD# is low

            while(control==0) {
                bus=gpio_get_all();
                control=bus&0x80000000;
            }

            // Set GP0-7 to INPUT

            gpio_set_dir_masked(0xff,0x00);

            continue;
        }

        // Check MERQ# & WR# (Senshi-RAM Write)
        // Senshi-RAM is written by accessing from 0x6000 to 0x7fff for RAM. (Both MAIN and EXT RAM)

        if((bus&0x50000000)==0) {

            address=(bus&0xffff00)>>8;

            if((address>=0x6000)&&(address<0x8000)) {
                address&=0x1fff;
                senshiram[address]=bus&0xff;

            }

            // Wait while WR# is low

            while(control==0) {
                bus=gpio_get_all();
                control=bus&0x40000000;
//                control=bus&0x08000000;
            }

            continue;
        }

        control=bus&0xf0000000;

        // Check IO Read

        if(control==0x50000000) {

            address=(bus&0xffff00)>>8;

            switch(address&0xff) {

                case 0x77:
                    data=rompage;
                    response=1;
                    break;

                case 0xfd:  // Ext Kanji (Even)

                    if(kanji_enable) {
                        response=1;
                        data=kanjiram[kanjiptr<<1];
                        break;
                    } else {
                        response=1;
                        data=0xff;
                        break;
                    }

                case 0xfe:  // Ext Kanji (Odd)

                    if(kanji_enable) {
                        response=1;
                        data=kanjiram[(kanjiptr<<1)+1];
                        break;
                    } else {
                        response=1;
                        data=0xff;
                        break;
                    }   

                default:
                    response=0;

            }

            if(response) {
                // Set GP0-7 to OUTPUT

                gpio_set_dir_masked(0xff,0xff);

                gpio_put_masked(0xff,data);

                // Wait while RD# is low

                while(control==0) {
                    bus=gpio_get_all();
                    control=bus&0x80000000;
                }

                // Set GP0-7 to INPUT

                gpio_set_dir_masked(0xff,0x00);

            } else {

                // Wait while RD# is low

                while(control==0) {
                    bus=gpio_get_all();
                    control=bus&0x80000000;
                }

            }

            continue;
        }

        // Check IO Write

        if(control==0x90000000) {

            address=(bus&0xffff00)>>8;
            data=bus&0xff;

            io_write(address,data);

            // Wait while WR# is low
            while(control==0) {
                bus=gpio_get_all();
                control=bus&0x40000000;
            }

            continue;
        }
        
    }
}

int main() {

    uint32_t menuprint=0;
    uint32_t filelist=0;
    uint32_t subcpu_wait;
    uint32_t rampacno;
    uint32_t pacpage;

    static uint32_t hsync_wait,vsync_wait;

    vreg_set_voltage(VREG_VOLTAGE_1_20);  // for overclock to 300MHz
    set_sys_clock_khz(300000 ,true);

    multicore_launch_core1(main_core1);

    init_emulator();

    // Set RESET# interrupt

//    gpio_add_raw_irq_handler(25,z80reset);
//    gpio_set_irq_enabled(25,GPIO_IRQ_EDGE_FALL,true);

    gpio_set_irq_enabled_with_callback(25,GPIO_IRQ_EDGE_FALL,true,z80reset);

    while(1) {

        if(flash_command!=0) {

            switch(flash_command&0xf0000000) {

                case 0x10000000:
                    memcpy(ramcart,romcarts+rompage*0x20000,0x20000);
                    rombank=0;
                    break;
            }
            flash_command=0;

        }
    }
}

