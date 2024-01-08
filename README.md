# TinyGPSAsync

## **Preliminary Test Version**

This library is currently only for **testing**.

## A multi-threaded NMEA parser for ESP32

NMEA is the standard format GPS devices use to report location, time, altitude, etc. TinyGPSAsync is a compact, multi-threaded library that parses location, time, altitude, etc., from the common RMC and GGA sentences, but also allows you to extract arbitrary data from ANY NMEA sentences that arrives from your module.

Because TinyGPSAync is multi-threaded, it automatically handles the processing of the inbound NMEA stream. This makes it much easier to write reliable code, because the user doesn't have to manage encoding each character.
