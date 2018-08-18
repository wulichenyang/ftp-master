#include <examples/netty/ftp/FtpCommandCodec.h>
#include <examples/netty/ftp/TransClient.h>

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

using namespace muduo;
using namespace muduo::net;


class FtpServer
{
 public:
  static const int kNumThreads = 4;
  static const int kMaxsize = 1024;

  FtpServer(EventLoop* loop, const InetAddress& listenAddr)//, int kNumThreads)
    : server_(loop, listenAddr, "FtpServer"),
      old_counter_(0),
      start_time_(Timestamp::now()),
      codec_(loop,
             boost::bind(&FtpServer::onTransConnection, this, _1),
             boost::bind(&FtpServer::onTransStringMessage, this, _1, _2, _3)) {
    server_.setConnectionCallback(
        boost::bind(&FtpServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&FtpServer::onMessage, this, _1, _2, _3));
    // server_.setThreadNum(kNumThreads);
  }

  void start() {
    LOG_INFO << "starting " << kNumThreads << " threads.";
    server_.start();
  }
  // bind to codec
  void onTransConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
      conn->setTcpNoDelay(true);
    }
    else {
      // loop_->quit();
      LOG_INFO << "tcp TransClient disconn";
      // ??????????????????????????????? how to disCon TransClient
      // conn->getLoop()->quit();
    }
  }

  void onTransStringMessage(const TcpConnectionPtr&,
                       Buffer* buf,
                       Timestamp time)
  {
    string message(buf->retrieveAllAsString());
    printf("<<< %s\n", message.c_str());
  }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
    if(conn->connected()) {
      conn->setTcpNoDelay(true);
    } else {
      conn->shutdown();
    }
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
    // refresh input in client 
    conn->send("input");
    
    char cmd[FtpServer::kMaxsize], args[FtpServer::kMaxsize];
    memset(cmd, 0x00, sizeof(cmd));
    memset(args, 0x00, sizeof(args));

    muduo::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
            << "data received at " << time.toString();
    printf("rec command <<< %s\n", msg.c_str());
    codec_.formatCmd(msg, cmd, args);
    codec_.handleCmd(cmd, args, conn);
  }

  TcpServer server_;
  int64_t old_counter_;
  Timestamp start_time_;
  FtpCommandCodec codec_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  // if (argc > 1)
  // {
      //  kNumThreads = atoi(argv[1]);
  // }
  EventLoop loop;
  // 在2121端口监听ftp请求
  InetAddress listenAddr(2121);
  FtpServer server(&loop, listenAddr);//, FtpServer::kNumThreads);
  server.start();
  loop.loop();
}

