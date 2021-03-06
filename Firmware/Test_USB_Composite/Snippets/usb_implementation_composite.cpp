/*
 * usb_implementation_composite.cpp
 *
 *  Created on: 30Oct.,2016
 *      Author: podonoghue
 */
#include <string.h>

#include "usb.h"
#include "usb_cdc_uart.h"

namespace USBDM {

enum InterfaceNumbers {
   /** Interface number for BDM channel */
   BULK_INTF_ID,

   /** Interface number for CDC Control channel */
   CDC_COMM_INTF_ID,
   /** Interface number for CDC Data channel */
   CDC_DATA_INTF_ID,
   /*
    * TODO Add additional Interface numbers here
    */
   /** Total number of interfaces */
   NUMBER_OF_INTERFACES,
};

/*
 * String descriptors
 */
static const uint8_t s_language[]        = {4, DT_STRING, 0x09, 0x0C};  //!< Language IDs
static const uint8_t s_manufacturer[]    = MANUFACTURER;                //!< Manufacturer
static const uint8_t s_product[]         = PRODUCT_DESCRIPTION;         //!< Product Description
static const uint8_t s_serial[]          = SERIAL_NO;                   //!< Serial Number

static const uint8_t s_bulk_interface[]   = "Bulk Interface";           //!< Bulk Interface

static const uint8_t s_cdc_interface[]   = "CDC Interface";             //!< Interface Association #2
static const uint8_t s_cdc_control[]     = "CDC Control Interface";     //!< CDC Control Interface
static const uint8_t s_cdc_data[]        = "CDC Data Interface";        //!< CDC Data Interface
/*
 * Add additional String descriptors here
 */

/**
 * String descriptor table
 */
const uint8_t *const Usb0::stringDescriptors[] = {
      s_language,
      s_manufacturer,
      s_product,
      s_serial,
	  
      s_bulk_interface,
      
      s_cdc_interface,
      s_cdc_control,
      s_cdc_data
      /*
       * Add additional String descriptors here
       */
};

/**
 * Device Descriptor
 */
const DeviceDescriptor Usb0::deviceDescriptor = {
      /* bLength             */ sizeof(DeviceDescriptor),
      /* bDescriptorType     */ DT_DEVICE,
      /* bcdUSB              */ nativeToLe16(0x0200),           // USB specification release No. [BCD = 2.00]
      /* bDeviceClass        */ 0xEF,                    // Device Class code [Miscellaneous Device Class]
      /* bDeviceSubClass     */ 0x02,                    // Sub Class code    [Common Class]
      /* bDeviceProtocol     */ 0x01,                    // Protocol          [Interface Association Descriptor]
      /* bMaxPacketSize0     */ CONTROL_EP_MAXSIZE,             // EndPt 0 max packet size
      /* idVendor            */ nativeToLe16(VENDOR_ID),        // Vendor ID
      /* idProduct           */ nativeToLe16(PRODUCT_ID),       // Product ID
      /* bcdDevice           */ nativeToLe16(VERSION_ID),       // Device Release    [BCD = 4.10]
      /* iManufacturer       */ s_manufacturer_index,           // String index of Manufacturer name
      /* iProduct            */ s_product_index,                // String index of product description
      /* iSerialNumber       */ s_serial_index,                 // String index of serial number
      /* bNumConfigurations  */ NUMBER_OF_CONFIGURATIONS        // Number of configurations
};

/**
 * Other descriptors type
 */
struct Usb0::Descriptors {
   ConfigurationDescriptor                  configDescriptor;

   InterfaceDescriptor                      bulk_interface;
   EndpointDescriptor                       bulk_out_endpoint;
   EndpointDescriptor                       bulk_in_endpoint;
   
   InterfaceAssociationDescriptor           interfaceAssociationDescriptorCDC;
   InterfaceDescriptor                      cdc_CCI_Interface;
   CDCHeaderFunctionalDescriptor            cdc_Functional_Header;
   CDCCallManagementFunctionalDescriptor    cdc_CallManagement;
   CDCAbstractControlManagementDescriptor   cdc_Functional_ACM;
   CDCUnionFunctionalDescriptor             cdc_Functional_Union;
   EndpointDescriptor                       cdc_notification_Endpoint;

   InterfaceDescriptor                      cdc_DCI_Interface;
   EndpointDescriptor                       cdc_dataOut_Endpoint;
   EndpointDescriptor                       cdc_dataIn_Endpoint;
   /*
    * TODO Add additional Descriptors here
    */
};

/**
 * All other descriptors
 */
const Usb0::Descriptors Usb0::otherDescriptors = {
      { // configDescriptor
            /* bLength                 */ sizeof(ConfigurationDescriptor),
            /* bDescriptorType         */ DT_CONFIGURATION,
            /* wTotalLength            */ nativeToLe16(sizeof(otherDescriptors)),
            /* bNumInterfaces          */ NUMBER_OF_INTERFACES,
            /* bConfigurationValue     */ CONFIGURATION_NUM,
            /* iConfiguration          */ 0,
            /* bmAttributes            */ 0x80,     //  = Bus powered, no wake-up
            /* bMaxPower               */ USBMilliamps(500)
      },
      /**
       * Bulk interface, 2 end-points
       */
      { // bulk_interface
            /* bLength                 */ sizeof(InterfaceDescriptor),
            /* bDescriptorType         */ DT_INTERFACE,
            /* bInterfaceNumber        */ BULK_INTF_ID,
            /* bAlternateSetting       */ 0,
            /* bNumEndpoints           */ 2,
            /* bInterfaceClass         */ 0xFF,                         // (Vendor specific)
            /* bInterfaceSubClass      */ 0xFF,                         // (Vendor specific)
            /* bInterfaceProtocol      */ 0xFF,                         // (Vendor specific)
            /* iInterface desc         */ s_bulk_interface_index,
      },
      { // bulk_out_endpoint - OUT, Bulk
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_OUT|BULK_OUT_ENDPOINT,
            /* bmAttributes            */ ATTR_BULK,
            /* wMaxPacketSize          */ nativeToLe16(BULK_OUT_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(1)
      },
      { // bulk_in_endpoint - IN, Bulk
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_IN|BULK_IN_ENDPOINT,
            /* bmAttributes            */ ATTR_BULK,
            /* wMaxPacketSize          */ nativeToLe16(BULK_IN_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(1)
      },
      { // interfaceAssociationDescriptorCDC
            /* bLength                 */ sizeof(InterfaceAssociationDescriptor),
            /* bDescriptorType         */ DT_INTERFACEASSOCIATION,
            /* bFirstInterface         */ CDC_COMM_INTF_ID,
            /* bInterfaceCount         */ 2,
            /* bFunctionClass          */ 0x02,                                   //  CDC Control
            /* bFunctionSubClass       */ 0x02,                                   //  Abstract Control Model
            /* bFunctionProtocol       */ 0x01,                                   //  AT CommandL V.250
            /* iFunction = ""          */ s_cdc_interface_index,
      },
      /**
       * CDC Control/Communication Interface, 1 end-point
       */
      { // cdc_CCI_Interface
            /* bLength                 */ sizeof(InterfaceDescriptor),
            /* bDescriptorType         */ DT_INTERFACE,
            /* bInterfaceNumber        */ CDC_COMM_INTF_ID,
            /* bAlternateSetting       */ 0,
            /* bNumEndpoints           */ 1,
            /* bInterfaceClass         */ 0x02,      //  CDC Communication
            /* bInterfaceSubClass      */ 0x02,      //  Abstract Control Model
            /* bInterfaceProtocol      */ 0x01,      //  V.25ter, AT Command V.250
            /* iInterface description  */ s_cdc_control_interface_index
      },
      { // cdc_Functional_Header
            /* bFunctionalLength       */ sizeof(CDCHeaderFunctionalDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_HEADER,
            /* bcdCDC                  */ nativeToLe16(0x0110),
      },
      { // cdc_CallManagement
            /* bFunctionalLength       */ sizeof(CDCCallManagementFunctionalDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_CALL_MANAGEMENT,
            /* bmCapabilities          */ 1,
            /* bDataInterface          */ CDC_DATA_INTF_ID,
      },
      { // cdc_Functional_ACM
            /* bFunctionalLength       */ sizeof(CDCAbstractControlManagementDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_ABSTRACT_CONTROL_MANAGEMENT,
            /* bmCapabilities          */ 0x06,
      },
      { // cdc_Functional_Union
            /* bFunctionalLength       */ sizeof(CDCUnionFunctionalDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_UNION_MANAGEMENT,
            /* bmControlInterface      */ CDC_COMM_INTF_ID,
            /* bSubordinateInterface0  */ {CDC_DATA_INTF_ID},
      },
      { // cdc_notification_Endpoint - IN,interrupt
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_IN|CDC_NOTIFICATION_ENDPOINT,
            /* bmAttributes            */ ATTR_INTERRUPT,
            /* wMaxPacketSize          */ nativeToLe16(CDC_NOTIFICATION_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(255)
      },
      /**
       * CDC Data Interface, 2 end-points
       */
      { // cdc_DCI_Interface
            /* bLength                 */ sizeof(InterfaceDescriptor),
            /* bDescriptorType         */ DT_INTERFACE,
            /* bInterfaceNumber        */ CDC_DATA_INTF_ID,
            /* bAlternateSetting       */ 0,
            /* bNumEndpoints           */ 2,
            /* bInterfaceClass         */ 0x0A,                         //  CDC DATA
            /* bInterfaceSubClass      */ 0x00,                         //  -
            /* bInterfaceProtocol      */ 0x00,                         //  -
            /* iInterface description  */ s_cdc_data_Interface_index
      },
      { // cdc_dataOut_Endpoint - OUT, Bulk
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_OUT|CDC_DATA_OUT_ENDPOINT,
            /* bmAttributes            */ ATTR_BULK,
            /* wMaxPacketSize          */ nativeToLe16(CDC_DATA_OUT_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(1)
      },
      { // cdc_dataIn_Endpoint - IN, Bulk
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_IN|CDC_DATA_IN_ENDPOINT,
            /* bmAttributes            */ ATTR_BULK,
            /* wMaxPacketSize          */ nativeToLe16(2*CDC_DATA_IN_EP_MAXSIZE), // x2 so all packets are terminating (short))
            /* bInterval               */ USBMilliseconds(1)
      },
      /*
       * TODO Add additional Descriptors here
       */
};

/**
 * Handler for Start of Frame Token interrupt (~1ms interval)
 */
void Usb0::sofCallback() {
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
//               UsbLed::on();
            }
            else {
               // Activity LED off when no USB connection
//               UsbLed::off();
            }
            break;
         case 1:
         case 2:
            break;
         case 3:
         default :
            if (activityFlag) {
               // Activity LED flashes
//               UsbLed::toggle();
               setActive(false);
            }
            break;
      }
   }
}

/**
 * Configure epCdcNotification for an IN transaction [Tx, device -> host, DATA0/1]
 */
void Usb0::epCdcCheckStatus() {
   const CDCNotification cdcNotification= {CDC_NOTIFICATION, SERIAL_STATE, 0, RT_INTERFACE, nativeToLe16(2)};
   uint8_t status = CdcUart::getSerialState().bits;

   if ((status & CdcUart::CDC_STATE_CHANGE_MASK) == 0) {
      // No change
      epCdcNotification.getHardwareState().state = EPIdle;
      return;
   }
   static_assert(epCdcNotification.BUFFER_SIZE>=sizeof(CDCNotification), "Buffer size insufficient");

   // Copy the Tx data to Tx buffer
   (void)memcpy(epCdcNotification.getBuffer(), &cdcNotification, sizeof(cdcNotification));
   epCdcNotification.getBuffer()[sizeof(cdcNotification)+0] = status&~CdcUart::CDC_STATE_CHANGE_MASK;
   epCdcNotification.getBuffer()[sizeof(cdcNotification)+1] = 0;

   // Set up to Tx packet
//   PRINTF("epCdcCheckStatus()\n");
   epCdcNotification.startTxTransaction(sizeof(cdcNotification)+2, nullptr, EPDataIn);
}

static uint8_t cdcOutBuff[10] = "Welcome\n";
static int cdcOutByteCount    = 8;

void Usb0::startCdcIn() {
   if ((epCdcDataIn.getHardwareState().state == EPIdle) && (cdcOutByteCount>0)) {
      static_assert(epCdcDataIn.BUFFER_SIZE>sizeof(cdcOutBuff), "Buffer too small");
      memcpy(epCdcDataIn.getBuffer(), cdcOutBuff, cdcOutByteCount);
      epCdcDataIn.startTxTransaction(cdcOutByteCount, nullptr, EPDataIn);
      cdcOutByteCount = 0;
   }
}
/**
 * Handler for Token Complete USB interrupts for
 * end-points other than EP0
 */
void Usb0::handleTokenComplete(void) {

   // Status from Token
   uint8_t   usbStat  = usb->STAT;

   // Endpoint number
   uint8_t   endPoint = ((uint8_t)usbStat)>>4;

   switch (endPoint) {
      case BULK_OUT_ENDPOINT: // Accept OUT token
         setActive();
         epBulkOut.flipOddEven(usbStat);
         epBulkOut.handleOutToken();
         return;
      case BULK_IN_ENDPOINT: // Accept IN token
         epBulkIn.flipOddEven(usbStat);
         epBulkIn.handleInToken();
         return;

      case CDC_NOTIFICATION_ENDPOINT: // Accept IN token
//         PRINTF("CDC_NOTIFICATION_ENDPOINT\n");
         epCdcNotification.flipOddEven(usbStat);
         epCdcCheckStatus();
         return;
      case CDC_DATA_OUT_ENDPOINT: // Accept OUT token
//         PRINTF("CDC_DATA_OUT_ENDPOINT\n");
         epCdcDataOut.flipOddEven(usbStat);
         epCdcDataOut.handleOutToken();
         return;
      case CDC_DATA_IN_ENDPOINT:  // Accept IN token
//         PRINTF("CDC_DATA_IN_ENDPOINT\n");
         epCdcDataIn.flipOddEven(usbStat);
         epCdcDataIn.handleInToken();
         return;
      /*
       * TODO Add additional End-point handling here
       */
   }
}

/**
 * Call-back handling CDC-OUT transaction complete
 */
void Usb0::cdcOutCallback(EndpointState state) {
   static uint8_t buff[] = "";
   PRINTF("%c\n", buff[0]);
   if (state == EPDataOut) {
      epCdcDataOut.startRxTransaction(sizeof(buff), buff, EPDataOut);
   }
}

/**
 * Call-back handling CDC-IN transaction complete
 */
void Usb0::cdcInCallback(EndpointState state) {
   static const uint8_t buff[] = "Hello There\n\r";
   if (state == EPDataIn) {
      epCdcDataIn.startTxTransaction(sizeof(buff), buff, EPDataIn);
   }
}

/**
 * Call-back handling BULK-OUT transaction complete
 */
void Usb0::bulkOutCallback(EndpointState state) {
   static uint8_t buff[] = "";
   PRINTF("%c\n", buff[0]);
   if (state == EPDataOut) {
      epBulkOut.startRxTransaction(sizeof(buff), buff, EPDataOut);
   }
}

/**
 * Call-back handling BULK-IN transaction complete
 */
void Usb0::bulkInCallback(EndpointState state) {
   static const uint8_t buff[] = "Hello There\n\r";
   if (state == EPDataIn) {
      epBulkIn.startTxTransaction(sizeof(buff), buff, EPDataIn);
   }
}

/**
 * Initialise the USB0 interface
 *
 *  @note Assumes clock set up for USB operation (48MHz)
 */
void Usb0::initialise() {
   UsbBase_T::initialise();

   // Add extra handling of CDC packets directed to EP0
   setUnhandledSetupCallback(handleCdcEp0);
   /*
    * TODO Additional initialisation
    */
}

/**
 * Handler for USB0 interrupt
 *
 * Determines source and dispatches to appropriate routine.
 */
void Usb0::irqHandler() {
   // All active flags
   uint8_t interruptFlags = usb->ISTAT;

   //   if (interruptFlags&~USB_ISTAT_SOFTOK_MASK) {
   //      PRINTF("ISTAT=%2X\n", interruptFlags);
   //   }

   // Get active and enabled interrupt flags
   uint8_t enabledInterruptFlags = interruptFlags & usb->INTEN;

   if ((enabledInterruptFlags&USB_ISTAT_USBRST_MASK) != 0) {
      // Reset signaled on Bus
      handleUSBReset();
      usb->ISTAT = USB_ISTAT_USBRST_MASK; // Clear source
      return;
   }
   if ((enabledInterruptFlags&USB_ISTAT_TOKDNE_MASK) != 0) {
      // Token complete interrupt
      UsbBase_T::handleTokenComplete();
      // Clear source
      usb->ISTAT = USB_ISTAT_TOKDNE_MASK;
   }
   else if ((enabledInterruptFlags&USB_ISTAT_RESUME_MASK) != 0) {
      // Resume signaled on Bus
      handleUSBResume();
      // Clear source
      usb->ISTAT = USB_ISTAT_RESUME_MASK;
   }
   else if ((enabledInterruptFlags&USB_ISTAT_STALL_MASK) != 0) {
      // Stall sent
      handleStallComplete();
      // Clear source
      usb->ISTAT = USB_ISTAT_STALL_MASK;
   }
   else if ((enabledInterruptFlags&USB_ISTAT_SOFTOK_MASK) != 0) {
      // SOF Token?
      handleSOFToken();
      usb->ISTAT = USB_ISTAT_SOFTOK_MASK; // Clear source
   }
   else if ((enabledInterruptFlags&USB_ISTAT_SLEEP_MASK) != 0) {
      // Bus Idle 3ms => sleep
      //      PUTS("Suspend");
      handleUSBSuspend();
      // Clear source
      usb->ISTAT = USB_ISTAT_SLEEP_MASK;
   }
   else if ((enabledInterruptFlags&USB_ISTAT_ERROR_MASK) != 0) {
      // Any Error
      PRINTF("Error s=0x%02X\n", usb->ERRSTAT);
      usb->ERRSTAT = 0xFF;
      // Clear source
      usb->ISTAT = USB_ISTAT_ERROR_MASK;
   }
   else  {
      // Unexpected interrupt
      // Clear & ignore
      PRINTF("Unexpected interrupt, flags=0x%02X\n", interruptFlags);
      // Clear & ignore
      usb->ISTAT = interruptFlags;
   }
}

/**
 *  Blocking reception of data over bulk OUT end-point
 *
 *   @param maxSize  = max # of bytes to receive
 *   @param buffer   = ptr to buffer for bytes received
 *
 *   @note : Doesn't return until command has been received.
 */
void Usb0::receiveData(uint8_t maxSize, uint8_t *buffer) {
   // Wait for USB connection
   while(deviceState.state != USBconfigured) {
      __WFI();
   }
   Usb0::enableNvicInterrupts(true);
   epBulkOut.startRxTransaction(maxSize, buffer, EPDataOut);
   while(epBulkOut.getHardwareState().state != EPIdle) {
      __WFI();
   }
}

/**
 *  Blocking transmission of data over bulk IN end-point
 *
 *  @param size   Number of bytes to send
 *  @param buffer Pointer to bytes to send
 *
 *   @note : Waits for idle BEFORE transmission but\n
 *   returns before data has been transmitted
 *
 */
void Usb0::sendData( uint8_t size, const uint8_t *buffer) {
//   commandBusyFlag = false;
   //   enableUSBIrq();
   while (epBulkIn.getHardwareState().state != EPIdle) {
      __WFI();
   }
   epBulkIn.startTxTransaction(size, buffer, EPDataIn);
}

/**
 * CDC Set line coding handler
 */
void Usb0::handleSetLineCoding() {
//   PRINTF("handleSetLineCoding()\n");

   // Call-back to do after transaction complete
   auto callback = [](){
      CdcUart::setLineCoding((LineCodingStructure * const)ep0.getBuffer());
      setSetupCompleteCallback(nullptr);
   };
   setSetupCompleteCallback(callback);

   // Don't use external buffer - this requires response to fit in internal EP buffer
   static_assert(sizeof(LineCodingStructure) < ep0.BUFFER_SIZE, "Buffer insufficient size");
   ep0.startRxTransaction(sizeof(LineCodingStructure), nullptr, EPDataOut);
}

/**
 * CDC Get line coding handler
 */
void Usb0::handleGetLineCoding() {
//   PRINTF("handleGetLineCoding()\n");
   // Send packet
   ep0StartTxTransaction( sizeof(LineCodingStructure), (const uint8_t*)CdcUart::getLineCoding());
}

/**
 * CDC Set line state handler
 */
void Usb0::handleSetControlLineState() {
//   PRINTF("handleSetControlLineState(%X)\n", ep0SetupBuffer.wValue.lo());
   CdcUart::setControlLineState(ep0SetupBuffer.wValue.lo());
   // Tx empty Status packet
   ep0StartTxTransaction( 0, nullptr );
}

/**
 * CDC Send break handler
 */
void Usb0::handleSendBreak() {
//   PRINTF("handleSendBreak()\n");
   CdcUart::sendBreak(ep0SetupBuffer.wValue);
   // Tx empty Status packet
   ep0StartTxTransaction( 0, nullptr );
}

/**
 * CDC EP0 SETUP handler
 */
void Usb0::handleCdcEp0(const SetupPacket &setup) {
   switch(REQ_TYPE(setup.bmRequestType)) {
      case REQ_TYPE_CLASS :
         // Class requests
         switch (setup.bRequest) {
            case SET_LINE_CODING :       handleSetLineCoding();       break;
            case GET_LINE_CODING :       handleGetLineCoding();       break;
            case SET_CONTROL_LINE_STATE: handleSetControlLineState(); break;
            case SEND_BREAK:             handleSendBreak();           break;
            default :                    ep0.stall();                 break;
         }
         break;
      default:
         ep0.stall();
         break;
   }
}

void idleLoop() {
   for(;;) {
      __asm__("nop");
   }
}

} // End namespace USBDM

