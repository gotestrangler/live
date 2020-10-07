

#include "ByteStreamFileSource.hh"
#include "DigestAuthentication.hh"
#include "RTSPServer.hh"
#include "Base64.hh"
#include "PassiveServerMediaSubsession.hh"
#include "ServerMediaSession.hh"
#include "Groupsock.hh"
#include "H264VideoStreamFramer.hh"
#include "MPEG4VideoStreamFramer.hh"
#include "MPEG2TransportStreamFramer.hh"
#include "SimpleRTPSink.hh"
#include <string>



class ByteStreamManifestSource: public ByteStreamFileSource {
public:
  static ByteStreamManifestSource* create(UsageEnvironment& env,
					 char const* fileName,
					 unsigned preferredFrameSize = 0,
					 unsigned playTimePerFrame = 0);
  // "preferredFrameSize" == 0 means 'no preference'
  // "playTimePerFrame" is in microseconds


  char* getPath() { return fPath; };


protected:
  ByteStreamManifestSource(UsageEnvironment& env,
		       FILE* fid,
		       unsigned preferredFrameSize,
		       unsigned playTimePerFrame);
	// called only by createNew()

  virtual ~ByteStreamManifestSource();

  void stripPath(char const* fileName);
  void stripChunks();
  void doReadFromFile();

  static void fileReadableHandler(ByteStreamManifestSource* source, int mask);
  
  private:
  // redefined virtual functions:
  void doGetNextFrame();
  virtual void doStopGettingFrames();


private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  Boolean fFidIsSeekable;
  unsigned fLastPlayTime;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream;


protected:
  u_int64_t fFileSize;
  char fChunks[1000][100];
  char * fManifest; 
  char * fPath;
  int numbChunks;




};