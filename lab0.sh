#!/bin/bash

#Ejercicio 1
{
	grep "model name" /proc/cpuinfo
} || {
	echo "Error en el ejercicio 1"
}

#Ejercicio 2
{
	grep "model name" /proc/cpuinfo | wc -l
}||{
	echo "Error en el ejercicio 2"
}

#Ejercicio 3
{
	wget -O Julieta_in_wonderland.txt https://www.gutenberg.org/files/11/11-0.txt && sed -i 's/Alice/Julieta/g' Julieta_in_wonderland.txt
}||{
	echo "Error en el ejercicio 3"
}

#Ejercicio 4
{
	sort -k5n -o weather_cordoba.in weather_cordoba.in && tail -n 1 weather_cordoba.in | awk '{print $1, $2, $3}' && head -n 1 weather_cordoba.in | awk '{print $1, $2, $3}'
}||{
	echo "Error en el ejercicio 4"
}

#Ejercicio 5
{
	sort -k3n -o atpplayers.in atpplayers.in
}||{
	echo "Error en el ejercicio 5"
}

#Ejercicio 6
{
	awk '{print $2, $7 - $8, $0}' superliga.in | sort -k1n | awk '{$1 = $2 = ""; print}' superliga.in > superliga.in
}||{
	echo "Error en el ejercicio 6"
}

#Ejercicio 7
{
	ip addr | grep ether
}||{
	echo "Error en el ejercicio 7"
}

#Ejercicio 8a
{
	mkdir friends && cd friends && for ((i=0;i<10;i++)); do touch friends_S01E0${i}_es.srt; done
}||{
	echo "Error en el ejercicio 8a"
}

#Ejercicio 8b
{
	for ((i=0;i<10;i++)); do mv friends_S01E0${i}_es.srt friends_S01E0${i}.srt; done
}||{
	echo "Error en el ejercicio 8b"
}
