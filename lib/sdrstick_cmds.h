/*
 * sdrstick_cmds.h
 *
 *  Created on: Jul 23, 2012
 *      Author: Charles Mesarosh
 *      Copyright 2012, 2013 Zephyr Engineering, Inc., All Rights Reserved
 */

#ifndef SDRSTICK_CMDS_H_
#define SDRSTICK_CMDS_H_

#define CMD_NOP			0x00000000	//generates a reply for testing
#define CMD_VERS		0x00000001	//returns firmware version
#define CMD_RUN 		0x00000002	//start radio, send udp data to client
#define CMD_STOP 		0x00000003	//stop radio and data to client
#define CMD_RSVD		0x00000004	//Reserved for future use
#define CMD_TESTDATA	0x00000005	//sends test data in place of radio data
#define CMD_SR			0x00000006	//set sample rate, command arg 0 = high rate, 1 = low rate
#define CMD_FREQ		0x00000007	//set nco frequency
#define CMD_PRODUCT		0x00000008	//request product code
#define CMD_SET_ATTN	0x00000009	//set attenuator value
#define CMD_SET_ADCCTRL 0x0000000a	//set adc control bits (dither and random)
#define CMD_PH_EN       0x0000000b  //enable headphone audio stream
#define CMD_PH_DIS      0x0000000c  //disable headphone audio stream
#define CMD_TX_ON		0x00001000	//TX2 transmitter on
#define CMD_TX_OFF		0x00001001	//TX2 transmitter off
#define CMD_TX_FREQ		0x00001002	//set TX2 transmitter frequency
#define CMD_TX_PWR		0x00001003	//set TX2 power level
#define CMD_TX_LOCCW	0x00001004	//set TX2 local cw mode
#define CMD_TX_MICEN    0x00001005  //enable TX2 Mic Audio stream
#define CMD_TX_MICDIS   0x00001006  //disable TX2 Mic Audio stream

#endif /* SDRSTICK_CMDS_H_ */
