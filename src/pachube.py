#!/usr/bin/env python

from __future__ import print_function
import eeml
import pg
import pgdb
import sys

db = pgdb.connect()
c = db.cursor()
c.execute("SET TIME ZONE 0")
c.execute("SELECT key FROM pachube WHERE id=%(feed)s", { "feed": sys.argv[1] })
key = c.fetchone()[0]
c.execute("SELECT COUNT(*) FROM dingdong")
count = c.fetchone()[0]
c.close()
del c
db.commit()

feed = eeml.Pachube("/api/feeds/{0}.xml".format(sys.argv[1]), key)
feed.update([eeml.Data("doorbell", count)])
feed.put()
