#/bin/bash

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` port"
  exit 1
fi

PORT=$1

echo "populate server"

echo Add Users
./TestIRCServer localhost $PORT "ADD-USER superman clarkkent"
./TestIRCServer localhost $PORT "ADD-USER spiderman peterpark"
./TestIRCServer localhost $PORT "ADD-USER mary 123456"

echo Create Room
./TestIRCServer localhost $PORT "CREATE-ROOM superman clarkkent java-programming"
./TestIRCServer localhost $PORT "CREATE-ROOM spiderman peterpark c-programming"

echo Enter room
./TestIRCServer localhost $PORT "ENTER-ROOM superman clarkkent java-programming"
./TestIRCServer localhost $PORT "ENTER-ROOM spiderman peterpark c-programming"

echo Send message
./TestIRCServer localhost $PORT "SEND-MESSAGE superman clarkkent java-programming Hello World!"
./TestIRCServer localhost $PORT "SEND-MESSAGE superman clarkkent java-programming Hi everybody!"
./TestIRCServer localhost $PORT "SEND-MESSAGE superman clarkkent java-programming Welcome to the talk program!"
./TestIRCServer localhost $PORT "SEND-MESSAGE spiderman peterpark c-programming Hello World!"
./TestIRCServer localhost $PORT "SEND-MESSAGE spiderman peterpark c-programming Hi everybody!"
./TestIRCServer localhost $PORT "SEND-MESSAGE spiderman peterpark c-programming Welcome to the talk program!"

