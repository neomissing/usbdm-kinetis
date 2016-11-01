/*! \file
    \brief Simple USB Stack for Kinetis

   \verbatim
    Kinetis USB Code

    Copyright (C) 2008-16  Peter O'Donoghue

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    \endverbatim

\verbatim
Change History
+=================================================================================
| 20 Oct 2016 | Updated USB stack to new version
| 15 Jul 2013 | Changed serial number to use chip UID
| 04 Sep 2011 | Ported to MK20DX128
+=================================================================================
\endverbatim
 */
#include <string.h>
#include <stdio.h>
#include <usb_cdc.h>
#include "derivative.h"
#include "Configure.h"
#include "Commands.h"
#include "usb_defs.h"
#include "usb.h"
#include "utilities.h"
#include "usb_implementation.h"

namespace USBDM {

/** BDTs organised by endpoint, odd/even, tx/rx */
EndpointBdtEntry endPointBdts[Usb0::NUMBER_OF_ENDPOINTS] __attribute__ ((aligned (512)));

#ifdef MS_COMPATIBLE_ID_FEATURE

#if WCHAR_MAX != 65535
#error "Wide chars must be 16-bits for this file (use -fshort-wchar option)"
#endif

#define DeviceInterfaceGUIDs L"DeviceInterfaceGUIDs"
#define DeviceGUID           L"{93FEBD51-6000-4E7E-A20E-A80FC78C7EA1}\0"
#define ICONS                L"Icons"
#define ICON_PATH            L"\%SystemRoot\%\\system32\\SHELL32.dll,-4\0"

// See https://github.com/pbatard/libwdi/wiki/WCID-Devices
//
const MS_CompatibleIdFeatureDescriptor msCompatibleIdFeatureDescriptor = {
      /* lLength;             */  CONST_NATIVE_TO_LE32((uint32_t)sizeof(MS_CompatibleIdFeatureDescriptor)),
      /* wVersion;            */  nativeToLe16(0x0100),
      /* wIndex;              */  nativeToLe16(0x0004),
      /* bnumSections;        */  1,
      /*---------------------- Section 1 -----------------------------*/
      /* bReserved1[7];       */  {0},
      /* bInterfaceNum;       */  0,
      /* bReserved2;          */  1,
      /* bCompatibleId[8];    */  "WINUSB\0",
      /* bSubCompatibleId[8]; */  {0},
      /* bReserved3[6];       */  {0}
};

struct MS_PropertiesFeatureDescriptor {
   uint32_t lLength;                //!< Size of this Descriptor in Bytes
   uint16_t wVersion;               //!< Version
   uint16_t wIndex;                 //!< Index (must be 5)
   uint16_t bnumSections;           //!< Number of property sections
   /*---------------------- Section 1 -----------------------------*/
   uint32_t lPropertySize0;          //!< Size of property section
   uint32_t ldataType0;              //!< Data type (1 = Unicode REG_SZ etc
   uint16_t wNameLength0;            //!< Length of property name
   wchar_t  bName0[sizeof(DeviceInterfaceGUIDs)/sizeof(wchar_t)];
   uint32_t wPropertyLength0;        //!< Length of property data
   wchar_t  bData0[sizeof(DeviceGUID)/sizeof(wchar_t)];
   /*---------------------- Section 2 -----------------------------*/
   uint32_t lPropertySize1;          //!< Size of property section
   uint32_t ldataType1;              //!< Data type (1 = Unicode REG_SZ etc
   uint16_t wNameLength1;            //!< Length of property name
   wchar_t  bName1[sizeof(ICONS)/sizeof(wchar_t)];
   uint32_t wPropertyLength1;        //!< Length of property data
   wchar_t  bData1[sizeof(ICON_PATH)/sizeof(wchar_t)];
};

static const MS_PropertiesFeatureDescriptor msPropertiesFeatureDescriptor = {
      /* U32 lLength;         */ CONST_NATIVE_TO_LE32((uint32_t)sizeof(MS_PropertiesFeatureDescriptor)),
      /* U16 wVersion;        */ nativeToLe16(0x0100),
      /* U16 wIndex;          */ nativeToLe16(5),
      /* U16 bnumSections;    */ nativeToLe16(2),
      /*---------------------- Section 1 -----------------------------*/
      /* U32 lPropertySize0;  */ CONST_NATIVE_TO_LE32(
            sizeof(msPropertiesFeatureDescriptor.lPropertySize0)+
            sizeof(msPropertiesFeatureDescriptor.ldataType0)+
            sizeof(msPropertiesFeatureDescriptor.wNameLength0)+
            sizeof(msPropertiesFeatureDescriptor.bName0)+
            sizeof(msPropertiesFeatureDescriptor.wPropertyLength0)+
            sizeof(msPropertiesFeatureDescriptor.bData0)
      ),
      /* U32 ldataType0;       */ CONST_NATIVE_TO_LE32(7UL), // 7 == REG_MULTI_SZ
      /* U16 wNameLength0;     */ nativeToLe16(sizeof(msPropertiesFeatureDescriptor.bName0)),
      /* U8  bName0[42];       */ DeviceInterfaceGUIDs,
      /* U32 wPropertyLength0; */ CONST_NATIVE_TO_LE32(sizeof(msPropertiesFeatureDescriptor.bData0)),
      /* U8  bData0[78];       */ DeviceGUID,
      /*---------------------- Section 2 -----------------------------*/
      /* U32 lPropertySize1;   */ CONST_NATIVE_TO_LE32(
            sizeof(msPropertiesFeatureDescriptor.lPropertySize1)+
            sizeof(msPropertiesFeatureDescriptor.ldataType1)+
            sizeof(msPropertiesFeatureDescriptor.wNameLength1)+
            sizeof(msPropertiesFeatureDescriptor.bName1)+
            sizeof(msPropertiesFeatureDescriptor.wPropertyLength1)+
            sizeof(msPropertiesFeatureDescriptor.bData1)
      ),
      /* U32 ldataType1;       */ CONST_NATIVE_TO_LE32(7UL), // 7 == REG_MULTI_SZ
      /* U16 wNameLength1;     */ nativeToLe16(sizeof(msPropertiesFeatureDescriptor.bName1)),
      /* U8  bName1[];         */ ICONS,
      /* U32 wPropertyLength1; */ CONST_NATIVE_TO_LE32(sizeof(msPropertiesFeatureDescriptor.bData1)),
      /* U8  bData1[];         */ ICON_PATH,
};

#define VENDOR_CODE 0x30
static const uint8_t osStringDescriptor[] = {18, DT_STRING, 'M',0,'S',0,'F',0,'T',0,'1',0,'0',0,'0',0,VENDOR_CODE,0x00};

#endif

/**
 * Get name of USB token
 *
 * @param  token USB token
 *
 * @return Pointer to static string
 */
const char *getTokenName(unsigned token) {
   static const char *names[] = {
         "Unknown #0",
         "OUTToken",   //  (0x1) - Out token
         "ACKToken",   //  (0x2) - Acknowledge
         "DATA0Token", //  (0x3) - Data 0
         "Unknown #4",
         "SOFToken",   //  (0x5) - Start of Frame token
         "NYETToken",  //  (0x6) - No Response Yet
         "DATA2Token", //  (0x7) - Data 2
         "Unknown #8",
         "INToken",    //  (0x9) - In token
         "NAKToken",   //  (0xA) - Negative Acknowledge
         "DATA1Token", //  (0xB) - Data 1
         "PREToken",   //  (0xC) - Preamble
         "SETUPToken", //  (0xD) - Setup token
         "STALLToken", //  (0xE) - Stall
         "MDATAToken", //  (0xF) - M data
   };
   const char *rc = "Unknown";
   if (token<(sizeof(names)/sizeof(names[0]))) {
      rc = names[token];
   }
   return rc;
}

/**
 * Get name of USB state
 *
 * @param  state USB state
 *
 * @return Pointer to static string
 */
const char *getStateName(EndpointState state){
   switch (state) {
      default         : return "Unknown";
      case EPIdle     : return "EPIdle";
      case EPDataIn   : return "EPDataIn";
      case EPDataOut  : return "EPDataOut,";
      case EPLastIn   : return "EPLastIn";
      case EPStatusIn : return "EPStatusIn";
      case EPStatusOut: return "EPStatusOut";
      case EPThrottle : return "EPThrottle";
      case EPStall    : return "EPStall";
      case EPComplete : return "EPComplete";
   }
}

/**
 * Get name of USB request
 *
 * @param  reqType Request type
 *
 * @return Pointer to static string
 */
const char *getRequestName(uint8_t reqType){
   static const char *names[] = {
         "GET_STATUS",              /* 0x00 */
         "CLEAR_FEATURE",           /* 0x01 */
         "Unknown #2",
         "SET_FEATURE",             /* 0x03 */
         "Unknown #4",
         "SET_ADDRESS",             /* 0x05 */
         "GET_DESCRIPTOR",          /* 0x06 */
         "SET_DESCRIPTOR",          /* 0x07 */
         "GET_CONFIGURATION",       /* 0x08 */
         "SET_CONFIGURATION",       /* 0x09 */
         "GET_INTERFACE",           /* 0x0a */
         "SET_INTERFACE",           /* 0x0b */
         "SYNCH_FRAME",             /* 0x0c */
         "Unknown #D",
         "Unknown #E",
         "Unknown #F",
   };
   const char *rc = "Unknown";
   if (reqType<(sizeof(names)/sizeof(names[0]))) {
      rc = names[reqType];
   }
   return rc;
}

/**
 * Report contents of BDT
 *
 * @param name    Descriptive name to use
 * @param bdt     BDT to report
 */
void reportBdt(const char *name, BdtEntry *bdt) {
   (void)name;
   (void)bdt;
   if (bdt->u.setup.own) {
      PRINTF("%s addr=0x%08lX, bc=%d, %s, %s, %s\n",
            name,
            bdt->addr, bdt->bc,
            bdt->u.setup.data0_1?"DATA1":"DATA0",
                  bdt->u.setup.bdt_stall?"STALL":"OK",
                        "USB"
      );
   }
   else {
      PRINTF("%s addr=0x%08lX, bc=%d, %s, %s\n",
            name,
            bdt->addr, bdt->bc,
            getTokenName(bdt->u.result.tok_pid),
            "PROC"
      );
   }
}

void reportLineCoding(const LineCodingStructure *lineCodingStructure) {
   (void)lineCodingStructure;
   PRINTF("rate   = %ld bps\n", lineCodingStructure->dwDTERate);
   PRINTF("format = %d\n", lineCodingStructure->bCharFormat);
   PRINTF("parity = %d\n", lineCodingStructure->bParityType);
   PRINTF("bits   = %d\n", lineCodingStructure->bDataBits);
}

/**
 * Format SETUP packet as string
 *
 * @param p SETUP packet
 *
 * @return Pointer to static buffer
 */
const char *reportSetupPacket(SetupPacket *p) {
   static char buff[100];
   snprintf(buff, sizeof(buff), "[%2X,%s,%d,%d,%d]",
         p->bmRequestType,
         getRequestName(p->bRequest),
         (int)(p->wValue),
         (int)(p->wIndex),
         (int)(p->wLength)
   );
   return buff;
}

void reportLineState(uint8_t value) {
   (void)value;
   PRINTF("Line state: RTS=%d, DTR=%d\n", (value&(1<<1))?1:0, (value&(1<<0))?1:0);
}

/**
 *  Creates a valid string descriptor in UTF-16-LE from a limited UTF-8 string
 *
 *  @param dest     - Where to place descriptor
 *  @param source   - Zero terminated UTF-8 C string
 *  @param maxSize  - Size of dest
 *
 *  @note Only handles UTF-16 characters that fit in a single UTF-16 'character'.
 */
void utf8ToStringDescriptor(uint8_t *dest, const uint8_t *source, unsigned maxSize) {
   uint8_t *size = dest; // 1st byte is where to place descriptor size

   *dest++ = 2;         // 1st byte = descriptor size (2 bytes so far)
   *dest++ = DT_STRING; // 2nd byte = DT_STRING;

   while (*source != '\0') {
      uint16_t utf16Char=0;
      *size  += 2;         // Update size
      if ((*size)>maxSize) {
         __BKPT();
      }
      if (*source < 0x80) {  // 1-byte
         utf16Char = *source++;
      }
      else if ((*source &0xE0) == 0xC0){   // 2-byte
         utf16Char  = (0x1F&*source++)<<6;
         utf16Char += (0x3F&*source++);
      }
      else if ((*source &0xF0) == 0xE0){   // 3-byte
         utf16Char  = (0x0F&*source++)<<12;
         utf16Char += (0x3F&*source++)<<6;
         utf16Char += (0x3F&*source++);
      }
      *dest++ = (char)utf16Char;       // Copy 16-bit UTF-16 value
      *dest++ = (char)(utf16Char>>8);  // in little-endian order
   }
}

#if (HW_CAPABILITY&CAP_CDC)
/**
 * Get line coding for CDC
 */
template<class Info, int EP0_SIZE>void  UsbBase_T<Info, EP0_SIZE>::handleGetLineCoding() {
   //   PUTS("getLineCoding");
   const LineCodingStructure *lc = cdc_getLineCoding();
   //   reportLineCoding(lc);
   // Send packet
   ep0StartTxTransaction( sizeof(LineCodingStructure), (const uint8_t*)lc);
}

/**
 * Set line coding callback for CDC
 */
template<class Info, int EP0_SIZE>void  UsbBase_T<Info, EP0_SIZE>::setLineCodingCallback() {
   cdc_setLineCoding((LineCodingStructure * const)&controlDataBuffer);
   //   reportLineCoding((LineCodingStructure * const)&controlDataBuffer);
}

/**
 * Set line coding for CDC
 */
template<class Info, int EP0_SIZE>void  UsbBase_T<Info, EP0_SIZE>::handleSetLineCoding() {
   //   PUTS("setLineCoding");
   ep0State.callback = setLineCodingCallback;

   // Don't use buffer - this requires sizeof(LineCodingStructure) < CONTROL_EP_MAXSIZE
   ep0StartRxTransaction(sizeof(LineCodingStructure), nullptr);
}

/**
 * Set Line state for CDC
 */
template<class Info, int EP0_SIZE>void  UsbBase_T<Info, EP0_SIZE>::handleSetControlLineState() {
   //   PUTS("setControlLineState");
   //   reportLineState(ep0SetupBuffer.wValue.lo());
   cdc_setControlLineState(ep0SetupBuffer.wValue.lo());
   ep0StartTxTransaction( 0, nullptr ); // Tx empty Status packet
}

/**
 * Send BREAK on CDC
 */
template<class Info, int EP0_SIZE>void  UsbBase_T<Info, EP0_SIZE>::handleSendBreak() {
   //   PUTS("sendBreak");

   cdc_sendBreak(ep0SetupBuffer.wValue);  // time in milliseconds, 0xFFFF => continuous
   ep0StartTxTransaction( 0, nullptr );   // Tx empty Status packet
}
#endif

} // End namespace USBDM
