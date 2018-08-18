#include <examples/netty/ftp/TransClient.h>
#include <examples/netty/ftp/FtpCommandCodec.h>

#include <muduo/net/EventLoop.h>

typedef boost::shared_ptr<FILE> FilePtr;

//服务就绪，可以执行新的用户请求
static const char req220[] = "220 service ready for new user\r\n";
//用户名正确，请输入密码
static const char req331[] = "331 user name okay, please specify the passwd\r\n";
//用户成功登录，请继续操作
static const char req230[] = "230 login successful\r\n";
//NAME系统类型
static const char req215[] = "215 NAME system type\r\n";
//返回path
//static const char req257[];
//命令确定，响应TYPE
static const char req200[] = "200 command okay.\r\n";
//文件状态正常，准备打开数据连接
static const char req150[] = "150 file status okay; about to open data connection.\r\n";
//关闭数据连接，请求的文件操作已经完成
static const char req226[] = "226 closing data connection.\r\n";
//服务关闭控制连接，响应QUIT
static const char req221[] = "221 service closing control connection.\r\n";
//请求的文件操作正确，已完成，响应
static const char req250[] = "250 requested file action okay, completed.\r\n";
//未执行命令，响应未实现命令
static const char req502[] = "502 command not implemented.\r\n";
//登录错误信息
static const char req530[] = "530 not logged in.\r\n";
//命令请求未实现
static const char req500[] = "500 Syntax error, command unrecognized.\r\n";
//未能打开文件
static const char req550[] = "550 Failed to open file.\r\n";

/*
* All callback
*/ 

// For ftpServer(transClient)
// Send result of ls -al for the first time
void FtpCommandCodec::onListConnection(const TcpConnectionPtr& conn) {
  // FtpServer::onTransConnection
	connectionCallback_(conn);
	// do_list
	//创建临时文件，将“ls -al”的信息输入到该文件中，并在文件的末尾补充\r\n
	if(conn->connected()) {
		int tmp_file_fd = makeTemp(0);
		// 读取文件并发送到RecServer fd -> fp
		FILE *fp = fdopen(tmp_file_fd, "rb");
		if (fp)	{
			readSend(fp, conn, 0, 0);
		}	else {
			conn->shutdown();
			LOG_INFO << "FileServer - no such file";
		}
	}
}
// For ftpServer(transClient)
// Send result of find for the first time
void FtpCommandCodec::onFindConnection(const TcpConnectionPtr& conn) {
  // FtpServer::onTransConnection
	connectionCallback_(conn);
	// do_find
	//创建临时文件，将“find -name”的信息输入到该文件中，并在文件的末尾补充\r\n
	if(conn->connected()) {
		int tmp_file_fd = makeTemp(1);
		// 读取文件并发送到RecServer fd -> fp
		FILE *fp = fdopen(tmp_file_fd, "rb");
		if (fp)	{
			readSend(fp, conn, 0, 0);
		}	else {
			conn->shutdown();
			LOG_INFO << "FileServer - no such file";
		}
	}
}
// For ftpServer(transClient)
// Read the file and send for the first time
void FtpCommandCodec::onGetConnection(const TcpConnectionPtr& conn) {
  // FtpServer::onTransConnection
	connectionCallback_(conn);
	// do_get -> download file from FtpServer
	if(conn->connected()) {
		FILE *fp = fopen(args_.c_str(), "rb");
		if (fp) {
			readSend(fp, conn, 1, static_cast<int>(args_.size()));
		} else {
			conn->shutdown();
			LOG_INFO << "FileServer - no such file";
		}
	}
}
// For ftpServer(transClient)
// Pack message and send filename back to ftpClient(RecServer)
void FtpCommandCodec::onPutConnection(const TcpConnectionPtr& conn) {
	// TODO: file exist

	connectionCallback_(conn);
	if(conn->connected()) {
		char buf[kBufSize + kHeaderLen];
		memset(buf, 0x00, sizeof(buf));

		// header =>
			// body len
		int32_t len = static_cast<int32_t>(kPayloadLen + args_.size());
		int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
    memcpy(buf, static_cast<void*>(&be32), sizeof(int32_t));
		// body =>
			// prepend => req_type(0 -> ls/find) len(filename_len)
			//            req_type(1 -> get    ) len(filename_len)
			//            req_type(2 -> put    ) len(filename_len)
			// [req_type(1B) | name_len(1B) | content(filename + content)]
		uint8_t req_type = static_cast<uint8_t>(2);
		uint8_t name_len = static_cast<uint8_t>(args_.size());
		buf[kHeaderLen] = static_cast<char>(req_type);
		buf[kHeaderLen + 1] = static_cast<char>(name_len);

		strncpy(buf + kHeaderLen + kPayloadLen, args_.c_str(), name_len);
		// file name
		// no content
		printf( "Send file name to RecServer>>>>>\n");
		conn->send(buf, static_cast<int>(kHeaderLen + kPayloadLen + name_len));
	}
}
// For ftpClient(recServer)
// Unpack message from ftpServer
void FtpCommandCodec::onPackMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
	// 判断是否超过包头，如果包头都超不过，那半个消息都算不上
	while(buf->readableBytes() >= kHeaderLen) {
		const void* data = buf->peek();
		// Header => message len
		int32_t be32 = *static_cast<const int32_t*>(data);
		const int32_t len = muduo::net::sockets::networkToHost32(be32);
		// 如果消息超过64K，或者长度小于0，不合法，干掉它。
		if (len > 65536 || len < 0) {
        LOG_ERROR << "Invalid length " << len;
        conn->shutdown(); 
        break;
    } else if(static_cast<int>(buf->readableBytes()) >= len + kHeaderLen) {
			buf->retrieve(kHeaderLen);  // 取头部
			// muduo::string message(buf->peek(), len);  // 取包体
			Buffer tmp_buf = *buf;
			messageCallback_(conn, &tmp_buf, time);   // 取出包体后就可以处理回调了
			buf->retrieve(len);  // 取包体
		} else { // 未达到一条完整的消息
    	break;
    }
	}
}

void FtpCommandCodec::emptyConnection(const TcpConnectionPtr& conn) {

}

void FtpCommandCodec::emptyMessage(const TcpConnectionPtr& conn,
																	Buffer* msg,
																	Timestamp time) {

}

// ftpServer(transClient) handle put message => Just append to the file
void FtpCommandCodec::onPutMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
	// do_put
	// Save file to FtpServer
	string content(buf->retrieveAllAsString());
	FILE *fp = ::fopen(args_.c_str(), "ab");
	if(fp) {
		::fwrite(content.c_str(), 1, content.size(), fp);
		LOG_INFO << "Save file <<<<<<<<<<<<<<<<<<<";
		::fclose(fp);
	} else {
		::fclose(fp);
		LOG_INFO << "Failed to save file";
	}
  // FtpServer::onStringMessage
	// const muduo::string msg = "put file";
	// messageCallback_(conn, msg, time);
}

// Default msg (no need for recv)
void FtpCommandCodec::defaultMessage(const TcpConnectionPtr&, Buffer* buf, Timestamp time) {
  buf->retrieveAll();
}

// For ftpServer(transClient) onWriteComplete for list/find
void FtpCommandCodec::onWriteComplete(const TcpConnectionPtr& conn) {
	// fp生命周期和conn相同
  if (readSend(conn, 0, 0) <= 0) {
    conn->shutdown();
    LOG_INFO << "LIST/FIND - done";
  }
	
}
// For ftpServer(transClient) onGetWriteComplete for get
void FtpCommandCodec::onGetWriteComplete(const TcpConnectionPtr& conn) {
	// fp生命周期和conn相同
	if (readSend(conn, 1, static_cast<int>(args_.size())) <= 0) {
    conn->shutdown();
    LOG_INFO << "Get file - done";
  }
}

// default writeComplete
void FtpCommandCodec::defaultWriteComplete(const TcpConnectionPtr& conn) {

}
/*
* handle event
*/ 
// For ftpServer(transClient) to handle ls -al
void FtpCommandCodec::doList(const TcpConnectionPtr& conn) {
  //服务器主动创建文件传输连接
	// 连接ftp客户端的2057端口，进行文件传输
  uint16_t port = static_cast<uint16_t>(2057);
  InetAddress peer_addr(conn->peerAddress().toIp(), port);
	// 新建list传输客户端
  TransClient trans_client(
		loop_, 
		peer_addr,
		boost::bind(&FtpCommandCodec::onListConnection, this, _1),
		boost::bind(&FtpCommandCodec::defaultMessage, this, _1, _2, _3),
		boost::bind(&FtpCommandCodec::onWriteComplete, this, _1)
	);
  trans_client.connect();
  loop_->loop();
}

// For ftpServer(transClient) to handle find
void FtpCommandCodec::doFind(const TcpConnectionPtr& conn, char* args) {
  //服务器主动创建文件传输连接
	// 连接ftp客户端的2057端口，进行文件传输
  uint16_t port = static_cast<uint16_t>(2057);
  InetAddress peer_addr(conn->peerAddress().toIp(), port);
	// 新建find传输客户端
	FtpCommandCodec codec(args,
												boost::bind(&FtpCommandCodec::emptyConnection, this, _1),
												boost::bind(&FtpCommandCodec::emptyMessage, this, _1, _2, _3));
	TransClient trans_client(
		loop_, 
		peer_addr,
		boost::bind(&FtpCommandCodec::onFindConnection, &codec, _1),
		boost::bind(&FtpCommandCodec::defaultMessage, &codec, _1, _2, _3),
		boost::bind(&FtpCommandCodec::onWriteComplete, &codec, _1)
	);
  trans_client.connect();
  loop_->loop();
}

// For ftpServer(transClient) to handle get
void FtpCommandCodec::doGet(const TcpConnectionPtr& conn, char* args) {
  //服务器主动创建文件传输连接
	// 连接ftp客户端的2057端口，进行文件传输
  uint16_t port = static_cast<uint16_t>(2057);
  InetAddress peer_addr(conn->peerAddress().toIp(), port);
	// 新建get传输客户端
	FtpCommandCodec codec(args,
												boost::bind(&FtpCommandCodec::emptyConnection, this, _1),
												boost::bind(&FtpCommandCodec::emptyMessage, this, _1, _2, _3));
  TransClient trans_client(
		loop_,
		peer_addr,
		boost::bind(&FtpCommandCodec::onGetConnection, &codec, _1),
		boost::bind(&FtpCommandCodec::defaultMessage, &codec, _1, _2, _3),
		boost::bind(&FtpCommandCodec::onGetWriteComplete, &codec, _1)
	);
  trans_client.connect();
  loop_->loop();
}
// For ftpServer(transClient) to handle put
void FtpCommandCodec::doPut(const TcpConnectionPtr& conn, char* args) {
  //服务器主动创建文件传输连接
	// 连接ftp客户端的2057端口，进行文件传输
  uint16_t port = static_cast<uint16_t>(2057);
  InetAddress peer_addr(conn->peerAddress().toIp(), port);
	// 新建put传输客户端
	FtpCommandCodec codec(args,
												boost::bind(&FtpCommandCodec::emptyConnection, this, _1),
												boost::bind(&FtpCommandCodec::emptyMessage, this, _1, _2, _3));
  TransClient trans_client(
		loop_,
		peer_addr,
		boost::bind(&FtpCommandCodec::onPutConnection, &codec, _1),
		boost::bind(&FtpCommandCodec::onPutMessage, &codec, _1, _2, _3),
		boost::bind(&FtpCommandCodec::defaultWriteComplete, &codec, _1)
	);
  trans_client.connect();
  loop_->loop();
}

int FtpCommandCodec::makeTemp(int command_type) {
	int tmp_file_fd;
	char tmp_file_name[] = "tmp_XXXXXX";
	if((tmp_file_fd = mkstemp(tmp_file_name)) == -1) {
		LOG_ERROR << "Make temp file failed";
		return -1;
	}
	char excbuff[64] = {'\0'};
	switch (command_type) {
		// ls
		case 0:
			sprintf(excbuff, "ls -al > %s", tmp_file_name);
			break;
		// find	
		case 1:
			sprintf(excbuff, "find -name %s > %s", args_.c_str(), tmp_file_name);
			break;
		default:
			LOG_ERROR << "Invalid command type";
			return -1;
			break;
	}
	// Excecute
	if(system(excbuff) == -1) {
		LOG_ERROR << "Exc ls/find failed";
		return -1;
	}
	// 取消连接，最后一个打开的进程关闭文件操作符
	// 或者程序退出后临时文件被自动彻底地删除
	unlink(tmp_file_name);
	lseek(tmp_file_fd, 0, SEEK_SET);
	return tmp_file_fd;
}
// first time to send
size_t FtpCommandCodec::readSend(FILE* fp, const TcpConnectionPtr& conn, int type, int namelen) {
		// fp生命周期和conn相同
		FilePtr ctx(fp, ::fclose);
		conn->setContext(ctx);
		char buf[kBufSize + kHeaderLen];
		memset(buf, 0x00, sizeof(buf));
		// body =>
			// prepend => req_type(0 -> ls/find) len(filename_len)
			//            req_type(1 -> get    ) len(filename_len)
			//            req_type(2 -> put    ) len(filename_len)
			// [req_type(1B) | name_len(1B) | content(filename + content)]

		// payload
		uint8_t req_type = static_cast<uint8_t>(type);
    uint8_t name_len = static_cast<uint8_t>(namelen);

		buf[kHeaderLen] = static_cast<char>(req_type);
		buf[kHeaderLen + 1] = static_cast<char>(name_len);
		// filename
		strncpy(buf + kHeaderLen + kPayloadLen, args_.c_str(), name_len);
		// file content
		size_t nread = ::fread(buf + kHeaderLen + kPayloadLen + name_len, 1, kBufSize - kHeaderLen - kPayloadLen - name_len, fp);
		// header =>
			// body len
		int32_t len = static_cast<int32_t>(kPayloadLen + name_len + nread);
    int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
    memcpy(buf, static_cast<void*>(&be32), sizeof(int32_t));

		printf( "Send file >>>>>>>>>>>>>%d\n", static_cast<int>(len));
		if(nread > 0) {
			conn->send(buf, static_cast<int>(kHeaderLen + kPayloadLen + namelen + nread));
		}
		return nread;
}
// not first time to send
size_t FtpCommandCodec::readSend(const TcpConnectionPtr& conn, int type, int namelen) {
		// fp生命周期和conn相同
	  const FilePtr& fp = boost::any_cast<const FilePtr&>(conn->getContext());
		char buf[kBufSize + kHeaderLen];
		memset(buf, 0x00, sizeof(buf));
		// body =>
			// prepend => req_type(0 -> ls/find) len(filename_len)
			//            req_type(1 -> get    ) len(filename_len)
			//            req_type(2 -> put    ) len(filename_len)
			// [req_type(1B) | name_len(1B) | content(filename + content)]

		// payload
		uint8_t req_type = static_cast<uint8_t>(type);
    uint8_t name_len = static_cast<uint8_t>(namelen);

		buf[kHeaderLen] = static_cast<char>(req_type);
		buf[kHeaderLen + 1] = static_cast<char>(name_len);
		// filename
		strncpy(buf + kHeaderLen + kPayloadLen, args_.c_str(), name_len);
		// file content
		size_t nread = ::fread(buf + kHeaderLen + kPayloadLen + name_len, 1, kBufSize - kHeaderLen - kPayloadLen - name_len, get_pointer(fp));
		// header =>
			// body len
		int32_t len = static_cast<int32_t>(kPayloadLen + name_len + nread);
    int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
    memcpy(buf, static_cast<void*>(&be32), sizeof(int32_t));

		printf( "Send file >>>>>>>>>>>>> len:%d \n", static_cast<int>(len));
		if(nread > 0) {
			conn->send(buf, static_cast<int>(kHeaderLen + kPayloadLen + namelen + nread));
		}
		return nread;
}
void FtpCommandCodec::handleCmd(char *cmd, char *args, const TcpConnectionPtr& conn) {
  	if(strncmp(cmd, "LIST", 4) == 0) {
			doList(conn);
		} else if(strncmp(cmd, "FIND", 4) == 0) {
			doFind(conn, args);
		} else if(strncmp(cmd, "GET", 3) == 0) {
			doGet(conn, args);
		} else if(strncmp(cmd, "PUT", 3) == 0) {
			doPut(conn, args);
		} 
		// else if(strncmp(cmd, "RETR", 4) == 0) {
		// 	do_retr();
		// }
		// else if(strncmp(cmd, "STOR", 4) == 0) {
		// 	do_stor();
		// }
		// else if(strncmp(cmd, "CWD", 3) == 0) {
		// 	do_cwd();
		// }
		// else if(strncmp(cmd, "PWD", 3) == 0) {
		// 	do_pwd();
		// }
		// else if(strncmp(cmd, "QUIT", 4) == 0) {
		// 	do_quit();
		// 	return 0;
		// }
		// else if(strncmp(cmd, "SYST", 4) == 0) {
		// 	do_syst();
		// }
		// else if(strncmp(cmd, "TYPE", 4) == 0) {
		// 	do_type();
		// }
		// else if(strncmp(cmd, "PORT", 4) == 0) {
		// 	do_port();
		// }
		else {
			// strcpy(m_writeBuff, req502);
		}

		// m_epollPtr->mod(this, EPOLLOUT);
}
void FtpCommandCodec::formatCmd(muduo::string msg, char* cmd, char *args) {
    //读取输入
    //消除左边的空格
    const char *kData = msg.c_str();
    int j = 0;
    while(*(kData + j) == ' ')
      ++j;
    if(!strstr(kData + j, " ")) {
      //未携带参数
      sscanf(kData + j, "%s\r\n", cmd);
    } else {
      //携带参数
      sscanf(kData + j, "%s %s\r\n", cmd, args);
    }
}
