#!/bin/bash

KEYPHRASE=testpass
PLAINTEXT=README
CIPHERTEXT_CPU=$PLAINTEXT.encrypted.cpu
CIPHERTEXT_GPU=$PLAINTEXT.encrypted.gpu
PLAINTEXT_GPU=$CIPHERTEXT_GPU.decrypted

REF=openssl
APP=apps/openssl

for CIPHER in -aes-128-ecb -aes-192-ecb -aes-256-ecb
do
	echo -n "Testing $CIPHER encryption... "
	$REF enc $CIPHER -in $PLAINTEXT -out $CIPHERTEXT_CPU -k $KEYPHRASE -nosalt
	$APP enc $CIPHER -in $PLAINTEXT -out $CIPHERTEXT_GPU -k $KEYPHRASE -nosalt
	diff $CIPHERTEXT_CPU $CIPHERTEXT_GPU > /dev/null 2>&1
	if [ $? -eq 0 ]
	then
		echo "Passed."
	else
		echo "Failed."
	fi

	echo -n "Testing $CIPHER decryption... "
	$REF enc -d $CIPHER -in $CIPHERTEXT_GPU -out $PLAINTEXT_GPU -k $KEYPHRASE -nosalt
	diff $PLAINTEXT_GPU $PLAINTEXT > /dev/null 2>&1
	if [ $? -eq 0 ]
	then
		echo "Passed."
	else
		echo "Failed."
	fi

	echo "Cleaning up..."
	rm -f $CIPHERTEXT_CPU
	rm -f $CIPHERTEXT_GPU
	rm -f $PLAINTEXT_GPU

	echo ""
done

