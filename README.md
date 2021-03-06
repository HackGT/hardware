# HackGT Hardware
Custom PCBs, electronics, and embedded hardware solutions

PCBs are designed with EAGLE.

Code can be built and flashed with the Arduino IDE with [ATtiny Core](https://github.com/SpenceKonde/ATTinyCore) installed.
* Be sure to configure the right board, clock speed, etc. in the Tools menu
* With new chips or when changing chip settings, select "Burn Bootloader" to set the correct fuses on the microcontroller (or else uploading code won't work)
	* All of the PCBs listed here do not use a bootloader and must have code uploaded over an ISP programmer
* Upload with Sketch -> Upload Using Programmer (*not* regular Upload -- that's only for Arduinos with a bootloader connected over Serial)
	* To flash manually (saves time when flashing many boards because compilation only happens once), Sketch -> Export compiled Binary. This will export a .hex file that you can then upload with `"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -c usbasp -p t167 -U flash:w:sketch_name.hex` where `usbasp` is your ISP programmer type and `t167` is the microcontroller specifier.

## Insight
![Insight PCB](https://644db4de3505c40a0444-327723bce298e3ff5813fb42baeefbaa.ssl.cf1.rackcdn.com/57f27d0ea22f6fa49fed8beb04706f60.png)
![Built Insight PCB](https://i.imgur.com/EuWBPjL.jpg)

* PCB: `PCBs/checkin-tiny/tiny.brd`
* Code: `Code/Insight` and `Code/Insight_Base` (for Arduino MEGA base station)
* Microcontroller: ATtiny167 @ 8 MHz internal clock
* VCC voltage: 3.3V (**Note: this device *requires* a 5V to 3.3V ISP level shifter to be programmed**)
* Features: 915 MHz radio, MFRC522 NFC interface, USB-C rechargeable battery, 20 pin expansion port

Insight is an inexpensive, handheld board designed to be distributed to sponsors at HackGT's events. It contains an NFC reader on one end and a 915 MHz radio on the other. In typical usage, sponsors scan participant badges over NFC, the device relays the decoded participant UUID to the base station (see below), which pops the participant's information (e.g. name, resume, favorite programming languages) up on the sponsor's phone or laptop. See [HackGT Insight](https://github.com/HackGT/insight) for more information. A 915 MHz radio is used instead of Wi-Fi due to reduced cost, reduced microcontroller requirements, and so that it will have available usable bandwidth even with thousands of 2.4 GHz radios (Wi-Fi / Bluetooth) active during a HackGT event.

The base station consists of an Arduino MEGA with an Ethernet Shield and an [Adafruit RFM69HCW radio module](https://www.adafruit.com/product/3070) added.

### Tech details:
* Power is *always* supplied from the 18650 Lithium Ion battery. This means that a battery must be installed for the device to work -- even if the USB-C connection is plugged in.
* The device **does not have reverse polarity protection**. Always double check the orientation of the Li-Ion battery.
* Voltage is stepped down from the battery's 4.2 volts (max) to 3.5 volts (min) to 3.3 volts VCC by a [linear low-dropout regulator](http://www.ti.com/lit/ds/symlink/tps736.pdf). This regulator drops out at 3.5 volts and can supply up to 400 mA of current.
* USB charging occurs at up to 500 mA (limit of charging IC chosen).
* MFRC522 (NFC) and RFM69HCW (radio) are connected to the ATtiny167 over the chip's hardware SPI bus. The SPI chip select lines are pulled high to deselect them when the ATtiny is programmed over ISP.
* A voltage divider with 4.7K ohm and 10K ohm resistors is used to measure the current battery level from the ATtiny's ADC. This read 10-bit value can then be easily converted to a voltage and/or battery percentage. See `Insight` and `Insight_Base` for the code that does this.
* A TTL serial header is exposed on the bottom edge of the PCB for debugging use. This header does not need to be soldered for final production devices.

### 20-pin expansion header
This device supports shields / daughterboard expansions using the 20-pin expansion header.

With board oriented as seen above:
```
[ 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 ]
[ 1 | 3 | 5 | 7 | 9  | 11 | 13 | 15 | 17 | 19 ]
```

Pin | Signal
----|-------
1 | V_USB (5 volts when USB plugged in, can also be used to charge battery)
2 | V3.3 (3.3 volts when switched on)
3 | RF_IRQ (radio to microcontroller interrupt)
4 | GND
5 | RX (TTL receive)
6 | RED (red status LED)
7 | TX (TTL transmit)
8 | GREEN (green status LED)
9 | BAT_REF (raw voltage divided battery reference, 3.3V safe)
10 | SPEAKER (connected to piezo buzzer, use with `tone()`)
11 | AUX0 (reserved for future use)
12 | NFC_RST (MRFC522 reset)
13 | CE0 (radio SPI chip select)
14 | CE1 (NFC SPI chip select)
15 | MISO (SPI / ISP)
16 | V_USB
17 | SCK (SPI / ISP)
18 | MOSI (SPI/ ISP)
19 | RST (ATtiny reset, ISP)
20 | GND

## ISP 5V to 3.3V Level Shifter
![ISP level shifter PCB](https://644db4de3505c40a0444-327723bce298e3ff5813fb42baeefbaa.ssl.cf1.rackcdn.com/6e1d70204c12f836f49d06b795b98a10.png)

* PCB: `PCBs/checkin-tiny/shifter.brd`
* Code: N/A
* Primary IC: ‎SN74LVC245AN logic level shifter
* VCC voltage: 5V input, 3.3V output

Nearly all HackGT hardware must be programmed over ISP, a protocol used by AVR microcontrollers based on the SPI standard. This can be done with $6 USB programmers such as [this one](https://www.amazon.com/HiLetgo-ATMEGA8-Programmer-USBasp-Cable/dp/B00AX4WQ00). However, note that even though this programmer claims to support 3.3V operation, this only applies to the VCC pin; the signal lines will still operate at 5V. This can damage 5V intolerant devices connected to the SPI bus (e.g. the MFRC522 NFC chip and RFM69HCW radio on Insight).

This tiny board connects to the 6-pin ISP header on the top, shifts the VCC and signal lines down to 3.3 volts, and then outputs these signals on the output ISP header on the bottom. Note that the USB programmer must be outputting 5 volts for this shifter to work correctly so be sure to configure it for 5 volt *not* 3.3 volt output.

**Be sure to double check the correct ISP header orientation when soldering the male/female headers on!**
