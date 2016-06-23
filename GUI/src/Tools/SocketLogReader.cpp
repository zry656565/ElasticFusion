#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "SocketLogReader.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

SocketLogReader::SocketLogReader(std::string file, bool flipColors, const char *hostname, int port)
        : LogReader(file, flipColors),
          lastFrameTime(-1),
          lastGot(-1),
          hostname(hostname),
          port(port)
{
    decompressionBufferDepth = new Bytef[Resolution::getInstance().numPixels() * 2];
    decompressionBufferImage = new Bytef[Resolution::getInstance().numPixels() * 3];

    imageSize = Resolution::getInstance().numPixels() * 3;
    depthSize = Resolution::getInstance().numPixels() * 2;

    imageReadBuffer = 0;
    depthReadBuffer = 0;

    std::cout << "Socket connecting... " << std::endl;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    // initialize socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    std::cout << "Host found." << std::endl;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    std::cout << "Connect succeeded." << std::endl;
}

SocketLogReader::~SocketLogReader()
{
    delete [] decompressionBufferDepth;
    delete [] decompressionBufferImage;
    close(sockfd);
}

void SocketLogReader::getNext()
{
    int data_len = Resolution::getInstance().numPixels() * 5;
    char data[data_len];
    int buffer_len = 50000;
    char buffer[buffer_len];

    int len = 0;
    int n = 0;
    bzero(data, sizeof(data));
    bzero(buffer, sizeof(buffer));
    while ((n = read(sockfd, buffer, buffer_len)) > 0) {
        memcpy(&data[len], buffer, n);
        len += n;
        printf("Read %d bytes, totally %d bytes...\n", n, len);
        bzero(buffer,sizeof(buffer));
        if (len >= data_len) break;
    }
    if (n < 0) error("ERROR reading from socket");

    memcpy(&decompressionBufferDepth[0], &data[0], Resolution::getInstance().numPixels() * 2);
    memcpy(&decompressionBufferImage[0], &data[Resolution::getInstance().numPixels() * 2], Resolution::getInstance().numPixels() * 3);

    // TODO, set timestamp;

    rgb = (unsigned char *)&decompressionBufferImage[0];
    depth = (unsigned short *)&decompressionBufferDepth[0];

    if(flipColors)
    {
        for(int i = 0; i < Resolution::getInstance().numPixels() * 3; i += 3)
        {
            std::swap(rgb[i + 0], rgb[i + 2]);
        }
    }
}

const std::string SocketLogReader::getFile()
{
    return Parse::get().baseDir().append("live");
}

int SocketLogReader::getNumFrames()
{
    return std::numeric_limits<int>::max();
}

bool SocketLogReader::hasMore()
{
    return true;
}
