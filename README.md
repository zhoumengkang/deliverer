PHP 祖传代码跑路拯救者，支持 PHP5 & PHP7

# 干嘛用

如果你对线上一个项目运行逻辑不熟悉，或者你对某个方法的调用来源不清楚，又或者线上有一个接口不知道是哪里异常退出了，还有线上调用特别耗时，都比较适合使用 Deliverer 来解决，帮助你避免跑路。

原理是通过在`PHP_MINIT`阶段，通过`zend_set_user_opcode_handler` 来设置对 `ZEND_DO_UCALL` 、`ZEND_DO_FCALL_BY_NAME`、`ZEND_DO_FCALL` 三类 `opcode` 的处理分析。

以下 PHP 版本号是我和其他使用者线上实际使用都 OK 的版本号，理论上 PHP5 和 PHP7 都支持

- [x] 5.3.29
- [x] 5.4.32
- [x] 5.6.40
- [x] 7.0.33
- [x] 7.1.33
- [x] 7.2.5

# 安装

> 如果安装遇到任何问题，可以加我微信 zhoumengkang

移动`./bin/deliverer`到你觉得合适的目录，比如到家目录

```bash
$ cd ./bin
$ mv deliverer ~/ && cd ~
$ chmod +x deliverer
``` 
重新下载的目录
```bash
$ cd ./extension
$ phpize
$ ./configure --with-php-config=/usr/local/php/bin/php-config
$ make && sudo make install
```

`phpize` 和 `php-config` 路径根据自己服务器修改

# 配置 php.ini
在`php.ini`追加以下内容
```bash
[deliverer]
extension=deliverer.so
```
# 重启 php-fpm
```bash
sudo service php-fpm restart
```

用一段我自己很久之前的祖传代码（我的博客）来跑下
```bash
$ ~/deliverer -t
```
这样会一直监控所有的 php 进程的执行

![0.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420468155102.jpg)


```bash
$ ~/deliverer -tAction::initUser -n3 -l5
```

![1.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420481407372.jpg)
![1.1.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420488305285.jpg)


参数 | 值 | 解释
-----|-----|-----
-t | Action::initUser | 过滤包含该调用的请求
-n | 3 | 统计三次然后退出
-l | 5 | 函数（方法）调用深度显示，最多显示 5 层，超出部分在末尾标出


```bash
$ ~/deliverer -v7979-1624369150991941
```
通过 `-v` `requestId` 来详细查看完整调用栈

![2.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420502378165.jpg)


```bash
$ ~/deliverer -tSqlExecute::getAll -n1 -l3
```

当要查询方法，函数调用栈过深，不在层级查询范围之内，则其外层调用显示红色

![3.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420511944914.jpg)