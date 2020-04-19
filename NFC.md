# HackGT NFC Specification
This specification details the creation and use of NFC staff and participant badges used at HackGT events

## What is NFC?

NFC stands for Near-Field Communication. You've probably used NFC in some form before! It's a common standard for low-speed electronic communication for information that intentionally should not be read at long range.

<img src="https://upload.wikimedia.org/wikipedia/en/thumb/8/83/Universal_Contactless_Card_Symbol.svg/1200px-Universal_Contactless_Card_Symbol.svg.png" height="75" />

- Google Pay / Apple Pay use NFC for payment data exchange with points-of-sale
	- This means that nearly all Android phones and current iPhones have NFC equipment built in (although API support and flexibility varies)
	- Some credit cards also support NFC for small payments
- Student IDs (such as BuzzCards) use NFC for location access
	- Georgia Tech is slowly transitioning their readers to NFC-only which is why newer buildings require a closer tap than the larger, older readers
- Devices such as cameras or headphones can use a NFC tag to automatically turn on and Bluetooth pair when a phone is tapped to the tag
- NFC tags can be embedded into advertising to open up a web link when tapped to a phone

NFC operates at 13.56 MHz and has an intentionally **physically** limited range of **just a few centimeters**. This means that it cannot be read from afar and individual tag reads must be done typically within a 4cm distance maximum.

## Badges

Each HackGT badge issued to participants, staff, sponsors, volunteers, etc. contains an NFC chip. These chips are typically embedded into the badge itself (used for large events like the Fall HackGT, more expensive, more professional looking) or embedded into an NFC sticker which is then applied to the badge (used for smaller events like BuildGT, Horizons, less expensive, takes up visual space on the badge and requires time to apply)

The chip type used in HackGT's NFC badges must be from the NXP NTAG213/215/216 series (for technical reasons described later). All chips from this series operate in the same exact manner. The only difference is the amount of data they can store. For typical HackGT operations, the cheapest option (NTAG213) is perfectly suitable.

## Badge data content

HackGT badges are written with an NDEF (NFC Data Exchange Format) message containing a URI record of the format `https://info.hack.gt?user=<uuid>`. This is a widely supported NDEF record that redirects to the HackGT info site containing the schedule, list of workshops, available resources, etc. if scanned by a typical phone.

This contains the UUID of the user which can be read and used to identify them. This UUID is created by HackGT's authentication system, [Ground Truth](https://github.com/HackGT/ground-truth) and used by [registration](https://github.com/HackGT/registration) and [checkin2](https://github.com/HackGT/checkin2), which many HackGT apps integrate with.

### Example data content

Extracted URI: https://info.hack.gt/?user=4a3b8751-21ce-45e8-874c-5764ecdee1d8

Raw badge data content:

```
[ 04:3F:8C:3F ] Addr. 00 : UID0 - UID2 / BCC0
[ 0A:E4:60:80 ] Addr. 01 : UID3 - UDI6
[ 0E:48:FF:FF ] Addr. 02 : BCC1 / INT. / LOCK0 - LOCK1
[ E1:10:12:0F ] Addr. 03 : OTP0 - OTP3
[ 01:03:A0:0C ] Addr. 04 : DATA
[ 34:03:3C:D1 ] Addr. 05 : DATA
[ 01:38:55:04 ] Addr. 06 : DATA
[ 69:6E:66:6F ] Addr. 07 : DATA
[ 2E:68:61:63 ] Addr. 08 : DATA
[ 6B:2E:67:74 ] Addr. 09 : DATA
[ 2F:3F:75:73 ] Addr. 0A : DATA
[ 65:72:3D:34 ] Addr. 0B : DATA
[ 61:33:62:38 ] Addr. 0C : DATA
[ 37:35:31:2D ] Addr. 0D : DATA
[ 32:31:63:65 ] Addr. 0E : DATA
[ 2D:34:35:65 ] Addr. 0F : DATA
[ 38:2D:38:37 ] Addr. 10 : DATA
[ 34:63:2D:35 ] Addr. 11 : DATA
[ 37:36:34:65 ] Addr. 12 : DATA
[ 63:64:65:65 ] Addr. 13 : DATA
[ 31:64:38:FE ] Addr. 14 : DATA
[ 00:00:00:00 ] Addr. 15 : DATA
[ 00:00:00:00 ] Addr. 16 : DATA
[ 00:00:00:00 ] Addr. 17 : DATA
[ 00:00:00:00 ] Addr. 18 : DATA
[ 00:00:00:00 ] Addr. 19 : DATA
[ 00:00:00:00 ] Addr. 1A : DATA
[ 00:00:00:00 ] Addr. 1B : DATA
[ 00:00:00:00 ] Addr. 1C : DATA
[ 00:00:00:00 ] Addr. 1D : DATA
[ 00:00:00:00 ] Addr. 1E : DATA
[ 00:00:00:00 ] Addr. 1F : DATA
[ 00:00:00:00 ] Addr. 20 : DATA
[ 00:00:00:00 ] Addr. 21 : DATA
[ 00:00:00:00 ] Addr. 22 : DATA
[ 00:00:00:00 ] Addr. 23 : DATA
[ 00:00:00:00 ] Addr. 24 : DATA
[ 00:00:00:00 ] Addr. 25 : DATA
[ 00:00:00:00 ] Addr. 26 : DATA
[ 00:00:00:00 ] Addr. 27 : DATA
[ FF:0F:00:BD ] Addr. 28 : LOCK2 - LOCK4
[ 04:00:00:FF ] Addr. 29 : CFG 0 (MIRROR / AUTH0)
[ 00:05:00:00 ] Addr. 2A : CFG 1 (ACCESS)
[ 00:00:00:00 ] Addr. 2B : PWD0 - PWD3
[ 00:00:00:00 ] Addr. 2C : PACK0 - PACK1
```

## How to parse NFC data

As seen in the data dump above, data on NTAG21x chips is organized into 4-byte blocks called _pages_. Pages 0 through 3 contain read-only information about the NFC tag itself and can be safely ignored. Data begins at page 4.

144 (NTAG213), 504 (NTAG215) or 888 (NTAG216) bytes are available as the freely available user Read/Write area (36, 126 or 222 pages respectively).













***



Content and link format
how to parse links

hardware interfaces
- android
- mfrc522
- usb reader arc122(?)

NTAG213 command protocol

implementations
- C#
- Rust
- TypeScript
- C++ (Arduino)
