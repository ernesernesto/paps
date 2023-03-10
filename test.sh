#!/usr/bin/env bash

mkdir test

clang -o sim8086 sim8086.c 

nasm listing37.asm -o test/listing37
nasm listing38.asm -o test/listing38
nasm listing39.asm -o test/listing39
nasm listing40.asm -o test/listing40

pushd test > /dev/null
../sim8086 listing37 >> listing37Test.asm
../sim8086 listing38 >> listing38Test.asm
../sim8086 listing39 >> listing39Test.asm
../sim8086 listing40 >> listing40Test.asm

nasm listing37Test.asm -o listing37Test
nasm listing38Test.asm -o listing38Test
nasm listing39Test.asm -o listing39Test
nasm listing40Test.asm -o listing40Test

diff -s listing37 listing37Test
diff -s listing38 listing38Test
diff -s listing39 listing39Test
diff -s listing40 listing40Test

popd > /dev/null
rm -r test

echo "Test Complete!"
