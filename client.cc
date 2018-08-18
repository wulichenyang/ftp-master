#include <examples/netty/ftp/RecServer.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <utility>

#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

class FtpClient : boost::noncopyable
{
 public:
  FtpClient(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop),
      client_(loop, server_addr, "FtpClient"),
      // 在2057端口监听file trans请求
      rec_server_(loop, InetAddress(2057))
  {
    client_.setConnectionCallback(
        boost::bind(&FtpClient::onConnection, this, _1));
    client_.setMessageCallback(
        boost::bind(&FtpClient::onMessage, this, _1, _2, _3));
    rec_server_.start();
    //client_.enableRetry();
  }
  void connect()
  {
    client_.connect();
  }

  void disconnect()
  {
    client_.disconnect();
  }

 private:
  // void handleCmd(const char* command) {
  //   char cmd[kMaxsize], args[kMaxsize];
  //   memset(cmd, 0x00, sizeof(cmd));
  //   memset(args, 0x00, sizeof(args));

  //   const char *kData = command;
  //   int j = 0;
  //   while(*(kData + j) == ' ')
  //     ++j;
  //   if(!strstr(kData + j, " ")) {
  //     //未携带参数
  //     sscanf(kData + j, "%s\r\n", cmd);
  //   } else {
  //     //携带参数
  //     sscanf(kData + j, "%s %s\r\n", cmd, args);
  //   }
  //   if(strncmp(cmd, "GET", 3) == 0) {
  //     // file exists
  //     if(access(args, F_OK) != -1) {
  //       // rm file in this server
  //       execlp("rm", "-rf", args, NULL);
  //     }
  //   }
  // }
  void onConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
        << conn->peerAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
      conn->setTcpNoDelay(true);
      LOG_INFO << "input ftp request command:";
      // 获取命令发送消息
      std::getline(std::cin, message_);
      // handleCmd(message_.c_str());
      conn->send(message_);
      // LOG_INFO << "input ftp req: command args";
      // while(std::getline(std::cin, message_)) {
      //   conn->send(message_);
      //   // LOG_INFO << "input ftp req: command args";
      // }
    }
    else {
      loop_->quit();
    }
  }

  // read from server in buf
  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
  {
    // new commands
    // conn->send(message_);
  }
  const static int kMaxsize = 64;
  EventLoop* loop_;
  TcpClient client_;
  // listen in 2057, wait for file trans
  RecServer rec_server_;
  std::string message_;
  MutexLock mutex_;
  // TcpConnectionPtr connection_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  // if (argc > 1)
  // {
  EventLoop loop;
  // 请求2121端口的ftp服务器
  InetAddress serverAddr("127.0.0.1", 2121);

  FtpClient client(&loop, serverAddr);
  client.connect();
  loop.loop();

  // }
  // else
  // {
  //   printf("Usage: %s host_ip [msg_size]\n", argv[0]);
  // }
}

