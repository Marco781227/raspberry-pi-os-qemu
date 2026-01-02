#include "user_sys.h"
#include "user.h"
#include "printf.h"

void loop(char* str)
{
	char buf[8] = {""};
  for (int i = 0; i < 8; i++){
    buf[i] = str[i];
  }
	while (1){
    call_sys_write(buf);
    user_delay(1000000);
	}
}

void create_and_loop(char* str, char* arg)
{
	char buf[25] = {""};
  for (int i = 0; i < 25; i++){
    buf[i] = str[i];
  }
	call_sys_write(buf);
	int pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("Error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0)
		loop(arg);

  call_sys_write("Process over\n\r");
  call_sys_exit();
}

void user_process()
{
	call_sys_write("User process\n\r");
  // First child
	call_sys_write("Creating process 1\n\r");
	int pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("Error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0)
		create_and_loop("Child 1 creating child\n\r","azazazaz");

  // Second child
	call_sys_write("Creating process 2\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("Error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0)
		create_and_loop("Child 2 creating child\n\r","erererer");

  // Third child
	call_sys_write("Creating process 3\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		create_and_loop("Child 3 creating child\n\r","tytytyty");
	}

  // Third child
	call_sys_write("Creating process 4\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		create_and_loop("Child 4 creating child\n\r","uiuiuiui");
	}

  // Fourth child
	call_sys_write("Creating process 5\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		create_and_loop("Child 5 creating child\n\r","opopopop");
	}

  // Fifth child
	call_sys_write("Creating process 6\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		create_and_loop("Child 6 creating child\n\r","qsqsqsqs");
	}

  // Parent execution
  create_and_loop("Parent creating child \n\r", "dfdfdfdf");
}

