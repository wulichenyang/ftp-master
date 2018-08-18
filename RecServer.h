#include <examples/netty/ftp/FtpCommandCodec.h>
#include <muduo/net/TcpServer.h>

#include <muduo/base/Atomic.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

typedef boost::shared_ptr<FILE> FilePtr;

using namespace muduo;
using namespace muduo::net;

int numThreads = 4;

class RecServer
{
 public:
  explicit RecServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(loop, listenAddr, "RecServer"),
      oldCounter_(0),
      startTime_(Timestamp::now()),
      codec_(boost::bind(&RecServer::onConnection, this, _1),
             boost::bind(&RecServer::onMessage, this, _1, _2, _3))
  {
    server_.setConnectionCallback(
        boost::bind(&RecServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&FtpCommandCodec::onPackMessage, codec_, _1, _2, _3));
    server_.setWriteCompleteCallback(
        boost::bind(&RecServer::onPutWriteComplete, this, _1));
    // server_.setThreadNum(numThreads);
  }
  ~RecServer(){}
  void start() {
    LOG_INFO << "starting " << numThreads << " threads.";
    server_.start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
        // if(conn->connected()) {
        
        // } else {
        //   conn->shutdown();
        // }
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    // string msg(buf->retrieveAllAsString());
    // // get type / name_len in msg
    LOG_INFO << "GET file >>>>>>>>>>>>>>";
    uint8_t type = static_cast<uint8_t>(buf->readInt8());
    uint8_t name_len = static_cast<uint8_t>(buf->readInt8());
    // list/find
    LOG_INFO << "namelen: " << name_len;
    int switch_on = static_cast<int>(type);
    switch (switch_on) {
      // ls/find
      case 0: {
        string msg(buf->retrieveAllAsString());
        printf("%s\n", msg.c_str());
      } break;
      // get
      case 1: {
        string filename(buf->retrieveAsString(static_cast<size_t>(name_len)));
        string content(buf->retrieveAllAsString());
        doSave(filename, content);
      } break;
      // put
      case 2: {
        string filename(buf->retrieveAsString(static_cast<size_t>(name_len)));
        doPut(filename, conn);
      } break;
      default:
        LOG_ERROR << "Error command type";
        break;
    }
  }
  
  // For ftpClient
  // Save file from ftpServer
  void doSave(const string& name, string& content) {
    FILE *fp = fopen(name.c_str(), "ab");
    if(fp) {
      LOG_INFO << "Writting into file";
      ::fwrite(content.c_str(), 1, content.size(), fp);
      ::fclose(fp);
    } else {
      ::fclose(fp);
      LOG_INFO << "Failed to save file";
    }
  }
  // For ftpClient
  // Put file content to server for the first time => No need to pack
  void doPut(const string& name, const TcpConnectionPtr& conn) {
    FILE *fp = fopen(name.c_str(), "rb");
    if (fp) {
		  // fp生命周期和conn相同
      FilePtr ctx(fp, ::fclose);
      conn->setContext(ctx);
      char buf[FtpCommandCodec::kBufSize];
      memset(buf, 0x00, sizeof(buf));

      size_t nread = ::fread(buf, 1, FtpCommandCodec::kBufSize, fp);
      printf( "Put file to >>>>>>>>>>\n");
      conn->send(buf, static_cast<int>(nread));
    } else { 
      conn->shutdown();
      LOG_INFO << "Client - no such file";
    }
  }
  // For ftpClient
  // Put rest of file
  void onPutWriteComplete(const TcpConnectionPtr& conn) {
    // fp生命周期和conn相同
    const FilePtr& fp = boost::any_cast<const FilePtr&>(conn->getContext());
    char buf[FtpCommandCodec::kBufSize];
    size_t nread = ::fread(buf, 1, sizeof(buf), get_pointer(fp));
    if (nread > 0) {
      printf( "Put file to >>>>>>>>>>\n");
      conn->send(buf, static_cast<int>(nread));
    } else {
      conn->shutdown();
      LOG_INFO << "PUT file - done";
    }
  }

  TcpServer server_;
  // AtomicInt64 transferred_;
  // AtomicInt64 receivedMessages_;
  int64_t oldCounter_;
  Timestamp startTime_;
  FtpCommandCodec codec_;
};

// int main(int argc, char* argv[])
// {
//   LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
//   if (argc > 1)
//   {
//     numThreads = atoi(argv[1]);
//   }
//   EventLoop loop;
//   InetAddress listenAddr(2007);
//   RecServer server(&loop, listenAddr);

//   server.start();

//   loop.loop();
// }

