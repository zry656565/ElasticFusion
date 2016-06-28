#ifndef SOCKET_LOG_READER_H
#define SOCKET_LOG_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>

#include <Utils/Parse.h>

#include "LogReader.h"
#include "OpenNI2Interface.h"

class SocketLogReader : public LogReader
{
public:
    SocketLogReader(std::string file, bool flipColors, const char *hostname, int port);

    virtual ~SocketLogReader();

    void getNext();

    int getNumFrames();

    bool hasMore();

    bool rewound()
    {
        return false;
    }

    void rewind()
    {

    }

    void getBack()
    {

    }

    void fastForward(int frame)
    {

    }

    const std::string getFile();

    void setAuto(bool value)
    {

    }

private:
    int64_t lastFrameTime;
    int lastGot;
    const char *hostname;
    int port;
    int sockfd;
};

#endif /* SOCKET_LOG_READER_H */
