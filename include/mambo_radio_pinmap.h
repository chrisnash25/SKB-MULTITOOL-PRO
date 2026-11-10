#pragma once

// Owner-supplied M5StickC Plus2 / interchangeable CC1101–nRF24
// socket wiring, transcribed from the 10 Nov 2026 screenshot.
// This header RECORDS pin numbers; it does NOT activate RF hardware.
//
// Same socket: CC1101 GDO0 == nRF24 CE (GPIO25), and optional
// CC1101 GDO2 == nRF24 IRQ (not connected).
// Single module only; power off before swapping.
#define MAMBO_RADIO_PIN_CSN 26
#define MAMBO_RADIO_PIN_SCK 0
#define MAMBO_RADIO_PIN_MOSI 32
#define MAMBO_RADIO_PIN_MISO 33
#define MAMBO_RADIO_PIN_CONTROL 25

// GPIO0 is an ESP32 boot-strapping pin. A hardware-enabled build must
// explicitly confirm the boot electrical state is safe. Ordinary builds
// do not define MAMBO_RADIO_HW_VERIFIED.
#if defined(MAMBO_RADIO_HW_VERIFIED) && !defined(MAMBO_GPIO0_BOOT_VERIFIED)
#error "GPIO0 used for SPI SCK: verify boot strapping and define MAMBO_GPIO0_BOOT_VERIFIED explicitly"
#endif
