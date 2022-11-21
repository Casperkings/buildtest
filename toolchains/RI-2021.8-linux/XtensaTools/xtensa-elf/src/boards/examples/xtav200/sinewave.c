/* Copyright (c) 2004-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

/*
This example outputs a sine wave to the audio output jack on the
XT-AV200-D daughterboard.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtensa/xtav200.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#ifndef XTBOARD_RAM_VADDR
int main(int argc, char *arv[])
{
  xtbsp_display_string("No system RAM");
  printf("With no system RAM configured, this example is too big.\n");
  return 0;
}
#elif !defined(AUDIO_INTNUM)
int main(int argc, char *arv[])
{
  xtbsp_display_string("No audio INT");
  printf("Insufficient interrupts for audio on this board.\n");
  return 0;
}
#else

/* I/O Device offsets */
#define SPI_START       *(volatile char*)(AUDIO_VADDR+0x00)
#define SPI_BUSY        *(volatile char*)(AUDIO_VADDR+0x04)
#define SPI_DATA        *(volatile int *)(AUDIO_VADDR+0x08)
#define I2S_START       *(volatile char*)(AUDIO_VADDR+0x0c)
#define I2S_DATA        *(volatile int *)(AUDIO_VADDR+0x10)
#define INT_REG         *(volatile int *)(AUDIO_VADDR+0x14)
#define INT_FIFOLEVEL   *(volatile int *)(AUDIO_VADDR+0x18)
#define NUM_FIFOENTRIES *(volatile int *)(AUDIO_VADDR+0x1c)

/* Codec register definitions */
#define AIC23_NUMREGS         10
#define AIC23_LEFTINVOL       0
#define AIC23_RIGHTINVOL      1
#define AIC23_LEFTHPVOL       2
#define AIC23_RIGHTHPVOL      3
#define AIC23_ANAPATH         4
#define AIC23_DIGPATH         5
#define AIC23_POWERDOWN       6
#define AIC23_DIGIF           7
#define AIC23_SAMPLERATE      8
#define AIC23_DIGACT          9
#define AIC23_RESET           15
#define AIC23_INPUT           1
#define AIC23_OUTPUT          2

/* Codec parameters */
#define FIFO_DEPTH_32BITS   2048
#define POOL_SIZE_32BITS    (8*FIFO_DEPTH_32BITS)
#define SAMPLERATE          48000

/* Static Data */
static int sw_upsample=0;
static int audio_channels = 2;
static int aic23_regs[AIC23_NUMREGS];
static int zero_buf[FIFO_DEPTH_32BITS];
static int stereo_buf[16*1024];         //worst case size assumed
static int upsample_buf[16*1024];       //worst case size assumed
static int volume_lookup_table [11] =
            { 48, 66, 80, 92, 102, 110, 116, 120, 123, 125, 127 };
static short sinebuf[] = {              // One sine curve
    0x0000, 0x0000, 0xb410, 0xb410, 0x2021, 0x2021, 0xfb30, 0xfb30,
    0xff3f, 0xff3f, 0xeb4d, 0xeb4d, 0x815a, 0x815a, 0x8b65, 0x8b65,
    0xd96e, 0xd96e, 0x4076, 0x4076, 0xa27b, 0xa27b, 0xe67e, 0xe67e,
    0xff7f, 0xff7f, 0xe67e, 0xe67e, 0xa27b, 0xa27b, 0x4076, 0x4076,
    0xd96e, 0xd96e, 0x8b65, 0x8b65, 0x815a, 0x815a, 0xeb4d, 0xeb4d,
    0xff3f, 0xff3f, 0xfb30, 0xfb30, 0x2021, 0x2021, 0xb410, 0xb410,
    0x0000, 0x0000, 0x4cef, 0x4cef, 0xe0de, 0xe0de, 0x05cf, 0x05cf,
    0x01c0, 0x01c0, 0x15b2, 0x15b2, 0x7fa5, 0x7fa5, 0x759a, 0x759a,
    0x2791, 0x2791, 0xc089, 0xc089, 0x5e84, 0x5e84, 0x1a81, 0x1a81,
    0x0180, 0x0180, 0x1a81, 0x1a81, 0x5e84, 0x5e84, 0xc089, 0xc089,
    0x2791, 0x2791, 0x759a, 0x759a, 0x7fa5, 0x7fa5, 0x15b2, 0x15b2,
    0x01c0, 0x01c0, 0x05cf, 0x05cf, 0xe0de, 0xe0de, 0x4cef, 0x4cef
};


/* Local Functions */

static void aic23_write_reg(int regno, int data)
{
    int dummy = 0;

    while(SPI_BUSY)
        dummy++;

    if (regno != AIC23_RESET)
    {
        aic23_regs[regno] = data;
    }

    data = (regno<<9)|data;
    SPI_DATA = data;    //write data
    SPI_START=1;

    while(SPI_BUSY)
        dummy++;

    SPI_START=0;
}

static void aic23_config_dac(void)
{
    SPI_START=0;

    memset((void*)aic23_regs, 0, sizeof(aic23_regs));
    aic23_write_reg(AIC23_RESET,      0x000); //reset AIC23
    aic23_write_reg(AIC23_LEFTINVOL,  0x017); //enabling left input
    aic23_write_reg(AIC23_RIGHTINVOL, 0x017); //enabling right input
    aic23_write_reg(AIC23_ANAPATH,    0x012); //analog audio path - enable DAC
    aic23_write_reg(AIC23_DIGPATH,    0x004); //digital audio path - default val
    aic23_write_reg(AIC23_POWERDOWN,  0x007); //power down control - enable clock
    aic23_write_reg(AIC23_DIGIF,      0x062); //digital audio interface format - master mode / LRSWAP
    aic23_write_reg(AIC23_SAMPLERATE, 0x023); //sample rate control - USB clock mode
    aic23_write_reg(AIC23_DIGACT,     0x001); //digital interface activation
}

//vol supported range 0-10
//actual value is 48 to 127 (79 steps)
static void aic23_set_hp_vol(int vol)
{
    aic23_write_reg(AIC23_LEFTHPVOL,    vol);
    aic23_write_reg(AIC23_RIGHTHPVOL,   vol);
}

static int aic23_get_hp_vol (void)
{
    int vol = aic23_regs[AIC23_LEFTHPVOL];
    return vol;
}

/* Registers setting for differnet sampling rates

 Freq    CKIN    SR3 SR2 SR1 SR0 BOSR    USBM    TOT

 48      0       0   0   0   0   0       1       0x001
 44.1    0       1   0   0   0   1       1       0x023
 32      0       0   1   1   0   0       1       0x019
 8       0       0   0   1   1   0       1       0x00D

 24      1       0   0   0   0   0       1       0x041
 22.05   1       1   0   0   0   1       1       0x063
 16      1       0   1   1   0   0       1       0x059

 Refer to the TLV320AIC23 part manual for details.
*/

static int aic23_set_freq(int freq)
{
    int retval=1;

    sw_upsample = 0;
    switch (freq)
    {
        case 48000:
        aic23_write_reg(AIC23_SAMPLERATE,   0x001);
        break;

        case 44100:
        aic23_write_reg(AIC23_SAMPLERATE,   0x023);
        break;

        case 32000:
        aic23_write_reg(AIC23_SAMPLERATE,   0x019);
        break;

        case 8000:
        aic23_write_reg(AIC23_SAMPLERATE,   0x00D);
        break;

        //i/p clk div by 2
        case 24000:
        aic23_write_reg(AIC23_SAMPLERATE,   0x041);
        break;

        //i/p clk div by 2
        case 22050:
        aic23_write_reg(AIC23_SAMPLERATE,   0x063);
        break;

        //i/p clk div by 2
        case 16000:
        aic23_write_reg(AIC23_SAMPLERATE,   0x059);
        break;

        // SW upsample by 2
        case 12000:
        sw_upsample = 1;
        aic23_write_reg(AIC23_SAMPLERATE,   0x041);
        break;

        // SW upsample by 2
        case 11025:
        sw_upsample = 1;
        aic23_write_reg(AIC23_SAMPLERATE,   0x063);
        break;

        default:
        retval = -1;
        break;
    }

    return retval;
}

static void init_i2s_fifo(void)
{
    int i;

    memset((void*)zero_buf, 0, sizeof(zero_buf));
    _xtos_ints_off(AUDIO_INTMASK);

    //stop i2s logic
    I2S_START=0;

    //clear any pending interrupts
    //clear lower 2 bits (fifo exchange and underrun)
    xthal_set_intclear(AUDIO_INTMASK);

    //refill both the fifos with (dummy) data
    for (i=0; i<FIFO_DEPTH_32BITS; i++)
    {
        I2S_DATA = 0;
        I2S_DATA = 0;
        I2S_DATA = 0;
        I2S_DATA = 0;
    }

    //start i2s logic
    I2S_START=1;

#pragma flush

    _xtos_ints_on(AUDIO_INTMASK);
}

static void i2s_fifo_urun_handler(void)
{
    int temp = INT_REG;
    if (temp&1)
    {
      I2S_START=0;
      INT_REG = 0;
      init_i2s_fifo();
    }
}

static void init_i2s_fifo_logic(void)
{
    _xtos_set_interrupt_handler_arg(AUDIO_INTNUM,
        i2s_fifo_urun_handler, 0);//underrun

    xthal_set_intclear(AUDIO_INTMASK);
    _xtos_ints_on(AUDIO_INTMASK);

    init_i2s_fifo();
}

static int hifi_audio_mute(int mute)
{
    static int mute_status = 0;
    if (mute_status == mute)
        return 0;
    static int prev_vol;
    mute_status = mute;
    if (mute)
    {
        prev_vol = aic23_get_hp_vol();
        aic23_set_hp_vol(0);
    }
    else
    {
        aic23_set_hp_vol(prev_vol);
    }
    return 0;
}

static int upsample(int *out_buf, int *inp_buf, int len_32bits)
{
    int i;

    for (i=0; i<len_32bits; i++)
    {
        out_buf[2*i] = inp_buf[i];
        out_buf[(2*i)+1] = inp_buf[i];
    }
    return len_32bits*2;
}

static int mono_to_stereo(int *out_buf, int *inp_buf, int len_32bits)
{
    int i;
    short *mono_buf = (short*)inp_buf;
    short *stero_buf = (short*)out_buf;

    for (i=0; i<len_32bits*2; i++)
    {
        stero_buf[2*i] = mono_buf[i];   //fill only Left channel
        stero_buf[(2*i)+1] = 0;
    }

    return len_32bits*2;
}

#if 0
static int hifi_audio_halt(int halt)
{
    static int halt_status = 0;
    if (halt && !halt_status)
    {
        _xtos_ints_off(AUDIO_INTMASK);
    }
    else
    {
        _xtos_ints_on(AUDIO_INTMASK);
    }
    halt_status = halt;
    return 0;
}
#endif

static int hifi_audio_write(char *data, int size, int len_32bits)
{
    int *out_buf = (int *)(data);
    int out_buf_len = len_32bits / sizeof (int);
    int i;
    short int *play_buf;

    if (1 == sw_upsample)
    {
        out_buf_len = upsample(upsample_buf, out_buf, out_buf_len);
        out_buf = upsample_buf;
    }

    if (1 == audio_channels)
    {
        //fill only Left channel
        out_buf_len = mono_to_stereo(stereo_buf, out_buf, out_buf_len);
        out_buf = stereo_buf;
    }

    play_buf = (short int *) out_buf;
    for (i = 0; i < out_buf_len; i += 1)
    {
        I2S_DATA = *play_buf++;
        I2S_DATA = *play_buf++;
    }

    return 0;
}

static int hifi_audio_set_freq (int freq)
{
    return aic23_set_freq (freq);
}

static int hifi_audio_set_vol (int vol_level)  //level from 0 to 10
{
    if (vol_level > 10)
        vol_level = 10;
    else if (vol_level < 0)
        vol_level = 0;

    vol_level = volume_lookup_table [vol_level];
    aic23_set_hp_vol (vol_level);

    return 0;
}

static int hifi_audio_open(void)
{
    aic23_config_dac(); //config DAC for playout

    return 0;
}

static int hifi_audio_init (int freq, int channels)
{
    audio_channels = channels;

    if (-1 == hifi_audio_set_freq (freq))
        return -1;

    hifi_audio_mute (1);
    init_i2s_fifo_logic();
    hifi_audio_mute (0);

    return 0;
}

#if 0
int hifi_audio_stop(void)
{
    _xtos_ints_off(AUDIO_INTMASK);
    xthal_set_intclear(AUDIO_INTMASK);
    I2S_START = 0;

    return 0;
}
#endif


/* Main Program */

int main (int argc, char *argv[])
{
    static short buf[SAMPLERATE];
    int i;
    int outsz =  sizeof (buf) /  2;

    hifi_audio_open();
    hifi_audio_init (SAMPLERATE, 2);
    hifi_audio_set_vol (5);
    
    /* Fill the buffer buf with sine waves. */
    for (i = 0; i < outsz; i++)
        buf[i] = sinebuf[i % (sizeof (sinebuf) / 2)];

    #ifdef XSHAL_IOBLOCK_BYPASS_VADDR
    xthal_set_region_attribute(
        (void*)XSHAL_IOBLOCK_BYPASS_VADDR,
        XSHAL_IOBLOCK_BYPASS_SIZE,
        XCHAL_CA_BYPASS,
        0
        );
    #endif
    #ifdef XTBOARD_ROM_VADDR
    xthal_set_region_attribute(
        (void*)XTBOARD_ROM_VADDR,
        XTBOARD_ROM_SIZE,
        XCHAL_CA_BYPASS,
        0
        );
    #endif

    for (i =    0; i < 1000; i++)
    {
        hifi_audio_write ((char*) buf, 0, outsz);
    }

    return 0;
}

#endif // XTBOARD_RAM_VADDR

