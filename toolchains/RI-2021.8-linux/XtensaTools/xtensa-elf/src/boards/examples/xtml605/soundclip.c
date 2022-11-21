/*********************************************************************************/
/*                                                                               */
/* Copyright (c) 2009-2011 by Tensilica Inc.  ALL RIGHTS RESERVED.               */
/* These coded instructions, statements, and computer programs are the           */
/* copyrighted works and confidential proprietary information of Tensilica Inc.  */
/* They may not be modified, copied, reproduced, distributed, or disclosed to    */
/* third parties in any manner, medium, or form, in whole or in part, without    */
/* the prior written consent of Tensilica Inc.                                   */
/*                                                                               */
/*********************************************************************************/

/*
This example records a short sound clip using the microphone jack and then 
outputs the recording to the audio output jack on the XT-ML605-D daughterboard.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <xtensa/xtml605.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#if !defined(XTBOARD_RAM_VADDR) || (XTBOARD_RAM_SIZE < 0x280000)
int main(int argc, char *arv[])
{
  xtbsp_display_string("SysRAM too small");
  printf("This example requires at least 2.5 MB of system RAM.\n");
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

/* I/O Device Offsets and Other Info */
/* SPI */
#define SPI_START       *(volatile char*)(SPI_VADDR+0x00)
#define SPI_BUSY        *(volatile char*)(SPI_VADDR+0x04)
#define SPI_DATA        *(volatile int *)(SPI_VADDR+0x08)

/* I2S Transmitter */
#define I2S_TXBASE         AUDIO_I2S_OUT_VADDR
#define I2S_TXVERSION      *(volatile int*)(I2S_TXBASE)
#define I2S_TXCONFIG       *(volatile int*)(I2S_TXBASE + 4)
#define I2S_TXINTMASK      *(volatile int*)(I2S_TXBASE + 8)
#define I2S_TXINTSTAT      *(volatile int*)(I2S_TXBASE + 12)
#define I2S_TXCHANNEL0     *(volatile int*)(I2S_TXBASE + 16)
#define I2S_TXDATA         I2S_TXCHANNEL0 

#define TXCONFIG_CHAN_EN            28
#define TXCONFIG_FIFO_LEVEL_INT     24
#define TXCONFIG_RES                16
#define TXCONFIG_RATIO              8
#define TXCONFIG_TSWAP              2
#define TXCONFIG_TINTEN             1
#define TXCONFIG_TXEN               0

/* I2S Receiver */
#define I2S_RXBASE      AUDIO_I2S_IN_VADDR
#define I2S_RXVERSION   *(volatile int*)(I2S_RXBASE)
#define I2S_RXCONFIG    *(volatile int*)(I2S_RXBASE + 4)
#define I2S_RX_INTMASK  *(volatile int*)(I2S_RXBASE + 8)
#define I2S_RX_INTSTAT  *(volatile int*)(I2S_RXBASE + 12)
#define I2S_RXCHANNEL0  *(volatile int*)(I2S_RXBASE + 16)

#define RXCONFIG_FIFO_LEVEL_INT 24
#define RXCONFIG_RES            16
#define RXCONFIG_TSWAP          2
#define RXCONFIG_TINTEN         1
#define RXCONFIG_TXEN           0


/* Codec Register Definitions */
#define AIC23_NUMREGS           10
#define AIC23_LEFTINVOL         0
#define AIC23_RIGHTINVOL        1
#define AIC23_LEFTHPVOL         2
#define AIC23_RIGHTHPVOL        3
#define AIC23_ANAPATH           4
#define AIC23_DIGPATH           5
#define AIC23_POWERDOWN         6
#define AIC23_DIGIF             7
#define AIC23_SAMPLERATE        8
#define AIC23_DIGACT            9
#define AIC23_RESET             15
#define AIC23_INPUT             1
#define AIC23_OUTPUT            2

/* Codec Parameters */
#define CLIPNUMSAMPLES          0x100000
#define I2S_TX_FIFO_DEPTH       2048
#define SAMPLERATE              48000

/* Static Data */
static int sw_upsample=0;
static int audio_channels = 2;
static int aic23_regs[AIC23_NUMREGS];
static int stereo_buf[16*1024];
static int upsample_buf[16*1024];

static short           destination[CLIPNUMSAMPLES];
static volatile int    samplesRecorded = 0;

static int volume_lookup_table [11] =
{
    48, 66, 80, 92, 102, 110, 116, 120, 123, 125, 127
};


//////////////////
// Local Functions
//////////////////

static int aic23_write_reg(int regno, int data)
{
    int dummy = 0, i, temp;

    while(SPI_BUSY)
        dummy++;

    if (regno != AIC23_RESET)
    {
        aic23_regs[regno] = data;
    }

    data = (regno<<9)|data;
    SPI_DATA = data;    //write data
    SPI_START=1;

    for (i=0; i<50; i++);   //wait for write to complete

    while(SPI_BUSY)
        dummy++;

    SPI_START=0;
}

static void aic23_config_dac(void)
{
    SPI_START=0;

    memset(aic23_regs, 0, sizeof(aic23_regs));
    aic23_write_reg(AIC23_RESET,      0x000); //reset AIC23
    aic23_write_reg(AIC23_LEFTINVOL,  0x017); //enabling left input
    aic23_write_reg(AIC23_RIGHTINVOL, 0x017); //enabling right input
    aic23_write_reg(AIC23_ANAPATH,    0x015); //analog audio path - enable DAC
    aic23_write_reg(AIC23_DIGPATH,    0x004); //digital audio path - default val
    aic23_write_reg(AIC23_POWERDOWN,  0x001); //power down control - enable clock
    aic23_write_reg(AIC23_DIGIF,      0x022); //digital aud interface format - slave mode / LRSWAP   
    aic23_write_reg(AIC23_SAMPLERATE, 0x000); //sample rate control - Clock mode normal
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


/////////////////////////
// I2S Receiver Functions
/////////////////////////

static unsigned int i2s_rx_init(unsigned char resolution, int fifoIntLevel)
{
    int intlevel = fifoIntLevel & 0xf;
    unsigned int config = I2S_RXCONFIG;

    I2S_RXCONFIG = 
        (config & 0x00c000ff) |
        intlevel << RXCONFIG_FIFO_LEVEL_INT |
        resolution << RXCONFIG_RES;  

    return I2S_RXCONFIG;
}
        
static unsigned int i2s_rx_read0(void)
{
    return(I2S_RXCHANNEL0);
}

static void i2s_rx_start(void)
{
    I2S_RXCONFIG |= 0x1;
}

static void i2s_rx_chswap(char swap)
{
    if (swap)
        I2S_RXCONFIG = I2S_RXCONFIG | 0x4;
    else
        I2S_RXCONFIG = I2S_RXCONFIG & 0xfffffffb;
   
}

static void i2s_rx_stop (void)
{
    unsigned int config = I2S_RXCONFIG;
    I2S_RXCONFIG = config & 0xfffffffe;
}


/////////////////////////
// I2S Output Functions
/////////////////////////

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

    _xtos_ints_off(AUDIO_I2S_OUT_INTMASK);

    //stop i2s logic
    i2s_tx_stop();
    
    //clear any pending interrupts
    //clear lower 2 bits (fifo exchange and underrun)
    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);

    //refill both the fifos with (dummy) data
    for (i=0; i<I2S_TX_FIFO_DEPTH; i++)
    {
        I2S_TXDATA = 0;
        I2S_TXDATA = 0;
        I2S_TXDATA = 0;
        I2S_TXDATA = 0;
    }

#pragma flush

    _xtos_ints_on(AUDIO_I2S_OUT_INTMASK);
}

static void i2s_fifo_urun_handler(void)
{
    int temp = I2S_TXINTSTAT;
    if (temp&1)
    {
      i2s_tx_start(0);
      I2S_TXINTSTAT = 0;
      init_i2s_fifo();
    }
}

static void init_i2s_fifo_logic(void)
{
    _xtos_set_interrupt_handler_arg(AUDIO_I2S_OUT_INTNUM,
        i2s_fifo_urun_handler, 0);//underrun

    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);

    init_i2s_fifo();
}


/////////////////////
// API Functions
////////////////////

int hifi_audio_aic_mute(int mute)
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

int upsample(int *out_buf, int *inp_buf, int len_32bits)
{
    int i;

    for (i=0; i<len_32bits; i++)
    {
        out_buf[2*i] = inp_buf[i];
        out_buf[(2*i)+1] = inp_buf[i];
    }
    return len_32bits*2;
}

int mono_to_stereo(int *out_buf, int *inp_buf, int len_32bits)
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

int hifi_audio_aic_halt(int halt)
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

int hifi_audio_aic_write(char *data, int len_32bits)
{
    int *out_buf = (int *)(data);
    int out_buf_len = len_32bits / sizeof (unsigned int);
    int i;
    short int *play_buf;
    short temp;

    if (1 == sw_upsample)
    {
        out_buf_len = upsample(upsample_buf, out_buf, out_buf_len);
        out_buf = upsample_buf;
    }

    if (1 == audio_channels)
    {
        out_buf_len = mono_to_stereo(stereo_buf, out_buf, out_buf_len); //fill only Left channel
        out_buf = stereo_buf;
    }

    play_buf = (short int *)out_buf;
    for (i = 0; i < out_buf_len; i += 1)
    {
        I2S_TXDATA = *play_buf++ << 16;
        I2S_TXDATA = *play_buf++ << 16;
    }

    return 0;
}

int hifi_audio_aic_set_freq (int freq)
{
    return aic23_set_freq (freq);
}


int hifi_audio_aic_set_vol (int vol_level)  //level from 0 to 10
{
    if (vol_level > 10)
        vol_level = 10;
    else if (vol_level < 0)
        vol_level = 0;

    vol_level = volume_lookup_table [vol_level];
    aic23_set_hp_vol (vol_level);

    return 0;
}

int hifi_audio_aic_open(void)
{
    aic23_config_dac(); //config DAC for playout

    return 0;
}

int hifi_audio_aic_init (int freq, int channels)
{
    int fifo_int_level_shift = 1;   
    int tx_bit_res = 32;
    int rx_bit_res = 16;

    audio_channels = channels;

    if (-1 == hifi_audio_aic_set_freq (freq))
        return -1;

    hifi_audio_aic_mute (1);
    i2s_tx_init(tx_bit_res, freq, freq*256, fifo_int_level_shift);
    init_i2s_fifo_logic();
    hifi_audio_aic_mute (0);
    i2s_rx_init(rx_bit_res,1);// 16 bit data, read fifo interrupt set at 8192>>1 = 4096 entries

    return 0;
    
}

int hifi_audio_aic_stop(void)
{
    _xtos_ints_off(AUDIO_I2S_OUT_INTMASK);
    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);
    i2s_tx_start(0);

    return 0;
}

int hifi_audio_aic_start(void)
{
    //start i2s logic
    i2s_tx_start(1);
    return 0;
}

int hifi_audio_mic_start(void)
{
    i2s_rx_start();
}

int hifi_audio_mic_stop(void)
{
    _xtos_ints_off(AUDIO_I2S_IN_INTMASK);
    xthal_set_intclear(AUDIO_I2S_IN_INTMASK);
    i2s_rx_stop();
    
    return 0;
}

void hifi_rx_intenable (char enable)
{
    if (enable)
        I2S_RXCONFIG = I2S_RXCONFIG | 0x2;
    else
        I2S_RXCONFIG = I2S_RXCONFIG & 0xfffffffd;
}

void hifi_rx_write_intmask (unsigned int mask)
{
    I2S_RX_INTMASK = mask;
}

unsigned int hifi_rx_clearint (unsigned int intnum)
{
    I2S_RX_INTSTAT = I2S_RX_INTSTAT & (1<<intnum);
    return (I2S_RX_INTSTAT);
}

unsigned int hifi_rx_get_intstatus (void)
{
    return (I2S_RX_INTSTAT);
}

int hifi_rx_read(char *dest, int numSamples)
{
    int i;
    short *sample;
    unsigned int rxdata;

    sample = (short *)dest;

    for (i=0;i<numSamples;i++)
    {
        rxdata = I2S_RXCHANNEL0;        
        *sample++ = rxdata;
    }
    return 0;
}

void init_i2s_adc_int(void)
{
    _xtos_ints_off(AUDIO_I2S_IN_INTMASK);
    hifi_rx_clearint(0);
    hifi_rx_clearint(1);
    // Enable half full buffer interrupt
    hifi_rx_write_intmask (0x2);
    hifi_rx_intenable(1);
#pragma flush
    xthal_set_intclear(AUDIO_I2S_IN_INTMASK);
    _xtos_ints_on(AUDIO_I2S_IN_INTMASK);
}

static void i2s_adc_handler(void)
{
    int i;

    _xtos_ints_off(AUDIO_I2S_IN_INTMASK);

    hifi_rx_read((char *)(destination+samplesRecorded),4096);
    samplesRecorded += 4096;
    hifi_rx_clearint(0);
    hifi_rx_clearint(1);

    printf (".");
    fflush(stdout);
#pragma flush
    xthal_set_intclear(AUDIO_I2S_IN_INTMASK);
    _xtos_ints_on(AUDIO_I2S_IN_INTMASK);
}

int main(int argc, char *arv[])
{
    int i;
    char ans;

    xthal_set_region_attribute((char *)0x50000000,0x10000000,0x2,0);
    xthal_set_region_attribute((char *)0x90000000,0x10000000,0x2,0);
    
    _xtos_set_interrupt_handler_arg(
        AUDIO_I2S_IN_INTNUM, i2s_adc_handler, 0); //receive fifo has 4096 entries
    
    hifi_audio_aic_open();
    hifi_audio_aic_init(SAMPLERATE, 2);
    init_i2s_adc_int();
    hifi_audio_aic_set_vol (5);

    printf("\n\n");
    printf("Welcome to the Tensilica ADC Recorder Test\n");
    printf("This application will record a short clip\n");
    printf("and then play it out on the headphone output\n");

    printf("\nHit 'Enter' to begin recording\n");
    scanf ("%c", &ans);
#pragma flush
    hifi_audio_aic_start();
    hifi_audio_mic_start();            

    printf ("\nRecording a short clip");
    fflush(stdout);
    while(1){
        if(samplesRecorded>=CLIPNUMSAMPLES){
            xthal_set_intclear(AUDIO_I2S_IN_INTMASK);
            _xtos_ints_off(AUDIO_I2S_IN_INTMASK);
            hifi_audio_mic_stop();                    
            break;
        }
    };

    printf("\n\nHit 'Enter' to begin playback\n");
    scanf ("%c", &ans);

    printf ("\nPlaying back the recorded clip\n");
    fflush(stdout);
    hifi_audio_aic_write((char *)destination, CLIPNUMSAMPLES << 1);
    hifi_audio_aic_stop();
    printf("\nPlayback Complete!\n");
}

#endif /* XTBOARD_RAM_VADDR */

