#ifndef _MUDUO_EXAMPLES_FTP_COMMAND_CODEC_H_
#define _MUDUO_EXAMPLES_FTP_COMMAND_CODEC_H_

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpClient.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <stdlib.h>

using namespace muduo;
using namespace muduo::net;

namespace muduo
{
namespace net
{
class EventLoop;
}
}

class FtpCommandCodec// : boost::noncopyable
{
public:
	static const int kBufSize = 64*1024;

	explicit FtpCommandCodec(EventLoop* loop,
													 const ConnectionCallback& cb,
													 const MessageCallback& mc)
		: loop_(loop),
		  connectionCallback_(cb),
		  messageCallback_(mc)
	{}
	// TODO: default callback
	FtpCommandCodec(EventLoop* loop) 
		: loop_(loop)
	{};
	FtpCommandCodec(char *args,
									const ConnectionCallback& cb,
									const MessageCallback& mc) 
		: args_(args),
			connectionCallback_(cb),
			messageCallback_(mc)
	{};
	FtpCommandCodec(const ConnectionCallback& cb,
								  const MessageCallback& mc) 
		:	connectionCallback_(cb),
			messageCallback_(mc)
	{};
	FtpCommandCodec()
	{};
	~FtpCommandCodec(){};
	void formatCmd(muduo::string msg, char* buf, char *args);
	void handleCmd(char *cmd, char *args, const TcpConnectionPtr& conn);
	void doList(const TcpConnectionPtr& conn);
	void doFind(const TcpConnectionPtr& conn, char* args);
	void doGet(const TcpConnectionPtr& conn, char *args);
	void doPut(const TcpConnectionPtr& conn, char *args);

	int makeTemp(int command_type);
	size_t readSend(FILE* fp, const TcpConnectionPtr& conn, int type, int namelen);
	size_t readSend(const TcpConnectionPtr& conn, int type, int namelen);

	void onListConnection(const TcpConnectionPtr& conn);
	void onFindConnection(const TcpConnectionPtr& conn);
	void onGetConnection(const TcpConnectionPtr& conn);
	void onPutConnection(const TcpConnectionPtr& conn);
	void onPackMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
	void onPutMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
	void emptyConnection(const TcpConnectionPtr& conn);
	void emptyMessage(const TcpConnectionPtr& conn, Buffer* msg, Timestamp time);
	void defaultMessage(const TcpConnectionPtr&, Buffer* buf, Timestamp time);
	void onWriteComplete(const TcpConnectionPtr& conn);
	void onGetWriteComplete(const TcpConnectionPtr& conn);
	void defaultWriteComplete(const TcpConnectionPtr& conn);
  /// Set connection callback.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

	//服务就绪，可以执行新的用户请求
	static const char req220[];
	//用户名正确，请输入密码
	static const char req331[];
	//用户成功登录，请继续操作
	static const char req230[];
	//NAME系统类型
	static const char req215[];
	//返回path
	//static const char req257[];
	//命令确定，响应TYPE
	static const char req200[];
	//文件状态正常，准备打开数据连接
	static const char req150[];
	//关闭数据连接，请求的文件操作已经完成
	static const char req226[];
	//服务关闭控制连接，响应QUIT
	static const char req221[];
	//请求的文件操作正确，已完成，响应
	static const char req250[];
	//未执行命令，响应未实现命令
	static const char req502[];
	//登录错误信息
	static const char req530[];
	//命令请求未实现
	static const char req500[];
	//未能打开文件
	static const char req550[];

private:
	EventLoop *loop_;
	std::string args_;
  ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	const static int kPayloadLen = 2;
	const static int kHeaderLen = sizeof(int32_t);
};

#endif  // _MUDUO_EXAMPLES_FTP_COMMAND_CODEC_H_