#include "MacWM.h"
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	MacWM xwm;
	xwm.readConfig("/etc/x/xwm_mac1984.conf");	
	xwm.run();
	return 0;
}
