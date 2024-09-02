.text
.global sha256_update
sha256_update:

  subi 1, 1, 176
  stxv 32+20, 0(1)
  stxv 32+21, 16(1)
  stxv 32+22, 32(1)
  stxv 32+23, 48(1)
  stxv 32+24, 64(1)
  stxv 32+25, 80(1)
  stxv 32+26, 96(1)
  stxv 32+28, 112(1)
  stxv 32+29, 128(1)
  stxv 32+30, 144(1)
  stxv 32+31, 160(1)
  li 10, 16
  lxvw4x 32+16, 0, 3
  lxvw4x 32+20, 10, 3
  vsldoi 17, 16, 16, 4
  vsldoi 18, 16, 16, 8
  vsldoi 19, 16, 16, 12
  vsldoi 21, 20, 20, 4
  vsldoi 22, 20, 20, 8
  vsldoi 23, 20, 20, 12
  b L1
.align 4
L0:
  xxmrghd 32+28, 32+16, 32+20
  xxmrghd 32+29, 32+17, 32+21
  xxmrghd 32+30, 32+18, 32+22
  xxmrghd 32+31, 32+19, 32+23
  lxvb16x 32+0, 0, 4
  lxvw4x 32+24, 0, 6
  addi 4, 4, 16
  addi 6, 6, 16
  vadduwm 23, 23, 24
  vsldoi 24, 24, 24, 4
  lxvb16x 32+4, 0, 4
  addi 4, 4, 16
  lxvb16x 32+8, 0, 4
  addi 4, 4, 16
  lxvb16x 32+12, 0, 4
  addi 4, 4, 16
  vsldoi 1, 0, 0, 4
  vsldoi 2, 0, 0, 8
  vsldoi 3, 0, 0, 12
  vadduwm 23, 23, 0
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 1
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 2
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 3
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vsldoi 5, 4, 4, 4
  vsldoi 6, 4, 4, 8
  vsldoi 7, 4, 4, 12
  vadduwm 19, 19, 4
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 5
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 6
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 7
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vsldoi 9, 8, 8, 4
  vsldoi 10, 8, 8, 8
  vsldoi 11, 8, 8, 12
  vadduwm 23, 23, 8
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 9
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 10
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 11
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vsldoi 13, 12, 12, 4
  vsldoi 14, 12, 12, 8
  vsldoi 15, 12, 12, 12
  vadduwm 19, 19, 12
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 13
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 14
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 15
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 1, 0, 0
  vadduwm 0, 0, 25
  vshasigmaw 26, 14, 0, 15
  vadduwm 0, 0, 26
  vadduwm 0, 0, 9
  vshasigmaw 25, 2, 0, 0
  vadduwm 1, 1, 25
  vshasigmaw 26, 15, 0, 15
  vadduwm 1, 1, 26
  vadduwm 1, 1, 10
  vshasigmaw 25, 3, 0, 0
  vadduwm 2, 2, 25
  vshasigmaw 26, 0, 0, 15
  vadduwm 2, 2, 26
  vadduwm 2, 2, 11
  vshasigmaw 25, 4, 0, 0
  vadduwm 3, 3, 25
  vshasigmaw 26, 1, 0, 15
  vadduwm 3, 3, 26
  vadduwm 3, 3, 12
  vshasigmaw 25, 5, 0, 0
  vadduwm 4, 4, 25
  vshasigmaw 26, 2, 0, 15
  vadduwm 4, 4, 26
  vadduwm 4, 4, 13
  vadduwm 23, 23, 0
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 1
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 2
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 3
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 6, 0, 0
  vadduwm 5, 5, 25
  vshasigmaw 26, 3, 0, 15
  vadduwm 5, 5, 26
  vadduwm 5, 5, 14
  vshasigmaw 25, 7, 0, 0
  vadduwm 6, 6, 25
  vshasigmaw 26, 4, 0, 15
  vadduwm 6, 6, 26
  vadduwm 6, 6, 15
  vshasigmaw 25, 8, 0, 0
  vadduwm 7, 7, 25
  vshasigmaw 26, 5, 0, 15
  vadduwm 7, 7, 26
  vadduwm 7, 7, 0
  vshasigmaw 25, 9, 0, 0
  vadduwm 8, 8, 25
  vshasigmaw 26, 6, 0, 15
  vadduwm 8, 8, 26
  vadduwm 8, 8, 1
  vadduwm 19, 19, 4
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 5
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 6
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 7
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 10, 0, 0
  vadduwm 9, 9, 25
  vshasigmaw 26, 7, 0, 15
  vadduwm 9, 9, 26
  vadduwm 9, 9, 2
  vshasigmaw 25, 11, 0, 0
  vadduwm 10, 10, 25
  vshasigmaw 26, 8, 0, 15
  vadduwm 10, 10, 26
  vadduwm 10, 10, 3
  vshasigmaw 25, 12, 0, 0
  vadduwm 11, 11, 25
  vshasigmaw 26, 9, 0, 15
  vadduwm 11, 11, 26
  vadduwm 11, 11, 4
  vshasigmaw 25, 13, 0, 0
  vadduwm 12, 12, 25
  vshasigmaw 26, 10, 0, 15
  vadduwm 12, 12, 26
  vadduwm 12, 12, 5
  vadduwm 23, 23, 8
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 9
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 10
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 11
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 14, 0, 0
  vadduwm 13, 13, 25
  vshasigmaw 26, 11, 0, 15
  vadduwm 13, 13, 26
  vadduwm 13, 13, 6
  vshasigmaw 25, 15, 0, 0
  vadduwm 14, 14, 25
  vshasigmaw 26, 12, 0, 15
  vadduwm 14, 14, 26
  vadduwm 14, 14, 7
  vshasigmaw 25, 0, 0, 0
  vadduwm 15, 15, 25
  vshasigmaw 26, 13, 0, 15
  vadduwm 15, 15, 26
  vadduwm 15, 15, 8
  vshasigmaw 25, 1, 0, 0
  vadduwm 0, 0, 25
  vshasigmaw 26, 14, 0, 15
  vadduwm 0, 0, 26
  vadduwm 0, 0, 9
  vadduwm 19, 19, 12
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 13
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 14
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 15
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 2, 0, 0
  vadduwm 1, 1, 25
  vshasigmaw 26, 15, 0, 15
  vadduwm 1, 1, 26
  vadduwm 1, 1, 10
  vshasigmaw 25, 3, 0, 0
  vadduwm 2, 2, 25
  vshasigmaw 26, 0, 0, 15
  vadduwm 2, 2, 26
  vadduwm 2, 2, 11
  vshasigmaw 25, 4, 0, 0
  vadduwm 3, 3, 25
  vshasigmaw 26, 1, 0, 15
  vadduwm 3, 3, 26
  vadduwm 3, 3, 12
  vshasigmaw 25, 5, 0, 0
  vadduwm 4, 4, 25
  vshasigmaw 26, 2, 0, 15
  vadduwm 4, 4, 26
  vadduwm 4, 4, 13
  vadduwm 23, 23, 0
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 1
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 2
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 3
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 6, 0, 0
  vadduwm 5, 5, 25
  vshasigmaw 26, 3, 0, 15
  vadduwm 5, 5, 26
  vadduwm 5, 5, 14
  vshasigmaw 25, 7, 0, 0
  vadduwm 6, 6, 25
  vshasigmaw 26, 4, 0, 15
  vadduwm 6, 6, 26
  vadduwm 6, 6, 15
  vshasigmaw 25, 8, 0, 0
  vadduwm 7, 7, 25
  vshasigmaw 26, 5, 0, 15
  vadduwm 7, 7, 26
  vadduwm 7, 7, 0
  vshasigmaw 25, 9, 0, 0
  vadduwm 8, 8, 25
  vshasigmaw 26, 6, 0, 15
  vadduwm 8, 8, 26
  vadduwm 8, 8, 1
  vadduwm 19, 19, 4
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 5
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 6
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 7
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 10, 0, 0
  vadduwm 9, 9, 25
  vshasigmaw 26, 7, 0, 15
  vadduwm 9, 9, 26
  vadduwm 9, 9, 2
  vshasigmaw 25, 11, 0, 0
  vadduwm 10, 10, 25
  vshasigmaw 26, 8, 0, 15
  vadduwm 10, 10, 26
  vadduwm 10, 10, 3
  vshasigmaw 25, 12, 0, 0
  vadduwm 11, 11, 25
  vshasigmaw 26, 9, 0, 15
  vadduwm 11, 11, 26
  vadduwm 11, 11, 4
  vshasigmaw 25, 13, 0, 0
  vadduwm 12, 12, 25
  vshasigmaw 26, 10, 0, 15
  vadduwm 12, 12, 26
  vadduwm 12, 12, 5
  vadduwm 23, 23, 8
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 9
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 10
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 11
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 14, 0, 0
  vadduwm 13, 13, 25
  vshasigmaw 26, 11, 0, 15
  vadduwm 13, 13, 26
  vadduwm 13, 13, 6
  vshasigmaw 25, 15, 0, 0
  vadduwm 14, 14, 25
  vshasigmaw 26, 12, 0, 15
  vadduwm 14, 14, 26
  vadduwm 14, 14, 7
  vshasigmaw 25, 0, 0, 0
  vadduwm 15, 15, 25
  vshasigmaw 26, 13, 0, 15
  vadduwm 15, 15, 26
  vadduwm 15, 15, 8
  vshasigmaw 25, 1, 0, 0
  vadduwm 0, 0, 25
  vshasigmaw 26, 14, 0, 15
  vadduwm 0, 0, 26
  vadduwm 0, 0, 9
  vadduwm 19, 19, 12
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 13
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 14
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 15
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 2, 0, 0
  vadduwm 1, 1, 25
  vshasigmaw 26, 15, 0, 15
  vadduwm 1, 1, 26
  vadduwm 1, 1, 10
  vshasigmaw 25, 3, 0, 0
  vadduwm 2, 2, 25
  vshasigmaw 26, 0, 0, 15
  vadduwm 2, 2, 26
  vadduwm 2, 2, 11
  vshasigmaw 25, 4, 0, 0
  vadduwm 3, 3, 25
  vshasigmaw 26, 1, 0, 15
  vadduwm 3, 3, 26
  vadduwm 3, 3, 12
  vshasigmaw 25, 5, 0, 0
  vadduwm 4, 4, 25
  vshasigmaw 26, 2, 0, 15
  vadduwm 4, 4, 26
  vadduwm 4, 4, 13
  vadduwm 23, 23, 0
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 1
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 2
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 3
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 6, 0, 0
  vadduwm 5, 5, 25
  vshasigmaw 26, 3, 0, 15
  vadduwm 5, 5, 26
  vadduwm 5, 5, 14
  vshasigmaw 25, 7, 0, 0
  vadduwm 6, 6, 25
  vshasigmaw 26, 4, 0, 15
  vadduwm 6, 6, 26
  vadduwm 6, 6, 15
  vshasigmaw 25, 8, 0, 0
  vadduwm 7, 7, 25
  vshasigmaw 26, 5, 0, 15
  vadduwm 7, 7, 26
  vadduwm 7, 7, 0
  vshasigmaw 25, 9, 0, 0
  vadduwm 8, 8, 25
  vshasigmaw 26, 6, 0, 15
  vadduwm 8, 8, 26
  vadduwm 8, 8, 1
  vadduwm 19, 19, 4
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 5
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 6
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 16, 16, 7
  vsel 25, 23, 22, 21
  vadduwm 23, 23, 24
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 10, 0, 0
  vadduwm 9, 9, 25
  vshasigmaw 26, 7, 0, 15
  vadduwm 9, 9, 26
  vadduwm 9, 9, 2
  vshasigmaw 25, 11, 0, 0
  vadduwm 10, 10, 25
  vshasigmaw 26, 8, 0, 15
  vadduwm 10, 10, 26
  vadduwm 10, 10, 3
  vshasigmaw 25, 12, 0, 0
  vadduwm 11, 11, 25
  vshasigmaw 26, 9, 0, 15
  vadduwm 11, 11, 26
  vadduwm 11, 11, 4
  vshasigmaw 25, 13, 0, 0
  vadduwm 12, 12, 25
  vshasigmaw 26, 10, 0, 15
  vadduwm 12, 12, 26
  vadduwm 12, 12, 5
  vadduwm 23, 23, 8
  vsel 25, 22, 21, 20
  vadduwm 22, 22, 24
  vadduwm 23, 23, 25
  vshasigmaw 26, 20, 1, 15
  vadduwm 23, 23, 26
  vxor 25, 16, 17
  vsel 25, 17, 18, 25
  vadduwm 19, 19, 23
  vshasigmaw 26, 16, 1, 0
  vadduwm 26, 26, 25
  vadduwm 23, 23, 26
  vsldoi 24, 24, 24, 4
  vadduwm 22, 22, 9
  vsel 25, 21, 20, 19
  vadduwm 21, 21, 24
  vadduwm 22, 22, 25
  vshasigmaw 26, 19, 1, 15
  vadduwm 22, 22, 26
  vxor 25, 23, 16
  vsel 25, 16, 17, 25
  vadduwm 18, 18, 22
  vshasigmaw 26, 23, 1, 0
  vadduwm 26, 26, 25
  vadduwm 22, 22, 26
  vsldoi 24, 24, 24, 4
  vadduwm 21, 21, 10
  vsel 25, 20, 19, 18
  vadduwm 20, 20, 24
  vadduwm 21, 21, 25
  vshasigmaw 26, 18, 1, 15
  vadduwm 21, 21, 26
  vxor 25, 22, 23
  vsel 25, 23, 16, 25
  vadduwm 17, 17, 21
  vshasigmaw 26, 22, 1, 0
  vadduwm 26, 26, 25
  vadduwm 21, 21, 26
  lxvw4x 32+24, 0, 6
  addi 6, 6, 16
  vadduwm 20, 20, 11
  vsel 25, 19, 18, 17
  vadduwm 19, 19, 24
  vadduwm 20, 20, 25
  vshasigmaw 26, 17, 1, 15
  vadduwm 20, 20, 26
  vxor 25, 21, 22
  vsel 25, 22, 23, 25
  vadduwm 16, 16, 20
  vshasigmaw 26, 21, 1, 0
  vadduwm 26, 26, 25
  vadduwm 20, 20, 26
  vsldoi 24, 24, 24, 4
  vshasigmaw 25, 14, 0, 0
  vadduwm 13, 13, 25
  vshasigmaw 26, 11, 0, 15
  vadduwm 13, 13, 26
  vadduwm 13, 13, 6
  vshasigmaw 25, 15, 0, 0
  vadduwm 14, 14, 25
  vshasigmaw 26, 12, 0, 15
  vadduwm 14, 14, 26
  vadduwm 14, 14, 7
  vshasigmaw 25, 0, 0, 0
  vadduwm 15, 15, 25
  vshasigmaw 26, 13, 0, 15
  vadduwm 15, 15, 26
  vadduwm 15, 15, 8
  vadduwm 19, 19, 12
  vsel 25, 18, 17, 16
  vadduwm 18, 18, 24
  vadduwm 19, 19, 25
  vshasigmaw 26, 16, 1, 15
  vadduwm 19, 19, 26
  vxor 25, 20, 21
  vsel 25, 21, 22, 25
  vadduwm 23, 23, 19
  vshasigmaw 26, 20, 1, 0
  vadduwm 26, 26, 25
  vadduwm 19, 19, 26
  vsldoi 24, 24, 24, 4
  vadduwm 18, 18, 13
  vsel 25, 17, 16, 23
  vadduwm 17, 17, 24
  vadduwm 18, 18, 25
  vshasigmaw 26, 23, 1, 15
  vadduwm 18, 18, 26
  vxor 25, 19, 20
  vsel 25, 20, 21, 25
  vadduwm 22, 22, 18
  vshasigmaw 26, 19, 1, 0
  vadduwm 26, 26, 25
  vadduwm 18, 18, 26
  vsldoi 24, 24, 24, 4
  vadduwm 17, 17, 14
  vsel 25, 16, 23, 22
  vadduwm 16, 16, 24
  vadduwm 17, 17, 25
  vshasigmaw 26, 22, 1, 15
  vadduwm 17, 17, 26
  vxor 25, 18, 19
  vsel 25, 19, 20, 25
  vadduwm 21, 21, 17
  vshasigmaw 26, 18, 1, 0
  vadduwm 26, 26, 25
  vadduwm 17, 17, 26
  vadduwm 16, 16, 15
  vsel 25, 23, 22, 21
  vadduwm 16, 16, 25
  vshasigmaw 26, 21, 1, 15
  vadduwm 16, 16, 26
  vxor 25, 17, 18
  vsel 25, 18, 19, 25
  vadduwm 20, 20, 16
  vshasigmaw 26, 17, 1, 0
  vadduwm 26, 26, 25
  vadduwm 16, 16, 26
  subi 6, 6, 256
  vsldoi 0, 28, 28, 8
  vsldoi 1, 29, 29, 8
  vsldoi 2, 30, 30, 8
  vsldoi 3, 31, 31, 8
  vadduwm 16, 16, 28
  vadduwm 17, 17, 29
  vadduwm 18, 18, 30
  vadduwm 19, 19, 31
  vadduwm 20, 20, 0
  vadduwm 21, 21, 1
  vadduwm 22, 22, 2
  vadduwm 23, 23, 3
  subi 5, 5, 1
.align 4
L1:
  cmpldi 5, 0
  bgt L0
  vmrghw 16, 16, 17
  vmrghw 18, 18, 19
  xxmrghd 32+16, 32+16, 32+18
  vmrghw 20, 20, 21
  vmrghw 22, 22, 23
  xxmrghd 32+20, 32+20, 32+22
  li 10, 16
  stxvw4x 32+16, 0, 3
  stxvw4x 32+20, 10, 3
  lxv 32+20, 0(1)
  lxv 32+21, 16(1)
  lxv 32+22, 32(1)
  lxv 32+23, 48(1)
  lxv 32+24, 64(1)
  lxv 32+25, 80(1)
  lxv 32+26, 96(1)
  lxv 32+28, 112(1)
  lxv 32+29, 128(1)
  lxv 32+30, 144(1)
  lxv 32+31, 160(1)
  addi 1, 1, 176

  blr

.section .note.GNU-stack,"",%progbits
