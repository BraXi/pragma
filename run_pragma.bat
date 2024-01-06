@echo off
cd build
start engine.exe +set rcon_password aaaa +set debugcon 1 +exec default +set developer 1337 +set cl_showmiss 1 +connect localhost:27910