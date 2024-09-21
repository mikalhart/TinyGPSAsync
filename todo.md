# TODO List

- [ ] Add encode function
- [ ] Implement packets
- [ ] All PVT fields
- [ ] Satellites
- [ ] Basic UBX commands set rate on popular sentences and UBX packets.
- [ ] Colourful output
- [ ] Bug: "Characters dropping" needs to be localised in time
- [ ] Generic Packet "AsUBX" or "AsNMEA"

Things we're interested in
Packets
  Sentences
  UBX packets
  Unknown packets
Satellites seen (GSV)
  List of talker id + PRN + Elevation + Azimuth + SNR
Satellites used (GSA)
  List of talker id + Fix Type, PRN, PDOP, HDOP, VDOP
Snapshot
Stats
UBX Commands
  set hz
  set which sentences and packets
  set rate
  set baud

I. Status
available: packet, sentence, ubx, unknown, satellites, stats, snapshot

II. Latest
  Packet
  Sentence
  Sentence-by-Id/Talker
  UBX-by-class/id
  Snapshot
  Stats
