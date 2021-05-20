#!/bin/sh

DEPLOY_FOLDER="/home/utnso/Documentos/Projects/SO_2016/Github/YoNoFui"

#CREATING DESTINATION FOLDER
mkdir -p $DEPLOY_FOLDER
cd $DEPLOY_FOLDER

#DOWNLOADING TP
git clone https://github.com/sisoputnfrba/tp-2016-1c-YoNoFui.git .

#DOWNLOADING SO-COMMONS
cd $DEPLOY_FOLDER/libraries
git clone https://github.com/sisoputnfrba/so-commons-library.git

# INSTALANDO SO-COMMONS
cd $DEPLOY_FOLDER/libraries/so-commons-library/
make
sudo make install 
#checkear instalacion commons
#ls -d /usr/include/commons

#INSTALANDO Parser
cd $DEPLOY_FOLDER/libraries/ansisop-parser/parser
make clean
make all
#checkear si esta la libreria
#ls build/libparser-ansisop.so
sudo make install

cd $DEPLOY_FOLDER/UMC/Debug
make

cd $DEPLOY_FOLDER/consola/Debug
make

cd $DEPLOY_FOLDER/CPU/Debug
make

cd $DEPLOY_FOLDER/libraries/Debug
make

cd $DEPLOY_FOLDER/nucleo/Debug
make

cd $DEPLOY_FOLDER/Swap/Debug
make

cd $DEPLOY_FOLDER
pwd

echo "Fin deploy - ejecutar: 'cd $DEPLOY_FOLDER'"
