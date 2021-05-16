use std::io::{stdin, stdout, Write};
use std::ffi::{CString,CStr};
use std::env;
use std::process::{Child, exit, Command, Stdio};
use nix::sys::{signal};
use libc::*;

static mut PROMPT_INDEX:i32 = 0;

extern fn handle_sigint(_signal: c_int) {
    println!("#");
    if unsafe{PROMPT_INDEX == 0}{
        print!("Shell>> ");
    }
    stdout().flush().unwrap();
}

fn main() -> ! {

    unsafe {
        let _status = signal::signal(signal::Signal::SIGINT, signal::SigHandler::Handler(handle_sigint));
    };

    let mut index = 0;
    let mut fd = -1;
    let mut stdin_num = -1;
    let mut stdout_num= -1;

    loop {
        if index==1{
            unsafe{dup2(stdout_num,1);close(fd);};
            index = 0;
        }
        else if index == 2{
            unsafe{dup2(stdin_num,0);close(fd);};
            index = 0;
        }

        print!("Shell>> ");
        stdout().flush().unwrap();

        unsafe{PROMPT_INDEX = 0;};

        let mut cmds = String::new();

        //handle ctrl+D
        if 0 == stdin().read_line(&mut cmds).unwrap() {
            println!("");
            exit(0);	// EOF
        }

        unsafe{PROMPT_INDEX = 1;};

        let mut cmds = cmds.trim().split("|").peekable();
        let mut pre_cmd:Option<Child> = None;
        
        while let Some(cmd) = cmds.next() {
            
            //To emilate the warning and error,The true use of these parameters are under this
            let mut args = cmd.split_whitespace();
            let mut prog = args.next();
            if prog.is_none(){
                continue;
            }

            //redirection
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
                        fd = open(c_word,O_RDWR| O_CREAT |O_APPEND, 77777);
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
            else if cmd.contains(">"){
                let mut redi_input = cmd.trim().split(">").peekable();
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
                        fd = open(c_word, O_RDWR | O_CREAT | O_TRUNC, 77777);
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
            else if cmd.contains("<"){
                let mut redi_output = cmd.trim().split("<").peekable();
                if let Some(input_cmd) = redi_output.next(){
                    args = input_cmd.split_whitespace();
                    prog = args.next();
                }
                else{
                    println!("You Lose The Command!");
                    continue;
                }
                if let Some(input_file) = redi_output.next(){
                    unsafe{
                        let c_str = CString::new(input_file.trim()).unwrap();
                        let c_word: *const c_char = c_str.as_ptr() as *const c_char;
                        fd = open(c_word, O_RDONLY);
                        stdin_num = dup(0);
                        dup2(fd,0);
                        index = 2;
                    };
                }
                else{
                    println!("You Lose The Input_file!");
                    continue;
                }
            }   
            else{
                args = cmd.split_whitespace();
                prog = args.next();
            }     

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


            match prog {
                None => {
                    continue;
                },
                Some(prog) => {
                    match prog {
                        "cd" => {
                            if let Some(dir) = args.next(){
                                env::set_current_dir(dir).expect("Changing current dir failed");
                            }
                            else{
                                env::set_current_dir("/").expect("Changing current dir failed");
                            }
                        }
                        "pwd" => {
                            let err = "Getting current dir failed";
                            println!("{}", env::current_dir().expect(err).to_str().expect(err));
                        }
                        "export" => {
                            for arg in args {
                                let mut assign = arg.split("=");
                                let name = assign.next().expect("No variable name");
                                let value = assign.next().expect("No variable value");
                                env::set_var(name, value);
                            }
                        }
                        "exit" => {
                            exit(0);
                        }
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

                            if echo_index == 0{
                                //change the stdin and stdout if have pipe
                                let stdin = pre_cmd.map_or(Stdio::inherit(),|output:Child| Stdio::from(output.stdout.unwrap()));
                                let stdout = if cmds.peek().is_none() {Stdio::inherit()} else {Stdio::piped()};
                                let child_status = Command::new(prog).args(args).stdin(stdin).stdout(stdout).spawn();
                                match child_status{
                                    Ok(child_process)=>{
                                        pre_cmd = Some(child_process);
                                    }
                                    Err(err)=>{
                                        pre_cmd = None;
                                        eprintln!("{}", err);//not die with wrong input
                                    }
                                }   
                            }   
                        }
                        _ => {
                            //change the stdin and stdout if have pipe
                            let stdin = pre_cmd.map_or(Stdio::inherit(),|output:Child| Stdio::from(output.stdout.unwrap()));
                            let stdout = if cmds.peek().is_none() {Stdio::inherit()} else {Stdio::piped()};

                            let child_status = Command::new(prog).args(args).stdin(stdin).stdout(stdout).spawn();
                            match child_status{
                                Ok(child_process)=>{
                                    pre_cmd = Some(child_process);
                                }
                                Err(err)=>{
                                    pre_cmd = None;
                                    eprintln!("{}", err);//not die with wrong input
                                }
                            }
                        }
                    }
                }
            }
        }

        if let Some(mut fin_cmd) = pre_cmd {
            match fin_cmd.wait(){
                Ok(_status) => continue,
                Err(_err) => continue,
            }
        }
    }
}