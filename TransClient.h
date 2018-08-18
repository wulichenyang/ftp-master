#include <examples/netty/ftp/FtpCommandCodec.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

class TransClient : boost::noncopyable
{
 public:
  TransClient(EventLoop* loop, 
              const InetAddress& peer_addr,
              ConnectionCallback cb,
              MessageCallback mb,
              WriteCompleteCallback wb)
    : client_(loop, peer_addr, "TransClient")
      // bind TransClient::Callback to codec
      // codec_(boost::bind(&TransClient::onTransConnection, this, _1),
      //        boost::bind(&TransClient::onStringMessage, this, _1, _2, _3)
      //       )
  {
    // bind FtpCommandCodec::Callback to client
    client_.setConnectionCallback(cb);
    client_.setMessageCallback(mb);
    client_.setWriteCompleteCallback(wb);
    // client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

  void disconnect()
  {
    client_.disconnect();
  }

  void write(const StringPiece& message)
  {
    // MutexLockGuard lock(mutex_);
    // if (connection_)
    // {
    //   codec_.send(get_pointer(connection_), message);
    // }
  }
  // not used xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  
  // void onConnection(const TcpConnectionPtr& conn)
  // {
  //   LOG_INFO << conn->localAddress().toIpPort() << " -> "
  //             << conn->peerAddress().toIpPort() << " is "
  //             << (conn->connected() ? "UP" : "DOWN");

  //   if (conn->connected()) {
  //     conn->setTcpNoDelay(true);
  //   }
  //   else {
  //     // ???
  //     conn->getLoop()->quit();
  //     // loop_->quit();
  //   }




    // MutexLockGuard lock(mutex_);
    // if (conn->connected())
    // {
    //   connection_ = conn;
    // }
    // else
    // {
    //   connection_.reset();
    // }
  // }

 private:
  TcpClient client_;
  // FtpCommandCodec codec_;
  MutexLock mutex_;
  TcpConnectionPtr connection_;
};


