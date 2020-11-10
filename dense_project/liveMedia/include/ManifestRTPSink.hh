<<<<<<< HEAD
#include "MultiFramedRTPSink.hh"
#include "CheckSource.hh"


class ManifestRTPSink: public MultiFramedRTPSink {
public:
  static ManifestRTPSink*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat,
	    unsigned rtpTimestampFrequency,
	    char const* sdpMediaTypeString,
	    char const* rtpPayloadFormatName,
	    unsigned numChannels = 1,
	    Boolean allowMultipleFramesPerPacket = True,
	    Boolean doNormalMBitRule = True, u_int32_t firstTimestamp = 0);

  // "doNormalMBitRule" means: If the medium (i.e., "sdpMediaTypeString") is other than "audio", set the RTP "M" bit
  // on each outgoing packet iff it contains the last (or only) fragment of a frame.
  // Otherwise (i.e., if "doNormalMBitRule" is False, or the medium is "audio"), leave the "M" bit unset.

  void setMBitOnNextPacket() { fSetMBitOnNextPacket = True; } // hack for optionally setting the RTP 'M' bit from outside the class

  u_int32_t firstTimestamp;

  void setCheckSource(CheckSource* ns);

  friend class CheckSource;

protected:
  ManifestRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		unsigned char rtpPayloadFormat,
		unsigned rtpTimestampFrequency,
		char const* sdpMediaTypeString,
		char const* rtpPayloadFormatName,
		unsigned numChannels,
		Boolean allowMultipleFramesPerPacket,
		Boolean doNormalMBitRule, u_int32_t firstTimestamp);
	// called only by createNew()

  virtual ~ManifestRTPSink();



protected: // redefined virtual functions
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);


  protected: // redefined virtual functions:
  virtual Boolean continuePlaying();
  void buildAndSendPacket(Boolean isFirstPacket);
  static void sendNext(void* firstArg);
  friend void sendNext(void*);
  void packFrame();
  void sendPacketIfNecessary();


  static void afterGettingFrame(void* clientData,
				unsigned numBytesRead, unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned numBytesRead, unsigned numTruncatedBytes,
			  struct timeval presentationTime,
			  unsigned durationInMicroseconds);

  

private:
  char const* fSDPMediaTypeString;
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnLastFrames, fSetMBitOnNextPacket;
  CheckSource* checkSource;


};
=======
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
// A simple RTP sink that packs frames into each outgoing
//     packet, without any fragmentation or special headers.
// C++ header


#include "MultiFramedRTPSink.hh"
#include "CheckSource.hh"


class ManifestRTPSink: public MultiFramedRTPSink {
public:
  static ManifestRTPSink*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat,
	    unsigned rtpTimestampFrequency,
	    char const* sdpMediaTypeString,
	    char const* rtpPayloadFormatName,
	    unsigned numChannels = 1,
	    Boolean allowMultipleFramesPerPacket = True,
	    Boolean doNormalMBitRule = True, u_int32_t firstTimestamp = 0);

  // "doNormalMBitRule" means: If the medium (i.e., "sdpMediaTypeString") is other than "audio", set the RTP "M" bit
  // on each outgoing packet iff it contains the last (or only) fragment of a frame.
  // Otherwise (i.e., if "doNormalMBitRule" is False, or the medium is "audio"), leave the "M" bit unset.

  void setMBitOnNextPacket() { fSetMBitOnNextPacket = True; } // hack for optionally setting the RTP 'M' bit from outside the class

  u_int32_t firstTimestamp;

  void setCheckSource(CheckSource* ns);

  friend class CheckSource;

protected:
  ManifestRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		unsigned char rtpPayloadFormat,
		unsigned rtpTimestampFrequency,
		char const* sdpMediaTypeString,
		char const* rtpPayloadFormatName,
		unsigned numChannels,
		Boolean allowMultipleFramesPerPacket,
		Boolean doNormalMBitRule, u_int32_t firstTimestamp);
	// called only by createNew()

  virtual ~ManifestRTPSink();



protected: // redefined virtual functions
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval framePresentationTime,
                                      unsigned numRemainingBytes);
  virtual
  Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
					 unsigned numBytesInFrame) const;
  virtual char const* sdpMediaType() const;
  virtual unsigned frameSpecificHeaderSize() const;

  protected: // redefined virtual functions:
  virtual Boolean continuePlaying();
  void buildAndSendPacket(Boolean isFirstPacket);
  static void sendNext(void* firstArg);
  friend void sendNext(void*);
  void packFrame();
  void sendPacketIfNecessary();
  void setXBit();


  static void afterGettingFrame(void* clientData,
				unsigned numBytesRead, unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned numBytesRead, unsigned numTruncatedBytes,
			  struct timeval presentationTime,
			  unsigned durationInMicroseconds);

  

private:
  char const* fSDPMediaTypeString;
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnLastFrames, fSetMBitOnNextPacket;
  CheckSource* checkSource;


};
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
