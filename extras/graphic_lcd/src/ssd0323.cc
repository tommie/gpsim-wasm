/*
   Copyright (C) 2007 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <config.h>
#ifdef HAVE_GUI

#define IN_MODULE

#include <cstdio>
#include <cstdlib>

#include "ssd0323.h"


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FUNCTION__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#define UNIT_TESTING 0

//========================================================================
//
// SSD0323 Solomon Systech Display controller
//
//  --  128X80, 16 Gray Scale Dot Matrix
//
//
// Questions
// 1) The command sequence involves writing multiple bytes. Under what
//    conditions is the state of this sequence modified?
//  -- Assume that each command write automatically advances the state
//  -- Assume that it is okay for CS and DC to change states between writes
//  -- Assume that a low on RES clears the state.
//  -- Assume that a data write before the command completes aborts the command.

//------------------------------------------------------------------------
//------------------------------------------------------------------------

#if UNIT_TESTING == 1
void unitTest(SSD0323 *pSSD0323);
#endif

SSD0323::SSD0323()
  : m_controlState(0),
    m_dataBus(0x100),
    m_commMode(SSD0323::eSPIMode),
    m_commState(0),
    m_SPIData(0),
    m_commandIndex(0),
    m_expectedCommandWords(0),
    m_colAddr(0), m_rowAddr(0),
    m_colStartAddr(0), m_colEndAddr(0x3f),
    m_rowStartAddr(0), m_rowEndAddr(0x4f),
    m_Remap(0), m_ContrastControl(0x40)
{
  randomizeRAM();
  cmdWords[0] = 0;
#if UNIT_TESTING == 1
  unitTest(this);
#endif
}


//------------------------------------------------------------------------
void SSD0323::abortCurrentTransaction()
{
  m_commState = 0;
  m_commandIndex = 0;
}


//------------------------------------------------------------------------
// setBS -- selects the communication interface:
//
//  BS   Mode
// 000   Serial
// 100   6800
// 110   8080

void SSD0323::setBS(unsigned int BSpin, bool newBS)
{
  //Dprintf(("pin:%d state:%d  currentBS state:%d\n",BSpin, newBS,m_commMode));
  if ((((1 << BSpin) & m_commMode) != 0) == newBS) {
    return;
  }

  m_commMode ^= (1 << BSpin);
  abortCurrentTransaction();
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
void SSD0323::setCS(bool newCS)
{
  if (bBitState(eCS) == newCS) {
    return;
  }

  m_controlState ^= eCS;
  //Dprintf(("state:%d\n",newCS));
  abortCurrentTransaction();
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
void SSD0323::setRES(bool newRES)
{
  if (bBitState(eRES) == newRES) {
    return;
  }

  m_controlState ^= eRES;
  Dprintf(("state:%d\n", newRES));
  abortCurrentTransaction();
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------

void SSD0323::setDC(bool newDC)
{
  if (bBitState(eDC) == newDC) {
    return;
  }

  m_controlState ^= eDC;
  //Dprintf(("state:%d\n",newDC));
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------

void SSD0323::setE_RD(bool newE_RD)
{
  if (bBitState(eE_RD) == newE_RD) {
    return;
  }

  m_controlState ^= eE_RD;
  Dprintf(("state:%d\n", newE_RD));

  if (!bEnabled()) {
    return;
  }

  switch (m_commMode) {
  case e8080Mode:

    // Take action on rising edge of RD#
    //Dprintf(("8080 mode\n"));
    if (newE_RD && bBitState(eRW)) {
      if (bBitState(eDC)) {
        driveDataBus(getData());
        advanceColumnAddress();

      } else {
        driveDataBus(getStatus());
      }
    }

    break;

  case e6800Mode:
    Dprintf(("6800 mode\n"));

    // Take action on the falling edge of E#
    if (!newE_RD) {
      if (bBitState(eRW) && !bBitState(eDC))
        // Read status
      {
        driveDataBus(getStatus());

      } else  if (!bBitState(eRW) && !bBitState(eDC))
        // Write command
      {
        executeCommand();

      } else  if (bBitState(eRW) && bBitState(eDC)) {
        // Read Data
        driveDataBus(getData());
        advanceColumnAddress();

      } else  if (!bBitState(eRW) && bBitState(eDC)) {
        // Write Data
        storeData();
      }
    }

    break;

  case eSPIMode:
    //Dprintf(("SPI mode\n"));
    break;

  default:
    break;
  }
}


//------------------------------------------------------------------------
void SSD0323::setRW(bool newRW)
{
  if (bBitState(eRW) == newRW) {
    return;
  }

  m_controlState ^= eRW;

  if (!bEnabled()) {
    return;
  }

  switch (m_commMode) {
  case e8080Mode:

    // Take action on rising edge or WR#
    //Dprintf(("8080 mode\n"));
    if (newRW && bBitState(eE_RD)) {
      if (bBitState(eDC)) {
        storeData();

      } else {
        executeCommand();
      }
    }

    break;

  case e6800Mode:
    //Dprintf(("6800 mode\n"));
    break;

  case eSPIMode:
    //Dprintf(("SPI mode\n"));
    break;

  default:
    break;
  }
}


const int bSCLK = 1;
const int bSDIN = 2;


void SSD0323::setData(unsigned int d)
{
  if (m_dataBus == d) {
    return;
  }

  if (m_commMode == eSPIMode  && bEnabled()) {
    if (~m_dataBus & d & bSCLK) {
      m_SPIData = (m_SPIData << 1) | (m_dataBus & bSDIN ? 1 : 0);
      Dprintf(("SSD SPI new bit. So far: 0x%02X\n", m_SPIData));

      if (++m_commState >= 8) {
        // Place the contents of the SPI receiver onto the
        // internal data bus and then perform the write operation.
        m_dataBus = m_SPIData;

        if (bBitState(eDC)) {
          storeData();

        } else {
          executeCommand();
        }

        m_commState = 0;
        m_SPIData = 0;
      }
    }
  }

  m_dataBus = d;
}


void SSD0323::setSCLK(bool bSCL)
{
  setData((m_dataBus & ~bSCLK) | (bSCL ? bSCLK : 0));
}


void SSD0323::setSDIN(bool bSDA)
{
  setData((m_dataBus & ~bSDIN) | (bSDA ? bSDIN : 0));
}


void SSD0323::driveDataBus(unsigned int d)
{
  m_dataBus = d;
}


void SSD0323::advanceColumnAddress()
{
  if (++m_colAddr <= m_colEndAddr) {
    return;
  }

  m_colAddr = m_colStartAddr;

  if (m_rowStartAddr != m_rowEndAddr) {
    advanceRowAddress();
  }
}


void SSD0323::advanceRowAddress()
{
  if (++m_rowAddr <= m_rowEndAddr) {
    return;
  }

  m_rowAddr = m_rowStartAddr;

  if (m_colStartAddr != m_colEndAddr) {
    advanceColumnAddress();
  }
}


void SSD0323::storeData()
{
  m_ram[m_rowAddr * 64 + m_colAddr] = m_dataBus;
  Dprintf((" data:0x%02x  row:%u col:%u\n", m_dataBus, m_rowAddr, m_colAddr));
  const int incMode = 4;

  if (m_Remap & incMode) {
    advanceRowAddress();

  } else {
    advanceColumnAddress();
  }
}


unsigned int SSD0323::getData()
{
  m_dataBus = m_ram[m_rowAddr * 64 + m_colAddr];
  return m_dataBus;
}


unsigned int SSD0323::getStatus()
{
  return 0;
}


void SSD0323::executeCommand()
{
  const int CmdSetColumnAddress = 0x15;
  const int CmdGraphicAccleration = 0x23;
  const int CmdDrawRectangle = 0x24;
  const int CmdCopy = 0x25;
  const int CmdHorizontalScroll = 0x26;
  const int CmdStopMoving = 0x2E;
  const int CmdStartMoving = 0x2F;
  const int CmdSetRowAddress = 0x75;
  const int CmdSetContrast = 0x81;
  const int CmdSetQuarterCurrent = 0x84;
  const int CmdSetHalfCurrent = 0x85;
  const int CmdSetFullCurrent = 0x86;
  const int CmdSetRemap = 0xA0;
  const int CmdSetDisplayStartLine = 0xA1;
  const int CmdSetDisplayOffset = 0xA2;
  const int CmdSetNormalDisplay = 0xA4;
  const int CmdSetAllOn = 0xA5;
  const int CmdSetAllOff = 0xA6;
  const int CmdSetInverse = 0xA7;
  const int CmdSetMultiplexRatio = 0xA8;
  const int CmdSetMasterCfg = 0xAD;
  const int CmdSetDisplayOff = 0xAE;
  const int CmdSetDisplayOn = 0xAF;
  const int CmdSetPreChargeCompensationEnable = 0xB0;
  const int CmdSetPhaseLength = 0xB1;
  const int CmdSetRowPeriod = 0xB2;
  const int CmdSetClockDivide = 0xB3;
  const int CmdSetPreChargeCompensationLevel = 0xB4;
  const int CmdSetGrayScaleTable = 0xB8;
  const int CmdSetPreChargeVoltage = 0xBC;
  const int CmdSetVCOMH = 0xBE;
  const int CmdSetVSL = 0xBF;
  const int CmdNop = 0xE3;
  cmdWords[m_commandIndex] = m_dataBus;
  m_commandIndex = (m_commandIndex + 1) & 0xf;

  if (m_commandIndex > sizeof(cmdWords)) {
    m_commandIndex = 0;
    printf("Warning: SSD0323::executeCommand() - command buffer overflow\n");
    return;
  }

  printf("%s:data=0x%x\n", __FUNCTION__, m_dataBus);

  // decode the command
  if (m_commandIndex == 1) {
    switch (m_dataBus) {
    // 1-word commands
    case CmdStopMoving:
    case CmdStartMoving:
    case CmdSetQuarterCurrent:
    case CmdSetHalfCurrent:
    case CmdSetFullCurrent:
    case CmdSetNormalDisplay:
    case CmdSetAllOn:
    case CmdSetAllOff:
    case CmdSetInverse:
    case CmdSetDisplayOff:
    case CmdSetDisplayOn:
    case CmdNop:
      m_commandIndex = 0;
      return;
      break;

    // 2-word commands
    case CmdGraphicAccleration:
    case CmdSetContrast:
    case CmdSetRemap:
    case CmdSetDisplayStartLine:
    case CmdSetDisplayOffset:
    case CmdSetMultiplexRatio:
    case CmdSetMasterCfg:
    case CmdSetPreChargeCompensationEnable:
    case CmdSetRowPeriod:
    case CmdSetPreChargeCompensationLevel:
    case CmdSetPreChargeVoltage:
    case CmdSetVCOMH:
    case CmdSetVSL:
    case CmdSetPhaseLength:
    case CmdSetClockDivide:
      m_expectedCommandWords = 2;
      break;

    // 3-word commands
    case CmdSetColumnAddress:
    case CmdSetRowAddress:
      m_expectedCommandWords = 3;
      break;

    case CmdHorizontalScroll:
      m_expectedCommandWords = 4;
      break;

    case CmdDrawRectangle:
      m_expectedCommandWords = 6;
      break;

    case CmdCopy:
      m_expectedCommandWords = 7;
      break;

    case CmdSetGrayScaleTable:
      m_expectedCommandWords = 9;
      break;

    default:
      printf("Warning: SSD received bad command 0x%x\n", m_dataBus);
    }
  }

  if (m_commandIndex == m_expectedCommandWords) {
    unsigned int i = cmdWords[0];
    printf("SSD0323 - executing command:0x%x\n", i);

    switch (cmdWords[0]) {
    // 1-word commands
    case CmdStopMoving:
    case CmdStartMoving:
    case CmdSetQuarterCurrent:
    case CmdSetHalfCurrent:
    case CmdSetFullCurrent:
    case CmdSetNormalDisplay:
    case CmdSetAllOn:
    case CmdSetAllOff:
    case CmdSetInverse:
    case CmdSetDisplayOff:
    case CmdSetDisplayOn:
    case CmdNop:
      break;

    case CmdSetContrast:
      m_ContrastControl = cmdWords[1] & 0x7f;
      break;

    case CmdSetRemap:
      m_Remap = cmdWords[1] & 0x7f;
      break;

    // 2-word commands
    case CmdGraphicAccleration:
    case CmdSetDisplayStartLine:
    case CmdSetDisplayOffset:
    case CmdSetMultiplexRatio:
    case CmdSetMasterCfg:
    case CmdSetPreChargeCompensationEnable:
    case CmdSetRowPeriod:
    case CmdSetPreChargeCompensationLevel:
    case CmdSetPreChargeVoltage:
    case CmdSetVCOMH:
    case CmdSetVSL:
    case CmdSetPhaseLength:
    case CmdSetClockDivide:
      m_expectedCommandWords = 2;
      break;

    // 3-word commands
    case CmdSetColumnAddress:
      m_colStartAddr = cmdWords[1] & 0x3f;
      m_colEndAddr = cmdWords[2] & 0x3f;
      m_colAddr = m_colStartAddr;
      break;

    case CmdSetRowAddress:
      m_rowStartAddr = cmdWords[1] & 0x7f;
      m_rowEndAddr = cmdWords[2] & 0x7f;
      m_rowAddr = m_rowStartAddr;
      break;

    case CmdHorizontalScroll:
      m_expectedCommandWords = 4;
      break;

    case CmdDrawRectangle:
      m_expectedCommandWords = 6;
      break;

    case CmdCopy:
      m_expectedCommandWords = 7;
      break;

    case CmdSetGrayScaleTable:
      m_expectedCommandWords = 16;
      break;

    default:
      printf("Warning: SSD received bad command 0x%x\n", m_dataBus);
    }

    m_commandIndex = 0;
  }
}


//------------------------------------------------------------------------
// dataBusDirection()
// return true if the controller is driving the data bus
bool SSD0323::dataBusDirection()
{
  if (!bEnabled()) {
    return false;
  }

  if (m_commMode == e8080Mode && !bBitState(eE_RD)) {
    return true;
  }

  if (m_commMode == e6800Mode && bBitState(eRW)) {
    return true;
  }

  return false;
}


//------------------------------------------------------------------------
unsigned int SSD0323::getDataBus()
{
  return m_dataBus;
}


//------------------------------------------------------------------------

void SSD0323::randomizeRAM()
{
  for (unsigned int i = 0; i < 128 * 80 / 2; i++) {
    m_ram[i] = (rand() >> 8) & 0xff;
  }
}


//------------------------------------------------------------------------
// prBadRam - private function called when ever an illegal access is made
// to the RAM
unsigned int &SSD0323::prBadRam(unsigned int index)
{
  printf("WARNING SSD0323 - illegal RAM access index=%u\n", index);
  return m_ram[(128 * 80 / 2)];
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------

void SSD0323::showState()
{
  printf("SSD0323 internal state:\n");

  switch (m_commMode) {
  case e8080Mode:
    printf(" 8080 mode\n");
    break;

  case e6800Mode:
    printf(" 6800 mode\n");
    break;

  case eSPIMode:
    printf(" SPI mode\n");
    break;
  }

  printf("remap: 0x%02x\n", m_Remap);
  printf("column start:0x%02x  end:0x%02x  curr:0x%02x\n", m_colStartAddr, m_colEndAddr, m_colAddr);
  printf("row start:0x%02x  end:0x%02x  curr:0x%02x\n", m_rowStartAddr, m_rowEndAddr, m_rowAddr);
}


#if UNIT_TESTING == 1

//========================================================================
static SSD0323 *p_gSSD0323 = nullptr;

void WriteCommand(unsigned int c)
{
  if (p_gSSD0323) {
    p_gSSD0323->setData(c);
    p_gSSD0323->executeCommand();
  }
}


void WriteData(unsigned int d)
{
  if (p_gSSD0323) {
    p_gSSD0323->setData(d);
    p_gSSD0323->storeData();
  }
}


void unitTest(SSD0323 *pSSD0323)
{
  printf("Running SSD0323 unit test\n");
  p_gSSD0323 = pSSD0323;
  // Column Address
  WriteCommand(0x15);    /* Set Column Address */
  WriteCommand(0x00);    /* Start = 0 */
  WriteCommand(0x3F);     /* End = 127 */
  // Row Address
  WriteCommand(0x75);    /* Set Row Address */
  WriteCommand(0x00);    /* Start = 0 */
  WriteCommand(0x3F);     /* End = 63 */
  // Contrast Control
  WriteCommand(0x81);    /* Set Contrast Control      */
  WriteCommand(0x6D);    /* 0 ~ 127 */
  // Current Range
  WriteCommand(0x86);    /* Set Current Range 84h:Quarter, 85h:Half, 86h:Full*/
  // Re-map
  WriteCommand(0xA0);    /* Set Re-map */
  WriteCommand(0x41);    /* [0]:MX, [1]:Nibble, [2]:H/V address [4]:MY, [6]:Com Split Odd/Even "1000010"*/
  // Display Start Line
  WriteCommand(0xA1);    /* Set Display Start Line */
  WriteCommand(0x00);    /* Start at row 0 */
  // Display Offset
  WriteCommand(0xA2);    /* Set Display Offset */
  WriteCommand(0x44);    /* Offset 68 rows */
  // Display Mode
  WriteCommand(0xA4);    /* Set DisplaMode,A4:Normal,     A5:All ON, A6: All OFF, A7:Inverse    */
  // Multiplex Ratio
  WriteCommand(0xA8);    /* Set Multiplex Ratio */
  WriteCommand(0x3F);    /* 64 mux*/
  // Phase Length
  WriteCommand(0xB1);    /* Set Phase Length */
  WriteCommand(0x22);    /* [3:0]:Phase 1 period of 1~16 clocks */
  /* [7:4]:Phase 2 period of 1~16 clocks // POR = 0111 0100 */
  // Set Pre-charge Compensation Enable
  WriteCommand(0xB0); /* Set Pre-charge Compensation Enable */
  WriteCommand(0x28); /* Enable*/
  // Set Pre-charge Compensation Level
  WriteCommand(0xB4); /* Set Pre-charge Compensation Level */
  WriteCommand(0x07); /* Higher level */
  // Row Period
  WriteCommand(0xB2); /* Set Row Period */
  WriteCommand(0x46); /* [7:0]:18~255, K=P1+P2+GS15 (POR:4+7+29)*/
  // Display Clock Divide
  WriteCommand(0xB3); /* Set Clock Divide (2) */
  WriteCommand(0x91); /* [3:0]:1~16, [7:4]:0~16, 100Hz */
  /* POR = 0000 0001 */
  // VSL
  WriteCommand(0xBF); /* Set VSL */
  WriteCommand(0x0D); /* [3:0]:VSL */
  // VCOMH
  WriteCommand(0xBE); /* Set VCOMH (3) */
  WriteCommand(0x02); /* [7:0]:VCOMH, (0.53 X Vref = 0.53 X 15 V = 7.95V)*/
  // VP
  /* Set VP (4) */
  WriteCommand(0xBC);
  WriteCommand(0x10);   /* [7:0]:VP, (0.67 X Vref = 0.67 X 15 V = 10.05V) */
  // Gamma
  WriteCommand(0xB8);    /* Set Gamma with next 8 bytes */
  WriteCommand(0x01);    /* L1[2:1] */
  WriteCommand(0x11);   /* L3[6:4], L2[2:0] 0001 0001 */
  WriteCommand(0x22);   /* L5[6:4], L4[2:0] 0010 0010 */
  WriteCommand(0x32);   /* L7[6:4], L6[2:0] 0011 1011 */
  WriteCommand(0x43);   /* L9[6:4], L8[2:0] 0100 0100 */
  WriteCommand(0x54);   /* LB[6:4], LA[2:0] 0101 0101 */
  WriteCommand(0x65);   /* LD[6:4], LC[2:0] 0110 0110 */
  WriteCommand(0x76);   /* LF[6:4], LE[2:0] 1000 0111 */
  // Set DC-DC
  WriteCommand(0xAD); /* Set DC-DC */
  WriteCommand(0x02); /* 03=ON, 02=Off */
  // Display ON/OFF
  WriteCommand(0xAF);          /* AF=ON, AE=Sleep Mode */
  //unsigned int i;
  //for(i=0; i<4; i++)
  WriteData(0xff);
  WriteData(0xff);
}

#endif // UNIT_TESTING
#endif // HAVE_GUI
