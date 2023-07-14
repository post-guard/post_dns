# post_dns

北京邮电大学计算机学院2021级计算机网络课程设置大作业——DNS中继服务器。

## 特点

- 使用`libuv`开发，事件驱动的设计思想
- 基于哈希表的本地缓存
- 提供了对于`A`、`AAAA`、`CNAME`三种类型记录的支持
- `A`、`AAAA`两种类型记录支持多个IP地址
- ~~超高校级的QPS~~

## 测试

采用[dnsperf](https://github.com/DNS-OARC/dnsperf)测试服务器的`QPS`。
使用的域名列表如下：
```
www.bilibili.com A
www.bupt.edu.cn A
my.bupt.edu.cn A
lib.bupt.edu.cn A
jwgl.bupt.edu.cn A
git.rrricardo.top AAAA
ucloud.bupt.edu.cn A
bilibili.com A
www.baidu.com A
www.bing.com A
weibo.com A
www.bupt.edu.cn AAAA
learn.microsoft.com A
wiki.archlinux.org A
www.csdn.net A
```

利用`dnsperf`持续发送20秒请求：
```shell
./dnsperf -d test.txt -l 20
```

得到的测试结果为
```
Statistics:

  Queries sent:         1820207
  Queries completed:    1820152 (100.00%)
  Queries lost:         55 (0.00%)

  Response codes:       NOERROR 1820152 (100.00%)
  Average packet size:  request 32, response 151
  Run time (s):         20.005138
  Queries per second:   90984.226152

  Average Latency (s):  0.000903 (min 0.000012, max 0.356858)
  Latency StdDev (s):   0.002106
```

## 开发

项目采用`cmake`作为编译工具，`libuv`作为子项目包含在工程中，理论上不依赖特定的平台和编译器。

在以下平台上经过测试：
- Linux
- Windows
- macOS

在编译完成之后，使用`-h`参数可以查看程序的帮助文档：
```shell
❯ ./PostDns -h               
[INFO] config.c:77: -h 打印帮助信息
[INFO] config.c:78: -s [server_address] 设置上游服务器地址
[INFO] config.c:79: -4 [file_name] ipv4配置文件
[INFO] config.c:80: -6 [file_name] ipv6配置文件
[INFO] config.c:81: -c [file_name] cname配置文件
[INFO] config.c:82: -l [0/1/2/3] 设置日志等级 0-debug 1-info 2-warn 3-error
```

其中配置文件的格式如下：
```
www.google.com 0.0.0.0
www.pornhub.com 0.0.0.0
www.baidu.cn 0.0.0.0
www.baidu.com 1.2.3.4
```
将域名的解析地址设为`0.0.0.0`即可将该域名屏蔽，返回`NXDOMAIN`。

项目中存在下列已知问题：
- 严重的内存溢出问题：长时间运行/处理大量请求会占用大量的内存
- 偶发的段错误

  我修复了程序中存在的大部分段错误，但是保留了一部分段错误，这样你才你知道你是在写C语言。

## 支持

如果您在学习或者是抄袭的过程中发现了问题，我们十分欢迎您提出， 您可以通过发起`issue`或者是发送电子邮件的方式联系我们。



