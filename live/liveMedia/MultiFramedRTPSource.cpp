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
// RTP source for a common kind of payload format: Those that pack multiple,
// complete codec frames (as many as possible) into each RTP packet.
// Implementation

#include "MultiFramedRTPSource.hh"
#include "RTCP.hh"
#include "GroupsockHelper.hh"
#include <string.h>
#include "stdio.h"
#include <stdlib.h>

#define TEST_LOSS 1

////////// ReorderingPacketBuffer definition //////////

class ReorderingPacketBuffer {
public:
  ReorderingPacketBuffer(BufferedPacketFactory* packetFactory);
  virtual ~ReorderingPacketBuffer();
  void reset();

<<<<<<< HEAD

  //I made these: 
  BufferedPacket* getLastCompletedPacket();
  BufferedPacket* getHead(){return fHeadPacket;}
  BufferedPacket* getTail(){return fTailPacket;}
  unsigned short getChunk(){return curChunk;}
  void setChunk(unsigned short in){curChunk = in;}

=======
  FILE * pullChunk(unsigned short numb, struct sockaddr_in* addr);
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
  BufferedPacket* getFreePacket(MultiFramedRTPSource* ourSource);
  Boolean storePacket(BufferedPacket* bPacket);
  BufferedPacket* getNextCompletedPacket(Boolean& packetLossPreceded);
  BufferedPacket* getLastCompletedPacket();
  void releaseUsedPacket(BufferedPacket* packet);
  void freePacket(BufferedPacket* packet) {
    if (packet != fSavedPacket) {
      delete packet;
    } else {
      fSavedPacketFree = True;
    }
  }
  Boolean isEmpty() const { return fHeadPacket == NULL; }
  BufferedPacket* getHead() { return fHeadPacket; } //Hihihihi
  BufferedPacket* getTail() { return fTailPacket; } //Gigidigidig 
  unsigned short getChunk() { return curChunk; }
  void setChunk(unsigned short in) {  curChunk = in; }


  void setThresholdTime(unsigned uSeconds) { fThresholdTime = uSeconds; }
  void resetHaveSeenFirstPacket() { fHaveSeenFirstPacket = False; }

private:
  BufferedPacketFactory* fPacketFactory;
  unsigned fThresholdTime; // uSeconds
  Boolean fHaveSeenFirstPacket; // used to set initial "fNextExpectedSeqNo"
  unsigned short fNextExpectedSeqNo;
  unsigned short curChunk;
  BufferedPacket* fHeadPacket;
  BufferedPacket* fTailPacket;
  BufferedPacket* fSavedPacket;
      // to avoid calling new/free in the common case
  Boolean fSavedPacketFree;
  unsigned short curChunk;
};


////////// MultiFramedRTPSource implementation //////////

MultiFramedRTPSource
::MultiFramedRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		       unsigned char rtpPayloadFormat,
		       unsigned rtpTimestampFrequency,
	         BufferedPacketFactory* packetFactory)
  : RTPSource(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency) {
  reset();
  fReorderingBuffer = new ReorderingPacketBuffer(packetFactory);
  
  fprintf(stderr, "\n\n    lager MultiFramedRTPSource  \n");
  sleep(5);

  // Try to use a big receive buffer for RTP:
  increaseReceiveBufferTo(env, RTPgs->socketNum(), 50*1024);
  first = 0;
  second = 0; 
  third = 0; 
  fourth = 0;
}

void MultiFramedRTPSource::reset() {
  fCurrentPacketBeginsFrame = True; // by default
  fCurrentPacketCompletesFrame = True; // by default
  fAreDoingNetworkReads = False;
  fPacketReadInProgress = NULL;
  fNeedDelivery = False;
  fPacketLossInFragmentedFrame = False;
}

MultiFramedRTPSource::~MultiFramedRTPSource() {
  delete fReorderingBuffer;
}

Boolean MultiFramedRTPSource
::processSpecialHeader(BufferedPacket* /*packet*/,
		       unsigned& resultSpecialHeaderSize) {
  // Default implementation: Assume no special header:
  resultSpecialHeaderSize = 0;
  return True;
}

Boolean MultiFramedRTPSource
::packetIsUsableInJitterCalculation(unsigned char* /*packet*/,
				    unsigned /*packetSize*/) {
  // Default implementation:
  return True;
}

void MultiFramedRTPSource::doStopGettingFrames() {
  if (fPacketReadInProgress != NULL) {
    fReorderingBuffer->freePacket(fPacketReadInProgress);
    fPacketReadInProgress = NULL;
  }
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  fRTPInterface.stopNetworkReading();
  fReorderingBuffer->reset();
  reset();
}

void MultiFramedRTPSource::doGetNextFrame() {
  fprintf(stderr, "\n     MultiFramedRTPSource::doGetNextFrame()\n");
  if (!fAreDoingNetworkReads) {
    // Turn on background read handling of incoming packets:
    fAreDoingNetworkReads = True;
    TaskScheduler::BackgroundHandlerProc* handler
      = (TaskScheduler::BackgroundHandlerProc*)&networkReadHandler;
    fRTPInterface.startNetworkReading(handler);
  }

  fSavedTo = fTo;
  fSavedMaxSize = fMaxSize;
  fFrameSize = 0; // for now
  fNeedDelivery = True;
  doGetNextFrame2();
}

void MultiFramedRTPSource::doGetNextFrame1() {

  while (fNeedDelivery) {
    // If we already have packet data available, then deliver it now.
    Boolean packetLossPrecededThis;
    BufferedPacket* nextPacket
      = fReorderingBuffer->getLastCompletedPacket();
    if (nextPacket == NULL) break;

    fNeedDelivery = False;

    if (nextPacket->useCount() == 0) {
      // Before using the packet, check whether it has a special header
      // that needs to be processed:
      unsigned specialHeaderSize;
      if (!processSpecialHeader(nextPacket, specialHeaderSize)) {
	// Something's wrong with the header; reject the packet:
	fReorderingBuffer->releaseUsedPacket(nextPacket);
	fNeedDelivery = True;
	continue;
      }
      nextPacket->skip(specialHeaderSize);
    }

    // Check whether we're part of a multi-packet frame, and whether
    // there was packet loss that would render this packet unusable:
    if (fCurrentPacketBeginsFrame) {
      if (packetLossPrecededThis || fPacketLossInFragmentedFrame) {
	// We didn't get all of the previous frame.
	// Forget any data that we used from it:
	fTo = fSavedTo; fMaxSize = fSavedMaxSize;
	fFrameSize = 0;
      }
      fPacketLossInFragmentedFrame = False;
    } else if (packetLossPrecededThis) {
      // We're in a multi-packet frame, with preceding packet loss
      fPacketLossInFragmentedFrame = True;
    }
    if (fPacketLossInFragmentedFrame) {
      // This packet is unusable; reject it:
      fReorderingBuffer->releaseUsedPacket(nextPacket);
      fNeedDelivery = True;
      continue;
    }

 unsigned short frek = fReorderingBuffer->getChunk();
 unsigned frameSize;
 int gunna = 0; 
    if(nextPacket->nowChunk() != frek){
      // The packet is usable. Deliver all or part of it to our caller:
      while(nextPacket != NULL){
      fprintf(stderr, "\n     in the while loop. gunna is %d\n", gunna++);

      nextPacket->use(fTo, fMaxSize, frameSize, fNumTruncatedBytes,
          fCurPacketRTPSeqNum, fCurPacketRTPTimestamp,
          fPresentationTime, fCurPacketHasBeenSynchronizedUsingRTCP,
          fCurPacketMarkerBit);
      fFrameSize += frameSize;
      if (!nextPacket->hasUsableData()) {
        // We're completely done with this packet now
        fReorderingBuffer->releaseUsedPacket(nextPacket);
      }
      nextPacket = fReorderingBuffer->getLastCompletedPacket();    



      }
    }


    if (fCurrentPacketCompletesFrame && fFrameSize > 0) {
        fprintf(stderr, "\n     fCurrentPacketCompletesFrame && fFrameSize > 0\n");

      // We have all the data that the client wants.
      if (fNumTruncatedBytes > 0) {
	envir() << "MultiFramedRTPSource::doGetNextFrame1(): The total received frame size exceeds the client's buffer size ("
		<< fSavedMaxSize << ").  "
		<< fNumTruncatedBytes << " bytes of trailing data will be dropped!\n";
      }
      // Call our own 'after getting' function, so that the downstream object can consume the data:
      if (fReorderingBuffer->isEmpty()) {
      fprintf(stderr, "\n     buffer is empty\n");

	// Common case optimization: There are no more queued incoming packets, so this code will not get
	// executed again without having first returned to the event loop.  Call our 'after getting' function
	// directly, because there's no risk of a long chain of recursion (and thus stack overflow):
	afterGetting(this);
      } else {
        fprintf(stderr, "\n     setting next task afterGetting\n");

	// Special case: Call our 'after getting' function via the event loop.
	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
								 (TaskFunc*)FramedSource::afterGetting, this);
      }
    } else {
      fprintf(stderr, "\n     keep getting data\n");

      // This packet contained fragmented data, and does not complete
      // the data that the client wants.  Keep getting data:
      fTo += frameSize; fMaxSize -= frameSize;
      fNeedDelivery = True;
    }
  }
}



void MultiFramedRTPSource::doGetNextFrame2() {

  while (fNeedDelivery) {
    // If we already have packet data available, then deliver it now.
    Boolean packetLossPrecededThis;
    BufferedPacket* nextPacket
      = fReorderingBuffer->getNextCompletedPacket(packetLossPrecededThis);
    if (nextPacket == NULL) break;

    fNeedDelivery = False;

    if (nextPacket->useCount() == 0) {
      // Before using the packet, check whether it has a special header
      // that needs to be processed:
      unsigned specialHeaderSize;
      if (!processSpecialHeader(nextPacket, specialHeaderSize)) {
	// Something's wrong with the header; reject the packet:
	fReorderingBuffer->releaseUsedPacket(nextPacket);
	fNeedDelivery = True;
	continue;
      }
      nextPacket->skip(specialHeaderSize);
    }

    // Check whether we're part of a multi-packet frame, and whether
    // there was packet loss that would render this packet unusable:
    if (fCurrentPacketBeginsFrame) {
      if (packetLossPrecededThis || fPacketLossInFragmentedFrame) {
	// We didn't get all of the previous frame.
	// Forget any data that we used from it:
	fTo = fSavedTo; fMaxSize = fSavedMaxSize;
	fFrameSize = 0;
      }
      fPacketLossInFragmentedFrame = False;
    } else if (packetLossPrecededThis) {
      // We're in a multi-packet frame, with preceding packet loss
      fPacketLossInFragmentedFrame = True;
    }
    if (fPacketLossInFragmentedFrame) {
      // This packet is unusable; reject it:
      fReorderingBuffer->releaseUsedPacket(nextPacket);
      fNeedDelivery = True;
      continue;
    }

<<<<<<< HEAD
      fprintf(stderr, "first %u second %u third %u fourth %u\n\n", first, second, third, fourth);

    if(packetLossPrecededThis){
      struct sockaddr_in* sa = nextPacket->addr();
      char buffer[INET_ADDRSTRLEN];
      inet_ntop( AF_INET, &sa->sin_addr, buffer, sizeof( buffer ));
      
      FramedSource::setAddr(sa);
      FramedSource::setPacketLossNotice();
      if(first == 0){
        first = fCurPacketRTPSeqNum;
        second = fCurPacketRTPSeqNum;
        third = fCurPacketRTPSeqNum;
        fourth = fCurPacketRTPSeqNum;
      }
      if(second == first){
        second = fCurPacketRTPSeqNum;
        third = fCurPacketRTPSeqNum;
        fourth = fCurPacketRTPSeqNum;
      }
      if(third == second){
        third = fCurPacketRTPSeqNum;
        fourth = fCurPacketRTPSeqNum;
      }
      if(fourth == third){
        fourth = fCurPacketRTPSeqNum;
      }
    }

=======




    /*
    if(fReorderingBuffer->getHead() != NULL){
      fprintf(stderr, "\n\n\n     ChunkCheck head: %u\n", fReorderingBuffer->getHead()->nowChunk());

      if(fReorderingBuffer->getTail() != NULL){
        fprintf(stderr, "\n\n\n     ChunkCheck tail: %u\n", fReorderingBuffer->getTail()->nowChunk());
        unsigned short frek = fReorderingBuffer->getChunk();
        if(nextPacket->nowChunk() != frek){
          fprintf(stderr, "\n\n\n     There is a new chunk in town %u, the last one is: %u\n", nextPacket->nowChunk(), frek);
          fReorderingBuffer->setChunk(nextPacket->nowChunk());

          FILE * makePacket = fReorderingBuffer->pullChunk(nextPacket->nowChunk(), nextPacket->addr());
          sleep(3);

        }

      }

  }*/


    //1. Check if the packet is the first in a new chunk 
    //2. Send out the saved packets from the last chunk 
    //3. Save the new packet
    //4. Try a switch up 
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0

    // The packet is usable. Deliver all or part of it to our caller:
    unsigned frameSize;
    nextPacket->use(fTo, fMaxSize, frameSize, fNumTruncatedBytes,
		    fCurPacketRTPSeqNum, fCurPacketRTPTimestamp,
		    fPresentationTime, fCurPacketHasBeenSynchronizedUsingRTCP,
		    fCurPacketMarkerBit);
    fFrameSize += frameSize;

    if (!nextPacket->hasUsableData()) {
      // We're completely done with this packet now
      fReorderingBuffer->releaseUsedPacket(nextPacket);
    }






    if (fCurrentPacketCompletesFrame && fFrameSize > 0) {
        fprintf(stderr, "\n     fCurrentPacketCompletesFrame && fFrameSize > 0\n");

      // We have all the data that the client wants.
      if (fNumTruncatedBytes > 0) {
	envir() << "MultiFramedRTPSource::doGetNextFrame1(): The total received frame size exceeds the client's buffer size ("
		<< fSavedMaxSize << ").  "
		<< fNumTruncatedBytes << " bytes of trailing data will be dropped!\n";
      }
      // Call our own 'after getting' function, so that the downstream object can consume the data:
      if (fReorderingBuffer->isEmpty()) {
      fprintf(stderr, "\n     buffer is empty\n");

	// Common case optimization: There are no more queued incoming packets, so this code will not get
	// executed again without having first returned to the event loop.  Call our 'after getting' function
	// directly, because there's no risk of a long chain of recursion (and thus stack overflow):
	afterGetting(this);
      } else {
        fprintf(stderr, "\n     setting next task afterGetting\n");

	// Special case: Call our 'after getting' function via the event loop.
	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
								 (TaskFunc*)FramedSource::afterGetting, this);
      }
    } else {
      fprintf(stderr, "\n     keep getting data\n");

      // This packet contained fragmented data, and does not complete
      // the data that the client wants.  Keep getting data:
      fTo += frameSize; fMaxSize -= frameSize;
      fNeedDelivery = True;
    }








  }
}

<<<<<<< HEAD
=======
void MultiFramedRTPSource::doGetNextFrame2() {
  
  
    BufferedPacket* nextPacket
      = fReorderingBuffer->getLastCompletedPacket();
    
    if (nextPacket == NULL){
      return;
    } 

    //1. Check if the packet is the first in a new chunk 
    //2. Send out the saved packets from the last chunk 
    //3. Save the new packet
    //4. Try a switch up 

    unsigned short frek = fReorderingBuffer->getChunk();
    if(nextPacket->nowChunk() != frek){
      
      Boolean packetLossPrecededThis;
      BufferedPacket* headPacket
      = fReorderingBuffer->getNextCompletedPacket(packetLossPrecededThis);
      while(headPacket != NULL){
        if (headPacket->useCount() == 0) {
          // Before using the packet, check whether it has a special header
          // that needs to be processed:
          unsigned specialHeaderSize;
          if (!processSpecialHeader(headPacket, specialHeaderSize)) {
            // Something's wrong with the header; reject the packet:
            fReorderingBuffer->releaseUsedPacket(headPacket);
            fNeedDelivery = True;
            continue;
          }
          headPacket->skip(specialHeaderSize);
        }

        // The packet is usable. Deliver all or part of it to our caller:
        unsigned frameSize;
        headPacket->use(fTo, fMaxSize, frameSize, fNumTruncatedBytes,
		    fCurPacketRTPSeqNum, fCurPacketRTPTimestamp,
		    fPresentationTime, fCurPacketHasBeenSynchronizedUsingRTCP,
		    fCurPacketMarkerBit);
        fFrameSize += frameSize;


        if (!nextPacket->hasUsableData()) {
          // We're completely done with this packet now
          fReorderingBuffer->releaseUsedPacket(nextPacket);
        }


        headPacket = fReorderingBuffer->getNextCompletedPacket(packetLossPrecededThis);
      }




    }



    /*
    if(fReorderingBuffer->getHead() != NULL){
      fprintf(stderr, "\n\n\n     ChunkCheck head: %u\n", fReorderingBuffer->getHead()->nowChunk());

      if(fReorderingBuffer->getTail() != NULL){
        fprintf(stderr, "\n\n\n     ChunkCheck tail: %u\n", fReorderingBuffer->getTail()->nowChunk());
        unsigned short frek = fReorderingBuffer->getChunk();
        if(nextPacket->nowChunk() != frek){
          fprintf(stderr, "\n\n\n     There is a new chunk in town %u, the last one is: %u\n", nextPacket->nowChunk(), frek);
          fReorderingBuffer->setChunk(nextPacket->nowChunk());

          FILE * makePacket = fReorderingBuffer->pullChunk(nextPacket->nowChunk(), nextPacket->addr());
          sleep(3);

        }

      }

  }*/



    if (fCurrentPacketCompletesFrame && fFrameSize > 0) {
      // We have all the data that the client wants.
      if (fNumTruncatedBytes > 0) {
	envir() << "MultiFramedRTPSource::doGetNextFrame1(): The total received frame size exceeds the client's buffer size ("
		<< fSavedMaxSize << ").  "
		<< fNumTruncatedBytes << " bytes of trailing data will be dropped!\n";
      }
      // Call our own 'after getting' function, so that the downstream object can consume the data:
      if (fReorderingBuffer->isEmpty()) {
	// Common case optimization: There are no more queued incoming packets, so this code will not get
	// executed again without having first returned to the event loop.  Call our 'after getting' function
	// directly, because there's no risk of a long chain of recursion (and thus stack overflow):
	afterGetting(this);
      } else {
	// Special case: Call our 'after getting' function via the event loop.
	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
								 (TaskFunc*)FramedSource::afterGetting, this);
      }
    } else {
            fprintf(stderr, "\n\n\n something is wrong. you deleted framesize, check doGetNextFrame1()\n");
            exit(0);

    }

  
}
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0

void MultiFramedRTPSource
::setPacketReorderingThresholdTime(unsigned uSeconds) {
  fReorderingBuffer->setThresholdTime(uSeconds);
}

#define ADVANCE(n) do { bPacket->skip(n); } while (0)

void MultiFramedRTPSource::networkReadHandler(MultiFramedRTPSource* source, int /*mask*/) {
  source->networkReadHandler1();
}

void MultiFramedRTPSource::networkReadHandler1() {
<<<<<<< HEAD

  unsigned chunkY;
=======
    

>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
  
  BufferedPacket* bPacket = fPacketReadInProgress;
  unsigned short chunkRef;
  if (bPacket == NULL) {
    // Normal case: Get a free BufferedPacket descriptor to hold the new network packet:
    bPacket = fReorderingBuffer->getFreePacket(this);
  }

  // Read the network packet, and perform sanity checks on the RTP header:
  Boolean readSuccess = False;
  do {
    

    struct sockaddr_in fromAddress;

    Boolean packetReadWasIncomplete = fPacketReadInProgress != NULL;
    if (!bPacket->fillInData(fRTPInterface, fromAddress, packetReadWasIncomplete)) {
      if (bPacket->bytesAvailable() == 0) { // should not happen??
	envir() << "MultiFramedRTPSource internal error: Hit limit when reading incoming packet over TCP\n";
      }
      fPacketReadInProgress = NULL;
      break;
    }
    if (packetReadWasIncomplete) {
      // We need additional read(s) before we can process the incoming packet:
      fPacketReadInProgress = bPacket;
      return;
    } else {
      fPacketReadInProgress = NULL;
    }
#ifdef TEST_LOSS
    setPacketReorderingThresholdTime(0);
       // don't wait for 'lost' packets to arrive out-of-order later
    if ((our_random()%10) == 0) break; // simulate 10% packet loss
#endif

    
    // Check for the 12-byte RTP header:
    if (bPacket->dataSize() < 12) break;
    unsigned rtpHdr = ntohl(*(u_int32_t*)(bPacket->data())); ADVANCE(4);
    Boolean rtpMarkerBit = (rtpHdr&0x00800000) != 0;
    unsigned rtpTimestamp = ntohl(*(u_int32_t*)(bPacket->data()));ADVANCE(4);
    unsigned rtpSSRC = ntohl(*(u_int32_t*)(bPacket->data())); ADVANCE(4);
    //unsigned ext = ntohl(*(u_int32_t*)(bPacket->data())); ADVANCE(4);
    fprintf(stderr, "\n     MultiFramedRTPSource::networkReadHandler ssrc %lu\n", rtpSSRC);
    //fprintf(stderr, "\n     MultiFramedRTPSource::networkReadHandler exthdr %lu\n", ext);
  //pull 

    struct in_addr destinationAddress;
    destinationAddress.s_addr = rtpSSRC;


    // now get it back and print it
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(destinationAddress), str, INET_ADDRSTRLEN);

    printf("bums bums %s\n", str); // prints "192.0.2.33"





  
    // Check the RTP version number (it should be 2):
    //fprintf(stderr, "Checking the RTP number\n");
    if ((rtpHdr&0xC0000000) != 0x80000000){
      fprintf(stderr, "OMG IT BREAKS MOTHER\n");
      break;
    } 

    // Check the Payload Type.
    unsigned char rtpPayloadType = (unsigned char)((rtpHdr&0x007F0000)>>16);
    if (rtpPayloadType != rtpPayloadFormat()) {
      

      //fprintf(stderr, "it is rtp format, correct!\n");
      if (fRTCPInstanceForMultiplexedRTCPPackets != NULL
	  && rtpPayloadType >= 64 && rtpPayloadType <= 95) {
	// This is a multiplexed RTCP packet, and we've been asked to deliver such packets.
	// Do so now:
	fRTCPInstanceForMultiplexedRTCPPackets
	  ->injectReport(bPacket->data()-12, bPacket->dataSize()+12, fromAddress);
      }
      break;
    }

    // Skip over any CSRC identifiers in the header:
    unsigned cc = (rtpHdr>>24)&0x0F;
    if (bPacket->dataSize() < cc*4) break;
    ADVANCE(cc*4);

    // Check for (& ignore) any RTP header extension
    
    if (rtpHdr&0x10000000) {
<<<<<<< HEAD

      fprintf(stderr, "The X bit is set, which means that this contains an exthdr\n");






      if (bPacket->dataSize() < 4){
        break;
      } 


      unsigned extHdr = ntohl(*(u_int32_t*)(bPacket->data())); ADVANCE(4);
      unsigned remExtSize = 4*(extHdr&0xFFFF);
      fprintf(stderr, "\n -> the exthedrsize: %u\n", remExtSize);

      chunkY = extHdr>>16;
      FramedSource::setCurChunk(chunkY);
      //fCurChunk = chunkY;
      fprintf(stderr, "\n\n\nChunky right now: %u -> the exthedrsize: %u\n", chunkY, remExtSize);

      if (bPacket->dataSize() < remExtSize){
=======
      fprintf(stderr, "\n     MultiFramedRTPSource::networkReadHandler ssrc 2 %u\n", rtpSSRC);

      if (bPacket->dataSize() < 4){
        fprintf(stderr, "\n     MultiFramedRTPSource::networkReadHandler1 -> break\n");

        break;
      } 
      unsigned extHdr = ntohl(*(u_int32_t*)(bPacket->data())); 
      
      ADVANCE(4);
      
            
      unsigned remExtSize = 4*(extHdr&0xFFFF);
      chunkRef = extHdr>>16;

      fprintf(stderr, "\n     MultiFramedRTPSource::networkReadHandler extHdr: %u and the remExtSize: %u\nchunk reference: %hu\nlength reference: %hu\n", extHdr, remExtSize, extHdr>>16, extHdr);

      // The first 32-bit word contains a profile-specific identifier (16 bits) and a length specifier (16 bits) 
      //that indicates the length of the extension in 32-bit units, excluding the 32 bits of the extension header. 
      //The extension header data follows.
      if (bPacket->dataSize() < remExtSize){
        fprintf(stderr, "\n     MultiFramedRTPSource::networkReadHandler1 -> break 2\n");
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
        break;
      } 
      ADVANCE(remExtSize);










    }

    // Discard any padding bytes:
    if (rtpHdr&0x20000000) {
      if (bPacket->dataSize() == 0) break;
      unsigned numPaddingBytes
	= (unsigned)(bPacket->data())[bPacket->dataSize()-1];
      if (bPacket->dataSize() < numPaddingBytes) break;
      bPacket->removePadding(numPaddingBytes);
    }

    // The rest of the packet is the usable data.  Record and save it:
    if (rtpSSRC != fLastReceivedSSRC) {
      // The SSRC of incoming packets has changed.  Unfortunately we don't yet handle streams that contain multiple SSRCs,
      // but we can handle a single-SSRC stream where the SSRC changes occasionally:
      fLastReceivedSSRC = rtpSSRC;
      fReorderingBuffer->resetHaveSeenFirstPacket();
    }
    unsigned short rtpSeqNo = (unsigned short)(rtpHdr&0xFFFF);

    long newstamp = (unsigned long)rtpTimestamp;
    long difference = newstamp - ourFirstTimestamp;

<<<<<<< HEAD
    fprintf(stderr, "Har faet ein pakke med timestamp: %lu\n FIRST TIMESTAMP ER FORTSATT: %lu\n som betyr at vi har kommet: %lu i strømmen\n\n",(unsigned long)rtpTimestamp, ourFirstTimestamp, difference);


=======
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0


    Boolean usableInJitterCalculation
      = packetIsUsableInJitterCalculation((bPacket->data()),
						  bPacket->dataSize());
    struct timeval presentationTime; // computed by:
    Boolean hasBeenSyncedUsingRTCP; // computed by:
    receptionStatsDB()
      .noteIncomingPacket(rtpSSRC, rtpSeqNo, rtpTimestamp,
			  timestampFrequency(),
			  usableInJitterCalculation, presentationTime,
			  hasBeenSyncedUsingRTCP, bPacket->dataSize());

    // Fill in the rest of the packet descriptor, and store it:
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    bPacket->assignMiscParams(rtpSeqNo, rtpTimestamp, presentationTime,
			      hasBeenSyncedUsingRTCP, rtpMarkerBit,
<<<<<<< HEAD
			      timeNow, chunkY, &fromAddress);
    if (!fReorderingBuffer->storePacket(bPacket)) break;
=======
			      timeNow, chunkRef, &fromAddress);
    if (!fReorderingBuffer->storePacket(bPacket)){
      break;
    }else{
      fprintf(stderr, "\n  HEY HOORAY THE PACKET HAS BEEN STORED\n");

    } 
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0

    readSuccess = True;
  } while (0);
  if (!readSuccess) fReorderingBuffer->freePacket(bPacket);

  doGetNextFrame2();
  // If we didn't get proper data this time, we'll get another chance
}


////////// BufferedPacket and BufferedPacketFactory implementation /////

#define MAX_PACKET_SIZE 65536

BufferedPacket::BufferedPacket()
  : fPacketSize(MAX_PACKET_SIZE),
    fBuf(new unsigned char[MAX_PACKET_SIZE]),
    fNextPacket(NULL) {
}

BufferedPacket::~BufferedPacket() {
  delete fNextPacket;
  delete[] fBuf;
}

void BufferedPacket::reset() {
  fHead = fTail = 0;
  fUseCount = 0;
  fIsFirstPacket = False; // by default
}

// The following function has been deprecated:
unsigned BufferedPacket
::nextEnclosedFrameSize(unsigned char*& /*framePtr*/, unsigned dataSize) {
  // By default, use the entire buffered data, even though it may consist
  // of more than one frame, on the assumption that the client doesn't
  // care.  (This is more efficient than delivering a frame at a time)
  return dataSize;
}

void BufferedPacket
::getNextEnclosedFrameParameters(unsigned char*& framePtr, unsigned dataSize,
				 unsigned& frameSize,
				 unsigned& frameDurationInMicroseconds) {
  // By default, use the entire buffered data, even though it may consist
  // of more than one frame, on the assumption that the client doesn't
  // care.  (This is more efficient than delivering a frame at a time)

  // For backwards-compatibility with existing uses of (the now deprecated)
  // "nextEnclosedFrameSize()", call that function to implement this one:
  frameSize = nextEnclosedFrameSize(framePtr, dataSize);

  frameDurationInMicroseconds = 0; // by default.  Subclasses should correct this.
}

Boolean BufferedPacket::fillInData(RTPInterface& rtpInterface, struct sockaddr_in& fromAddress,
				   Boolean& packetReadWasIncomplete) {

  fprintf(stderr, "\n     Boolean BufferedPacket::fillInData\n");
  if (!packetReadWasIncomplete) reset();

  unsigned const maxBytesToRead = bytesAvailable();
  if (maxBytesToRead == 0) return False; // exceeded buffer size when reading over TCP

  unsigned numBytesRead;
  int tcpSocketNum; // not used
  unsigned char tcpStreamChannelId; // not used
  if (!rtpInterface.handleRead(&fBuf[fTail], maxBytesToRead,
			       numBytesRead, fromAddress,
			       tcpSocketNum, tcpStreamChannelId,
			       packetReadWasIncomplete)) {
    return False;
  }
  fTail += numBytesRead;
  return True;
}

void BufferedPacket
::assignMiscParams(unsigned short rtpSeqNo, unsigned rtpTimestamp,
		   struct timeval presentationTime,
		   Boolean hasBeenSyncedUsingRTCP, Boolean rtpMarkerBit,
		   struct timeval timeReceived, unsigned short chunkRef, struct sockaddr_in* addr) {
  fRTPSeqNo = rtpSeqNo;
  fRTPTimestamp = rtpTimestamp;
  fPresentationTime = presentationTime;
  fHasBeenSyncedUsingRTCP = hasBeenSyncedUsingRTCP;
  fRTPMarkerBit = rtpMarkerBit;
  fTimeReceived = timeReceived;
<<<<<<< HEAD
  fChunkRef = chunkRef; 
  fAddr = addr;
=======
  fChunkRef = chunkRef;
  fAddr = addr; 
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
}

void BufferedPacket::skip(unsigned numBytes) {
  fHead += numBytes;
  if (fHead > fTail) fHead = fTail;
}

void BufferedPacket::removePadding(unsigned numBytes) {
  if (numBytes > fTail-fHead) numBytes = fTail-fHead;
  fTail -= numBytes;
}

void BufferedPacket::appendData(unsigned char* newData, unsigned numBytes) {
    fprintf(stderr, "Appending data\n" );

  if (numBytes > fPacketSize-fTail) numBytes = fPacketSize - fTail;
  memmove(&fBuf[fTail], newData, numBytes);
  fTail += numBytes;
}

void BufferedPacket::use(unsigned char* to, unsigned toSize,
			 unsigned& bytesUsed, unsigned& bytesTruncated,
			 unsigned short& rtpSeqNo, unsigned& rtpTimestamp,
			 struct timeval& presentationTime,
			 Boolean& hasBeenSyncedUsingRTCP,
			 Boolean& rtpMarkerBit) {

<<<<<<< HEAD
=======
  fprintf(stderr, "We are USING a packet\n" );
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0

  unsigned char* origFramePtr = &fBuf[fHead];
  unsigned char* newFramePtr = origFramePtr; // may change in the call below
  unsigned frameSize, frameDurationInMicroseconds;
  getNextEnclosedFrameParameters(newFramePtr, fTail - fHead,
				 frameSize, frameDurationInMicroseconds);
  if (frameSize > toSize) {
    bytesTruncated += frameSize - toSize;
    bytesUsed = toSize;
  } else {
    bytesTruncated = 0;
    bytesUsed = frameSize;
  }

<<<<<<< HEAD
  //unsigned short transfer = 156;
  //fprintf(stderr, "\n\n    right before memmove %u\n", transfer);

  //memmove(to, &transfer, 2);
=======

>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
  memmove(to, newFramePtr, bytesUsed);
  
  fHead += (newFramePtr - origFramePtr) + frameSize;
  ++fUseCount;

  rtpSeqNo = fRTPSeqNo;
  rtpTimestamp = fRTPTimestamp;
  presentationTime = fPresentationTime;
  hasBeenSyncedUsingRTCP = fHasBeenSyncedUsingRTCP;
  rtpMarkerBit = fRTPMarkerBit;

  // Update "fPresentationTime" for the next enclosed frame (if any):
  fPresentationTime.tv_usec += frameDurationInMicroseconds;
  if (fPresentationTime.tv_usec >= 1000000) {
    fPresentationTime.tv_sec += fPresentationTime.tv_usec/1000000;
    fPresentationTime.tv_usec = fPresentationTime.tv_usec%1000000;
  }
}

BufferedPacketFactory::BufferedPacketFactory() {
}

BufferedPacketFactory::~BufferedPacketFactory() {
}

BufferedPacket* BufferedPacketFactory
::createNewPacket(MultiFramedRTPSource* /*ourSource*/) {
  return new BufferedPacket;
}


////////// ReorderingPacketBuffer implementation //////////

ReorderingPacketBuffer
::ReorderingPacketBuffer(BufferedPacketFactory* packetFactory)
  : fThresholdTime(100000) /* default reordering threshold: 100 ms */,
    fHaveSeenFirstPacket(False), fHeadPacket(NULL), fTailPacket(NULL), fSavedPacket(NULL), fSavedPacketFree(True), curChunk(NULL) {
  fPacketFactory = (packetFactory == NULL)
    ? (new BufferedPacketFactory)
    : packetFactory;
}

ReorderingPacketBuffer::~ReorderingPacketBuffer() {
  reset();
  delete fPacketFactory;
}

void ReorderingPacketBuffer::reset() {
  if (fSavedPacketFree) delete fSavedPacket; // because fSavedPacket is not in the list
  delete fHeadPacket; // will also delete fSavedPacket if it's in the list
  resetHaveSeenFirstPacket();
  fHeadPacket = fTailPacket = fSavedPacket = NULL;
  curChunk = NULL;
}

FILE * ReorderingPacketBuffer::pullChunk(unsigned short numb, struct sockaddr_in* addr) {

  AddressString *adret = new AddressString(*addr);
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s\n", adret->val());

  const char *reqstart = "wget -d ";
  int len0 = strlen(reqstart);
  const char * addrstr = adret->val();
  int len1 = strlen(addrstr);
  const char *manstart = "/manifes";
  int len2 = strlen(manstart);
  const char *manend = ".ts";
  int len3 = strlen(manend);
  const char *reqend = " --header \"Host: denseserver.com\"";
  int len4 = strlen(reqend);
  int reqlen = len0 + len1 + len2 + len3 + len4;
  //char * chu = itoa(numb);

  //char * endRes = reqstart + manstart + chu + manend + reqend;
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk addr: %d \n", len1);
  char newpath[reqlen + 2];
  
  
  memcpy(newpath, reqstart, len0);
  newpath[len0] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s \n", newpath);

  memcpy(newpath + len0, addrstr, len1);
  newpath[len0 + len1] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s \n", newpath);

  
  memcpy(newpath + len0 + len1, manstart, len2);
  newpath[len0 + len1 + len2] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s \n", newpath);

  char chunkchar = '0' + numb;
  newpath[len0 + len1 + len2] = chunkchar;
  newpath[len0 + len1 + len2 + 1] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s \n", newpath);

  memcpy(newpath + len0 + len1 + len2 + 1, manend, len3);
  newpath[len0 + len1 + len2 + len3 + 1] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s \n", newpath);

  memcpy(newpath + len0 + len1 + len2 + len3 + 1, reqend, len4);
  newpath[reqlen + 1] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s \n", newpath);
  
  FILE* file = popen(newpath,"r");

  return file;

  
}



BufferedPacket* ReorderingPacketBuffer::getFreePacket(MultiFramedRTPSource* ourSource) {
  if (fSavedPacket == NULL) { // we're being called for the first time
    fSavedPacket = fPacketFactory->createNewPacket(ourSource);
    fSavedPacketFree = True;
  }

  if (fSavedPacketFree == True) {
    fSavedPacketFree = False;
    return fSavedPacket;
  } else {
    return fPacketFactory->createNewPacket(ourSource);
  }
}

Boolean ReorderingPacketBuffer::storePacket(BufferedPacket* bPacket) {

  unsigned short rtpSeqNo = bPacket->rtpSeqNo();
  unsigned short rtpChunk = bPacket->nowChunk();

  if (!fHaveSeenFirstPacket) {
    fNextExpectedSeqNo = rtpSeqNo; // initialization
    bPacket->isFirstPacket() = True;
    fHaveSeenFirstPacket = True;
  }




  // Ignore this packet if its sequence number is less than the one
  // that we're looking for (in this case, it's been excessively delayed).
  if (seqNumLT(rtpSeqNo, fNextExpectedSeqNo)){
    return False;
  } 


  if (fTailPacket == NULL) {
    // Common case: There are no packets in the queue; this will be the first one:
    bPacket->nextPacket() = NULL;
    fHeadPacket = fTailPacket = bPacket;
    return True;
  }

  if (seqNumLT(fTailPacket->rtpSeqNo(), rtpSeqNo)) {
    // The next-most common case: There are packets already in the queue; this packet arrived in order => put it at the tail:
    bPacket->nextPacket() = NULL;
    fTailPacket->nextPacket() = bPacket;
    fTailPacket = bPacket;
    return True;
  } 

  if (rtpSeqNo == fTailPacket->rtpSeqNo()) {
    // This is a duplicate packet - ignore it
    return False;
  }

  // Rare case: This packet is out-of-order.  Run through the list (from the head), to figure out where it belongs:
  BufferedPacket* beforePtr = NULL;
  BufferedPacket* afterPtr = fHeadPacket;
  while (afterPtr != NULL) {
    if (seqNumLT(rtpSeqNo, afterPtr->rtpSeqNo())) break; // it comes here
    if (rtpSeqNo == afterPtr->rtpSeqNo()) {
      // This is a duplicate packet - ignore it
      return False;
    }

    beforePtr = afterPtr;
    afterPtr = afterPtr->nextPacket();
  }

  // Link our new packet between "beforePtr" and "afterPtr":
  bPacket->nextPacket() = afterPtr;
  if (beforePtr == NULL) {
    fHeadPacket = bPacket;
  } else {
    beforePtr->nextPacket() = bPacket;
  }

  return True;
}

void ReorderingPacketBuffer::releaseUsedPacket(BufferedPacket* packet) {
  // ASSERT: packet == fHeadPacket
  // ASSERT: fNextExpectedSeqNo == packet->rtpSeqNo()
  ++fNextExpectedSeqNo; // because we're finished with this packet now

  fHeadPacket = fHeadPacket->nextPacket();
  if (!fHeadPacket) { 
    fTailPacket = NULL;
  }
  packet->nextPacket() = NULL;

  freePacket(packet);
}

BufferedPacket* ReorderingPacketBuffer
::getNextCompletedPacket(Boolean& packetLossPreceded) {
  if (fHeadPacket == NULL) return NULL;

  // Check whether the next packet we want is already at the head
  // of the queue:
  // ASSERT: fHeadPacket->rtpSeqNo() >= fNextExpectedSeqNo
  if (fHeadPacket->rtpSeqNo() == fNextExpectedSeqNo) {
    packetLossPreceded = fHeadPacket->isFirstPacket();
        // (The very first packet is treated as if there was packet loss beforehand.)
    return fHeadPacket;
  }

  // We're still waiting for our desired packet to arrive.  However, if
  // our time threshold has been exceeded, then forget it, and return
  // the head packet instead:
  Boolean timeThresholdHasBeenExceeded;
  if (fThresholdTime == 0) {
    timeThresholdHasBeenExceeded = True; // optimization
  } else {
    struct timeval timeNow;
    gettimeofday(&timeNow, NULL);
    unsigned uSecondsSinceReceived
      = (timeNow.tv_sec - fHeadPacket->timeReceived().tv_sec)*1000000
      + (timeNow.tv_usec - fHeadPacket->timeReceived().tv_usec);
    timeThresholdHasBeenExceeded = uSecondsSinceReceived > fThresholdTime;
  }
  if (timeThresholdHasBeenExceeded) {
    fNextExpectedSeqNo = fHeadPacket->rtpSeqNo();
        // we've given up on earlier packets now
    packetLossPreceded = True;
    fprintf(stderr, "\n\n    PACKET LOSS PACKET LOSS\n");
   


    return fHeadPacket;
  }

  // Otherwise, keep waiting for our desired packet to arrive:
  return NULL;
}

<<<<<<< HEAD
BufferedPacket* ReorderingPacketBuffer::getLastCompletedPacket() {
  return fTailPacket;
}
=======
BufferedPacket* ReorderingPacketBuffer
::getLastCompletedPacket() {
  if (fTailPacket == NULL){
    return NULL;
  }

  return fTailPacket;
}
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
