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
// File sinks
// Implementation

#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
#include <io.h>
#include <fcntl.h>
#endif
#include "FileSink.hh"
#include "GroupsockHelper.hh"
#include "OutputFile.hh"
#include "MultiFramedRTPSource.hh"

////////// FileSink //////////

FileSink::FileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
		   char const* perFrameFileNamePrefix)
  : MediaSink(env), fOutFid(fid), fBufferSize(bufferSize), fSamePresentationTimeCounter(0) {
  fBuffer = new unsigned char[bufferSize];
  if (perFrameFileNamePrefix != NULL) {
    fPerFrameFileNamePrefix = strDup(perFrameFileNamePrefix);
    fPerFrameFileNameBuffer = new char[strlen(perFrameFileNamePrefix) + 100];
  } else {
    fPerFrameFileNamePrefix = NULL;
    fPerFrameFileNameBuffer = NULL;
  }
  fPrevPresentationTime.tv_sec = ~0; fPrevPresentationTime.tv_usec = 0;
}

FileSink::~FileSink() {
  delete[] fPerFrameFileNameBuffer;
  delete[] fPerFrameFileNamePrefix;
  delete[] fBuffer;
  if (fOutFid != NULL) fclose(fOutFid);
}

FileSink* FileSink::createNew(UsageEnvironment& env, char const* fileName,
			      unsigned bufferSize, Boolean oneFilePerFrame) {
<<<<<<< HEAD
              
=======
    
  fprintf(stderr, "\n     FileSink::createNew\n");

>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
  do {
    FILE* fid;
    char const* perFrameFileNamePrefix;
    if (oneFilePerFrame) {
      // Create the fid for each frame
      fid = NULL;
      perFrameFileNamePrefix = fileName;
    } else {
      // Normal case: create the fid once
      fid = OpenOutputFile(env, fileName);
      if (fid == NULL) break;
      perFrameFileNamePrefix = NULL;
    }

    
    FileSink* ret = new FileSink(env, fid, bufferSize, perFrameFileNamePrefix);
    ret->fWritten = 0;
    ret->curChunk = -1;
    ret->lastOffset = 0;
    ret->antLoss = 0;
    ret->writeFromPacket = true; 
    return ret;
  } while (0);

  return NULL;
}

Boolean FileSink::continuePlaying() {
  fprintf(stderr, "\n       FileSink::continuePlaying() - buffersize: %u\n", fBufferSize);
  if (fSource == NULL) return False;

  fSource->getNextFrame(fBuffer, fBufferSize,
			afterGettingFrame, this,
			onSourceClosure, this);

  return True;
}

void FileSink::afterGettingFrame(void* clientData, unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime,
				 unsigned /*durationInMicroseconds*/) {
  FileSink* sink = (FileSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime);
}

void FileSink::addData(unsigned char const* data, unsigned dataSize,
		       struct timeval presentationTime) {
  if (fPerFrameFileNameBuffer != NULL && fOutFid == NULL) {
    // Special case: Open a new file on-the-fly for this frame
    if (presentationTime.tv_usec == fPrevPresentationTime.tv_usec &&
	presentationTime.tv_sec == fPrevPresentationTime.tv_sec) {
      // The presentation time is unchanged from the previous frame, so we add a 'counter'
      // suffix to the file name, to distinguish them:
      sprintf(fPerFrameFileNameBuffer, "%s-%lu.%06lu-%u", fPerFrameFileNamePrefix,
	      presentationTime.tv_sec, presentationTime.tv_usec, ++fSamePresentationTimeCounter);
    } else {
      sprintf(fPerFrameFileNameBuffer, "%s-%lu.%06lu", fPerFrameFileNamePrefix,
	      presentationTime.tv_sec, presentationTime.tv_usec);
      fPrevPresentationTime = presentationTime; // for next time
      fSamePresentationTimeCounter = 0; // for next time
    }
    fOutFid = OpenOutputFile(envir(), fPerFrameFileNameBuffer);
  }

  fprintf(stderr, "\n     FileSink::addData - this is where fwrite is called\n");


  // Write to our file:
#ifdef TEST_LOSS
  static unsigned const framesPerPacket = 10;
  static unsigned const frameCount = 0;
  static Boolean const packetIsLost;
  if ((frameCount++)%framesPerPacket == 0) {
    packetIsLost = (our_random()%10 == 0); // simulate 10% packet loss #####
  }

  if (!packetIsLost)
#endif

  FramedSource* chunkSet = source();
  unsigned short throww = chunkSet->getCurChunk();
  Boolean loss = chunkSet->getPacketLossNotice();
  

  
  //pullAll();
  

  if(loss && fWritten != 0 && writeFromPacket){
    pullPatch();

  }
  
  if(throww != curChunk){
    lastOffset = fWritten; 
    curChunk = throww;
    writeFromPacket = true;
  }

  fprintf(stderr, "\n %u %u %s\n", fOutFid, data, writeFromPacket ? "true" : "false");
  if (fOutFid != NULL && data != NULL && writeFromPacket) {

    fWritten += fwrite(data, 1, dataSize, fOutFid);
    fprintf(stderr, "\naddData after fWritten the chunk here is: %u\n\n the fWritten is: %u\n\n last offset: %u ant loss: %u\n", throww, fWritten, lastOffset, antLoss);

    


  }
}

void FileSink::pullPatch(){
  FILE* somebody;
  long fileSize; 
  FramedSource* chunkSet = source();
  unsigned short throww = chunkSet->getCurChunk();
  Boolean loss = chunkSet->getPacketLossNotice();
  struct sockaddr_in* piece = chunkSet->getAddr();

  fprintf(stderr, "\naddData PACKET LOSS PRECEEDED THIS fWritten: %u\n", fWritten);
  fileSize = pullChunk(throww, piece); 
  antLoss++; 
  chunkSet->removePacketLossNotice();
  writeFromPacket = false; 
  fWritten = lastOffset + fileSize; 

}

void FileSink::pullBeginning(int ant){
  long fileSize; 
  FramedSource* chunkSet = source();
  struct sockaddr_in* piece = chunkSet->getAddr();

  for(int i = 0; i < ant; i++){
    fprintf(stderr, "\naddData PACKET LOSS PRECEEDED THIS fWritten: %u, lastOffset: %u\n", fWritten, lastOffset);
    fileSize = pullChunk(i, piece); 
    chunkSet->removePacketLossNotice();
    writeFromPacket = false; 
    fWritten = fileSize;
    curChunk = i; 
  }

}

void FileSink::pullAll(){

  
  long somebody;
  unsigned long fileSize; 
  FramedSource* chunkSet = source();
  size_t written; 
  //unsigned short throww = chunkSet->getCurChunk();
  //Boolean loss = chunkSet->getPacketLossNotice();
  struct sockaddr_in* piece = chunkSet->getAddr();
  unsigned long total = 0;
  for(int i = 0; i < 10; i++){

    somebody = pullChunk(i, piece); 
    /*
    fseek(somebody, 0, SEEK_END);
    fileSize = ftell(somebody);
    fseek(somebody, 0, SEEK_SET);
    written += fwrite(somebody, 1, fileSize, fOutFid);
    fprintf(stderr, "\npullAll i: %lu total: %lu fileSize: %lu ftell(fOutFid): %lu written: %zu\n", i, total, fileSize, ftell(fOutFid), written);
    total += written;
    fclose(somebody);
    fileSize = 0; */
  }

  fprintf(stderr, "\noutside\n");

  fclose(fOutFid);
  exit(0);
}

void FileSink::afterGettingFrame(unsigned frameSize,
				 unsigned numTruncatedBytes,
				 struct timeval presentationTime) {


<<<<<<< HEAD
    FramedSource* chunkSet = source();
    //MultiFramedRTPSource* casted = dynamic_cast<MultiFramedRTPSource*>(chunkSet);

    fprintf(stderr, "\n       HEY HEY HEY HEY afterGettingFrame the chunk here is: %u\n", chunkSet->getCurChunk());

=======
  fprintf(stderr, "\n       FileSink::afterGettingFrame - buffersize: %u\n", fBufferSize);

  
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0
  if (numTruncatedBytes > 0) {
    envir() << "FileSink::afterGettingFrame(): The input frame data was too large for our buffer size ("
	    << fBufferSize << ").  "
            << numTruncatedBytes << " bytes of trailing data was dropped!  Correct this by increasing the \"bufferSize\" parameter in the \"createNew()\" call to at least "
            << fBufferSize + numTruncatedBytes << "\n";
  }


  addData(fBuffer, frameSize, presentationTime);

  if (fOutFid == NULL || fflush(fOutFid) == EOF) {
    fprintf(stderr, "\n       filesink closing 1\n");

    // The output file has closed.  Handle this the same way as if the input source had closed:
    if (fSource != NULL) fSource->stopGettingFrames();
        fprintf(stderr, "\n       filesink closing 2\n");

    onSourceClosure();
    return;
  }

  if (fPerFrameFileNameBuffer != NULL) {
    if (fOutFid != NULL) { fclose(fOutFid); fOutFid = NULL; }
  }

  // Then try getting the next frame:
    fprintf(stderr, "\n     FileSink::addData - continuePlaying() TTTTTTTTTTTTTTTTTTTTTTTTTTTTT\n");

  continuePlaying();
}

long FileSink::pullChunk(unsigned short numb, struct sockaddr_in* addr) {

  AddressString *adret = new AddressString(*addr);
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s\n", adret->val());

  const char *reqstart = "wget ";
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
  char fileName[len2 + len3 + 1];
  
  
  memcpy(newpath, reqstart, len0);
  newpath[len0] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s og %s\n", newpath, fileName);

  memcpy(newpath + len0, addrstr, len1);
  newpath[len0 + len1] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s og %s\n", newpath, fileName);

  
  memcpy(newpath + len0 + len1, manstart, len2);
  memcpy(fileName, manstart + 1, len2 - 1);
  fileName[len2] = '\0';
  newpath[len0 + len1 + len2] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s og %s\n", newpath, fileName);

  char chunkchar = '0' + numb;
  newpath[len0 + len1 + len2] = chunkchar;
  newpath[len0 + len1 + len2 + 1] = '\0';
  fileName[len2 - 1] = chunkchar;
  fileName[len2] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s og %s\n", newpath, fileName);

  memcpy(newpath + len0 + len1 + len2 + 1, manend, len3);
  newpath[len0 + len1 + len2 + len3 + 1] = '\0';
  memcpy(fileName + len2, manend, len3);
  fileName[len2 + len3] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s og %s\n", newpath, fileName);

  memcpy(newpath + len0 + len1 + len2 + len3 + 1, reqend, len4);
  newpath[reqlen + 1] = '\0';
  fprintf(stderr, "\n\n    ReorderingPacketBuffer::pullChunk %s og %s\n", newpath, fileName);
  
  system(newpath);

  FILE* file = fopen(fileName, "rb");
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fprintf(stderr, "\nFILESIZE: %ld foutfid: %d\n", fileSize, ftell(fOutFid));
  fseek(file, 0, SEEK_SET);

  char * buffer = new char [fileSize];

  fread(buffer, 1, fileSize, file);
  fseek(fOutFid, 0, lastOffset);
  long written = fwrite(buffer, 1, fileSize, fOutFid);
  fprintf(stderr, "\nwritten: %zu foutfid: %d\n", written, ftell(fOutFid));
  
  fclose(file);


  delete[] buffer;
  return written;

  
}
