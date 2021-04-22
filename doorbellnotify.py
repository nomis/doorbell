#!/usr/bin/env python2
# coding=utf8

from __future__ import print_function
import argparse
import daemon
import notifylib
import os
import subprocess
import sys

EXIT_SUCCESS, EXIT_FAILURE = range(0, 2)

class Execer:
	def __init__(self, cmd):
		self.cmd = cmd
		self.log = notifylib.Log("doorbellnotify")

	def ring(self, ts):
		try:
			process = subprocess.Popen([self.cmd, str(ts)], stdin=open(os.devnull), stdout=open(os.devnull), stderr=subprocess.PIPE)
			stdout, stderr = process.communicate()
			print("Return code: {0}".format(process.returncode))

			self.log("{0} [{1}]".format(ts, process.returncode))
			if stderr != "":
				for msg in stderr.split("\n"):
					self.log("  {0}".format(msg))
			return process.returncode == EXIT_SUCCESS
		except OSError, e:
			print(e)

			self.log("{0} {1}".format(ts, e))
			return False

class NotifyExec(notifylib.Handler):
	def __init__(self, db, doorbell, cmd):
		notifylib.Handler.__init__(self, db, doorbell)
		self.execer = Execer(cmd)

	def handle_ring(self, ts):
		return self.execer.ring(ts)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Exec notify on doorbell rings')
	parser.add_argument('-d', '--daemon', action='store_true', help='Run in the background')
	parser.add_argument('doorbell', help='Doorbell identifier')
	parser.add_argument('cmd', help='Command name')
	args = parser.parse_args()

	db = notifylib.DB()
	notify = NotifyExec(db, args.doorbell, args.cmd)

	if args.daemon:
		with daemon.DaemonContext():
			notify.main_loop()
	else:
		notify.main_loop()

	sys.exit(EXIT_FAILURE)
