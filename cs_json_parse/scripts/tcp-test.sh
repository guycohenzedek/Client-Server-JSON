#! /bin/bash

# Run server
#valgrind --leak-check=yes --track-origins=yes 
../bin/gsi_parse_json_server --cfg=../config/gsi_parse_json_config_server.conf &

# Save server PID for later
P1=$!
sleep 2

# Run client 1
#valgrind --leak-check=yes --track-origins=yes 
../bin/gsi_parse_json_client_1 --cfg=../config/gsi_parse_json_config_client1.conf &
sleep 2

# Run client 2
#valgrind --leak-check=yes --track-origins=yes 
../bin/gsi_parse_json_client_2 --cfg=../config/gsi_parse_json_config_client2.conf &
sleep 2

# Run client 3
#valgrind --leak-check=yes --track-origins=yes 
../bin/gsi_parse_json_client_3 --cfg=../config/gsi_parse_json_config_client3.conf &

# Wait to server to finish its job
wait $P1
