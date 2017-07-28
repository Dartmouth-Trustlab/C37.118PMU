#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2017 anatharaman <anatharaman@CSL-C14997>
#
# Distributed under terms of the MIT license.

"""

"""
import struct
import pcap
import dpkt
from dpkt.ip import IP
from dpkt.tcp import TCP

file = open("datadump1","wb")
for ts,raw_packet in pcap.pcap("C37.118_1PMU_TCP.pcap"):
    ip = IP(raw_packet[14:])
    tcp = ip.data
    if len(tcp.data) != 0:
        byte = ":".join("{:02x}".format(ord(c)) for c in tcp.data)
        file.write(bytearray(tcp.data + "\n"))
file.close()
