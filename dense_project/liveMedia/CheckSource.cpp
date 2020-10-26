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
// Implementation

#include "include/CheckSource.hh"
#include "InputFile.hh"
#include "GroupsockHelper.hh"
#include <inttypes.h>


////////// CheckSource //////////

CheckSource*
CheckSource::createNew(UsageEnvironment& env, char const* fileName,
				unsigned preferredFrameSize,
				unsigned playTimePerFrame) {
  FILE* fid = OpenInputFile(env, fileName);
  if (fid == NULL) return NULL;

  CheckSource* newSource
    = new CheckSource(env, fid, preferredFrameSize, playTimePerFrame);
  newSource->fFileSize = GetFileSize(fileName, fid);
  newSource->fReadSoFar = 0; 
  newSource->curChunk = 0; 

    newSource->stripChunks();
    newSource->stripPath(fileName);

  int ein = strlen(newSource->fPath);
  int twei = strlen(newSource->fChunks[0]);
  char newpath[ein + twei + 1];
  memcpy(newpath, newSource->fPath, ein);
  memcpy(newpath + ein, newSource->fChunks[0], twei);
  newpath[ein + twei - 1] = '\0';

  fprintf(stderr, "In the creator i am changing the filename to: %s\n", newpath);
  fprintf(stderr, "In the creator check check: %s, %s\n", newSource->fPath, newSource->fChunks[0]);

  FILE* newFid = OpenInputFile(env, newpath);
 
  if (newFid == NULL) return NULL;
  newSource->fFid = newFid; 
  newSource->fFileSize = GetFileSize(newpath, newFid);

  fprintf(stderr, "In the creator the filesize for the first chunk: %d\n", newSource->fFileSize);

  return newSource;
}



void CheckSource::stripPath(char const* fileName ){


  int len = strlen(fileName);
  int i = 0; 
  for(i = len; i > 0; i--){
    if (fileName[i] == '/'){
      break; 
    }
  }

  memcpy(fPath , &fileName[0], i + 1 );
  fPath[i + 1] = '\0';


  fprintf(stderr, "ByteStreamManifestSource::stripPath -> %s\n", fPath);
}

void CheckSource::stripChunks(){
  
    fprintf(stderr, "ByteStreamManifestSource::stripChunks()\n");

    int sum = 0;
    int ant = 0;
    size_t x = 0;
    char* line = NULL; 

    getline(&line, &x, fFid);
    fprintf(stderr, "ByteStreamManifestSource::stripChunks() -> line is %s, line[0] is %c\n", line, line[0]);

    if(line[0] != '#'){
      fprintf(stderr, "ByteStreamManifestSource::stripChunks() -> THIS IS NOT CORRECT MANIFEST FORMAT STARTING WITH # -> line is %s\n", line);
      exit(0);
    }
   
    
    while(getline(&line, &x, fFid) > 0){
      fprintf(stderr, "ByteStreamManifestSource::stripChunks() %s\n", line);

  
      if(line[0] != '#'){
        fprintf(stderr, "ByteStreamManifestSource::stripChunks() -> addet chunk: %d string %s\n", sum, line);

        memcpy(fChunks[sum], line, strlen(line) + 1);


        sum++; 
      }else{
        fprintf(stderr, "ByteStreamManifestSource::stripChunks() -> hoppet over: %s nummer %d\n", line, ant);
      }
      ant++; 
    }
    
    numbChunks = sum;

}

void CheckSource::seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream) {
  SeekFile64(fFid, (int64_t)byteNumber, SEEK_SET);

  fNumBytesToStream = numBytesToStream;
  fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void CheckSource::seekToByteRelative(int64_t offset, u_int64_t numBytesToStream) {
  SeekFile64(fFid, offset, SEEK_CUR);

  fNumBytesToStream = numBytesToStream;
  fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void CheckSource::seekToEnd() {
  SeekFile64(fFid, 0, SEEK_END);
}

CheckSource::CheckSource(UsageEnvironment& env, FILE* fid,
					   unsigned preferredFrameSize,
					   unsigned playTimePerFrame)
  : FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
    fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
    fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0) {
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  makeSocketNonBlocking(fileno(fFid));
#endif

  // Test whether the file is seekable
  fFidIsSeekable = FileIsSeekable(fFid);
}

CheckSource::~CheckSource() {
  if (fFid == NULL) return;

#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
#endif

  CloseInputFile(fFid);
}

void CheckSource::doGetNextFrame() {

  fprintf(stderr, "CheckSource::doGetNextFrame()\n");

  if (feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
    handleClosure();
    return;
  }

#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  doReadFromFile();
#else
  if (!fHaveStartedReading) {
    // Await readable data from the file:
    envir().taskScheduler().turnOnBackgroundReadHandling(fileno(fFid),
	       (TaskScheduler::BackgroundHandlerProc*)&fileReadableHandler, this);
    fHaveStartedReading = True;
  }
#endif
}

void CheckSource::doStopGettingFrames() {
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
  envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
  fHaveStartedReading = False;
#endif
}

void CheckSource::fileReadableHandler(CheckSource* source, int /*mask*/) {
  

  if (!source->isCurrentlyAwaitingData()) {
    source->doStopGettingFrames(); // we're not ready for the data yet
    return;
  }
  source->doReadFromFile();
}

int CheckSource::manageManifest(){
    fprintf(stderr, "CheckSource::manageManifest() - det er %"PRIu64", %"PRIu64", %"PRIu64"\n", fFileSize, fReadSoFar, fFileSize - fReadSoFar);
    
    if( (fFileSize - fReadSoFar) == 0 && curChunk <= numbChunks){
        curChunk++; 
        fprintf(stderr, "     KANON! STOP! %s\n", fPath);
 
        int ein = strlen(fPath);
        int twei = strlen(fChunks[curChunk]);
        char newpath[ein + twei + 1];
        memcpy(newpath, fPath, ein);
        memcpy(newpath + ein, fChunks[curChunk], twei);
        newpath[ein + twei - 1] = '\0';

        fprintf(stderr, "In the CheckSource::manageManifest() i am changing the filename to: %s\n", newpath);
        fprintf(stderr, "In the CheckSource::manageManifest(): %s, %s\n", newpath, fChunks[curChunk]);
        //memcpy(fPath , &newpath[0], ein + twei); 

        FILE* newFid = OpenInputFile(envir(), newpath);
        fprintf(stderr, "In the CheckSource::manageManifest(): MID MID\n");

        if (newFid == NULL){
            fprintf(stderr, "In the CheckSource::manageManifest(): UH OHHHHH \n");

        } 
        fFid = newFid; 
        fFileSize = GetFileSize(newpath, newFid);

    
        fprintf(stderr, "In the CheckSource::manageManifest(): har opnet en ny fil %s med storrelse: %d\n", newpath, fFileSize);
        
        fReadSoFar = 0; 

    
        fprintf(stderr, "In the CheckSource::manageManifest(): END END\n");

    }
    
    return 1;
}

void CheckSource::doReadFromFile() {
  fprintf(stderr, "CheckSource::doReadFromFile()\n");
  u_int32_t firstTimestamp;
  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
    fMaxSize = (unsigned)fNumBytesToStream;
  }
  if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }



    fFrameSize = fread(fTo, 1, fMaxSize, fFid);
    fReadSoFar += fFrameSize;
    int readSize = manageManifest();
    


  if (fFrameSize == 0) {
    handleClosure();
    return;
  }
  fNumBytesToStream -= fFrameSize;


  // Set the 'presentation time':
  if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
    fprintf(stderr, "BOOm %lu\n", fPlayTimePerFrame);


    if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      // This is the first frame, so use the current time:
      gettimeofday(&fPresentationTime, NULL);


    } else {


      // Increment by the play time of the previous data:
      unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
      fPresentationTime.tv_sec += uSeconds/1000000;
      fPresentationTime.tv_usec = uSeconds%1000000;

    }

    // Remember the play time of this data:
    fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
    fDurationInMicroseconds = fLastPlayTime;


  } else {
    // We don't know a specific play time duration for this data,
    // so just record the current time as being the 'presentation time':
    gettimeofday(&fPresentationTime, NULL);
  
  }

  // Inform the reader that he has data:
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
  // To avoid possible infinite recursion, we need to return to the event loop to do this:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				(TaskFunc*)FramedSource::afterGetting, this);
#else
  // Because the file read was done from the event loop, we can call the
  // 'after getting' function directly, without risk of infinite recursion:
  FramedSource::afterGetting(this);
#endif
}
