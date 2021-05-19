# 实验报告 实验二 PB19000071彭怡腾

## 1、简要介绍

本shell基于rust编写，需要在**linux**环境下编译，编译时请按照如下结构组织Cargo.toml文件和main.rs文件：

```
.
|-  src                     // 放置源文件的目录
    |- main.rs			    // 源文件
|-  Cargo.toml              // Cargo的配置文件
```

编译时，只需要在Cargo.toml目录下运行

```
cargo build
```

即可。编译出的shell程序将位于./target/debug目录下，在此目录下运行

```
./shell
```

即可运行shell程序。

## 2、功能实现

#### 0、可用性

shell会先输出一个“Shell>>”提示等待用户输入，方便观察哪些是输入部分，哪些是输出部分。同时，在处理无效输入和空行时，不会直接退出而是会重新等待下一次输入，正如我们平常使用的shell一样。

![img](./img/use.png)

#### 1、支持管道

shell实现了支持单个管道符和多个管道符的功能，如下图所示：

![img](./img/pipe.png)

在rust中使用如下语句将输入的字符串进行分割，并执行每一条指令：

```rust
    let mut cmds = cmds.trim().split("|").peekable();
    let mut pre_cmd:Option<Child> = None;
```

同时使用如下的语句将输入输出源重定向，使得我们的shell可以支持管道功能：

```rust
                        let stdin = pre_cmd.map_or(Stdio::inherit(),|output:Child| Stdio::from(output.stdout.unwrap()));
                        let stdout = if cmds.peek().is_none() {Stdio::inherit()} else {Stdio::piped()};
```

#### 2、支持重定向

shell支持了重定向功能，如下图所示：

![img](./img/redi.png)

使用如下代码（以“>>”的为例）完成重定向功能，首先判断是否含有“>>”,“>”,“<”，如果有的话，将该命令分割，前面的应为命令，后面的应为文件，然后使用dup()函数储存当前的输入或输出的设备描述符，以在本次重定向之后还原为默认状态。使用open函数和dup2()函数配合实现文本的重定向，将输入或输出源更改为指定的文件。

```rust
        if cmd.contains(">>"){
            let mut redi_input = cmd.trim().split(">>").peekable();
            if let Some(input_cmd) = redi_input.next(){
                args = input_cmd.split_whitespace();
                prog = args.next();
            }
            else{
                println!("You Lose The Command!");
                continue;
            }
            if let Some(output_file) = redi_input.next(){
                unsafe{
                    let c_str = CString::new(output_file.trim()).unwrap();
                    let c_word: *const c_char = c_str.as_ptr() as *const c_char;
                    fd = open(c_word,O_WRONLY| O_CREAT |O_APPEND, 00777);
                    stdout_num = dup(1);
                    dup2(fd,1);
                    index = 1;
                };
            }
            else{
                println!("You Lose The Output_File!");
                continue;
            }
        }
```

#### 3、处理ctrl+D

shell可以处理ctrl+D的按键，如下图所示：

![img](./img/ctrld.png)

因为ctrl+D实际上为输入EOF，因此当判断读入一个EOF（不是换行符）时，就退出程序，代码如下：

```rust
        //handle ctrl+D
        if 0 == stdin().read_line(&mut cmds).unwrap() {
            println!("");
            exit(0);	// EOF
        }
```



#### 4、处理ctrl+C

shell可以处理ctrl+C的按键，如下图所示：

![img](./img/ctrlc.png)

在输入一半命令的时候输入ctrl+C就会丢弃当前命令进入下一行，而在有程序运行时输入ctrl+C则会打断程序开始新的一行。

由于ctrl+C为输入SIGINT信号，因此使用如下语句注册一个handler函数，对SIGINT信号做简要处理即可。

```rust
let _status = signal::signal(signal::Signal::SIGINT, signal::SigHandler::Handler(handle_sigint));

extern fn handle_sigint(_signal: c_int) {
    println!("#");
    if unsafe{PROMPT_INDEX == 0}{
        print!("Shell>> ");
    }
    stdout().flush().unwrap();
}
```

#### 5、选做部分

实现了两个在实验要求中位于“更多功能”部分的选做。

###### 1、echo $SHELL

这一个命令的含义是，若$号后面的为一个环境变量，则将其输出。如下图所示，本shell实现了这个功能，可以在$号后为环境变量的时候输出对应环境变量的值。

![img](./img/envecho.png)

对应的代码略有冗余（主要的冗余是类型转换和使其支持管道），简单来说就是使用函数getenv()判断$号后面的内容是否存在于环境变量之中，若存在则输出其环境变量对应的值，否则使用command()函数调用原来的echo，直接对后面的部分进行输出。

```rust
//echo can print the environment value
             "echo" => {
                    let mut echo_index = 0;
					let env_para = args.clone().next();
                        if let Some(para) = env_para{
                            let mut para_list = para.trim().split("$").peekable();
                            if let Some(temp_text) = para_list.next(){
                                if temp_text == ""{
                                    if let Some(last_text) = para_list.next() {
                                        if last_text != "" {
                                            echo_index = 1;
                                            
                                            //change the stdin and stdout if have pipe
                                            let stdin = pre_cmd.map_or(Stdio::inherit(),|output:Child| Stdio::from(output.stdout.unwrap()));
                                            let stdout = if cmds.peek().is_none() {Stdio::inherit()} else {Stdio::piped()};
                                            
                                            let c_str = CString::new(last_text.trim()).unwrap();
                                            let c_word: *const c_char = c_str.as_ptr() as *const c_char;

                                            unsafe{
                                                let env_value = getenv(c_word);
                                                let mut str_slice: &str  = para;

                                                if env_value != (0x0 as *mut i8){
                                                    let c_str: &CStr = CStr::from_ptr(env_value);
                                                    str_slice = c_str.to_str().unwrap();
                                                }
                                                
                                                let child_status = Command::new("echo").arg(str_slice).stdin(stdin).stdout(stdout).spawn();
                                                match child_status{
                                                    Ok(child_process)=>{
                                                        pre_cmd = Some(child_process);
                                                    }
                                                    Err(err)=>{
                                                        pre_cmd = None;
                                                        eprintln!("{}", err);//not die with wrong input
                                                    }
                                                } 
                                            };                                                
                                        }
                                    }
                                }
                            }
                        }
```

###### 2、A=1 env

这一个命令的含义是，将tabel = value中的tabel临时的添加到环境变量中，如果原变量存在则直接将其覆盖，并且输出当前的环境变量表。如下图所示，本shell实现了这个功能：

![img](./img/setenv1.png)

中间省略大量环境变量……

![img](./img/setenv2.png)

同时我们还可以使用上面实现的功能来相互印证正确性。

![img](./img/echoset.png)

对应的代码如下所示，简要来说就是判断是不是以“env”结尾，是的话，则把前面的部分按照“=”划分，调用setenv()函数添加新的环境变量即可。

```rust
        //setenv handle after the redirection, also need support pipe
        let end_env = args.clone().last();
        if let Some(end_parm) = end_env{
            if end_parm == "env"{
                let dict_env = prog.clone();
                if let Some(dict_parm) = dict_env{
                    let mut env_list = dict_parm.trim().split("=").peekable();
                    let name = env_list.next();
                    let value = env_list.next();
                    if let Some(env_name) = name{
                        if let Some(env_value) = value{
                            let c_str = CString::new(env_name.trim()).unwrap();
                            let c_name: *const c_char = c_str.as_ptr() as *const c_char;
                            let c_str = CString::new(env_value.trim()).unwrap();
                            let c_value: *const c_char = c_str.as_ptr() as *const c_char;
                            unsafe{setenv(c_name,c_value,1);};
                            let empty_iter = "";
                            args = empty_iter.split_whitespace();
                            prog = Some("env");
                        }
                    }
                }
            }
        }
```

## 3、strace工具

我们使用strace工具追踪ls命令

```bash
strace ls
```

执行上述命令后的，显示的内容如下所示：

```bash
execve("/usr/bin/ls", ["ls"], 0x7ffce82c0c50 /* 51 vars */) = 0
brk(NULL)                               = 0x564162ef4000
arch_prctl(0x3001 /* ARCH_??? */, 0x7fff7c1c4320) = -1 EINVAL (无效的参数)
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (没有那个文件或目录)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=70363, ...}) = 0
mmap(NULL, 70363, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f23808cf000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libselinux.so.1", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0@p\0\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0644, st_size=163200, ...}) = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f23808cd000
mmap(NULL, 174600, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f23808a2000
mprotect(0x7f23808a8000, 135168, PROT_NONE) = 0
mmap(0x7f23808a8000, 102400, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x6000) = 0x7f23808a8000
mmap(0x7f23808c1000, 28672, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1f000) = 0x7f23808c1000
mmap(0x7f23808c9000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x26000) = 0x7f23808c9000
mmap(0x7f23808cb000, 6664, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f23808cb000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\360q\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
pread64(3, "\4\0\0\0\20\0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0", 32, 848) = 32
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\t\233\222%\274\260\320\31\331\326\10\204\276X>\263"..., 68, 880) = 68
fstat(3, {st_mode=S_IFREG|0755, st_size=2029224, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
pread64(3, "\4\0\0\0\20\0\0\0\5\0\0\0GNU\0\2\0\0\300\4\0\0\0\3\0\0\0\0\0\0\0", 32, 848) = 32
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\t\233\222%\274\260\320\31\331\326\10\204\276X>\263"..., 68, 880) = 68
mmap(NULL, 2036952, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f23806b0000
mprotect(0x7f23806d5000, 1847296, PROT_NONE) = 0
mmap(0x7f23806d5000, 1540096, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x25000) = 0x7f23806d5000
mmap(0x7f238084d000, 303104, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x19d000) = 0x7f238084d000
mmap(0x7f2380898000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1e7000) = 0x7f2380898000
mmap(0x7f238089e000, 13528, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f238089e000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libpcre2-8.so.0", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\340\"\0\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0644, st_size=584392, ...}) = 0
mmap(NULL, 586536, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f2380620000
mmap(0x7f2380622000, 409600, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x2000) = 0x7f2380622000
mmap(0x7f2380686000, 163840, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x66000) = 0x7f2380686000
mmap(0x7f23806ae000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x8d000) = 0x7f23806ae000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libdl.so.2", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0 \22\0\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0644, st_size=18816, ...}) = 0
mmap(NULL, 20752, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f238061a000
mmap(0x7f238061b000, 8192, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1000) = 0x7f238061b000
mmap(0x7f238061d000, 4096, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x3000) = 0x7f238061d000
mmap(0x7f238061e000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x3000) = 0x7f238061e000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libpthread.so.0", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\201\0\0\0\0\0\0"..., 832) = 832
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\345Ga\367\265T\320\374\301V)Yf]\223\337"..., 68, 824) = 68
fstat(3, {st_mode=S_IFREG|0755, st_size=157224, ...}) = 0
pread64(3, "\4\0\0\0\24\0\0\0\3\0\0\0GNU\0\345Ga\367\265T\320\374\301V)Yf]\223\337"..., 68, 824) = 68
mmap(NULL, 140408, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f23805f7000
mmap(0x7f23805fe000, 69632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x7000) = 0x7f23805fe000
mmap(0x7f238060f000, 20480, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x18000) = 0x7f238060f000
mmap(0x7f2380614000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1c000) = 0x7f2380614000
mmap(0x7f2380616000, 13432, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f2380616000
close(3)                                = 0
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f23805f5000
arch_prctl(ARCH_SET_FS, 0x7f23805f6400) = 0
mprotect(0x7f2380898000, 12288, PROT_READ) = 0
mprotect(0x7f2380614000, 4096, PROT_READ) = 0
mprotect(0x7f238061e000, 4096, PROT_READ) = 0
mprotect(0x7f23806ae000, 4096, PROT_READ) = 0
mprotect(0x7f23808c9000, 4096, PROT_READ) = 0
mprotect(0x5641629aa000, 4096, PROT_READ) = 0
mprotect(0x7f238090e000, 4096, PROT_READ) = 0
munmap(0x7f23808cf000, 70363)           = 0
set_tid_address(0x7f23805f66d0)         = 4137
set_robust_list(0x7f23805f66e0, 24)     = 0
rt_sigaction(SIGRTMIN, {sa_handler=0x7f23805febf0, sa_mask=[], sa_flags=SA_RESTORER|SA_SIGINFO, sa_restorer=0x7f238060c3c0}, NULL, 8) = 0
rt_sigaction(SIGRT_1, {sa_handler=0x7f23805fec90, sa_mask=[], sa_flags=SA_RESTORER|SA_RESTART|SA_SIGINFO, sa_restorer=0x7f238060c3c0}, NULL, 8) = 0
rt_sigprocmask(SIG_UNBLOCK, [RTMIN RT_1], NULL, 8) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
statfs("/sys/fs/selinux", 0x7fff7c1c4270) = -1 ENOENT (没有那个文件或目录)
statfs("/selinux", 0x7fff7c1c4270)      = -1 ENOENT (没有那个文件或目录)
brk(NULL)                               = 0x564162ef4000
brk(0x564162f15000)                     = 0x564162f15000
openat(AT_FDCWD, "/proc/filesystems", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0444, st_size=0, ...}) = 0
read(3, "nodev\tsysfs\nnodev\ttmpfs\nnodev\tbd"..., 1024) = 378
read(3, "", 1024)                       = 0
close(3)                                = 0
access("/etc/selinux/config", F_OK)     = -1 ENOENT (没有那个文件或目录)
openat(AT_FDCWD, "/usr/lib/locale/locale-archive", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=14537584, ...}) = 0
mmap(NULL, 14537584, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f237f817000
close(3)                                = 0
ioctl(1, TCGETS, {B38400 opost isig icanon echo ...}) = 0
ioctl(1, TIOCGWINSZ, {ws_row=24, ws_col=80, ws_xpixel=0, ws_ypixel=0}) = 0
openat(AT_FDCWD, ".", O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_DIRECTORY) = 3
fstat(3, {st_mode=S_IFDIR|0775, st_size=4096, ...}) = 0
getdents64(3, /* 11 entries */, 32768)  = 328
getdents64(3, /* 0 entries */, 32768)   = 0
close(3)                                = 0
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0), ...}) = 0
write(1, "build  deps  examples  increment"..., 61build  deps  examples  incremental  redi.txt  shell  shell.d
) = 61
close(1)                                = 0
close(2)                                = 0
exit_group(0)                           = ?
+++ exited with 0 +++
```

以下分析shell中没有实现的三个系统调用：execve，fstat，mmap。

通过查看Linux内置文档：

```bash
man 2 系统调用名称
```

得知上述三个系统调用的作用如下：

###### 1、execve

int execve(const char *pathname, char *const argv[], char *const envp[]);

它的作用是运行另外一个指定的程序，程序由pathname指定。它会把新的程序加载到当前进程的内存空间中，并用新的堆，栈，段数据去覆盖原有进程的对应部分。因此它在此处的功能就是去执行”/usr/bin/“的ls程序，并传入["ls"]和0x7ffce82c0c50 /* 51 vars */作为参数。

###### 2、fstat

int fstat(int fd, struct stat *statbuf);

它的作用是将文件的信息返回到statbuf所指的结构体中，要检索的文件信息由文件描述符fd指定。因此诸如fstat(3, {st_mode=S_IFDIR|0775, st_size=4096, ...})的系统调用的功能应该就是获得文件描述符”3“对应的文件信息。

###### 3、mmap

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

它为调用它的进程在虚拟地址中创建一个新的映射，将fd指定的文件映射到进程的虚拟内存空间，通过对这段内存的读取和修改，来实现对文件的读取和修改,而不需要再调用read，write等操作。 新映射的起始地址在addr中指定，如果为NULL，则由系统指定起始地址，，Length参数指定映射的长度(必须大于0)，prot描述了期待的映射保护模式，flags描述了映射区在更新时是否写回底层文件，及更新对其他进程是否可见等特性，offset规定了以文件开始处的偏移量，如果为0则代表从文件头开始映射。因此mmap(0x7f238060f000, 20480, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x18000)的意思应该是将文件描述符为3的文件，从偏移量为0x18000的位置映射到0x7f238060f000的虚拟地址，映射的文件长度为20480，映射区的保护模式是映射区只可被读取，映射区的特性是不写回原文件，并且必须映射到addr指定的地址（MAP_FIXED）。