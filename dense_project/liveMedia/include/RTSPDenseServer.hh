#ifndef _RTSP_DENSE_SERVER_HH
#define _RTSP_DENSE_SERVER_HH
#endif

#include "DigestAuthentication.hh"
#include "RTSPServer.hh"
#include "Base64.hh"
#include "PassiveServerMediaSubsession.hh"
#include "ServerMediaSession.hh"
#include "Groupsock.hh"
#include "ByteStreamFileSource.hh"
#include "H264VideoStreamFramer.hh"





///// DENSE SERVER //////
class RTSPDenseClientConnection; //Forward
class RTSPDenseClientSession; //Forward
class RTSPDenseServer: public RTSPServer {
    public:
    static RTSPDenseServer* createNew(UsageEnvironment& env, Port ourPort = 554,
                            UserAuthenticationDatabase* authDatabase = NULL,
                            unsigned reclamationSeconds = 65,
                            Boolean streamRTPOverTCP = False);
    


    protected:
    RTSPDenseServer(UsageEnvironment& env, int ourSocket, Port ourPort,
                    UserAuthenticationDatabase* authDatabase,
                    unsigned reclamationSeconds,
                    Boolean streamRTPOverTCP);
    // called only by createNew();
    virtual ~RTSPDenseServer ();
    

    friend class RTSPServer;
    friend class GenericMediaServer;


    private:
    Boolean fStreamRTPOverTCP; //Jsut so that i know where new variables should be. Remember to initialise in createNew to something
    Boolean fAllowStreamingRTPOverTCP;
    HashTable* fTCPStreamingDatabase;


    //Til oprettelsen: 

    HashTable* denseTable;

        //HashTable::create(ONE_WORD_HASH_KEYS)
        /*
        //Sockets til RTP og RTCP 
        Groupsock * rtpGroupsock;
        Groupsock * rtcpGroupsock;

        //RTP 
        RTPSink* videoSink;

        //RTCP
        RTCPInstance* rtcp;

        //Passive
        PassiveServerMediaSubsession* passiveSession;

        //Session
        ServerMediaSession* denseSession; 

        //File streamer and framer
        ByteStreamFileSource* fileSource;
        H264VideoStreamFramer* videoSource; */



    //HashTable* fServerMediaSessions;


    
    // A data structure that is used to implement "fTCPStreamingDatabase"
    // (and the "noteTCPStreamingOnSocket()" and "stopTCPStreamingOnSocket()" member functions):
    class streamingOverTCPRecord {
    public:
    streamingOverTCPRecord(u_int32_t sessionId, unsigned trackNum, streamingOverTCPRecord* next)
        : fNext(next), fSessionId(sessionId), fTrackNum(trackNum) {
    }
    virtual ~streamingOverTCPRecord() {
        delete fNext;
    }

    streamingOverTCPRecord* fNext;
    u_int32_t fSessionId;
    unsigned fTrackNum;
    };




    public: 
    class DenseSession{
        public: 
        DenseSession(Groupsock* rtpG, Groupsock* rtcpG, RTPSink* videoSink, RTCPInstance* rtcp, 
        PassiveServerMediaSubsession* passiveSession,
        ServerMediaSession* denseSession,
        ByteStreamFileSource* fileSource,
        H264VideoStreamFramer* videoSource){

        }
    
        ~DenseSession(){
            
        }

        void setRTPGSock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl);
        void setRTCPGSock(UsageEnvironment &env, in_addr destinationAddress, Port rtpPort, u_int8_t ttl);

        //Sockets til RTP og RTCP 
        Groupsock * rtpGroupsock;
        Groupsock * rtcpGroupsock;

        //RTP 
        RTPSink* videoSink;

        //RTCP
        RTCPInstance* rtcp;

        //Passive
        PassiveServerMediaSubsession* passiveSession;

        //Session
        ServerMediaSession* serversession; 

        //File streamer and framer
        ByteStreamFileSource* fileSource;
        H264VideoStreamFramer* videoSource;
    };

    DenseSession* createNewDenseSession(Groupsock* rtpG, Groupsock* rtcpG, RTPSink* videoSink, RTCPInstance* rtcp, 
        PassiveServerMediaSubsession* passiveSession,
        ServerMediaSession* denseSession,
        ByteStreamFileSource* fileSource,
        H264VideoStreamFramer* videoSource);






    


    /////CLASS DENSE CLIENT CONNECTION
    public:
    // The state of a TCP connection used by a RTSP client:
    class RTSPDenseClientSession; // forward
    class RTSPDenseClientConnection: public RTSPServer::RTSPClientConnection {

    void handleRequestBytes(int newBytesRead);

    void handleCmd_OPTIONS(char * urlSuffix);

    void handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr);

    void make(ServerMediaSession *session);
    
    protected:
        RTSPDenseClientConnection(RTSPDenseServer& ourServer, int clientSocket, struct sockaddr_in clientAddr);
        virtual ~RTSPDenseClientConnection();

        RTSPDenseServer& fOurRTSPServer;
        

        friend class RTSPDenseServer;
        friend class RTSPServer;

        RTSPDenseClientSession* ourClientSession; 


    private: 
        friend class RTSPDenseClientSession;
        int ref; 
        

    };

    protected: // redefined virtual functions
    // If you subclass "RTSPClientConnection", then you must also redefine this virtual function in order
    // to create new objects of your subclass:
    virtual RTSPClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr);
    







    /////CLASS DENSE CLIENT SESSION
    public:
    class RTSPDenseClientSession: public RTSPServer::RTSPClientSession {

    
    void handleCmd_SETUP(RTSPDenseServer::RTSPDenseClientConnection* ourClientConnection,
		  char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr);
    
    

    protected:
        RTSPDenseClientSession(RTSPDenseServer& ourServer, u_int32_t sessionId);
        virtual ~RTSPDenseClientSession();

        RTSPDenseServer& fOurRTSPServer;

        friend class RTSPDenseServer;
        friend class RTSPServer;

        RTSPDenseClientConnection* ourClientConnection; 
        Boolean newDenseRequest; 

    private:
        friend class RTSPDenseClientConnection;
        
    };

    protected:
    // If you subclass "RTSPClientSession", then you must also redefine this virtual function in order
    // to create new objects of your subclass:
    virtual RTSPClientSession* createNewClientSession(u_int32_t sessionId);

};



// A special version of "parseTransportHeader()", used just for parsing the "Transport:" header
// in an incoming "REGISTER" command:
void parseTransportHeaderForREGISTER(char const* buf, // in
				     Boolean &reuseConnection, // out
				     Boolean& deliverViaTCP, // out
				     char*& proxyURLSuffix); // out








 

