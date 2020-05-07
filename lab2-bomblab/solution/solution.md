# Bomb Lab
## Introduction
In this lab we will use *gdb* to to execute, debug and analyze the assembly code. With *gdb* we can add break points and execute the assembly code step by step, just as we normally do with an IDE. And we can also check the values of registers, addresses, etc. However, most of us might not have experience with *gdb* before. I found CMU's recitation for bomb lab (I referred to [17 fall](http://www.cs.cmu.edu/afs/cs/academic/class/15213-f17/www/recitations/recitation03-bomblab.pdf), or you can view the [latest](https://www.cs.cmu.edu/~213/recitations/recitation04-bomblab.pdf) as well) pretty useful. Besides, [two-page x86-64 GDB cheat sheet](http://csapp.cs.cmu.edu/3e/docs/gdbnotes-x86-64.pdf) is also helpful.  

Here are some gdb commands that I used a lot in this lab:
- `gdb <file>`
- `quit`
- `run`
- `break phase_1`
- `break *0x123456`
- `stepi`
- `continue`
- `disas`
- `print /x $rsp`
- `print (char *) 0x123456`
- `x/w $0x7ffffffffdde0`
- `info registers`

Here's the steps to defuse a bomb:
1. use command `gdb bomb` to start gdb
2. set break point to a specific phase (from `phase_1` to `phase_6`)
3. use `run` to start the program
4. type known correct string to pass defused phases
5. type a random string to get into the procedure
6. analyze the underlying logic by inspecting instructions and registers (main work here)
7. get the correct string

I will keep notes on sixth step for each phase.

## Phase 1

Here is the assembly code of `phase_1`.

```assembly
Dump of assembler code for function phase_1:
   0x0000000000400ee0 <+0>:     sub    $0x8,%rsp
   0x0000000000400ee4 <+4>:     mov    $0x402400,%esi
   0x0000000000400ee9 <+9>:     callq  0x401338 <strings_not_equal>
   0x0000000000400eee <+14>:    test   %eax,%eax
   0x0000000000400ef0 <+16>:    je     0x400ef7 <phase_1+23>
   0x0000000000400ef2 <+18>:    callq  0x40143a <explode_bomb>
   0x0000000000400ef7 <+23>:    add    $0x8,%rsp
   0x0000000000400efb <+27>:    retq
```

The first phase is relatively simple.  

There is a magical number `$0x402400` on the second line. We can use `print (char* ) $0x402400` to get the value. It's a string and it is passed to `strings_not_equal` function as the second argument.   

(If interested, we can check the assembly code for `strings_not_equal` as well. It is a function to check whether two strings are the same. It invokes another function `string_length` to get the length of string.)  

Now the corresponding C code is straight-forward, if we don't consider the details of `strings_not_equal` and `string_length`:

```c
void phase_1(char* input) {
		char* correct = "Border relations with Canada have never been better.";
  	if (string_not_equal(input, correct)) {
      	explode_bomb();
    }
}
```

So there we go, the first answer is the string stored at address `$0x402400`, which is `Border relations with Canada have never been better.`.

## Phase 2

```assembly
Dump of assembler code for function phase_2:
   0x0000000000400efc <+0>:     push   %rbp
   0x0000000000400efd <+1>:     push   %rbx
   0x0000000000400efe <+2>:     sub    $0x28,%rsp
   0x0000000000400f02 <+6>:     mov    %rsp,%rsi
   0x0000000000400f05 <+9>:     callq  0x40145c <read_six_numbers>
   0x0000000000400f0a <+14>:    cmpl   $0x1,(%rsp)
   0x0000000000400f0e <+18>:    je     0x400f30 <phase_2+52>
   0x0000000000400f10 <+20>:    callq  0x40143a <explode_bomb>
   0x0000000000400f15 <+25>:    jmp    0x400f30 <phase_2+52>
   0x0000000000400f17 <+27>:    mov    -0x4(%rbx),%eax
   0x0000000000400f1a <+30>:    add    %eax,%eax
   0x0000000000400f1c <+32>:    cmp    %eax,(%rbx)
   0x0000000000400f1e <+34>:    je     0x400f25 <phase_2+41>
   0x0000000000400f20 <+36>:    callq  0x40143a <explode_bomb>
   0x0000000000400f25 <+41>:    add    $0x4,%rbx
   0x0000000000400f29 <+45>:    cmp    %rbp,%rbx
   0x0000000000400f2c <+48>:    jne    0x400f17 <phase_2+27>
   0x0000000000400f2e <+50>:    jmp    0x400f3c <phase_2+64>
   0x0000000000400f30 <+52>:    lea    0x4(%rsp),%rbx
   0x0000000000400f35 <+57>:    lea    0x18(%rsp),%rbp
   0x0000000000400f3a <+62>:    jmp    0x400f17 <phase_2+27>
   0x0000000000400f3c <+64>:    add    $0x28,%rsp
   0x0000000000400f40 <+68>:    pop    %rbx
   0x0000000000400f41 <+69>:    pop    %rbp
   0x0000000000400f42 <+70>:    retq   
```

```assembly
Dump of assembler code for function read_six_numbers:
   0x000000000040145c <+0>:     sub    $0x18,%rsp
   0x0000000000401460 <+4>:     mov    %rsi,%rdx
   0x0000000000401463 <+7>:     lea    0x4(%rsi),%rcx
   0x0000000000401467 <+11>:    lea    0x14(%rsi),%rax
   0x000000000040146b <+15>:    mov    %rax,0x8(%rsp)
   0x0000000000401470 <+20>:    lea    0x10(%rsi),%rax
   0x0000000000401474 <+24>:    mov    %rax,(%rsp)
   0x0000000000401478 <+28>:    lea    0xc(%rsi),%r9
   0x000000000040147c <+32>:    lea    0x8(%rsi),%r8
   0x0000000000401480 <+36>:    mov    $0x4025c3,%esi
   0x0000000000401485 <+41>:    mov    $0x0,%eax
   0x000000000040148a <+46>:    callq  0x400bf0 <__isoc99_sscanf@plt>
   0x000000000040148f <+51>:    cmp    $0x5,%eax
   0x0000000000401492 <+54>:    jg     0x401499 <read_six_numbers+61>
   0x0000000000401494 <+56>:    callq  0x40143a <explode_bomb>
   0x0000000000401499 <+61>:    add    $0x18,%rsp
   0x000000000040149d <+65>:    retq  
```

The second phase is much more complex, especially it involves six integer pointers. I spend a lot of time checking registers and addresses. I'd suggest using pen and paper to track the change of values around `$rsp` (those are addresses/pointers) and values of those addresses.  

The simplified logic of C code is something like this:

```c
void phase_2(char* input) {
		int numbers[6];
    read_six_numbers(input, numbers);
		if (numbers[0] != 1) {
      	explode_bomb();
    }
    for (int i = 0; i < 5; i++) {
        if (numbers[i + 1] != 2 * numbers[i]) {
            explode_bomb();
        }
    }
}

void read_six_numbers(char* input, int* p) {
    char* format = "%d %d %d %d %d %d";
    int* p1 = p;
    int* p2 = p + 1;
    int* p3 = p + 2;
    int* p4 = p + 3;
    int* p5 = p + 4;
    int* p6 = p + 5;
    int numbers = 0;
    numbers = mystery_scanf(input, format, p1, p2, p3, p4, p5, p6);
    // input should match the format and the mystery function
    // would store the values in the given pointers,
    // it should return 6, which means the input contains 6 numbers
    if (numbers <= 5) {
        explode_bomb();
    }
}
```

Therefore, the input should be `1 2 4 8 16 32`.