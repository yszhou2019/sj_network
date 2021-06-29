
all:
#	g++ -o mysql `mysql_config --libs --cflags` test_mysql.cpp
#	g++ -o a.out test_tools.cpp
	g++ -o echo test_echo.cpp -I../../../include ../../../common/my_etc.cpp
	g++ -o client test_client.cpp -I../../../include ../../../common/my_etc.cpp

clean:
	rm -rf mysql a.out echo client