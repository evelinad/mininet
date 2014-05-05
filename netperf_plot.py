#!/usr/bin/python
import re

fileName= "log8.txt"
f = open(fileName, "r");
packet_rates = []
rtts = []
throughputs = []
throughput_confid = []

#line at the beginig of a test
line = f.readline()
finished = False
while (finished == False):

	#12 15
	for i in range(12):
			line = f.readline()		
			if not line:
				finished = True

	if finished == True:
		break;
	regx = re.compile('THROUGHPUT=')
	
	line = f.readline()
	line = re.sub('[^0-9_\-\.\=A-Za-z]+','',line)
	throughput = float(regx.sub('',line))
	throughputs.append(throughput)

	regx = re.compile('RT_LATENCY=')
	line = f.readline()	
	line = re.sub('[^0-9_\-\.\=A-Za-z]+','',line)
	rtt = float(regx.sub('',line))
	rtts.append(rtt)			

	regx = re.compile('THROUGHPUT_CONFID=')				
	line = f.readline()
	line = re.sub('[^0-9_\-\.\=A-Za-z]+','',line)
	confid = float(regx.sub('',line))				
	throughput_confid.append(confid)
	
	regx = re.compile('-b[ ]*[0-9]+')
	regx2 = re.compile('[^0-9]+')
	line = f.readline()
	line = re.sub('[^0-9_\-\.\=A-Za-z]+','',line)	
	rate = int(regx2.sub('',regx.findall(line)[0]))
	packet_rates.append(rate)
	
				
	for i in range(14):
			line = f.readline()
			if not line:
				finished = True
						
		
f.close()
print packet_rates
print rtts
print throughputs
print throughput_confid		
		
