`deliverer` 祖传代码跑路拯救者
> 基于 7.2.5 开发，其他版本还未做兼容，本周跟上

# 安装
```bash
$ phpize
$ ./configure --with-php-config=/usr/local/php/bin/php-config
$ make && sudo make install
```
# 配置 php.ini
追加
```bash
[deliverer]
extension=deliverer.so
```
# 重启 php-fpm
```bash
sudo service php-fpm restart
```

# 使用分析工具
可以移动`./bin/deliverer`到你觉得合适的目录，假如在当前目录
```bash
$ chmod +x deliverer
```
用一段我自己很久之前的祖传代码（我的博客）来跑下
```bash
$ ./bin/deliverer -t
```
这样会一直监控所有的 php 进程的执行

![0.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420468155102.jpg)


```bash
$ ./bin/deliverer -tAction::initUser -n3 -l5
```

![1.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420481407372.jpg)
![1.1.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420488305285.jpg)


参数 | 值 | 解释
-----|-----|-----
-t | Action::initUser | 过滤包含该调用的请求
-n | 3 | 统计三次然后退出
-l | 5 | 函数（方法）调用深度显示，最多显示 5 层，超出部分在末尾标出


```bash
$ ./bin/deliverer -v7979-1624369150991941
```
通过 `-v` `requestId` 来详细查看完整调用栈

![2.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420502378165.jpg)


```bash
$ ./bin/deliverer -tSqlExecute::getAll -n1 -l3
```

当要查询方法，函数调用栈过深，不在层级查询范围之内，则其外层调用显示红色

![3.jpg](https://static.mengkang.net/upload/image/2021/0623/1624420511944914.jpg)


# todo

1. 配置项优化
2. 命令行和扩展联动配置
2. 其他版本还未做兼容，本周跟上