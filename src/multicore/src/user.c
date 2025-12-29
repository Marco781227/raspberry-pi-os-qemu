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
		loop("azazazaz");

  // Second child
	call_sys_write("Creating process 2\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("Error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0)
		loop("erererer");

  // Third child
	call_sys_write("Creating process 3\n\r");
	pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("error during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		loop("tytytyty");
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
		loop("uiuiuiui");
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
		loop("opopopop");
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
		loop("qsqsqsqs");
	}

  // Parent execution
  loop("dfdfdfdf");
}

