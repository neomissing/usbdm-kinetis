/*
 ============================================================================
 * @file    main.cpp (180.ARM_Peripherals)
 * @brief   Basic C++ demo using GPIO class
 *
 *  Created on: 10/1/2016
 *      Author: podonoghue
 ============================================================================
 */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <random>
#include <stdlib.h>
#include "hardware.h"
#include "usb.h"
#include "targetVddInterface.h"
#include "resetInterface.h"
#include "delay.h"
#include "configure.h"
#include "commands.h"
#include "bdmCommon.h"
#include "bdm.h"
#include "cmdProcessingSWD.h"
#include "cmdProcessingHCS.h"

#if 0
/** Check error code from USBDM API function
 *
 * @param rc       Error code to report
 * @param file     Filename to report
 * @param lineNum  Line number to report
 *
 * An error message is printed with line # and the program exited if
 * rc indicates any error
 */
void check(USBDM_ErrorCode rc , const char *file = NULL, unsigned lineNum = 0 ) {
   (void)rc;
   (void)file;
   (void)lineNum;
   if (rc == BDM_RC_OK) {
      //      PRINTF("OK,     [%s:#%4d]\n", file, lineNum);
      return;
   }
   PRINTF("Failed, [%s:#%4d] Reason= %d\n", file, lineNum, rc);
   __BKPT();
}
/**
 * Convenience macro to add line number information to check()
 */
#define CHECK(x) check((x), __FILE__, __LINE__)

uint8_t buffer[20] = {
      0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20
};

using namespace Hcs;
using namespace Cfv1;

/**
 *  Sets up dummy command for testing f_CMD_WRITE_MEM()
 */
USBDM_ErrorCode memWrite(uint32_t address, uint8_t opSize, uint8_t size, const uint8_t data[]) {
   //                          SIZE     #BYTES   Address
   uint8_t operation[] = {0,0, opSize,   size,   (uint8_t)(address>>24), (uint8_t)(address>>16), (uint8_t)(address>>8), (uint8_t)address};

   memcpy(commandBuffer,                   operation, sizeof(operation));
   memcpy(commandBuffer+sizeof(operation), data,      size);

   return f_CMD_WRITE_MEM();
}

/**
 *  Sets up dummy command for testing f_CMD_READ_MEM()
 */
USBDM_ErrorCode memRead(uint32_t address, uint8_t opSize, uint8_t size, uint8_t data[]) {
   //                          SIZE     #BYTES   Address
   uint8_t operation[] = {0,0, opSize,   size,   (uint8_t)(address>>24), (uint8_t)(address>>16), (uint8_t)(address>>8), (uint8_t)address};

   memcpy(commandBuffer, operation, sizeof(operation));
   USBDM_ErrorCode rc = f_CMD_READ_MEM();
   if (rc == BDM_RC_OK) {
      memcpy(data, commandBuffer+1, size);
   }
   return rc;
}

/**
 *
 * Examples
 *   SWD : testmem(0x20000000, 0x10000);
 *
 */
USBDM_ErrorCode testmem(uint32_t addressStart, uint32_t addrRange) {

//   PRINTF("Connection speed = %ld Hz\n", getSpeed());

   CHECK(f_CMD_CONNECT());
//   CHECK(Swd::powerUp());
   PRINTF("Connected\n");

   uint8_t randomData[sizeof(commandBuffer)];
   for (unsigned i=0; i<sizeof(randomData);i++) {
      randomData[i] = rand();
   }
   for(;;) {
      static const uint8_t sizes[] = {1,2,4};
      int sizeIndex    = rand()%3;
      uint8_t  opSize  = sizes[sizeIndex];
      uint8_t  size    = rand()%(sizeof(commandBuffer)-20)+1;
      uint32_t address = addressStart+rand()%(addrRange-size);

      uint32_t mask = ~((1<<sizeIndex)-1);
      address = address & mask;
      size    = size & mask;

      //                          SIZE     #BYTES   Address
      uint8_t operation[] = {0,0, opSize,   size,   (uint8_t)(address>>24), (uint8_t)(address>>16), (uint8_t)(address>>8), (uint8_t)address};

      PRINTF("Testing [%08lX..%08lX]:%c\n", address, address+size-1, "-BW-L"[opSize]);

      memcpy(commandBuffer, operation, sizeof(operation));
      memcpy(commandBuffer+sizeof(operation), randomData, size);
      CHECK(f_CMD_WRITE_MEM());

      memset(commandBuffer, 0, sizeof(commandBuffer));
      memcpy(commandBuffer, operation, sizeof(operation));
      CHECK(f_CMD_READ_MEM());

      if (memcmp(commandBuffer+1, randomData, size) != 0) {
         CHECK(BDM_RC_CF_DATA_INVALID);
      }
   }
   return BDM_RC_OK;
}

#endif
#if 0
USBDM_ErrorCode recover() {
   Swd::initialise();
   USBDM_ErrorCode rc = Swd::lineReset();
   if (rc != BDM_RC_OK) {
      PRINTF("Failed lineReset(), rc = %d\n", rc);
      rc = Swd::connect();
      if (rc != BDM_RC_OK) {
         PRINTF("Failed connect(), rc = %d\n", rc);
      }
   }
   if (rc == BDM_RC_OK) {
      PRINTF("lineReset()/connect() done\n");
   }
   uint32_t status;
   rc = Swd::readReg(Swd::SWD_RD_DP_STATUS, status);
   if (rc != BDM_RC_OK) {
      PRINTF("Failed SWD_RD_DP_STATUS, rc = %d\n", rc);
   }
   else {
      PRINTF("DP Status = 0x%08lX\n", status);
   }
   rc = Swd::clearStickyBits();
   if (rc != BDM_RC_OK) {
      PRINTF("Failed clearStickyBits(), rc = %d\n", rc);
   }
   else {
      PRINTF("Cleared sticky bits\n");
   }
   return rc;
}

void testMassErase() {
   Swd::initialise();

   CHECK(Swd::f_CMD_CONNECT());
   CHECK(Swd::powerUp());

   PRINTF("Connected\n");

   USBDM_ErrorCode rc = Swd::kinetisMassErase();
   if (rc != BDM_RC_OK) {
      PRINTF("Failed massErase(), rc = %d\n", rc);
   }
   else {
      PRINTF("OK massErase()\n");
   }
}

/**
 *
 */
USBDM_ErrorCode testmem() {

   Swd::initialise();

   PRINTF("Connection speed = %ld Hz\n", Swd::getSpeed());

   CHECK(Swd::f_CMD_CONNECT());
   CHECK(Swd::powerUp());
   PRINTF("Connected\n");

   uint8_t randomData[sizeof(commandBuffer)];
   for (unsigned i=0; i<sizeof(randomData);i++) {
      randomData[i] = rand();
   }
   for(;;) {
      static const uint8_t sizes[] = {1,2,4};
      int sizeIndex    = rand()%3;
      uint8_t  opSize  = sizes[sizeIndex];
      uint32_t address = 0x20000000+rand()%10000;
      uint8_t  size    = rand()%(sizeof(commandBuffer)-20)+1;

      uint32_t mask = ~((1<<sizeIndex)-1);
      address = address & mask;
      size    = size & mask;

      //                          SIZE     #BYTES   Address
      uint8_t operation[] = {0,0, opSize,   size,   (uint8_t)(address>>24), (uint8_t)(address>>16), (uint8_t)(address>>8), (uint8_t)address};

      PRINTF("Testing [%08lX..%08lX]:%c\n", address, address+size-1, "-BW-L"[opSize]);

      memcpy(commandBuffer, operation, sizeof(operation));
      memcpy(commandBuffer+sizeof(operation), randomData, size);
      CHECK(Swd::f_CMD_WRITE_MEM());

      memset(commandBuffer, 0, sizeof(commandBuffer));
      memcpy(commandBuffer, operation, sizeof(operation));
      CHECK(Swd::f_CMD_READ_MEM());

      if (memcmp(commandBuffer+1, randomData, size) != 0) {
         return BDM_RC_CF_DATA_INVALID;
      }
   }
   return BDM_RC_OK;
}

USBDM_ErrorCode checkIDcode() {
   Swd::initialise();

   CHECK(Swd::f_CMD_CONNECT());
   CHECK(Swd::powerUp());
   PRINTF("Connected\n");

   uint32_t idcode;
   USBDM_ErrorCode rc = Swd::readReg(Swd::SWD_RD_DP_IDCODE, idcode);
   if (rc != BDM_RC_OK) {
      PRINTF("Failed SWD_RD_DP_IDCODE, rc = %d\n", rc);
      return rc;
   }
   else {
      if (idcode != 0x2BA01477) {
         PRINTF("Wrong IDCODE = 0x%08lX\n", idcode);
         return rc;
      }
      else {
         PRINTF("IDCODE = 0x%08lX\n", idcode);
      }
   }
   return BDM_RC_OK;
}

void testReset() {
   for(;;) {
      ResetInterface::low();
      USBDM::waitMS(100);
      ResetInterface::high();
      USBDM::waitMS(100);
      ResetInterface::low();
      USBDM::waitMS(100);
      ResetInterface::highZ();
      USBDM::waitMS(100);
   }
}

void hcs08Testing () {
   // Need to initialise for debug UART0
   ::initialise();

   PRINTF("SystemBusClock  = %ld\n", SystemBusClock);
   PRINTF("SystemCoreClock = %ld\n", SystemCoreClock);

   PRINTF("Target Vdd = %f\n", TargetVdd::readVoltage());

   USBDM_ErrorCode rc;
   do {
      rc = setTarget(T_HCS08);
      if (rc != BDM_RC_OK) {
         continue;
      }
      rc = f_CMD_CONNECT();
      if (rc != BDM_RC_OK) {
         continue;
      }
      rc = f_CMD_HALT();
      if (rc != BDM_RC_OK) {
         continue;
      }
      testmem(0x80,1024);
   } while (true);

   for(;;) {
   }

}
#endif

void initialise() {
   ResetInterface::initialise();
   UsbLed::initialise();
   TargetVddInterface::initialise();
   TargetVddInterface::setCallback(targetVddSense);
   Debug::initialise();

   InterfaceEnable::setOutput();
   InterfaceEnable::high();

   // Wait for Vbdm stable
   USBDM::wait(10*USBDM::ms);

   // Update power status
   checkTargetVdd();
}

//char debugBuffer[200] = {0};
//char logBuffer[128] = {0};
//char logIndex = 0;

int main() {
//   hcs08Testing();

   // Need to initialise for debug UART0
   ::initialise();

//   for(;;) {
//      TargetVddInterface::vdd5VOn();
//      int id = targetVddMeasure();
//      PRINTF("ID  = %d\n", id);
//   }

   PRINTF("SystemBusClock  = %ld\n", SystemBusClock);
   PRINTF("SystemCoreClock = %ld\n", SystemCoreClock);
   PRINTF("SystemCoreClock = %d\n",  HardwareId::getId());

   PRINTF("Target Vdd = %f\n", TargetVddInterface::readVoltage());

   USBDM::UsbImplementation::initialise();

//   Bdm::initialise();
//   for(;;) {
//      FTM0->SWOCTRL = 0xFF;
//      FTM0->SWOCTRL = 0xFFFF;
//   }
   for(;;) {
      // Wait for USB connection
      while(!USBDM::UsbImplementation::isConfigured()) {
         __WFI();
      }
      // Process commands
      commandLoop();
      // Re-initialise
      ::initialise();
   }
}
