/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */

#endif
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int evenBits(void) {
int y=(0x55<<24)|(0x55<<16)|(0x55<<8)|0x55;/*the number is 01010101010101.....32 bit number*/
  return y;
}
/* 
 * isEqual - return 1 if x == y, and 0 otherwise 
 *   Examples: isEqual(5,5) = 1, isEqual(4,5) = 0
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int isEqual(int x, int y) {
int k =x^y;/*use XOR property to see if the numbers are same*/
return !k;
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
int nb=n<<3;/*find the number of bits that has to be moved as n is in bytes*/
int mb=m<<3;/*find the number of bits that has to be moved as m is in bytes*/
int b=0xff;
int nbx=(b<<nb)&x;/*to create a mask to extract the nb bits from x*/
int mbx=(b<<mb)&x;/*to create a mask to extract the mb bits from x*/
int notchb=(~((b<<nb)|(b<<mb)))&x;/*finding the part of x that is not changing*/
nbx=(nbx>>nb)&b;
mbx=(mbx>>mb)&b;
/*shifting the bits to their new positions*/
nbx=nbx<<mb;
mbx=mbx<<nb;
b=notchb|nbx|mbx;/*appending the 3 parts to get the whole number whose bytes are now swapped*/ 
return b;
}
/* 
 * rotateRight - Rotate x to the right by n
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateRight(0x87654321,4) = 0x76543218
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 3 
 */
int rotateRight(int x, int n) {
int b=(x)<<(32+~n+1);/*left shift the number by 32-n to get n bits that has to be shifted right*/
int a=x>>(n);/*shifting the number by n to accomodate the shifted bits*/
int k = (1<<(32+~n+1))+~1+1;/*to handle the case when the MSB of x is 1*/
a = a & k;
x=a|b;/*combining the last bits and the shifted number to get the rotated number*/ 
return x;
}
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
int m=(x>>16)|x;/* uses the ability of shifts to compute powers of 2 */
m=(m>>8)|m;
m=(m>>4)|m;
m=(m>>2)|m;
m=(m>>1)|m;
m=(m^1)&1;/*putting all values as 1 bit */
return m;
}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
int y=~(8<<28);/*tmax=10000000 in 2s complement*/
return y;
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
int negr = (x>>31);/*all ones if a number is negetive*/
int posr = (x>>31) + (!!x);/*it becomes 1 if a number is positive*/
return (negr | posr);
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
int m=(~(x^y)) & (y+~x+1);/*check if y-x and msb of x and y are of opposite sign */
int n=(y& (x^y));/*opposite case when x>y*/ 
int mn=(m|n)>>31;
return !!mn;
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
int r=x+(~y+1);/* subtract (x-y)*/
int over=(x&~y&~r)|(~x&y&r);/*overflow only occures on subtracting 2 different sign numbers*/
int overmsb=(over>>31)&1;/*find the msb to see if overflow occures*/
  return overmsb^1;
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          maximum possible value, and when negative overflow occurs,
 *          it returns minimum positive value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {
int povflo, novflo, r;
int min=1<<31;/*finding the minimum number that can be represented*/
int max=~min;/*finding the maximum number that can be represented*/
int signx=x>>31;/*finding sign of x*/
int signy=y>>31;/*finding sign of y*/
int sum=x+y;
int signsum=sum>>31;
int ovflo=~(signx^signy);/*overflow only occures on subtracting 2 different sign numbers*/
ovflo=(signx^signsum) & ovflo;
povflo=ovflo & (~(signx^0));/*If the sum of two positive numbers yields a negative result, the sum has overflowed*/
novflo=~povflo;/*If the sum of two negative numbers yields a positive result, the sum has overflowed*/
r=(povflo & max)|(novflo & min);/*to make it return max/min depending what kind of overflow occures*/ 
r=r & ovflo;
r=r|(~ovflo&sum);
return r;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
int mask=x>>31;/*find if x is positive or negetive*/
int m1,m2,m3,m4,m5,m6,count;
int mk1=(1<<31)>>15;/*mask to extract the first 16 bits*/
int mk2=(0xff<<8);/*mask to extract the first 8 bits of that 16 bits*/
int mk3=0xf0;/*mask to extract the first 4 bits of that 8 bits*/
int mk4=0x0c;/*mask to extract the first 2 bits of that 4 bits*/
int mk5=0x02;/*mask to extract the first 1 bit of that 2 bits*/
int mk6=0x01;/*mask to extract the first 1 bit*/
x=(mask & ~x)|(~mask & x);/*make x such that it is always a positive number with a leading 0*/
m1=((!!(x& (mk1)))<<31)>>31;/*implementing divide and conqure algorithm*/
x=((x>>16)&m1)|(x&~m1);
m2=((!!(x& (mk2)))<<31)>>31;
x=((x>>8)&m2)|(x&~m2);
m3=((!!(x& (mk3)))<<31)>>31;
x=((x>>4)&m3)|(x&~m3);
m4=((!!(x& (mk4)))<<31)>>31;
x=((x>>2)&m4)|(x&~m4);
m5=((!!(x& (mk5)))<<31)>>31;
x=((x>>1)&m5)|(x&~m5);
m6=((!!(x& (mk6)))<<31)>>31;
x=((x>>1) & m6)|(x & ~m6);
count=1;
count=count+(16 & m1)+(8 & m2)+(4 & m3)+(2 & m4)+(1 & m5)+(1 & m6);

 return count;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
int y;
int f=(0x007fffff)&uf;/*mask out he fractional part of the argument*/
int e=(0x7f800000)&uf;/*mask out he exponent part of the argument*/
int E=(uf>>23)&0xff;
int s=(0x80000000)&uf;/*mask out he sign part of the argument*/
if (!(e^0x7f800000)){ 
	return uf;/*if e=127 then return the number*/
	}
if (!e) {
	return s|((!((f&3)^3)+f)>>1);/*if e=0 then round the fractional part and then right shift the fractional part*/
	}	
if ((e>>23) == 1) {
	y=f>>1;
	y=y|(1<<22);/*to handle the overflow case when e=1 and f=all ones*/
	y=y+!((f&3)^3);
        return s|y;
        }  
return s|((E-1)<<23)|f ;/*all other cases we subtract 1 from E to divide the number by 2*/ 
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
int f=(0x007fffff)&uf;
int e=(0x7f800000)&uf;
int e8bit=e>>23;/*change exponent to 8 bit number*/
int E=e8bit-127;/*general formula for E for normalized numbers*/
int num=0;
int s=(0x80000000)&uf;
        if (E<0){
                return 0;/*rounds down to zero if there is no left shift*/
                }
        if ((E>=0)&&(E<31))
        {
        if (E<=23)
                num= f>>(23-E);/*shift left if shift > 23*/
        else if (E>23)
                num=f<<(E-23);/*else shift right*/

        }
        num=num+(1<<E);/*normalized, append a 1 to the left of the frac*/
if (E>31){
        return 0x80000000u; /*return if out of range*/
        }
if (s){
        return -num;/*Change the sign if s=1 i.e its negative number*/
        }
return num;
}
