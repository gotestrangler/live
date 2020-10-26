#include "include/ManifestRTPSink.hh"
#include "GroupsockHelper.hh"
//#define TEST_LOSS 1

ManifestRTPSink::ManifestRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
			     unsigned char rtpPayloadFormat,
			     unsigned rtpTimestampFrequency,
			     char const* sdpMediaTypeString,
			     char const* rtpPayloadFormatName,
			     unsigned numChannels,
			     Boolean allowMultipleFramesPerPacket,
			     Boolean doNormalMBitRule, u_int32_t firstTimestamp)
  : MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat,
		       rtpTimestampFrequency, rtpPayloadFormatName,
		       numChannels),
    fAllowMultipleFramesPerPacket(allowMultipleFramesPerPacket), fSetMBitOnNextPacket(False) {
  fSDPMediaTypeString
    = strDup(sdpMediaTypeString == NULL ? "unknown" : sdpMediaTypeString);
  fSetMBitOnLastFrames = doNormalMBitRule && strcmp(fSDPMediaTypeString, "audio") != 0;
}

ManifestRTPSink::~ManifestRTPSink() {
  delete[] (char*)fSDPMediaTypeString;
}

ManifestRTPSink*
ManifestRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			 unsigned char rtpPayloadFormat,
			 unsigned rtpTimestampFrequency,
			 char const* sdpMediaTypeString,
			 char const* rtpPayloadFormatName,
			 unsigned numChannels,
			 Boolean allowMultipleFramesPerPacket,
			 Boolean doNormalMBitRule, u_int32_t firstTimestamp) {
  return new ManifestRTPSink(env, RTPgs,
			   rtpPayloadFormat, rtpTimestampFrequency,
			   sdpMediaTypeString, rtpPayloadFormatName,
			   numChannels,
			   allowMultipleFramesPerPacket,
			   doNormalMBitRule, firstTimestamp);
}

Boolean ManifestRTPSink::continuePlaying() {
  fprintf(stderr, "\n     MultiFramedRTPSink::continuePlaying()\n");
  // Send the first packet.
  // (This will also schedule any future sends.)
  buildAndSendPacket(True);
  return True;
}

void ManifestRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
					   unsigned char* frameStart,
					   unsigned numBytesInFrame,
					   struct timeval framePresentationTime,
					   unsigned numRemainingBytes) {


  if (numRemainingBytes == 0) {
    // This packet contains the last (or only) fragment of the frame.
    // Set the RTP 'M' ('marker') bit, if appropriate:
    if (fSetMBitOnLastFrames) setMarkerBit();
  }
  if (fSetMBitOnNextPacket) {
    // An external object has asked for the 'M' bit to be set on the next packet:
    setMarkerBit();
    fSetMBitOnNextPacket = False;
  }


  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
					     frameStart, numBytesInFrame,
					     framePresentationTime,
					     numRemainingBytes);
}


void ManifestRTPSink::setCheckSource(CheckSource* ns){
    fprintf(stderr, "\n ManifestRTPSink::setCheckSource\n");

    checkSource = ns; 
}




void ManifestRTPSink::buildAndSendPacket(Boolean isFirstPacket) {

  fprintf(stderr, "ManifestRTPSink buildandsendpacket %d\n", fSeqNo);
  nextTask() = NULL;
  fIsFirstPacket = isFirstPacket;

  // Set up the RTP header:
  //setXBit();
  //unsigned rtpHdr = 0x80000000; // RTP version 2; marker ('M') bit not set (by default; it can be set later)
  unsigned rtpHdr = 0x90000000;
  rtpHdr |= (fRTPPayloadType<<16);
  rtpHdr |= fSeqNo; // sequence number
  fOutBuf->enqueueWord(rtpHdr);



  // Note where the RTP timestamp will go.
  // (We can't fill this in until we start packing payload frames.)
  fTimestampPosition = fOutBuf->curPacketSize();
  fOutBuf->skipBytes(4); // leave a hole for the timestamp

  fOutBuf->enqueueWord(SSRC());
  //fOutBuf->skipBytes(4); 
  //TODO: Make sure you didnt ruin anything now!!


  //unsigned short chunkreftest = 9;
  
  unsigned short chunkreftest = checkSource->getNowChunk();
  unsigned vdhdr = chunkreftest<<16;
  vdhdr |= 1; 
  fOutBuf->enqueueWord(vdhdr);
  //setSpecialHeaderWord(vdhdr, fOutBuf->curPacketSize());






  // Begin packing as many (complete) frames into the packet as we can:
  fTotalFrameSpecificHeaderSizes = 0;
  fNoFramesLeft = False;
  fNumFramesUsedSoFar = 0;
  packFrame();
}


// The following is called after each delay between packet sends:
void ManifestRTPSink::sendNext(void* firstArg) {
  //fprintf(stderr, "\n     MultiFramedRTPSink::sendNext\n");
  ManifestRTPSink* sink = (ManifestRTPSink*)firstArg;
  sink->buildAndSendPacket(False);
}



void ManifestRTPSink::packFrame() {

  fprintf(stderr, "ManifestRTPSink::packFrame()\n");

  // Get the next frame.

  // First, skip over the space we'll use for any frame-specific header:
  fCurFrameSpecificHeaderPosition = fOutBuf->curPacketSize();
  fCurFrameSpecificHeaderSize = frameSpecificHeaderSize();
  fOutBuf->skipBytes(fCurFrameSpecificHeaderSize);
  fTotalFrameSpecificHeaderSizes += fCurFrameSpecificHeaderSize;

  // See if we have an overflow frame that was too big for the last pkt
  if (fOutBuf->haveOverflowData()) {
    // Use this frame before reading a new one from the source
    unsigned frameSize = fOutBuf->overflowDataSize();
    struct timeval presentationTime = fOutBuf->overflowPresentationTime();
    unsigned durationInMicroseconds = fOutBuf->overflowDurationInMicroseconds();
    fOutBuf->useOverflowData();

    afterGettingFrame1(frameSize, 0, presentationTime, durationInMicroseconds);
  } else {
    // Normal case: we need to read a new frame from the source
    if (fSource == NULL) return;
    fSource->getNextFrame(fOutBuf->curPtr(), fOutBuf->totalBytesAvailable(),
			  afterGettingFrame, this, ourHandleClosure, this);
  }
}

void ManifestRTPSink::afterGettingFrame(void* clientData, unsigned numBytesRead,
		    unsigned numTruncatedBytes,
		    struct timeval presentationTime,
		    unsigned durationInMicroseconds) {
  fprintf(stderr, "ManifestRTPSink::afterGettingFrame\n");
  ManifestRTPSink* sink = (ManifestRTPSink*)clientData;
  sink->afterGettingFrame1(numBytesRead, numTruncatedBytes,
			   presentationTime, durationInMicroseconds);
}

void ManifestRTPSink
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
		     struct timeval presentationTime,
		     unsigned durationInMicroseconds) {
  
  fprintf(stderr, "ManifestRTPSink::afterGettingFrame1\n");

  if (fIsFirstPacket) {
    // Record the fact that we're starting to play now:
    gettimeofday(&fNextSendTime, NULL);
  }

  fMostRecentPresentationTime = presentationTime;
  if (fInitialPresentationTime.tv_sec == 0 && fInitialPresentationTime.tv_usec == 0) {
    fInitialPresentationTime = presentationTime;
  }    

  if (numTruncatedBytes > 0) {
    unsigned const bufferSize = fOutBuf->totalBytesAvailable();
    envir() << "MultiFramedRTPSink::afterGettingFrame1(): The input frame data was too large for our buffer size ("
	    << bufferSize << ").  "
	    << numTruncatedBytes << " bytes of trailing data was dropped!  Correct this by increasing \"OutPacketBuffer::maxSize\" to at least "
	    << OutPacketBuffer::maxSize + numTruncatedBytes << ", *before* creating this 'RTPSink'.  (Current value is "
	    << OutPacketBuffer::maxSize << ".)\n";
  }
  unsigned curFragmentationOffset = fCurFragmentationOffset;
  unsigned numFrameBytesToUse = frameSize;
  unsigned overflowBytes = 0;

  // If we have already packed one or more frames into this packet,
  // check whether this new frame is eligible to be packed after them.
  // (This is independent of whether the packet has enough room for this
  // new frame; that check comes later.)
  if (fNumFramesUsedSoFar > 0) {
    if ((fPreviousFrameEndedFragmentation
	 && !allowOtherFramesAfterLastFragment())
	|| !frameCanAppearAfterPacketStart(fOutBuf->curPtr(), frameSize)) {
      // Save away this frame for next time:
      numFrameBytesToUse = 0;
      fOutBuf->setOverflowData(fOutBuf->curPacketSize(), frameSize,
			       presentationTime, durationInMicroseconds);
    }
  }
  fPreviousFrameEndedFragmentation = False;

  if (numFrameBytesToUse > 0) {
    // Check whether this frame overflows the packet
    if (fOutBuf->wouldOverflow(frameSize)) {
      // Don't use this frame now; instead, save it as overflow data, and
      // send it in the next packet instead.  However, if the frame is too
      // big to fit in a packet by itself, then we need to fragment it (and
      // use some of it in this packet, if the payload format permits this.)
      if (isTooBigForAPacket(frameSize)
          && (fNumFramesUsedSoFar == 0 || allowFragmentationAfterStart())) {
        // We need to fragment this frame, and use some of it now:
        overflowBytes = computeOverflowForNewFrame(frameSize);
        numFrameBytesToUse -= overflowBytes;
        fCurFragmentationOffset += numFrameBytesToUse;
      } else {
        // We don't use any of this frame now:
        overflowBytes = frameSize;
        numFrameBytesToUse = 0;
      }
      fOutBuf->setOverflowData(fOutBuf->curPacketSize() + numFrameBytesToUse,
			       overflowBytes, presentationTime, durationInMicroseconds);
    } else if (fCurFragmentationOffset > 0) {
      // This is the last fragment of a frame that was fragmented over
      // more than one packet.  Do any special handling for this case:
      fCurFragmentationOffset = 0;
      fPreviousFrameEndedFragmentation = True;
    }
  }

  if (numFrameBytesToUse == 0 && frameSize > 0) {
    // Send our packet now, because we have filled it up:
    sendPacketIfNecessary();
  } else {
    // Use this frame in our outgoing packet:
    unsigned char* frameStart = fOutBuf->curPtr();
    fOutBuf->increment(numFrameBytesToUse);
        // do this now, in case "doSpecialFrameHandling()" calls "setFramePadding()" to append padding bytes

    // Here's where any payload format specific processing gets done:
    doSpecialFrameHandling(curFragmentationOffset, frameStart,
			   numFrameBytesToUse, presentationTime,
			   overflowBytes);

    ++fNumFramesUsedSoFar;

    // Update the time at which the next packet should be sent, based
    // on the duration of the frame that we just packed into it.
    // However, if this frame has overflow data remaining, then don't
    // count its duration yet.
    if (overflowBytes == 0) {
      fNextSendTime.tv_usec += durationInMicroseconds;
      fNextSendTime.tv_sec += fNextSendTime.tv_usec/1000000;
      fNextSendTime.tv_usec %= 1000000;
    }

    // Send our packet now if (i) it's already at our preferred size, or
    // (ii) (heuristic) another frame of the same size as the one we just
    //      read would overflow the packet, or
    // (iii) it contains the last fragment of a fragmented frame, and we
    //      don't allow anything else to follow this or
    // (iv) only one frame per packet is allowed:
    if (fOutBuf->isPreferredSize()
        || fOutBuf->wouldOverflow(numFrameBytesToUse)
        || (fPreviousFrameEndedFragmentation &&
            !allowOtherFramesAfterLastFragment())
        || !frameCanAppearAfterPacketStart(fOutBuf->curPtr() - frameSize,
					   frameSize) ) {
      // The packet is ready to be sent now
      sendPacketIfNecessary();
    } else {
      // There's room for more frames; try getting another:
      packFrame();
    }
  }
}

static unsigned const rtpHeaderSize = 12;

void ManifestRTPSink::sendPacketIfNecessary() {
  fprintf(stderr, "\n     sendPacketIfNecessary()\n");
  if (fNumFramesUsedSoFar > 0) {
    // Send the packet:
#ifdef TEST_LOSS
    if ((our_random()%10) != 0) // simulate 10% packet loss #####
#endif
      if (!fRTPInterface.sendPacket(fOutBuf->packet(), fOutBuf->curPacketSize())) {
	// if failure handler has been specified, call it
	if (fOnSendErrorFunc != NULL) (*fOnSendErrorFunc)(fOnSendErrorData);
      }
    ++fPacketCount;
    fTotalOctetCount += fOutBuf->curPacketSize();
    fOctetCount += fOutBuf->curPacketSize()
      - rtpHeaderSize - fSpecialHeaderSize - fTotalFrameSpecificHeaderSizes;

    ++fSeqNo; // for next time
  }
  if (fOutBuf->haveOverflowData()
      && fOutBuf->totalBytesAvailable() > fOutBuf->totalBufferSize()/2) {
    // Efficiency hack: Reset the packet start pointer to just in front of
    // the overflow data (allowing for the RTP header and special headers),
    // so that we probably don't have to "memmove()" the overflow data
    // into place when building the next packet:
    unsigned newPacketStart = fOutBuf->curPacketSize()
      - (rtpHeaderSize + fSpecialHeaderSize + frameSpecificHeaderSize());
    fOutBuf->adjustPacketStart(newPacketStart);
  } else {
    // Normal case: Reset the packet start pointer back to the start:
    fOutBuf->resetPacketStart();
  }
  fOutBuf->resetOffset();
  fNumFramesUsedSoFar = 0;

  if (fNoFramesLeft) {
      //fprintf(stderr, "\n     sendPacketIfNecessary() -> fNoFramesLeft\n");

    
    // We're done:
    onSourceClosure();
  } else {
    // We have more frames left to send.  Figure out when the next frame
    // is due to start playing, then make sure that we wait this long before
    // sending the next packet.
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    int secsDiff = fNextSendTime.tv_sec - timeNow.tv_sec;
    int64_t uSecondsToGo = secsDiff*1000000 + (fNextSendTime.tv_usec - timeNow.tv_usec);
    if (uSecondsToGo < 0 || secsDiff < 0) { // sanity check: Make sure that the time-to-delay is non-negative:
      uSecondsToGo = 0;
    }

    // Delay this amount of time:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecondsToGo, (TaskFunc*)sendNext, this);
  }
}