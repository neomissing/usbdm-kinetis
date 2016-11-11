/**
 * @file     usb.h
 * @brief    Universal Serial Bus
 *
 * @version  V4.12.1.80
 * @date     13 April 2016
 */
#ifndef PROJECT_HEADERS_USB_H_
#define PROJECT_HEADERS_USB_H_
/*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include <stdio.h>
#include <cstring>
#include "hardware.h"
#include "usb_defs.h"
#include "utilities.h"
#include "usb_endpoint.h"

namespace USBDM {

/**
 * Base class representing an USB Interface.\n
 * Holds shared constants and utility functions.
 */
class UsbBase {

public:
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
    * Endpoint Status in USB format
    */
   struct EndpointStatus {
      int stall  : 1;   //!< Endpoint is stalled
      int res1   : 7;   //!< Reserved
      int res2   : 8;   //!< Reserved
   };
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
   static const char *getTokenName(unsigned token);

   /**
    * Get name of USB state
    *
    * @param  state USB state
    *
    * @return Pointer to static string
    */
   static const char *getStateName(EndpointState state);

   /**
    * Get name of USB request
    *
    * @param  reqType Request type
    *
    * @return Pointer to static string
    */
   static const char *getRequestName(uint8_t reqType);

   /**
    * Report contents of BDT
    *
    * @param name    Descriptive name to use
    * @param bdt     BDT to report
    */
   static void reportBdt(const char *name, BdtEntry *bdt);

   /**
    * Format SETUP packet as string
    *
    * @param p SETUP packet
    *
    * @return Pointer to static buffer
    */
   static const char *reportSetupPacket(SetupPacket *p);

   /**
    *  Creates a valid string descriptor in UTF-16-LE from a limited UTF-8 string
    *
    *  @param to       - Where to place descriptor
    *  @param from     - Zero terminated UTF-8 C string
    *  @param maxSize  - Size of destination
    *
    *  @note Only handles UTF-8 characters that fit in a single UTF-16 value.
    */
   static void utf8ToStringDescriptor(uint8_t *to, const uint8_t *from, unsigned maxSize);
};

/**
 * Class representing an USB Interface
 *
 * @tparam Info      USB info class
 * @tparam EP0_SIZE  Size of EP0 endpoint transactions
 */
template <class Info, int EP0_SIZE>
class UsbBase_T : public UsbBase {

protected:
   /** USB Control endpoint number - must be zero */
   static constexpr int CONTROL_ENDPOINT = 0;

   /** USB Control endpoint - always EP0 */
   static ControlEndpoint<Info, EP0_SIZE>controlEndpoint;

   /** Magic number for MS driver feature */
   static constexpr uint8_t MS_VENDOR_CODE = 0x30;

   /** Magic string for MS driver feature */
   static const     uint8_t ms_osStringDescriptor[];

   /** End-points in use */
   static Endpoint *endPoints[];

   /** Mask for all USB interrupts */
   static constexpr uint8_t USB_INTMASKS =
         USB_INTEN_STALLEN_MASK|USB_INTEN_TOKDNEEN_MASK|
         USB_INTEN_SOFTOKEN_MASK|USB_INTEN_USBRSTEN_MASK|
         USB_INTEN_SLEEPEN_MASK;

   /** USB connection state */
   static volatile DeviceConnectionStates connectionState;

   /** Active USB configuration */
   static uint8_t deviceConfiguration;

   /** USB Device status */
   static DeviceStatus deviceStatus;

   /** Pointer to USB hardware */
   static constexpr volatile USB_Type *usb = Info::usb;

   /** Pointer to USB clock register */
   static constexpr volatile uint32_t *clockReg = Info::clockReg;

   /** Buffer for EP0 Setup packet (copied from USB RAM) */
   static SetupPacket ep0SetupBuffer;

   /** USB activity indicator */
   static volatile bool activityFlag;

   /** SOF callback */
   static SOFCallbackFunction sofCallbackFunction;

   /**
    * Unhandled SETUP callback \n
    * Called for unhandled SETUP transactions
    */
   static SetupCallbackFunction unhandledSetupCallback;

   /** Function to call when SETUP transaction is complete */
   static void (*setupCompleteCallback)();

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

   /**
    * Checks if the USB device is configured i.e. connected and enumerated etc.
    *
    * @return true if configured
    */
   static bool isConfigured() {
      return connectionState == USBconfigured;
   }

protected:
   /**
    * Callback used for EP0 transaction complete
    *
    * @param state State active immediately before call-back\n
    * (End-point state is currently EPIdle)
    */
   static void ep0TransactionCallback(EndpointState state);

   /**
    * Does base initialisation of the USB interface
    *
    *  @note Assumes clock set up for USB operation (48MHz)
    */
   static void initialise();

   /**
    * Adds an endpoint.
    *
    * @param endpoint The end-point to add
    */
   static void addEndpoint(Endpoint *endpoint);

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
    *
    * @param callback The call-back function to execute
    */
   static void setUnhandledSetupCallback(SetupCallbackFunction callback) {
      unhandledSetupCallback = callback;
   }

   /**
    *  Sets the function to call when SETUP transaction is complete
    *
    * @param callback The call-back function to execute
    */
   static void setSetupCompleteCallback(void (*callback)()) {
      setupCompleteCallback = callback;
   }

   /**
    * Set callback for SOF transactions
    *
    * @param callback The call-back function to execute
    */
   static void setSOFCallback(SOFCallbackFunction callback) {
      sofCallbackFunction = callback;
   }

   /**
    * Configure EP0 for a status IN transactions [Tx, device -> host, DATA0/1]\n
    * Used to acknowledge a DATA out transaction
    */
   static void ep0TxStatus() {
      controlEndpoint.startTxTransaction(EPStatusIn);
   }

   /**
    * Configure EP0 for a series of IN transactions [Tx, device -> host, DATA0/1]\n
    * This will be in response to a SETUP transaction\n
    * The data may be split into multiple DATA0/DATA1 packets
    *
    * @param bufSize Size of buffer to send
    * @param bufPtr  Pointer to buffer (may be NULL to indicate controlEndpoint.fDatabuffer is being used directly)
    */
   static void ep0StartTxTransaction(uint16_t bufSize, const uint8_t *bufPtr) {
      if (bufSize > ep0SetupBuffer.wLength) {
         // More data than requested in SETUP request - truncate
         bufSize = (uint8_t)ep0SetupBuffer.wLength;
      }
      // If short response we may need ZLP
      controlEndpoint.setNeedZLP(bufSize < ep0SetupBuffer.wLength);
      controlEndpoint.startTxTransaction(EPDataIn, bufSize, bufPtr);
   }

   /**
    * Configure EP0-out for a SETUP transaction [Rx, device<-host, DATA0]\n
    * Endpoint state is changed to EPIdle
    * There is no external buffer associated with this transaction.
    */
   static void ep0ConfigureSetupTransaction() {
      // Set up EP0-RX to Rx SETUP packets
      controlEndpoint.startRxTransaction(EPIdle);
   }

   /**
    * Set USB interface to default state
    */
   static void setUSBdefaultState() {
      connectionState                = USBdefault;
      usb->ADDR                        = 0;
      deviceConfiguration        = 0;
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
         connectionState                = USBaddressed;
         usb->ADDR                        = address;
         deviceConfiguration        = 0;
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
         connectionState                = USBconfigured;
         deviceConfiguration        = config;
      }
   }

   /**
    * Initialises EP0 and clears other end-points
    */
   static void initialiseEndpoints(void);

   /**
    * Handles SETUP Packet
    */
   static void handleSetupToken();

   /**
    * Handler for Token Complete USB interrupt for EP0\n
    * Handles controlEndpoint [SETUP, IN & OUT]
    *
    * @return true indicates token has been processed.\n
    * false token still needs processing
    */
   static bool handleTokenComplete();

   /**
    * Handler for USB Bus reset\n
    * Re-initialises the interface
    */
   static void handleUSBReset();

   /**
    * STALL completed - re-enable controlEndpoint for SETUP
    */
   static void handleStallComplete() {
      controlEndpoint.clearStall();
      ep0ConfigureSetupTransaction();
   }

   /**
    * Handler for Start of Frame Token interrupt (~1ms interval)
    */
   static void handleSOFToken() {
      if (sofCallbackFunction != nullptr) {
         sofCallbackFunction();
      }
   }

   /**
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
    * Handles unexpected SETUP requests on EP0\n
    * May call unhandledSetupCallback() if initialised \n
    * otherwise stalls EP0
    */
   static void handleUnexpected() {
      if (unhandledSetupCallback != nullptr) {
         unhandledSetupCallback(ep0SetupBuffer);
      }
      else {
         controlEndpoint.stall();
      }
   }

   /**
    * Get Status - Device Request 0x00
    */
   static void handleGetStatus();

   /**
    * Clear Feature - Device Request 0x01
    */
   static void handleClearFeature();

   /**
    * Set Feature - Device Request 0x03
    */
   static void handleSetFeature();

   /**
    * Get Descriptor - Device Request 0x06
    */
   static void handleGetDescriptor();

   /**
    * Set device Address - Device Request 0x05
    */
   static void handleSetAddress() {
      //   PUTS("setAddress - ");

      if (ep0SetupBuffer.bmRequestType != (EP_OUT|RT_DEVICE)) {// Out,Standard,Device
         controlEndpoint.stall(); // Illegal format - stall controlEndpoint
         return;
      }

      // Setup callback to update address at end of transaction
      static uint8_t newAddress  = ep0SetupBuffer.wValue.lo();
      static auto callback = []() {
         setUSBaddressedState(newAddress);
      };
      setSetupCompleteCallback(callback);

      ep0TxStatus(); // Tx empty Status packet
   }

   /**
    * Get Configuration - Device Request 0x08
    */
   static void handleGetConfiguration() {
      static uint8_t temp = deviceConfiguration;
      ep0StartTxTransaction( 1, &temp );
   }

   /**
    * Set Configuration - Device Request 0x09
    * Treated as soft reset
    */
   static void handleSetConfiguration();

   /**
    * Set interface - Device Request 0x0B
    * Not required to be implemented
    */
   static void handleSetInterface();

   /**
    * Get interface - Device Request 0x0A
    */
   static void handleGetInterface() {
      static const uint8_t interfaceAltSetting = 0;

      //   PUTS("getInterface");

      if ((ep0SetupBuffer.bmRequestType != (EP_IN|RT_INTERFACE)) || // NOT In,Standard,Interface
            (ep0SetupBuffer.wLength != 1)) {                        // NOT correct length
         controlEndpoint.stall(); // Error
         return;
      }
      // Send packet
      ep0StartTxTransaction( sizeof(interfaceAltSetting), &interfaceAltSetting );
   }

public:

};

/** SOF callback */
template<class Info, int EP0_SIZE>
UsbBase::SOFCallbackFunction UsbBase_T<Info, EP0_SIZE>::sofCallbackFunction = nullptr;

/**
 * Unhandled SETUP callback \n
 * Called for unhandled SETUP transactions
 */
template<class Info, int EP0_SIZE>
UsbBase::SetupCallbackFunction UsbBase_T<Info, EP0_SIZE>::unhandledSetupCallback = nullptr;

/** Function to call when SETUP transaction is complete */
template <class Info, int EP0_SIZE>
void (*UsbBase_T<Info, EP0_SIZE>::setupCompleteCallback)();

/** USB connection state */
template<class Info, int EP0_SIZE>
volatile DeviceConnectionStates UsbBase_T<Info, EP0_SIZE>::connectionState;

/** Active USB configuration */
template<class Info, int EP0_SIZE>
uint8_t UsbBase_T<Info, EP0_SIZE>::deviceConfiguration;

/** USB Device status */
template<class Info, int EP0_SIZE>
UsbBase::DeviceStatus UsbBase_T<Info, EP0_SIZE>::deviceStatus;

/** Buffer for EP0 Setup packet (copied from USB RAM) */
template<class Info, int EP0_SIZE>
SetupPacket UsbBase_T<Info, EP0_SIZE>::ep0SetupBuffer;

/** USB activity indicator */
template<class Info, int EP0_SIZE>
volatile bool UsbBase_T<Info, EP0_SIZE>::activityFlag = false;

/** USB Control endpoint - always EP0 */
template <class Info, int EP0_SIZE>
ControlEndpoint<Info, EP0_SIZE> UsbBase_T<Info, EP0_SIZE>::controlEndpoint;

} // End namespace USBDM

/*
 * Implementation class
 */
#include "usb_implementation.h"

/*
 * Implementation of methods for UsbBase_T
 */
namespace USBDM {

/** End-points in use */
template <class Info, int EP0_SIZE>
Endpoint *UsbBase_T<Info, EP0_SIZE>::endPoints[UsbImplementation::NUMBER_OF_ENDPOINTS];

#if defined(MS_COMPATIBLE_ID_FEATURE)
template<class Info, int EP0_SIZE>
const uint8_t UsbBase_T<Info, EP0_SIZE>::ms_osStringDescriptor[] = {
      18, DT_STRING, 'M',0,'S',0,'F',0,'T',0,'1',0,'0',0,'0',0,MS_VENDOR_CODE,0x00
};

// UTF16LE strings
#define MS_DEVICE_INTERFACE_GUIDs u"MS_DEVICE_INTERFACE_GUIDs"
#define MS_DEVICE_GUID            u"{93FEBD51-6000-4E7E-A20E-A80FC78C7EA1}\0"
#define MS_ICONS                  u"MS_ICONS"
#define MS_ICON_PATH              u"\%SystemRoot\%\\system32\\SHELL32.dll,-4\0"

struct MS_PropertiesFeatureDescriptor {
   uint32_t lLength;                //!< Size of this Descriptor in Bytes
   uint16_t wVersion;               //!< Version
   uint16_t wIndex;                 //!< Index (must be 5)
   uint16_t bnumSections;           //!< Number of property sections
   /*---------------------- Section 1 -----------------------------*/
   uint32_t lPropertySize0;          //!< Size of property section
   uint32_t ldataType0;              //!< Data type (1 = Unicode REG_SZ etc
   uint16_t wNameLength0;            //!< Length of property name
   char16_t bName0[sizeof(MS_DEVICE_INTERFACE_GUIDs)/sizeof(MS_DEVICE_INTERFACE_GUIDs[0])];
   uint32_t wPropertyLength0;        //!< Length of property data
   char16_t bData0[sizeof(MS_DEVICE_GUID)/sizeof(MS_DEVICE_GUID[0])];
   /*---------------------- Section 2 -----------------------------*/
   uint32_t lPropertySize1;          //!< Size of property section
   uint32_t ldataType1;              //!< Data type (1 = Unicode REG_SZ etc
   uint16_t wNameLength1;            //!< Length of property name
   char16_t bName1[sizeof(MS_ICONS)/sizeof(MS_ICONS[0])];
   uint32_t wPropertyLength1;        //!< Length of property data
   char16_t bData1[sizeof(MS_ICON_PATH)/sizeof(MS_ICON_PATH[0])];
};
#endif

extern const MS_CompatibleIdFeatureDescriptor msCompatibleIdFeatureDescriptor;
extern const MS_PropertiesFeatureDescriptor   msPropertiesFeatureDescriptor;

/**
 * Handles SETUP Packet
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleSetupToken() {

   // Save data from SETUP packet
   memcpy(&ep0SetupBuffer, controlEndpoint.getBuffer(), sizeof(ep0SetupBuffer));

   controlEndpoint.getHardwareState().state   = EPIdle;
   controlEndpoint.getHardwareState().txData1 = DATA1;
   controlEndpoint.getHardwareState().rxData1 = DATA1;

   // Call-backs only persist during a SETUP transaction
   setSetupCompleteCallback(nullptr);

//   PRINTF("handleSetupToken(%s)\n", reportSetupPacket(&ep0SetupBuffer));

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
#ifdef MS_COMPATIBLE_ID_FEATURE
            case MS_VENDOR_CODE :
               //               PUTS("REQ_TYPE_VENDOR - VENDOR_CODE");
               if (ep0SetupBuffer.wIndex == 0x0004) {
                  //                  PUTS("REQ_TYPE_VENDOR - MS_CompatibleIdFeatureDescriptor");
                  ep0StartTxTransaction(sizeof(MS_CompatibleIdFeatureDescriptor), (const uint8_t *)&msCompatibleIdFeatureDescriptor);
               }
               else if (ep0SetupBuffer.wIndex == 0x0005) {
                  //                  PUTS("REQ_TYPE_VENDOR - MS_PropertiesFeatureDescriptor");
                  ep0StartTxTransaction(sizeof(MS_PropertiesFeatureDescriptor), (const uint8_t *)&msPropertiesFeatureDescriptor);
               }
               else {
                  handleUnexpected();
               }
               break;
#endif

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
   controlEndpoint.initialiseBdtRx();

   // Allow transactions post SETUP packet (clear TXSUSPENDTOKENBUSY)
   usb->CTL = USB_CTL_USBENSOFEN_MASK;
}

/**
 * Handler for Token Complete USB interrupt for EP0\n
 * Handles controlEndpoint [SETUP, IN & OUT]
 *
 * @return true indicates token has been processed.\n
 * false token still needs processing
 */
template<class Info, int EP0_SIZE>
bool UsbBase_T<Info, EP0_SIZE>::handleTokenComplete() {
   // Status from Token
   uint8_t usbStat  = usb->STAT;

   // Endpoint number
   uint8_t   endPoint = ((uint8_t)usbStat)>>4;

   if (endPoint != CONTROL_ENDPOINT) {
      // Hasn't been processed
      return false;
   }
   controlEndpoint.flipOddEven(usbStat);

   // Relevant BDT
   BdtEntry *bdt = &bdts[usbStat>>2];

   // Control - Accept SETUP, IN or OUT token
#if 0
   if (bdt->u.result.tok_pid == SETUPToken) {
      PUTS("\n=====");
   }
   PRINTF("\nTOKEN=%s, STATE=%s, size=%d, %s, %s, %s\n",
         getTokenName(bdt->u.result.tok_pid),
         getStateName(controlEndpoint.getHardwareState().state),
         bdt->bc,
         (usbStat&USB_STAT_TX_MASK)?"Tx":"Rx",
               bdt->u.result.data0_1?"DATA1":"DATA0",
                     (usbStat&USB_STAT_ODD_MASK)?"Odd":"Even");
#endif
   switch (bdt->u.result.tok_pid) {
      case SETUPToken:
         handleSetupToken();
         break;
      case INToken:
         controlEndpoint.handleInToken();
         break;
      case OUTToken:
         controlEndpoint.handleOutToken();
         break;
      default:
         PRINTF("Unexpected token on EP0 = %s\n", getTokenName(bdt->u.result.tok_pid));
         break;
   }
   // Indicate processed
   return true;
}

/**
 * Handler for USB Bus reset\n
 * Re-initialises the interface
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleUSBReset() {
//   PUTS("\nReset");
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
   initialiseEndpoints();

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
   if (connectionState != USBconfigured) {
      // Ignore if not configured
      return;
   }
   // Clear the sleep and resume interrupt flags
   usb->ISTAT    = USB_ISTAT_SLEEP_MASK;

   // Asynchronous Resume Interrupt Enable (USB->CPU)
   // Only enable if transceiver is disabled
   //   usb->USBTRC0  |= USB_USBTRC0_USBRESMEN_MASK;

   // Enable resume detection or reset interrupts from the USB
   usb->INTEN   |= (USB_ISTAT_RESUME_MASK|USB_ISTAT_USBRST_MASK);
   connectionState = USBsuspended;

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

   if (connectionState != USBsuspended) {
      // Ignore if not suspended
      return;
   }
   // Mask further resume interrupts
   //   usb->USBTRC0 &= ~USB_USBTRC0_USBRESMEN_MASK;

   // Clear the sleep and resume interrupt flags
   usb->ISTAT = USB_ISTAT_SLEEP_MASK|USB_ISTAT_RESUME_MASK;

   connectionState = USBconfigured;

   UsbImplementation::initialiseEndpoints();

   // Enable the transmit or receive of packets
   usb->CTL = USB_CTL_USBENSOFEN_MASK;
}

/**
 * Callback used for EP0 transaction complete
 *
 * @param state State active immediately before call-back\n
 * (End-point state is currently EPIdle)
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::ep0TransactionCallback(EndpointState state) {
   switch (state) {
      case EPDataOut:
         // Just completed a series of OUT transfers on EP0 -
         // Now idle - make sure ready for SETUP reception
         ep0ConfigureSetupTransaction();
         // Do empty status packet transmission - no response expected
         ep0TxStatus();
         break;
      case EPDataIn:
         // Just completed a series of IN transfers on EP0 -
         // Do empty status packet reception
         controlEndpoint.startRxTransaction(EPStatusOut);
         break;
      case EPStatusIn:
         // Just completed an IN transaction acknowledging a series of OUT transfers -
         // SETUP transaction is complete
         if (setupCompleteCallback != nullptr) {
            // Do SETUP transaction complete callback
            setupCompleteCallback();
         }
         break;
      case EPStatusOut:
         // Just completed an OUT transaction acknowledging a series of IN transfers -
         // Now idle - make sure ready for SETUP reception
         ep0ConfigureSetupTransaction();
         break;
      default:
         PRINTF("Unhandled %d\n", state);
         break;
   }

}

/**
 * Does base initialisation of the USB interface
 *
 *  @note Assumes clock set up for USB operation (48MHz)
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::initialise() {
   enable();

   sofCallbackFunction = nullptr;

   // Make sure no interrupt during setup
   enableNvicInterrupts(false);

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

   // Initialise end-points
   initialiseEndpoints();

   // Enable USB interrupts
   enableNvicInterrupts();
}

/**
 * Adds an endpoint.
 *
 * @param endpoint The end-point to add
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::addEndpoint(Endpoint *endpoint) {
   endPoints[endpoint->fEndpointNumber] = endpoint;
}

/**
 * Initialises EP0 and clears other end-points
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::initialiseEndpoints() {

   //   PUTS("initialiseEndpoints()");

   // Clear all BDTs
   memset((uint8_t*)endPointBdts, 0, sizeof(EndpointBdtEntry[UsbImplementation::NUMBER_OF_ENDPOINTS]));

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

   controlEndpoint.initialise();

   controlEndpoint.setCallback(ep0TransactionCallback);

   // Set up to receive 1st SETUP packet
   ep0ConfigureSetupTransaction();
}

/**
 * Get Status - Device Request 0x00
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleGetStatus() {

   PRINTF("handleGetStatus()\n");

   static const uint8_t        zeroReturn[]    = {0,0};
   static const EndpointStatus epStatusStalled = {1,0,0};
   static const EndpointStatus epStatusOK      = {0,0,0};

   const uint8_t *dataPtr = nullptr;
   uint8_t size;
   uint8_t epNum;

   switch(ep0SetupBuffer.bmRequestType) {
      case EP_IN|RT_DEVICE : // Device Status
      dataPtr = (uint8_t *) &deviceStatus;
      size    = sizeof(deviceStatus);
      break;

      case EP_IN|RT_INTERFACE : // Interface Status - reserved
      dataPtr = zeroReturn;
      size = sizeof(zeroReturn);
      break;

      case EP_IN|RT_ENDPOINT : // Endpoint Status
      epNum = ep0SetupBuffer.wIndex&0x07;
      if (epNum <= UsbImplementation::NUMBER_OF_ENDPOINTS) {
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
      controlEndpoint.stall();
   }
}

/**
 * Clear Feature - Device Request 0x01
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
         deviceStatus.remoteWakeup = 0;
         okResponse                      = true;
         break;

      case RT_INTERFACE : // Interface Feature
         break;

      case RT_ENDPOINT : { // Endpoint Feature ( Out,Standard,Endpoint )
         uint8_t epNum = ep0SetupBuffer.wIndex&0x07;
         if ((ep0SetupBuffer.wValue != ENDPOINT_HALT) || // Not Endpoint Stall ?
               (epNum >= UsbImplementation::NUMBER_OF_ENDPOINTS))  { // or illegal EP# (ignores direction)
            break;
         }
         assert(endPoints[epNum] != nullptr);
         endPoints[epNum]->clearStall();
         okResponse = true;
      }
      break;

      default : // Illegal
         break;
   }

   if (okResponse) {
      ep0TxStatus(); // Tx empty Status packet
   }
   else {
      controlEndpoint.stall();
   }
}

/**
 * Set Feature - Device Request 0x03
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
         deviceStatus.remoteWakeup = 1;
         okResponse                      = true;
         break;

      case RT_INTERFACE : // Interface Feature
         break;

      case RT_ENDPOINT : { // Endpoint Feature ( Out,Standard,Endpoint )
         uint8_t epNum = ep0SetupBuffer.wIndex&0x07;
         if ((ep0SetupBuffer.wValue != ENDPOINT_HALT) || // Not Endpoint Stall ?
               (epNum >= UsbImplementation::NUMBER_OF_ENDPOINTS))  {                   // or illegal EP# (ignores direction)
            break;
         }
         assert(endPoints[epNum] != nullptr);
         endPoints[epNum]->stall();
         okResponse = true;
      }
      break;

      default : // Illegal
         break;
   }
   if (okResponse) {
      ep0TxStatus(); // Tx empty Status packet
   }
   else {
      controlEndpoint.stall();
   }
}

/**
 * Get Descriptor - Device Request 0x06
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleGetDescriptor() {
   unsigned        descriptorIndex = ep0SetupBuffer.wValue.lo();
   uint16_t        dataSize = 0;
   const uint8_t  *dataPtr = nullptr;

//   PRINTF("handleGetDescriptor(%d:%d)\n", ep0SetupBuffer.wValue.hi(), ep0SetupBuffer.wValue.lo());

   if (ep0SetupBuffer.bmRequestType != (EP_IN|RT_DEVICE)) {
      // Must be In,Standard,Device
      controlEndpoint.stall();
      return;
   }
   switch (ep0SetupBuffer.wValue.hi()) {

      case DT_DEVICE: // Get Device Descriptor - 1
         //      PUTS("getDescriptor-device - ");
         dataPtr  = (uint8_t *) &UsbImplementation::deviceDescriptor;
         dataSize = sizeof(UsbImplementation::deviceDescriptor);
         break;
      case DT_CONFIGURATION: // Get Configuration Descriptor - 2
         //      PUTS("getDescriptor-config - ");
         if (ep0SetupBuffer.wValue.lo() != 0) {
            controlEndpoint.stall();
            return;
         }
         dataPtr  = (uint8_t *) &UsbImplementation::otherDescriptors;
         dataSize = sizeof(UsbImplementation::otherDescriptors);
         break;
      case DT_DEVICEQUALIFIER: // Get Device Qualifier Descriptor
         //      PUTS("getDescriptor-deviceQ - ");
         controlEndpoint.stall();
         return;
      case DT_STRING: // Get String Desc.- 3
         //      PRINTF("getDescriptor-string - %d\n", descriptorIndex);
#ifdef MS_COMPATIBLE_ID_FEATURE
         if (descriptorIndex == 0xEE) {
            //         PUTS("getDescriptor-string - MS_COMPATIBLE_ID_FEATURE");
            dataPtr  = ms_osStringDescriptor;
            dataSize = *dataPtr;
            break;
         }
#endif
         if (descriptorIndex >= sizeof(UsbImplementation::stringDescriptors)/sizeof(UsbImplementation::stringDescriptors[0])) {
            // Illegal string index - stall
            controlEndpoint.stall();
            return;
         }
         if (descriptorIndex == 0) {
            // Language bytes (unchanged)
            dataPtr  = UsbImplementation::stringDescriptors[0];
         }
#if defined(UNIQUE_ID)
         else if (descriptorIndex == UsbImplementation::s_serial_index) {
            uint8_t utf8Buff[sizeof(SERIAL_NO)+10];

            // Generate Semi-unique Serial number
            uint32_t uid = SIM->UIDH^SIM->UIDMH^SIM->UIDML^SIM->UIDL;
            snprintf((char *)utf8Buff, sizeof(utf8Buff), SERIAL_NO, uid);

            // Use end-point internal buffer directly - may result in truncation
            dataPtr = controlEndpoint.getBuffer();
            utf8ToStringDescriptor(controlEndpoint.getBuffer(), utf8Buff, controlEndpoint.BUFFER_SIZE);
         }
#endif
         else {
            // Use end-point internal buffer directly - may result in truncation
            dataPtr = controlEndpoint.getBuffer();
            utf8ToStringDescriptor(controlEndpoint.getBuffer(), UsbImplementation::stringDescriptors[descriptorIndex], controlEndpoint.BUFFER_SIZE);
         }
         dataSize = *dataPtr;
         break;
      default:
         // Shouldn't happen
         controlEndpoint.stall();
         return;
   } // switch

   // Set up transmission
   ep0StartTxTransaction( dataSize, dataPtr );
}

/**
 * Set Configuration - Device Request 0x09
 * Treated as soft reset
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleSetConfiguration() {
   if ((ep0SetupBuffer.bmRequestType != (EP_OUT|RT_DEVICE)) || // Out,Standard,Device
         ((ep0SetupBuffer.wValue.lo() != 0) &&       // Only supports 0=> un-configure, 1=> only valid configuration
               (ep0SetupBuffer.wValue.lo() != UsbImplementation::CONFIGURATION_NUM))) {
      controlEndpoint.stall();
      return;
   }
   setUSBconfiguredState(ep0SetupBuffer.wValue.lo());

   // Initialise end-points via derived class
   UsbImplementation::initialiseEndpoints();

   // Tx empty Status packet
   ep0TxStatus();
}

/**
 * Set interface - Device Request 0x0B
 * Not required to be implemented
 */
template<class Info, int EP0_SIZE>
void UsbBase_T<Info, EP0_SIZE>::handleSetInterface() {
   PUTS("setInterface");

   if ((ep0SetupBuffer.bmRequestType != (EP_OUT|RT_INTERFACE)) || // NOT In,Standard,Interface
         (ep0SetupBuffer.wLength != 0) ||                         // NOT correct length
         (connectionState != USBconfigured)) {                  // NOT in addressed state
      controlEndpoint.stall(); // Error
      return;
   }
   // Only support one Alternate Setting == 0
   if (ep0SetupBuffer.wValue != 0) {
      controlEndpoint.stall(); // Error
      return;
   }
   for (int epNum=1; epNum<UsbImplementation::NUMBER_OF_ENDPOINTS; epNum++) {
      // Reset DATA0/1 toggle
      endPoints[epNum]->clearStall();
   }
   // Transmit empty Status packet
   ep0TxStatus();
}

}; // namespace
#endif /* PROJECT_HEADERS_USB_H_ */
