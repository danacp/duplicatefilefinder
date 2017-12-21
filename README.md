# Duplicate File Finder
Given a folder path, the program calculates duplicated files using and comparing their md5-hash which can be made in two modes; by library(md5-lib) or by calling an application(md5-app) which calculate the hash and send it back to the main program thru a pipe. There is an integer given by entry which indicates the number of threads(minimun:1), they go through the differents sub-tree of folder and use as shared resource a list of all the files with their respective md5-hash called "visitados"(visited).

format of execution: ./duplicados -t <number of threads> -d <main directory> -m <e|l>
    e:execution mode
    l:library mode
