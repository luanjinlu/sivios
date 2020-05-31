## sivios
Similar Videos Search

## 简介 
该项目用于构造海量视频特征信息库，对海量视频进行实时相似查找，提供独立API服务。

## 依赖库 
系统依赖libevent和openssl库

## 基本功能描述
有两种模式，api服务器模式和cli命令行模式。  
在进行相似视频检索前，需要先计算视频特征信息，并将视频特征信息添加到视频特征库中，  
即可食用 sivios 对视频特征信息进行相似检索。  
检索的结果用海明距离表示，数值越小表示相似度越高，一般距离在10以内即可认为比较相似，  
可根据实际需求，适当缩小这个距离值。  
  
创建特征数据库  
$ mkdir db  
$ sivioc -C -b db  

将视频特征数据添加到数据库  
$ cat fp.txt | sivioc -b db -a  

查看更多帮助信息  
$ sivioc -h  

sivios 启动需要如下参数  
$ sivios -a <server address> -p <port> -b <base> -n <len> -r -d  
其中	 
	server address:   
		服务器地址，默认0.0.0.0，表示任意地址，如为127.0.0.1，则为本地访问地址；    
	port, 端口号，默认 7000;  
	base, 特征数据库地址，默认 /data/f；  
	len, 视频特征数据的最大长度，默认50;  
	-r, 只读模式，即沒有增加、刪除等操作；  
	-d, 非加密模式；  