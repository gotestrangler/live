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
