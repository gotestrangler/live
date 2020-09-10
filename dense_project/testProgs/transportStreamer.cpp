/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2019, Live Networks, Inc.  All rights reserved
// A test program that reads a MPEG-2 Transport Stream file,
// and streams it using RTP
// main program

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include <../liveMedia/include/RTSPDenseServer.hh>

// To stream using "source-specific multicast" (SSM), uncomment the following:
//#define USE_SSM 1
#ifdef USE_SSM
Boolean const isSSM = True;
#else
Boolean const isSSM = False;
#endif

// To set up an internal RTSP server, uncomment the following:
#define IMPLEMENT_RTSP_SERVER 1
// (Note that this RTSP server works for multicast only)

#define TRANSPORT_PACKET_SIZE 188
#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 7
// The product of these two numbers must be enough to fit within a network packet

UsageEnvironment* env;
char const* inputFileName = "../extras/chunks/mix.ts";
FramedSource* videoSource;
RTPSink* videoSink;

void play(); // forward

int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);


/*
  // Create 'groupsocks' for RTP and RTCP:
  char const* destinationAddressStr
#ifdef USE_SSM
    = "232.255.42.42";
#else
  = "239.255.42.42";
  // Note: This is a multicast address.  If you wish to stream using
  // unicast instead, then replace this string with the unicast address
  // of the (single) destination.  (You may also need to make a similar
  // change to the receiver program.)
#endif
  const unsigned short rtpPortNum = 1234;
  const unsigned short rtcpPortNum = rtpPortNum+1;
  const unsigned char ttl = 7; // low, in case routers don't admin scope

  struct in_addr destinationAddress;
  destinationAddress.s_addr = our_inet_addr(destinationAddressStr);
  const Port rtpPort(rtpPortNum);
  const Port rtcpPort(rtcpPortNum);

  Groupsock rtpGroupsock(*env, destinationAddress, rtpPort, ttl);
  Groupsock rtcpGroupsock(*env, destinationAddress, rtcpPort, ttl);
#ifdef USE_SSM
  rtpGroupsock.multicastSendOnly();
  rtcpGroupsock.multicastSendOnly();
#endif

*/

  // Create an appropriate 'RTP sink' from the RTP 'groupsock':
  //videoSink = SimpleRTPSink::createNew(*env, &rtpGroupsock, 33, 90000, "video", "MP2T", 1, True, False /*no 'M' bit*/);

  /*

  // Create (and start) a 'RTCP instance' for this RTP sink:
  const unsigned estimatedSessionBandwidth = 5000; // in kbps; for RTCP b/w share
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen+1];
  gethostname((char*)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case

  RTCPInstance* rtcp =
    RTCPInstance::createNew(*env, &rtcpGroupsock,
			    estimatedSessionBandwidth, CNAME,
			    videoSink, NULL, isSSM);
  // Note: This starts RTCP running automatically

*/

  RTSPDenseServer* rtspServer = RTSPDenseServer::createNew(*env, 8554);
  // Note that this (attempts to) start a server on the default RTSP server
  // port: 554.  To use a different port number, add it as an extra
  // (optional) parameter to the "RTSPServer::createNew()" call above.
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }


 for(int i = 0; i < (argc - 1); i++){
   *env << "Adding filename: " << argv[i + 1] << " to the denseServer\n";
   rtspServer->filenames->Add((const char *)i, argv[i + 1]);
 }



  ServerMediaSession* sms
    = ServerMediaSession::createNew(*env, "testStream", (char const*)rtspServer->filenames->Lookup((const char *)1),
		   "Session streamed by \"testMPEG2TransportStreamer\"", isSSM);
  rtspServer->addServerMediaSession(sms);
  

  char* url = rtspServer->rtspURL(sms);
  *env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;


  // Finally, start the streaming:
  *env << "Beginning streaming...\n";
  //play();

  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}


