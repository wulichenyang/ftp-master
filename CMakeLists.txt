# 添加动态库
add_library(muduo_ftp 
  FtpCommandCodec.cc 
  TransClient.cc 
  RecServer.cc)
# 连接旧动态库
target_link_libraries(muduo_ftp muduo_net)
# 安装动态库到lib
install(TARGETS muduo_ftp DESTINATION lib)
# 安装头文件到include
install(FILES 
          FtpCommandCodec.h
          TransClient.h 
          RecServer.h 
          DESTINATION include/muduo/netty/ftp)
# 添加可执行文件
add_executable(netty_ftp_server server.cc)
add_executable(netty_ftp_client client.cc)
# 连接到ftp库
target_link_libraries(netty_ftp_server muduo_ftp)
target_link_libraries(netty_ftp_client muduo_ftp)