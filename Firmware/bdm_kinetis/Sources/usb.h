/*
 * usb.h
 *
 *  Created on: 25/10/2013
 *      Author: podonoghue
 */

#ifndef USB_H_
#define USB_H_
/*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include <stdio.h>
#include "hardware.h"
#include "usb_defs.h"
#include "utilities.h"
#include "configure.h"
#include "usb_endpoint.h"

namespace USBDM {

//#undef HW_CAPABILITY // For testing

//#define MS_COMPATIBLE_ID_FEATURE

#if defined(MS_COMPATIBLE_ID_FEATURE)
extern const MS_CompatibleIdFeatureDescriptor msCompatibleIdFeatureDescriptor;
extern const MS_PropertiesFeatureDescriptor msPropertiesFeatureDescriptor;
#endif

/**
 * Type definition for USB interrupt call back
 *
 *  @param status Interrupt status e.g. USB_ISTAT_SOFTOK_MASK, USB_ISTAT_STALL_MASK etc
 */
typedef void (*USBCallbackFunction)(uint8_t status);

/**
 * Type definition for USB SOF call back
 */
typedef void (*SOFCallbackFunction)();

/**
 * Type definition for USB SETUP call back\n
 * This function is called for SETUP transactions not automatically handled
 *
 *  @param setup SETUP packet information
 */
typedef void (*SetupCallbackFunction)(const SetupPacket &setup);

/**
 * Get name of USB token
 *
 * @param  token USB token
 *
 * @return Pointer to static string
 */
extern const char *getTokenName(unsigned token);

/**
 * Get name of USB state
 *
 * @param  state USB state
 *
 * @return Pointer to static string
 */
extern const char *getStateName(EndpointState state);

/**
 * Get name of USB request
 *
 * @param  reqType Request type
 *
 * @return Pointer to static string
 */
extern const char *getRequestName(uint8_t reqType);

/**
 * Report contents of BDT
 *
 * @param name    Descriptive name to use
 * @param bdt     BDT to report
 */
extern void reportBdt(const char *name, BdtEntry *bdt);

/**
 * Format SETUP packet as string
 *
 * @param p SETUP packet
 *
 * @return Pointer to static buffer
 */
extern const char *reportSetupPacket(SetupPacket *p);

/**
 *  Creates a valid string descriptor in UTF-16-LE from a limited UTF-8 string
 *
 *  @param source - Zero terminated UTF-8 C string
 *
 *  @param dest   - Where to place descriptor
 *
 *  @note Only handles UTF-16 characters that fit in a single UTF-16 'character'.
 */
extern void utf8ToStringDescriptor(uint8_t *dest, const uint8_t *source, unsigned maxSize);

/**
 * Class representing an USB Interface
 *
 * @tparam Info      USB info class
 * @tparam EP0_SIZE  Size of EP0 endpoint transactions
 */
template <class Info, int EP0_SIZE>
class UsbBase_T {

public:
   /** EP0 = Required control endpoint */
   static const ControlEndpoint<Info, EP0_SIZE>ep0;

   /**
    * Device Status
    */
   struct DeviceStatus {
      int selfPowered  : 1;    //!< Device is self-powered
      int remoteWakeup : 1;    //!< Supports remote wake-up
      int portTest     : 1;    //!< Port test
      int res1         : 5;    //!< Reserved
      int res2         : 8;    //!< Reserved
   };

   /**
    * Device State
    */
   struct DeviceState {
      DeviceConnectionStates  state:8;
      uint8_t                 configuration;
      uint8_t                 interfaceAltSetting;
      DeviceStatus            status;
      uint8_t                 newUSBAddress;
   };

protected:
   /** Mask for all USB interrupts */
   static constexpr uint8_t USB_INTMASKS =
         USB_INTEN_STALLEN_MASK|USB_INTEN_TOKDNEEN_MASK|
         USB_INTEN_SOFTOKEN_MASK|USB_INTEN_USBRSTEN_MASK|
         USB_INTEN_SLEEPEN_MASK;

   /** USB device state information */
   static DeviceState deviceState;

   /** Pointer to USB hardware */
   static constexpr volatile USB_Type *usb      = Info::usb;

   /** Pointer to USB clock register */
   static constexpr volatile uint32_t *clockReg = Info::clockReg;

   /** Buffer for EP0 Setup packet (copied from USB RAM) */
   static SetupPacket ep0SetupBuffer;

   /** Indicates device has been active within the last second */
   static bool activityFlag;

   /** SOF callback */
   static SOFCallbackFunction sofCallbackFunction;

   /** Unhandled SETUP callback */
   static SetupCallbackFunction setupCallbackFunction;

public:
   /**
    * Initialise USB to default settings\n
    * Configures all USB pins
    */
   static void enable() {
      *clockReg |= Info::clockMask;
      __DMB();

      Info::initPCRs();

      enableNvicInterrupts();
   }

   /**
    * Enable/disable interrupts in NVIC
    *
    * @param enable true to enable, false to disable
    */
   static void enableNvicInterrupts(bool enable=true) {

      if (enable) {
         // Enable interrupts
         NVIC_EnableIRQ(Info::irqNums[0]);

         // Set priority level
         NVIC_SetPriority(Info::irqNums[0], Info::irqLevel);
      }
      else {
         // Disable interrupts
         NVIC_DisableIRQ(Info::irqNums[0]);
      }
   }

   /**
    * Enable/disable interrupts
    *
    * @param mask Mask of interrupts to enable e.g. USB_INTEN_SOFTOKEN_MASK, USB_INTEN_STALLEN_MASK etc
    */
   static void enableInterrupts(uint8_t mask=0xFF) {
      usb->INTEN = mask;
   }

   /**
    * Enable/disable OTG interrupts
    *
    * @param mask Mask of interrupts to enable e.g. USB_OTGICR_IDEN_MASK, USB_OTGICR_ONEMSECEN_MASK etc
    */
   static void enableOtgInterrupts(uint8_t mask=0xFF) {
      usb->OTGICR = mask;
   }

protected:
   /**
    * Does base initialisation
    */
   static void initialise();

   /**
    * Set the USB activity flag
    *
    * @param busy True to indicates there was recent activity
    */
   static void setActive(bool busy=true) {
      activityFlag = busy;
   }

   /**
    * Set callback for unhandled SETUP transactions
    */
   static void setSetupCallback(SetupCallbackFunction callback) {
      setupCallbackFunction = callback;
   }

   /**
    * Set callback for SOF transactions
    */
   static void setSOFCallback(SOFCallbackFunction callback) {
      sofCallbackFunction = callback;
   }

   /**
    * Configure EP0 for a series of IN transactions [Tx, device -> host, DATA0/1]\n
    * This will be in response to a SETUP transaction\n
    * The data may be split into multiple DATA0/DATA1 packets
    *
    * @param bufSize Size of buffer to send
    * @param bufPtr  Pointer to buffer (may be NULL to indicate ep0.fDatabuffer is being used directly)
    */
   static void ep0StartTxTransaction( uint16_t bufSize, const uint8_t *bufPtr ) {
      if (bufSize > ep0SetupBuffer.wLength) {
         // More data than requested in SETUP request - truncate
         bufSize = (uint8_t)ep0SetupBuffer.wLength;
      }
      // If short response we may need ZLP
      ep0.setNeedZLP(bufSize < ep0SetupBuffer.wLength);
      ep0.startTxTransaction(bufSize, bufPtr);
   }

   /**
    * Configure EP0-out for a SETUP transaction [Rx, device<-host, DATA0]
    * Endpoint state is changed to EPIdle
    */
   static void ep0ConfigureSetupTransaction() {
      // Set up EP0-RX to Rx SETUP packets
      ep0.initialiseBdtRx();
      ep0.getHardwareState().state = EPIdle;
   }

   /**
    *  Configure EP0-RX for a SETUP transaction [Rx, device<-host, DATA0]
    *  Only done if endpoint is not already configured for some other OUT transaction
    *  Endpoint state unchanged.
    */
   static void ep0EnsureReadyForSetupTransaction() {
      ep0.initialiseBdtRx();
   }

   /**
    * Set USB interface to default state
    */
   static void setUSBdefaultState() {
      UsbLed::off();
      deviceState.state                = USBdefault;
      usb->ADDR                        = 0;
      deviceState.configuration        = 0;
      deviceState.interfaceAltSetting  = 0;
   }

   /**
    * Set addressed state
    *
    * @param address The USB address to set
    */
   static void setUSBaddressedState( uint8_t address ) {
      if (address == 0) {
         // Unaddress??
         setUSBdefaultState();
      }
      else {
         UsbLed::off();
         deviceState.state                = USBaddressed;
         usb->ADDR                        = address;
         deviceState.configuration        = 0;
         deviceState.interfaceAltSetting  = 0;
      }
   }

   /**
    * Set configures state
    *
    * @param config The number of the configuration to set
    */
   static void setUSBconfiguredState( uint8_t config ) {
      if (config == 0) {
         // unconfigure
         setUSBaddressedState(usb->ADDR);
      }
      else {
         UsbLed::on();
         deviceState.state                = USBconfigured;
         deviceState.configuration        = config;
         deviceState.interfaceAltSetting  = 0;
      }
   }

   /**
    * Initialises EP0 and clear other end-points
    */
   static void initialiseEndpoints(void);

   /**
    * Handles SETUP Packet
    */
   static void handleSetupToken();

   /**
    * Handler for Token Complete USB interrupt for EP0\n
    * Handles ep0 [SETUP, IN & OUT]
    */
   static void handleTokenCompleteEp0();

   /**
    * Handler for USB Bus reset\n
    * Re-initialises the interface
    */
   static void handleUSBReset();

   /**
    * STALL completed - re-enable ep0 for SETUP
    */
   static void handleStallComplete() {
      ep0.clearStall();
      ep0ConfigureSetupTransaction();
   }

   /**
    * Handler for Start of Frame Token interrupt (~1ms interval)
    */
   static void handleSOFToken() {
      // Activity LED
      // Off                     - no USB activity, not connected
      // On                      - no USB activity, connected
      // Off, flash briefly on   - USB activity, not connected
      // On,  flash briefly off  - USB activity, connected
      if (usb->FRMNUML==0) { // Every ~256 ms
         switch (usb->FRMNUMH&0x03) {
            case 0:
               if (deviceState.state == USBconfigured) {
                  // Activity LED on when USB connection established
                  UsbLed::on();
               }
               else {
                  // Activity LED off when no USB connection
                  UsbLed::off();
               }
               break;
            case 1:
            case 2:
               break;
            case 3:
            default :
               if (activityFlag) {
                  // Activity LED flashes on BDM activity
                  UsbLed::toggle();
                  setActive(false);
               }
               break;
         }
      }
      if (sofCallbackFunction != nullptr) {
         sofCallbackFunction();
      }
#if (HW_CAPABILITY&CAP_CDC)
      // Check if need to restart EP5 (CDC IN)
      ep5StartTxTransactionIfIdle();
#endif
   }

   /*
    * Handler for USB Suspend
    *   - Enables the USB module to wake-up the CPU
    *   - Stops the CPU
    * On wake-up
    *   - Re-checks the USB after a small delay to avoid wake-ups by noise
    *   - The RESUME interrupt is left pending so the resume handler can execute
    */
   static void handleUSBSuspend();

   /**
    * Handler for USB Resume
    *
    * Disables further USB module wake-ups
    */
   static void handleUSBResume();

   /**
    * ep0 - IN
    */
   static void ep0HandleInToken();

   /**
    * ep0 - OUT
    */
   static void ep0HandleOutToken();

   /**
    * Handles unexpected SETUP requests on EP0\n
    * May call setupCallbackFunction if initialised \n
    * otherwise stalls EP0
    */
   static void handleUnexpected() {
      if (setupCallbackFunction != nullptr) {
         setupCallbackFunction(ep0SetupBuffer);
      }
      else {
         ep0.stall();
      }
   }

   /**
    * Get Status - Device Req 0x00
    */
   static void handleGetStatus();

   /**
    * Clear Feature - Device Req 0x01
    */
   static void handleClearFeature();

   /**
    * Set Feature - Device Req 0x03
    */
   static void handleSetFeature();

   /**
    * Get Descriptor - Device Req 0x06
    */
   static void handleGetDescriptor();

   /**
    * Set device Address - Device Req 0x05
    */
   static void handleSetAddress() {
      //   PUTS("setAddress - ");

      if (ep0SetupBuffer.bmRequestType != (EP_OUT|RT_DEVICE)) {// Out,Standard,Device
         ep0.stall(); // Illegal format - stall ep0
         return;
      }

      // Setup callback to update address at end of transaction
      static uint8_t newAddress  = ep0SetupBuffer.wValue.lo();
      static auto callback = []() {
         setUSBaddressedState(newAddress);
      };
      ep0.setCallback(callback);

      ep0StartTxTransaction( 0, nullptr ); // Tx empty Status packet
   }

   /**
    * Get Configuration - Device Req 0x08
    */
   static void handleGetConfiguration() {
      ep0StartTxTransaction( 1, &deviceState.configuration );
   }

   /**
    * Set Configuration - Device Req 0x09
    * Treated as soft reset
    */
   static void handleSetConfiguration();

   /**
    * Set interface - Device Req 0x0B
    * Not required to be implemented
    */
   static void handleSetInterface() {
      //   PUTS("setInterface");

      if ((ep0SetupBuffer.bmRequestType != (EP_OUT|RT_INTERFACE)) || // NOT In,Standard,Interface
            (ep0SetupBuffer.wLength != 0) ||                         // NOT correct length
            (deviceState.state != USBconfigured)) {                  // NOT in addressed state
         ep0.stall(); // Error
         return;
      }
      // Only support one Alternate Setting == 0
      if (ep0SetupBuffer.wValue != 0) {
         ep0.stall(); // Error
         return;
      }
      // Tx empty Status packet
      ep0StartTxTransaction( 0, nullptr );
   }

   /**
    * Get interface - Device Req 0x0A
    */
   static void handleGetInterface() {
      static const uint8_t interfaceAltSetting = 0;

      //   PUTS("getInterface");

      if ((ep0SetupBuffer.bmRequestType != (EP_IN|RT_INTERFACE)) || // NOT In,Standard,Interface
            (ep0SetupBuffer.wLength != 1)) {                        // NOT correct length
         ep0.stall(); // Error
         return;
      }
      // Send packet
      ep0StartTxTransaction( sizeof(interfaceAltSetting), &interfaceAltSetting );
   }

public:

};

/** SOF callback */
template<class Info, int EP0_SIZE>
SOFCallbackFunction UsbBase_T<Info, EP0_SIZE>::sofCallbackFunction = nullptr;

/** SETUP callback */
template<class Info, int EP0_SIZE>
SetupCallbackFunction UsbBase_T<Info, EP0_SIZE>::setupCallbackFunction = nullptr;

/** USB device state information */
template<class Info, int EP0_SIZE>
typename UsbBase_T<Info, EP0_SIZE>::DeviceState UsbBase_T<Info, EP0_SIZE>::deviceState = {USBattached, 0, 0, {0,0,0,0,0}, 0};

/** Buffer for EP0 Setup packet (copied from USB RAM) */
template<class Info, int EP0_SIZE>
SetupPacket UsbBase_T<Info, EP0_SIZE>::ep0SetupBuffer;   //!< Buffer for EP0 Setup packet (copied from USB RAM)

/** USB activity indicator */
template<class Info, int EP0_SIZE>
bool UsbBase_T<Info, EP0_SIZE>::activityFlag = false;

/** USB Control endpoint EP0 */
template <class Info, int EP0_SIZE>
const ControlEndpoint<Info, EP0_SIZE> UsbBase_T<Info, EP0_SIZE>::ep0;

} // End namespace USBDM

/*
 * Implementation class
 */
#include "usb_implementation.h"

/*
 * Implementation of methods for UsbBase_T
 */
namespace USBDM {
/**
 * Handles SETUP Packet
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleSetupToken() {

   // Save data from SETUP packet
   memcpy(&ep0SetupBuffer, ep0.getBuffer(), sizeof(ep0SetupBuffer));

   ep0.getHardwareState().state   = EPIdle;
   ep0.getHardwareState().txData1 = DATA1;
   ep0.getHardwareState().rxData1 = DATA1;

   // Call-backs only persist during a SETUP transaction
   ep0.setCallback(nullptr);

   PRINTF("handleSetupToken(%s)\n", reportSetupPacket(&ep0SetupBuffer));

   switch(REQ_TYPE(ep0SetupBuffer.bmRequestType)) {
      case REQ_TYPE_STANDARD :
         // Standard device requests
         switch (ep0SetupBuffer.bRequest) {
            case GET_STATUS :          handleGetStatus();            break;
            case CLEAR_FEATURE :       handleClearFeature();         break;
            case SET_FEATURE :         handleSetFeature();           break;
            case SET_ADDRESS :         handleSetAddress();           break;
            case GET_DESCRIPTOR :      handleGetDescriptor();        break;
            case GET_CONFIGURATION :   handleGetConfiguration();     break;
            case SET_CONFIGURATION :   handleSetConfiguration();     break;
            case SET_INTERFACE :       handleSetInterface();         break;
            case GET_INTERFACE :       handleGetInterface();         break;
            case SET_DESCRIPTOR :
            case SYNCH_FRAME :
            default :                  handleUnexpected();           break;
         }
         break;

      case REQ_TYPE_CLASS :
         // Class requests
         handleUnexpected();
         break;

      case REQ_TYPE_VENDOR :
         // Handle special commands here
         switch (ep0SetupBuffer.bRequest) {
            case ICP_GET_VER : {
               // Tell command handler to re-initialise
               //TODO               reInit = true;

               // Version response
               static const uint8_t versionResponse[5] = {
                     BDM_RC_OK,
                     VERSION_SW,      // BDM SW version
                     VERSION_HW,      // BDM HW version
                     0,               // ICP_Version_SW;
                     VERSION_HW,      // ICP_Version_HW;
               };
               // Send response
               ep0StartTxTransaction( sizeof(versionResponse),  versionResponse );
            }
            break;

#ifdef MS_COMPATIBLE_ID_FEATURE
            case VENDOR_CODE :
               //               PUTS("REQ_TYPE_VENDOR - VENDOR_CODE");
               if (ep0SetupBuffer.wIndex == 0x0004) {
                  //                  PUTS("REQ_TYPE_VENDOR - msCompatibleIdFeatureDescriptor");
                  ep0StartTxTransaction(sizeof(msCompatibleIdFeatureDescriptor), (const uint8_t *)&msCompatibleIdFeatureDescriptor);
               }
               else if (ep0SetupBuffer.wIndex == 0x0005) {
                  //                  PUTS("REQ_TYPE_VENDOR - msPropertiesFeatureDescriptor");
                  ep0StartTxTransaction(sizeof(msPropertiesFeatureDescriptor), (const uint8_t *)&msPropertiesFeatureDescriptor);
               }
               else {
                  handleUnexpected();
               }
               break;
#endif

            case CMD_USBDM_ICP_BOOT :
            {
               auto reboot = []() {/* TODO */};
               // Reboots to ICP mode
               ep0.setCallback(reboot);
               ep0StartTxTransaction( 0, nullptr ); // Tx empty Status packet
            }
            break;

            default :
               handleUnexpected();
               break;
         }
      break;

      case REQ_TYPE_OTHER :
         handleUnexpected();
         break;
   }
   // In case another SETUP packet
   ep0EnsureReadyForSetupTransaction();

   // Allow transactions post SETUP packet (clear TXSUSPENDTOKENBUSY)
   usb->CTL = USB_CTL_USBENSOFEN_MASK;
}

/**
 * Handler for Token Complete USB interrupt for EP0\n
 * Handles ep0 [SETUP, IN & OUT]
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleTokenCompleteEp0() {
   // Status from Token
   uint8_t usbStat  = usb->STAT;

   // Relevant BDT
   BdtEntry *bdt = &bdts[usbStat>>2];

   ep0.flipOddEven(usbStat);

   // Control - Accept SETUP, IN or OUT token
#if 0
   if (bdt->u.result.tok_pid == SETUPToken) {
      PUTS("\n=====");
   }
   PRINTF("\nTOKEN=%s, STATE=%s, size=%d, %s, %s, %s\n",
         getTokenName(bdt->u.result.tok_pid),
         getStateName(ep0.getHardwareState().state),
         bdt->bc,
         (usbStat&USB_STAT_TX_MASK)?"Tx":"Rx",
               bdt->u.result.data0_1?"DATA1":"DATA0",
                     (usbStat&USB_STAT_ODD_MASK)?"Odd":"Even");
#endif
   switch (bdt->u.result.tok_pid) {
      case SETUPToken:
         handleSetupToken();
         if (((int)ep0.fHardwareState.txOdd)>=2) {
            __BKPT();
         }

         break;
      case INToken:
         ep0.handleInToken();
         if (((int)ep0.fHardwareState.txOdd)>=2) {
            __BKPT();
         }

         break;
      case OUTToken:
         ep0.handleOutToken();
         if (((int)ep0.fHardwareState.txOdd)>=2) {
            __BKPT();
         }

         break;
      default:
         PRINTF("Unexpected token on EP0 = %s\n", getTokenName(bdt->u.result.tok_pid));
         break;
   }

   if (((int)ep0.fHardwareState.txOdd)>=2) {
      if (bdt->u.result.tok_pid == SETUPToken) {
         PUTS("\n=====");
      }
      PRINTF("\nTOKEN=%s, STATE=%s, size=%d, %s, %s, %s\n",
            getTokenName(bdt->u.result.tok_pid),
            getStateName(ep0.getHardwareState().state),
            bdt->bc,
            (usbStat&USB_STAT_TX_MASK)?"Tx":"Rx",
                  bdt->u.result.data0_1?"DATA1":"DATA0",
                        (usbStat&USB_STAT_ODD_MASK)?"Odd":"Even");
      __BKPT();
   }
}

/**
 * Handler for USB Bus reset\n
 * Re-initialises the interface
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleUSBReset() {
   PUTS("\nReset");
   //   pushState('R');

   // Disable all interrupts
   usb->INTEN = 0x00;
   usb->ERREN = 0x00;

   // Clear USB error flags
   usb->ERRSTAT = 0xFF;

   // Clear all USB interrupt flags
   usb->ISTAT = 0xFF;

   setUSBdefaultState();

   // Initialise end-points via derived class
   UsbImplementation::initialiseEndpoints();

   // Set up to receive 1st SETUP packet
   ep0ConfigureSetupTransaction();

   // Enable various interrupts
   usb->INTEN = USB_INTMASKS|USB_INTEN_ERROREN_MASK;
   usb->ERREN = 0xFF;
}

/*
 * Handler for USB Suspend
 *   - Enables the USB module to wake-up the CPU
 *   - Stops the CPU
 * On wake-up
 *   - Re-checks the USB after a small delay to avoid wake-ups by noise
 *   - The RESUME interrupt is left pending so the resume handler can execute
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleUSBSuspend() {
   //   PUTS("Suspend");
   if (deviceState.state != USBconfigured) {
      // Ignore if not configured
      return;
   }
   // Need to disable BDM interface & everything else to reduce power
   UsbLed::off();

   // Clear the sleep and resume interrupt flags
   usb->ISTAT    = USB_ISTAT_SLEEP_MASK;

   // Asynchronous Resume Interrupt Enable (USB->CPU)
   // Only enable if transceiver is disabled
   //   usb->USBTRC0  |= USB_USBTRC0_USBRESMEN_MASK;

   // Enable resume detection or reset interrupts from the USB
   usb->INTEN   |= (USB_ISTAT_RESUME_MASK|USB_ISTAT_USBRST_MASK);
   deviceState.state = USBsuspended;
   UsbLed::off();

   // A re-check loop is used here to ensure USB bus noise doesn't wake-up the CPU
   do {
      usb->ISTAT = USB_ISTAT_RESUME_MASK;       // Clear resume interrupt flag

      __WFI();  // Processor stop for low power

      // The CPU has woken up!

      if ((usb->ISTAT&(USB_ISTAT_RESUME_MASK|USB_ISTAT_USBRST_MASK)) == 0) {
         // Ignore if not resume or reset from USB module
         continue;
      }
      usb->ISTAT = USB_ISTAT_RESUME_MASK;  // Clear resume interrupt flag

      // Wait for a second resume (or existing reset) ~5 ms
      for(int delay=0; delay<24000; delay++) {
         // We should have another resume interrupt by end of loop
         if ((usb->ISTAT&(USB_ISTAT_RESUME_MASK|USB_ISTAT_USBRST_MASK)) != 0) {
            break;
         }
      }
   } while ((usb->ISTAT&(USB_ISTAT_RESUME_MASK|USB_ISTAT_USBRST_MASK)) == 0);

   // Disable resume interrupts
   usb->INTEN   &= ~USB_ISTAT_RESUME_MASK;
   return;
}

/**
 * Handler for USB Resume
 *
 * Disables further USB module wake-ups
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleUSBResume() {
   //   PUTS("Resume");
   // Mask further resume interrupts
   usb->INTEN   &= ~USB_INTEN_RESUMEEN_MASK;

   if (deviceState.state != USBsuspended) {
      // Ignore if not suspended
      return;
   }
   // Mask further resume interrupts
   //   usb->USBTRC0 &= ~USB_USBTRC0_USBRESMEN_MASK;

   // Clear the sleep and resume interrupt flags
   usb->ISTAT = USB_ISTAT_SLEEP_MASK|USB_ISTAT_RESUME_MASK;

   deviceState.state = USBconfigured;

   // Set up to receive setup packet
   ep0ConfigureSetupTransaction();

   // Enable the transmit or receive of packets
   usb->CTL = USB_CTL_USBENSOFEN_MASK;
}

/**
 * Initialise the USB interface
 *
 *  @note Assumes clock set up for USB operation (48MHz)
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::initialise() {
   enable();

   sofCallbackFunction = nullptr;

   // Make sure no interrupt during setup
   enableNvicInterrupts(false);

   // Clear USB RAM (includes BDTs)
   memset((uint8_t*)endPointBdts, 0, sizeof(EndpointBdtEntry[Usb0::NUMBER_OF_ENDPOINTS]));

   usb->OTGISTAT = 0;
   usb->OTGICR   = 0;
   usb->OTGCTL   = 0;
   usb->INTEN    = 0;
   usb->ERRSTAT  = 0;
   usb->ERREN    = 0;
   usb->CTL      = 0;
   usb->ADDR     = 0;
   for (unsigned i=0; i<(sizeof(usb->ENDPOINT)/sizeof(usb->ENDPOINT[0])); i++) {
      usb->ENDPOINT[i].ENDPT = 0;
   }
   usb->USBCTRL = 0;
   usb->CONTROL = 0;
   usb->USBTRC0 = 0;

#ifdef MPU_CESR_VLD_MASK
   // Disable MPU & clear errors
   MPU->CESR = MPU_CESR_SPERR_MASK;
#endif

   // Enable USB regulator
   SIM->SOPT1CFG  = SIM_SOPT1CFG_URWE_MASK;
   SIM->SOPT1    |= SIM_SOPT1_USBREGEN_MASK;

#if 0
   // Enable in LP modes
   SIM->SOPT1CFG  = SIM_SOPT1CFG_URWE_MASK;
   SIM->SOPT1    &= ~(SIM_SOPT1_USBSSTBY_MASK|SIM_SOPT1_USBVSTBY_MASK);
#endif

#if 0
   // Removed due to errata e5928: USBOTG: USBx_USBTRC0[USBRESET] bit does not operate as expected in all cases
   // Reset USB H/W
   USB0_USBTRC0 = 0x40|USB_USBTRC0_USBRESET_MASK;
   while ((USB0_USBTRC0&USB_USBTRC0_USBRESET_MASK) != 0) {
   }
#endif

   // This bit is undocumented but seems to be necessary
   usb->USBTRC0 = 0x40;

   // Set initial USB state
   setUSBdefaultState();

   // Point USB at BDT array
   usb->BDTPAGE3 = (uint8_t) (((unsigned)endPointBdts)>>24);
   usb->BDTPAGE2 = (uint8_t) (((unsigned)endPointBdts)>>16);
   usb->BDTPAGE1 = (uint8_t) (((unsigned)endPointBdts)>>8);

   // Clear all pending interrupts
   usb->ISTAT = 0xFF;

   // Enable usb reset interrupt
   usb->INTEN = USB_INTEN_USBRSTEN_MASK|USB_INTEN_SLEEPEN_MASK;

   // Weak pull downs
   usb->USBCTRL = USB_USBCTRL_PDE_MASK;

   // Enable Pull-up
   usb->CONTROL = USB_CONTROL_DPPULLUPNONOTG_MASK;

   // Enable interface
   usb->CTL = USB_CTL_USBENSOFEN_MASK;

   // Initialise end-points via derived class
   UsbImplementation::initialiseEndpoints();

   // Enable USB interrupts
   enableNvicInterrupts();
}

/**
 * Initialises EP0 and clears other end-points
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::initialiseEndpoints() {

   //   PUTS("initialiseEndpoints()");

   // Clear all BDTs
   memset((uint8_t*)endPointBdts, 0, sizeof(EndpointBdtEntry[Usb0::NUMBER_OF_ENDPOINTS]));

   // Clear hardware state
   //   memset((uint8_t*)epHardwareState, 0, sizeof(epHardwareState));

#if (HW_CAPABILITY&CAP_CDC)
   ep3StartTxTransaction();       // Interrupt pipe IN - status
   ep4InitialiseBdtRx();          // Tx pipe OUT
   ep5StartTxTransactionIfIdle(); // Rx pipe IN
#endif

   // Clear odd/even bits & Enable Rx/Tx
   usb->CTL = USB_CTL_USBENSOFEN_MASK|USB_CTL_ODDRST_MASK;
   usb->CTL = USB_CTL_USBENSOFEN_MASK;

   ep0.initialise();
}

/**
 * Get Status - Device Req 0x00
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleGetStatus() {

   PRINTF("handleGetStatus()\n");

   /** Endpoint Status in USB format */
   struct EndpointStatus {
      int stall  : 1;   //!< Endpoint is stalled
      int res1   : 7;   //!< Reserved
      int res2   : 8;   //!< Reserved
   };
   static const uint8_t        zeroReturn[]    = {0,0};
   static const EndpointStatus epStatusStalled = {1,0,0};
   static const EndpointStatus epStatusOK      = {0,0,0};

   const uint8_t *dataPtr = nullptr;
   uint8_t size;
   uint8_t epNum;

   switch(ep0SetupBuffer.bmRequestType) {
      case EP_IN|RT_DEVICE : // Device Status
      dataPtr = (uint8_t *) &deviceState.status;
      size    = sizeof(deviceState.status);
      break;

      case EP_IN|RT_INTERFACE : // Interface Status - reserved
      dataPtr = zeroReturn;
      size = sizeof(zeroReturn);
      break;

      case EP_IN|RT_ENDPOINT : // Endpoint Status
      epNum = ep0SetupBuffer.wIndex&0x07;
      if (epNum <= Usb0::NUMBER_OF_ENDPOINTS) {
         if (usb->ENDPOINT[epNum].ENDPT&USB_ENDPT_EPSTALL_MASK) {
            dataPtr = (uint8_t*)&epStatusStalled;
         }
         else {
            dataPtr = (uint8_t*)&epStatusOK;
         }
         size = sizeof(EndpointStatus);
      }
      break;
   }
   if (dataPtr != nullptr) {
      ep0StartTxTransaction( size, dataPtr );
   }
   else {
      ep0.stall();
   }
}

/**
 * Clear Feature - Device Req 0x01
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleClearFeature() {
   bool okResponse = false;

   switch(ep0SetupBuffer.bmRequestType) {
      case RT_DEVICE : // Device Feature
         if ((ep0SetupBuffer.wValue != DEVICE_REMOTE_WAKEUP) || // Device remote wake up
               (ep0SetupBuffer.wIndex != 0))   {                // Device index must be 0
            break;
         }
         deviceState.status.remoteWakeup = 0;
         okResponse                      = true;
         break;

      case RT_INTERFACE : // Interface Feature
         break;

      case RT_ENDPOINT : { // Endpoint Feature ( Out,Standard,Endpoint )
         uint8_t epNum = ep0SetupBuffer.wIndex&0x07;
         if ((ep0SetupBuffer.wValue != ENDPOINT_HALT) || // Not Endpoint Stall ?
               (epNum >= Usb0::NUMBER_OF_ENDPOINTS))  // or illegal EP# (ignores direction)
            break;
         // TODO     epClearStall(epNum);
         okResponse = true;
      }
      break;

      default : // Illegal
         break;
   }

   if (okResponse) {
      ep0StartTxTransaction( 0, nullptr ); // Tx empty Status packet
   }
   else {
      ep0.stall();
   }
}

/**
 * Set Feature - Device Req 0x03
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleSetFeature() {
   bool okResponse = false;

   switch(ep0SetupBuffer.bmRequestType) {
      case RT_DEVICE : // Device Feature
         if ((ep0SetupBuffer.wValue != DEVICE_REMOTE_WAKEUP) || // Device remote wakeup
               (ep0SetupBuffer.wIndex != 0)) {                  // device wIndex must be 0
            break;
         }
         deviceState.status.remoteWakeup = 1;
         okResponse                      = true;
         break;

      case RT_INTERFACE : // Interface Feature
         break;

      case RT_ENDPOINT : { // Endpoint Feature ( Out,Standard,Endpoint )
         uint8_t epNum = ep0SetupBuffer.wIndex&0x07;
         if ((ep0SetupBuffer.wValue != ENDPOINT_HALT) || // Not Endpoint Stall ?
               (epNum >= Usb0::NUMBER_OF_ENDPOINTS))  {                   // or illegal EP# (ignores direction)
            break;
         }
         // TODO epStall(epNum);
         okResponse = true;
      }
      break;

      default : // Illegal
         break;
   }

   if (okResponse) {
      ep0StartTxTransaction( 0, nullptr ); // Tx empty Status packet
   }
   else {
      ep0.stall();
   }
}

/**
 * Get Descriptor - Device Req 0x06
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleGetDescriptor() {
   unsigned        descriptorIndex = ep0SetupBuffer.wValue.lo();
   uint16_t        dataSize = 0;
   const uint8_t  *dataPtr = nullptr;

   PRINTF("handleGetDescriptor(%d:%d)\n", ep0SetupBuffer.wValue.hi(), ep0SetupBuffer.wValue.lo());

   if (ep0SetupBuffer.bmRequestType != (EP_IN|RT_DEVICE)) {
      // Must be In,Standard,Device
      ep0.stall();
      return;
   }
   switch (ep0SetupBuffer.wValue.hi()) {

      case DT_DEVICE: // Get Device Descriptor - 1
         //      PUTS("getDescriptor-device - ");
         dataPtr  = (uint8_t *) &Usb0::deviceDescriptor;
         dataSize = sizeof(DeviceDescriptor);
         break;
      case DT_CONFIGURATION: // Get Configuration Descriptor - 2
         //      PUTS("getDescriptor-config - ");
         if (ep0SetupBuffer.wValue.lo() != 0) {
            ep0.stall();
            return;
         }
         dataPtr  = (uint8_t *) &Usb0::otherDescriptors;
         dataSize = sizeof(Usb0::otherDescriptors);
         break;
      case DT_DEVICEQUALIFIER: // Get Device Qualifier Descriptor
         //      PUTS("getDescriptor-deviceQ - ");
         ep0.stall();
         return;
      case DT_STRING: // Get String Desc.- 3
         //      PRINTF("getDescriptor-string - %d\n", descriptorIndex);
#ifdef MS_COMPATIBLE_ID_FEATURE
         if (descriptorIndex == 0xEE) {
            //         PUTS("getDescriptor-string - MS_COMPATIBLE_ID_FEATURE");
            dataPtr  = osStringDescriptor;
            dataSize = *dataPtr;
            break;
         }
#endif
         if (descriptorIndex >= sizeof(Usb0::stringDescriptors)/sizeof(Usb0::stringDescriptors[0])) {
            // Illegal string index - stall
            ep0.stall();
            return;
         }
         if (descriptorIndex == 0) {
            // Language bytes (unchanged)
            dataPtr  = (uint8_t *)Usb0::stringDescriptors[0];
         }
#if defined(UNIQUE_ID)
         else if (descriptorIndex == s_serial_index) {
            uint8_t utf8Buff[sizeof(SERIAL_NO)+10];

            // Generate Semi-unique Serial number
            uint32_t uid = SIM->UIDH^SIM->UIDMH^SIM->UIDML^SIM->UIDL;
            snprintf((char *)utf8Buff, sizeof(utf8Buff), SERIAL_NO, uid);

            dataPtr = commandBuffer;
            utf8ToStringDescriptor(utf8Buff, commandBuffer);
         }
#endif
         else {
            // Strings are stored in limited UTF-8 and need conversion
            //            static uint8_t buffer[100];
            //            dataPtr = buffer;
            //            utf8ToStringDescriptor(buffer, Usb0::stringDescriptors[descriptorIndex], sizeof(buffer));
            dataPtr = ep0.getBuffer();
            utf8ToStringDescriptor(ep0.getBuffer(), Usb0::stringDescriptors[descriptorIndex], ep0.BUFFER_SIZE);
         }
         dataSize = *dataPtr;
         break;
      default:
         // Shouldn't happen
         ep0.stall();
         return;
   } // switch

   // Set up transmission
   ep0StartTxTransaction( dataSize, dataPtr );
}

/**
 * Set Configuration - Device Req 0x09
 * Treated as soft reset
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleSetConfiguration() {
   //TODO   reInit = true;  // tell command handler to re-init

   if ((ep0SetupBuffer.bmRequestType != (EP_OUT|RT_DEVICE)) || // Out,Standard,Device
         ((ep0SetupBuffer.wValue.lo() != 0) &&       // Only supports 0=> un-configure, 1=> only valid configuration
               (ep0SetupBuffer.wValue.lo() != Usb0::CONFIGURATION_NUM))) {
      ep0.stall();
      return;
   }
   setUSBconfiguredState(ep0SetupBuffer.wValue.lo());

   // Initialise end-points via derived class
   UsbImplementation::initialiseEndpoints();

   //TODO   ep1.initialiseBdtRx();

   // Tx empty Status packet
   ep0StartTxTransaction( 0, nullptr );
}

}; // namespace
#endif /* USB_H_ */
