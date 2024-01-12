#!/bin/bash

if (( $# == 0 )); then
    echo "Veuillez entrer en paramètre votre fichier de code (.c)"
else
    gcc -pthread $1 -o ./src/executable
    cd src
    su
    ./executable
    rm executable
fi
