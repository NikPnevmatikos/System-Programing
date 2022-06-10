Onoma: Nikolaos
Pnevmatikos: Pnevmatikos
AM: 1115201900157

-------------------------------------------------------------------------------------------------

compile:

    client: make client
    server: make server

run:

    client: ./remoteClient -i <server_ip> -p <server_port> -d <directory>
    server: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>

-------------------------------------------------------------------------------------------------