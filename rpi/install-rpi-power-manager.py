#!/usr/bin/python

# install-rpi-power-manager.py
# Version: 1.2
#
# Copyright (c) 2014 Christian Isaksson
#
# Requires: ATtinyXX_PowerManager version 1.2

import os

rpiPowerManagerScriptFile = '/etc/init.d/rpi-power-manager.sh'
rpiPowerManagerScriptContent = [
	'#!/bin/sh',
	'',
	'# Set up GPIO 24 as output and set it low','echo "24" > /sys/class/gpio/export',
	'echo "out" > /sys/class/gpio/gpio24/direction',
	'echo "0" > /sys/class/gpio/gpio24/value',
	'',
	'# Set up GPIO 23 as input','echo "23" > /sys/class/gpio/export',
	'echo "in" > /sys/class/gpio/gpio23/direction',
	'',
	'# Loop as long as shutdown init request have not been received from ATtiny13',
	'while [ `cat /sys/class/gpio/gpio23/value` -eq 0 ]',
	'do',
	'\t# Sleep for a second',
	'\tsleep 1',
	'done',
	'',
	'# Power off Raspberry Pi',
	'poweroff',
	'',
	'exit 0'
]
shutdownScriptFile = '/sbin/shutdown'
shutdownScriptContent = [
	'#!/bin/sh',
	'',
	'# Setup GPIO 24 as output',
	'echo "out" > /sys/class/gpio/gpio24/direction',
	'',
	'# Check if poweroff has been initiated',
	'case "$*" in',
	'\t*-h*)',
	'\t\t# Notify ATtiny of shutdown',
	'\t\techo "1" > /sys/class/gpio/gpio24/value',
	'\t\t;;',
	'\t*)',
	'esac',
	'',
	'# Call the original linux shutdown command with all captured parameters',
	'`linux-shutdown $*`',
	'',
	'exit 0'
]

# Write content, line by line, to file
def createFile(file, content):
	# Open script file
	f = open(file,'w')
	
	# Print each line of the script content to the file
	for line in content:
		f.write(line)
		f.write('\n')
	
	# Close the file
	f.close()

# Main area
powerManagerAlreadyExists = os.path.exists(rpiPowerManagerScriptFile)

if powerManagerAlreadyExists:
	os.system('ps -ef | grep "rpi-power-manager" | awk \'{print $2}\' | xargs kill')

print 'Create rpi-power-manager.sh'
createFile(rpiPowerManagerScriptFile, rpiPowerManagerScriptContent)
os.system('chmod +x ' + rpiPowerManagerScriptFile)
os.system('sudo ' + rpiPowerManagerScriptFile + ' &')

if not powerManagerAlreadyExists:
	print 'Add rpi-power-manager.sh to autostart'
	os.system('echo "' + rpiPowerManagerScriptFile + ' &" >> /etc/init.d/rc.local')

if not os.path.exists('/sbin/linux-shutdown'):
	print 'Rename original shutdown command'
	os.system('mv /sbin/shutdown /sbin/linux-shutdown')

print 'Create new shutdown command'
createFile(shutdownScriptFile, shutdownScriptContent)
os.system('chmod +x ' + shutdownScriptFile)

print 'Install complete'
