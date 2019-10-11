# Homework 0 Feedback

### Summary
**Net ID**: tdavis52  
**Total score**: 72/92 (78.26%)

### Comment
```
2.3 long long is usually 8 bytes
2.5 *(data+3)
2.6 12
2.7 5
3.1 Check argc or iterate through argv until NULL is found
3.3 env is stored above the stack
4.11 Forgot to set friends on each other
4.10 should be **friends
```

### Score breakdown
**1.1 Write a program that uses write() to print out "Hi! My name is ".** `2/2`  
**1.2 Write a program that uses write() to print out a triangle of height n to Standard Error** `2/2`  
**1.3 Take your program from "Hello World" and have it write to a file called "hello world.txt" (without the quotes).** `2/2`  
**1.5 Take your program from "Writing to files" and replace it with printf()** `2/2`  
**1.6 Name some differences from write() and printf()** `2/2`  
**2.1 How many bits are there in a byte?** `2/2`  
**2.2 How many bytes is a char?** `2/2`  
**2.3 Tell me how many bytes the following are on your machine: int, double, float, long, long long** `1/2`  
**2.4. On a machine with 8 byte integers (refer to code snippet below)** `2/2`  
**2.5 What is data[3] equivalent to in C?** `0/2`  
**2.6 Why does this segfault (refer to code snippet below)?** `2/2`  
**2.7 What does sizeof("Hello\0World") return?** `0/2`  
**2.8 What does strlen("Hello\0World") return?** `0/2`  
**2.9 Give an example of X such that sizeof(X) is 3** `2/2`  
**2.10 Give an example of Y such at sizeof(Y) might be 4 or 8 depending on the machine.** `2/2`  
**3.1 Name me two ways to find the length of argv** `1/2`  
**3.2 What is argv[0]** `2/2`  
**3.3 Where are the pointers to environment variables stored?** `0/2`  
**3.4 On a machine where pointers are 8 bytes (refer to the code snippet)** `2/2`  
**3.5 What datastucture is managing the lifetime of automatic variables?** `2/2`  
**4.1 If I want to use data after the lifetime of the function it was created in, then where should I put it and how do I put it there?** `2/2`  
**4.2 What are the differences between heap and stack memory?** `2/2`  
**4.3 Are there other kinds of memory in a process?** `2/2`  
**4.4 Fill in the blank. In a good C program: "For every malloc there is a ___".** `2/2`  
**4.5 Name one reason malloc can fail.** `2/2`  
**4.6 Name some differences between time() and ctime()** `2/2`  
**4.7 What is wrong with this code snippet?** `2/2`  
**4.8 What is wrong with this code snippet?** `2/2`  
**4.9 How can one avoid the previous 2 mistakes?** `2/2`  
**4.10 Create a struct that represents a Person and typedef, so that "struct Person" can be replaced with a single word. A person should contain the following information: name, age, friends (pointer to an array of pointers to People).** `1/2`  
**4.11 Now make two people "Agent Smith" and "Sonny Moore" on the heap who are 128 and 256 years old respectively and are friends with each other.** `1/2`  
**4.12 `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).** `2/2`  
**4.13 `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.** `2/2`  
**5.1. What functions can be used for getting characters for stdin and writing them to stdout?** `2/2`  
**5.2 Name one issue with gets()** `2/2`  
**5.3 Write code that parses a the string "Hello 5 World" and initializes 3 variables to ("Hello", 5, "World") respectively.** `2/2`  
**5.4 What does one need to define before using getline()?** `0/2`  
**5.5 Write a C program to print out the content of a file line by line using getline()** `0/2`  
**6.1 What compiler flag is used to generate a debug build?** `2/2`  
**6.2 You modify the Makefile to generate debug builds and type make again. Explain why this is insufficient to generate a new build.** `0/2`  
**6.3 Are tabs or spaces used in Makefiles?** `2/2`  
**6.4 What does `svn commit` do? What's a revision number?** `2/2`  
**6.5. What does `svn log` show you?** `2/2`  
**6.6 What does git status tell you and how would the contents of .gitignore change its output?** `0/2`  
**6.7 What does git push do? Why is it not just sufficient to commit with git commit -m 'fixed all bugs'?** `2/2`  
**6.8 What does a non-fast-forward error git push reject mean? What is the most common way of dealing with this?** `2/2`