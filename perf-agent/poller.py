'''
Created on Jun 14, 2013

@author: martino
'''

import time
import datetime

from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import Column,Integer,String,TIMESTAMP,BigInteger
from sqlalchemy.schema import ForeignKey

Base = declarative_base()

engine = None

def now():
    return datetime.datetime.now()


class Host(Base):
    __tablename__ = 'hosts'
    id = Column(Integer,primary_key=True)
    host = Column(String(255))
    description = Column(String(255))
    
    def __init__(self,host,description=None):
        self.host = host
        self.description = description
        
        
class Device(Base):
    __tablename__ = 'devices'
    id = Column(Integer,primary_key=True)
    host = Column(Integer,ForeignKey('hosts.id'))
    device = Column(String(255))
    description = Column(String(255))
    type = Column(String(255))
    
    def __init__(self,device,description,host):
        self.device = device
        self.description = description
        self.host = host
        
class PerfDataBlockDev(Base):
    # class containing performance data relative to a block device
    # NOTE partitions might use a separate class, as completion times
    # are not available
    
    __tablename__ = 'perfdatablock'
    
    id = Column(Integer,primary_key=True)
    host = Column(Integer,ForeignKey('hosts.id'))
    timestamp = Column(TIMESTAMP())
    device = Column(Integer,ForeignKey('devices.id'))
    readscompleted = Column(BigInteger) # n of read reqs completed
    readsmerged = Column(BigInteger) # n of reads merged by scheduler
    sectorsread = Column(BigInteger) # n of sectors read during sample period
    writescompleted = Column(BigInteger) # n of write requests completed
    sectorswritten = Column(BigInteger) # sectors written during period
    kbwritten = Column(BigInteger) # sum of Kb written during sample period
    kbread = Column(BigInteger) # sum of Kb read during sample period
    averagereadtime = Column(Integer) # avg of ms spent doing writes
    averagewritetime = Column(Integer) # avg of ms spent doing reads
    iotime = Column(Integer) # Combined I/O execution time in ms
    interval = Column(Integer) # Sample interval in s
    writespeed = Column(Integer) # W in Kb/s
    readspeed = Column(Integer) # R in Kb/s
    devicerate = Column(Integer) # Rate of combined R+W in KB/s
    cpass = Column(Integer) # The reducing pass the record had been trough
    
    
    def __init__(self,**kwargs):
        self.host = kwargs.get('host')
        self.device = kwargs.get('device')
        self.readscompleted = kwargs.get('readscompleted')
        self.readsmerged = kwargs.get('readsmerged')
        self.sectorsread = kwargs.get('sectorsread')
        self.writescompleted = kwargs.get('writescompleted')
        self.sectorswritten = kwargs.get('sectorswritten')
        self.kbwritten = kwargs.get('kbwritten')
        self.kbread = kwargs.get('kbread')
        self.averagereadtime = kwargs.get('averagereadtime')
        self.averagewritetime = kwargs.get('averagewritetime')
        self.iotime = kwargs.get('iotime')
        self.writespeed = kwargs.get('writespeed')
        self.readspeed = kwargs.get('readspeed')
        self.devicerate = kwargs.get('devicerate')
        self.interval = kwargs.get('interval')
        self.timestamp = kwargs.get('timestamp')
        self.cpass = kwargs.get('cpass')
        
#     def getDict(self):
#         d = {}
#         d['host'] = self.host
#         d['device'] = self.device
#         d['readscompleted'] = self.readscompleted
#         d['readsmerged'] = self.readsmerged
#         d['sectorsread'] = self.sectorsread
#         d['writescompleted'] = self.writescompleted
#         d['sectorswritten'] = self.sectorswritten
#         d['kbwritten'] = self.kbwritten
#         d['kbread'] = self.kbread
#         d['iotime'] = self.iotime
#         d['averagereadtime'] = self.averagereadtime
#         d['averagewritetime'] = self.averagewritetime
#         d['readspeed'] = self.readspeed
#         d['devicerate'] = self.devicerate
#         d['interval'] = self.interval
        
def getNCPU():
    # Gets the number of active CPUs in the system (to compute 
    pass

def getDeviceSectorSize(device):
    qs = '/sys/block/%s/queue/hw_sector_size' % device
    sectsizef = open(qs,'r')
    sectsize = int(sectsizef.read())
    sectsizef.close()
    return sectsize

def createStatistics(interval,devices,hostid):
    # computes differences from the disk statistics and outputs tuples which can be stored into DB
    initialdata = readDiskStats(devices)
    time.sleep(interval)
    finaldata = readDiskStats(devices)
    host = hostid
    iodata = {}
    
    
    
    for device in initialdata:
        blocksize = getDeviceSectorSize(device)
        stats = {}
        iodata[device] = stats
        iotime = 0
        
        stats['host'] = host
        
        stats['device'] = devices[device]
        
        stats['readscompleted'] = int(finaldata[device]['readscompleted']) - int(
                                    initialdata[device]['readscompleted'])
        stats['readsmerged'] = int(finaldata[device]['readsmerged']) - int(
                                    initialdata[device]['readsmerged'])
        stats['sectorsread'] = int(finaldata[device]['sectorsread']) - int(
                                    initialdata[device]['sectorsread'])
        stats['writescompleted'] = int(finaldata[device]['writescompleted']) - int(
                                    initialdata[device]['writescompleted'])
        stats['sectorswritten'] = int(finaldata[device]['sectorswritten']) - int(
                                    initialdata[device]['sectorswritten'])
        
        # This will not apply to partitions as they are not listed in /sys/block
        # you have to reference the parent block device in order to get total
        
        stats['kbwritten'] = stats['sectorswritten'] * blocksize / 1024
        stats['kbread'] = stats['sectorsread'] * blocksize / 1024
        
        stats['writespeed'] = stats['kbwritten'] / interval
        stats['readspeed'] = stats['kbread'] / interval
        
        # Average times for writing and reading in miliseconds
        # obtained from total time to complete divided by the number of writes
        # all within the specified interval
        timereading = (int(finaldata[device]['timereading']) - int(
                        initialdata[device]['timereading']))
        
        timewriting = (int(finaldata[device]['timewriting']) - int(
                        initialdata[device]['timewriting']))
        
        # if both writes and reads are equal to 0 set the iotime to 0 (cannot divide by 0)
        if stats['readscompleted'] == 0 and stats['writescompleted'] == 0:
            iotime = 0
        else:
            iotime = (int(finaldata[device]['ioprocessingtime']) - int(
                            initialdata[device]['ioprocessingtime'])) / (stats['readscompleted'] + stats['writescompleted'])
        
        if stats['readscompleted'] == 0 or timereading==0:
            stats['averagereadtime'] = 0
        else:
            stats['averagereadtime'] = timereading / stats['readscompleted']
        
        
        timewriting = int(finaldata[device]['timewriting']) - int(
                        initialdata[device]['timewriting'])
        
        if stats['writescompleted'] == 0 or timewriting==0:
            stats['averagewritetime'] = 0
        else:
            stats['averagewritetime'] = timewriting / stats['writescompleted']

        stats['interval'] = interval
        stats['iotime'] = iotime
        stats['devicerate'] = stats['readspeed'] + stats['writespeed']
        
    return iodata

def readDiskStats(devices):
    # applies only to block devices!
    # for partitions need to write another function
    # devices a list/tuple containg the devices to read
    dsf = '/proc/diskstats'
    ds = open(dsf,'r')
    stats = ds.readlines()
    ds.close()
    iodata = {}
    for line in stats:
        data = line.split() #splits into a list by whitespace bounduary
        # capture only devices we want
        if data[2] in devices:
            #store data into a dictionary
            statistics = {}
            iodata[data[2]] = statistics
            statistics['readscompleted'] = data[3]
            statistics['readsmerged'] = data[4]
            statistics['sectorsread'] = data[5]
            statistics['timereading'] = data[6]
            statistics['writescompleted'] = data[7]
            statistics['sectorswritten'] = data[9]
            statistics['timewriting'] = data[10]
            statistics['ioprocessingtime'] = data[13]
    return iodata


def initTable(engine):
    try:
        Base.metadata.create_all(engine)
    except Exception as err:
        print('Unable to connect to the database %s' % err)
        exit(1)
