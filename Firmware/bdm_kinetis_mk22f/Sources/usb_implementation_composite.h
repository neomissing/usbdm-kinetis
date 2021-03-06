/**
 * @file     usb_implementation_composite.h
 * @brief    USB Kinetis implementation
 *
 * @version  V4.12.1.150
 * @date     13 Nov 2016
 *
 *  This file provides the implementation specific code for the USB interface.
 *  It will need to be modified to suit an application.
 */
#ifndef PROJECT_HEADERS_USB_IMPLEMENTATION_H_
#define PROJECT_HEADERS_USB_IMPLEMENTATION_H_

/*
 * Under Windows 8, or 10 there is no need to install a driver for
 * the bulk end-points if the MS_COMPATIBLE_ID_FEATURE is enabled.
 * winusb.sys driver will be automatically loaded.
 *
 * Under Windows 10 the usbser.sys driver will be loaded automatically
 * for the CDC (serial) interface
 *
 * Under Linux drivers for bulk and CDC are automatically loaded
 */
#define MS_COMPATIBLE_ID_FEATURE
#include "usb_cdc_uart.h"

#define UNIQUE_ID
#include "configure.h"

#include "queue.h"

namespace USBDM {

//======================================================================
// Customise for each USB device
//

/** Causes a semi-unique serial number to be generated for each USB device */
#define UNIQUE_ID

#ifndef SERIAL_NO
#ifdef UNIQUE_ID
#define SERIAL_NO           "USBDM-%lu"
#else
#define SERIAL_NO           "USBDM-0001"
#endif
#endif
#ifndef PRODUCT_DESCRIPTION
#define PRODUCT_DESCRIPTION "USB ARM"
#endif
#ifndef MANUFACTURER
#define MANUFACTURER        "pgo"
#endif

#ifndef VENDOR_ID
#define VENDOR_ID  (0x16D0)
#endif
#ifndef PRODUCT_ID
#define PRODUCT_ID (0xFFFF)
#endif
#ifndef VERSION_ID
#define VERSION_ID (0x0100)
#endif

//======================================================================
// Maximum packet sizes for each endpoint
//
static constexpr uint  CONTROL_EP_MAXSIZE           = 64; //!< Control in/out    64

static constexpr uint  BULK_OUT_EP_MAXSIZE          = 64; //!< Bulk out          64
static constexpr uint  BULK_IN_EP_MAXSIZE           = 64; //!< Bulk in           64

static constexpr uint  CDC_NOTIFICATION_EP_MAXSIZE  = 16; //!< CDC notification  16
static constexpr uint  CDC_DATA_OUT_EP_MAXSIZE      = 16; //!< CDC data out      16
static constexpr uint  CDC_DATA_IN_EP_MAXSIZE       = 16; //!< CDC data in       16

#ifdef USBDM_USB0_IS_DEFINED
/**
 * Class representing USB0
 */
class Usb0 : public UsbBase_T<Usb0Info, CONTROL_EP_MAXSIZE> {

   // Select UART to use
   using Uart = CdcUart<Uart1Info>;

   // Allow superclass to access handleTokenComplete(void);
   friend UsbBase_T<Usb0Info, CONTROL_EP_MAXSIZE>;

public:
   /**
    * String indexes
    *
    * Must agree with stringDescriptors[] order
    */
   enum StringIds {
      /** Language information for string descriptors */
      s_language_index=0,    // Must be zero
      /** Manufacturer */
      s_manufacturer_index,
      /** Product Description */
      s_product_index,
      /** Serial Number */
      s_serial_index,
      /** Configuration Index */
      s_config_index,

      /** Name of Bulk interface */
      s_bulk_interface_index,

      /** Name of CDC interface */
      s_cdc_interface_index,
      /** CDC Control Interface */
      s_cdc_control_interface_index,
      /** CDC Data Interface */
      s_cdc_data_Interface_index,

      /** Marks last entry */
      s_number_of_string_descriptors
   };

   /**
    * Endpoint numbers\n
    * Must be consecutive
    */
   enum EndpointNumbers {
      /** USB Control endpoint number - must be zero */
      CONTROL_ENDPOINT  = 0,

      /* end-points are assumed consecutive */

      /** Bulk out endpoint number */
      BULK_OUT_ENDPOINT,
      /** Bulk in endpoint number */
      BULK_IN_ENDPOINT,

      /** CDC Control endpoint number */
      CDC_NOTIFICATION_ENDPOINT,
      /** CDC Data out endpoint number */
      CDC_DATA_OUT_ENDPOINT,
      /** CDC Data in endpoint number */
      CDC_DATA_IN_ENDPOINT,

      /** Total number of end-points */
      NUMBER_OF_ENDPOINTS,
   };

   /**
    * Configuration numbers, consecutive from 1
    */
   enum Configurations {
     CONFIGURATION_NUM = 1,
     /*
      * Assumes single configuration
      */
     /** Total number of configurations */
     NUMBER_OF_CONFIGURATIONS = CONFIGURATION_NUM,
   };

   /**
    * String descriptor table
    */
   static const uint8_t *const stringDescriptors[];

protected:
   /* end-points */
   static OutEndpoint <Usb0Info, Usb0::BULK_OUT_ENDPOINT, BULK_OUT_EP_MAXSIZE> epBulkOut;
   static InEndpoint  <Usb0Info, Usb0::BULK_IN_ENDPOINT,  BULK_IN_EP_MAXSIZE>  epBulkIn;

   static InEndpoint  <Usb0Info, Usb0::CDC_NOTIFICATION_ENDPOINT, CDC_NOTIFICATION_EP_MAXSIZE>  epCdcNotification;
   static OutEndpoint <Usb0Info, Usb0::CDC_DATA_OUT_ENDPOINT,     CDC_DATA_OUT_EP_MAXSIZE>      epCdcDataOut;
   static InEndpoint  <Usb0Info, Usb0::CDC_DATA_IN_ENDPOINT,      CDC_DATA_IN_EP_MAXSIZE>       epCdcDataIn;

   /** Force command handler to exit and restart */
   static bool forceCommandHandlerInitialise;

public:

   /**
    * Initialise the USB interface
    */
   static void initialise();

   /**
    *  Blocking transmission of data over bulk IN end-point
    *
    *  @param size   Number of bytes to send
    *  @param buffer Pointer to bytes to send
    *
    *  @note : Waits for idle BEFORE transmission but\n
    *  returns before data has been transmitted
    */
   static void sendBulkData(const uint8_t size, const uint8_t *buffer);

   /**
    *  Blocking reception of data over bulk OUT end-point
    *
    *   @param maxSize Maximum number of bytes to receive
    *   @param buffer  Pointer to buffer for bytes received
    *
    *   @return Number of bytes received
    *
    *   @note Doesn't return until command has been received.
    */
   static int receiveBulkData(uint8_t maxSize, uint8_t *buffer);

   /**
    * CDC Transmit
    *
    * @param data Pointer to data to transmit
    * @param size Number of bytes to transmit
    */
   static void sendCdcData(const uint8_t *data, unsigned size);

   /**
    * CDC Receive
    *
    * @param data    Pointer to data to receive
    * @param maxSize Maximum number of bytes to receive
    *
    * @return Number of bytes received
    */
   static int receiveCdcData(uint8_t *data, unsigned maxSize);

   static bool putCdcChar(uint8_t ch);

   /**
    * Device Descriptor
    */
   static const DeviceDescriptor deviceDescriptor;

   /**
    * Other descriptors type
    */
   struct Descriptors {
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
   };

   /**
    * All other descriptors
    */
   static const Descriptors otherDescriptors;

protected:
   /**
    * Initialises all end-points
    */
   static void initialiseEndpoints(void) {
      epBulkOut.initialise();
      addEndpoint(&epBulkOut);
      epBulkOut.setCallback(bulkOutTransactionCallback);

      epBulkIn.initialise();
      addEndpoint(&epBulkIn);
      epBulkIn.setCallback(bulkInTransactionCallback);

      epCdcNotification.initialise();
      addEndpoint(&epCdcNotification);

      epCdcDataOut.initialise();
      addEndpoint(&epCdcDataOut);
      epCdcDataOut.setCallback(cdcOutTransactionCallback);

      // Make sure epCdcDataOut is ready for polling (OUT)
      epCdcDataOut.startRxTransaction(EPDataOut, epCdcDataOut.BUFFER_SIZE);

      epCdcDataIn.initialise();
      addEndpoint(&epCdcDataIn);
      epCdcDataIn.setCallback(cdcInTransactionCallback);

      // Start CDC status transmission
      epCdcSendNotification();

      static const uint8_t cdcInBuff[] = "Hello there\n";
      epCdcDataIn.startTxTransaction(EPDataIn, sizeof(cdcInBuff), cdcInBuff);
   }

   /**
    * Callback for SOF tokens
    */
   static void sofCallback();

   /**
    * Call-back handling BULK-OUT transaction complete
    */
   static void bulkOutTransactionCallback(EndpointState state);

   /**
    * Call-back handling BULK-IN transaction complete
    */
   static void bulkInTransactionCallback(EndpointState state);

   /**
    * Call-back handling CDC-INtransaction complete
    */
   static void cdcInTransactionCallback(EndpointState state);

   /**
    * Call-back handling CDC-OUT transaction complete
    */
   static void cdcOutTransactionCallback(EndpointState state);

   /**
    * Handler for Token Complete USB interrupts for\n
    * end-points other than EP0
    */
   static void handleTokenComplete(void);

   /**
    * Start CDC IN transaction\n
    * A packet is only sent if data is available
    */
   static void startCdcIn();

   /**
    * Configure epCdcNotification for an IN transaction [Tx, device -> host, DATA0/1]
    */
   static void epCdcSendNotification();

   /**
    * Handle SETUP requests not handled by base handler
    *
    * @param setup SETUP packet received from host
    *
    * @note Provides CDC extensions
    * @note Provides BDM extensions
    */
   static void handleUserEp0SetupRequests(const SetupPacket &setup);

   /**
    * CDC Set line coding handler
    */
   static void handleSetLineCoding();

   /**
    * CDC Get line coding handler
    */
   static void handleGetLineCoding();

   /**
    * CDC Set line state handler
    */
   static void handleSetControlLineState();

   /**
    * CDC Send break handler
    */
   static void handleSendBreak();

};

using UsbImplementation = Usb0;

#endif // USBDM_USB0_IS_DEFINED

} // End namespace USBDM

#endif /* PROJECT_HEADERS_USB_IMPLEMENTATION_H_ */
