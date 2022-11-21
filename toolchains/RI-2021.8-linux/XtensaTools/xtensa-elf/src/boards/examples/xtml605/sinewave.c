/* Copyright (c) 2004-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

/*
This example outputs a sine wave to the audio output jack on the
XT-ML605-D daughterboard.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtensa/xtml605.h>

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
#elif !defined(AUDIO_I2S_OUT_INTNUM)
int main(int argc, char *arv[])
{
  xtbsp_display_string("No audio INT");
  printf("Insufficient interrupts for audio on this board.\n");
  return 0;
}
#else

/* I/O Device offsets */
/* SPI */
#define SPI_START       *(volatile char*)(SPI_VADDR+0x00)
#define SPI_BUSY        *(volatile char*)(SPI_VADDR+0x04)
#define SPI_DATA        *(volatile int *)(SPI_VADDR+0x08)

/* I2S Transmitter */
#define I2S_BASE         AUDIO_I2S_OUT_VADDR
#define I2S_VERSION     *(volatile int*)(I2S_BASE)
#define I2S_TXCONFIG    *(volatile int*)(I2S_BASE + 4)
#define I2S_INTMASK     *(volatile int*)(I2S_BASE + 8)
#define I2S_INTSTAT     *(volatile int*)(I2S_BASE + 12)
#define I2S_TXCHANNEL0  *(volatile int*)(I2S_BASE + 16)
#define I2S_TXCHANNEL1  *(volatile int*)(I2S_BASE + 20)
#define I2S_TXCHANNEL2  *(volatile int*)(I2S_BASE + 24)
#define I2S_TXCHANNEL3  *(volatile int*)(I2S_BASE + 28)

#define TXCONFIG_CHAN_EN            28
#define TXCONFIG_FIFO_LEVEL_INT     24
#define TXCONFIG_RES                16
#define TXCONFIG_RATIO              8
#define TXCONFIG_TSWAP              2
#define TXCONFIG_TINTEN             1
#define TXCONFIG_TXEN               0

/* I2C Master */
#define I2C_BASE            I2C_VADDR
#define rPRERlo             (*(volatile unsigned *)I2C_BASE)                 
#define rPRERhi             (*(volatile unsigned *)(I2C_BASE + 4))
#define rCTR                (*(volatile unsigned *)(I2C_BASE + 8))
#define rTXR                (*(volatile unsigned *)(I2C_BASE + 12))
#define rRXR                (*(volatile unsigned *)(I2C_BASE + 12))
#define rCR                 (*(volatile unsigned *)(I2C_BASE + 16))
#define rSR                 (*(volatile unsigned *)(I2C_BASE + 16))

/* Clock synthesizer */
/* I2C address */
#define CLK_SYN_I2C_ADDR                    0xD2
/* Internal registers */
#define CLK_SYN_BASE_OSCILLATOR_M_REG       0x84
#define CLK_SYN_BASE_OSCILLATOR_N_L_REG     0x85
#define CLK_SYN_BASE_OSCILLATOR_N_H_REG     0x86


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
#define BIT_RESOLUTION      16

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
    aic23_write_reg(AIC23_DIGIF,      0x022); //digital aud interface format - slave mode / LRSWAP
    aic23_write_reg(AIC23_SAMPLERATE, 0x000); //sample rate control - clock mode normal
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

/* Registers setting for differnet sampling rates */

static int aic23_set_freq(int freq)
{
    int retval=1;

    sw_upsample = 0;
    switch (freq)
    {
        case 48000:
        aic23_write_reg(AIC23_SAMPLERATE,   0x000);
        break;

        case 44100:
        aic23_write_reg(AIC23_SAMPLERATE,   0x020);
        break;

        default:
        retval = -1;
        break;
    }

    return retval;
}

static int hifi_audio_aic_mute(int mute)
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

/* I2C functions -- Start -- */

void i2c_init(unsigned int busfreq, unsigned int i2cfreq)
{
  unsigned int divisor;
  char upper,lower;

  divisor = (busfreq / (5 * i2cfreq)) - 1;
  lower = divisor & 0xff;
  upper = (divisor & 0xff00) >> 8;

  rPRERlo = lower;
  rPRERhi = upper;

  rCTR = 0x80;
}
    
void i2c_write_addr (char addr, unsigned int read){

  addr = (addr & 0xfe) | (read);
  rTXR = addr;
  rCR = 0x90;

  // Wait until transfer complete
  while (rSR & 0x2){}
  // Wait for ACK
  while (rSR & 0x80){}
  
}

void i2c_write_data (char data){

  rTXR = data;
  rCR = 0x10;
 
  // Wait until transfer complete
  while (rSR & 0x2){}
  // Wait for ACK
  while (rSR & 0x80){}
}

char i2c_read_data(){
  rCR = 0x28;

  // Wait until transfer complete
  while (rSR & 0x2){}
  return (rRXR);
}

void i2c_stop(){
  rCR = 0x40;

  // Wait until stopped
  while (rSR & 0x40){}  
}

/* I2C functions -- End -- */

/* Clock Synthesizer functions -- Start -- */
void writeClkSynReg (char addr, char data){

  i2c_write_addr (CLK_SYN_I2C_ADDR,0x0);
  i2c_write_data (addr);
  i2c_write_data (data);
  i2c_stop();  
}

int clk_syn_init(int aud_samp_freq)
{
    int ref_divider_M;
    int fb_divider_N_L;
    int fb_divider_N_H;
    int bus_freq = 50000000;
    int i2c_freq = 100000;

    /* Initialize */
    i2c_init (bus_freq, i2c_freq);  
    
    /* Audio clock setup */ 
    switch(aud_samp_freq) {
        case 48000: /* 48000 sampling freq => 12.288MHz Audio clock */
            ref_divider_M = 0xE1;
            fb_divider_N_L = 0x00;
            fb_divider_N_H = 0xE8;
            break;
        case 44100: /* 44100 sampling freq => 11.2896MHz Audio clock */
            ref_divider_M = 0x77;
            fb_divider_N_L = 0x20;
            fb_divider_N_H = 0xED;
            break;
        default:
            printf("Audio sampling frequncy not supported!\n");
            return -1;
    }    

    /* Set reference divider M */
    writeClkSynReg(CLK_SYN_BASE_OSCILLATOR_M_REG,
                      ref_divider_M);

    /* Set feedback divider N lower bits */
    writeClkSynReg(CLK_SYN_BASE_OSCILLATOR_N_L_REG,
                      fb_divider_N_L);

    /* Set feedback divider N higher bits */
    writeClkSynReg(CLK_SYN_BASE_OSCILLATOR_N_H_REG,
                  fb_divider_N_H);

    return 0;
}
/* Clock Synthesizer functions -- End   -- */

/* I2S functions -- Start -- */

static void i2s_tx_init(unsigned char resolution, unsigned int sample_freq, unsigned int bus_freq, int fifoIntLevel)
{
  unsigned int ratio;
  unsigned int config;
  int intlevel = fifoIntLevel & 0xf;

  ratio = (bus_freq - (sample_freq * resolution * 8)) / (sample_freq * resolution * 4);
  config = I2S_TXCONFIG;

  I2S_TXCONFIG = 
    (config & 0xf0c000ff) |
    intlevel << TXCONFIG_FIFO_LEVEL_INT |
    resolution << TXCONFIG_RES |
    ratio << TXCONFIG_RATIO;  

}

static void i2s_tx_start (unsigned char channels)
{
  unsigned int config = 
    (I2S_TXCONFIG & 0x0fffffff) |
    channels << TXCONFIG_CHAN_EN;
  
  I2S_TXCONFIG = config | 0x1;
}

static void i2s_tx_stop ()
{
  unsigned int config = I2S_TXCONFIG;
  I2S_TXCONFIG = config & 0xfffffffe;
}

static void init_i2s_fifo(void)
{
    int i;

    memset((void*)zero_buf, 0, sizeof(zero_buf));
    _xtos_ints_off(AUDIO_I2S_OUT_INTMASK);

    //stop i2s logic
    i2s_tx_stop();

    //clear any pending interrupts
    //clear lower 2 bits (fifo exchange and underrun)
    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);

    //refill both the fifos with (dummy) data
    for (i=0; i<FIFO_DEPTH_32BITS; i++)
    {
        I2S_TXCHANNEL0 = 0;
        I2S_TXCHANNEL0 = 0;
        I2S_TXCHANNEL0 = 0;
        I2S_TXCHANNEL0 = 0;
    }

    //start i2s logic
    i2s_tx_start(0x1);

#pragma flush

    _xtos_ints_on(AUDIO_I2S_OUT_INTMASK);
}

static void i2s_fifo_urun_handler(void)
{
    int temp = I2S_INTSTAT;
    if (temp&1)
    {
      i2s_tx_stop();
      I2S_INTSTAT = 0;
      init_i2s_fifo();
    }
}

static void init_i2s_fifo_logic(void)
{
    _xtos_set_interrupt_handler_arg(AUDIO_I2S_OUT_INTNUM,
        i2s_fifo_urun_handler, 0);//underrun

    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);
    _xtos_ints_on(AUDIO_I2S_OUT_INTMASK);

    init_i2s_fifo();
}

/* I2S functions -- End -- */

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
static int hifi_audio_aic_halt(int halt)
{
    static int halt_status = 0;
    if (halt && !halt_status)
    {
        _xtos_ints_off(AUDIO_I2S_OUT_INTMASK);
    }
    else
    {
        _xtos_ints_on(AUDIO_I2S_OUT_INTMASK);
    }
    halt_status = halt;
    return 0;
}
#endif

static int hifi_audio_aic_write(char *data, int size, int len_32bits)
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
        I2S_TXCHANNEL0 = *play_buf++;
        I2S_TXCHANNEL0 = *play_buf++;
    }

    return 0;
}

static int hifi_audio_aic_set_freq (int freq)
{
    return aic23_set_freq (freq);
}

static int hifi_audio_aic_set_vol (int vol_level)  //level from 0 to 10
{
    if (vol_level > 10)
        vol_level = 10;
    else if (vol_level < 0)
        vol_level = 0;

    vol_level = volume_lookup_table [vol_level];
    aic23_set_hp_vol (vol_level);

    return 0;
}

static int hifi_audio_aic_open(void)
{
    aic23_config_dac(); //config DAC for playout

    return 0;
}

static int hifi_audio_aic_init (int freq, int channels)
{
    int fifo_int_level_shift = 1;   
    int bit_res = BIT_RESOLUTION;

    audio_channels = channels;
    clk_syn_init(freq);

    if (-1 == hifi_audio_aic_set_freq (freq))
        return -1;

    hifi_audio_aic_mute (1);
    i2s_tx_init(bit_res, freq, freq*256, fifo_int_level_shift);
    init_i2s_fifo_logic();
    hifi_audio_aic_mute (0);

    return 0;
}

#if 0
int hifi_audio_aic_stop(void)
{
    _xtos_ints_off(AUDIO_I2S_OUT_INTMASK);
    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);
    i2s_tx_stop();

    return 0;
}
#endif


/* Main Program */

int main (int argc, char *argv[])
{
    static short buf[SAMPLERATE];
    int i;
    int outsz =  sizeof (buf) /  2;

    hifi_audio_aic_open();
    hifi_audio_aic_init (SAMPLERATE, 2);
    hifi_audio_aic_set_vol (5);
    
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
        hifi_audio_aic_write ((char*) buf, 0, outsz * 2);
    }

    return 0;
}

#endif // XTBOARD_RAM_VADDR

