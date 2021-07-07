【本地前后两次的扫描结果的比对】
【遍历本次扫描的内存结果】
for 本次启动后的扫描结果，遍历每一条记录，都执行一次stat
    if file_old存在:
	    if file.stat == file_old.stat  (也就是file.stat.size == file_old.stat.size && file.stat.mtime == file.stat.mtime)
		   then [pass]
		else 
			then 
				[计算本地file.md5]
		        if file.md5 == file_old.md5
				   then [pass]
				else 
				   then [upload]
				endif
				[更新本地对应的记录 更新md5]
	else:(代表这个文件是新增文件，直接进行上传)
		[计算本地file.md5]
	    [upload]
		[更新本地对应的记录，更新md5]
endfor

【再遍历一遍数据库中的记录】
for 数据库中的每一条记录
	if 扫描结果存在
	   then [pass]
	else 扫描结果不存在
	   then 
			[检查task队列是否存在对应的上传or下载任务]
				if task队列存在对应文件名的任务 (是为了考虑断点续传，传到一半后客户端离线情况下，发生文件的删除操作)
					then [从task队列中获取taskID]
						 [从req队列中移除掉所有匹配taskID的req请求]
			[告知server删除对应的记录]
	        [本地删除对应的记录]
endfor
// 注意，更新本地记录的时候，path+name stat md5 ，都需要写入到本地数据库中
		
/*
【本地前后两次的扫描结果的比对】
for 本次启动后的扫描结果，遍历每一条记录，都执行一次stat
    if file_old存在:
	    if file.stat == file_old.stat  (也就是file.stat.size == file_old.stat.size && file.stat.mtime == file.stat.mtime)
		   then [pass]
		else if file.stat.size != file_old.stat.size (也就是file.stat.size发生变化，这种情况直接上传) 	   
		    then [upload] 
			     [更新本地对应的记录，更新md5]
		else(也就是file.stat.size == file_old.stat.size && file.stat.mtime != file_old.stat.mtime，这种情况下需要检查md5)
		   then 
		        if file.md5 == file_old.md5
				   then [pass]
				else 
				   then [upload]
				endif
				[更新本地对应的记录 更新md5]
	else:(代表这个文件是新增文件，直接进行上传)
	    [upload]
		[更新本地对应的记录，并且计算md5]
*/
		

【存储记录的格式】
路径名+文件名 stat.size stat.mtime md5 is_dir

【改进】
先简单地再把前一次的扫描结果遍历一遍？
如果本次扫描不存在对应的路径名+文件名，那么就把对应的记录删除掉

【本地和远程目录的比对】
for 远程目录(简称里面的记录为file_remote)
    if 文件存在 (此时，客户端本地的对比已经结束，所有本地文件的md5都是已知的)
		if md5 相同
			then [pass]
		else 
			then [download] [添加到本地的新记录中]
	else (代表 本地不存在对应的文件)
	     then [download] [添加到本地的新记录中]
	
endfor


=======================================================================================================

【初次对比】
扫描本地所有文件，调用upload API尝试上传

【本地和远程目录的比对】
for 远程目录(简称里面的记录为file_remote)
    if 文件存在 (此时，客户端本地的对比已经结束，所有本地文件的md5都是已知的)
		if md5 相同
			then [pass]
		else 
			then [download] [添加到本地的新记录中]
	else (代表 本地不存在对应的文件)
	     then [download] [添加到本地的新记录中]
	
endfor

