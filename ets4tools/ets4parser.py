#!/usr&bin/env python
#
# vim: set encoding=utf-8 tabstop=4 softtabstop=4 shiftwidth=4 expandtab
#########################################################################
# Copyright 2012 KNX-User-Forum e.V.            http://knx-user-forum.de/
#########################################################################
#  This file is part of SmartHome.py.   http://smarthome.sourceforge.net/
#
#  SmartHome.py is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SmartHome.py is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with SmartHome.py. If not, see <http://www.gnu.org/licenses/>.
##########################################################################

import os
import sys
from collections import namedtuple
import xml.etree.ElementTree as ET

# erster Knoten ist meist das Projekt, damit das nicht mit abgebildet wird
# folgende Option auf 1 setzen
IGNORE_TOP_LEVEL = 1

NS_URL = '{http://knx.org/xml/project/10}'
FIND_BUILDINGS = NS_URL + 'Buildings'
FIND_BUILDINGPART = NS_URL + 'BuildingPart'
FIND_DEVICEREF = NS_URL + 'DeviceInstanceRef'
FIND_DEVICE = NS_URL + 'DeviceInstance'
FIND_COMREF = NS_URL + 'ComObjectInstanceRef'
FIND_CONNECTOR = NS_URL + 'Connectors'
FIND_SEND = NS_URL + 'Send'
FIND_RECEIVE = NS_URL + 'Receive'
FIND_GA = NS_URL + 'GroupAddress'
FIND_DPT = NS_URL + 'DatapointType'
FIND_DPST = NS_URL + 'DatapointSubtype'

def processBuildingPart(root, part, depth, f, dpts):
	print "processing " + part.tag + " " + part.attrib['Name'] + " (" + part.attrib['Type'] + ")"

	if part.attrib['Type'] != "DistributionBoard":
		write_item(part.attrib['Name'], depth, f)
		
		for devref in part.findall(FIND_DEVICEREF):
			processDevice(root, devref.attrib['RefId'], depth + 1, f, dpts)

	for subpart in part.findall(FIND_BUILDINGPART):
		processBuildingPart(root, subpart, depth + 1, f, dpts)
		
	f.write('\n')

def processDevice(root, ref, depth, f, dpts):
	print "process device " + ref
	device = root.findall('.//' + FIND_DEVICE + "[@Id='" + ref + "']")[0]
	if 'Description' in device.attrib.keys():
		print device.attrib['Description']

	for comobj in device.findall('.//' + FIND_COMREF):
		if 'DatapointType' not in comobj.attrib.keys():
			continue

		if comobj.attrib['DatapointType'] not in dpts:
			dpt = 1
		else:
			dpt = dpts[comobj.attrib['DatapointType']].dpt.number

		for connector in comobj.findall('.//' + FIND_CONNECTOR):

			for send in connector.findall('.//' + FIND_SEND):
				if 'GroupAddressRefId' in send.keys():
					ga_ref = send.attrib['GroupAddressRefId']
					print "process ga " + ga_ref
					ga = root.findall('.//' + FIND_GA + "[@Id='" + ga_ref + "']")[0]
					ga_str = ga2str(int(ga.attrib['Address']))
					print "Send GA: " + ga_str + " (" + ga.attrib['Name'] + ")"

					if len(ga_str) > 0:
						write_item(ga.attrib['Name'], depth, f)
						write_dpt(dpt, depth + 1, f)
						write_param("knx_send=" + ga_str, depth + 1, f)
						write_param("knx_listen=" + ga_str, depth + 1, f)

			for receive in connector.findall('.//' + FIND_RECEIVE):
				if 'GroupAddressRefId' in receive.keys():
					ga_ref = receive.attrib['GroupAddressRefId']
					print "process ga " + ga_ref
					ga = root.findall('.//' + FIND_GA + "[@Id='" + ga_ref + "']")[0]
					ga_str = ga2str(int(ga.attrib['Address']))
					print "Receive GA: " + ga_str + " (" + ga.attrib['Name'] + ")"

					if len(ga_str) > 0:
						write_item(ga.attrib['Name'], depth, f)
						write_dpt(dpt, depth + 1, f)
						write_param("knx_read=" + ga_str, depth + 1, f)
						write_param("knx_listen=" + ga_str, depth + 1, f)

def write_dpt(dpt, depth, f):
	if dpt == 1:
		write_param("type=bool", depth, f)
		write_param("visu=toggle", depth, f)
	elif dpt == 2 or dpt == 3 or dpt == 10 or dpt == 11:
		write_param("type=foo", depth, f)
	elif dpt == 4 or dpt == 24:
		write_param("type=str", depth, f)
		write_param("visu=div", depth, f)
	else:
		write_param("type=num", depth, f)
		write_param("visu=slider", depth, f)
        write_param("knx_dpt=5001", depth, f)
        return

	write_param("knx_dpt=" + str(dpt), depth, f)

def write_param(string, depth, f):
	for i in range(depth):
		f.write('    ')

	f.write(string + '\n')

def write_item(string, depth, f):
	for i in range(depth):
		f.write('    ')

	for i in range(depth + 1):
		f.write('[')

	f.write("'" + string.encode('UTF-8').lower() + "'")

	for i in range(depth + 1):
		f.write(']')

	f.write('\n')

def ga2str(ga):
	return "%d/%d/%d" % ((ga >> 11) & 0xf, (ga >> 8) & 0x7, ga & 0xff)

def pa2str(pa):
	return "%d.%d.%d" % (pa >> 12, (pa >> 8) & 0xf, pa & 0xff)

##############################################################
#		Main
##############################################################

KNXMASTERFILE = sys.argv[1]
PROJECTFILE = sys.argv[2]
OUTFILE = sys.argv[3]

print "Master: " + KNXMASTERFILE
print "Project: " + PROJECTFILE
print "Outfile: " + OUTFILE

if (os.path.exists(OUTFILE)):
	os.remove(OUTFILE)

master = ET.parse(KNXMASTERFILE)
root = master.getroot()
dpts = {}

DPT = namedtuple('DPT', ['id', 'number', 'name', 'text', 'size', 'dpsts'])
DPST = namedtuple('DPST', ['id', 'number', 'name', 'text', 'dpt'])

for dpt in root.findall('.//' + FIND_DPT):
	item = DPT(id = dpt.attrib['Id'], number = int(dpt.attrib['Number']), name = dpt.attrib['Name'], text = dpt.attrib['Text'], size = int(dpt.attrib['SizeInBit']), dpsts = {})

	for dpst in dpt.findall('.//' + FIND_DPST):
		sub = DPST(id = dpst.attrib['Id'], number = int(dpst.attrib['Number']), name = dpst.attrib['Name'], text = dpst.attrib['Text'], dpt = item)
		item.dpsts[sub.id] = sub
		dpts[sub.id] = sub

	dpts[item.id] = item


project = ET.parse(PROJECTFILE)
root = project.getroot()
buildings = root.find('.//' + FIND_BUILDINGS)

if buildings is None:
	print "Buildings not found"
else:
	with open(OUTFILE, 'w') as f:
		if IGNORE_TOP_LEVEL:
			for part in buildings[0]:
				processBuildingPart(root, part, 0, f, dpts)
		else:
			for part in buildings:
				processBuildingPart(root, part, 0, f, dpts)


