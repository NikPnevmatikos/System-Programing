run:
	g++ -pthread -o dataServer dataServer.cpp
	./dataServer -s 2 -q 3 -b 4 -p 1

	g++ -o remoteClient remoteClient.cpp
	./remoteClient -i 1 -p 2 -d lala


server:
	g++ -pthread -o dataServer inet_str_server.cpp
	./dataServer -p 9002 -s 2 -q 5 -b 30


client:
	g++ -o remoteClient client/inet_str_client.cpp
	./remoteClient 127.0.1.1 9002

run_client:
	./remoteClient 127.0.1.1 9001