#!/usr/bin/python
import re

fileName= "tt_tghr.dat"
f = open(fileName, "r");
packet_rates = []
throughputs = []
while (True):
	line = f.readline()
	if not line:
		break
	regx = re.compile('--rate[ ]*[=]?[ ]*[0-9]+')
	regx2 = re.compile('--rate[ ]*[=]?[ ]*')
	packet_rate = regx2.sub('',''.join(regx.findall(line)))
	packet_rates.append(packet_rate)

	line = f.readline()
	regx = re.compile('throughput:[ ]+[0-9]+ Bps')
	regx2 = re.compile('[^0-9]+')
	throughput = 0
	throughput += int(regx2.sub('',regx.findall(line)[0]))

	line = f.readline()
	regx = re.compile('throughput:[ ]+[0-9]+ Bps')
	regx2 = re.compile('[^0-9]+')
	throughput += int(regx2.sub('',regx.findall(line)[0]))

	line = f.readline()
	regx = re.compile('throughput:[ ]+[0-9]+ Bps')
	regx2 = re.compile('[^0-9]+')
	throughput += int(regx2.sub('',regx.findall(line)[0]))

	line = f.readline()
	regx = re.compile('throughput:[ ]+[0-9]+ Bps')
	regx2 = re.compile('[^0-9]+')
	throughput += int(regx2.sub('',regx.findall(line)[0]))
	throughputs.append(throughput)
			
f.close()
print packet_rates	
print throughputs
