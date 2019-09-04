Homework 0
----------

### Watch the videos and write up your answers to the following questions

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

<http://cs-education.github.io/sys/>

Questions? Comments? Use Piazza: <https://piazza.com/illinois/fall2019/cs241>

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, **so copy your code to a separate document.** The post-video challenges are not part of homework 0 but you learn the most by doing rather than just passively watching - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Please use this document to write the answers. This will be hand graded.

### Chapter 1

In which our intrepid hero battles standard out, standard error, file descriptors and writing to files

1.  **Hello, World! (system call style)** Write a program that uses `write()` to print out “Hi! My name is &lt;Your Name&gt;”.

```c
write(1, “Hi! My name is Anastasia”, 24)
```

2.  **Hello, Standard Error Stream!** Write a function to print out a triangle of height `n` to standard error. Your function should have the signature `void write_triangle(int n)` and should use `write()`. The triangle should look like this, for n = 3:

```
*
**
***
```

```c
// Your code here
void write_triangle(int n) {
	int lineCnt;
	for(lineCnt = 0; lineCnt < n; lineCnt++) {
		int count;
		for(count = 0; count <= lineCnt; count++) {
			write(STDERR_FILENO, "*", 1);
		}
		
		write(STDERR_FILENO, "\n", 1);
	}
}
```



3.  **Writing to files** Take your program from “Hello, World!” modify it write to a file called `hello_world.txt`. Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).

```c
// Your code here
mode_t mode = S_IRUSR | S_IWUSR;
int hello_file = open("hello_world.txt", O_CREAT | O_RDWR, mode);
char * message = "Hi! My name is Anastasia";
write(hello_file, message, 24);
close(hello_file);
``` 


4. **Not everything is a system call** Take your program from “Writing to files” and replace `write()` with `printf()`. *Make sure to print to the file instead of standard out!*

```c
	close(1);
	mode_t fileMode = S_IRUSR | S_IWUSR;
	int file = open("hello_world.txt", O_CREAT | O_RDWR, fileMode);
	printf("Hi! My name is Anastasia");
	close(file);
```



5.  What are some differences between `write()` and `printf()`?

```c
Differences between write and printf:
Write is a system call, printf is not
Printf allows for substitution of given values into a template, whereas write does not
Write allows for direct control over the destination of the string, whereas printf prints to whatever stream is marked STDOUT
```

### Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

1.  How many bits are there in a byte?

```c
As many bits as the computer’s architecture treats as a distinct group for memory addressing  purposes, usually 8 in modern machines
```
As many bits as the computer’s architecture treats as a distinct group for memory addressing  purposes, usually 8 in modern machines

2.  How many bytes are there in a `char`?
1 byte
```c
1
```

3.  How many bytes the following are on your machine? 

* `int`: 4
* `double`: 8 
* `float`: 4
* `long`: 8
* `long long`: 16 

4.  On a machine with 8 byte integers, the declaration for the variable `data` is `int data[8]`. If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?

```c
data + 2 = 0x7fbd9d40 + 16
```
data + 2 = 0x7fbd9d40 + 16

5.  What is `data[3]` equivalent to in C? Hint: what does C convert `data[3]` to before dereferencing the address? Remember, the type of a string constant `abc` is an array.

```c
data[3] = &(data + 3(8))
```
data + 2 = 0x7fbd9d40 + 16

6.  Why does this segfault?

```c
char *ptr = "hello";
*ptr = 'J';
```

The character array is read-only after initialization, hence segfault when attempting to modify it

7.  What does `sizeof("Hello\0World")` return?


```c
5
```

8.  What does `strlen("Hello\0World")` return?

```c
4
```


9.  Give an example of X such that `sizeof(X)` is 3.

```c
char * X = “hi”
```
10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.

```c
int Y = 3
```

### Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

1.  What are two ways to find the length of `argv`?

	Ways to find length of argv:
		calculate sizeof(argv) / sizeof(argv[0])
		check the value of argc

2.  What does `argv[0]` represent?
		the name of the program's executable
3.  Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?
	in the data segment
4.  On a machine where pointers are 8 bytes, and with the following code:

    ``` c
    char *ptr = "Hello";
    char array[] = "Hello";
    ```
	
    What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?

```c
sizeof(ptr) is 8, because the argument is a pointer which has size 8 bytes on the given machine. sizeof(array) is 6, because it has 5 characters (1 byte each)  plus the null terminator.

```

5.  What data structure manages the lifetime of automatic variables?
	 a stack
### Chapter 4

Heap and stack memory, and working with structs

1.  If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?
	put it on the heap, using malloc
2.  What are the differences between heap and stack memory?
	Stack memory is allocated and freed automatically, but heap memory must be managed manually. The lifetime of variables created on the stack (automatic variables) finishes at the end of the function that created them, whereas heap memory remains allocated (and the data is secure against being overwritten) until it is explicitly freed.
3.  Are there other kinds of memory in a process?
	Yes. In addition to the stack and heap, processes will also have memory for environment variables, code, globals, and constants.
4.  Fill in the blank: “In a good C program, for every malloc, there is a \_\_\_”.
	free
5.  What is one reason `malloc` can fail?
	Not enough free memory remains on the heap to allocate more
6.  What are some differences between `time()` and `ctime()`?
	time() is a system call that returns the time in a special time_t format; it takes as an argument a pointer to a time_t variable to store the result in. ctime() is not a system call, takes the memory address of a time_t variable as its only argument, and returns the time in a human-readable string format (pointer to char array). 
7.  What is wrong with this code snippet?

``` c
free(ptr);
free(ptr);
```
The code attempts to free the same memory twice.

8.  What is wrong with this code snippet?

``` c
free(ptr);
printf("%s\n", ptr);
```
The code attempts to access data on the heap after the memory has already been freed
9.  How can one avoid the previous two mistakes?
To avoid problems from freeing/accessing invalid memory, set the value of the pointer that initially pointed to the freed memory to be 'NULL'.

10. Use the following space for the next four questions

```c
// 10
struct Person {
	char * name;
	int age;
	struct (Person*)[] * friends;
};

typedef struct Person person_t;
// 12
	person_t * create(char * name, int age) {
	person_t * person = (person_t*)malloc(sizeof(person_t));
	person->name = strdup(name);
	person->age = age;
	person->friends = malloc(sizeof(person_t*) * 10);
	
	return person;
}

// 13

void destroy(person_t * person) {
	free(person->name);
	free(person->friends);
	memset(person, 0, sizeof(person_t));
	free(person);
	
}

int main() {
// 11
	char * smith_name = "Agent Smith";
	char * moore_name = "Sonny Moore";
	
	person_t * Agent_Smith = create(smith_name, 128);
	person_t * Sonny_Moore = create(moore_name, 256);
	
	
	destroy(Agent_Smith);
	destroy(Sonny_Moore);
	return 0;
}
```

* Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s). 

*  Now, make two persons on the heap, “Agent Smith” and “Sonny Moore”, who are 128 and 256 years old respectively and are friends with each other. Create functions to create and destroy a Person (Person’s and their names should live on the heap).

* `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).

* `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.


### Chapter 5

Text input and output and parsing using `getchar`, `gets`, and `getline`.

1.  What functions can be used for getting characters from `stdin` and writing them to `stdout`?
	* gets
	* getchar
	* puts
	* printf
2.  Name one issue with `gets()`.
	If gets() reads more than 12 bytes, the memory allocated to its buffer will be insufficient and the buffer will overflow into parts of memory used for storing variables	 
3.  Write code that parses the string “Hello 5 World” and initializes 3 variables to “Hello”, 5, and “World”.


```c

char * input = "Hello 5 World";

char buffer[10] = "string1";
int num = 0;
char buffer_2[10] = "string2";

sscanf(input, "%s %d %s", buffer, & num, buffer_2);
```

4.  What does one need to define before including `getline()`?

5.  Write a C program to print out the content of a file line-by-line using `getline()`.

```c
// Your code here
```

### C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here


1.  What compiler flag is used to generate a debug build?
-g

2.  You fix a problem in the Makefile and type `make` again. Explain why this may be insufficient to generate a new build.


3.  Are tabs or spaces used to indent the commands after the rule in a Makefile?
	tabs

4.  What does `git commit` do? What’s a `sha` in the context of git?
	git commit creates a "snapshot" of the repository's state at the time of commit.
	the "sha" is the unique ID given to each commit

5.  What does `git log` show you?
	git log lists commits to the repository with their SHA and commit message, in reverse chronological order.

6.  What does `git status` tell you and how would the contents of `.gitignore` change its output?
	

7.  What does `git push` do? Why is it not just sufficient to commit with `git commit -m ’fixed all bugs’ `?
	git push copies the local commits to the specified repository. Since local commits are not automatically sent to the remote repository, making a commit on the local repository will not change the state of files on the remote repository.
8.  What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?
	Pushes would be rejected with the non-fast-forward error if the remote repository is 'ahead' of the local repo by some number of other commits. The most common method to resolve this is to do a git pull, resolve any conflicts, and then try pushing again. 

### Optional: Just for fun

-   Convert your a song lyrics into System Programming and C code covered in this wiki book and share on Piazza.

-   Find, in your opinion, the best and worst C code on the web and post the link to Piazza.

-   Write a short C program with a deliberate subtle C bug and post it on Piazza to see if others can spot your bug.

-   Do you have any cool/disastrous system programming bugs you’ve heard about? Feel free to share with your peers and the course staff on piazza.
