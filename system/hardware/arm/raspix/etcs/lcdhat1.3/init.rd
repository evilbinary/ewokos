/bin/rundev /drivers/raspix/uartd          /dev/tty0
/bin/rundev /drivers/raspix/lcdhatd        /dev/fb0 
/bin/rundev /drivers/raspix/hat13_joykeybd /dev/keyb0
#/bin/rundev /drivers/raspix/hat13_joystickd /dev/joystick

/bin/rundev /drivers/fontd                 /dev/font /usr/system/fonts/system.ttf
/bin/rundev /drivers/consoled              /dev/console0

/bin/rundev /drivers/timerd                /dev/timer
/bin/rundev /drivers/nulld                 /dev/null
/bin/rundev /drivers/ramfsd                /tmp
/bin/rundev /drivers/proc/sysinfod         /proc/sysinfo
/bin/rundev /drivers/proc/stated           /proc/state

/bin/rundev /drivers/displayd              /dev/display /dev/fb0
/bin/rundev /drivers/xserverd              /dev/x

#/bin/rundev /drivers/xconsoled             /dev/console0

@/sbin/x/xim_none /dev/keyb0 esc_home&
#@/sbin/x/xjoystickd /dev/joystick&
@/sbin/x/xim_vkey &

@/bin/x/launcher &
@/bin/session &