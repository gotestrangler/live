LIVE_DIR = ../../live/
OWN = /include
INCLUDES = -I$(OWN) -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a


all:

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./CheckSource.cpp

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./ManifestRTPSink.cpp

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./ManifestSink.cpp

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./RTSPDenseServer.cpp

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./ManifestRTPSink.cpp

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./ByteStreamManifestSource.cpp

	

clean:
	rm -f *.o
