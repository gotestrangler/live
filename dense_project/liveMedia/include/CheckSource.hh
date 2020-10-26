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
// A file source that is a plain byte stream (rather than frames)
// C++ header


#include "FramedFileSource.hh"


class CheckSource: public FramedFileSource {
public:
  static CheckSource* createNew(UsageEnvironment& env,
					 char const* fileName,
					 unsigned preferredFrameSize = 0,
					 unsigned playTimePerFrame = 0);
  // "preferredFrameSize" == 0 means 'no preference'
  // "playTimePerFrame" is in microseconds

  static CheckSource* createNew(UsageEnvironment& env,
					 FILE* fid,
					 unsigned preferredFrameSize = 0,
					 unsigned playTimePerFrame = 0);
      // an alternative version of "createNew()" that's used if you already have
      // an open file.

  u_int64_t fileSize() const { return fFileSize; }
      // 0 means zero-length, unbounded, or unknown

  void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0);
    // if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
  void seekToByteRelative(int64_t offset, u_int64_t numBytesToStream = 0);
  void seekToEnd(); // to force EOF handling on the next read

  int getNowChunk(){return curChunk;}

protected:
  CheckSource(UsageEnvironment& env,
		       FILE* fid,
		       unsigned preferredFrameSize,
		       unsigned playTimePerFrame);
	// called only by createNew()

  virtual ~CheckSource();

  void stripPath(char const* fileName);
  void stripChunks();
 

  static void fileReadableHandler(CheckSource* source, int mask);
  void doReadFromFile();

private:
  // redefined virtual functions:
  int manageManifest();
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();


protected:
  u_int64_t fFileSize;
  u_int64_t fReadSoFar;
  char fChunks[1000][100];
  int curChunk; 
  char * fManifest; 
  char fPath[100];
  int numbChunks;
  //lagre peker til forrige fil, så du kan close den 

private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  Boolean fFidIsSeekable;
  unsigned fLastPlayTime;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
};

