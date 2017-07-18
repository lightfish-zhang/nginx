# 查看进程的堆栈信息

## strace工具，实时查看程序的系统调用的

- 大多数linux发行版都有`strace`的安装包，也可以找到[strace源码地址][1]自行安装
- 常用选项有
    + `-p $pid` 指定进程
    + `-o $filename` 结果输出到某个文件
    + `-f` 跟踪`fork`调用产生的子进程
    + `-t` 输出每一个系统调动的发起时间
    + `-T` 输出每一个系统调用消耗的时间

- 例子

以下案例, 查看nginx的`worker`进程的系统调用的记录。

```
ps -eo pid,stat,command|grep nginx|grep -v grep
    5752 Ss   nginx: master process ./objs/nginx -c /home/lightfish/Git/nginx/conf/nginx.conf
    5753 S    nginx: worker process
sudo strace -p 5753 -t -T 
    strace: Process 5753 attached
    10:59:14 epoll_wait(8, [{EPOLLIN, {u32=166162448, u64=140484251447312}}], 512, -1) = 1 <9.358547>
    10:59:23 accept4(6, {sa_family=AF_INET, sin_port=htons(50874), sin_addr=inet_addr("127.0.0.1")}, [110->16], SOCK_NONBLOCK) = 3 <0.000028>
    10:59:23 epoll_ctl(8, EPOLL_CTL_ADD, 3, {EPOLLIN|EPOLLET, {u32=166162816, u64=140484251447680}}) = 0 <0.000018>
    10:59:23 epoll_wait(8, [{EPOLLIN, {u32=166162816, u64=140484251447680}}], 512, 60000) = 1 <0.000015>
    10:59:23 recvfrom(3, "GET / HTTP/1.1\r\nHost: localhost\r"..., 1024, 0, NULL, NULL) = 73 <0.000015>
    10:59:23 stat("/usr/local/nginx/html/index.html", {st_mode=S_IFREG|0777, st_size=612, ...}) = 0 <0.000020>
    10:59:23 open("/usr/local/nginx/html/index.html", O_RDONLY|O_NONBLOCK) = 9 <0.000020>
    10:59:23 fstat(9, {st_mode=S_IFREG|0777, st_size=612, ...}) = 0 <0.000011>
    10:59:23 writev(3, [{iov_base="HTTP/1.1 200 OK\r\nServer: nginx/1"..., iov_len=215}], 1) = 215 <0.000052>
    10:59:23 sendfile(3, 9, [0] => [612], 612) = 612 <0.000038>
    10:59:23 write(4, "127.0.0.1 - - [16/Jul/2017:10:59"..., 86) = 86 <0.000048>
    10:59:23 close(9)                       = 0 <0.000016>
    10:59:23 setsockopt(3, SOL_TCP, TCP_NODELAY, [1], 4) = 0 <0.000016>
    10:59:23 recvfrom(3, "", 1024, 0, NULL, NULL) = 0 <0.000017>
    10:59:23 close(3)                       = 0 <0.000059>
    10:59:23 epoll_wait(8,
```

## 使用gdb查看进程的堆栈信息

```
(gdb) file /proc/$pid/exe
(gdb) attach $pid
(gdb) bt 
```

## 附录

[strace源码地址][1]

[1]:https://github.com/strace/strace