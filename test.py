#!/usr/bin/python

from mininet.net import Mininet, Host
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import Intf
from mininet.log import setLogLevel, info
from mininet.link import TCLink

def myNetwork():

    net = Mininet( topo=None,
                   link=TCLink)

    info( '*** Adding controller\n' )
    net.addController(name='c0')

    info( '*** Add hosts\n')
    h1 = net.addHost('h1',ip = '10.0.1.1', prefixLen = 24)
    #Intf( 'eth2', node=h1)
    h2 = net.addHost('h2',ip = '10.0.1.2',cls=Host)
    #Intf('eth5',node = h2)
    info( '*** Add links\n')
    net.addLink(h1, h2,bw=100)
    net.addLink(h1, h2,bw=10)
    #Intf( 'eth2', node=h1)
    #host1_node = net.getNodeByName('h1')
    #host1_node.addIntf('h1-eth2')
    h1.setIP('10.0.1.1',prefixLen=24,intf='h1-eth0')
    h1.setIP('10.0.2.1',prefixLen=24,intf='h1-eth1')
    h2.setIP('10.0.2.2', prefixLen=24,intf='h2-eth1')	
    print h1
    info( '*** Starting network\n')
    net.start()
    #h1.cmdPrint('dhclient '+h1.defaultIntf().name)
    CLI(net)
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    myNetwork()
