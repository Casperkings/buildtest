/*********************************************************************************/
/*                                                                               */
/* Copyright (c) 2009-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.               */
/* These coded instructions, statements, and computer programs are the           */
/* copyrighted works and confidential proprietary information of Tensilica Inc.  */
/* They may not be modified, copied, reproduced, distributed, or disclosed to    */
/* third parties in any manner, medium, or form, in whole or in part, without    */
/* the prior written consent of Tensilica Inc.                                   */
/*                                                                               */
/*********************************************************************************/

/*
This example records a short sond clip using the microphone jack and then 
outputs the recording to the audio output jack on the XT-AV200-D daughterboard.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <xtensa/xtav200.h>

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
#elif !defined(AUDIO_INTNUM)
int main(int argc, char *arv[])
{
  xtbsp_display_string("No audio INT");
  printf("Insufficient interrupts for audio on this board.\n");
  return 0;
}
#else

/* I/O Device Offsets and Other Info */
#define SPI_BASE        (XSHAL_IOBLOCK_BYPASS_PADDR) + 0x0d070000
#define SPI_START       *(volatile char*)(SPI_BASE)
#define SPI_BUSY        *(volatile char*)(SPI_BASE + 4)
#define SPI_DATA        *(volatile int*) (SPI_BASE + 8)
#define I2S_START       *(volatile char*)(SPI_BASE + 12)
#define I2S_DATA        *(volatile int*) (SPI_BASE + 16)
#define INT_REG         *(volatile int*) (SPI_BASE + 20)
#define INT_FIFOLEVEL   *(volatile int*) (SPI_BASE + 24)
#define NUM_FIFOENTRIES *(volatile int*) (SPI_BASE + 28)
#define I2S_RXBASE      (XSHAL_IOBLOCK_BYPASS_PADDR) + 0x0d088000
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
#define I2S_FIFO_DEPTH_32BITS   4096


/* Static Data */
static int sw_upsample=0;
static int audio_channels = 2;
static int aic23_regs[AIC23_NUMREGS];
static int stereo_buf[16*1024];
static int upsample_buf[16*1024];

static unsigned short  destination[CLIPNUMSAMPLES];
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

static void init_i2s_fifo(void)
{
    int i;

    _xtos_ints_off(AUDIO_INTMASK);

    //clear any pending interrupts
    //clear lower 2 bits (fifo exchange and underrun)
    xthal_set_intclear(AUDIO_INTMASK);

    //refill both the fifos with (dummy) data
    for (i=0; i<I2S_FIFO_DEPTH_32BITS; i++)
    {
        I2S_DATA = 0;
        I2S_DATA = 0;
        I2S_DATA = 0;
        I2S_DATA = 0;
    }

#pragma flush

    //  _xtos_ints_on(AUDIO_INTMASK);
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

    init_i2s_fifo();
}


/////////////////////
// API Functions
////////////////////

int hifi_audio_mute(int mute)
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

int hifi_audio_halt(int halt)
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

int hifi_audio_write(char *data, int len_32bits)
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
        I2S_DATA = *play_buf++;
        I2S_DATA = *play_buf++;
    }

    return 0;
}

int hifi_audio_set_freq (int freq)
{
    return aic23_set_freq (freq);
}


int hifi_audio_set_vol (int vol_level)  //level from 0 to 10
{
    if (vol_level > 10)
        vol_level = 10;
    else if (vol_level < 0)
        vol_level = 0;

    vol_level = volume_lookup_table [vol_level];
    aic23_set_hp_vol (vol_level);

    return 0;
}

int hifi_audio_open(void)
{
    aic23_config_dac(); //config DAC for playout

    return 0;
}

int hifi_audio_init (int freq, int channels)
{
    audio_channels = channels;

    if (-1 == hifi_audio_set_freq (freq))
        return -1;

    hifi_audio_mute (1);
    init_i2s_fifo_logic();
    hifi_audio_mute (0);
    i2s_rx_init(16,1);// 16 bit data, read fifo interrupt set at 8192>>1 = 4096 entries
}

int hifi_audio_stop(void)
{
    _xtos_ints_off(AUDIO_INTMASK);
    xthal_set_intclear(AUDIO_INTMASK);
    I2S_START = 0;

    return 0;
}

int hifi_audio_start(void)
{
    //start i2s logic
    I2S_START=1;
    return 0;
}

int hifi_mic_start(void)
{
    i2s_rx_start();
}

int hifi_mic_stop(void)
{
    _xtos_ints_off(AUDIO_ADCLVL_INTMASK);
    xthal_set_intclear(AUDIO_ADCLVL_INTMASK);
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
    unsigned short *sample;
    unsigned int rxdata;

    sample = (unsigned short *)dest;

    for (i=0;i<numSamples;i++)
    {
        rxdata = I2S_RXCHANNEL0;        
        *sample++ = rxdata;
    }
    return 0;
}

void init_i2s_adc_int(void)
{
    _xtos_ints_off(AUDIO_ADCLVL_INTMASK);
    hifi_rx_clearint(0);
    hifi_rx_clearint(1);
    // Enable half full buffer interrupt
    hifi_rx_write_intmask (0x2);
    hifi_rx_intenable(1);
#pragma flush
    xthal_set_intclear(AUDIO_ADCLVL_INTMASK);
    _xtos_ints_on(AUDIO_ADCLVL_INTMASK);
}

static void i2s_adc_handler(void)
{
    int i;

    _xtos_ints_off(AUDIO_ADCLVL_INTMASK);

    hifi_rx_read((char *)(destination+samplesRecorded),4096);
    samplesRecorded += 4096;
    hifi_rx_clearint(0);
    hifi_rx_clearint(1);

    printf (".");
    fflush(stdout);
#pragma flush
    xthal_set_intclear(AUDIO_ADCLVL_INTMASK);
    _xtos_ints_on(AUDIO_ADCLVL_INTMASK);
}

int main(int argc, char *arv[])
{
    int i;
    char ans;

    xthal_set_region_attribute((char *)0x50000000,0x10000000,0x2,0);
    xthal_set_region_attribute((char *)0x90000000,0x10000000,0x2,0);
    
    _xtos_set_interrupt_handler_arg(
        AUDIO_ADCLVL_INTNUM, i2s_adc_handler, 0); //fifo has 4096 entries
    
    hifi_audio_open();
    hifi_audio_init(44100, 2);
    init_i2s_adc_int();
    hifi_audio_set_vol (5);
    hifi_audio_start();

    printf("\n\n");
    printf("Welcome to the Tensilica ADC Recorder Test\n");
    printf("This application will record a short clip\n");
    printf("and then play it out on the headphone output\n");

    printf("\nHit 'Enter' to begin recording\n");
    scanf ("%c", &ans);
#pragma flush
    hifi_mic_start();            

    printf ("\nRecording a short clip");
    fflush(stdout);
    while(1){
        if (samplesRecorded>=CLIPNUMSAMPLES){
            xthal_set_intclear(AUDIO_ADCLVL_INTMASK);
            _xtos_ints_off(AUDIO_ADCLVL_INTMASK);
            hifi_mic_stop();                    
            break;
        }
    };

    printf("\n\nHit 'Enter' to begin playback\n");
    scanf ("%c", &ans);

    printf ("\nPlaying back the recorded clip\n");
    fflush(stdout);
    hifi_audio_start();
    hifi_audio_write((char *)destination, CLIPNUMSAMPLES << 1);
    hifi_audio_stop();
    printf("\nPlayback Complete!\n");
}

#endif /* XTBOARD_RAM_VADDR */

