# $Id$

'''
Created on Jun 14, 2013

@author: martino
'''

import ConfigParser

from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

from poller import createStatistics,Device,PerfDataBlockDev,initTable,now,Host

version = 'ESOS perf-agent 0.0.1'
configfile = '/etc/perf-agent.conf'

Base = declarative_base()

engine = None
smaker = None

# reference instance used to shutdown
p = None

def connectDB(cfg):
    global engine 
    global smaker
    engine = create_engine(cfg.get('Database','DBURI'),echo=True)
    smaker = sessionmaker(bind=engine)

def getSession():
    global smaker
    return smaker()

def initconfig(configfile):
    cfg = ConfigParser.ConfigParser()
    try:
        cfg.read(configfile)
    except:
        print "Error unable to read a valid configuration file"
        exit(1)
    return cfg

class Poller():
    
    def __init__(self,cfg,smaker,engine):
        self.cfg = cfg
        self.smaker = smaker
        self.engine = engine
        self.stop = False
        
        self.blockdevices = cfg.get('Poller', 'BlockDevices').split()
        self.interval = int(cfg.get('Poller', 'PollerInterval'))

        self.host = cfg.get('Database','System')
        self.getHostID()

        self.createBlockDevices()
        
        
    def getHostID(self):
        # Creates a host if non existent, used to reference metrics in multi host scenario
        # and gets the ID of exising host
        session = self.smaker()
        h = session.query(Host).filter(Host.host == self.host).first()
        
        if h == None:
            h = Host(self.host)
            session.add(h)
            session.flush()
            session.refresh(h)
            self.hostid = h.id
            session.commit()
            session.close()
        else:
            self.hostid = h.id
            session.close()
            
    def createBlockDevices(self):
        # This creates the block devices needed to be referenced in any perf data entry for the current host
        # It creates a dict used for caching (avoiding querying the device.id every poll)
        # The dict is {device: id}
        session = self.smaker()
        self.devs = {}
        for device in self.blockdevices:
            d = session.query(Device).filter(Device.host == self.hostid,
                                            Device.device == device).first()
            if d == None:
                d = Device(device, 'Block Device',self.hostid)
                d.host = self.hostid
                session.add(d)
                session.flush()
                session.refresh(d)
                self.devs[device] = d.id
                session.commit()
                session.close()
            else:
                self.devs[device] = d.id
        print self.devs

    def stop(self):
        self.stop = True
    
    def start(self):
        print self.hostid
        while self.stop == False:
            session = self.smaker()
            
            stats = createStatistics(self.interval,self.devs,self.hostid)
            for device in stats:
                s = PerfDataBlockDev(**stats[device])
                s.timestamp = now()
                try:
                    session.add(s)
                    session.commit()
                except:
                    # Todo: replace with logging or silence the messages entirely
                    print 'Error communicating with database while adding %s stats' % device
                del s
            session.close()
            del session
            
def printstatus(signum,frame):
    print 'test'
    
def start():
    cfg = initconfig(configfile)
    connectDB(cfg)
    initTable(engine)
    p = Poller(cfg, smaker,engine)
    p.start()
    
if __name__ == '__main__':
    start()
    
