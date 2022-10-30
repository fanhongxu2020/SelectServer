#include <array>
#include <cassert>
#include <iostream>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
  constexpr int kPort = 14564;
};

int SetNonBlock(int fd) {
  int flags;
  // 设置描述符位非阻塞模式
#if defined(O_NONBLOCK)
  if (-1 == (flags = fcntl(fd, F_GETFL, 0))) {
    flags = 0;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  flags = 1;
  return ioctl(fd, FIONBIO, &flags);
#endif
}

int StartListening(int port) {
  // extern int socket (int __domain, int __type, int __protocol) __THROW;
  // 作用：创建套接字
  // __domain：地址族（Address Family），就是IP地址类型，常用的有AF_INET、AF_INET6。
  //           AF_INET：表示IPv4地址，例如：192.168.1.10
  //           AF_INET6：表示IPv6地址，例如：1030::C9B4:FF12:48AA:1A2B
  // __type：数据传送方式。常用的有SOCK_STREAM（流格式套接字，面向连接）、SOCK_DGRAM（数据报套接字，无连接）
  // __protocol：传输协议。IPPROTO_TCP、IPPROTO_UDP
  // 返回值：描述符。-1表示错误，并设置errno
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    std::cerr << "cannot open socket!" << std::endl;
    return -1;
  }

  // sockaddr_in：套接字地址结构体
  struct sockaddr_in sock_addr;
  sock_addr.sin_family = AF_INET;
  // htons：将16位无符号整数从主机字节顺序转换位网络字节顺序
  sock_addr.sin_port = htons(port);
  // htonl：和htons一样，区别在于参数类型为32位无符号整数
  // INADDR_ANY表示绑定到任意IP地址。假设你的机器有3个IP地址，对方通过connect来连接3个中的任意一个都可以成功
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind：将套接字和地址信息关联
  // 0：成功 -1：失败
  int ret = bind(fd, reinterpret_cast<sockaddr*>(&sock_addr), sizeof(sock_addr));
  // int ret = bind(fd, (sockaddr*)&sock_addr, sizeof(sock_addr));
  if (ret == -1) {
    std::cerr << "failed to bind socket!" << std::endl;
    return -1;
  }

  ret = SetNonBlock(fd);
  if (ret == -1) {
    std::cerr << "failed to set non-blocking mode" << std::endl;
    return -1;
  }

  // 将主动连接套接字设置为被动连接套接字
  // int listen(int __fd, int __n)
  // __n：请求连接队列，队列中的请求连接数超过该值返回错误
  ret = listen(fd, SOMAXCONN);
  if (ret == -1) {
    std::cerr << "failed to listen fd" << std::endl;
  }

  return fd;
}

int MaxSocket(int server_socket, const std::set<int>& client_sockets) {
  if (client_sockets.empty()) {
    return server_socket;
  } else {
    return std::max(server_socket, *client_sockets.rbegin());
  }
}

int main () {
  int socket_server = StartListening(kPort);
  if (socket_server == -1) {
    std::cerr << "invalid socket!" << std::endl;
    return 1;
  }

  std::set<int> client_sockets;
  fd_set fds;

  while (true) {
    // 将指定的文件描述符集清空，在对文件描述符集合进行设置前，必须对其进行初始化，
    // 如果不清空，由于在系统分配内存空间后，通常并不作清空处理，所以结果是不可知的。
    FD_ZERO(&fds);
    // 将fd加入fd_set
    FD_SET(socket_server, &fds);

    for (auto sock : client_sockets) {
      FD_SET(sock, &fds);
    }

    int max = MaxSocket(socket_server, client_sockets);
  }

  return 0;
}
