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
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that represents an existing
// 'RTPSink', rather than one that creates new 'RTPSink's on demand.
// Implementation

#include "PassiveServerMediaSubsession.hh"
#include <GroupsockHelper.hh>

////////// PassiveServerMediaSubsession //////////

PassiveServerMediaSubsession*
PassiveServerMediaSubsession::createNew(RTPSink& rtpSink,
					RTCPInstance* rtcpInstance) {
  return new PassiveServerMediaSubsession(rtpSink, rtcpInstance);
}

PassiveServerMediaSubsession
::PassiveServerMediaSubsession(RTPSink& rtpSink, RTCPInstance* rtcpInstance)
  : ServerMediaSubsession(rtpSink.envir()),
    fSDPLines(NULL), fRTPSink(rtpSink), fRTCPInstance(rtcpInstance) {


           Groupsock const& gs = rtpSink.groupsockBeingUsed();

          Groupsock const& control = (rtcpInstance)->groupsockBeingUsed();
  

        fprintf(stderr, "\n     making new PassiveServerMediaSubsession with RTPSINKPORT: %hu\n", (gs.port().num()));
        fprintf(stderr, "       making new PassiveServerMediaSubsession with RTCP PORT: %hu\n", (control.port().num())); 


          Groupsock const& fgs = fRTPSink.groupsockBeingUsed();

          Groupsock const& fcontrol = (fRTCPInstance)->groupsockBeingUsed();

        fprintf(stderr, "\n     MEN MEN MEN PassiveServerMediaSubsession with RTPSINKPORT: %hu\n", (fgs.port().num()));
        fprintf(stderr, "       MEN MEN MEN PassiveServerMediaSubsession with RTCP PORT: %hu\n", (fcontrol.port().num())); 

  fClientRTCPSourceRecords = HashTable::create(ONE_WORD_HASH_KEYS);
}

class RTCPSourceRecord {
public:
  RTCPSourceRecord(netAddressBits addr, Port const& port)
    : addr(addr), port(port) {
  }

  netAddressBits addr;
  Port port;
};

PassiveServerMediaSubsession::~PassiveServerMediaSubsession() {
  delete[] fSDPLines;

  // Clean out the RTCPSourceRecord table:
  while (1) {
    RTCPSourceRecord* source = (RTCPSourceRecord*)(fClientRTCPSourceRecords->RemoveNext());
    if (source == NULL) break;
    delete source;
  }

  delete fClientRTCPSourceRecords;
}

Boolean PassiveServerMediaSubsession::rtcpIsMuxed() {
  if (fRTCPInstance == NULL) return False;

  // Check whether RTP and RTCP use the same "groupsock" object:
  return &(fRTPSink.groupsockBeingUsed()) == fRTCPInstance->RTCPgs();
}

char const*
PassiveServerMediaSubsession::sdpLines() {
  //fprintf(stderr, "       PassiveServerMediaSubsession::sdpLines() -> bgining\n");

  

  if (fSDPLines == NULL ) {
    //fprintf(stderr, "       sdpLines() -> fSDPLines == NULL\n");
    // Construct a set of SDP lines that describe this subsession:
    // Use the components from "rtpSink":
    Groupsock const& gs = fRTPSink.groupsockBeingUsed();

    Groupsock const& control = (fRTCPInstance)->groupsockBeingUsed();

    fprintf(stderr, "       sdpLines() -> RTSPSINK PORT: %hu\n", (gs.port().num()));

    fprintf(stderr, "       sdpLines() -> RTCP PORT: %hu\n", (control.port().num()));

    AddressString groupAddressStr(gs.groupAddress());
    fprintf(stderr, "       sdpLines() -> AddressString groupAddressStr: %s\n", groupAddressStr.val());
    unsigned short portNum = ntohs(gs.port().num());
    //fprintf(stderr, "       sdpLines() -> unsigned short portNum = ntohs(gs.port().num()): %d\n", gs.port().num());
    unsigned char ttl = gs.ttl();
    //fprintf(stderr, "       sdpLines() -> unsigned char ttl = gs.ttl(): %ch\n", gs.ttl());
    unsigned char rtpPayloadType = fRTPSink.rtpPayloadType();
    //fprintf(stderr, "       sdpLines() -> unsigned char rtpPayloadType = fRTPSink.rtpPayloadType()\n");
    char const* mediaType = fRTPSink.sdpMediaType();
    //fprintf(stderr, "       sdpLines() -> char const* mediaType = fRTPSink.sdpMediaType()\n");
    unsigned estBitrate
      = fRTCPInstance == NULL ? 50 : fRTCPInstance->totSessionBW();
      //fprintf(stderr, "       sdpLines() -> fRTCPInstance == NULL ? 50 : fRTCPInstance->totSessionBW(): %d\n", estBitrate);
    char* rtpmapLine = fRTPSink.rtpmapLine();
    //fprintf(stderr, "       sdpLines() -> fRTPSink.rtpmapLine(): %s\n", rtpmapLine);
    char const* rtcpmuxLine = rtcpIsMuxed() ? "a=rtcp-mux\r\n" : "";
    //fprintf(stderr, "       sdpLines() -> rtcpIsMuxed()\n");
    char const* rangeLine = rangeSDPLine();
    //fprintf(stderr, "       sdpLines() -> rangeSDPLine()\n");
    char const* auxSDPLine = fRTPSink.auxSDPLine();
    
    if (auxSDPLine == NULL){
      //fprintf(stderr, "       sdpLines() -> auxSDPLine() == NULL\n");
      auxSDPLine = "";
    } 

    char const* const sdpFmt =
      "m=%s %d RTP/AVP %d\r\n"
      "c=IN IP4 %s/%d\r\n"
      "b=AS:%u\r\n"
      "%s"
      "%s"
      "%s"
      "%s"
      "a=control:%s\r\n";
    unsigned sdpFmtSize = strlen(sdpFmt)
      + strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
      + strlen(groupAddressStr.val()) + 3 /* max char len */
      + 20 /* max int len */
      + strlen(rtpmapLine)
      + strlen(rtcpmuxLine)
      + strlen(rangeLine)
      + strlen(auxSDPLine)
      + strlen(trackId());
    char* sdpLines = new char[sdpFmtSize];
    sprintf(sdpLines, sdpFmt,
	    mediaType, // m= <media>
	    portNum, // m= <port>
	    rtpPayloadType, // m= <fmt list>
	    groupAddressStr.val(), // c= <connection address>
	    ttl, // c= TTL
	    estBitrate, // b=AS:<bandwidth>
	    rtpmapLine, // a=rtpmap:... (if present)
	    rtcpmuxLine, // a=rtcp-mux:... (if present)
	    rangeLine, // a=range:... (if present)
	    auxSDPLine, // optional extra SDP line
	    trackId()); // a=control:<track-id>
    delete[] (char*)rangeLine; delete[] rtpmapLine;

    fprintf(stderr, "       INNI sdpLines() -> \n%s\n", sdpLines);

    fSDPLines = strDup(sdpLines);
    delete[] sdpLines;
  }

  fprintf(stderr, "       sdpLines() -> fSDPLines != NULL -> returninng fSDPLines\n");

  return fSDPLines;
}

void PassiveServerMediaSubsession::getInstances(Port& serverRTPPort, Port& serverRTCPPort){

  //fprintf(stderr, "       getInstances -> before groupsockBeingUsed()\n");

 
 
  Groupsock & gs = fRTPSink.groupsockBeingUsed();


  serverRTPPort = gs.port();

  //fprintf(stderr, "       getInstances -> before RTCPgs()\n");


  //Groupsock* rtcpGS = fRTCPInstance->RTCPgs();
  Groupsock const& rtcpGS  = (fRTCPInstance)->groupsockBeingUsed();

  serverRTCPPort = rtcpGS.port();

  fprintf(stderr, "       getInstances -> RTSPSINK PORT: %hu\n", serverRTPPort.num());
  fprintf(stderr, "        getInstances -> RTCP PORT: %hu\n", serverRTCPPort.num());

  AddressString ipAddressStr(ourIPAddress(envir()));
  unsigned ipAddressStrSize = strlen(ipAddressStr.val());      

  ipAddressStrSize = ipAddressStrSize + 1;

  char* adr = new char[ipAddressStrSize + 2];
  sprintf(adr, ipAddressStr.val());
  adr[ipAddressStrSize + 1] = '\0';
        
  fprintf(stderr, "        getInstances -> RTSP ADDRESS: %s\n", adr);  



}

void PassiveServerMediaSubsession
::getStreamParameters(unsigned clientSessionId,
		      netAddressBits clientAddress,
		      Port const& /*clientRTPPort*/,
		      Port const& clientRTCPPort,
		      int /*tcpSocketNum*/,
		      unsigned char /*rtpChannelId*/,
		      unsigned char /*rtcpChannelId*/,
		      netAddressBits& destinationAddress,
		      u_int8_t& destinationTTL,
		      Boolean& isMulticast,
		      Port& serverRTPPort,
		      Port& serverRTCPPort,
		      void*& streamToken) {

  isMulticast = True;
  Groupsock& gs = fRTPSink.groupsockBeingUsed();


  serverRTPPort = gs.port();

  fprintf(stderr, "       getstreamparameters -> HAVE LOOKED UP RTP-Port: %d\n", htons(serverRTPPort.num()));
  fprintf(stderr, "       getstreamparameters -> HAVE LOOKED UP RTP-Port: %d\n", ntohs(serverRTPPort.num()));


    if (fRTCPInstance != NULL) {

    Groupsock* rtcpGS = fRTCPInstance->RTCPgs();
    serverRTCPPort = rtcpGS->port();

    fprintf(stderr, "       getstreamparameters -> HAVE LOOKED UP RTCP-Port: %d\n", htons(serverRTCPPort.num()));
    fprintf(stderr, "       getstreamparameters -> HAVE LOOKED UP RTCP-Port: %d\n", ntohs(serverRTCPPort.num()));

  }else{
    fprintf(stderr, "       getstreamparameters -> RTCP is NULL :( \n");
  }



  if (destinationTTL == 255){
    fprintf(stderr, "       getstreamparameters -> destinationTTL == 255\n");
    destinationTTL = gs.ttl();
  } 

  if (destinationAddress == 0) { // normal case

    

    destinationAddress = gs.groupAddress().s_addr;
    fprintf(stderr, "       getstreamparameters -> normal case getting groupaddress\n");
  } else { // use the client-specified destination address instead:
    fprintf(stderr, "       getstreamparameters -> use the client-specified destination address insteadn");
    struct in_addr destinationAddr; destinationAddr.s_addr = destinationAddress;
    gs.changeDestinationParameters(destinationAddr, 0, destinationTTL);
    if (fRTCPInstance != NULL) {

      fprintf(stderr, "       getstreamparameters -> fRTCPInstance != NULL - Changing paramters\n");
      
 
      Groupsock* rtcpGS = fRTCPInstance->RTCPgs();
      rtcpGS->changeDestinationParameters(destinationAddr, 0, destinationTTL);
    }
  }



  streamToken = NULL; // not used

  // Make a record of this client's source - for RTCP RR handling:
  RTCPSourceRecord* source = new RTCPSourceRecord(clientAddress, clientRTCPPort);


  fprintf(stderr, "       getstreamparameters -> normal case getting groupaddress: %d\n", clientRTCPPort.num());

  fClientRTCPSourceRecords->Add((char const *)clientSessionId, source);

  



}

void PassiveServerMediaSubsession::startStream(unsigned clientSessionId,
					       void* /*streamToken*/,
					       TaskFunc* rtcpRRHandler,
					       void* rtcpRRHandlerClientData,
					       unsigned short& rtpSeqNum,
					       unsigned& rtpTimestamp,
					       ServerRequestAlternativeByteHandler* /*serverRequestAlternativeByteHandler*/,
					       void* /*serverRequestAlternativeByteHandlerClientData*/) {
  rtpSeqNum = fRTPSink.currentSeqNo();
  rtpTimestamp = fRTPSink.presetNextTimestamp();

  // Try to use a big send buffer for RTP -  at least 0.1 second of
  // specified bandwidth and at least 50 KB
  unsigned streamBitrate = fRTCPInstance == NULL ? 50 : fRTCPInstance->totSessionBW(); // in kbps
  unsigned rtpBufSize = streamBitrate * 25 / 2; // 1 kbps * 0.1 s = 12.5 bytes
  if (rtpBufSize < 50 * 1024) rtpBufSize = 50 * 1024;
  increaseSendBufferTo(envir(), fRTPSink.groupsockBeingUsed().socketNum(), rtpBufSize);

  if (fRTCPInstance != NULL) {
    // Hack: Send a RTCP "SR" packet now, so that receivers will (likely) be able to
    // get RTCP-synchronized presentation times immediately:
    fRTCPInstance->sendReport();

    // Set up the handler for incoming RTCP "RR" packets from this client:
    RTCPSourceRecord* source = (RTCPSourceRecord*)(fClientRTCPSourceRecords->Lookup((char const*)clientSessionId));
    if (source != NULL) {
      fRTCPInstance->setSpecificRRHandler(source->addr, source->port,
					  rtcpRRHandler, rtcpRRHandlerClientData);
    }
  }
}

float PassiveServerMediaSubsession::getCurrentNPT(void* streamToken) {
  // Return the elapsed time between our "RTPSink"s creation time, and the current time:
  struct timeval const& creationTime  = fRTPSink.creationTime(); // alias

  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);

  return (float)(timeNow.tv_sec - creationTime.tv_sec + (timeNow.tv_usec - creationTime.tv_usec)/1000000.0);
}

void PassiveServerMediaSubsession
::getRTPSinkandRTCP(void* streamToken,
		    RTPSink const*& rtpSink, RTCPInstance const*& rtcp) {
  rtpSink = &fRTPSink;
  rtcp = fRTCPInstance;
}

void PassiveServerMediaSubsession::deleteStream(unsigned clientSessionId, void*& /*streamToken*/) {
  // Lookup and remove the 'RTCPSourceRecord' for this client.  Also turn off RTCP "RR" handling:
  RTCPSourceRecord* source = (RTCPSourceRecord*)(fClientRTCPSourceRecords->Lookup((char const*)clientSessionId));
  if (source != NULL) {
    if (fRTCPInstance != NULL) {
      fRTCPInstance->unsetSpecificRRHandler(source->addr, source->port);
    }

    fClientRTCPSourceRecords->Remove((char const*)clientSessionId);
    delete source;
  }
}
