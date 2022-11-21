/* Copyright (c) 2004-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

/*
This example outputs a sine wave to the audio output jack on the
XT-AV110-D daughterboard.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xtensa/xtav110.h>

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
/* Clock synthesizer Internal registers */
#define CLK_SYN_BASE_OSCILLATOR_M_REG       0x84
#define CLK_SYN_BASE_OSCILLATOR_N_L_REG     0x85
#define CLK_SYN_BASE_OSCILLATOR_N_H_REG     0x86

/* HDMI AD9889B */
//I2C Addr
#define HDMI_ADDR                   0x7a //PD pulled high

//General Control
#define HDMI_SYSTEM_PD              0x41
#define HDMI_PD_POL                 0x42
#define HDMI_MASK1                  0x94
#define HDMI_MASK2                  0x95
#define HDMI_HPD_INT                0x96

//HDMI DVI Selection
#define HDMI_DVI_SELECT             0xaf
#define HDMI_MODE                   0xc6

//TMDS Power down
#define HDMI_TMDSPOWERDOWN          0xa1

//System Monitoring
#define HDMI_HDCP_ERR_INT           0x97
#define HDMI_AUDIO_DETECTION        0xb8
#define HDMI_HDCP_CONTROLLER_ERROR  0xc8

//Reserved Registers that must be set properly!!
#define HDMI_RSVD0                  0x0a //set to 0x0
#define HDMI_RSVD1                  0x98 //set to 0x03
#define HDMI_RSVD2                  0x9c //set to 0x38
#define HDMI_RSVD3                  0x9d //set to 0x61
#define HDMI_DIFFDATA_DRIVE_STRENGTH 0xa2 //set to 0x84
#define HDMI_DIFFCLK_DRIVE_STRENGTH 0xa3 //set to 0x84
#define HDMI_RSVD4                  0xbb //set to 0xff

//Video Setup
#define HDMI_VFE_INPUT_ID           0x15
#define HDMI_VFE_422_WIDTH          0x16

//ReadOnly
#define HDMI_VFE_FMT_VID            0x3e
#define HDMI_VFE_AUX_VID            0x3f

//Pixel Repeat Related Registers
#define HDMI_EXT_AUDIOSF_SEL        0x3b
#define HDMI_EXT_VID_TO_RX          0x3c
#define HDMI_PR_TO_RX               0x3d

//DE HSYNC/VSYNC Registers
#define HDMI_SYNC_POLARITY          0x17
#define HDMI_VFE_HS_PLA_MSB         0x30
#define HDMI_VFE_HS_PLA             0x31
#define HDMI_VFE_HS_DUR             0x32
#define HDMI_VFE_VS_PLA             0x33
#define HDMI_VFE_VS_DUR             0x34
#define HDMI_VFE_HSDELAYIN_MSB      0x35
#define HDMI_VFE_HSDELAYIN          0x36
#define HDMI_VFE_INTERLACE_OFFSET   0x37
#define HDMI_VFE_WIDTH              0x38
#define HDMI_VFE_HEIGHT_MSB         0x39
#define HDMI_VFE_HEIGHT             0x3a
#define HDMI_COLOR_SPACE_CONV       0x3b

// Info Frames Setup
#define HDMI_AVIIF_PKT              0x44
#define HDMI_Y1Y0                   0x45
#define HDMI_SCAN_INFO              0x46
#define HDMI_ACTIVE_FORMAT_ASPECT_RATIO 0x47

//Audio Setup
#define HDMI_N_19_16                0x01
#define HDMI_N_15_8                 0x02
#define HDMI_N_7_0                  0x03
#define HDMI_SPDIF_SF               0x04
#define HDMI_CTS_INT_15_8           0x05
#define HDMI_CTS_INT_7_0            0x06
#define HDMI_CTS_EXT_19_16          0x07
#define HDMI_CTS_EXT_15_8           0x08
#define HDMI_CTS_EXT_7_0            0x09
#define HDMI_CTS_SEL                0x0a
#define HDMI_MCLK_POL               0x0b
#define HDMI_I2S_ENABLE             0x0c
#define HDMI_I2S_BIT_WIDTH          0x0d
#define HDMI_COPYRIGHT              0x12
#define HDMI_AUDIO_WORD_LENGTH      0x14
#define HDMI_AUDIO_IF_CC            0x50
#define HDMI_SPEAKER_MAPPING        0x51

/* Codec register definitions */
/* Codec parameters */
#define FIFO_DEPTH_32BITS   2048
#define POOL_SIZE_32BITS    (8*FIFO_DEPTH_32BITS)
#define SAMPLERATE          48000
#define BIT_RESOLUTION      32

/* Static Data */
static int sw_upsample=0;
static int audio_channels = 2;
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
static int hdmi_audio_channels = 6;

/* I2C functions -- Start -- */

static void i2c_init(unsigned int busfreq, unsigned int i2cfreq)
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
    
static void i2c_write_addr (char addr, unsigned int read){

  addr = (addr & 0xfe) | (read);
  rTXR = addr;
  rCR = 0x90;

  // Wait until transfer complete
  while (rSR & 0x2){}
  // Wait for ACK
  while (rSR & 0x80){}
  
}

static void i2c_write_data (char data){

  rTXR = data;
  rCR = 0x10;
 
  // Wait until transfer complete
  while (rSR & 0x2){}
  // Wait for ACK
  while (rSR & 0x80){}
}

static char i2c_read_data(){
  rCR = 0x28;

  // Wait until transfer complete
  while (rSR & 0x2){}
  return (rRXR);
}

static void i2c_stop(){
  rCR = 0x40;

  // Wait until stopped
  while (rSR & 0x40){}  
}

/* I2C functions -- End -- */

/* Clock Synthesizer functions -- Start -- */
static void writeClkSynReg (char addr, char data){

  i2c_write_addr (CLK_SYN_I2C_ADDR,0x0);
  i2c_write_data (addr);
  i2c_write_data (data);
  i2c_stop();  
}

static int clk_syn_init(int aud_samp_freq)
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

void i2s_tx_write0 (unsigned int data)
{
  I2S_TXCHANNEL0 = data; 
}

void i2s_tx_write1 (unsigned int data)
{
  I2S_TXCHANNEL1 = data; 
}

void i2s_tx_write2 (unsigned int data)
{
  I2S_TXCHANNEL2 = data; 
}

void i2s_tx_write3 (unsigned int data)
{
  I2S_TXCHANNEL3 = data; 
}

static void padAudioStart(int numSamples)
{
  int i;

  /* Pad silence at the start */

  for (i = 0; i < numSamples; i++){
    if(hdmi_audio_channels ==  2) {
        i2s_tx_write0(0);
        i2s_tx_write0(0);
    }
    else { /* 6 Channels */
        i2s_tx_write0(0);
        i2s_tx_write0(0);
        i2s_tx_write1(0);
        i2s_tx_write1(0);
        i2s_tx_write2(0);
        i2s_tx_write2(0);
    }
  }
}

static void init_i2s_fifo(void)
{
    int i;
    int i2s_start_val = hdmi_audio_channels == 6 ? 0x7 : 0x1;

    _xtos_ints_off(AUDIO_I2S_OUT_INTMASK);

    //stop i2s logic
    i2s_tx_stop();

    //clear any pending interrupts
    //clear lower 2 bits (fifo exchange and underrun)
    xthal_set_intclear(AUDIO_I2S_OUT_INTMASK);

    padAudioStart(FIFO_DEPTH_32BITS);

    //start i2s logic
    i2s_tx_start(i2s_start_val);

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

static char readHdmiReg (char addr){
  char data;

  i2c_write_addr (HDMI_ADDR,0x0);
  i2c_write_data (addr);
  i2c_write_addr (HDMI_ADDR,0x1);
  data = i2c_read_data();
  i2c_stop();
  return data;
}

static void writeHdmiReg (char addr, char data){

  i2c_write_addr (HDMI_ADDR,0x0);
  i2c_write_data (addr);
  i2c_write_data (data);
  i2c_stop();  
}

static void writeHdmiMult (char addr, char *data, int numBytes){
  int i;
  
  for (i=0;i<numBytes;i++){
    writeHdmiReg (addr+i,*data);
    data++;
  }

}

static void readHdmiMult (char addr, char *data, int numBytes){
  int i;
  
  for (i=0;i<numBytes;i++){
    *data = readHdmiReg (addr+i);
    data++;
  }

}

static void hdmi_powerdown(){

  writeHdmiReg (HDMI_SYSTEM_PD, 0x00);
  readHdmiReg (HDMI_SYSTEM_PD);

}
static void hdmi_init(){
  writeHdmiReg (HDMI_SYSTEM_PD, 0x10);
  readHdmiReg (HDMI_SYSTEM_PD);
  writeHdmiReg (HDMI_RSVD0, 0x0);
  readHdmiReg (HDMI_RSVD0);
  writeHdmiReg (HDMI_RSVD1, 0x03);
  readHdmiReg (HDMI_RSVD1);
  writeHdmiReg (HDMI_RSVD2, 0x38);
  readHdmiReg (HDMI_RSVD2);

  writeHdmiReg (HDMI_RSVD3, 0x61);
  readHdmiReg (HDMI_RSVD3);

  writeHdmiReg (HDMI_DIFFDATA_DRIVE_STRENGTH, 0x84);
  readHdmiReg (HDMI_DIFFDATA_DRIVE_STRENGTH);
  writeHdmiReg (HDMI_DIFFCLK_DRIVE_STRENGTH, 0x84);
  readHdmiReg (HDMI_DIFFCLK_DRIVE_STRENGTH);
  writeHdmiReg (HDMI_RSVD4, 0xff);
  readHdmiReg (HDMI_RSVD4);
  writeHdmiReg (HDMI_ACTIVE_FORMAT_ASPECT_RATIO, 0x80);
  readHdmiReg (HDMI_ACTIVE_FORMAT_ASPECT_RATIO);  

  //HDMI DVI Selection
  writeHdmiReg (HDMI_DVI_SELECT, 0x06);
  readHdmiReg (HDMI_DVI_SELECT);  
  readHdmiReg (HDMI_MODE);  

}

static void hdmi_video_init()
{
  char rgb_rgb[24] = {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0};  
  char data[24];

  writeHdmiMult(0x18,rgb_rgb,24);
  readHdmiMult(0x18,data,24);

  // HDMI INFO frames
  writeHdmiReg (HDMI_Y1Y0, 0x80);
  readHdmiReg (HDMI_Y1Y0);  

  writeHdmiReg (HDMI_SCAN_INFO, 0x14);
  readHdmiReg (HDMI_SCAN_INFO);  
  
  // Video Setup
  writeHdmiReg (HDMI_EXT_VID_TO_RX, 0x01);

  writeHdmiReg (HDMI_VFE_INPUT_ID, 0x0a);
  readHdmiReg (HDMI_VFE_INPUT_ID);

  writeHdmiReg(HDMI_VFE_422_WIDTH, 0x2);
  readHdmiReg (HDMI_VFE_422_WIDTH);

  writeHdmiReg(HDMI_SYNC_POLARITY, 0x68);
  readHdmiReg(HDMI_SYNC_POLARITY);

  readHdmiReg(HDMI_COLOR_SPACE_CONV);
  writeHdmiReg(HDMI_COLOR_SPACE_CONV, 0x80);
  readHdmiReg(HDMI_COLOR_SPACE_CONV);

}

static void hdmi_audio_init(int freq, int channels, int resolution)
{
  int fifo_int_level_shift = 1;

  switch (freq)
    {
    case 48000:
      printf("Set N Value to 1800\n");
      writeHdmiReg (HDMI_N_19_16, 0);
      readHdmiReg (HDMI_N_19_16);
      
      writeHdmiReg (HDMI_N_15_8, 0x18);
      readHdmiReg (HDMI_N_15_8);
      
      writeHdmiReg (HDMI_N_7_0, 0x00);
      readHdmiReg (HDMI_N_7_0);      

      writeHdmiReg (HDMI_VFE_INPUT_ID, 0x2a);
      readHdmiReg (HDMI_VFE_INPUT_ID);
      break;
      
    case 44100:
      printf("Set N Value to 1880\n");
      writeHdmiReg (HDMI_N_19_16, 0);
      readHdmiReg (HDMI_N_19_16);
      
      writeHdmiReg (HDMI_N_15_8, 0x18);
      readHdmiReg (HDMI_N_15_8);
      
      writeHdmiReg (HDMI_N_7_0, 0x80);
      readHdmiReg (HDMI_N_7_0);      

      printf("Set External CTS Value to 0x6c81\n");
      writeHdmiReg (HDMI_CTS_EXT_19_16, 0x00);
      readHdmiReg (HDMI_CTS_EXT_19_16);
      
      writeHdmiReg (HDMI_CTS_EXT_15_8, 0x6c);
      readHdmiReg (HDMI_CTS_EXT_15_8);
      
      writeHdmiReg (HDMI_CTS_EXT_7_0, 0x81);
      readHdmiReg (HDMI_CTS_EXT_7_0);

      break;
      
    case 32000:
      printf("32k not supported yet!\n");
      break;
    }


  switch (channels)
    {
    case 2:
      writeHdmiReg (HDMI_I2S_ENABLE, 0x04);
      readHdmiReg (HDMI_I2S_ENABLE);
      writeHdmiReg (HDMI_AUDIO_IF_CC, 0x20);
      readHdmiReg (HDMI_AUDIO_IF_CC);
      writeHdmiReg (HDMI_SPEAKER_MAPPING, 0x0);
      readHdmiReg (HDMI_SPEAKER_MAPPING);
      break;
      
    case 6:
      writeHdmiReg (HDMI_I2S_ENABLE, 0x1c);
      readHdmiReg (HDMI_I2S_ENABLE);
      writeHdmiReg (HDMI_AUDIO_IF_CC, 0xa0);
      readHdmiReg (HDMI_AUDIO_IF_CC);
      writeHdmiReg (HDMI_SPEAKER_MAPPING, 0xb);
      readHdmiReg (HDMI_SPEAKER_MAPPING);

      break;
      
    default:
      printf("%d not supported yet!\n",channels);
      break;
    }


  readHdmiReg (HDMI_CTS_SEL);
  writeHdmiReg (HDMI_CTS_SEL, 0x01);
  readHdmiReg (HDMI_CTS_SEL);

  writeHdmiReg (HDMI_MCLK_POL, 0x0e);
  readHdmiReg (HDMI_MCLK_POL);

  writeHdmiReg (HDMI_COPYRIGHT, 0x20);
  readHdmiReg (HDMI_COPYRIGHT);

  switch (resolution){
  case 16:
    writeHdmiReg (HDMI_AUDIO_WORD_LENGTH, 0x2);
    readHdmiReg (HDMI_AUDIO_WORD_LENGTH);    
    break;
  case 24:
    writeHdmiReg (HDMI_AUDIO_WORD_LENGTH, 0xb);
    readHdmiReg (HDMI_AUDIO_WORD_LENGTH);    
    break;
  default:
    printf("Unknown resolution\n");
    break;
  }
}

#if 0
int hifi_audio_hdmi_stop()
{
  while(!(i2s_get_intstatus() & 0x1));    
  _xtos_ints_off(AUDIO_I2S_OUT_MASK);
  xthal_set_intclear(AUDIO_I2S_OUT_MASK);
  i2s_tx_stop();
  
  return 0;
}
#endif

int hifi_audio_hdmi_open()
{
  i2c_init (50000000, 100000);
  hdmi_powerdown();
  hdmi_init();
  hdmi_video_init();
  return 0;
}

int hifi_audio_hdmi_init(int freq, int channels)   
{
  int fifo_int_level_shift = 1;
  int bit_res = BIT_RESOLUTION;

  hdmi_audio_channels = channels;
  
  clk_syn_init(freq);
  hdmi_audio_init(freq,channels,24);
  
  i2s_tx_init(bit_res, freq, freq*256, fifo_int_level_shift);
  init_i2s_fifo_logic();
  
  return 0;
}

int hifi_audio_hdmi_write(char *data, int size, int length)
{
  int *DataBuffer = (int *)(data);
  int out_buf_len = length * size / sizeof (int);
  int i,frameoffset;
  int left_sample, right_sample;
  
  int* play_buf = (int *) data;
  for (i = 0; i < out_buf_len; i += hdmi_audio_channels){
    if(hdmi_audio_channels ==  2) {
        left_sample  = *play_buf++;
        i2s_tx_write0(left_sample << 8);
        right_sample = *play_buf++;
        i2s_tx_write0(right_sample << 8);
    }
    else { /* 6 Channels */
        i2s_tx_write0((play_buf[0])<<8);
        i2s_tx_write0((play_buf[2])<<8);
        i2s_tx_write1((play_buf[5])<<8);
        i2s_tx_write1((play_buf[1])<<8);
        i2s_tx_write2((play_buf[3])<<8);
        i2s_tx_write2((play_buf[4])<<8);
        play_buf += 6;
    }
  }

  return 0;
}

/* Main Program */

int main (int argc, char *argv[])
{
    static int buf[SAMPLERATE];
    int i;
    int outsz =  sizeof (buf) / sizeof(int);

    hifi_audio_hdmi_open();
    hifi_audio_hdmi_init (SAMPLERATE, 6);
    
    /* Fill the buffer buf with sine waves. */
    for (i = 0; i < outsz; i++)
        buf[i] = sinebuf[i % (sizeof (sinebuf) / 2)] << 8; //Convert to 24 bit

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
        hifi_audio_hdmi_write ((char*) buf, 1, outsz * sizeof(int));
    }

    return 0;
}

#endif // XTBOARD_RAM_VADDR

