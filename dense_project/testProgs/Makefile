LIVE_DIR = ../../live/
INCLUDES = -I$(LIVE_DIR)UsageEnvironment/include -I$(LIVE_DIR)groupsock/include -I$(LIVE_DIR)liveMedia/include -I$(LIVE_DIR)BasicUsageEnvironment/include -I.
LIBS = $(LIVE_DIR)liveMedia/libliveMedia.a $(LIVE_DIR)groupsock/libgroupsock.a $(LIVE_DIR)BasicUsageEnvironment/libBasicUsageEnvironment.a $(LIVE_DIR)UsageEnvironment/libUsageEnvironment.a

TEST_H264_VIDEO_STREAMER = testH264VideoStreamer
TEST_H264_VIDEO_STREAMER_ORIG = testH264VideoStreamerOrig
TEST_MP4_VIDEO_STREAMER = testMP4VideoStreamer
TRANSPORT_STREAMER = transportStreamer
CLIENT = client
CLIENT2 = client2
SERVER = ../liveMedia/RTSPDenseServer
MANIFEST = ../liveMedia/ByteStreamManifestSource
CHECK = ../liveMedia/CheckSource

trans:
	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./$(TRANSPORT_STREAMER).cpp
	c++ -g -o $(TRANSPORT_STREAMER) -L. $(SERVER).o $(MANIFEST).o $(CHECK).o $(TRANSPORT_STREAMER).o $(LIBS)

all:

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./$(TRANSPORT_STREAMER).cpp
	c++ -g -o $(TRANSPORT_STREAMER) -L. $(SERVER).o $(MANIFEST).o $(CHECK).o $(TRANSPORT_STREAMER).o $(LIBS)

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./$(TEST_H264_VIDEO_STREAMER).cpp
	c++ -g -o $(TEST_H264_VIDEO_STREAMER) -L. $(SERVER).o $(MANIFEST).o $(CHECK).o $(TEST_H264_VIDEO_STREAMER).o $(LIBS)

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./$(TEST_MP4_VIDEO_STREAMER).cpp
	c++ -g -o $(TEST_MP4_VIDEO_STREAMER) -L. $(SERVER).o $(MANIFEST).o $(CHECK).o $(TEST_MP4_VIDEO_STREAMER).o $(LIBS)

	c++ -c -g $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./$(TEST_H264_VIDEO_STREAMER_ORIG).cpp
	c++ -g -o $(TEST_H264_VIDEO_STREAMER_ORIG) -L. $(SERVER).o $(MANIFEST).o $(CHECK).o $(TEST_H264_VIDEO_STREAMER_ORIG).o $(LIBS)

	c++ -c $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./client.cpp
	c++ -o client -L. $(CLIENT).o $(LIBS)

	c++ -c $(INCLUDES)  -DBSD=1 -O -DSOCKLEN_T=socklen_t -DHAVE_SOCKADDR_LEN=1 -DTIME_BASE=int -Wall ./client2.cpp
	c++ -o client2 -L. $(CLIENT2).o $(LIBS)

clean:
	rm -f $(TEST_H264_VIDEO_STREAMER)
	rm -f $(TEST_H264_VIDEO_STREAMER_ORIG)
	rm -f $(TEST_MP4_VIDEO_STREAMER)
	rm -f $(TRANSPORT_STREAMER)
	rm -f client
	rm -f client2
	rm -f *.o