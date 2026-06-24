#!/usr/bin/env python3

import paho.mqtt.client as mqtt
import posix_ipc
import struct
import subprocess
import sys
import syslog
import time
import yaml

syslog.openlog("doorbell-mqtt")

with open(sys.argv[1], "r") as f:
	config = yaml.safe_load(f)

queue = posix_ipc.MessageQueue("/doorbell", flags=posix_ipc.O_CREAT, max_messages=4096, max_message_size=17)
last = None

def on_connect(client, userdata, flags, reason_code, properties=None):
	syslog.syslog("connected")
	client.subscribe("door/main/buzzer")

def on_connect_fail(client, userdata):
	syslog.syslog("connection failed")

def on_disconnect(client, userdata, flags, reason_code, properties=None):
	syslog.syslog("disconnected")

def on_message(client, userdata, message):
	global last

	if message.topic == "door/main/buzzer":
		now = time.time()
		now_s = int(now)
		now_us = int((now - now_s) * 1000000)
		buzzer = message.payload == b"1"

		if buzzer != last:
			syslog.syslog(f"{now} {message.topic} {repr(message.payload)}")
			queue.send(struct.pack("=QQ?", now_s, now_us, buzzer))
		last = buzzer

mqttc = mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_connect_fail = on_connect_fail
mqttc.on_disconnect = on_disconnect
mqttc.on_message = on_message
mqttc.username_pw_set(config["username"], config["password"])
mqttc.connect(config["hostname"], keepalive=10)
mqttc.loop_forever()
