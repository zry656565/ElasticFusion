#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <chrono>

#include "SocketLogReader.h"

#define TIMING

#ifdef TIMING
#define INIT_TIMER auto start = std::chrono::high_resolution_clock::now();
#define START_TIMER  start = std::chrono::high_resolution_clock::now();
#define STOP_TIMER(name)  std::cout << "RUNTIME of " << name << ": " << \
    std::chrono::duration_cast<std::chrono::milliseconds>( \
            std::chrono::high_resolution_clock::now()-start \
    ).count() << " ms " << std::endl;
#else
#define INIT_TIMER
#define START_TIMER
#define STOP_TIMER(name)
#endif

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
    depthReadBuffer = new unsigned char[numPixels * 2];
    imageReadBuffer = new unsigned char[numPixels * 3];
    decompressionBufferDepth = new Bytef[Resolution::getInstance().numPixels() * 2];
    decompressionBufferImage = new Bytef[Resolution::getInstance().numPixels() * 3];

    imageSize = Resolution::getInstance().numPixels() * 3;
    depthSize = Resolution::getInstance().numPixels() * 2;

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
    delete [] depthReadBuffer;
    delete [] imageReadBuffer;
    delete [] decompressionBufferDepth;
    delete [] decompressionBufferImage;
    close(sockfd);
}

void SocketLogReader::getNext()
{
    int data_len = Resolution::getInstance().numPixels() * 5;
    unsigned char data[data_len];
    int buffer_len = 50000;
    unsigned char buffer[buffer_len];

    int len = 0;
    int n = 0;
    bzero(data, sizeof(data));
    bzero(buffer, sizeof(buffer));
    depthSize = 0;
    imageSize = 0;
    while ((n = read(sockfd, buffer, buffer_len)) > 0) {
        memcpy(&data[len], buffer, n);
        if (depthSize == 0 || imageSize == 0) {
            memcpy(&imageSize, buffer, sizeof(int32_t));
            memcpy(&depthSize, buffer + 4, sizeof(int32_t));
            data_len = imageSize + depthSize + 8;
            printf("Image Size: %d, Depth Size: %d\n", imageSize, depthSize);
        }
        len += n;
        printf("Read %d bytes, totally %d bytes...\n", n, len);
        bzero(buffer,sizeof(buffer));
        if (len >= data_len) break;
    }
    if (n < 0) error("ERROR reading from socket");

    std::cout << "Get One Frame..." << std::endl;

    memcpy(imageReadBuffer, data + 8, imageSize);
    memcpy(depthReadBuffer, data + 8 + imageSize, depthSize);

    // TODO, set timestamp;

    n = write(sockfd,"DONE",4);
    if (n < 0) error("ERROR writing to socket");
    printf("send `DONE` message to server\n");

    // decompress
    if(depthSize == numPixels * 2)
    {
        memcpy(&decompressionBufferDepth[0], depthReadBuffer, numPixels * 2);
    }
    else
    {
        unsigned long decompLength = numPixels * 2;
        uncompress(&decompressionBufferDepth[0], (unsigned long *)&decompLength, (const Bytef *)depthReadBuffer, depthSize);
    }

    if(imageSize == numPixels * 3)
    {
        memcpy(&decompressionBufferImage[0], imageReadBuffer, numPixels * 3);
    }
    else if(imageSize > 0)
    {
        INIT_TIMER
        START_TIMER
        jpeg.readData(imageReadBuffer, imageSize, (unsigned char *)&decompressionBufferImage[0]);
        STOP_TIMER("Read JPEG")
    }
    else
    {
        memset(&decompressionBufferImage[0], 0, numPixels * 3);
    }

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
