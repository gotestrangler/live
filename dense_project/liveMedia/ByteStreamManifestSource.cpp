

#include "include/ByteStreamManifestSource.hh"
#include "InputFile.hh"

////////// ByteStreamManifestSource //////////

ByteStreamManifestSource*
ByteStreamManifestSource::create(UsageEnvironment& env, char const* fileName,
				unsigned preferredFrameSize,
				unsigned playTimePerFrame) {
  FILE* fid = OpenInputFile(env, fileName);
  if (fid == NULL) return NULL;

  ByteStreamManifestSource* newSource
    = new ByteStreamManifestSource(env, fid, preferredFrameSize, playTimePerFrame);
  
  

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

ByteStreamManifestSource::ByteStreamManifestSource(UsageEnvironment& env, FILE* fid,
					   unsigned preferredFrameSize,
					   unsigned playTimePerFrame)
  : ByteStreamFileSource(env, fid, preferredFrameSize, playTimePerFrame) {

}

ByteStreamManifestSource::~ByteStreamManifestSource() {
  fprintf(stderr, "ByteStreamManifestSource::cloooooooooooooooooooooooooosing\n");
  delete[] fChunks;
}

void ByteStreamManifestSource::stripPath(char const* fileName ){


  int len = strlen(fileName);
  int i = 0; 
  for(i = len; i > 0; i--){
    if (fileName[i] == '/'){
      break; 
    }
  }

  char subbuff[i + 1];
  memcpy(subbuff, &fileName[0], i + 1 );
  subbuff[i + 1] = '\0';

  fprintf(stderr, "ByteStreamManifestSource::stripPath -> %s\n", subbuff);

  fPath = subbuff; 

  fprintf(stderr, "ByteStreamManifestSource::stripPath -> %s\n", fPath);
}

void ByteStreamManifestSource::stripChunks(){
  
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

void ByteStreamManifestSource::doGetNextFrame() {
  
      fprintf(stderr, "ByteStreamManifestSource::doGetNextFrame()\n");

  if (feof(fFid) || ferror(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
    handleClosure();
    return;
  }

    fprintf(stderr, "ByteStreamManifestSource::doGetNextFrame()\n");

  doReadFromFile();

}

void ByteStreamManifestSource::doStopGettingFrames() {
        fprintf(stderr, "ByteStreamManifestSource::doStopGettingFrames()\n");

  envir().taskScheduler().unscheduleDelayedTask(nextTask());

}

void ByteStreamManifestSource::fileReadableHandler(ByteStreamManifestSource* source, int /*mask*/) {
  

  if (!source->isCurrentlyAwaitingData()) {
    source->doStopGettingFrames(); // we're not ready for the data yet
    return;
  }
  source->doReadFromFile();
}

void ByteStreamManifestSource::doReadFromFile() {
  
  u_int32_t firstTimestamp;
  // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
  if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
    fMaxSize = (unsigned)fNumBytesToStream;
  }
  if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
    fMaxSize = fPreferredFrameSize;
  }
  
  char * newfilepath = strcat(fPath, fChunks[0]);

  fprintf(stderr, "ByteStreamManifestSource::doReadFromFile() %s\n", newfilepath);

  FILE* fid = OpenInputFile(envir(), newfilepath);
  GetFileSize(newfilepath, fid);

  fFrameSize = fread(fTo, 1, fMaxSize, fFid);
  
  exit(0);


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

      //fprintf(stderr, "ByteStreamFileSource::doReadFromFile() - presentation time> %lu %lu\n", fPresentationTime.tv_sec, fPresentationTime.tv_usec);

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


