#! /usr/bin/python

# $Id$

'''
Created on Jul 27, 2013

@author: Marcin Czupryniak
@license: GPLv3
@copyright: ESOS Project - Marcin Czupryniak

This module is used for compacting the statistics in the database, computing averages and deleting the 5 seconds samples.
It uses native database drivers and not SQLAlchemy for optimal speed and memory utilization

Dictionaries are returned from queries

cpass - NULL = standard sample
cpass - 1 = 15 min avg for samples older than 24 hours
cpass - 2 = hourly avg for samples older than 7 days
cpass - 3 = daily avg for samples older than 1 month

Keep hourly records for 31 days
Keep daily records for 1 year

Minimal version of Postgresql due to database uri is 9.2
'''

import ConfigParser
import datetime

version = 'ESOS perf-agent 0.0.1'
configfile = '/etc/perf-agent.conf'

def initconfig(configfile):
    cfg = ConfigParser.ConfigParser()
    try:
        cfg.read(configfile)
    except:
        print "Error unable to read a valid configuration file"
        exit(1)
    return cfg



###############################
#
# module globals

class mvars: pass

mvars.dbconn = None
mvars.cfg = initconfig(configfile)
dbtype = mvars.cfg.get('Database','DBURI').split(':')[0]
###############################

# Generic insert query
iquery = '''
INSERT INTO perfdatablock (host, timestamp, device, readscompleted, readsmerged, sectorsread, writescompleted,
sectorswritten, kbwritten, kbread, averagereadtime, averagewritetime, iotime, interval, writespeed, readspeed, devicerate, cpass) 

VALUES (%(host)s, %(timestamp)s, %(device)s, %(readscompleted)s, %(readsmerged)s,
%(sectorsread)s, %(writescompleted)s, %(sectorswritten)s, %(kbwritten)s, %(kbread)s, %(averagereadtime)s,
%(averagewritetime)s, %(iotime)s, %(interval)s, %(writespeed)s, %(readspeed)s, %(devicerate)s, %(cpass)s) 
'''


# Universal cursor function
def getcur(**kwargs):
    
    # no args needed for mySQL
    if dbtype == 'postgres': return mvars.dbconn.cursor(**kwargs)
    return mvars.dbconn.cursor()

def connectDB():
    if dbtype == 'postgres':
        global psycopg2
        global DictCursor # Make them a global import in case have to be used in other functions

        import psycopg2
        from psycopg2.extras import DictCursor
        
        try:
            mvars.dbconn = psycopg2.connect(mvars.cfg.get('Database','DBURI'))
        except Exception as err:
            print 'Unable to connect to DB'
            print err
            exit(1)

def getmyhostid():
    myhost = mvars.cfg.get('Database','System')
    query = "select id from hosts where host = %s"
    
    cur = getcur()
    
    cur.execute(query,(myhost,))
    res = cur.fetchone()
    cur.close()
    
    # If my system is not in the DB exit with error status
    if res == None: exit(1)
        
    return res[0]

def getmydevices(myhostid):
    idlist = []
    query = 'select id from devices where host = %s'

    cur = getcur()
    
    cur.execute(query,(myhostid,))
    
    res = cur.fetchall()
    
    for rec in res:
        idlist.append(rec[0])
        
    cur.close()
    
    return idlist
    
    
def interavg(host,device):
    # comuptes the 15 minute avg of the last day and store into DB
    # compresses records of the previous day (run it before midnight!)
    
    # Delete query
    dquery = '''
    delete from perfdatablock where 
    perfdatablock.timestamp BETWEEN %s and %s and device = %s and cpass IS NULL;
    '''
    
    # Select query
    squery = '''
    select SUM(readscompleted) as readscompleted,SUM(readsmerged) as readsmerged,SUM(sectorsread) as sectorsread,
    SUM(sectorswritten) as sectorswritten,SUM(kbwritten) as kbwritten,SUM(kbread) as kbread,AVG(averagereadtime) as averagereadtime,
    AVG(averagewritetime) as averagewritetime,AVG(iotime) as iotime,AVG(writespeed) as writespeed,AVG(readspeed) as readspeed,
    AVG(devicerate) as devicerate, SUM(writescompleted) as writescompleted, COUNT(*) as nrecords
    
    from perfdatablock where 
    
    perfdatablock.timestamp BETWEEN %s and %s and device = %s and cpass IS NULL;
    '''
    
    startdate = (datetime.datetime.now()-datetime.timedelta(days=1)).replace(hour=0, minute=0, second=0,microsecond=0)
    dates = []
    dates.append(startdate)
    for i in xrange(1,96):
        d = startdate + datetime.timedelta(minutes=(i*15))
        dates.append(d)
    enddate = startdate.replace(hour=23, minute=59, second=59,microsecond=999999)
    dates.append(enddate)
        
    for i in xrange(0,96):
        # Select Query Block
        cur = getcur(cursor_factory=DictCursor)
        cur.execute(squery,(dates[i],dates[i+1],device))
        rec = cur.fetchone()
        
        if rec['nrecords'] == 0: continue
        
        # Transform into a normal dict
        nrec = rec.copy()
        
        cur.close()
        
        nrec['host'] = host
        nrec['device'] = device
        
        # shifting the timestamp by 7 min and 30 seconds since first sample (middle of the 15 min window)
        nrec['timestamp'] = dates[i] + datetime.timedelta(minutes = 7, seconds = 30)
        nrec['cpass'] = 1
        nrec['interval'] = 300
        
        cur = getcur(cursor_factory=DictCursor)
        
        # rollback any transaction pending and start a new one
        mvars.dbconn.rollback()
        
        try:
            # Insert the record
            cur.execute(iquery,nrec)
            
            # Delete all computed records
            cur.execute(dquery,(dates[i],dates[i+1],device))
            
            mvars.dbconn.commit()
        
        except Exception as err:
            mvars.dbconn.rollback()
            print err    

def hourlyavg(host,device):
    # This will be executed 24 times in a row until the whole day gets processed
    # Processes samples older than 7 days
    
    # Delete query
    dquery = '''
    delete from perfdatablock where 
    perfdatablock.timestamp BETWEEN %s and %s and device = %s and cpass = 1;
    '''
    
    # Select query
    squery = '''
    select SUM(readscompleted) as readscompleted,SUM(readsmerged) as readsmerged,SUM(sectorsread) as sectorsread,
    SUM(sectorswritten) as sectorswritten,SUM(kbwritten) as kbwritten,SUM(kbread) as kbread,AVG(averagereadtime) as averagereadtime,
    AVG(averagewritetime) as averagewritetime,AVG(iotime) as iotime,AVG(writespeed) as writespeed,AVG(readspeed) as readspeed,
    AVG(devicerate) as devicerate, SUM(writescompleted) as writescompleted, COUNT(*) as nrecords
    
    from perfdatablock where 
    
    perfdatablock.timestamp BETWEEN %s and %s and device = %s and cpass = 1;
    '''
    
    # starts at 00:00:00 of the previous week
    startdate = (datetime.datetime.now()-datetime.timedelta(days=7)).replace(hour=0, minute=0, second=0,microsecond=0)
    
    # build the dates list - hours within a day
    dates = []
    dates.append(startdate)
    for i in xrange(1,24):
        d = startdate + datetime.timedelta(hours=i)
        dates.append(d)
    enddate = startdate.replace(hour=23, minute=59, second=59,microsecond=999999)
    dates.append(enddate)
    
    for i in xrange(0,24):
        # Select Query Block
        cur = getcur(cursor_factory=DictCursor)
        cur.execute(squery,(dates[i],dates[i+1],device))
        rec = cur.fetchone()
        if rec['nrecords'] == 0: continue
        # cast into a normal dict
        nrec = rec.copy()
        cur.close()
        nrec['host'] = host
        nrec['device'] = device
        # shifting the timestamp by 30 min since first sample
        nrec['timestamp'] = dates[i] + datetime.timedelta(minutes = 30)
        nrec['cpass'] = 2
        nrec['interval'] = 3600
        # Insert Query Block
        cur = getcur(cursor_factory=DictCursor)
        
        # rollback any transaction pending and start a new one
        mvars.dbconn.rollback()
        
        try:
            # Insert the record
            cur.execute(iquery,nrec)
            
            # Delete all computed records
            cur.execute(dquery,(dates[i],dates[i+1],device))
            
            mvars.dbconn.commit()
        
        except Exception as err:
            mvars.dbconn.rollback()
            print err
        
    cur.close()

def dailyavg(host,device):
    # This should be executed daily by the cron script, otherwise it will skip 
    
    # 31 days
    startdate = (datetime.datetime.now()-datetime.timedelta(days=31)).replace(hour=0, minute=0, second=0,microsecond=0)    
    enddate = startdate.replace(hour=23, minute=59, second=59,microsecond=999999)
    
    # Select query
    squery = '''
    select SUM(readscompleted) as readscompleted,SUM(readsmerged) as readsmerged,SUM(sectorsread) as sectorsread,
    SUM(sectorswritten) as sectorswritten,SUM(kbwritten) as kbwritten,SUM(kbread) as kbread,AVG(averagereadtime) as averagereadtime,
    AVG(averagewritetime) as averagewritetime,AVG(iotime) as iotime,AVG(writespeed) as writespeed,AVG(readspeed) as readspeed,
    AVG(devicerate) as devicerate, COUNT(*) as nrecords
    
    from perfdatablock where 
    
    perfdatablock.timestamp >= %s and perfdatablock.timestamp <= %s and device = %s and cpass = 2
    '''
    # Delete query
    dquery = '''
    delete from perfdatablock where 
    perfdatablock.timestamp BETWEEN %s and %s and device = %s and cpass = 2;
    '''
    cur = getcur(cursor_factory=DictCursor)
    cur.execute(squery,(startdate,enddate,0))
    rec = cur.fetchone()
    
    # make sure that we got some records from the query otherwise
    # don't store anything in the database and exit this function
    if rec['nrecords'] == 0: return
    
    # cast into a normal dict
    nrec = rec.copy()
    cur.close()
    nrec['host'] = host
    nrec['device'] = device
    # shifting the timestamp by 12 hours since first sample
    nrec['timestamp'] = startdate + datetime.timedelta(hours=12)
    nrec['cpass'] = 3 
    nrec['interval'] = 86400 # 24 hours
    # Insert Query Block
    cur = getcur(cursor_factory=DictCursor)
    
    # rollback any transaction pending and start a new one
    mvars.dbconn.rollback()
    
    try:
        # Insert the record
        cur.execute(iquery,nrec)
        
        # Delete all computed records
        cur.execute(dquery,(startdate,enddate,device))
        
        mvars.dbconn.commit()
    
    except Exception as err:
        mvars.dbconn.rollback()
        print err

def start():
    connectDB()
    myhostid = getmyhostid()
    devices = getmydevices(myhostid)

    for device in devices:
        # 15 min for last day samples
        interavg(myhostid, device)
        # 1 hour for records > 7 days
        hourlyavg(myhostid,device)
        # 1 day for records older than a month
        dailyavg(myhostid, device)
        
    # close the dbconnection
    mvars.dbconn.close()
    
    
    
if __name__ == '__main__':
    start()
