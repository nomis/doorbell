#!/usr/bin/env python2
# coding=utf8

from __future__ import division
from __future__ import print_function
import datetime
import pg
import pgdb
import select
import sys
import syslog
import time

class Rings:
	class NoSuchDoorbell(Exception):
		pass

	def __init__(self, db, doorbell):
		data = db.select1("SELECT id FROM doorbells WHERE id = %(id)s", { "id": doorbell })
		if data is None:
			raise self.NoSuchDoorbell(doorbell)

		self.doorbell = doorbell
		self.db = db

	def get_next(self):
		print("Getting ring...")
		data = self.db.select1("SELECT start FROM dingdong WHERE doorbell = %(id)s AND notify = 'f' ORDER BY start LIMIT 1", { "id": self.doorbell })

		if data is not None and data[0] is not None:
			print("Ring: {0}".format(data[0]))
			return data[0]
		return None

	def mark_notified(self, ring):
		return self.db.update("UPDATE dingdong SET notify = 't' WHERE doorbell = %(id)s AND start = %(ring)s", { "id": self.doorbell, "ring": ring })

class DB:
	class Reconnect(Exception):
		pass

	def __init__(self):
		self.db = None

	def abort(self, e=None):
		if e is not None:
			print(e, file=sys.stderr)
		if self.db is not None:
			try:
				self.db.close()
			except pg.DatabaseError:
				pass
		self.db = None

	def connect(self):
		try:
			if self.db is None:
				print("Connecting to DB...")
				self.db = pgdb.connect()
				self.listen()
				self.commit()
		except pg.DatabaseError, e:
			self.abort(e)
			return False
		except self.Reconnect:
			return False
		else:
			return True

	def listen(self):
		try:
			c = self.db.cursor()
			c.execute("LISTEN changed")
			c.close()
		except pg.DatabaseError, e:
			self.abort(e)
			raise self.Reconnect

	def select(self, query, data):
		if not self.connect():
			raise self.Reconnect
		try:
			c = self.db.cursor()
			c.execute(query, data)
			data = c.fetchall()
			c.close()
		except pg.DatabaseError, e:
			self.abort(e)
			raise self.Reconnect
		else:
			return data

	def select1(self, query, data):
		data = self.select(query, data)
		return data[0] if len(data) > 0 else None

	def update(self, query, data):
		if not self.connect():
			raise self.Reconnect
		try:
			c = self.db.cursor()
			c.execute(query, data)
			rows = c.rowcount
			c.close()
		except pg.DatabaseError, e:
			self.abort(e)
			raise self.Reconnect
		else:
			return rows

	def commit(self):
		if self.db is None:
			raise self.Reconnect
		try:
			self.db.commit()
		except pg.DatabaseError, e:
			self.abort(e)
			raise self.Reconnect

	def rollback(self):
		if self.db is None:
			raise self.Reconnect
		try:
			self.db.rollback()
		except pg.DatabaseError, e:
			self.abort(e)
			raise self.Reconnect

	def wait(self):
		if not self.connect():
			raise self.Reconnect
		try:
			notify = self.db._cnx.getnotify()
			if notify is None:
				print("Listening...")

			while notify is None:
				if self.db._cnx.fileno() < 0:
					raise self.Reconnect
				select.select([self.db._cnx], [], [self.db._cnx])
				notify = self.db._cnx.getnotify()
			print("Notified")
		except self.Reconnect:
			self.abort()
			raise self.Reconnect
		except pg.DatabaseError, e:
			self.abort(e)
			raise self.Reconnect

class Log:
	def __init__(self, name):
		syslog.openlog(name)

	def __call__(self, message):
		syslog.syslog(message)

class Handler:
	def __init__(self, db, doorbell):
		self.db = db
		self.rings = Rings(db, doorbell)

	def handle_ring(self, ts):
		pass

	def process_ring(self):
		ring = self.rings.get_next()
		if ring is None:
			self.db.commit()
			return None
		else:
			self.rings.mark_notified(ring)

			ok = self.handle_ring(ring)
			try:
				if ok:
					self.db.commit()
				else:
					self.db.rollback()
			except DB.Reconnect:
				ok = False

			return ok

	def wait_for_ring(self):
		self.db.wait()

	def main_loop(self):
		while True:
			try:
				ok = self.process_ring()
				if ok != False:
					self.wait_for_ring()
				# allow some time for invalid readings to be reverted
				time.sleep(0.5)
			except DB.Reconnect:
				time.sleep(5)
				while not self.db.connect():
					time.sleep(5)
