/**
 * @file      hardware.cpp (generated from MK22D5.usbdmHardware)
 * @version   1.2.0
 * @brief     Pin declarations for MK22DX256VLF5
 *
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */

#include "hardware.h"

namespace USBDM {

/**
 * Used to configure pin-mapping before 1st use of peripherals
 */
void mapAllPins() {
      enablePortClocks(PORTA_CLOCK_MASK|PORTB_CLOCK_MASK|PORTC_CLOCK_MASK|PORTD_CLOCK_MASK);

      ((PORT_Type *)PORTA_BasePtr)->GPCHR = 0x0000UL|PORT_GPCHR_GPWE(0x000CUL);
      ((PORT_Type *)PORTA_BasePtr)->GPCLR = 0x0703UL|PORT_GPCLR_GPWE(0x0009UL);
      ((PORT_Type *)PORTB_BasePtr)->GPCLR = 0x0000UL|PORT_GPCLR_GPWE(0x0004UL);
      ((PORT_Type *)PORTB_BasePtr)->GPCLR = 0x0100UL|PORT_GPCLR_GPWE(0x0009UL);
      ((PORT_Type *)PORTB_BasePtr)->GPCHR = 0x0303UL|PORT_GPCHR_GPWE(0x0003UL);
      ((PORT_Type *)PORTC_BasePtr)->GPCLR = 0x0000UL|PORT_GPCLR_GPWE(0x0080UL);
      ((PORT_Type *)PORTC_BasePtr)->GPCLR = 0x0103UL|PORT_GPCLR_GPWE(0x0013UL);
      ((PORT_Type *)PORTC_BasePtr)->GPCLR = 0x0203UL|PORT_GPCLR_GPWE(0x0064UL);
      ((PORT_Type *)PORTC_BasePtr)->GPCLR = 0x0403UL|PORT_GPCLR_GPWE(0x0008UL);
      ((PORT_Type *)PORTD_BasePtr)->GPCLR = 0x0100UL|PORT_GPCLR_GPWE(0x0080UL);
      ((PORT_Type *)PORTD_BasePtr)->GPCLR = 0x0102UL|PORT_GPCLR_GPWE(0x0002UL);
      ((PORT_Type *)PORTD_BasePtr)->GPCLR = 0x0103UL|PORT_GPCLR_GPWE(0x0001UL);
      ((PORT_Type *)PORTD_BasePtr)->GPCLR = 0x0203UL|PORT_GPCLR_GPWE(0x0008UL);
      ((PORT_Type *)PORTD_BasePtr)->GPCLR = 0x0403UL|PORT_GPCLR_GPWE(0x0050UL);
}

} // End namespace USBDM

