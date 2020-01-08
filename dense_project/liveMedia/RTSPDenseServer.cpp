#include "include/RTSPDenseServer.hh"
#include "RTSPCommon.hh"
#include "RTSPRegisterSender.hh"
#include "ProxyServerMediaSession.hh"
#include "GroupsockHelper.hh"
#include "Base64.hh"
#include "H264VideoStreamFramer.hh"
#include "RTPSink.hh"
#include "H264VideoRTPSink.hh"
#include "PassiveServerMediaSubsession.hh"
#include "ByteStreamFileSource.hh"
#include "Groupsock.hh"


H264VideoStreamFramer* videoSource1;
RTPSink* videoSink1;




//////// DENSE SERVER ///// 
RTSPDenseServer* RTSPDenseServer
::createNew(UsageEnvironment& env, Port ourPort,
	    UserAuthenticationDatabase* authDatabase,
	    unsigned reclamationSeconds,
	    Boolean streamRTPOverTCP, int number) {


  fprintf(stderr, "\n 2 number is: %d\n", number);
  int ourSocket = setUpOurSocket(env, ourPort);
  if (ourSocket == -1) return NULL;

  RTSPDenseServer * ne = new RTSPDenseServer(env, ourSocket, ourPort,
					    authDatabase,
					    reclamationSeconds,
					    streamRTPOverTCP, number);

  ne->ref = 0; 
  
  return ne;
}

RTSPDenseServer::RTSPDenseServer(UsageEnvironment& env, int ourSocket, Port ourPort,
				 UserAuthenticationDatabase* authDatabase,
				 unsigned reclamationSeconds,
				 Boolean streamRTPOverTCP, int number)
  : RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationSeconds),
    fStreamRTPOverTCP(streamRTPOverTCP), fAllowStreamingRTPOverTCP(True), 
    fTCPStreamingDatabase(HashTable::create(ONE_WORD_HASH_KEYS)), denseTable(HashTable::create(ONE_WORD_HASH_KEYS)), filenames(HashTable::create(ONE_WORD_HASH_KEYS)), number(number) {
}

RTSPDenseServer::~RTSPDenseServer() {
    cleanup();
}



void RTSPDenseServer::DenseSession::setRTPGSock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl){

  fprintf(stderr, "setRTPGSock\n");
   Groupsock* gsock = createNewGroupSock(env, destinationAddress, rtpPort, ttl);
   rtpGroupsock = gsock; 

  AddressString groupAddressStr(rtpGroupsock->groupAddress());
  fprintf(stderr, "       setRTPGSock -> AddressString groupAddressStr: %s\n", groupAddressStr.val());
  unsigned short portNum = ntohs(rtpGroupsock->port().num());
  fprintf(stderr, "       setRTPGSock -> portnum: %hu\n", portNum);


}

void RTSPDenseServer::DenseSession::setRTCPGSock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl){
  fprintf(stderr, "setRTPCONTROL gsock\n");
  Groupsock* gsock = createNewGroupSock(env, destinationAddress, rtpPort, ttl);
  rtcpGroupsock = gsock;

  AddressString groupAddressStr(rtcpGroupsock->groupAddress());
  fprintf(stderr, "       setRTPCONTROL gsock -> AddressString groupAddressStr: %s\n", groupAddressStr.val());
  unsigned short portNum = ntohs(rtcpGroupsock->port().num());
  fprintf(stderr, "       setRTPCONTROL gsock -> portnum: %hu\n", portNum);

}



RTSPDenseServer::DenseSession*
RTSPDenseServer::createNewDenseSession(Groupsock* rtpG, Groupsock* rtcpG, RTPSink* videoSink, RTCPInstance* rtcp, 
        PassiveServerMediaSubsession* passiveSession,
        ServerMediaSession* denseSession,
        ByteStreamFileSource* fileSource,
        H264VideoStreamFramer* videoSource) {

  fprintf(stderr, "creTING NEW DENSE SESSION:\n" );
  return new DenseSession(rtpG, rtcpG, videoSink, rtcp, 
        passiveSession,
        denseSession,
        fileSource,
        videoSource);
}






//// IMPLEMENTATION OF DENSE CLIENT CONNECTION ///////
RTSPDenseServer::RTSPDenseClientConnection
::RTSPDenseClientConnection(RTSPDenseServer& ourServer, int clientSocket, struct sockaddr_in clientAddr)
  : RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr), fOurRTSPServer(ourServer), ourClientSession(NULL) {
  resetRequestBuffer();
}

RTSPDenseServer::RTSPDenseClientConnection::~RTSPDenseClientConnection() {
  closeSocketsRTSP();
}

RTSPServer::RTSPClientConnection*
RTSPDenseServer::createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr) {
  fprintf(stderr, "creTING NEW CLIENT CONNECTION: %d\nn", htons(clientAddr.sin_port));
  fprintf(stderr, "creTING NEW CLIENT CONNECTION: %d\n", ntohs(clientAddr.sin_port));
  return new RTSPDenseClientConnection(*this, clientSocket, clientAddr);

  
}




//// IMPLEMENTATION OF DENSE CLIENT SESSION ///////
RTSPDenseServer::RTSPDenseClientSession
::RTSPDenseClientSession(RTSPDenseServer& ourServer, u_int32_t sessionId)
  : RTSPServer::RTSPClientSession(ourServer, sessionId), fOurRTSPServer(ourServer), ourClientConnection(NULL), newDenseRequest(False){
}

RTSPDenseServer::RTSPDenseClientSession::~RTSPDenseClientSession() {
  reclaimStreamStates();
}

RTSPServer::RTSPClientSession*
RTSPDenseServer::createNewClientSession(u_int32_t sessionId) {

  return new RTSPDenseClientSession(*this, sessionId);
}












/*




newDenseConnection(){




}


*/



























typedef enum StreamingMode {
  RTP_UDP,
  RTP_TCP,
  RAW_UDP
} StreamingMode;


static void parseTransportHeader(char const* buf,
				 StreamingMode& streamingMode,
				 char*& streamingModeString,
				 char*& destinationAddressStr,
				 u_int8_t& destinationTTL,
				 portNumBits& clientRTPPortNum, // if UDP
				 portNumBits& clientRTCPPortNum, // if UDP
				 unsigned char& rtpChannelId, // if TCP
				 unsigned char& rtcpChannelId // if TCP
				 ) {
  // Initialize the result parameters to default values:
  streamingMode = RTP_UDP;
  streamingModeString = NULL;
  destinationAddressStr = NULL;
  destinationTTL = 255;
  clientRTPPortNum = 0;
  clientRTCPPortNum = 1;
  rtpChannelId = rtcpChannelId = 0xFF;
  
  portNumBits p1, p2;
  unsigned ttl, rtpCid, rtcpCid;
  //fprintf(stderr, "     parseTransportHeader() - extracting info on client\n");
  // First, find "Transport:"
  while (1) {
    if (*buf == '\0') return; // not found
    if (*buf == '\r' && *(buf+1) == '\n' && *(buf+2) == '\r') return; // end of the headers => not found
    if (_strncasecmp(buf, "Transport:", 10) == 0) break;
    ++buf;
  }
  
  // Then, run through each of the fields, looking for ones we handle:
  char const* fields = buf + 10;
  while (*fields == ' ') ++fields;
  char* field = strDupSize(fields);
  while (sscanf(fields, "%[^;\r\n]", field) == 1) {
    if (strcmp(field, "RTP/AVP/TCP") == 0) {
      streamingMode = RTP_TCP;
    } else if (strcmp(field, "RAW/RAW/UDP") == 0 ||
	       strcmp(field, "MP2T/H2221/UDP") == 0) {
      streamingMode = RAW_UDP;
      streamingModeString = strDup(field);
    } else if (_strncasecmp(field, "destination=", 12) == 0) {
      delete[] destinationAddressStr;
      destinationAddressStr = strDup(field+12);
    } else if (sscanf(field, "ttl%u", &ttl) == 1) {
      destinationTTL = (u_int8_t)ttl;
    } else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2) {
      clientRTPPortNum = p1;
      clientRTCPPortNum = streamingMode == RAW_UDP ? 0 : p2; // ignore the second port number if the client asked for raw UDP
    } else if (sscanf(field, "client_port=%hu", &p1) == 1) {
      clientRTPPortNum = p1;
      clientRTCPPortNum = streamingMode == RAW_UDP ? 0 : p1 + 1;
    } else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
      rtpChannelId = (unsigned char)rtpCid;
      rtcpChannelId = (unsigned char)rtcpCid;
    }
    
    fields += strlen(field);
    while (*fields == ';' || *fields == ' ' || *fields == '\t') ++fields; // skip over separating ';' chars or whitespace
    if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
  }
  delete[] field;
}

static Boolean parsePlayNowHeader(char const* buf) {
  // Find "x-playNow:" header, if present
  while (1) {
    if (*buf == '\0') return False; // not found
    if (_strncasecmp(buf, "x-playNow:", 10) == 0) break;
    ++buf;
  }
  
  return True;
}
















void RTSPDenseServer::RTSPDenseClientConnection::handleRequestBytes(int newBytesRead) {
  int numBytesRemaining = 0;
  ++fRecursionCount;
  fprintf(stderr, "//////////// handleRequestBytes//////////////\n");
    do {
    RTSPDenseServer::RTSPDenseClientSession* clientSession = NULL;

    if (newBytesRead < 0 || (unsigned)newBytesRead >= fRequestBufferBytesLeft) {
      // Either the client socket has died, or the request was too big for us.
      // Terminate this connection:
      fIsActive = False;
      break;
    }
    //fprintf(stderr, "1\n");
    Boolean endOfMsg = False;
    unsigned char* ptr = &fRequestBuffer[fRequestBytesAlreadySeen];

    
    if (fClientOutputSocket != fClientInputSocket && numBytesRemaining == 0) {
      // We're doing RTSP-over-HTTP tunneling, and input commands are assumed to have been Base64-encoded.
      // We therefore Base64-decode as much of this new data as we can (i.e., up to a multiple of 4 bytes).
      
      // But first, we remove any whitespace that may be in the input data:
      unsigned toIndex = 0;
      for (int fromIndex = 0; fromIndex < newBytesRead; ++fromIndex) {
	char c = ptr[fromIndex];
	if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) { // not 'whitespace': space,tab,CR,NL
	  ptr[toIndex++] = c;
	}
      }
      newBytesRead = toIndex;
      
      unsigned numBytesToDecode = fBase64RemainderCount + newBytesRead;
      unsigned newBase64RemainderCount = numBytesToDecode%4;
      numBytesToDecode -= newBase64RemainderCount;
      if (numBytesToDecode > 0) {
	ptr[newBytesRead] = '\0';
	unsigned decodedSize;
	unsigned char* decodedBytes = base64Decode((char const*)(ptr-fBase64RemainderCount), numBytesToDecode, decodedSize);
#ifdef DEBUG
	fprintf(stderr, "Base64-decoded %d input bytes into %d new bytes:", numBytesToDecode, decodedSize);
	for (unsigned k = 0; k < decodedSize; ++k) fprintf(stderr, "%c", decodedBytes[k]);
	fprintf(stderr, "\n");
#endif
	
	// Copy the new decoded bytes in place of the old ones (we can do this because there are fewer decoded bytes than original):
	unsigned char* to = ptr-fBase64RemainderCount;
	for (unsigned i = 0; i < decodedSize; ++i) *to++ = decodedBytes[i];
	
	// Then copy any remaining (undecoded) bytes to the end:
	for (unsigned j = 0; j < newBase64RemainderCount; ++j) *to++ = (ptr-fBase64RemainderCount+numBytesToDecode)[j];
	
	newBytesRead = decodedSize - fBase64RemainderCount + newBase64RemainderCount;
	  // adjust to allow for the size of the new decoded data (+ remainder)
	delete[] decodedBytes;
      }
      fBase64RemainderCount = newBase64RemainderCount;
    }
    
    unsigned char* tmpPtr = fLastCRLF + 2;
    if (fBase64RemainderCount == 0) { // no more Base-64 bytes remain to be read/decoded
      // Look for the end of the message: <CR><LF><CR><LF>
      if (tmpPtr < fRequestBuffer) tmpPtr = fRequestBuffer;
      while (tmpPtr < &ptr[newBytesRead-1]) {
	if (*tmpPtr == '\r' && *(tmpPtr+1) == '\n') {
	  if (tmpPtr - fLastCRLF == 2) { // This is it:
	    endOfMsg = True;
	    break;
	  }
	  fLastCRLF = tmpPtr;
	}
	++tmpPtr;
      }
    }
    //fprintf(stderr, "2\n");
    fRequestBufferBytesLeft -= newBytesRead;
    fRequestBytesAlreadySeen += newBytesRead;
    
    if (!endOfMsg) break; // subsequent reads will be needed to complete the request
    
    // Parse the request string into command name and 'CSeq', then handle the command:
    fRequestBuffer[fRequestBytesAlreadySeen] = '\0';
    char cmdName[RTSP_PARAM_STRING_MAX];
    char urlPreSuffix[RTSP_PARAM_STRING_MAX];
    char urlSuffix[RTSP_PARAM_STRING_MAX];
    char cseq[RTSP_PARAM_STRING_MAX];
    char sessionIdStr[RTSP_PARAM_STRING_MAX];
    unsigned contentLength = 0;
    Boolean playAfterSetup = False;
    fLastCRLF[2] = '\0'; // temporarily, for parsing

    //fprintf(stderr, "handleRequestBytes -> parseRTSPRequestString FØR\n");

    Boolean parseSucceeded = parseRTSPRequestString((char*)fRequestBuffer, fLastCRLF+2 - fRequestBuffer,
						    cmdName, sizeof cmdName,
						    urlPreSuffix, sizeof urlPreSuffix,
						    urlSuffix, sizeof urlSuffix,
						    cseq, sizeof cseq,
						    sessionIdStr, sizeof sessionIdStr,
						    contentLength);

    //fprintf(stderr, "handleRequestBytes -> parseRTSPRequestString ETTER -> urlPreSuffix: %s, urlSuffix: %s\n", urlPreSuffix, urlSuffix);

    fLastCRLF[2] = '\r'; // restore its value
    // Check first for a bogus "Content-Length" value that would cause a pointer wraparound:
    if (tmpPtr + 2 + contentLength < tmpPtr + 2) {
      //fprintf(stderr, "tmpPtr + 2 + contentLength < tmpPtr + 2\n");
#ifdef DEBUG
      fprintf(stderr, "parseRTSPRequestString() returned a bogus \"Content-Length:\" value: 0x%x (%d)\n", contentLength, (int)contentLength);
#endif
      contentLength = 0;
      parseSucceeded = False;
    }
    if (parseSucceeded) {
      //fprintf(stderr, "parsesuceeded\n");
#ifdef DEBUG
      fprintf(stderr, "parseRTSPRequestString() succeeded, returning cmdName \"%s\", urlPreSuffix \"%s\", urlSuffix \"%s\", CSeq \"%s\", Content-Length %u, with %d bytes following the message.\n", cmdName, urlPreSuffix, urlSuffix, cseq, contentLength, ptr + newBytesRead - (tmpPtr + 2));
#endif
      // If there was a "Content-Length:" header, then make sure we've received all of the data that it specified:
      if (ptr + newBytesRead < tmpPtr + 2 + contentLength){
        //fprintf(stderr, "break\n");
        break; // we still need more data; subsequent reads will give it to us 
      } 
      
      // If the request included a "Session:" id, and it refers to a client session that's
      // current ongoing, then use this command to indicate 'liveness' on that client session:
      Boolean const requestIncludedSessionId = sessionIdStr[0] != '\0';
      if (requestIncludedSessionId) {
        //fprintf(stderr, "3\n");

        //fprintf(stderr, "handleRequestBytes -> lookupClientSession\n");
	clientSession
	  = (RTSPDenseServer::RTSPDenseClientSession*)(fOurRTSPServer.lookupClientSession(sessionIdStr));
	if (clientSession != NULL){
    //fprintf(stderr, "handleRequestBytes -> noteliveness on clientsession\n");
    clientSession->noteLiveness();
  } 
      }else{
        //fprintf(stderr, "handleRequestBytes -> request did not include session ID\n");
      }

      //fprintf(stderr, "handleRequestBytes -> We now have a complete RTSP request\n");
      // We now have a complete RTSP request.
      // Handle the specified command (beginning with commands that are session-independent):
      fCurrentCSeq = cseq;
      if (strcmp(cmdName, "OPTIONS") == 0) {
        fprintf(stderr, "OPTIONS\n");
	// If the "OPTIONS" command included a "Session:" id for a session that doesn't exist,
	// then treat this as an error:
	if (requestIncludedSessionId && clientSession == NULL) {
#ifdef DEBUG
	  fprintf(stderr, "Calling handleCmd_sessionNotFound() (case 1)\n");
#endif
	  handleCmd_sessionNotFound();
	} else {
	  // Normal case:


	  handleCmd_OPTIONS(urlSuffix);
	}
      } else if (urlPreSuffix[0] == '\0' && urlSuffix[0] == '*' && urlSuffix[1] == '\0') {
	// The special "*" URL means: an operation on the entire server.  This works only for GET_PARAMETER and SET_PARAMETER:
	if (strcmp(cmdName, "GET_PARAMETER") == 0) {
    fprintf(stderr, "GET_PARAMETER\n");
	  handleCmd_GET_PARAMETER((char const*)fRequestBuffer);
	} else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
    fprintf(stderr, "SET_PARAMETER\n");
	  handleCmd_SET_PARAMETER((char const*)fRequestBuffer);
	} else {
	  handleCmd_notSupported();
	}
      } else if (strcmp(cmdName, "DESCRIBE") == 0) {
        fprintf(stderr, "DESCRIBE\n");
	handleCmd_DESCRIBE(urlPreSuffix, urlSuffix, (char const*)fRequestBuffer);
      } else if (strcmp(cmdName, "SETUP") == 0) {
	Boolean areAuthenticated = True;
      //fprintf(stderr, "cmnd name is setup\n");

	if (!requestIncludedSessionId) {
    //fprintf(stderr, "the request did not include session id\n");
    //fprintf(stderr, "request did not include session id\n");
	  // No session id was present in the request.
	  // So create a new "RTSPClientSession" object for this request.

	  // But first, make sure that we're authenticated to perform this command:
	  char urlTotalSuffix[2*RTSP_PARAM_STRING_MAX];
	      // enough space for urlPreSuffix/urlSuffix'\0'
	  urlTotalSuffix[0] = '\0';
	  if (urlPreSuffix[0] != '\0') {
	    strcat(urlTotalSuffix, urlPreSuffix);
	    strcat(urlTotalSuffix, "/");
	  }
	  strcat(urlTotalSuffix, urlSuffix);
	  if (authenticationOK("SETUP", urlTotalSuffix, (char const*)fRequestBuffer)) {
      //fprintf(stderr, "createnewClientsesion\n");
	    clientSession
	      = (RTSPDenseServer::RTSPDenseClientSession*)fOurRTSPServer.createNewClientSessionWithId();
	  } else {
	    areAuthenticated = False;
      //fprintf(stderr, "not autentichated\n");
	  }
	}
	if (clientSession != NULL) {
    //fprintf(stderr, "clientsession is NOT NULL - go to handle command setup\n");
	  clientSession->handleCmd_SETUP(this, urlPreSuffix, urlSuffix, (char const*)fRequestBuffer);
	  playAfterSetup = clientSession->fStreamAfterSETUP;
	} else if (areAuthenticated) {
    //fprintf(stderr, "going to session not found\n");

	  handleCmd_sessionNotFound();
	}
      } else if (strcmp(cmdName, "TEARDOWN") == 0
		 || strcmp(cmdName, "PLAY") == 0
		 || strcmp(cmdName, "PAUSE") == 0
		 || strcmp(cmdName, "GET_PARAMETER") == 0
		 || strcmp(cmdName, "SET_PARAMETER") == 0) {
	if (clientSession != NULL) {
    //fprintf(stderr, "handleCMD within session\n");
	  clientSession->handleCmd_withinSession(this, cmdName, urlPreSuffix, urlSuffix, (char const*)fRequestBuffer);
	} else {
#ifdef DEBUG
	  fprintf(stderr, "Calling handleCmd_sessionNotFound() (case 3)\n");
#endif
	  handleCmd_sessionNotFound();
	}
      } else if (strcmp(cmdName, "REGISTER") == 0 || strcmp(cmdName, "DEREGISTER") == 0) {
	// Because - unlike other commands - an implementation of this command needs
	// the entire URL, we re-parse the command to get it:
	char* url = strDupSize((char*)fRequestBuffer);
	if (sscanf((char*)fRequestBuffer, "%*s %s", url) == 1) {
	  // Check for special command-specific parameters in a "Transport:" header:
	  Boolean reuseConnection, deliverViaTCP;
	  char* proxyURLSuffix;
	  parseTransportHeaderForREGISTER((const char*)fRequestBuffer, reuseConnection, deliverViaTCP, proxyURLSuffix);

	  handleCmd_REGISTER(cmdName, url, urlSuffix, (char const*)fRequestBuffer, reuseConnection, deliverViaTCP, proxyURLSuffix);
	  delete[] proxyURLSuffix;
	} else {
	  handleCmd_bad();
	}
	delete[] url;
      } else {
	// The command is one that we don't handle:
	handleCmd_notSupported();
      }
    } else {
      //fprintf(stderr, "parse-failed\n");
#ifdef DEBUG
      fprintf(stderr, "parseRTSPRequestString() failed; checking now for HTTP commands (for RTSP-over-HTTP tunneling)...\n");
#endif
      // The request was not (valid) RTSP, but check for a special case: HTTP commands (for setting up RTSP-over-HTTP tunneling):
      char sessionCookie[RTSP_PARAM_STRING_MAX];
      char acceptStr[RTSP_PARAM_STRING_MAX];
      *fLastCRLF = '\0'; // temporarily, for parsing
      parseSucceeded = parseHTTPRequestString(cmdName, sizeof cmdName,
					      urlSuffix, sizeof urlPreSuffix,
					      sessionCookie, sizeof sessionCookie,
					      acceptStr, sizeof acceptStr);
      *fLastCRLF = '\r';
      if (parseSucceeded) {
#ifdef DEBUG
	fprintf(stderr, "parseHTTPRequestString() succeeded, returning cmdName \"%s\", urlSuffix \"%s\", sessionCookie \"%s\", acceptStr \"%s\"\n", cmdName, urlSuffix, sessionCookie, acceptStr);
#endif
	// Check that the HTTP command is valid for RTSP-over-HTTP tunneling: There must be a 'session cookie'.
	Boolean isValidHTTPCmd = True;
	if (strcmp(cmdName, "OPTIONS") == 0) {
	  handleHTTPCmd_OPTIONS();
	} else if (sessionCookie[0] == '\0') {
	  // There was no "x-sessioncookie:" header.  If there was an "Accept: application/x-rtsp-tunnelled" header,
	  // then this is a bad tunneling request.  Otherwise, assume that it's an attempt to access the stream via HTTP.
	  if (strcmp(acceptStr, "application/x-rtsp-tunnelled") == 0) {
	    isValidHTTPCmd = False;
	  } else {
	    handleHTTPCmd_StreamingGET(urlSuffix, (char const*)fRequestBuffer);
	  }
	} else if (strcmp(cmdName, "GET") == 0) {
	  handleHTTPCmd_TunnelingGET(sessionCookie);
	} else if (strcmp(cmdName, "POST") == 0) {
	  // We might have received additional data following the HTTP "POST" command - i.e., the first Base64-encoded RTSP command.
	  // Check for this, and handle it if it exists:
	  unsigned char const* extraData = fLastCRLF+4;
	  unsigned extraDataSize = &fRequestBuffer[fRequestBytesAlreadySeen] - extraData;
	  if (handleHTTPCmd_TunnelingPOST(sessionCookie, extraData, extraDataSize)) {
	    // We don't respond to the "POST" command, and we go away:
	    fIsActive = False;
	    break;
	  }
	} else {
	  isValidHTTPCmd = False;
	}
	if (!isValidHTTPCmd) {
	  handleHTTPCmd_notSupported();
	}
      } else {
#ifdef DEBUG
	fprintf(stderr, "parseHTTPRequestString() failed!\n");
#endif
	handleCmd_bad();
      }
    }
    
#ifdef DEBUG
    fprintf(stderr, "sending response: %s", fResponseBuffer);
#endif
    send(fClientOutputSocket, (char const*)fResponseBuffer, strlen((char*)fResponseBuffer), 0);
    
    if (playAfterSetup) {
      // The client has asked for streaming to commence now, rather than after a
      // subsequent "PLAY" command.  So, simulate the effect of a "PLAY" command:
      clientSession->handleCmd_withinSession(this, "PLAY", urlPreSuffix, urlSuffix, (char const*)fRequestBuffer);
    }
    
    // Check whether there are extra bytes remaining in the buffer, after the end of the request (a rare case).
    // If so, move them to the front of our buffer, and keep processing it, because it might be a following, pipelined request.
    unsigned requestSize = (fLastCRLF+4-fRequestBuffer) + contentLength;
    numBytesRemaining = fRequestBytesAlreadySeen - requestSize;
    resetRequestBuffer(); // to prepare for any subsequent request
    
    if (numBytesRemaining > 0) {
      memmove(fRequestBuffer, &fRequestBuffer[requestSize], numBytesRemaining);
      newBytesRead = numBytesRemaining;
    }
  } while (numBytesRemaining > 0);
  
  --fRecursionCount;
  if (!fIsActive) {
    if (fRecursionCount > 0) closeSockets(); else delete this;
    // Note: The "fRecursionCount" test is for a pathological situation where we reenter the event loop and get called recursively
    // while handling a command (e.g., while handling a "DESCRIBE", to get a SDP description).
    // In such a case we don't want to actually delete ourself until we leave the outermost call.
  }
}



void afterPlaying1(void* /*clientData*/) {
  videoSink1->stopPlaying();
  Medium::close(videoSource1);
  // Note that this also closes the input file that this source read from.

  // Start playing once again:
  //play();
  exit(1);
}







void RTSPDenseServer::RTSPDenseClientConnection::make(ServerMediaSession *session, int number){

      fprintf(stderr, "/////////////// YOUR MAKE /////////////\n");
      fprintf(stderr, "Checking input socket ID: %d\n", fClientInputSocket);
      fprintf(stderr, "Checking output socket ID: %d\n", fClientOutputSocket);
      fprintf(stderr, "ref of the densesession: %d\n", fOurRTSPServer.ref );


        UsageEnvironment& env = envir();

        DenseSession* firstsesh = fOurRTSPServer.createNewDenseSession(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        firstsesh->serversession = session; 
        // Create 'groupsocks' for RTP and RTCP:
        struct in_addr destinationAddress;
        destinationAddress.s_addr = chooseRandomIPv4SSMAddress(env);

        const unsigned short rtpPortNum = 18888 + fOurRTSPServer.ref;
        fprintf(stderr, "rtpPortNum: %hu\n", rtpPortNum);
        const unsigned short rtcpPortNum = rtpPortNum+ 1 + fOurRTSPServer.ref;
        fprintf(stderr, "rtcpPortNum: %hu\n", rtcpPortNum);
        const unsigned char ttl = 255;

        Port rtpPort(rtpPortNum);
        Port rtcpPort(rtcpPortNum);

        firstsesh->setRTPGSock(env, destinationAddress, rtpPort, ttl);

        firstsesh->setRTCPGSock(env, destinationAddress, rtcpPort, ttl);

        // Create a 'H264 Video RTP' sink from the RTP 'groupsock':
        OutPacketBuffer::maxSize = 100000;
        //RTPSink* videoSink;
        firstsesh->videoSink = H264VideoRTPSink::createNew(env, firstsesh->rtpGroupsock, 96);
        videoSink1 = firstsesh->videoSink;
        fprintf(stderr, "       Made Sink - CHECK INFO!! \n");

        
        // Create (and start) a 'RTCP instance' for this RTP sink:
        const unsigned estimatedSessionBandwidth = 500; // in kbps; for RTCP b/w share
        const unsigned maxCNAMElen = 100;
        unsigned char CNAME[maxCNAMElen+1];
        gethostname((char*)CNAME, maxCNAMElen);
        CNAME[maxCNAMElen] = '\0'; // just in case
        // *env << "CNAME: " << CNAME  << "\n";
        fprintf(stderr, "       Making RTCP with CNAME: %s\n", CNAME);
        //RTCPInstance* rtcp;
        firstsesh->rtcp = RTCPInstance::createNew(env, firstsesh->rtcpGroupsock,
                estimatedSessionBandwidth, CNAME,
                firstsesh->videoSink, NULL // we're a server 
                ,True ); // we're a SSM source
        // Note: This starts RTCP running automatically

        fprintf(stderr, "       Made RTCP\n");

        session->addSubsession(PassiveServerMediaSubsession::createNew(*firstsesh->videoSink, firstsesh->rtcp));

        fprintf(stderr, "       finsihed addservermediaession\n");

        char const* inputFileName = (char const*)fOurRTSPServer.filenames->Lookup((char const*)number);
        //char const* inputFileName2 = "output.264";

        firstsesh->fileSource = ByteStreamFileSource::createNew(envir(), inputFileName);

       
        if (firstsesh->fileSource == NULL) {
          envir() << "Unable to open file \"" << inputFileName
              << "\" as a byte-stream file source\n";
          exit(1);
        }

        FramedSource* videoES = firstsesh->fileSource;


        firstsesh->videoSource = H264VideoStreamFramer::createNew(envir(), videoES);
        videoSource1 = firstsesh->videoSource; 
 
        firstsesh->videoSink->startPlaying(*firstsesh->videoSource, afterPlaying1, firstsesh->videoSink);//AFTERPLAYING!!! INSTEAD OF NULL

        fOurRTSPServer.denseTable->Add((const char *)fClientInputSocket, firstsesh);

  
        fOurRTSPServer.ref += 1; 

        fprintf(stderr, "/////////////// FERDIG MAKE /////////////\n");

}







void RTSPDenseServer::RTSPDenseClientConnection
::handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr) {
  fprintf(stderr, "/////////////// YOUR DESCRIBE /////////////\n");
  
  ServerMediaSession* session = NULL;
  char* sdpDescription = NULL;
  char* rtspURL = NULL;
  do {
    char urlTotalSuffix[2*RTSP_PARAM_STRING_MAX];
        // enough space for urlPreSuffix/urlSuffix'\0'
    urlTotalSuffix[0] = '\0';
    if (urlPreSuffix[0] != '\0') {
      strcat(urlTotalSuffix, urlPreSuffix);
      strcat(urlTotalSuffix, "/");
    }
    strcat(urlTotalSuffix, urlSuffix);

    fprintf(stderr, "     Have assembled url-total-suffix: %s\n", urlTotalSuffix);

  
    if (!authenticationOK("DESCRIBE", urlTotalSuffix, fullRequestStr)){
      fprintf(stderr, "     describebreak\n");
      break;
    } 


    // We should really check that the request contains an "Accept:" #####
    // for "application/sdp", because that's what we're sending back #####
    
    // Begin by looking up the "ServerMediaSession" object for the specified "urlTotalSuffix":
    fprintf(stderr, "\nTryin to find it first\n");
    session = fOurServer.lookupServerMediaSession(urlTotalSuffix);
    if (session == NULL) {
      fprintf(stderr, "This is describe -> the lookupservermediasession = NULL\n");
      handleCmd_notFound();
      break;
    }
      
      int fNumStreamStates = session->numSubsessions();
      fprintf(stderr, "     Before make() the number of subsessions is: %d\n", fNumStreamStates);
      
      
     
        fprintf(stderr, "\nNumber number number: %d \n", fOurRTSPServer.number);

        int i;
        for(i = 0; i < fOurRTSPServer.number; i++){
          make(session, i);
        }

        
        /*


      DenseSession* dsession = (DenseSession*)fOurRTSPServer.denseTable->Lookup((const char *)fClientInputSocket);
      Groupsock * rt = dsession->rtpGroupsock;
      Groupsock * rc = dsession->rtcpGroupsock;

      fprintf(stderr, "\nAfter make rtp port> %d\n", rt->port().num());
      fprintf(stderr, "\nAfter make rtcp port> %d\n", rc->port().num());
      

      session = dsession->serversession;
      fprintf(stderr, "\nAfter make rtcp port> %s\n", session->streamName());
      //session = fOurServer.lookupServerMediaSession(newNameSuffix);
      
    */
    fNumStreamStates = session->numSubsessions();




    //fprintf(stderr, "This is describe -> the lookupservermediasession != NULL moving on\n");
    
    // Increment the "ServerMediaSession" object's reference count, in case someone removes it
    // while we're using it:
    session->incrementReferenceCount();


    // Then, assemble a SDP description for this session:
    sdpDescription = session->generateSDPDescription();
    if (sdpDescription == NULL) {
      // This usually means that a file name that was specified for a
      // "ServerMediaSubsession" does not exist.
      setRTSPResponse("404 File Not Found, Or In Incorrect Format");
      break;
    }
    unsigned sdpDescriptionSize = strlen(sdpDescription);

    fprintf(stderr, "   have assembled a sdpDescription: \n%s\n", sdpDescription);
    
    
    
    // Also, generate our RTSP URL, for the "Content-Base:" header
    // (which is necessary to ensure that the correct URL gets used in subsequent "SETUP" requests).
    rtspURL = fOurRTSPServer.rtspURL(session, fClientInputSocket);
    
    snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	     "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
	     "%s"
	     "Content-Base: %s/\r\n"
	     "Content-Type: application/sdp\r\n"
	     "Content-Length: %d\r\n\r\n"
	     "%s",
	     fCurrentCSeq,
	     dateHeader(),
	     rtspURL,
	     sdpDescriptionSize,
	     sdpDescription);
  } while (0);
  
  if (session != NULL) {
    // Decrement its reference count, now that we're done using it:
    session->decrementReferenceCount();
    if (session->referenceCount() == 0 && session->deleteWhenUnreferenced()) {
      fOurServer.removeServerMediaSession(session);
    }
  }

  delete[] sdpDescription;
  delete[] rtspURL;
}











void RTSPDenseServer::RTSPDenseClientConnection::handleCmd_OPTIONS(char* urlSuffix) {

  fprintf(stderr, "/////////////// YOUR OPTIONS /////////////\n");

    ServerMediaSession * session = fOurServer.lookupServerMediaSession(urlSuffix);
    /*
    if(ref < 1){
    make(session);
  }*/

  snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
	   "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n",
	   fCurrentCSeq, dateHeader(), fOurRTSPServer.allowedCommandNames());


  
  
}










void RTSPDenseServer::RTSPDenseClientSession
::handleCmd_SETUP(RTSPDenseServer::RTSPDenseClientConnection* ourClientConnection,
		  char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr) {
  // Normally, "urlPreSuffix" should be the session (stream) name, and "urlSuffix" should be the subsession (track) name.
  // However (being "liberal in what we accept"), we also handle 'aggregate' SETUP requests (i.e., without a track name),
  // in the special case where we have only a single track.  I.e., in this case, we also handle:
  //    "urlPreSuffix" is empty and "urlSuffix" is the session (stream) name, or
  //    "urlPreSuffix" concatenated with "urlSuffix" (with "/" inbetween) is the session (stream) name.
  char const* streamName = urlPreSuffix; // in the normal case
  char const* trackId = urlSuffix; // in the normal case
  char* concatenatedStreamName = NULL; // in the normal case

  fprintf(stderr, "############### DIN SETUP ###############\n");
  fprintf(stderr, "urlPreSuffix(streamname): %s        urlSuffix(subsession/track): %s \n", urlPreSuffix, urlSuffix);
  
  do {
    // First, make sure the specified stream name exists:
    ServerMediaSession* sms
      = fOurServer.lookupServerMediaSession(streamName, fOurServerMediaSession == NULL);
    if (sms == NULL) {
      //fprintf(stderr, "   handleCmd_SETUP Server Media Session = NULL first time -> %s\n", streamName);
      // Check for the special case (noted above), before we give up:
      if (urlPreSuffix[0] == '\0') {
	streamName = urlSuffix;
      } else {
	concatenatedStreamName = new char[strlen(urlPreSuffix) + strlen(urlSuffix) + 2]; // allow for the "/" and the trailing '\0'
	sprintf(concatenatedStreamName, "%s/%s", urlPreSuffix, urlSuffix);
	streamName = concatenatedStreamName;
      }
      trackId = NULL;
      
      // Check again:
      fprintf(stderr, "   handleCmd_SETUP Server Media Session = checking the concatenated stream name: %s\n", streamName);
      sms = fOurServer.lookupServerMediaSession(streamName, fOurServerMediaSession == NULL);
    }
    if (sms == NULL) {
      if (fOurServerMediaSession == NULL) {
	// The client asked for a stream that doesn't exist (and this session descriptor has not been used before):
  //fprintf(stderr, "   handleCmd_SETUP Server Media Session = fOurServerMediaSession == NULL: %s\n", streamName);
	ourClientConnection->handleCmd_notFound();
      } else {
	// The client asked for a stream that doesn't exist, but using a stream id for a stream that does exist. Bad request:
  //fprintf(stderr, "   handleCmd_SETUP Server Media Session = NULL badRequest()\n");
	ourClientConnection->handleCmd_bad();
      }
      break;
    } else {
      //fprintf(stderr, "   handleCmd_SETUP Server Media Session -> sms ER IKKE NULL!\n");
      if (fOurServerMediaSession == NULL) {
	// We're accessing the "ServerMediaSession" for the first time.
  //fprintf(stderr, "   handleCmd_SETUP Server Media Session -> fOurServerMediaSession = sms;\n");
	fOurServerMediaSession = sms;
  //fprintf(stderr, "   handleCmd_SETUP Server Media Session -> fOurServerMediaSession->incrementReferenceCount();\n");
	fOurServerMediaSession->incrementReferenceCount();
      } else if (sms != fOurServerMediaSession) {
	// The client asked for a stream that's different from the one originally requested for this stream id.  Bad request:
  //fprintf(stderr, "   handleCmd_SETUP Server Media Session = NULL badRequest()\n");
	ourClientConnection->handleCmd_bad();
	break;
      }
    }
    
  

    if (fStreamStates == NULL) {
      // This is the first "SETUP" for this session.  Set up our array of states for all of this session's subsessions (tracks):
      //fprintf(stderr, "   This is the first SETUP for this session.  Set up our array of states for all tracks\n");

      fNumStreamStates = fOurServerMediaSession->numSubsessions();
      //fprintf(stderr, "         fNumStreamStates: %d\n", fNumStreamStates);
      fStreamStates = new struct streamState[fNumStreamStates];
      
      ServerMediaSubsessionIterator iter(*fOurServerMediaSession);
      ServerMediaSubsession* subsession;
      for (unsigned i = 0; i < fNumStreamStates; ++i) {
	subsession = iter.next();
	fStreamStates[i].subsession = subsession;
	fStreamStates[i].tcpSocketNum = -1; // for now; may get set for RTP-over-TCP streaming
	fStreamStates[i].streamToken = NULL; // for now; it may be changed by the "getStreamParameters()" call that comes later
      }
    }
    
    // Look up information for the specified subsession (track):
    ServerMediaSubsession* subsession = NULL;
    unsigned trackNum;
    if (trackId != NULL && trackId[0] != '\0') { // normal case


      for (trackNum = 0; trackNum < fNumStreamStates; ++trackNum) {
	subsession = fStreamStates[trackNum].subsession;
	if (subsession != NULL && strcmp(trackId, subsession->trackId()) == 0){
    //fprintf(stderr, "         subsession != NULL && strcmp(trackId, subsession->trackId()) == 0\n");
    //fprintf(stderr, "         BREAK\n");
    break;
  }
      }
      if (trackNum >= fNumStreamStates) {
	// The specified track id doesn't exist, so this request fails:
  //fprintf(stderr, "         handlecommandnotfound: %d\n", fNumStreamStates);
	ourClientConnection->handleCmd_notFound();
	break;
      }
    } else {
      // Weird case: there was no track id in the URL.
      // This works only if we have only one subsession:
      if (fNumStreamStates != 1 || fStreamStates[0].subsession == NULL) {
        //fprintf(stderr, "         fNumStreamStates != 1 || fStreamStates[0].subsession == NULL\n");
        

        





	ourClientConnection->handleCmd_bad();
	break;
      }
      trackNum = 0;
      subsession = fStreamStates[trackNum].subsession;
    }
    // ASSERT: subsession != NULL
    
    void*& token = fStreamStates[trackNum].streamToken; // alias
    if (token != NULL) {
      // We already handled a "SETUP" for this track (to the same client),
      // so stop any existing streaming of it, before we set it up again:
      //fprintf(stderr, "   handleCmd_SETUP The setup has happened before - go again!\n");
      subsession->pauseStream(fOurSessionId, token);
      fOurRTSPServer.unnoteTCPStreamingOnSocket(fStreamStates[trackNum].tcpSocketNum, this, trackNum);
      subsession->deleteStream(fOurSessionId, token);
    }

    // Look for a "Transport:" header in the request string, to extract client parameters:
    //fprintf(stderr, "   handleCmd_SETUP Looking for client parameters\n");
    StreamingMode streamingMode;
    char* streamingModeString = NULL; // set when RAW_UDP streaming is specified
    char* clientsDestinationAddressStr;
    u_int8_t clientsDestinationTTL;
    portNumBits clientRTPPortNum, clientRTCPPortNum;
    unsigned char rtpChannelId, rtcpChannelId;
    parseTransportHeader(fullRequestStr, streamingMode, streamingModeString,
			 clientsDestinationAddressStr, clientsDestinationTTL,
			 clientRTPPortNum, clientRTCPPortNum,
			 rtpChannelId, rtcpChannelId);
    if ((streamingMode == RTP_TCP && rtpChannelId == 0xFF) ||
	(streamingMode != RTP_TCP && ourClientConnection->fClientOutputSocket != ourClientConnection->fClientInputSocket)) {
      // An anomolous situation, caused by a buggy client.  Either:
      //     1/ TCP streaming was requested, but with no "interleaving=" fields.  (QuickTime Player sometimes does this.), or
      //     2/ TCP streaming was not requested, but we're doing RTSP-over-HTTP tunneling (which implies TCP streaming).
      // In either case, we assume TCP streaming, and set the RTP and RTCP channel ids to proper values:
      streamingMode = RTP_TCP;
      rtpChannelId = fTCPStreamIdCount; rtcpChannelId = fTCPStreamIdCount+1;

      //fprintf(stderr, "   An anomolous situation, caused by a buggy client\n");

    }
    if (streamingMode == RTP_TCP){
      //fprintf(stderr, "   handleCmd_SETUP - streamingMode: TCP\n");
      fTCPStreamIdCount += 2;
    }
    
    Port clientRTPPort(clientRTPPortNum);
    Port clientRTCPPort(clientRTCPPortNum);

    //fprintf(stderr, "   handleCmd_SETUP - found client RTP-port: %d, and client RTCP-port: %d\n", clientRTPPort.num(), clientRTCPPort.num());
    
    // Next, check whether a "Range:" or "x-playNow:" header is present in the request.
    // This isn't legal, but some clients do this to combine "SETUP" and "PLAY":
    double rangeStart = 0.0, rangeEnd = 0.0;
    char* absStart = NULL; char* absEnd = NULL;
    Boolean startTimeIsNow;
    if (parseRangeHeader(fullRequestStr, rangeStart, rangeEnd, absStart, absEnd, startTimeIsNow)) {
      delete[] absStart; delete[] absEnd;
      //fprintf(stderr, "   handleCmd_SETUP - this setup is also a PLAY cmd, dumb but ok, starting after setup\n");
      fStreamAfterSETUP = True;
    } else if (parsePlayNowHeader(fullRequestStr)) {
      //fprintf(stderr, "   handleCmd_SETUP - this setup is also a PLAY cmd, dumb but ok, starting after setup\n");
      fStreamAfterSETUP = True;
    } else {
      //fprintf(stderr, "   handleCmd_SETUP - fStreamAfterSETUP = False\n");
      fStreamAfterSETUP = False;
    }
    
    // Then, get server parameters from the 'subsession':
    if (streamingMode == RTP_TCP) {
      //fprintf(stderr, "   handleCmd_SETUP - streamingMode == RTP_TCP\n");
      // Note that we'll be streaming over the RTSP TCP connection:
      fStreamStates[trackNum].tcpSocketNum = ourClientConnection->fClientOutputSocket;
      fOurRTSPServer.noteTCPStreamingOnSocket(fStreamStates[trackNum].tcpSocketNum, this, trackNum);
    }
    netAddressBits destinationAddress = 0;
    u_int8_t destinationTTL = 255;
#ifdef RTSP_ALLOW_CLIENT_DESTINATION_SETTING
    if (clientsDestinationAddressStr != NULL) {
      // Use the client-provided "destination" address.
      // Note: This potentially allows the server to be used in denial-of-service
      // attacks, so don't enable this code unless you're sure that clients are
      // trusted.
      destinationAddress = our_inet_addr(clientsDestinationAddressStr);
    }
    // Also use the client-provided TTL.
    destinationTTL = clientsDestinationTTL;
#endif
    delete[] clientsDestinationAddressStr;
    Port serverRTPPort(0);
    Port serverRTCPPort(0);

    //fprintf(stderr, "   hei hei 1\n");
    
    // Make sure that we transmit on the same interface that's used by the client (in case we're a multi-homed server):
    struct sockaddr_in sourceAddr; SOCKLEN_T namelen = sizeof sourceAddr;
    getsockname(ourClientConnection->fClientInputSocket, (struct sockaddr*)&sourceAddr, &namelen);
    netAddressBits origSendingInterfaceAddr = SendingInterfaceAddr;
    netAddressBits origReceivingInterfaceAddr = ReceivingInterfaceAddr;

    char sessionIdStr[8+1];
    sprintf(sessionIdStr, "%08X", fOurSessionId);
    sessionIdStr[8] = '\0';
    fprintf(stderr, "   hei hei 2 -> %s\n", sessionIdStr);

    subsession->getStreamParameters(fOurSessionId, ourClientConnection->fClientAddr.sin_addr.s_addr,
				    clientRTPPort, clientRTCPPort,
				    fStreamStates[trackNum].tcpSocketNum, rtpChannelId, rtcpChannelId,
				    destinationAddress, destinationTTL, fIsMulticast,
				    serverRTPPort, serverRTCPPort,
				    fStreamStates[trackNum].streamToken);

    fprintf(stderr, "   hei hei 3\n");

    SendingInterfaceAddr = origSendingInterfaceAddr;
    ReceivingInterfaceAddr = origReceivingInterfaceAddr;
    
    AddressString destAddrStr(destinationAddress);
    AddressString sourceAddrStr(sourceAddr);
    char timeoutParameterString[100];
    if (fOurRTSPServer.fReclamationSeconds > 0) {
      sprintf(timeoutParameterString, ";timeout=%u", fOurRTSPServer.fReclamationSeconds);
    } else {
      timeoutParameterString[0] = '\0';
    }
    if (fIsMulticast) {
      //fprintf(stderr, "   handleCmd_SETUP - this setup is multicast, if you get something else here there is something wrong\n");
      switch (streamingMode) {
          case RTP_UDP: {
          //fprintf(stderr, "   handleCmd_SETUP - this setup is RTP_UDP ok\n");



	    snprintf((char*)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
		     "RTSP/1.0 200 OK\r\n"
		     "CSeq: %s\r\n"
		     "%s"
		     "Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%d-%d;ttl=%d\r\n"
		     "Session: %08X%s\r\n\r\n",
		     ourClientConnection->fCurrentCSeq,
		     dateHeader(),
		     destAddrStr.val(), sourceAddrStr.val(), ntohs(serverRTPPort.num()), ntohs(serverRTCPPort.num()), destinationTTL,
		     fOurSessionId, timeoutParameterString);




	    break;
	  }
          case RTP_TCP: {
          //fprintf(stderr, "   handleCmd_SETUP - TCP multicast - impossible! Break down\n");
	    // multicast streams can't be sent via TCP
	    ourClientConnection->handleCmd_unsupportedTransport();
	    break;
	  }
          case RAW_UDP: {
            //fprintf(stderr, "   handleCmd_SETUP - this setup is RAW_UDP ok\n");
	    snprintf((char*)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
		     "RTSP/1.0 200 OK\r\n"
		     "CSeq: %s\r\n"
		     "%s"
		     "Transport: %s;multicast;destination=%s;source=%s;port=%d;ttl=%d\r\n"
		     "Session: %08X%s\r\n\r\n",
		     ourClientConnection->fCurrentCSeq,
		     dateHeader(),
		     streamingModeString, destAddrStr.val(), sourceAddrStr.val(), ntohs(serverRTPPort.num()), destinationTTL,
		     fOurSessionId, timeoutParameterString);
	    break;
	  }
      }
    } else {
      //fprintf(stderr, "   handleCmd_SETUP - this setup is NOT MULTICAST and thats a problem\n");
      switch (streamingMode) {
          case RTP_UDP: {
	    snprintf((char*)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
		     "RTSP/1.0 200 OK\r\n"
		     "CSeq: %s\r\n"
		     "%s"
		     "Transport: RTP/AVP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d\r\n"
		     "Session: %08X%s\r\n\r\n",
		     ourClientConnection->fCurrentCSeq,
		     dateHeader(),
		     destAddrStr.val(), sourceAddrStr.val(), ntohs(clientRTPPort.num()), ntohs(clientRTCPPort.num()), ntohs(serverRTPPort.num()), ntohs(serverRTCPPort.num()),
		     fOurSessionId, timeoutParameterString);
	    break;
	  }
          case RTP_TCP: {
	    if (!fOurRTSPServer.fAllowStreamingRTPOverTCP) {
	      ourClientConnection->handleCmd_unsupportedTransport();
	    } else {
	      snprintf((char*)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
		       "RTSP/1.0 200 OK\r\n"
		       "CSeq: %s\r\n"
		       "%s"
		       "Transport: RTP/AVP/TCP;unicast;destination=%s;source=%s;interleaved=%d-%d\r\n"
		       "Session: %08X%s\r\n\r\n",
		       ourClientConnection->fCurrentCSeq,
		       dateHeader(),
		       destAddrStr.val(), sourceAddrStr.val(), rtpChannelId, rtcpChannelId,
		       fOurSessionId, timeoutParameterString);
	    }
	    break;
	  }
          case RAW_UDP: {
	    snprintf((char*)ourClientConnection->fResponseBuffer, sizeof ourClientConnection->fResponseBuffer,
		     "RTSP/1.0 200 OK\r\n"
		     "CSeq: %s\r\n"
		     "%s"
		     "Transport: %s;unicast;destination=%s;source=%s;client_port=%d;server_port=%d\r\n"
		     "Session: %08X%s\r\n\r\n",
		     ourClientConnection->fCurrentCSeq,
		     dateHeader(),
		     streamingModeString, destAddrStr.val(), sourceAddrStr.val(), ntohs(clientRTPPort.num()), ntohs(serverRTPPort.num()),
		     fOurSessionId, timeoutParameterString);
	    break;
	  }
      }
    }
    delete[] streamingModeString;
  } while (0);
  
  delete[] concatenatedStreamName;
}
  
