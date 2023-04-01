#!/usr/bin/env bash

clang -o sim8086 sim8086.c 

sim="../../../../sim8086"

dir="computer_enhance/perfaware/part1/test"
mkdir $dir
pushd $dir > /dev/null

nasm ../listing_0037_single_register_mov.asm -o listing_0037_single_register_mov
nasm ../listing_0038_many_register_mov.asm -o listing_0038_many_register_mov
nasm ../listing_0039_more_movs.asm -o listing_0039_more_movs
nasm ../listing_0040_challenge_movs.asm -o listing_0040_challenge_movs
nasm ../listing_0041_add_sub_cmp_jnz.asm -o listing_0041_add_sub_cmp_jnz


$sim listing_0037_single_register_mov >> listing_0037_single_register_mov_test.asm
$sim listing_0038_many_register_mov >> listing_0038_many_register_mov_test.asm
$sim listing_0039_more_movs >> listing_0039_more_movs_test.asm
$sim listing_0040_challenge_movs >> listing_0040_challenge_movs_test.asm
$sim listing_0041_add_sub_cmp_jnz >> listing_0041_add_sub_cmp_jnz_test.asm

nasm listing_0037_single_register_mov_test.asm -o listing_0037_single_register_mov_test
nasm listing_0038_many_register_mov_test.asm -o listing_0038_many_register_mov_test
nasm listing_0039_more_movs_test.asm -o listing_0039_more_movs_test
nasm listing_0040_challenge_movs_test.asm -o listing_0040_challenge_movs_test
nasm listing_0041_add_sub_cmp_jnz_test.asm -o listing_0041_add_sub_cmp_jnz_test

diff -w -s listing_0037_single_register_mov listing_0037_single_register_mov_test
diff -w -s listing_0038_many_register_mov listing_0038_many_register_mov_test
diff -w -s listing_0039_more_movs listing_0039_more_movs_test
diff -w -s listing_0040_challenge_movs listing_0040_challenge_movs_test
diff -w -s listing_0041_add_sub_cmp_jnz listing_0041_add_sub_cmp_jnz_test

#####

nasm ../listing_0043_immediate_movs.asm -o listing_0043_immediate_movs
nasm ../listing_0044_register_movs.asm -o listing_0044_register_movs

$sim -exec listing_0043_immediate_movs >> listing_0043_immediate_movs_test.txt 
$sim -exec listing_0044_register_movs >> listing_0044_register_movs_test.txt 

diff -w -s ../listing_0043_immediate_movs.txt listing_0043_immediate_movs_test.txt 
diff -w -s ../listing_0044_register_movs.txt listing_0044_register_movs_test.txt 

popd > /dev/null
rm -r $dir

echo "Test Complete!"
